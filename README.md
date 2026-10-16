# Birdtray Lite

This fork is meant for my personal use on Fedora KDE Wayland and comes with no guarantee that it will work on other computers. I recommend using the upstream repositories instead.

## Why this fork

- Make inbox discovery easier to set up.
- Add a simple autostart checkbox.
- Simplify installation.
- Keep dependencies to a minimum.
- Clean up code I will never use.

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
- Fixed unread count on the tray icon.
- Option to start at login.
- Closing Thunderbird on quit works on Wayland.
- Install script.

### Removed

- Windows, macOS and X11 support: Thunderbird is controlled through KWin only.
- Auto-updater, translations, snooze, email templates and blinking.
- Per-folder notification colors: one count color is used for all folders.
- Advanced settings, such as custom icons, fonts and running a command on new mail.
- Ignoring the current unread emails.
- Command line options `--dump-mork` and `--decode`.
- Legacy settings migration and portable mode.
- Qt Network, Qt Svg and libX11 dependencies.
- Tests, CI, the Windows installer and other upstream tooling.

### Deleted

| Part | Files | Lines |
|---|---:|---:|
| Tests and test data | 17 | 34,898 |
| Translations | 42 | 17,251 |
| Source files | 28 | 2,879 |
| Windows installer | 11 | 2,039 |
| CI, IDE files, contrib and screenshots | 16 | 1,354 |
| **Total** | **114** | **58,421** |

In total 62,326 lines were removed and 1,008 added. The repository went from 159 to 50 files, and the C++ code from 8,173 to 4,283 lines.

## Credits and license

Based on [Birdtray](https://github.com/gyunaev/birdtray) by George Yunaev and contributors, and [birdtray-wayland](https://github.com/stark1tty/birdtray-wayland) by Nika Shilobod, which added hide/show support for KDE Plasma Wayland and Flatpak Thunderbird.

Licensed under the [GNU General Public License v3](LICENSE.txt).
