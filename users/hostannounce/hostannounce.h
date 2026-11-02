// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Key combos sent to the current host just before the keyboard leaves for
// Bluetooth host 1, 2 or 3. The V6 Max has physical F13-F16 keys, so the plain
// keys are not usable as signals; with Ctrl+Alt+Shift held they never occur by
// accident. Under X11 these arrive as <Primary><Alt><Shift>XF86Tools,
// ...XF86Launch5 and ...XF86Launch6.
#ifndef HOST_ANNOUNCE_KEY_1
#    define HOST_ANNOUNCE_KEY_1 C(A(S(KC_F13)))
#endif
#ifndef HOST_ANNOUNCE_KEY_2
#    define HOST_ANNOUNCE_KEY_2 C(A(S(KC_F14)))
#endif
#ifndef HOST_ANNOUNCE_KEY_3
#    define HOST_ANNOUNCE_KEY_3 C(A(S(KC_F15)))
#endif

// How long the switch waits after the announcement. It then also waits until
// the Bluetooth report queue is empty, because Keychron clears that queue on
// disconnect and a lost key-up would leave the combo stuck on the old host.
#ifndef HOST_ANNOUNCE_DELAY_MS
#    define HOST_ANNOUNCE_DELAY_MS 100
#endif
#ifndef HOST_ANNOUNCE_MAX_EXTRA_MS
#    define HOST_ANNOUNCE_MAX_EXTRA_MS 200
#endif
