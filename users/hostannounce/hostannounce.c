// SPDX-License-Identifier: GPL-2.0-or-later
//
// Announce a Bluetooth host switch before it happens.
//
// A stock Keychron just drops the link when a host key (BT_HST1..3) is pressed,
// so the computer it leaves never learns where it went. Here the host key first
// sends a key combo naming the target host to the computer that is still
// connected, then performs the normal switch a moment later. Software on that
// computer can react, for example by moving a Logitech mouse to the same slot.
//
// The switch itself is Keychron's own code: the held-back press (and, for a
// quick tap, the release) is replayed through process_record_kb, so long-press
// pairing, indicators and everything else behave as in the stock firmware.

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "transport.h"
#include "wireless.h"
#ifndef DISABLE_REPORT_BUFFER
#    include "report_buffer.h"
#endif
#include "hostannounce.h"

static const uint16_t announce_keys[] = {HOST_ANNOUNCE_KEY_1, HOST_ANNOUNCE_KEY_2, HOST_ANNOUNCE_KEY_3};

// Bluetooth host index 1..3 of the last connection. Only trusted together with
// wireless_get_state() == WT_CONNECTED; Keychron already owns the disconnect hook.
static uint8_t  connected_host = 0;
static uint16_t pending_kc     = KC_NO;
static bool     pending_up     = false;
static bool     replaying      = false;
static uint16_t waited_ms      = 0;

void wireless_enter_connected_kb(uint8_t host_idx) {
    connected_host = (get_transport() == TRANSPORT_BLUETOOTH) ? host_idx : 0;
}

static bool queue_empty(void) {
#ifndef DISABLE_REPORT_BUFFER
    return report_buffer_is_empty();
#else
    return true;
#endif
}

static void replay(bool pressed) {
    keyrecord_t record = {.event = MAKE_KEYEVENT(0, 0, pressed)};
    record.event.time  = timer_read() | 1;
    replaying          = true;
    process_record_kb(pending_kc, &record);
    replaying = false;
}

static uint32_t switch_host_cb(uint32_t trigger_time, void *arg) {
    if (!queue_empty() && waited_ms < HOST_ANNOUNCE_MAX_EXTRA_MS) {
        waited_ms += 10;
        return 10; // try again in 10 ms
    }
    replay(true);
    if (pending_up) replay(false);
    pending_kc = KC_NO;
    return 0;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (replaying || keycode < BT_HST1 || keycode > BT_HST3) return true;

    if (!record->event.pressed) {
        if (keycode != pending_kc) return true; // release after the switch: normal path
        pending_up = true;                      // quick tap: replayed by the callback
        return false;
    }

    if (pending_kc != KC_NO) return false; // a switch is already under way

    uint8_t target = keycode - BT_HST1 + 1;
    // Announce only when there is someone to tell and the keyboard will really leave.
    if (get_transport() != TRANSPORT_BLUETOOTH || wireless_get_state() != WT_CONNECTED || connected_host == 0 || target == connected_host) {
        return true;
    }

    tap_code16(announce_keys[target - 1]);
    pending_kc = keycode;
    pending_up = false;
    waited_ms  = 0;
    defer_exec(HOST_ANNOUNCE_DELAY_MS, switch_host_cb, NULL);
    return false;
}
