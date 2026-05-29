#pragma once

extern "C" {
#include "lvgl.h"
}

#include "fetch.h"

void ui_create(lv_display_t* disp);
void ui_update(const UsageData& d, const Config& cfg);
