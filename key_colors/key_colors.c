#include QMK_KEYBOARD_H
#ifdef SPLIT_KEYBOARD
#    include "transactions.h"
#endif
#include "key_colors.h"
#include <string.h>

ASSERT_COMMUNITY_MODULES_MIN_API_VERSION(1, 0, 0);

#if defined(RGB_MATRIX_ENABLE)

// ─── Startup animation ───────────────────────────────────────────────────────

#define STARTUP_STEP_MS    10
#define STARTUP_TAIL_WIDTH 84

// ─── Per-key/per-layer color table ────────────────────────────────────────────
// Host (WebGUI, via keymap-level Raw HID glue) owns this data. Firmware never
// derives color from keycode/keymap content — only from LED index + active
// layer. RGB_MATRIX_LED_COUNT is QMK's own per-board define, so this table is
// sized correctly for whatever board the module is compiled into.

typedef struct { uint8_t r, g, b; } rgb_color_t;

static rgb_color_t key_colors[DYNAMIC_KEYMAP_LAYER_COUNT][RGB_MATRIX_LED_COUNT];
static rgb_color_t blink_colors[DYNAMIC_KEYMAP_LAYER_COUNT][RGB_MATRIX_LED_COUNT];
static uint8_t      key_lock_flags[DYNAMIC_KEYMAP_LAYER_COUNT][RGB_MATRIX_LED_COUNT];
static bool         colors_dirty = false;

// Set true whenever any LED on any layer carries LOCK_FLAG_BLINK — gates the
// 800ms phase-flip check in render_lighting_range() so boards with no blink
// keys configured never pay the extra cache_rebuild() tax.
static bool any_blink = false;

static void recompute_any_blink(void) {
    any_blink = false;
    for (uint8_t layer = 0; layer < DYNAMIC_KEYMAP_LAYER_COUNT && !any_blink; layer++) {
        for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; led++) {
            if (key_lock_flags[layer][led] & LOCK_FLAG_BLINK) {
                any_blink = true;
                break;
            }
        }
    }
}

static struct {
    uint32_t anim_timer;
    bool     done;
} startup;

static struct {
    rgb_color_t   colors[RGB_MATRIX_LED_COUNT];
    layer_state_t layer_state;
    uint8_t       led_state_raw; // host_keyboard_led_state().raw — für Lock-Flag-Gating
    uint8_t       blink_phase;   // (timer_read32()/800)%2 as of last rebuild
    bool          valid;
} cache;

// Stateless — recomputed on demand, no stored animation timer. 800ms/phase =
// 1.6s full cycle.
static rgb_color_t slow_blink_pick(rgb_color_t a, rgb_color_t b) {
    return (timer_read32() / 800) % 2 ? a : b;
}

// ─── EEPROM ────────────────────────────────────────────────────────────────

static void load_colors(void) {
    if (eeconfig_is_kb_datablock_valid()) {
        eeconfig_read_kb_datablock(key_colors, 0, sizeof(key_colors));
        eeconfig_read_kb_datablock(blink_colors, sizeof(key_colors), sizeof(blink_colors));
        eeconfig_read_kb_datablock(key_lock_flags, sizeof(key_colors) + sizeof(blink_colors), sizeof(key_lock_flags));
    } else {
        memset(key_colors, 0, sizeof(key_colors));
        memset(blink_colors, 0, sizeof(blink_colors));
        memset(key_lock_flags, 0, sizeof(key_lock_flags));
    }
    recompute_any_blink();
}

// ─── Local mutators ──────────────────────────────────────────────────────────
// Pure array writes, no split-sync awareness — called directly on whichever
// half receives a delta (see "Split sync" below), and by the public setters
// below on the initiating half.

static void apply_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes) {
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT) return;
    for (uint8_t i = 0; i < count && (uint16_t)led_offset + i < RGB_MATRIX_LED_COUNT; i++) {
        rgb_color_t *c = &key_colors[layer][led_offset + i];
        c->r = rgb_bytes[i * 3 + 0];
        c->g = rgb_bytes[i * 3 + 1];
        c->b = rgb_bytes[i * 3 + 2];
    }
    colors_dirty = true;
}

static void apply_blink_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes) {
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT) return;
    for (uint8_t i = 0; i < count && (uint16_t)led_offset + i < RGB_MATRIX_LED_COUNT; i++) {
        rgb_color_t *c = &blink_colors[layer][led_offset + i];
        c->r = rgb_bytes[i * 3 + 0];
        c->g = rgb_bytes[i * 3 + 1];
        c->b = rgb_bytes[i * 3 + 2];
    }
    colors_dirty = true;
}

static void apply_lock_flags(uint8_t layer, uint8_t led, uint8_t flags) {
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT || led >= RGB_MATRIX_LED_COUNT) return;
    key_lock_flags[layer][led] = flags;
    colors_dirty = true;
    recompute_any_blink();
}

// ─── Split sync (delta) ──────────────────────────────────────────────────────
// Only compiled for split boards — a non-split board has no other half to
// sync to, the public setters below just apply locally in that case.
//
// Each mutation is mirrored to the other half as the same small chunk the
// host sent — not a full-table broadcast. The receiving half only applies;
// it never re-forwards (avoids RPC ping-pong). Both halves end up with
// byte-identical key_colors[]/blink_colors[]/key_lock_flags[] and each
// resolves its own render cache independently (see render_lighting_range).

// WebGUI chunk sizes — bounded by the 32-byte raw HID report the keymap-level
// glue uses (5-byte header: family, subcmd, layer, led_offset, count).
#define KEY_COLORS_MAX_COLOR_CHUNK 9  // 3 bytes/LED
#define KEY_COLORS_MAX_FLAG_CHUNK  27 // 1 byte/LED

#ifdef SPLIT_KEYBOARD

static void colors_delta_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;
    const uint8_t *p = in_data;
    if (in_buflen < 3 || !p) return;
    uint8_t count = p[2];
    if ((uint16_t)3 + (uint16_t)count * 3 > in_buflen) return;
    apply_colors(p[0], p[1], count, &p[3]);
}

static void blink_delta_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;
    const uint8_t *p = in_data;
    if (in_buflen < 3 || !p) return;
    uint8_t count = p[2];
    if ((uint16_t)3 + (uint16_t)count * 3 > in_buflen) return;
    apply_blink_colors(p[0], p[1], count, &p[3]);
}

static void lock_flags_delta_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)out_buflen;
    (void)out_data;
    const uint8_t *p = in_data;
    if (in_buflen < 3 || !p) return;
    apply_lock_flags(p[0], p[1], p[2]);
}

static void commit_delta_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)in_buflen;
    (void)in_data;
    (void)out_buflen;
    (void)out_data;
    // Persist THIS half's own (already delta-synced) tables to its own
    // EEPROM — each half's flash is independent, so a commit on one side
    // never implicitly persists the other.
    eeconfig_update_kb_datablock(key_colors, 0, sizeof(key_colors));
    eeconfig_update_kb_datablock(blink_colors, sizeof(key_colors), sizeof(blink_colors));
    eeconfig_update_kb_datablock(key_lock_flags, sizeof(key_colors) + sizeof(blink_colors), sizeof(key_lock_flags));
}

static void startup_sync_handler(uint8_t in_buflen, const void *in_data, uint8_t out_buflen, void *out_data) {
    (void)in_buflen;
    (void)in_data;
    (void)out_buflen;
    (void)out_data;
    startup.anim_timer = 0;
    startup.done       = false;
    cache.valid        = false;
}

#endif // SPLIT_KEYBOARD

// ─── Public API ──────────────────────────────────────────────────────────────

void key_colors_set_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes) {
    if (count > KEY_COLORS_MAX_COLOR_CHUNK) count = KEY_COLORS_MAX_COLOR_CHUNK;
    apply_colors(layer, led_offset, count, rgb_bytes);
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        uint8_t buf[3 + KEY_COLORS_MAX_COLOR_CHUNK * 3];
        buf[0] = layer;
        buf[1] = led_offset;
        buf[2] = count;
        memcpy(&buf[3], rgb_bytes, (size_t)count * 3);
        transaction_rpc_send(KEY_COLORS_COLORS_DELTA, 3 + (uint16_t)count * 3, buf);
    }
#endif
}

void key_colors_get_colors(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_rgb_bytes) {
    memset(out_rgb_bytes, 0, (size_t)count * 3);
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT) return;
    for (uint8_t i = 0; i < count && (uint16_t)led_offset + i < RGB_MATRIX_LED_COUNT; i++) {
        rgb_color_t c            = key_colors[layer][led_offset + i];
        out_rgb_bytes[i * 3 + 0] = c.r;
        out_rgb_bytes[i * 3 + 1] = c.g;
        out_rgb_bytes[i * 3 + 2] = c.b;
    }
}

void key_colors_set_blink_colors(uint8_t layer, uint8_t led_offset, uint8_t count, const uint8_t *rgb_bytes) {
    if (count > KEY_COLORS_MAX_COLOR_CHUNK) count = KEY_COLORS_MAX_COLOR_CHUNK;
    apply_blink_colors(layer, led_offset, count, rgb_bytes);
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        uint8_t buf[3 + KEY_COLORS_MAX_COLOR_CHUNK * 3];
        buf[0] = layer;
        buf[1] = led_offset;
        buf[2] = count;
        memcpy(&buf[3], rgb_bytes, (size_t)count * 3);
        transaction_rpc_send(KEY_COLORS_BLINK_DELTA, 3 + (uint16_t)count * 3, buf);
    }
#endif
}

void key_colors_get_blink_colors(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_rgb_bytes) {
    memset(out_rgb_bytes, 0, (size_t)count * 3);
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT) return;
    for (uint8_t i = 0; i < count && (uint16_t)led_offset + i < RGB_MATRIX_LED_COUNT; i++) {
        rgb_color_t c            = blink_colors[layer][led_offset + i];
        out_rgb_bytes[i * 3 + 0] = c.r;
        out_rgb_bytes[i * 3 + 1] = c.g;
        out_rgb_bytes[i * 3 + 2] = c.b;
    }
}

void key_colors_set_lock_flags(uint8_t layer, uint8_t led, uint8_t flags) {
    apply_lock_flags(layer, led, flags);
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        uint8_t buf[3] = {layer, led, flags};
        transaction_rpc_send(KEY_COLORS_LOCK_FLAGS_DELTA, sizeof(buf), buf);
    }
#endif
}

void key_colors_get_lock_flags(uint8_t layer, uint8_t led_offset, uint8_t count, uint8_t *out_flags) {
    memset(out_flags, 0, count);
    if (layer >= DYNAMIC_KEYMAP_LAYER_COUNT) return;
    for (uint8_t i = 0; i < count && (uint16_t)led_offset + i < RGB_MATRIX_LED_COUNT; i++) {
        out_flags[i] = key_lock_flags[layer][led_offset + i];
    }
}

void key_colors_commit_colors(void) {
    eeconfig_update_kb_datablock(key_colors, 0, sizeof(key_colors));
    eeconfig_update_kb_datablock(blink_colors, sizeof(key_colors), sizeof(blink_colors));
    eeconfig_update_kb_datablock(key_lock_flags, sizeof(key_colors) + sizeof(blink_colors), sizeof(key_lock_flags));
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        transaction_rpc_send(KEY_COLORS_COMMIT, 0, NULL);
    }
#endif
}

// ─── Cache ───────────────────────────────────────────────────────────────────
// Repaints from key_colors[] only when the active layer changes, new color/
// lock-flag data arrives (colors_dirty), or the live lock state changes
// (needed for lock-gated LEDs to react instantly) — not on every rgb_matrix
// tick otherwise. Both halves run this identically against their own
// (delta-synced) tables and their own host_keyboard_led_state() — no
// master/slave asymmetry, unlike the old full-cache-broadcast design.

static void cache_rebuild(void) {
    uint8_t layer     = get_highest_layer(layer_state);
    led_t   led_state = host_keyboard_led_state();

    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; led++) {
        uint8_t flags   = key_lock_flags[layer][led];
        bool    visible = !(((flags & LOCK_FLAG_NUM) && !led_state.num_lock) ||
                          ((flags & LOCK_FLAG_CAPS) && !led_state.caps_lock) ||
                          ((flags & LOCK_FLAG_SCRL) && !led_state.scroll_lock));
        if (!visible) {
            cache.colors[led] = (rgb_color_t){0, 0, 0};
        } else if (flags & LOCK_FLAG_BLINK) {
            cache.colors[led] = slow_blink_pick(key_colors[layer][led], blink_colors[layer][led]);
        } else {
            cache.colors[led] = key_colors[layer][led];
        }
    }

    cache.layer_state   = layer_state;
    cache.led_state_raw = led_state.raw;
    cache.blink_phase   = (timer_read32() / 800) % 2;
    cache.valid         = true;
}

static void cache_flush_range(uint8_t led_min, uint8_t led_max) {
    uint8_t v = rgb_matrix_config.hsv.v;
    for (uint8_t led = led_min; led < led_max && led < RGB_MATRIX_LED_COUNT; led++) {
        rgb_color_t c = cache.colors[led];
        rgb_matrix_set_color(led, (uint16_t)c.r * v / 255, (uint16_t)c.g * v / 255, (uint16_t)c.b * v / 255);
    }
}

// ─── Render dispatch ─────────────────────────────────────────────────────────

static void render_lighting_range(uint8_t led_min, uint8_t led_max) {
    bool content_stale = !cache.valid || cache.layer_state != layer_state || colors_dirty ||
                          cache.led_state_raw != host_keyboard_led_state().raw ||
                          (any_blink && ((timer_read32() / 800) % 2 != cache.blink_phase));

    if (content_stale) {
        cache_rebuild();
        colors_dirty = false;
    }

    cache_flush_range(led_min, led_max);
}

// ─── Startup animation ───────────────────────────────────────────────────────

static void startup_tick(uint8_t led_min, uint8_t led_max) {
    if (startup.anim_timer == 0) startup.anim_timer = timer_read32();

    int16_t  head    = (int16_t)(timer_elapsed32(startup.anim_timer) / STARTUP_STEP_MS * 10);
    uint16_t max_pos = 0;

    for (uint8_t led = 0; led < RGB_MATRIX_LED_COUNT; led++) {
        uint8_t  x       = g_led_config.point[led].x;
        uint8_t  y       = g_led_config.point[led].y;
        uint8_t  row     = y / 13;
        uint16_t local_x = (row % 2) ? 220 - x : x;
        uint16_t pos     = (uint16_t)(row * 240) + local_x;
        if (pos > max_pos) max_pos = pos;
    }

    for (uint8_t led = led_min; led < led_max && led < RGB_MATRIX_LED_COUNT; led++) {
        uint8_t  x       = g_led_config.point[led].x;
        uint8_t  y       = g_led_config.point[led].y;
        uint8_t  row     = y / 13;
        uint16_t local_x = (row % 2) ? 220 - x : x;
        uint16_t pos     = (uint16_t)(row * 240) + local_x;
        int16_t  dist    = head - (int16_t)pos;

        if (dist < 0 || dist >= STARTUP_TAIL_WIDTH) {
            rgb_matrix_set_color(led, 0, 0, 0);
            continue;
        }

        uint8_t value = 255 - (uint8_t)((uint16_t)dist * 255 / STARTUP_TAIL_WIDTH);
        uint8_t hue   = (uint8_t)(timer_elapsed32(startup.anim_timer) / 8) + (uint8_t)(pos / 2);
        RGB     rgb   = hsv_to_rgb((HSV){hue, 255, value});
        rgb_matrix_set_color(led, rgb.r, rgb.g, rgb.b);
    }

    if (head > (int16_t)(max_pos + STARTUP_TAIL_WIDTH)) {
        startup.done = true;
        cache.valid  = false;
    }
}

static void startup_reset(void) {
    startup.anim_timer = 0;
    startup.done       = false;
    cache.valid        = false;
}

// ─── Module lifecycle ────────────────────────────────────────────────────────

void keyboard_post_init_key_colors(void) {
    load_colors();
#ifdef SPLIT_KEYBOARD
    transaction_register_rpc(KEY_COLORS_COLORS_DELTA, colors_delta_handler);
    transaction_register_rpc(KEY_COLORS_BLINK_DELTA, blink_delta_handler);
    transaction_register_rpc(KEY_COLORS_LOCK_FLAGS_DELTA, lock_flags_delta_handler);
    transaction_register_rpc(KEY_COLORS_COMMIT, commit_delta_handler);
    transaction_register_rpc(KEY_COLORS_STARTUP, startup_sync_handler);
#endif
    startup_reset();
}

void key_colors_on_mode_enter(void) {
    startup_reset();
#ifdef SPLIT_KEYBOARD
    if (is_keyboard_master()) {
        transaction_rpc_send(KEY_COLORS_STARTUP, 0, NULL);
    }
#endif
}

void key_colors_render(uint8_t led_min, uint8_t led_max) {
    if (!startup.done) {
        startup_tick(led_min, led_max);
        return;
    }
    render_lighting_range(led_min, led_max);
}

#endif // RGB_MATRIX_ENABLE
