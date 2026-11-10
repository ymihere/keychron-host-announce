# keychron-host-announce

**Make a Logitech mouse follow a Keychron keyboard from computer to computer.**

Custom firmware for the Keychron V6 Max. When you switch the keyboard to another computer, it
first tells the computer it is leaving where it is going. Software on that computer can then
send your mouse, or anything else, to the same place.

The full guide, with everything learned while getting it to work, is
[`docs/setup.md`](docs/setup.md).

## The problem

Many wireless keyboards and mice can pair with up to three computers. You press a key and the
device jumps to another one. But each device switches on its own. Press Fn+2 on the keyboard and
the mouse stays behind, so you reach for its switch button every time.

Logitech solves this between its own devices: an MX keyboard announces a switch before it
leaves, and software moves the mouse along. A stock Keychron sends nothing. It just drops the
Bluetooth link, and the computer it leaves never learns where it went.

## What this firmware does

It adds one step to the host keys. Everything else is Keychron's stock firmware.

```
You press Fn+2
   │
   ├─ 1. Keyboard sends Ctrl+Alt+Shift+F14 to the computer it is still connected to
   │       └─ a shortcut there runs "move the mouse to slot 2"
   │
   └─ 2. About 100 ms later the keyboard switches to Bluetooth host 2, as usual
```

| Key | Signal sent before leaving | Meaning |
|---|---|---|
| Fn+1 | Ctrl+Alt+Shift+F13 | going to host 1 |
| Fn+2 | Ctrl+Alt+Shift+F14 | going to host 2 |
| Fn+3 | Ctrl+Alt+Shift+F15 | going to host 3 |

- **No signal when nothing changes.** Nothing is sent in 2.4 GHz or cable mode, while the keyboard
  is disconnected, or when you press the key of the host it is already on.
- **Stock behaviour is kept.** Holding a host key for pairing works as before. The keymap is
  Keychron's own, so Keychron Launcher and VIA keep working.
- **Any key can be a host key.** The firmware acts on the Bluetooth host keycodes wherever they
  sit. In Keychron Launcher you can put "Bluetooth Host 1–3" on F13–F15, for example, and the
  mouse follows from those keys too ([guide, section 5.1](docs/setup.md#51-optional-switch-with-f13f15-too-set-up-in-keychron-launcher)).
- **The signal is just a key combo.** Any computer can act on it with a normal keyboard shortcut.
  Moving a mouse is one use. Switching a monitor input or a KVM is another.

## What you need

- A **Keychron V6 Max** (ANSI, ISO or JIS). Other Keychron wireless boards from the same firmware
  family should work with a small addition, see [Other keyboards](#other-keyboards).
- On each computer, **something that reacts to the combos**. For a Logitech mouse on Linux that is
  `mx-push-mouse` from the `mx-easy-switch` project, a command that moves the mouse to a given
  Easy-Switch slot. This repo ships a script that binds the combos to such a command on the
  Cinnamon desktop.
- The **same slot numbers on both devices**. Keyboard host 1 and mouse slot 1 must be the same
  computer, and so on.

## Install

### 1. Get the firmware

Every push to this repo builds all three layouts on GitHub Actions. Download the `.bin` for your
layout from the **latest** release. To build it yourself, see [Building](#building).

### 2. Flash it

1. Set the mode switch to cable. Hold **Esc** while you plug the cable in. The keyboard starts in
   its bootloader and shows up as USB device `0483:df11`.
2. **Back up the stock firmware first:**
   ```sh
   dfu-util -a 0 -d 0483:df11 -s 0x08000000:262144 -U stock-backup.bin
   ```
3. **Do not flash the `.bin` as is.** It fills the keyboard's settings sector with zeros. Make a
   copy with that gap set to `0xFF` ([guide, section 5.0](docs/setup.md#50-flashing-step-by-step-as-done-on-2026-11-10)),
   then flash and read back to verify:
   ```sh
   dfu-util -a 0 -d 0483:df11 -s 0x08000000 -D hostannounce-iso-flash.bin
   dfu-util -a 0 -d 0483:df11 -s 0x08000000:262144 -U readback.bin
   ```
4. Unplug and replug to start the new firmware.
5. To go back, flash your backup, or Keychron's stock firmware, the same way.

Only the main keyboard firmware changes. The separate Bluetooth and 2.4 GHz module firmware is
not touched. Keychron warns against updating that one yourself.

### 3. Set up each computer

On Linux with Cinnamon, bind the combos to your mouse command:

```sh
./host/install-cinnamon-shortcuts.sh --dry-run   # show what would change
./host/install-cinnamon-shortcuts.sh             # add or update the three shortcuts
./host/install-cinnamon-shortcuts.sh --remove    # take them out again
```

By default it runs `~/.local/bin/mx-push-mouse <slot>`, with slots counted from 0. Point it at
another command with `MX_PUSH_MOUSE=/path/to/command`. The script works around two Cinnamon
traps: Num Lock blocks these shortcuts unless they are registered a second time with it, and
Cinnamon only reloads custom shortcuts whose name contains "custom" (guide, section 6.1).

Every computer you switch *away from* needs this, because the old computer is the one that moves
the mouse. Use an `mx-push-mouse` with the scan retry (from 2026-11-10): an older one fails
because the keyboard disappears while it scans. On other desktops, Windows or macOS, bind the
same combos with the desktop's own shortcuts, AutoHotkey or Hammerspoon (guide, section 6.2).

## Customising

Change the combos or the delay in `users/hostannounce/hostannounce.h`, then rebuild. The V6 Max
has real F13 to F16 keys, which is why the plain keys are not used as signals.

## Other keyboards

All the logic is in `users/hostannounce/`. Adding another Keychron wireless board means copying
its stock `keychron` keymap to `keyboards/keychron/<board>/<layout>/keymaps/hostannounce/`, adding
`VIA_ENABLE = yes` to its `rules.mk`, and adding the target to `qmk.json`.

## Building

This is a [QMK userspace](https://docs.qmk.fm/newbs_external_userspace). Keychron's wireless
code is not in mainline QMK, so it builds against Keychron's fork, `Keychron/qmk_firmware`
branch `2025q3`. GitHub Actions does this on every push. Locally:

```sh
git clone -b 2025q3 --recurse-submodules https://github.com/Keychron/qmk_firmware.git
qmk config user.qmk_home=$PWD/qmk_firmware user.overlay_dir=$PWD
qmk userspace-compile
```

## Status

**Works on a keyboard (2026-11-10).** The ISO build was flashed on a V6 Max and used between two
Linux Mint / Cinnamon computers:

- the combo reaches the old computer before the link drops;
- the mouse follows in both directions, from Fn+1/2/3 and from F13–F15 remapped in Launcher.

Not yet checked:

- that no key stays stuck on the old computer after many switches;
- that a four-second hold on a host key still starts pairing;
- that nothing is sent in cable or 2.4 GHz mode;
- the ANSI and JIS builds on hardware, and a local build.

## License

GPL-2.0-or-later, like QMK. The keymaps are Keychron's, unchanged.
