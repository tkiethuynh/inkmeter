#include "fetch.h"
#include <cstdio>
#include <ctime>
#include <fstream>
#include <string>

static std::string run(const std::string& cmd) {
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return {};
    std::string out;
    char buf[4096];
    while (fgets(buf, sizeof(buf), fp) && out.size() < 65536)
        out += buf;
    pclose(fp);
    return out;
}

static std::string jstr(const std::string& j, const std::string& key) {
    auto needle = "\"" + key + "\":";
    auto pos = j.find(needle);
    if (pos == std::string::npos) return {};
    pos += needle.size();
    while (pos < j.size() && j[pos] == ' ') pos++;
    if (j[pos] == '"') {
        auto e = j.find('"', pos + 1);
        return e == std::string::npos ? "" : j.substr(pos + 1, e - pos - 1);
    }
    auto e = j.find_first_of(",}\n", pos);
    return j.substr(pos, e - pos);
}

static double jdbl(const std::string& j, const std::string& key) {
    auto s = jstr(j, key);
    if (s.empty()) return 0;
    try { return std::stod(s); } catch (...) { return 0; }
}

/* Format an ISO-8601 UTC timestamp as local "Wed 21:08" (day-of-week + time). */
static std::string fmt_abs(const std::string& iso) {
    if (iso.empty()) return {};
    int Y, M, D, h, m, s;
    if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &Y, &M, &D, &h, &m, &s) != 6)
        return {};
    struct tm t{};
    t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D;
    t.tm_hour = h; t.tm_min = m; t.tm_sec = s;
    time_t e = timegm(&t);
    char b[32];
    strftime(b, sizeof(b), "%a %H:%M", localtime(&e));
    return b;
}

static int mins_until(const std::string& iso) {
    if (iso.empty()) return 0;
    int Y, M, D, h, m, s;
    if (sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &Y, &M, &D, &h, &m, &s) != 6)
        return 0;
    struct tm t{};
    t.tm_year = Y - 1900; t.tm_mon = M - 1; t.tm_mday = D;
    t.tm_hour = h; t.tm_min = m; t.tm_sec = s;
    time_t target = timegm(&t);
    time_t now    = time(nullptr);
    int delta     = (int)((target - now) / 60);
    return delta > 0 ? delta : 0;
}

Config load_config(const char* path) {
    Config cfg;
    std::ifstream f(path);
    if (!f.is_open()) return cfg;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto k = line.substr(0, eq);
        auto v = line.substr(eq + 1);
        if (k == "SESSION_KEY")  cfg.session_key = v;
        if (k == "REFRESH_MINS") cfg.refresh_mins = std::stoi(v);
    }
    return cfg;
}

static std::string claude_get(const std::string& url, const std::string& sk) {
    return run(
        "curl -sf --max-time 20 "
        "-H 'Cookie: sessionKey=" + sk + "' "
        "-H 'User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
              "(KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36' "
        "-H 'Accept: application/json' "
        "-H 'Referer: https://claude.ai/' "
        "'" + url + "'"
    );
}

UsageData fetch_usage(const Config& cfg) {
    UsageData d;

    if (cfg.session_key.empty()) {
        d.error = "No session key set"; d.auth_error = true; return d;
    }

    /* Step 1: get org UUID */
    auto orgs = claude_get("https://claude.ai/api/organizations", cfg.session_key);
    if (orgs.empty()) { d.error = "Fetch failed"; return d; }   /* transient/network */

    auto org_uuid = jstr(orgs, "uuid");
    if (org_uuid.empty()) {
        d.error = "Session expired"; d.auth_error = true; return d;
    }

    /* Plan from capabilities */
    if      (orgs.find("\"claude_max\"") != std::string::npos) d.plan = "Max";
    else if (orgs.find("\"claude_pro\"") != std::string::npos) d.plan = "Pro";
    else                                                         d.plan = "Free";

    /* Step 2: get usage */
    auto usage = claude_get(
        "https://claude.ai/api/organizations/" + org_uuid + "/usage",
        cfg.session_key);
    if (usage.empty()) { d.error = "Usage fetch failed"; return d; }

    /* five_hour block */
    auto fh_pos = usage.find("\"five_hour\":");
    if (fh_pos != std::string::npos) {
        auto fh = usage.substr(fh_pos);
        d.block_pct        = (int)jdbl(fh, "utilization");
        d.block_reset_mins = mins_until(jstr(fh, "resets_at"));
    }

    /* seven_day window */
    auto sd_pos = usage.find("\"seven_day\":");
    if (sd_pos != std::string::npos) {
        auto sd = usage.substr(sd_pos);
        d.weekly_pct        = (int)jdbl(sd, "utilization");
        auto sd_reset       = jstr(sd, "resets_at");
        d.weekly_reset_mins = mins_until(sd_reset);
        d.weekly_reset_abs  = fmt_abs(sd_reset);   /* absolute "Wed 21:08" */
    }

    time_t now = time(nullptr);
    char buf[8];
    strftime(buf, sizeof(buf), "%H:%M", localtime(&now));
    d.updated = buf;
    d.ok = true;
    return d;
}
