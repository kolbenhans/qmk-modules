// Keymap-level Raw HID glue for the kolbenhans/layerhook module.
// Community modules can't hook raw_hid_receive_kb (not in QMK's
// module-hookable API list) — this file owns it when layerhook is used alone.
#include "layerhook.h"

#ifdef RAW_ENABLE

#ifndef LAYERHOOK_DISABLE_RAW_HID_HANDLER

// keypeek: optional, auto-detected only if the keymap also lists
// srwi/keypeek_layer_notify in keymap.json.
#if __has_include("keypeek_layer_notify.h")
#    include "keypeek_layer_notify.h"
#    define LAYERHOOK_HAS_KEYPEEK
#endif

void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
#ifdef LAYERHOOK_HAS_KEYPEEK
    if (keypeek_handle_command(data, length)) return;
#endif
    layerhook_hid_handle_command(data, length);
}

#endif // LAYERHOOK_DISABLE_RAW_HID_HANDLER

#endif // RAW_ENABLE
