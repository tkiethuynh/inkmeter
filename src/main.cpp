#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

extern "C" {
#include "lvgl.h"
}

#include "fetch.h"
#include "mxcfb.h"
#include "ui.h"

/* One-shot renderer: clear screen, fetch usage, draw it, exit.
   All power management (frontlight, suspend, RTC wake, touch handling)
   is owned by dashboard.sh — this binary only paints the screen. */

static int      g_fb_fd     = -1;
static uint8_t* g_fb_mem    = nullptr;
static uint32_t g_fb_stride = 0;
static uint32_t g_fb_w      = 0;
static uint32_t g_fb_h      = 0;
static uint32_t g_marker    = 0;

/* Send e-ink refresh and block until the controller finishes. */
static void eink_update(int x1, int y1, int x2, int y2, bool full) {
    g_marker++;
    struct mxcfb_update_data u{};
    u.update_region.top    = (uint32_t)y1;
    u.update_region.left   = (uint32_t)x1;
    u.update_region.width  = (uint32_t)(x2 - x1 + 1);
    u.update_region.height = (uint32_t)(y2 - y1 + 1);
    u.waveform_mode        = full ? WAVEFORM_MODE_GC16 : WAVEFORM_MODE_AUTO;
    u.update_mode          = full ? UPDATE_MODE_FULL   : UPDATE_MODE_PARTIAL;
    u.update_marker        = g_marker;
    u.temp                 = TEMP_USE_AMBIENT;
    ioctl(g_fb_fd, MXCFB_SEND_UPDATE, &u);

    struct mxcfb_update_marker_data md{ g_marker, 0 };
    ioctl(g_fb_fd, MXCFB_WAIT_FOR_UPDATE_COMPLETE, &md);
}

/* LVGL flush: convert RGB565 tile to 8-bit grayscale into the framebuffer.
   The full-screen GC16 fires once afterwards in flush_screen(). */
static void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    if (!g_fb_mem) { lv_display_flush_ready(disp); return; }

    const uint16_t* src = (const uint16_t*)px_map;
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    for (int32_t row = 0; row < h; row++) {
        uint8_t* dst = g_fb_mem + (area->y1 + row) * g_fb_stride + area->x1;
        for (int32_t col = 0; col < w; col++) {
            uint16_t c = src[row * w + col];
            uint8_t r  = ((c >> 11) & 0x1F) * 255 / 31;
            uint8_t g2 = ((c >>  5) & 0x3F) * 255 / 63;
            uint8_t b  = ( c        & 0x1F) * 255 / 31;
            dst[col]   = (uint8_t)((r * 77 + g2 * 150 + b * 29) >> 8);
        }
    }
    lv_display_flush_ready(disp);
}

/* Pump LVGL until all dirty regions are written, then one full GC16 refresh. */
static void flush_screen() {
    lv_refr_now(nullptr);
    for (int i = 0; i < 8; i++) { lv_timer_handler(); usleep(5000); }
    eink_update(0, 0, (int)g_fb_w - 1, (int)g_fb_h - 1, true);
}

static lv_display_t* init_display(const char* fbdev, bool ghost_clear) {
    g_fb_fd = open(fbdev, O_RDWR);
    if (g_fb_fd < 0) { perror("open framebuffer"); return nullptr; }

    struct fb_var_screeninfo vinfo{};
    struct fb_fix_screeninfo finfo{};
    ioctl(g_fb_fd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(g_fb_fd, FBIOGET_FSCREENINFO, &finfo);

    g_fb_w      = vinfo.xres;
    g_fb_h      = vinfo.yres;
    g_fb_stride = finfo.line_length;

    g_fb_mem = (uint8_t*)mmap(nullptr, g_fb_stride * g_fb_h,
                               PROT_READ | PROT_WRITE, MAP_SHARED, g_fb_fd, 0);
    if (g_fb_mem == MAP_FAILED) { perror("mmap framebuffer"); return nullptr; }

    /* Ghost-clear (black GC16 → white GC16) cycles every cell through its full
       voltage range, erasing the dense Kindle home screen. Only needed on the
       first render; later refreshes already sit on our own sparse screen, so we
       skip these two full GC16 refreshes to save power and avoid a flash. */
    if (ghost_clear) {
        memset(g_fb_mem, 0x00, g_fb_stride * g_fb_h);
        eink_update(0, 0, (int)g_fb_w - 1, (int)g_fb_h - 1, true);
        memset(g_fb_mem, 0xFF, g_fb_stride * g_fb_h);
        eink_update(0, 0, (int)g_fb_w - 1, (int)g_fb_h - 1, true);
    }

    lv_display_t* disp = lv_display_create(g_fb_w, g_fb_h);
    size_t buf_bytes = g_fb_w * 32 * sizeof(uint16_t);
    lv_display_set_buffers(disp, malloc(buf_bytes), nullptr,
                           buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);
    return disp;
}

static uint32_t tick_cb() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int main(int argc, char* argv[]) {
    setenv("TZ", "UTC-7", 1);  /* Saigon UTC+7 for the "Updated HH:MM" line */
    tzset();

    const char* config_path = "/mnt/us/extensions/ai-usage/config";
    const char* fbdev_path  = "/dev/fb0";
    bool ghost_clear = false;   /* pass --clear on the first render only */
    if (argc > 1) config_path = argv[1];
    if (argc > 2) fbdev_path  = argv[2];
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "--clear") == 0) ghost_clear = true;

    Config cfg = load_config(config_path);

    lv_init();
    lv_tick_set_cb(tick_cb);

    lv_display_t* disp = init_display(fbdev_path, ghost_clear);
    if (!disp) return 1;

    ui_create(disp);                /* build widgets (not drawn yet) */

    /* Fetch first, then draw the final frame once — avoids a "Connecting..."
       flash on every wake. Screen stays blank-white during the brief fetch.
       Retry a few times: right after a resume the WiFi link often reports an IP
       before it actually carries traffic, so the first curl can fail. */
    UsageData d;
    for (int attempt = 0; attempt < 4; attempt++) {
        d = fetch_usage(cfg);
        if (d.ok) break;
        sleep(3);
    }

    /* Repaint when we have fresh data, or an auth error the user must act on, or
       on the first launch. On a transient network failure keep the last good
       frame (don't overwrite good data with a passing glitch). */
    if (d.ok || d.auth_error || ghost_clear) {
        ui_update(d, cfg);
        flush_screen();
    }

    return d.ok ? 0 : 2;
}
