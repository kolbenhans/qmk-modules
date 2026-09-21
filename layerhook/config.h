#pragma once

// hid_glue/layerhook_hid.c owns raw_hid_receive_kb when neither key_colors nor
// audio_visualizer is present, chains keypeek in itself — stop keypeek
// defining its own copy (duplicate symbol). No-op if keypeek isn't present.
#ifndef KEYPEEK_DISABLE_RAW_HID_HANDLER
#    define KEYPEEK_DISABLE_RAW_HID_HANDLER
#endif
