#pragma once

#ifdef SPLIT_KEYBOARD

// Split-sync RPC IDs — see viz_relay.c. Folded into the shared cross-keyboard
// transaction-id enum by QMK's module build system; numeric slots are
// auto-assigned, so these never collide with a keymap's own
// SPLIT_TRANSACTION_IDS_USER entries as long as the identifier names differ.
#define SPLIT_TRANSACTION_IDS_MODULE_VIZ_RELAY \
    VIZ_RELAY_SYNC_RGB_DIRECT, VIZ_RELAY_ENTRY_WAVE_STARTUP

#endif // SPLIT_KEYBOARD
