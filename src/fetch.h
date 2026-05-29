#pragma once
#include <string>

struct UsageData {
    std::string plan;

    int  block_pct        = 0;
    int  block_reset_mins = 0;

    int  weekly_pct         = 0;
    int  weekly_reset_mins  = 0;
    std::string weekly_reset_abs;   /* "Tue 21:08" local time, for the 7-day block */

    int  battery = -1;              /* Kindle battery %, -1 if unknown */

    std::string updated;            /* "HH:MM" local time of this fetch */
    bool ok = false;
    bool auth_error = false;        /* session key missing/invalid -> user must act */
    std::string error;
};

struct Config {
    std::string session_key;  /* claude.ai sessionKey cookie */
    int         refresh_mins = 5;
    std::string tz;           /* POSIX TZ string, e.g. "UTC-7" (Saigon) */
};

Config    load_config(const char* path);
UsageData fetch_usage(const Config& cfg);
