#pragma once

#include "quantum.h"

// Raw HID layer control for the layerhook host app
// (https://github.com/kolbenhans/layerhook). Returns true if consumed.
bool layerhook_hid_handle_command(uint8_t *data, uint8_t length);
