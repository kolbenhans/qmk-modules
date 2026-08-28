#pragma once

#include "quantum.h"

void key_colors_on_mode_enter(void);
void key_colors_render(uint8_t led_min, uint8_t led_max);

// Host-assigned per-key/per-layer colors. Wire protocol (Raw HID opcodes,
// chunk framing, etc.) is a keymap-level concern — this module only exposes
// plain array mutators/accessors and does not touch raw_hid_receive_kb.
void key_colors_set_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes);
void key_colors_get_colors(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_rgb_bytes);
void key_colors_commit_colors(void);

// Second color a LED alternates with while LOCK_FLAG_BLINK is set —
// persisted/committed alongside key_colors above.
void key_colors_set_blink_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes);
void key_colors_get_blink_colors(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_rgb_bytes);

// Per-key/per-layer lock-state gating — a flagged LED only shows its
// assigned color while the given lock is active (live
// host_keyboard_led_state(), independent of keymap/keycode content).
#define LOCK_FLAG_NUM   (1 << 0)
#define LOCK_FLAG_CAPS  (1 << 1)
#define LOCK_FLAG_SCRL  (1 << 2)
// Alternates the LED between key_colors and blink_colors every 800ms while
// otherwise visible (composes with the lock flags above: they gate on/off
// first, this picks which of the two colors shows while "on").
#define LOCK_FLAG_BLINK (1 << 3)
void key_colors_set_lock_flags(uint8_t layer, uint8_t led, uint8_t flags);
void key_colors_get_lock_flags(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_flags);
