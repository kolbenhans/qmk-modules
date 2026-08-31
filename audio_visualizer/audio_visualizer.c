#include QMK_KEYBOARD_H
#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#endif
#include "audio_visualizer.h"
#include <string.h>

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

#if defined(RGB_MATRIX_ENABLE)

#ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
extern HSV g_direct_mode_colors[RGB_MATRIX_LED_COUNT];
#endif

// ─── Entry wave trigger state ────────────────────────────────────────────────

static bool     entry_wave_active = false;
static uint32_t entry_wave_timer  = 0;

static void entry_wave_start(void) {
    entry_wave_active = true;
    entry_wave_timer  = timer_read32();
}

bool audio_visualizer_running(void) {
    return entry_wave_active;
}

uint32_t audio_visualizer_elapsed(void) {
    return timer_elapsed32(entry_wave_timer);
}

void audio_visualizer_stop(void) {
    entry_wave_active = false;
}

#ifdef SPLIT_KEYBOARD

static void entry_wave_sync_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)in_buflen;
    (void)in_data;
    (void)out_buflen;
    (void)out_data;
    entry_wave_start();
}

#endif // SPLIT_KEYBOARD

// Starts the wave locally and, on a split board, pushes it to the other half
// too — a plain entry_wave_start() only fires where it's called, which is
// always the master (raw HID / key events never reach the slave directly).
// Nothing to push on a non-split board — one half is the whole board.
void audio_visualizer_trigger(void) {
    entry_wave_start();
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        transaction_rpc_send(AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP, 0, NULL);
    }
#endif
}

// ─── Direct-mode (audio_visualizer) split sync ──────────────────────────────
// Split-only: g_direct_mode_colors is filled via Raw HID FASTSET, which only
// reaches the master half over USB — the slave half needs its portion pushed
// over the split link explicitly. On a non-split board g_direct_mode_colors
// already covers the whole board directly, nothing to relay.
//
// LED indices 0..SYNC_HALF_SIZE-1 are always the physical left half and
// SYNC_HALF_SIZE..RGB_MATRIX_LED_COUNT-1 the right half (fixed by
// g_led_config, independent of which side is plugged in as master) —
// is_keyboard_left() picks the right offset regardless of which physical
// side is master.

#if defined(SPLIT_KEYBOARD) && defined(RGB_MATRIX_EFFECT_VIALRGB_DIRECT)

#define SYNC_HALF_SIZE (RGB_MATRIX_LED_COUNT / 2)

static void rgb_direct_sync_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;
    if (in_buflen != SYNC_HALF_SIZE * sizeof(HSV) || in_data == NULL) return;
    uint8_t local_offset = is_keyboard_left() ? 0 : SYNC_HALF_SIZE;
    memcpy(&g_direct_mode_colors[local_offset], in_data, in_buflen);
}

#endif

void housekeeping_task_audio_visualizer(void) {
#if defined(SPLIT_KEYBOARD) && defined(RGB_MATRIX_EFFECT_VIALRGB_DIRECT)
    if (!is_keyboard_master()) return;
    if (rgb_matrix_get_mode() != RGB_MATRIX_COMMUNITY_MODULE_audio_visualizer) return;

    static uint32_t last_sync = 0;
    if (timer_elapsed32(last_sync) < 20) return;
    last_sync = timer_read32();

    uint8_t remote_offset = is_keyboard_left() ? SYNC_HALF_SIZE : 0;
    transaction_rpc_send(AUDIO_VISUALIZER_SYNC_RGB_DIRECT, SYNC_HALF_SIZE * sizeof(HSV), &g_direct_mode_colors[remote_offset]);
#endif
}

// ─── Module lifecycle ────────────────────────────────────────────────────────

void keyboard_post_init_audio_visualizer(void) {
#ifdef SPLIT_KEYBOARD
    transaction_register_rpc(AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP, entry_wave_sync_handler);
#    ifdef RGB_MATRIX_EFFECT_VIALRGB_DIRECT
    transaction_register_rpc(AUDIO_VISUALIZER_SYNC_RGB_DIRECT, rgb_direct_sync_handler);
#    endif
#endif
}

#endif // RGB_MATRIX_ENABLE
