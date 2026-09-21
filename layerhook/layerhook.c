#include "layerhook.h"

#ifdef RAW_ENABLE

// 0x02 <cmd> ...  — same family byte as the other kolbenhans modules.
//   0xB0 SET_LAYER <layer>  force-switch (layer_move), echoes the request
//   0xB1 GET_LAYER          replies with the highest active layer
bool layerhook_hid_handle_command(uint8_t *data, uint8_t length) {
    if (length < 2 || data[0] != 0x02) return false;

    uint8_t resp[32] = {0x02, data[1]};

    switch (data[1]) {
        case 0xB0:
            if (length < 3 || data[2] >= MAX_LAYER) return true;
            layer_move(data[2]);
            resp[2] = data[2];
            break;

        case 0xB1:
            resp[2] = get_highest_layer(layer_state);
            break;

        default:
            return false;
    }

    host_raw_hid_send(resp, sizeof(resp));
    return true;
}

#endif // RAW_ENABLE
