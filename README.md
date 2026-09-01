# QMK Modules

## Install

```sh
cd /path/to/vial-qmk
git submodule add https://github.com/kolbenhans/qmk-modules.git modules/kolbenhans
git submodule update --init --recursive
```

Add the module identifier (below) to your keymap's `keymap.json`
`"modules": [...]` array. No `keymap.json` yet? Create one with just that
array. Already have one with entries? Add to the existing array — don't
add a second `"modules"` block (duplicate JSON key, one silently wins).

**LED layout data** ([`key_colors`' WebGUI](https://github.com/kolbenhans/qmk-webgui),
[`audio_visualizer`'s companion](https://github.com/kolbenhans/audio-visualizer-gui)):
see [docs/vial-json-rgb-matrix-layout.md](docs/vial-json-rgb-matrix-layout.md).

## `key_colors`

Per-key/per-layer RGB, EEPROM-persisted, split-synced. Effect:
`RGB_MATRIX_COMMUNITY_MODULE_key_colors`.

**`keymap.json`:**

```json
{
    "modules": ["kolbenhans/key_colors"]
}
```

**`config.h`:**

> [!IMPORTANT]
> **Split boards only**,
>
> ```c
> // vial-qmk build if you get "undeclared identifier"
> #define SPLIT_TRANSACTION_IDS_USER \
>     KEY_COLORS_COLORS_DELTA, KEY_COLORS_BLINK_DELTA, KEY_COLORS_LOCK_FLAGS_DELTA, \
>     KEY_COLORS_COMMIT, KEY_COLORS_STARTUP
> ```

```c
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_COMMUNITY_MODULE_key_colors // optional — boot default effect
// uncomment if build fails to fit
// #define WEAR_LEVELING_BACKING_SIZE (WEAR_LEVELING_LOGICAL_SIZE * 2)
```

**`rules.mk`:**

```make
RGB_MATRIX_ENABLE = yes #if not enabled yet
SRC += key_colors_hid.c
```

Copy [`hid_glue/key_colors_hid.c`](hid_glue/key_colors_hid.c) into your keymap
folder, beside `rules.mk`.

```
vial-qmk/
├── modules/
│   └── kolbenhans/
│       └── hid_glue/
│           └── key_colors_hid.c     # copy this to your keymap folder
└── keyboards/
    └── exampleKeyboard/
        └── keymaps/
            └── exampleKeymap/
                ├── keymap.json      # changes mentioned above
                ├── config.h         # changes mentioned above
                ├── rules.mk         # changes mentioned above
                ├── key_colors_hid.c # <- copied from modules/kolbenhans/hid_glue/
                └── ...
```

**`keymap.c`** (optional — key to switch back into `key_colors` mode):

```c
#include "key_colors.h"

#define KEYBIND_USER01 0x7E01 // any unused custom keycode value works

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KEYBIND_USER01:
            if (record->event.pressed) {
                key_colors_on_mode_enter();
                rgb_matrix_mode_noeeprom(RGB_MATRIX_COMMUNITY_MODULE_key_colors);
            }
            return false;
    }
    return true;
}
```

Place `USER01` in your `LAYOUT()` (or bind later in VIAL/Pipette). Existing
`process_record_user`? Add just the `case`.

---

## `audio_visualizer`

Entry-wave transition + renders VialRGB's `g_direct_mode_colors` (host FASTSET).
Effect: `RGB_MATRIX_COMMUNITY_MODULE_audio_visualizer`.

**`keymap.json`:**

```json
{
    "modules": ["kolbenhans/audio_visualizer"]
}
```

> [!IMPORTANT]
> **Split boards only**, in `config.h`:
>
> ```c
> #define RPC_M2S_BUFFER_SIZE <at least (RGB_MATRIX_LED_COUNT / 2) * 3>
>
> // only if you get "undeclared identifier" (vial-qmk older than
> // SPLIT_TRANSACTION_IDS_MODULE_* support):
> #define SPLIT_TRANSACTION_IDS_USER \
>     AUDIO_VISUALIZER_SYNC_RGB_DIRECT, AUDIO_VISUALIZER_ENTRY_WAVE_STARTUP
> ```

**`rules.mk`:**

```make
RGB_MATRIX_ENABLE = yes # required
VIALRGB_ENABLE = yes    # required — skip if your keymap already sets this
SRC += audio_visualizer_hid.c
```

Copy [`hid_glue/audio_visualizer_hid.c`](hid_glue/audio_visualizer_hid.c) into
your keymap folder, beside `rules.mk`.

```
vial-qmk/
├── modules/
│   └── kolbenhans/
│       └── hid_glue/
│           └── audio_visualizer_hid.c  # copy this to your keymap folder
└── keyboards/
    └── exampleKeyboard/
        └── keymaps/
            └── exampleKeymap/
                ├── keymap.json         # changes mentioned above
                ├── config.h            # changes mentioned above
                ├── rules.mk            # changes mentioned above
                ├── audio_visualizer_hid.c # <- copied from modules/kolbenhans/hid_glue/
                └── ...
```

---

## Running modules together

Copy each module's own `hid_glue` file. Auto-composes, no extra setup.
