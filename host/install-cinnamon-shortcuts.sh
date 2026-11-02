#!/usr/bin/env bash
# Bind the three host-announce combos to mx-push-mouse on a Cinnamon desktop.
#
# The keyboard sends Ctrl+Alt+Shift+F13/F14/F15 to the computer it is leaving,
# naming Bluetooth host 1/2/3. Each combo moves the Logitech mouse to the same
# Easy-Switch slot, so keyboard host N and mouse slot N must be the same computer.
#
# Needs mx-push-mouse from https://github.com/ymihere/mx-easy-switch (mx-setup.sh).
#
#   ./install-cinnamon-shortcuts.sh --dry-run   # show what would change
#   ./install-cinnamon-shortcuts.sh             # add or update the shortcuts
#   ./install-cinnamon-shortcuts.sh --remove    # take them out again
set -euo pipefail

SCHEMA=org.cinnamon.desktop.keybindings
ITEM_SCHEMA=$SCHEMA.custom-keybinding
BASE=/org/cinnamon/desktop/keybindings/custom-keybindings
PUSH=${MX_PUSH_MOUSE:-$HOME/.local/bin/mx-push-mouse}
# name in custom-list | X11 binding | mouse host index (0-based)
ENTRIES=(
  "hostannounce1|<Primary><Alt><Shift>XF86Tools|0"
  "hostannounce2|<Primary><Alt><Shift>XF86Launch5|1"
  "hostannounce3|<Primary><Alt><Shift>XF86Launch6|2"
)

mode=install
case "${1:-}" in
  --dry-run) mode=dry ;;
  --remove) mode=remove ;;
  "") ;;
  *) echo "usage: $0 [--dry-run|--remove]" >&2; exit 2 ;;
esac

[[ $mode == remove || -x $PUSH ]] || { echo "mx-push-mouse not found at $PUSH" >&2; exit 1; }

run() { if [[ $mode == dry ]]; then printf '  would run:'; printf ' %q' "$@"; echo; else "$@"; fi; }

list=$(gsettings get $SCHEMA custom-list)
names=$(python3 -c 'import ast,sys; print("\n".join(ast.literal_eval(sys.argv[1].replace("@as ",""))))' "$list")
new=$names
for e in "${ENTRIES[@]}"; do
  IFS='|' read -r name binding idx <<<"$e"
  if [[ $mode == remove ]]; then
    new=$(grep -vx "$name" <<<"$new" || true)
    run gsettings reset-recursively "$ITEM_SCHEMA:$BASE/$name/"
    continue
  fi
  grep -qx "$name" <<<"$new" || new=$(printf '%s\n%s' "$new" "$name" | sed '/^$/d')
  run gsettings set "$ITEM_SCHEMA:$BASE/$name/" name "Keychron host announce: mouse to slot $((idx + 1))"
  run gsettings set "$ITEM_SCHEMA:$BASE/$name/" command "$PUSH $idx"
  run gsettings set "$ITEM_SCHEMA:$BASE/$name/" binding "['$binding']"
done
newlist=$(python3 -c 'import sys; print(repr([n for n in sys.argv[1].split("\n") if n]))' "$new")
run gsettings set $SCHEMA custom-list "$newlist"
echo "custom-list: $list -> $newlist"
