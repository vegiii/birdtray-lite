#!/bin/bash
# Build and install or update Birdtray Lite for the current user, then restart it if Plasma is running.
set -e
cd "$(dirname "$0")"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j "$(nproc)"
cmake --install build

# systemd's autostart generator does not search ~/.local/bin, so use the full path.
exec_path="Exec=$HOME/.local/bin/birdtray-lite"
sed -i "s|^Exec=birdtray-lite|$exec_path|" ~/.local/share/applications/io.github.vegiii.BirdtrayLite.desktop

if systemctl --user is-active -q plasma-plasmashell.service; then
    pkill -x birdtray-lite || true
    while pgrep -x birdtray-lite >/dev/null; do sleep 0.2; done
    setsid -f ~/.local/bin/birdtray-lite >/dev/null 2>&1
fi
