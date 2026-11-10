# Keychron host announce — setup guide

**For:** a person or an AI agent with shell access.
**Goal:** pressing Fn+1, Fn+2 or Fn+3 on a Keychron V6 Max also moves a Logitech MX mouse to the
same computer, the way an MX keyboard already does with Logitech's software.

Written 2026-11-10 from a working setup: a Keychron V6 Max ISO (USB `3434:0961`), a Logitech MX
Anywhere 3S, and two Linux Mint 22.3 machines with Cinnamon on X11. Host names in this guide are
placeholders: `linux-a`, `windows-pc` and `linux-b`.

The [README](../README.md) is the short version. This guide has everything that was learned
while getting it to work, including the traps.

---

## 0. Status (2026-11-10)

| Item | State | How it was checked |
|---|---|---|
| Build | **verified**: ANSI, ISO and JIS compile without errors or warnings against `Keychron/qmk_firmware` branch `2025q3` | GitHub Actions "Build firmware" |
| Binaries | release `latest`: `keychron_v6_max_{ansi,iso,jis}_encoder_hostannounce.bin`, about 109 KB each | release page |
| Keymap | **verified** byte-identical to Keychron's stock `keymaps/keychron/keymap.c` (ISO) | `diff` against Keychron's fork |
| Flashed on a keyboard | **yes**, ISO build, with the settings gap set to `0xFF` (section 5.0). The keyboard starts with it and reports USB device version 1.1.1. Typing works | DFU readback identical to the image |
| Combo arrives | **verified**: Fn+3 sends Ctrl+Shift+Alt+F15 (X11 keycodes 37, 50, 64, 193), and the keyboard leaves about 6 s later | raw XI2 key log |
| `linux-a` → `linux-b` | **works**: Fn+3 moves keyboard and mouse. It took three fixes on the computer side: the two Cinnamon traps and the scan retry in section 6.1 | `~/.cache/mx-push-mouse.log` |
| `linux-b` → `linux-a` | **works**: Fn+1 moves keyboard and mouse | user report |
| F13–F15 as host keys | **works**: remapped to Bluetooth Host 1–3 in Keychron Launcher (section 5.1) | user report |
| Still to test | stuck modifiers after many switches, 4 s hold for pairing, no signal in cable or 2.4 GHz mode (section 6.3, tests 3–5) | — |

---

## 1. The problem

The keyboard and the mouse can each pair with three computers. But each one switches on its own.
Press Fn+2 on the keyboard and the mouse stays behind.

Logitech solves this between its own devices. An MX keyboard sends a HID++ `CHANGE HOST`
notification to the computer it is leaving. Solaar (or Logi Options+) on that computer then tells
the mouse to switch too. That is the whole `mx-easy-switch` setup.

A stock Keychron sends nothing. It drops the Bluetooth link, and the computer it leaves never
learns where it went. Nothing on the computer can fix that, so the change has to go into the
keyboard firmware.

## 2. How it works

The firmware adds one step to the Bluetooth host keys `BT_HST1..3`. Everything else is Keychron's
stock firmware.

```
You press Fn+2 (keyboard on host 1)
   │
   ├─ 1. Keyboard taps Ctrl+Alt+Shift+F14 to host 1, which is still connected
   │       └─ a desktop shortcut on host 1 runs "mx-push-mouse 1" → the mouse moves to slot 2
   │
   └─ 2. After 100 ms, and once the Bluetooth report queue is empty
          (at most 200 ms more), the keyboard switches to host 2 as usual
```

| Key | Combo sent before leaving | X11 binding | Shortcut runs |
|---|---|---|---|
| Fn+1 | Ctrl+Alt+Shift+F13 | `<Primary><Alt><Shift>XF86Tools` | `mx-push-mouse 0` |
| Fn+2 | Ctrl+Alt+Shift+F14 | `<Primary><Alt><Shift>XF86Launch5` | `mx-push-mouse 1` |
| Fn+3 | Ctrl+Alt+Shift+F15 | `<Primary><Alt><Shift>XF86Launch6` | `mx-push-mouse 2` |

`mx-push-mouse` counts slots from 0. Keyboard host N is mouse slot N−1.

Design decisions, and why:

- **Why a key combo and not something smarter.** Any operating system can bind a key combo to a
  command with no extra software. Moving the mouse is one use. A KVM or a monitor input switch
  would be another.
- **Why Ctrl+Alt+Shift.** The V6 Max has real F13 to F16 keys, so plain F13 to F15 would also fire
  on a normal key press. With all three modifiers held they never happen by accident.
- **The switch is deferred, not reimplemented.** `process_record_user` swallows the host key
  press and taps the combo. It then schedules a callback with `defer_exec`. The callback replays
  the held-back press, and for a quick tap also the release, through `process_record_kb`, which is
  Keychron's own handler. Long-press pairing (hold 4 s), the indicators and everything else
  therefore work as in stock firmware.
- **Why it waits for the queue.** Keychron clears its Bluetooth report queue on disconnect. If
  the combo's key-up were still queued, the old computer would see Ctrl+Alt+Shift+F14 held down
  forever. So after the 100 ms delay the callback polls every 10 ms until
  `report_buffer_is_empty()`, for at most 200 ms more.
- **When nothing is sent:**
  - in 2.4 GHz or cable mode;
  - while the keyboard is not connected;
  - when the current host is unknown;
  - when you press the key of the host it is already on.

  The current host comes from Keychron's `wireless_enter_connected_kb(host_idx)` hook.
- **Only one switch at a time.** A host key pressed while a switch is pending is ignored.

**On a hold:** while the key is held, the replayed press sits in Keychron's handler as a normal
press. The real release then goes through the normal path (`keycode != pending_kc` once the
callback has cleared `pending_kc`), so a 4 s hold should reach Keychron's pairing timer. This is
reasoning from the code, not yet a test result.

## 3. Preconditions

1. **The same slot numbers on both devices.** Keyboard host 1 and mouse slot 1 must be the same
   computer, and so on. In the reference setup:

   | Key / slot | Machine |
   |---|---|
   | 1 | `linux-a` (mouse through a Logi Bolt receiver) |
   | 2 | `windows-pc` (gone; nothing at the moment) |
   | 3 | `linux-b` (mouse over Bluetooth) |

   So pair the Keychron's Bluetooth host 1 with `linux-a` and host 3 with `linux-b`. To pair,
   hold Fn+1 or Fn+3 for about 4 s and pair from the computer's Bluetooth settings.
2. **`mx-push-mouse` on every computer you switch away from.** The computer the keyboard
   *leaves* is the one that moves the mouse. Install it with `mx-setup.sh` from the
   `mx-easy-switch` project. That also installs Solaar rules for an MX keyboard. They do no harm
   when the Keychron is used instead. It must be a version with the scan retry (section 6.1,
   third trap).
3. **The mouse must be reachable from that computer:** through its receiver or its Bluetooth link.
   If the mouse is not there, `mx-push-mouse` logs `mouse not found` and nothing happens.

## 4. Get the firmware

**Download (recommended).** GitHub Actions builds all three layouts on every push and publishes
them as the release `latest`. Download the `.bin` for your layout from the release page, or from
inside a clone of this repo:

```sh
gh release download latest -p '*iso*'   # or *ansi* / *jis*
```

Pick the layout of the keyboard: ISO for a German or Austrian layout, ANSI for US.

**Build locally.** The firmware is a [QMK userspace](https://docs.qmk.fm/newbs_external_userspace).
Keychron's wireless code is not in mainline QMK, so it builds against Keychron's fork:

```sh
python3 -m pip install --user qmk         # the QMK CLI, if missing
git clone -b 2025q3 --recurse-submodules https://github.com/Keychron/qmk_firmware.git
cd keychron-host-announce
qmk config user.qmk_home=$PWD/../qmk_firmware user.overlay_dir=$PWD
qmk userspace-compile                     # builds all targets in qmk.json
```

The toolchain needs `arm-none-eabi-gcc`. The local path has not been run yet; CI is the only
build that has been verified.

**Rebuild the keymaps from Keychron's.** The keymaps here are Keychron's stock ones. To refresh
them:

```sh
for l in ansi iso jis; do
  d=keyboards/keychron/v6_max/${l}_encoder/keymaps/hostannounce
  mkdir -p $d
  cp ../qmk_firmware/keyboards/keychron/v6_max/${l}_encoder/keymaps/keychron/keymap.c $d/
  echo 'VIA_ENABLE = yes' > $d/rules.mk
done
```

The user code in `users/hostannounce/` is picked up automatically, because QMK sets `USER_NAME`
to the keymap name.

## 5. Flash and revert

1. Set the keyboard's mode switch to cable.
2. Hold **Esc** while you plug the cable in. The keyboard starts in its STM32 DFU bootloader:
   ```sh
   lsusb | grep 0483:df11
   ```
   If holding Esc does not start the bootloader, hold the reset button under the space bar
   instead while plugging in (from Keychron's `keyboards/keychron/v6_max/readme.md`; not tried).
3. Back up, prepare the image and flash it as in section 5.0.
4. **To revert**, flash your backup, or Keychron's stock V6 Max firmware from keychron.com, the
   same way.

Only the main MCU firmware changes. The separate Bluetooth and 2.4 GHz module firmware is not
touched. Keychron warns against updating that one yourself.

Because the keymap is Keychron's own and `VIA_ENABLE = yes`, Keychron Launcher and VIA keep
working, and existing remaps survive.

### 5.0 Flashing step by step, as done on 2026-11-10

`sudo apt install dfu-util`. `dfu-util` then works without sudo, because the package's udev rule
`60-dfu-util.rules` already gives the logged-in user access to `0483:df11`. The DFU memory map
shows a 256 KB STM32F401 (`04*016Kg,01*064Kg,01*128Kg`).

1. **Backup first.** Read out the whole flash before writing anything:
   ```sh
   dfu-util -a 0 -d 0483:df11 -s 0x08000000:262144 -U stock-backup.bin
   ```
   This is the exact factory firmware plus its settings. To restore it, flash it the same way as
   below.
2. **Fix the settings gap.**
   - Factory image: vector table at `0x08000000`, settings (wear-levelled EEPROM) at
     `0x08004000–0x08007FFF`, code from `0x08008000`, everything else `0xFF`.
   - This build: the same layout, even the same stack and reset vector (`0x20000400`,
     `0x0800811d`).
   - But the build's `.bin` fills the gap `0x0400–0x7FFF` with **zeros**, so flashing it as is
     writes zeros over the settings sector. A Keychron `.bin` from their website probably does
     the same, and QMK should then reset its settings.
   - To be on the safe side, set that gap to `0xFF`, the erased state of a new chip:
     ```sh
     python3 - <<'PY'
     n = bytearray(open('keychron_v6_max_iso_encoder_hostannounce.bin', 'rb').read())
     assert n[0x400:0x8000] == bytes(0x8000 - 0x400)
     n[0x400:0x8000] = b'\xff' * (0x8000 - 0x400)
     open('hostannounce-iso-flash.bin', 'wb').write(n)
     PY
     ```
3. **Flash, then verify by reading back:**
   ```sh
   dfu-util -a 0 -d 0483:df11 -s 0x08000000 -D hostannounce-iso-flash.bin
   dfu-util -a 0 -d 0483:df11 -s 0x08000000:262144 -U readback.bin
   ```
   The readback must be identical to the image, with the rest erased (`0xFF`).
4. **Unplug and replug** without holding a key to start the new firmware. Flashing without
   `:leave` keeps the chip in DFU mode for the readback.

### 5.1 Optional: switch with F13–F15 too, set up in Keychron Launcher

The stock firmware sends F13, F14 and F15 from the keys above the number pad (F16 from the
fourth). These can switch computers too. Fn+1/2/3 keep working alongside.

**No firmware change is needed.** The firmware catches the Bluetooth host keys (`BT_HST1..3`)
wherever they sit on the keyboard. Keychron Launcher and VIA can put them on any key: Keychron's
VIA definition for the V6 Max (`keyboards/keychron/v6_max/via_json/`, branch `2025q3`) lists them
as custom keycodes "Bluetooth Host 1", "Bluetooth Host 2" and "Bluetooth Host 3". So the keyboard
announces the switch from those keys too, and the mouse follows.

| Key | Set in Launcher to | Keyboard goes to | Mouse follows |
|---|---|---|---|
| F13 | Bluetooth Host 1 | computer 1 | yes |
| F14 | Bluetooth Host 2 | computer 2 | yes |
| F15 | Bluetooth Host 3 | computer 3 | yes |
| Fn+1/2/3 | unchanged | computer 1/2/3 | yes |

**First, on Linux: let Chrome reach the keyboard.** Launcher talks to the keyboard through
WebHID, which opens the keyboard's `/dev/hidraw*` nodes. By default those belong to root with
mode 600. Chrome then lists the keyboard, or connects, but nothing in Launcher responds. A
one-line udev rule gives the logged-in user access:

```sh
sudo tee /etc/udev/rules.d/70-keychron-hidraw.rules >/dev/null <<'EOF'
# Let the logged-in user open Keychron keyboards' hidraw nodes, so Keychron Launcher / VIA
# (WebHID in Chrome) can talk to them. Vendor 3434 = Keychron. uaccess must run before 73-seat-late.
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="3434", TAG+="uaccess"
EOF
sudo udevadm control --reload && sudo udevadm trigger --subsystem-match=hidraw
```

Then unplug and replug the cable, and check it worked:

```sh
for h in /sys/class/hidraw/hidraw*; do grep -q 3434 $h/device/uevent && getfacl -p /dev/${h##*/} | grep "^user:$USER"; done
```

Each Keychron node should show `user:<you>:rw-`. A Logitech receiver does not need this, because
Solaar's package ships its own rule (`60-solaar.rules`). The rule is only needed on the computer
you run Launcher from.

**Steps, after flashing the firmware:**

1. Set the mode switch to cable and plug the keyboard in.
2. Open https://launcher.keychron.com in Chrome or another Chromium browser and connect the
   keyboard. It keeps working with this firmware because the keymap and its USB IDs are stock.
3. Open the keymap page. The keyboard has four layers:

   | Layer | Used when |
   |---|---|
   | 0 | Mac base |
   | 1 | Mac Fn |
   | 2 | Windows base |
   | 3 | Windows Fn |

   The Mac/Windows switch on the keyboard picks the base layer.
4. On **layer 0**, click the F13 key and choose **Bluetooth Host 1** from the special or
   Keychron keycodes. Do the same for F14 → **Bluetooth Host 2** and F15 → **Bluetooth Host 3**.
5. Do the same on **layer 2**. On layer 2 these keys are transparent in the stock keymap: they show
   as `▽` or blank. A transparent key falls through to layer 0, so step 4 alone would already
   work. Setting layer 2 too makes it explicit, and it still works if layer 0 is changed later.
6. Leave layers 1 and 3 alone, so Fn+1/2/3 stay as they are.

Launcher saves the change straight to the keyboard. It survives unplugging and does not need
Launcher again.

**To undo**, set the three keys back to F13, F14 and F15 on layer 0, and back to transparent on
layer 2. A factory reset also does it, but it resets everything else too.

**Things to know:**

- **Holds:** a host key held for about 4 s starts Bluetooth pairing for that host. That is stock
  behaviour, and it applies to the new keys as much as to Fn+1/2/3.
- **Cable and 2.4 GHz modes:** the new keys probably do nothing in these modes, because host keys
  only act in Bluetooth mode. Not verified.
- **Plain F13 to F15 are gone** as normal keys. The announce signals (Ctrl+Alt+Shift+F13..F15)
  are not affected, because the firmware creates them itself.
- **Launcher alone is not enough.** With Keychron's stock firmware these keys would switch the
  keyboard, but the mouse would stay behind. The mouse follows only with this firmware.
- **After a later reflash,** check in Launcher that the keys are still mapped. Whether a flash
  keeps the saved keymap depends on the firmware version.

## 6. Set up each computer and test

### 6.1 Cinnamon (Linux Mint)

Run `host/install-cinnamon-shortcuts.sh`:

```sh
./host/install-cinnamon-shortcuts.sh --dry-run   # show what would change
./host/install-cinnamon-shortcuts.sh             # add or update the three shortcuts
./host/install-cinnamon-shortcuts.sh --remove    # take them out again
```

It adds three custom keybindings, `custom-hostannounce1..3`, under
`org.cinnamon.desktop.keybindings`. They appear in *Keyboard → Shortcuts → Custom Shortcuts* as
"Keychron host announce: mouse to slot N". It needs `~/.local/bin/mx-push-mouse`, or another
command given in `MX_PUSH_MOUSE=/path`. The script appends to the existing custom list and does
not touch other shortcuts.

Three traps, all handled by the scripts (found 2026-11-10):

- **Num Lock.** Cinnamon does not ignore Num Lock for these bindings. With Num Lock on,
  `<Primary><Alt><Shift>XF86Launch6` never fires, while `xdotool key --clearmodifiers` (which
  turns Num Lock off for the moment) makes it look fine. The script therefore registers each
  binding twice, the second time with `<Mod2>`.
- **Changes are not picked up unless the name contains "custom".** When `custom-list` changes,
  Cinnamon (`/usr/share/cinnamon/js/ui/keybindings.js`, `remove_custom_keybindings`) drops and
  re-reads only the entries whose *name* contains `custom`, as the Settings app's `custom0`,
  `custom1` do. An entry with another name keeps its old command until Cinnamon restarts, as long
  as its binding is unchanged. The first version of the script used `hostannounce1..3` and fell
  into this: a test command stayed attached to F15, so the first real Fn+3 ran it instead of
  `mx-push-mouse`, and a "log-only" test ran the real command and sent the mouse away. The
  script now uses `custom-hostannounce1..3`, removes the old names (clearing their bindings first,
  so Cinnamon drops the grabs), and sets the list to `[]` and back. To test safely, run the
  script with `MX_PUSH_MOUSE=` pointing at a small script that only logs its argument.
- **Solaar's scan crashes while the keyboard leaves** (not a Cinnamon problem, but found here).
  The shortcut fires a moment before the keyboard drops its Bluetooth link. `mx-push-mouse` then
  scans the HID devices through Solaar 1.1.11, and Solaar's `find_paired_node` crashes with
  `AttributeError: 'NoneType' object has no attribute 'get'` on the Keychron's node, which is
  being removed. Simulated combos never show this, because the keyboard stays connected.
  `mx-push-mouse` now retries the scan up to 8 times, 0.3 s apart (`mx-easy-switch`, fault 9).
  Every computer that uses this needs that version.

The shortcut for the computer's *own* slot is harmless: the firmware never announces the host it
is already on.

### 6.2 Other systems

- **GNOME:** the same idea with `org.gnome.settings-daemon.plugins.media-keys custom-keybindings`.
  There is no script for it yet.
- **KDE:** add custom shortcuts in System Settings that run `mx-push-mouse 0/1/2`.
- **Bare Wayland compositors** (Hyprland, sway): bind the combo in the compositor config. For
  example, in Hyprland: `bind = CTRL ALT SHIFT, XF86Tools, exec, ~/.local/bin/mx-push-mouse 0`.
- **Windows:** binding the combo is easy with AutoHotkey (`^!+F13::Run ...`). But there is no
  `mx-push-mouse` for Windows, and Logi Options+ cannot be triggered from a command. Some tool that
  sends the HID++ `CHANGE HOST` (feature `0x1814`) `setCurrentHost` request would be needed. None
  has been chosen.
- **macOS:** Hammerspoon can bind the combo. The same missing-tool problem applies.

### 6.3 Hardware test

Run this once after flashing, on a computer with keyboard and mouse on slot 1:

1. **The combo arrives.** Before installing the shortcuts, run
   `xev -event keyboard | grep -A2 KeyPress` and press Fn+3. You should see Control, Alt, Shift
   and then `XF86Launch6`, and the keyboard should then leave. If the keyboard leaves and no
   combo appears, the delay is too short (section 8). *Passed 2026-11-10.*
2. **The mouse follows.** Install the shortcuts and press Fn+3. The mouse should be on computer
   3. `~/.cache/mx-push-mouse.log` should show `pushed mouse to host 2 (<name>)`.
   *Passed 2026-11-10, both directions.*
3. **Nothing stuck.** Come back (Fn+1 on the other computer, once it has its shortcuts) and type
   normally. If letters come out as shortcuts, or Ctrl/Alt/Shift behave as if held, the key-up
   was lost. Press and release each modifier once to recover. Then raise
   `HOST_ANNOUNCE_MAX_EXTRA_MS` or `HOST_ANNOUNCE_DELAY_MS`.
4. **Stock behaviour.** A quick tap on Fn+1/2/3 switches. A 4 s hold enters pairing, with the
   indicator blinking as in stock firmware. Pressing the key of the current host does nothing
   new.
5. **No announcement off Bluetooth.** In 2.4 GHz and cable mode, the host keys behave as stock and
   `xev` shows no combo.
6. **The Launcher keys (section 5.1),** if set up. Run tests 1 to 4 with F13/F14/F15 instead of
   Fn+1/2/3, in the keyboard's Windows mode and in its Mac mode. *Switching works (2026-11-10).*
7. **Cinnamon does not fire.** If `xev` sees the combo but the shortcut does not fire, first
   check that each binding also has a `<Mod2>` variant and that the names contain `custom`
   (section 6.1). Then check whether another binding already uses `XF86Tools` or
   `XF86Launch5/6`:
   ```sh
   gsettings list-recursively | grep -i 'XF86Tools\|XF86Launch[56]'
   ```
   A GNOME `media-keys` entry with plain `XF86Tools` (no modifiers) shows up on Mint; it does not
   get in the way.

## 7. Customising

Change the combos or the timing in `users/hostannounce/hostannounce.h`, then rebuild. Every value
is wrapped in `#ifndef`, so it can also be overridden in a keymap's `config.h`.

| Setting | Default | Meaning |
|---|---|---|
| `HOST_ANNOUNCE_KEY_1..3` | `C(A(S(KC_F13)))`, `…F14`, `…F15` | combo sent before leaving for host 1..3 |
| `HOST_ANNOUNCE_DELAY_MS` | 100 | wait after the combo before switching |
| `HOST_ANNOUNCE_MAX_EXTRA_MS` | 200 | extra wait, at most, for the report queue to empty |

**Other Keychron wireless boards** from the same firmware family need only a keymap folder:

1. Copy the board's stock `keychron` keymap to `keyboards/keychron/<board>/<layout>/keymaps/hostannounce/`.
2. Add `VIA_ENABLE = yes` to its `rules.mk`.
3. Add the target to `qmk.json`.

## 8. Risks and things to watch

- **Lost combo.** The combo is sent while the link is still up and the switch waits 100–300 ms,
  so it should arrive. If the old host is slow to process Bluetooth input, raise
  `HOST_ANNOUNCE_DELAY_MS`.
- **Stuck modifiers.** This is the reason for the queue wait. See test 3.
- **Slot mismatch.** If keyboard host N and mouse slot N are different machines, the mouse goes
  to the wrong one. The firmware cannot know the mouse's slots.
- **The way back needs the other computer.** Each computer only handles departures from itself.
  A computer without the shortcut (or without a working `mx-push-mouse`) leaves the mouse behind.
- **Solaar quirks may still apply** to `mx-push-mouse`. With an MX keyboard over Bluetooth, a
  switch within about 6 s after the keyboard came back can be lost (`mx-easy-switch`, fault 8).
  Whether anything like that happens with the Keychron trigger has not been tested.
- **Settings sector.** Flashing the build's `.bin` as is writes zeros over the keyboard's
  settings. Use the `0xFF` copy from section 5.0.
