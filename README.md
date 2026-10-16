# Birdtray Lite

Tray icon showing the unread mail count for Thunderbird, with hide/show of the Thunderbird window on X11 and KDE Plasma Wayland.

A trimmed-down fork of [Birdtray](https://github.com/gyunaev/birdtray) by George Yunaev, licensed under GPLv3.

## Build

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qtsvg-devel libX11-devel &&
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release &&
cmake --build build
```

Settings are stored in `~/.config/birdtray-lite/config.json`.
