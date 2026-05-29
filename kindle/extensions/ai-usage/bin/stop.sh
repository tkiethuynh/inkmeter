#!/bin/sh
# Stop the dashboard and restore normal Kindle operation.

RTC=/sys/class/rtc/rtc0
FL=/sys/class/backlight/max77696-bl/brightness
LOCK=/tmp/ai-usage-dashboard.pid

# Kill the dashboard loop if running
[ -f "$LOCK" ] && kill "$(cat "$LOCK")" 2>/dev/null
killall dashboard.sh 2>/dev/null
rm -f "$LOCK"

# Clear any pending RTC wake alarm
echo 0 > "$RTC/wakealarm" 2>/dev/null || true

# Remove legacy cron jobs (from older versions)
crontab -r 2>/dev/null

# Restore Kindle UI, frontlight, WiFi
lipc-set-prop com.lab126.pillow disableEnablePillow enable 2>/dev/null || true
echo 12 > "$FL" 2>/dev/null || true
wpa_cli reconnect > /dev/null 2>&1

echo "Dashboard stopped. Kindle restored to normal."
