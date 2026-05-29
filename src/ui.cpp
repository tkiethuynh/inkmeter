#include "ui.h"
#include <cstdio>
#include <string>

static lv_obj_t* g_plan_label     = nullptr;
static lv_obj_t* g_block_pct      = nullptr;
static lv_obj_t* g_block_bar      = nullptr;
static lv_obj_t* g_block_sub      = nullptr;
static lv_obj_t* g_week_pct       = nullptr;
static lv_obj_t* g_week_bar       = nullptr;
static lv_obj_t* g_week_sub       = nullptr;
static lv_obj_t* g_status_label   = nullptr;
static lv_obj_t* g_time_label     = nullptr;

static std::string fmt_mins(int m) {
    if (m <= 0)  return "resetting soon";
    if (m < 60)  { char b[32]; snprintf(b,sizeof(b),"%dm",m); return b; }
    char b[32]; snprintf(b,sizeof(b),"%dh %dm", m/60, m%60); return b;
}

static lv_obj_t* label(lv_obj_t* p, const lv_font_t* f, lv_color_t c, lv_text_align_t a) {
    lv_obj_t* l = lv_label_create(p);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, c, 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(l, a, 0);
    return l;
}

static lv_obj_t* sep(lv_obj_t* p) {
    lv_obj_t* l = lv_obj_create(p);
    lv_obj_set_size(l, lv_pct(90), 2);
    lv_obj_set_style_bg_color(l, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_border_width(l, 0, 0);
    lv_obj_set_style_radius(l, 0, 0);
    lv_obj_set_style_pad_all(l, 0, 0);
    return l;
}

/* Usage block: heading + big % + bar + subtitle. Returns bar widget. */
static lv_obj_t* usage_section(lv_obj_t* p, const char* title,
                                lv_obj_t** pct_out, lv_obj_t** sub_out) {
    /* section title */
    lv_obj_t* h = label(p, &lv_font_montserrat_24, lv_color_black(), LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(h, title);
    lv_obj_set_width(h, lv_pct(90));

    /* large percentage number */
    *pct_out = label(p, &lv_font_montserrat_48, lv_color_black(), LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(*pct_out, "0%");
    lv_obj_set_width(*pct_out, lv_pct(90));

    /* bar — white track with a black outline so the full 100% length is always
       visible on e-ink (a light grey fill is invisible on this display), and a
       solid black fill for the used portion. */
    lv_obj_t* bar = lv_bar_create(p);
    lv_obj_set_width(bar, lv_pct(85));
    lv_obj_set_height(bar, 28);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_set_style_border_color(bar, lv_color_black(), 0);
    lv_obj_set_style_border_width(bar, 3, 0);
    lv_obj_set_style_border_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, lv_color_black(), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 6, 0);
    lv_obj_set_style_radius(bar, 6, LV_PART_INDICATOR);

    /* subtitle: "resets in Xh Ym" */
    *sub_out = label(p, &lv_font_montserrat_24, lv_color_black(), LV_TEXT_ALIGN_LEFT);
    lv_label_set_text(*sub_out, "");
    lv_obj_set_width(*sub_out, lv_pct(90));

    return bar;
}

void ui_create(lv_display_t*) {
    lv_obj_t* scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(scr, 56, 0);
    lv_obj_set_style_pad_bottom(scr, 40, 0);
    lv_obj_set_style_pad_row(scr, 22, 0);

    /* Title */
    lv_obj_t* t = label(scr, &lv_font_montserrat_48, lv_color_black(), LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(t, "Claude Usage");
    lv_obj_set_width(t, lv_pct(100));

    /* Plan */
    g_plan_label = label(scr, &lv_font_montserrat_24, lv_color_black(), LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(g_plan_label, "Pro Plan");
    lv_obj_set_width(g_plan_label, lv_pct(100));

    sep(scr);

    /* 5-hour block */
    g_block_bar = usage_section(scr, "CURRENT BLOCK  (5 hours)", &g_block_pct, &g_block_sub);

    sep(scr);

    /* Weekly */
    g_week_bar = usage_section(scr, "THIS WEEK  (7 days)", &g_week_pct, &g_week_sub);

    sep(scr);

    /* Status / updated */
    g_status_label = label(scr, &lv_font_montserrat_24, lv_color_black(), LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(g_status_label, "Connecting...");
    lv_obj_set_width(g_status_label, lv_pct(100));

    g_time_label = label(scr, &lv_font_montserrat_24, lv_color_black(), LV_TEXT_ALIGN_CENTER);
    lv_label_set_text(g_time_label, "");
    lv_obj_set_width(g_time_label, lv_pct(100));
}

void ui_update(const UsageData& d, const Config&) {
    if (!d.ok) {
        /* No fresh data. Show why, and don't pass off 0% as real numbers. */
        lv_label_set_text(g_block_pct, "--");
        lv_label_set_text(g_week_pct,  "--");
        lv_bar_set_value(g_block_bar, 0, LV_ANIM_OFF);
        lv_bar_set_value(g_week_bar,  0, LV_ANIM_OFF);
        lv_label_set_text(g_block_sub, "");
        lv_label_set_text(g_week_sub,  "");
        std::string msg = d.error;
        if (d.auth_error) msg += " - update sessionKey in config";
        lv_label_set_text(g_status_label, msg.c_str());
        lv_label_set_text(g_time_label, "");
        return;
    }

    if (!d.plan.empty()) {
        std::string p = d.plan + " Plan";
        lv_label_set_text(g_plan_label, p.c_str());
    }

    /* Block section — relative ("resets in 2h 8m") */
    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", d.block_pct);
    lv_label_set_text(g_block_pct, buf);
    lv_bar_set_value(g_block_bar, d.block_pct, LV_ANIM_OFF);
    std::string bsub = "resets in " + fmt_mins(d.block_reset_mins);
    lv_label_set_text(g_block_sub, bsub.c_str());

    /* Weekly section — absolute day+time ("resets Wed 21:08") */
    snprintf(buf, sizeof(buf), "%d%%", d.weekly_pct);
    lv_label_set_text(g_week_pct, buf);
    lv_bar_set_value(g_week_bar, d.weekly_pct, LV_ANIM_OFF);
    std::string wsub = d.weekly_reset_abs.empty()
        ? "resets in " + fmt_mins(d.weekly_reset_mins)
        : "resets " + d.weekly_reset_abs;
    lv_label_set_text(g_week_sub, wsub.c_str());

    /* Footer: last-updated */
    lv_label_set_text(g_status_label, "");
    std::string upd = "Updated " + d.updated;
    lv_label_set_text(g_time_label, upd.c_str());
}
