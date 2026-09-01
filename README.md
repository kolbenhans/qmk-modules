# QMK Modules

QMK Community Modules for custom RGB keyboard firmware.

This repository currently provides two modules:

| Module                                  | Description                                                                     |
| --------------------------------------- | ------------------------------------------------------------------------------- |
| [`key_colors`](#key_colors)             | Per-key, per-layer RGB colors with EEPROM persistence and split synchronization |
| [`audio_visualizer`](#audio_visualizer) | Audio-reactive RGB relay using VialRGB direct-mode data                         |

Both modules are designed for **Vial-QMK** and are currently used by several custom keyboard projects.

---

## Modules

### `key_colors`

Custom per-key and per-layer RGB colors with persistent storage and split-keyboard synchronization.

The module provides the RGB Matrix effect:

```text
RGB_MATRIX_COMMUNITY_MODULE_key_colors
```

Features include:

* Per-key RGB colors
* Per-layer color configuration
* EEPROM persistence
* Split synchronization
* Blink colors
* Lock-state handling
* Browser-based WebGUI support
* Optional keybinding to return to the `key_colors` RGB mode

The WebGUI communicates with the firmware through HID.

---

### `audio_visualizer`

An audio-reactive RGB relay for QMK/VialRGB.

The module receives RGB data from the host application and renders it through QMK's RGB Matrix system.

The module provides the RGB Matrix effect:

```text
RGB_MATRIX_COMMUNITY_MODULE_audio_visualizer
```

It supports:

* VialRGB direct-mode colors
* Host-driven RGB updates
* Entry-wave startup animation
* Split synchronization

The audio analysis itself is performed on the host. The keyboard firmware receives the resulting data and handles the actual RGB rendering.

The host-side application is:

[**kolbenhans/audio-visualizer-gui**](https://github.com/kolbenhans/audio-visualizer-gui)

---

# Installation

These modules use QMK's **Community Modules** mechanism.

The following examples assume a Vial-QMK checkout at:

```text
~/projects/vial-qmk
```

## 1. Add the repository

```bash
cd ~/projects/vial-qmk

git submodule add \
    https://github.com/kolbenhans/qmk-modules.git \
    modules/kolbenhans

git submodule update --init --recursive
```

After installation, the relevant directory structure is:

```text
vial-qmk/
├── modules/
│   └── kolbenhans/
│       ├── audio_visualizer/
│       ├── docs/
│       ├── hid_glue/
│       └── key_colors/
└── ...
```

---

## 2. Enable a module

Add the module identifier to the `"modules"` array in your keymap's `keymap.json`.

For `key_colors`:

```json
{
    "modules": [
        "kolbenhans/key_colors"
    ]
}
```

For `audio_visualizer`:

```json
{
    "modules": [
        "kolbenhans/audio_visualizer"
    ]
}
```

If your `keymap.json` already contains a `"modules"` array, add the module to that existing array.

**Do not create a second `"modules"` property.** Duplicate JSON keys can result in one definition silently overriding the other.

Multiple modules can be enabled together:

```json
{
    "modules": [
        "kolbenhans/key_colors",
        "kolbenhans/audio_visualizer"
    ]
}
```

---

# `key_colors`

## Basic configuration

Add the module to `keymap.json`:

```json
{
    "modules": [
        "kolbenhans/key_colors"
    ]
}
```

### `config.h`

For split keyboards, older Vial-QMK versions may require the module's transaction IDs to be added manually:

```c
#define SPLIT_TRANSACTION_IDS_USER \
    KEY_COLORS_COLORS_DELTA, \
    KEY_COLORS_BLINK_DELTA, \
    KEY_COLORS_LOCK_FLAGS_DELTA, \
    KEY_COLORS_COMMIT, \
    KEY_COLORS_STARTUP
```

If your Vial-QMK version supports `SPLIT_TRANSACTION_IDS_MODULE_*`, this manual definition should not be necessary.

Optionally set `key_colors` as the default RGB Matrix mode:

```c
#define RGB_MATRIX_DEFAULT_MODE \
    RGB_MATRIX_COMMUNITY_MODULE_key_colors
```

If the build fails because the firmware does not fit into the available wear-leveling storage, increase the backing size:

```c
#define WEAR_LEVELING_BACKING_SIZE \
    (WEAR_LEVELING_LOGICAL_SIZE * 2)
```

Only add this when required by the target keyboard.

---

## `rules.mk`

Make sure RGB Matrix is enabled:

```make
RGB_MATRIX_ENABLE = yes
```

The HID glue source file is also required:

```make
SRC += key_colors_hid.c
```

Copy:

```text
modules/kolbenhans/hid_glue/key_colors_hid.c
```

into the keymap directory, next to `rules.mk`.

The resulting structure should look like:

```text
vial-qmk/
├── modules/
│   └── kolbenhans/
│       └── hid_glue/
│           └── key_colors_hid.c
│
└── keyboards/
    └── exampleKeyboard/
        └── keymaps/
            └── exampleKeymap/
                ├── keymap.json
                ├── config.h
                ├── rules.mk
                ├── key_colors_hid.c
                └── ...
```

---

## Optional keybinding

A key can be assigned to return to the `key_colors` RGB Matrix mode.

Include the module header:

```c
#include "key_colors.h"
```

Define an unused custom keycode:

```c
#define KEYBIND_USER01 0x7E01
```

Then handle it in `process_record_user()`:

```c
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KEYBIND_USER01:
            if (record->event.pressed) {
                key_colors_on_mode_enter();
                rgb_matrix_mode_noeeprom(
                    RGB_MATRIX_COMMUNITY_MODULE_key_colors
                );
            }
            return false;
    }

    return true;
}
```

Bind `USER01` in your layout or later through Vial/Pipette.

If your keymap already has a `process_record_user()` function, add only the corresponding `case` rather than creating a second function.

---

# `audio_visualizer`

## Basic configuration

Add the module to `keymap.json`:

```json
{
    "modules": [
        "kolbenhans/audio_visualizer"
    ]
}
```

For split keyboards, the module requires sufficient RPC buffer space for the RGB data.

### `config.h`

Set:

```c
#define RPC_M2S_BUFFER_SIZE \
    <at least (RGB_MATRIX_LED_COUNT / 2) * 3>
```

For example, if the keyboard has 60 LEDs:

```c
#define RPC_M2S_BUFFER_SIZE 90
```

The required size is based on the number of LEDs on one half:

```text
(RGB_MATRIX_LED_COUNT / 2) × 3
```

The factor of `3` accounts for the RGB channels.

---

## Older Vial-QMK versions

If your Vial-QMK version does not yet provide the module transaction-ID mechanism, you may also need:

```c
#define SPLIT_TRANSACTION_IDS_USER \
    AUDIO_VISUALIZER_SYNC_RGB_DIRECT, \
    AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP
```

This is only required when the build reports the corresponding transaction identifiers as undeclared.

---

## `rules.mk`

The following features are required:

```make
RGB_MATRIX_ENABLE = yes
VIALRGB_ENABLE = yes
```

If `VIALRGB_ENABLE` is already enabled by the keymap, do not add it a second time.

Add the HID glue source:

```make
SRC += audio_visualizer_hid.c
```

Copy:

```text
modules/kolbenhans/hid_glue/audio_visualizer_hid.c
```

into the keymap directory next to `rules.mk`.

The resulting structure should look like:

```text
vial-qmk/
├── modules/
│   └── kolbenhans/
│       └── hid_glue/
│           └── audio_visualizer_hid.c
│
└── keyboards/
    └── exampleKeyboard/
        └── keymaps/
            └── exampleKeymap/
                ├── keymap.json
                ├── config.h
                ├── rules.mk
                ├── audio_visualizer_hid.c
                └── ...
```

---

# Using Both Modules

`key_colors` and `audio_visualizer` can be used together.

Enable both modules in `keymap.json`:

```json
{
    "modules": [
        "kolbenhans/key_colors",
        "kolbenhans/audio_visualizer"
    ]
}
```

Copy both HID glue files into the keymap:

```text
key_colors_hid.c
audio_visualizer_hid.c
```

No additional module-specific composition is required.

The modules automatically coexist and provide their respective RGB Matrix functionality.

A typical configuration therefore contains:

```text
keymap.json
config.h
rules.mk

key_colors_hid.c
audio_visualizer_hid.c
```

---

# HID Glue

The files in [`hid_glue/`](hid_glue/) are intentionally kept separate from the module directories.

They bridge the QMK module functionality with the keyboard/keymap-specific build environment.

### `key_colors_hid.c`

Required by the `key_colors` WebGUI/HID interface.

### `audio_visualizer_hid.c`

Required by the host-side audio visualizer application.

Both files must be copied into the keymap directory and added to the build through `rules.mk`.

---

# WebGUI

The `key_colors` module can be configured using the browser-based WebGUI:

[**kolbenhans/qmk-webgui**](https://github.com/kolbenhans/qmk-webgui)

The WebGUI communicates directly with compatible keyboards through WebHID.

It provides per-key and per-layer color configuration without requiring the firmware source to be modified.

The keyboard must expose the corresponding `key_colors` HID functionality.

For the required LED layout data, see:

[`docs/vial-json-rgb-matrix-layout.md`](docs/vial-json-rgb-matrix-layout.md)

---

# Audio Visualizer

The host-side audio visualizer is provided by:

[**kolbenhans/audio-visualizer-gui**](https://github.com/kolbenhans/audio-visualizer-gui)

The architecture is:

```text
┌──────────────────────────┐
│     Host Application     │
│                          │
│  Audio capture / FFT     │
│  Peak detection          │
│  RGB / palette handling  │
└────────────┬─────────────┘
             │
             │ USB HID
             ▼
┌──────────────────────────┐
│      QMK Firmware        │
│                          │
│    audio_visualizer      │
│           │              │
│           ▼              │
│       RGB Matrix         │
└──────────────────────────┘
```

The host performs the audio analysis. The keyboard firmware receives the resulting data and renders the RGB effect.

---

# LED Layout Data

Both the WebGUI and audio visualizer depend on correct RGB Matrix LED layout information.

See:

[`docs/vial-json-rgb-matrix-layout.md`](docs/vial-json-rgb-matrix-layout.md)

The layout data describes the physical LED positions used by the host-side tools.

Incorrect or missing layout information can result in colors being assigned to the wrong keys or visualizer effects appearing in the wrong positions.

---

# Troubleshooting

## `undeclared identifier`

If the build reports errors involving:

```text
KEY_COLORS_...
```

or:

```text
AUDIO_VISUALIZER_...
```

your Vial-QMK version may not provide the corresponding module transaction-ID support.

For `key_colors`:

```c
#define SPLIT_TRANSACTION_IDS_USER \
    KEY_COLORS_COLORS_DELTA, \
    KEY_COLORS_BLINK_DELTA, \
    KEY_COLORS_LOCK_FLAGS_DELTA, \
    KEY_COLORS_COMMIT, \
    KEY_COLORS_STARTUP
```

For `audio_visualizer`:

```c
#define SPLIT_TRANSACTION_IDS_USER \
    AUDIO_VISUALIZER_SYNC_RGB_DIRECT, \
    AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP
```

Use the definition appropriate to the module you are enabling.

---

## Firmware does not fit

The `key_colors` module stores its configuration persistently.

If the build reports that the wear-leveling storage does not fit, try:

```c
#define WEAR_LEVELING_BACKING_SIZE \
    (WEAR_LEVELING_LOGICAL_SIZE * 2)
```

The exact storage requirements depend on the keyboard's EEPROM/wear-leveling configuration.

---

## Audio visualizer fails on a split keyboard

Check:

```c
#define RPC_M2S_BUFFER_SIZE \
    <at least (RGB_MATRIX_LED_COUNT / 2) * 3>
```

The buffer must be large enough to transport the RGB data for one half of the keyboard.

---

## WebGUI cannot find the keyboard

Check:

1. The keyboard firmware contains `key_colors`.
2. `key_colors_hid.c` is present in the keymap.
3. The HID interface is compiled into the firmware.
4. The browser supports WebHID.
5. The keyboard is connected directly via USB.

---

# Used By

These modules are currently used in several custom keyboard projects:

* [**SofleRGB**](https://github.com/kolbenhans/SofleRGB)
* [**ymdk-id75v3rp-RGB**](https://github.com/kolbenhans/ymdk-id75v3rp-RGB)
* [**ymdk-BCorne_M57-RGB**](https://github.com/kolbenhans/ymdk-BCorne_M57-RGB)

They form the firmware-side building blocks for the per-key RGB and audio-reactive RGB functionality used by these projects.

---

# Repository Structure

```text
qmk-modules/
├── audio_visualizer/
├── docs/
│   └── vial-json-rgb-matrix-layout.md
├── hid_glue/
│   ├── audio_visualizer_hid.c
│   └── key_colors_hid.c
└── key_colors/
```

---

# Requirements

The modules are intended for **Vial-QMK** with Community Modules support.

Some configuration details depend on the Vial-QMK version, particularly the split transaction-ID mechanism.

For older Vial-QMK versions, the `SPLIT_TRANSACTION_IDS_USER` definitions described above may be required.

---

# License

See [`LICENSE`](LICENSE) for license information.
