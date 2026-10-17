#!/bin/bash
# Build and install or update Birdtray Lite for the current user, then restart it if Plasma is running.
# Runs from a checkout, or downloads the source when piped from curl.
set -e

main() {
    missing=()
    for pkg in cmake gcc-c++ qt6-qtbase-devel; do
        rpm -q "$pkg" >/dev/null 2>&1 || missing+=("$pkg")
    done
    if [ ${#missing[@]} -gt 0 ]; then
        # stdin is the script when piped from curl, so read the confirmation from the terminal.
        sudo dnf install "${missing[@]}" </dev/tty
    fi

    if [ -f "$(dirname "${BASH_SOURCE[0]}")/CMakeLists.txt" ] && [ -f "${BASH_SOURCE[0]}" ]; then
        cd "$(dirname "${BASH_SOURCE[0]}")"
    else
        src=$(mktemp -d)
        trap 'rm -rf "$src"' EXIT
        curl -fsSL https://github.com/vegiii/birdtray-lite/archive/refs/heads/main.tar.gz |
            tar -xz -C "$src" --strip-components=1
        cd "$src"
    fi

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
}

main
