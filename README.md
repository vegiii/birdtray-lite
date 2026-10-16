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

## Changes from birdtray-wayland

### Added

- Inboxes of all Thunderbird accounts are monitored automatically on the first start, and an "Add inboxes" button adds new ones.
- The folder dialog finds the Thunderbird profile in `~/.config/thunderbird`, shows accounts by email address and shows monitored folders as selected.
- A "Start Birdtray Lite at login" checkbox.
- An install script.
- Closing Thunderbird on quit now works on Wayland.

### Removed

- Auto-updater.
- Windows and macOS support.
- X11 support: Thunderbird is controlled through KWin only, which also drops the libX11 dependency.
- Translations.
- Email templates: "New Email" opens a blank message.
- Snooze.
- Ignoring the current unread emails.
- Running a command when the unread count changes.
- Advanced settings: font, bold, border, custom icons, opacity, fade, font sizes, count offsets, forced re-read and change delay.
- Separate "and hide it" options, merged into "Start Thunderbird hidden".
- "Hide when minimized" and "Support non-NETWM compliant WMs" (X11 only).
- Command line options `--dump-mork` and `--decode`.
- Legacy settings migration, portable mode and the installer settings import.
- Qt Network and Qt Svg dependencies.
- Tests, CI, the Windows installer, IDE files and other upstream tooling.

### Deleted

| Part | Files | Lines |
|---|---:|---:|
| Tests and test data | 17 | 34,898 |
| Translations | 42 | 17,251 |
| Source files | 28 | 2,879 |
| Windows installer | 11 | 2,039 |
| CI, IDE files, contrib and screenshots | 16 | 1,354 |
| **Total** | **114** | **58,421** |

In total 61,935 lines were removed and 941 added. The repository went from 159 to 50 files, and the C++ code from 8,173 to 4,585 lines.

## Credits and license

Based on [Birdtray](https://github.com/gyunaev/birdtray) by George Yunaev and contributors, and [birdtray-wayland](https://github.com/stark1tty/birdtray-wayland) by Nika Shilobod, which added hide/show support for KDE Plasma Wayland and Flatpak Thunderbird.

Licensed under the [GNU General Public License v3](LICENSE.txt).
