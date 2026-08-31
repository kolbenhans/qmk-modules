# Requirement: `rgb_matrix.layout` in your keymap's `vial.json`

Not needed to build or flash `key_colors`/`viz_relay` themselves — needed by
their companion host-side tools once you actually want to use them:

- **`key_colors`' WebGUI**: its whole wire protocol (`key_colors_hid.c`)
  addresses LEDs purely by raw `led_offset` — there's no firmware command
  that answers "which LED index belongs to the key at row/col X,Y". For the
  WebGUI to let someone click a key on a rendered keyboard and set *that
  key's* color, it needs a client-side matrix → LED-index mapping. The only
  place that mapping is available over the Vial protocol is
  `rgb_matrix.layout`'s `matrix` field (paired with each entry's position in
  the array, which *is* the LED index). Without it, the WebGUI can't
  correctly address any key.
- **`viz_relay`'s companion (`viz-gui-rs`)**: needs the same mapping *plus*
  `x`/`y`, for its spatially-aware effects (Bars/Waterdrop) to place color
  by physical LED position, not just by index.

So both need it, for overlapping but distinct reasons — one for correct
per-key addressing, the other additionally for spatial rendering.

## What's missing, and why it's almost certainly missing on your board

Your keymap's `vial.json` needs a `rgb_matrix` block with a `layout` array —
one entry per LED, giving its physical `x`/`y` position and, for per-key
LEDs, which key matrix position it belongs to:

```json
"rgb_matrix": {
    "layout": [
        { "matrix": [0, 0], "x": 0,   "y": 0,  "flags": 4 },
        { "matrix": [0, 1], "x": 16,  "y": 0,  "flags": 4 },
        { "x": 0, "y": 64, "flags": 2 }
    ]
}
```

This is a real, verified gap — not a guess. A scan across the entire
`vial-qmk/keyboards` tree found 83 boards that both (a) define
`rgb_matrix.layout` in their `keyboard.json` and (b) ship a `vial` keymap.
**Zero of those 83 carry the layout over into `vial.json`.** `sofle_choc`,
`sofle_pico`, and `keebart/sofle_choc_pro` are three concrete examples, but
this is the norm, not the exception.

That's not board-maintainer negligence. Vial's own GUI never asks for it —
its Lighting tab (VialRGB) only controls global effect parameters (mode,
hue, sat, val, speed), not individual LED positions or per-key addressing,
so there's never been a reason for stock keymaps to populate this. It's
purely a requirement of *external* tooling — `key_colors`' WebGUI and
`viz-gui-rs` alike — that needs to address or place LEDs individually, read
live over HID via the Vial protocol at runtime.

## Why your firmware build works fine without it

`rgb_matrix.layout` in `vial.json` and the LED layout your firmware
actually uses at runtime (`g_led_config`, sourced from either your board's
`keyboard.json` `rgb_matrix.layout` or a hand-written `led_config_t` in a
board `.c` file) are **two completely independent paths**:

- `g_led_config` is what QMK's `rgb_matrix` subsystem uses to physically
  drive your LEDs. Comes from `keyboard.json` (auto-generated, `weak` symbol)
  or your board's own `.c` file (strong symbol, silently wins if both exist
  — no error, no warning either way).
- `vial.json` is compressed as-is (`util/vial_generate_definition.py` just
  minifies + LZMA-compresses the whole file, no parsing/validation of its
  contents) into a header embedded in firmware, served over HID via the
  Vial protocol.

Nothing at build time cross-checks these two against each other. Your board
lights up correctly regardless of what — if anything — `vial.json`'s
`rgb_matrix` section contains. The failure mode is entirely on the
*consumer* side: a tool expecting `rgb_matrix.layout` either falls back to
a generic default layout or fails to read a layout at all — silently, no
build error anywhere to point you at this file.

## What to do about it

Two options — pick based on how permanent you want the fix, and whether
your board even has a `vial.json` in the first place:

### Option A: bake it into `vial.json` (permanent, Vial boards only)

1. **Your board's `keyboard.json` already has `rgb_matrix.layout`?** (Most
   boards using QMK's newer data-driven config do.) Copy that array
   verbatim into your keymap's `vial.json`, under a top-level `"rgb_matrix":
   { "layout": [...] }` key.
2. **Your board defines LEDs via a hand-written `g_led_config` in a `.c`
   file instead** (custom boards, e.g. our own `id75v3rp`/`sofle_rgb`)?
   Derive the array manually from that struct's `point[]` (x/y per LED) and
   `flags[]` arrays — same order as the LED index, `"matrix"` is optional
   per entry (omit it for underglow-only LEDs that aren't tied to a key).

Field format is QMK's own schema (`data/schemas/keyboard.jsonschema`,
`rgb_matrix.layout`): `x`/`y` required, `matrix` (`[row, col]`) and `flags`
optional per entry. Works forever afterward — no per-session step, the
layout is embedded in firmware and read automatically on every connect.

### Option B: export + load fallback (also covers plain-VIA boards)

Requires no `vial.json` edit at all — works even for boards that don't use
Vial at all (`VIAL_ENABLE` off), which don't have a `vial.json` to edit in
the first place and so can't use Option A. Same idea KeyPeek uses for its
own layout requirement on plain QMK/VIA keyboards (its README, step 5):
export once locally,

```sh
qmk info -kb <keyboard> -km <keymap> -f json > keyboard_info.json
```

then point the consuming tool at that file when the live-read layout is
missing. `viz-gui-rs` supports this today: ☰ menu → "LED layout override" →
paste the exported file's path → Load. Same `rgb_matrix.layout` JSON shape
as Option A, just read from a local file instead of over HID — see
`hid::load_led_layout_file()` / `hid::parse_rgb_matrix_layout()` in
`viz-gui-rs/src/hid.rs`, shared with the device-fetch path so both stay in
sync. `key_colors`' WebGUI doesn't have this fallback yet (separate
codebase, not covered here) — worth adding the same pattern there.

Downside vs. Option A: has to be redone (or at least re-selected) if you
ever run the tool on a different machine, or if you clear/lose the saved
path — it's a per-install convenience setting, not baked into the keyboard
itself.

> [!IMPORTANT]
> This is unrelated to the `g_led_config` "weak symbol" gotcha (a board's
> own `.c` definition silently overriding one auto-generated from
> `keyboard.json`) — that's a *build-time* firmware concern. This
> `vial.json` requirement is a completely separate, *runtime* concern for
> external tooling. Don't conflate the two when troubleshooting either one.
