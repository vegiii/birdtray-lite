# Birdtray Lite

This fork is meant for my personal use on Fedora KDE Wayland and comes with no guarantee that it will work on other computers. I recommend using the upstream repositories instead.

## Why this fork

- Make inbox discovery easier to set up.
- Add a simple autostart checkbox.
- Simplify installation.
- Keep dependencies to a minimum.
- Clean up code I will never use.

## Install

```bash
sudo dnf install cmake gcc-c++ qt6-qtbase-devel &&
gh repo clone vegiii/birdtray-lite ~/Documents/Git/birdtray-lite &&
~/Documents/Git/birdtray-lite/install.sh
```

## Update

```bash
cd ~/Documents/Git/birdtray-lite &&
git pull --ff-only &&
./install.sh
```

Settings are stored in `~/.config/birdtray-lite/config.json`.

## Credits and license

Based on [Birdtray](https://github.com/gyunaev/birdtray) by George Yunaev and contributors, and [birdtray-wayland](https://github.com/stark1tty/birdtray-wayland) by Nika Shilobod, which added hide/show support for KDE Plasma Wayland and Flatpak Thunderbird.

Licensed under the [GNU General Public License v3](LICENSE.txt).
