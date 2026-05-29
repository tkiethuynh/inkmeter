#!/bin/bash
# Build the ARM binary with Zig and deploy it to a jailbroken Kindle.
#
#   KINDLE=root@<kindle-ip> ./deploy.sh
#
# Requires: cmake, and ssh/scp access to the Kindle (USBNetwork or WiFi SSH).
set -e

KINDLE="${KINDLE:-$1}"
if [ -z "$KINDLE" ]; then
    echo "Usage: KINDLE=root@<kindle-ip> ./deploy.sh" >&2
    exit 1
fi

DEST=/mnt/us/extensions/ai-usage
ZIG_VER=0.13.0
ZIG=/tmp/zig-linux-x86_64-$ZIG_VER/zig

# 1. Fetch Zig (self-contained cross-compiler; no kernel-version constraints)
if [ ! -x "$ZIG" ]; then
    echo "==> Downloading Zig $ZIG_VER..."
    wget -q "https://ziglang.org/download/$ZIG_VER/zig-linux-x86_64-$ZIG_VER.tar.xz" -O /tmp/zig.tar.xz
    tar -xf /tmp/zig.tar.xz -C /tmp/
fi

# 2. Zig cc/c++/ar wrappers for the Kindle's ARMv7 soft-float musl target
cat > /tmp/zig-arm-cc  <<EOF
#!/bin/sh
exec $ZIG cc  -target arm-linux-musleabi -mcpu=cortex_a9 "\$@"
EOF
cat > /tmp/zig-arm-cxx <<EOF
#!/bin/sh
exec $ZIG c++ -target arm-linux-musleabi -mcpu=cortex_a9 "\$@"
EOF
cat > /tmp/zig-arm-ar  <<EOF
#!/bin/sh
exec $ZIG ar "\$@"
EOF
chmod +x /tmp/zig-arm-cc /tmp/zig-arm-cxx /tmp/zig-arm-ar

# 3. Build + strip
echo "==> Building..."
cmake . -Bbuild-zig -DCMAKE_TOOLCHAIN_FILE=toolchain-arm.cmake -DCMAKE_BUILD_TYPE=Release -Wno-dev >/dev/null
cmake --build build-zig -j"$(nproc)"
"$ZIG" objcopy --strip-all build-zig/ai-usage build-zig/ai-usage.stripped

# 4. Deploy
echo "==> Deploying to $KINDLE:$DEST ..."
ssh "$KINDLE" "mkdir -p $DEST/bin; cp /dev/null $DEST/bin/ai-usage 2>/dev/null || true"
scp build-zig/ai-usage.stripped                 "$KINDLE:$DEST/bin/ai-usage"
scp kindle/extensions/ai-usage/config.xml       "$KINDLE:$DEST/config.xml"
scp kindle/extensions/ai-usage/menu.json        "$KINDLE:$DEST/menu.json"
scp kindle/extensions/ai-usage/bin/dashboard.sh "$KINDLE:$DEST/bin/dashboard.sh"
scp kindle/extensions/ai-usage/bin/start.sh     "$KINDLE:$DEST/bin/start.sh"
scp kindle/extensions/ai-usage/bin/stop.sh      "$KINDLE:$DEST/bin/stop.sh"
ssh "$KINDLE" "chmod +x $DEST/bin/*"

# 5. Seed config on first install only (never clobber an existing key)
if ! ssh "$KINDLE" "[ -f $DEST/config ]"; then
    scp kindle/config.example "$KINDLE:$DEST/config"
    echo "==> Seeded $DEST/config — edit it on the Kindle and set your SESSION_KEY."
fi

echo "==> Done. Launch via KUAL > AI Usage > Start Dashboard."
