#pragma once

#ifdef SPLIT_KEYBOARD

// Split-sync RPC IDs. Folded into the shared cross-keyboard transaction-id
// enum by QMK's module build system (quantum/split_common/transaction_id_define.h)
// — numeric slots are auto-assigned by the compiler, so these never collide
// with a keymap's own SPLIT_TRANSACTION_IDS_USER entries as long as the
// identifier names differ. See key_colors.c for what each one carries.
#define SPLIT_TRANSACTION_IDS_MODULE_KEY_COLORS \
    KEY_COLORS_COLORS_DELTA, KEY_COLORS_BLINK_DELTA, KEY_COLORS_LOCK_FLAGS_DELTA, \
    KEY_COLORS_COMMIT, KEY_COLORS_STARTUP

// Required for lock-gated LEDs (Num/Caps/Scroll) to resolve correctly on
// both halves: each half now resolves its own cache independently from its
// own host_keyboard_led_state() (delta-synced tables), instead of only the
// master resolving and broadcasting the finished result.
#ifndef SPLIT_LED_STATE_ENABLE
#    define SPLIT_LED_STATE_ENABLE
#endif

// Same reasoning, for layer_state: each half now resolves cache_rebuild()'s
// get_highest_layer(layer_state) itself instead of only the master doing so.
// Layer switches are only ever processed on the master (that's where key
// presses land) — without this, the non-master half's layer_state never
// changes from its boot value and every layer past the first renders wrong
// on that half. (Found the hard way: WebGUI colors synced fine, but layer
// switches only showed up on one half — see git history for the report.)
#ifndef SPLIT_LAYER_STATE_ENABLE
#    define SPLIT_LAYER_STATE_ENABLE
#endif

#endif // SPLIT_KEYBOARD
