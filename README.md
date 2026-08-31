# QMK Modules

## Install

```sh
cd /path/to/vial-qmk
git submodule add https://github.com/kolbenhans/qmk-modules.git modules/kolbenhans
git submodule update --init --recursive
```

Then enable the module(s) you want in your keymap's `keymap.json` (see each
module below for its identifier) — required, this is how `qmk compile` knows
which modules to pull in for that keymap. If your keymap folder doesn't have
a `keymap.json` yet, create one with just the `modules` array shown below.

If it already has one — for example it already lists `srwi/keypeek_layer_notify`
for KeyPeek support — don't add a second `"modules": [...]` block. A JSON
file can't have the same key twice; whichever one QMK reads last would
silently win and the other module would just not load. Instead, add the
new module as another entry in the array that's already there, so it ends
up listing all of them together, e.g. `"modules": ["srwi/keypeek_layer_notify", "kolbenhans/key_colors"]`.

## `key_colors`

Host-assigned per-key/per-layer RGB, EEPROM-persisted, delta-synced across
split halves. Colors are drawn by a custom RGB Matrix effect the module
brings with it, `RGB_MATRIX_COMMUNITY_MODULE_key_colors` — how you activate
that effect (boot default and or a keycode) is up to
you, see below.

No dependency on `srwi/keypeek_layer_notify` — if your keymap also lists that
module, `key_colors_hid.c` auto-detects it (`__has_include`) and forwards its
packets; nothing to configure either way.

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

`EECONFIG_KB_DATA_SIZE` is sized automatically (`key_colors/config.h`, from
`DYNAMIC_KEYMAP_LAYER_COUNT * RGB_MATRIX_LED_COUNT * 7`) — no need to set it
yourself unless you want to shrink it.

> [!IMPORTANT]
> **Split boards only** — add this on top of the `config.h` block above.
> Non-split needs nothing more _in `config.h`_ — the `hid_glue` file and
> `rules.mk` line below are still required on every board:
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
SRC += key_colors_hid.c
```

Copy [`hid_glue/key_colors_hid.c`](hid_glue/key_colors_hid.c) — `hid_glue/` lives
in _this_ repo (`qmk-modules/hid_glue/`), not in your build — into your
keymap folder, beside its `rules.mk` — same file for split and non-split.

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

**`keymap.c`** (optional but recommended — a way back into `key_colors` mode if something
else, e.g. `RM_NEXT`, switches the RGB effect away from it):

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
Already have a `process_record_user`? Add the `case` to your existing `switch`.

---

## `viz_relay`

Entry-wave transition + renders whatever's pushed into VialRGB's
`g_direct_mode_colors` (host-side FASTSET). Also drawn by a custom RGB
Matrix effect the module brings with it, `RGB_MATRIX_COMMUNITY_MODULE_viz_frame`
— activation is up to you, see below.

**`keymap.json`:**

```json
{
    "modules": ["kolbenhans/viz_relay"]
}
```

**`config.h`:**

```c
#define VIALRGB_ENABLE
```

> [!IMPORTANT]
> **Split boards only** — add this on top of the `config.h` block above
> (`g_direct_mode_colors` already covers the whole board directly on
> non-split, there's no second half to relay to). The `hid_glue` file and
> `rules.mk` line below are still required on every board:
>
> ```c
> #define RPC_M2S_BUFFER_SIZE <at least (RGB_MATRIX_LED_COUNT / 2) * 3>
>
> // only if you get "undeclared identifier" (vial-qmk older than
> // SPLIT_TRANSACTION_IDS_MODULE_* support):
> #define SPLIT_TRANSACTION_IDS_USER \
>     VIZ_RELAY_SYNC_RGB_DIRECT, VIZ_RELAY_ENTRY_WAVE_STARTUP
> ```

**`rules.mk`:**

```make
SRC += viz_glue.c
```

Copy [`hid_glue/viz_glue.c`](hid_glue/viz_glue.c) — `hid_glue/` lives in
_this_ repo (`qmk-modules/hid_glue/`), not in your build — into your
keymap folder, beside its `rules.mk` — same file for split and non-split.

```
vial-qmk/
├── modules/
│   └── kolbenhans/
│       └── hid_glue/
│           └── viz_glue.c          # copy this to your keymap folder
└── keyboards/
    └── exampleKeyboard/
        └── keymaps/
            └── exampleKeymap/
                ├── keymap.json     # changes mentioned above
                ├── config.h        # changes mentioned above
                ├── rules.mk        # changes mentioned above
                ├── viz_glue.c      # <- copied from modules/kolbenhans/hid_glue/
                └── ...
```

## Running both modules together

Only one `raw_hid_receive_kb` can exist per keymap. Copy
[`hid_glue/rgb_glue.c`](hid_glue/rgb_glue.c) — again from _this_ repo's
`hid_glue/` — into your keymap folder, beside its `rules.mk`, instead of
the two files above.

**`rules.mk`:**

```make
SRC += rgb_glue.c
```
