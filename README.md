# QMK Modules

## Install

```sh
cd /path/to/vial-qmk
git submodule add https://github.com/kolbenhans/qmk-modules.git modules/kolbenhans
git submodule update --init --recursive
```

Enable the module(s) you want in your keymap's `keymap.json` (identifier per
module below). No `keymap.json` yet? Create one with just the `modules`
array shown below.

Already have one? Merge into its existing `"modules": [...]` array — don't
add a second block (duplicate JSON key, one silently wins).

## `key_colors`

Host-assigned per-key/per-layer RGB, EEPROM-persisted, delta-synced across
split halves. Custom RGB Matrix effect: `RGB_MATRIX_COMMUNITY_MODULE_key_colors`.

**`keymap.json`:**

```json
{
    "modules": ["kolbenhans/key_colors"]
}
```

**`config.h`:**

```c
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_COMMUNITY_MODULE_key_colors // optional — boot default effect
#define WEAR_LEVELING_BACKING_SIZE (WEAR_LEVELING_LOGICAL_SIZE * 2) // only if build fails to fit
```

> [!IMPORTANT]
> **Split boards only**, on top of the `config.h` block above:
>
> ```c
> // only if you get "undeclared identifier" (vial-qmk older than
> // SPLIT_TRANSACTION_IDS_MODULE_* support):
> #define SPLIT_TRANSACTION_IDS_USER \
>     KEY_COLORS_COLORS_DELTA, KEY_COLORS_BLINK_DELTA, KEY_COLORS_LOCK_FLAGS_DELTA, \
>     KEY_COLORS_COMMIT, KEY_COLORS_STARTUP
> ```

**`rules.mk`:**

```make
RGB_MATRIX_ENABLE = yes #if not enabled yet
SRC += key_colors_hid.c
```

Copy [`hid_glue/key_colors_hid.c`](hid_glue/key_colors_hid.c) into your keymap
folder, beside its `rules.mk` — same file for split and non-split.

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

**`keymap.c`** (optional — way back into `key_colors` mode if another rgb effect had been activated):

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

Place `USER01` somewhere in your `LAYOUT()` (or define a key later in VIAL/Pipette).
Already have a `process_record_user`? Add just the `case` to your existing `switch`.

---

## `audio_visualizer`

Entry-wave transition + renders whatever's pushed into VialRGB's
`g_direct_mode_colors` (host-side FASTSET). Custom RGB Matrix effect:
`RGB_MATRIX_COMMUNITY_MODULE_audio_visualizer`.

**`keymap.json`:**

```json
{
    "modules": ["kolbenhans/audio_visualizer"]
}
```

**`config.h`:**

```c
#define VIALRGB_ENABLE
```

> [!IMPORTANT]
> **Split boards only**, on top of the `config.h` block above:
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
SRC += audio_visualizer_hid.c
```

Copy [`hid_glue/audio_visualizer_hid.c`](hid_glue/audio_visualizer_hid.c) into
your keymap folder, beside its `rules.mk` — same file for split and non-split.

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

Copy each module's own `hid_glue` file — they auto-detect each other and
compose, no extra setup, no combined file to pick instead.
