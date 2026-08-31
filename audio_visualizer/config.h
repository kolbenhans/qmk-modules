#pragma once

// audio_visualizer_hid.c owns raw_hid_receive_kb when key_colors isn't
// present, chains keypeek in itself — stop it defining its own copy
// (duplicate symbol). No-op if keypeek isn't present.
#ifndef KEYPEEK_DISABLE_RAW_HID_HANDLER
#    define KEYPEEK_DISABLE_RAW_HID_HANDLER
#endif

#ifdef SPLIT_KEYBOARD

// Split-sync RPC IDs — see audio_visualizer.c. Folded into the shared
// cross-keyboard transaction-id enum by QMK's module build system; numeric
// slots are auto-assigned, so these never collide with a keymap's own
// SPLIT_TRANSACTION_IDS_USER entries as long as the identifier names differ.
#define SPLIT_TRANSACTION_IDS_MODULE_AUDIO_VISUALIZER \
    AUDIO_VISUALIZER_SYNC_RGB_DIRECT, AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP

#endif // SPLIT_KEYBOARD
