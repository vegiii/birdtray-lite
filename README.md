# Birdtray Lite

A fork of [birdtray-wayland](https://github.com/stark1tty/birdtray-wayland), which is based on [Birdtray](https://github.com/gyunaev/birdtray).

This fork is meant for my personal use on Fedora KDE Wayland and comes with no guarantee that it will work on other systems. This project is developed with the help of AI coding agents.

## Why this fork

- Make inbox discovery easier to set up.
- Add a simple autostart checkbox.
- Simplify installation.
- Keep dependencies to a minimum.
- Remove unused features to keep the code small.

## Install or update

Builds in `/tmp` and installs to `~/.local`. Run the same command again to update.

```bash
sudo dnf install cmake gcc-c++ git qt6-qtbase-devel &&
rm -rf /tmp/birdtray-lite &&
git clone --depth 1 https://github.com/vegiii/birdtray-lite /tmp/birdtray-lite &&
/tmp/birdtray-lite/install.sh &&
rm -rf /tmp/birdtray-lite
```

Settings are stored in `~/.config/birdtray-lite/config.json`.

## Changes from upstream

### Added

- Monitors all Thunderbird inboxes on the first start.
- Fixed unread count on the tray icon, shown as a number or a badge.
- Simpler settings, with the mail folders selected directly in the Monitoring tab.
- Option to start at login.
- Option to hide Thunderbird in the tray when minimized.
- Own icon, with an option to use the icon theme's Thunderbird icon in the tray.
- Install script.

### Changed

- KWin is controlled through Qt D-Bus instead of the `qdbus6` command, so `qt6-qttools` is no longer needed.
- The KWin script is written to the private runtime directory instead of `/tmp`.
- Closing Thunderbird on quit also goes through KWin, so it works on Wayland.
- Own app ID and settings file, so it can be installed next to Birdtray.
- Single-instance check uses D-Bus instead of a local socket.
- The Thunderbird command, including the Flatpak, is detected at every start.

### Removed

- Windows, macOS and X11 support: Thunderbird is controlled through KWin only.
- Auto-updater, translations, snooze, email templates and blinking.
- Per-folder notification colors and the free color picker: the count uses one of six preset colors.
- Advanced settings, such as custom icons, fonts and running a command on new mail.
- Ignoring the current unread emails.
- Command line options `--dump-mork`, `--decode`, `--reset-settings` and `--log`.
- Legacy settings migration and portable mode.
- Qt Network, Qt Svg and libX11 dependencies.
- Tests, CI, the Windows installer and other upstream tooling.

### Deleted

| Part | Files | Lines |
|---|---:|---:|
| Tests and test data | 17 | 34,898 |
| Translations | 42 | 17,251 |
| Source files | 37 | 3,781 |
| Windows installer | 11 | 2,039 |
| CI, IDE files, contrib and screenshots | 17 | 1,358 |
| Icons | 5 | – |
| **Total** | **129** | **59,327** |

In total 63,562 lines were removed and 1,409 added. The repository went from 159 to 42 files, and the C++ code from 8,173 to 3,557 lines.

## Credits and license

Based on [Birdtray](https://github.com/gyunaev/birdtray) by George Yunaev and contributors, and [birdtray-wayland](https://github.com/stark1tty/birdtray-wayland) by Nika Shilobod, which added hide/show support for KDE Plasma Wayland and Flatpak Thunderbird.

The icon is based on the Thunderbird icon from the [Papirus icon theme](https://github.com/PapirusDevelopmentTeam/papirus-icon-theme) (GPL v3).

Licensed under the [GNU General Public License v3](LICENSE.txt).
