#!/usr/bin/env bash
main() {
    # Install or update for the current user. Fedora dependencies are installed as needed.
    set -euo pipefail

    assume_yes=false
    restart=true
    for arg in "$@"; do
        case "$arg" in
            --yes) assume_yes=true ;;
            --no-restart) restart=false ;;
            --help|-h)
                printf 'Usage: %s [--yes] [--no-restart]\n\nInstall or update Birdtray Lite for the current user.\n  --yes         Accept dependency installation prompts.\n  --no-restart  Do not launch apps or restart apps, services, or Plasma.\n' "${0##*/}"
                exit 0
                ;;
            *) printf 'Unknown option: %s\nUse --help for usage.\n' "$arg" >&2; exit 2 ;;
        esac
    done
    if [[ $EUID -eq 0 ]]; then
        printf 'Run this installer as the target user, without sudo.\n' >&2
        exit 1
    fi
    export PATH="$HOME/.local/bin:$PATH"
    trap 'printf "Installation failed at line %s.\n" "$LINENO" >&2' ERR

    # Keep dependency ownership here so setup scripts only need to call install.sh.
    packages=(cmake gcc-c++ make qt6-qtbase-devel procps-ng util-linux curl tar)
    if command -v dnf >/dev/null && command -v rpm >/dev/null; then
        missing=()
        for package in "${packages[@]}"; do
            rpm -q --whatprovides "$package" >/dev/null 2>&1 || missing+=("$package")
        done
        if (( ${#missing[@]} )); then
            if [[ "$assume_yes" == true ]]; then
                sudo dnf install -y "${missing[@]}"
            else
                # Also works when the installer itself is piped from curl.
                sudo dnf install "${missing[@]}" </dev/tty
            fi
        fi
    else
        printf 'Automatic dependency installation requires Fedora. Install these manually: %s\n' "${packages[*]}" >&2
    fi

    if [[ -f "${BASH_SOURCE[0]}" && -f "$(dirname -- "${BASH_SOURCE[0]}")/CMakeLists.txt" ]]; then
        cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")"
    else
        src=$(mktemp -d)
        trap 'rm -rf -- "$src"' EXIT
        curl -fsSL https://github.com/vegiii/birdtray-lite/archive/refs/heads/main.tar.gz |
            tar -xz -C "$src" --strip-components=1
        cd "$src"
    fi

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
    cmake --build build --parallel "$(nproc)"
    cmake --install build

    # systemd's autostart generator needs the full executable path.
    exec_path="Exec=$HOME/.local/bin/birdtray-lite"
    sed -i "s|^Exec=birdtray-lite|$exec_path|" "$HOME/.local/share/applications/io.github.vegiii.BirdtrayLite.desktop"

    if [[ "$restart" == true ]] && systemctl --user is-active -q plasma-plasmashell.service 2>/dev/null; then
        pkill -u "$(id -u)" -x birdtray-lite || true
        for ((attempt=0; attempt<50; attempt++)); do
            pgrep -u "$(id -u)" -x birdtray-lite >/dev/null || break
            sleep 0.1
        done
        if pgrep -u "$(id -u)" -x birdtray-lite >/dev/null; then
            echo 'Birdtray Lite is still running. Quit it before relaunching.' >&2
            exit 1
        fi
        setsid -f "$HOME/.local/bin/birdtray-lite" >/dev/null 2>&1
    fi
    printf 'Installed Birdtray Lite.\n'
}

main "$@"
