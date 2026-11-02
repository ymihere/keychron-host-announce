# keychron-host-announce

Firmware for the **Keychron V6 Max** that tells the computer it is leaving where it is going.
With it, a Logitech MX mouse can follow the keyboard between computers, like it does with an
MX keyboard in [mx-easy-switch](https://github.com/ymihere/mx-easy-switch).

A stock Keychron just drops the Bluetooth link when you press a host key (Fn+1/2/3), so the old
computer never learns the target. This firmware first sends a key combo naming the target to the
computer that is still connected, then switches as usual about 100 ms later.

| Host key | Combo sent to the old computer | Linux shortcut runs |
|---|---|---|
| Fn+1 (Bluetooth host 1) | Ctrl+Alt+Shift+F13 | `mx-push-mouse 0` |
| Fn+2 (Bluetooth host 2) | Ctrl+Alt+Shift+F14 | `mx-push-mouse 1` |
| Fn+3 (Bluetooth host 3) | Ctrl+Alt+Shift+F15 | `mx-push-mouse 2` |

Nothing is sent when the keyboard is in 2.4 GHz or cable mode, when it is not connected, or when
you press the key of the host it is already on. Long-press pairing still works as in stock firmware.

## Layout

- `users/hostannounce/` — all of the logic, shared by every keymap. Change the combos or the delay
  in `hostannounce.h`.
- `keyboards/keychron/v6_max/{ansi,iso,jis}_encoder/keymaps/hostannounce/` — Keychron's own VIA
  keymap, unchanged. Keychron Launcher keeps working.
- `host/install-cinnamon-shortcuts.sh` — binds the three combos to `mx-push-mouse` on Cinnamon.
- `.github/workflows/build_binaries.yaml` — builds against Keychron's fork
  (`Keychron/qmk_firmware`, branch `2025q3`); mainline QMK lacks Keychron's wireless code.

## Getting the firmware

Every push builds all three layouts. The `.bin` files are attached to the **latest** release.
The 2.4 GHz and Bluetooth module firmware is separate and is not touched; Keychron warns against
updating it.

## Flashing

1. Unplug the cable, set the mode switch to cable, hold **Esc**, plug the cable in. The keyboard is
   now in the STM32 DFU bootloader and shows up as `0483:df11`.
2. Flash the file for your layout, for example:
   ```sh
   dfu-util -a 0 -d 0483:df11 -s 0x08000000:leave -D keychron_v6_max_iso_encoder_hostannounce.bin
   ```
   QMK Toolbox or `qmk flash` work too.
3. To go back, flash Keychron's stock firmware the same way.

## Computer side

Pair keyboard host N and mouse Easy-Switch slot N with the same computer. On each Linux computer:

```sh
./host/install-cinnamon-shortcuts.sh --dry-run
./host/install-cinnamon-shortcuts.sh
```

The mouse is pushed by the computer the keyboard leaves, so every computer you switch *away from*
needs this. Windows has no `mx-push-mouse`; a Windows host needs its own handler for the combos.

## Status

Builds in CI. **Not yet tested on a keyboard.** Things to check on real hardware:

- the combo arrives on the old computer before the link drops (look in `~/.cache/mx-push-mouse.log`);
- no key stays stuck on the old computer;
- a quick tap and a 4-second hold (pairing) on Fn+1/2/3 behave as in stock firmware.
