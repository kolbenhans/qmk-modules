// Keymap-level Raw HID glue for the kolbenhans/viz_relay module. Community
// modules can't hook raw_hid_receive_kb (not in QMK's module-hookable API
// list) — mode-switch dispatch stays here. This keymap's own keycode-derived
// engine (dynamicLights.c) is unrelated and untouched by this file.
#include "dynamicLights.h"
#include "viz_relay.h"
#include "keypeek_layer_notify.h" // keypeek

#ifdef RAW_ENABLE

// Mode-switch commands sent by the Python viz tool (viz_hid.py).
void raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    // --- keypeek: claims its own subscribe/keepalive packets, ignores everything else ---
    if (keypeek_handle_command(data, length)) return;
    // --- end keypeek ---

    if (length < 2 || data[0] != 0x02) return;

    switch (data[1]) {
        case 0xA3:
            viz_relay_trigger();
            rgb_matrix_mode_noeeprom(RGB_MATRIX_COMMUNITY_MODULE_viz_frame);
            break;

        case 0xA4:
            dynamic_lights_on_mode_enter();
            rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_dynamic_lights);
            break;
    }
}

#endif // RAW_ENABLE
