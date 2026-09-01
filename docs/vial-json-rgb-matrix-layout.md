# `rgb_matrix.layout` for key_colors/audio_visualizer

To assign colors to LEDs, tools like `key_colors`' WebGUI and
`audio_visualizer`'s companion (`audio-visualizer-gui`) need an
`rgb_matrix.layout` — it maps LED indices to physical positions and key
matrix coordinates.

## Example

```json
"rgb_matrix": {
    "layout": [
        { "matrix": [0, 0], "x": 0,   "y": 0,  "flags": 4 },
        { "matrix": [0, 1], "x": 16,  "y": 0,  "flags": 4 },
        { "x": 0, "y": 64, "flags": 2 }
    ]
}
```

Two ways to provide it:

## Option A: bake into `vial.json` (permanent, Vial boards)

Copy the `rgb_matrix.layout` array from your board's `keyboard.json`
into your keymap's `vial.json`, under a top-level `"rgb_matrix": {
"layout": [...] }` key. If your board defines LEDs via a hand-written
`g_led_config` instead, derive the array manually from its `point[]`
(x/y per LED) and `flags[]` arrays, same order as the LED index.

This embeds the layout in the firmware — tools read it automatically
over the Vial protocol.

## Option B: export + load as a file (also works without Vial)

```sh
qmk info -kb <keyboard> -km <keymap> -m -f json > keyboard_info.json
```

Load that file in the tool instead:
- `audio-visualizer-gui`: ☰ menu → "LED layout override" → select the file
- `key_colors`' WebGUI: "Load layout…" button

Same `rgb_matrix.layout` shape as Option A, just read from a local file
instead of over HID.
