#!/bin/sh
# KUAL launcher: detaches the dashboard loop so it survives KUAL closing.
# Uses absolute paths so it works regardless of KUAL's working directory.

DASH=/mnt/us/extensions/ai-usage/bin/dashboard.sh

# Don't start a second instance
if [ -f /tmp/ai-usage-dashboard.pid ] && kill -0 "$(cat /tmp/ai-usage-dashboard.pid)" 2>/dev/null; then
    exit 0
fi

setsid "$DASH" < /dev/null > /tmp/ai-usage-dash.log 2>&1 &
exit 0
