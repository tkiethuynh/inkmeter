#pragma once
#include <linux/ioctl.h>
#include <stdint.h>

/* Kindle PaperWhite e-ink controller (Freescale EPDC) ioctls */

struct mxcfb_rect {
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
};

struct mxcfb_alt_buffer_data {
    uint32_t phys_addr;
    uint32_t width;
    uint32_t height;
    struct mxcfb_rect alt_update_region;
};

struct mxcfb_update_data {
    struct mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int      temp;
    unsigned int flags;
    int      dither_mode;
    int      quant_bit;
    struct mxcfb_alt_buffer_data alt_buffer_data;
};

#define WAVEFORM_MODE_AUTO   257
#define WAVEFORM_MODE_GC16     2  /* full quality, slow */
#define WAVEFORM_MODE_GL16     3  /* ghosting visible, medium */
#define WAVEFORM_MODE_A2       6  /* fast, black & white only */
#define UPDATE_MODE_PARTIAL  0x0
#define UPDATE_MODE_FULL     0x1
#define TEMP_USE_AMBIENT     0x1001

struct mxcfb_update_marker_data {
    uint32_t update_marker;
    uint32_t collision_test;
};

#define MXCFB_SEND_UPDATE             _IOW ('F', 0x2E, struct mxcfb_update_data)
#define MXCFB_WAIT_FOR_UPDATE_COMPLETE _IOWR('F', 0x2F, struct mxcfb_update_marker_data)
