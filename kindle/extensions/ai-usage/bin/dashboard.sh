#!/bin/sh
# Self-managed suspend loop — turns the Kindle into a usage dashboard.
#
# Behaviour:
#   * Renders the usage screen (no frontlight), then truly suspends the device.
#   * While suspended: touch is ignored (like a lockscreen), near-zero power.
#   * An RTC alarm wakes it every REFRESH_MINS to re-render, then it suspends again.
#   * Pressing the POWER button wakes it early -> we restore the normal Kindle UI
#     (home screen + frontlight) and exit.

APP=/mnt/us/extensions/ai-usage/bin/ai-usage
CONFIG=/mnt/us/extensions/ai-usage/config
RTC=/sys/class/rtc/rtc0
FL=/sys/class/backlight/max77696-bl/brightness
LOG=/tmp/ai-usage.log
LOCK=/tmp/ai-usage-dashboard.pid

# Config values (POSIX TZ string + refresh interval), with sane defaults.
TZ=$(grep '^TZ=' "$CONFIG" 2>/dev/null | cut -d= -f2)
[ -n "$TZ" ] && export TZ

REFRESH_MINS=$(grep '^REFRESH_MINS=' "$CONFIG" 2>/dev/null | cut -d= -f2)
[ -z "$REFRESH_MINS" ] && REFRESH_MINS=5
REFRESH=$((REFRESH_MINS * 60))

echo $$ > "$LOCK"
echo "$(date '+%H:%M:%S') dashboard start pid=$$ refresh=${REFRESH}s" >> "$LOG"

# Drop any legacy cron jobs — this loop drives everything now.
crontab -r 2>/dev/null

restore_and_exit() {
    lipc-set-prop com.lab126.powerd preventScreenSaver 0 2>/dev/null || true
    lipc-set-prop com.lab126.pillow disableEnablePillow enable 2>/dev/null || true
    echo 12 > "$FL" 2>/dev/null || true
    wpa_cli reconnect > /dev/null 2>&1
    rm -f "$LOCK"
    exit 0
}
trap restore_and_exit TERM INT

# Let KUAL finish closing and the framework settle on the home screen before we
# take over — otherwise KUAL's exit re-enables pillow and redraws home over us.
sleep 5
lipc-set-prop com.lab126.powerd preventScreenSaver 1 2>/dev/null || true
echo 0 > "$FL" 2>/dev/null || true

FIRST=1
while true; do
    # Kill the frontlight immediately on (re)entry — the resume path may have
    # restored the previous brightness, which would otherwise stay on during
    # the whole awake refresh window every cycle.
    echo 0 > "$FL" 2>/dev/null || true

    # --- bring WiFi up and wait for a real IP (reassociation after resume can
    #     take a while; too short a wait => fetch fails on the cycle) ---
    ifconfig wlan0 up 2>/dev/null
    wpa_cli reconnect > /dev/null 2>&1
    IP=""
    i=0
    while [ $i -lt 30 ]; do
        IP=$(wpa_cli status 2>/dev/null | grep '^ip_address=' | cut -d= -f2)
        [ -n "$IP" ] && break
        sleep 1; i=$((i + 1))
    done
    echo "$(date '+%H:%M:%S') wifi ip=${IP:-NONE} after ${i}s" >> "$LOG"

    if [ -n "$IP" ]; then
        # Re-assert screen takeover (KUAL/framework may have re-enabled it).
        lipc-set-prop com.lab126.pillow disableEnablePillow disable 2>/dev/null || true
        # Ghost-clear (black->white) only on the very first successful render, to
        # erase the Kindle home screen; later renders sit on our own screen, so a
        # single GC16 is enough (saves two full refreshes — power + no flash).
        if [ "$FIRST" = "1" ]; then
            "$APP" "$CONFIG" /dev/fb0 --clear >> "$LOG" 2>&1
            # First good render: keep the light on 60s so it's easy to read.
            echo 12 > "$FL" 2>/dev/null || true
            sleep 60
            echo 0 > "$FL" 2>/dev/null || true
            FIRST=0
        else
            "$APP" "$CONFIG" /dev/fb0 >> "$LOG" 2>&1
        fi
    else
        # No network this cycle — keep the last good frame, retry next wake.
        echo "$(date '+%H:%M:%S') no IP, skipping render (keep last frame)" >> "$LOG"
    fi
    wpa_cli disconnect > /dev/null 2>&1
    tail -40 "$LOG" > "$LOG.tmp" 2>/dev/null && mv "$LOG.tmp" "$LOG"

    # --- arm RTC alarm, then stay suspended until it fires or POWER is pressed ---
    NOW=$(cat "$RTC/since_epoch")
    ALARM=$((NOW + REFRESH))
    echo 0       > "$RTC/wakealarm" 2>/dev/null
    echo "$ALARM" > "$RTC/wakealarm" 2>/dev/null
    echo "$(date '+%H:%M:%S') suspend now=$NOW alarm=$ALARM (+${REFRESH}s)" >> "$LOG"

    while true; do
        # Power-button presses bump this IRQ counter; it persists across suspend,
        # so comparing before/after reliably tells a button wake from RTC/other.
        PRESS_BEFORE=$(awk '/max77696-onkey_press/{print $2}' /proc/interrupts)
        echo mem > /sys/power/state                # SUSPEND (blocks until resume)
        PRESS_AFTER=$(awk '/max77696-onkey_press/{print $2}' /proc/interrupts)

        if [ "$PRESS_AFTER" != "$PRESS_BEFORE" ]; then
            echo "$(date '+%H:%M:%S') POWER button -> exit to Kindle" >> "$LOG"
            restore_and_exit
        fi

        WOKE=$(cat "$RTC/since_epoch")
        if [ "$WOKE" -ge $((ALARM - 5)) ]; then
            echo "$(date '+%H:%M:%S') RTC wake -> refresh" >> "$LOG"
            break                                  # alarm fired -> go refresh
        fi

        # Woke early without a button press => framework/powerd housekeeping.
        # Re-arm the original alarm and suspend again (no spurious refresh/exit).
        echo "$(date '+%H:%M:%S') housekeeping wake (woke=$WOKE) -> re-suspend" >> "$LOG"
        sleep 1   # safety: bound the loop to 1Hz if a suspend ever returns instantly
        echo 0       > "$RTC/wakealarm" 2>/dev/null
        echo "$ALARM" > "$RTC/wakealarm" 2>/dev/null
    done
done
