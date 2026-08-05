# Jackass MK1 porting guide

## Source contract

The `master` branch starts at QMK tag `0.33.13` (`332fa30e173e5b0ecc0c70ff166974b6db86525e`). The Keychron source is the `2025q3` branch at `07bfc38a4b11b8dac7ab758dfc5868b4229499ca`.

The port deliberately used two phases:

1. Import the Keychron K2 HE sources without modification. Commit `41e0fd9f66` records the exact K2 HE/common import, and commit `452b2d748f` records the exact Keychron SNLED27351 SPI driver import.
2. Reduce those imported sources in place for the K2 HE ANSI board and the requested feature set.

This preserves a reviewable Keychron source trail. The implementation is a reduction and board-local QMK compatibility adaptation, not a rewrite using analogous upstream QMK features.

The repository also contains:

- `keychron-2025q3`, tracking `keychron/2025q3`.
- `qmk-master`, tracking `qmk/master`.
- `master`, based on QMK `0.33.13` and containing Jackass MK1.

## Resulting scope

Everything specific to the keyboard lives below `keyboards/jackass/mk1`. No source path reaches into `keyboards/keychron` or modifies QMK core.

Retained Keychron behavior:

- K2 HE ANSI Hall sensor scanning, calibration, and external calibration EEPROM.
- Two regular-trigger profiles, persisted through QMK wear-leveling.
- LKBT51 Bluetooth and 2.4 GHz transport, HID reports, host pairing/reconnection, battery telemetry, charging state, low-battery shutdown, remote wake, and low-power handling.
- The stock physical Bluetooth/2.4 GHz/wired mode selection and Mac/Windows layer selection.
- NKRO and Keychron's Mac/Windows shortcut macros.
- The K2 HE LED map and Keychron SNLED27351 SPI driver.

Removed behavior and source:

- ISO and JIS layouts and all non-default keymaps.
- VIA, raw HID, factory testing, and retail/demo paths.
- Joystick, gamepad, XInput, SOCD, OKMC, rapid trigger, and actuation toggle.
- RGB effects, color controls, retail lighting, and connection-state lighting.
- Generic board/MCU/driver branches not used by the K2 HE ANSI hardware.

The only visible lighting states are solid white, red Caps Lock, the battery gauge, and the requested one-second profile confirmations.

## Board-local QMK 0.33.13 adaptations

Keychron's branch differs from QMK `0.33.13`. The compatibility changes are intentionally local:

- `keyboard.json` uses QMK's custom matrix, custom RGB Matrix driver, STM32F401, DFU, dip-switch, and embedded-flash wear-leveling declarations.
- `drivers/rgb_driver.c` adapts the retained Keychron SNLED27351 SPI calls to QMK's four-function `rgb_matrix_driver_t` interface.
- `debounce.c` preserves Keychron's no-debounce copy behavior using QMK `0.33.13`'s custom debounce signature.
- Keychron GPIO calls use the equivalent QMK `0.33.13` GPIO names.
- The Hall scan writes its resulting rows through QMK's current custom-matrix callback.
- Keyboard EEPROM data is versioned and validated before profile data is used; valid external Hall calibration is restored after a keyboard-data reset.

Do not replace these paths with similar QMK-native implementations when rebasing. Start from the newer Keychron source, import it exactly, and then replay the focused reductions and local interface adaptations.

## Profiles and bindable actions

The default profile is profile 1:

| Keycode | Action | Actuation | Confirmation |
| --- | --- | ---: | --- |
| `JM_PROF1` | Select work profile | 2.5 mm | Full red, 1 second |
| `JM_PROF2` | Select play profile | 1.5 mm | Full yellow, 1 second |
| `JM_PROF_NEXT` | Cycle profile 1/2 | Selected profile | Selected profile color, 1 second |
| `BAT_LVL` | Show battery gauge | n/a | White gauge, 3 seconds |

`JM_PROF_NEXT` defaults to the physical screenshot key between F12 and Delete. `BAT_LVL` defaults to Fn+B. These are normal keyboard keycodes in `common/keychron_common.h`, so a source keymap can bind them elsewhere.

## Docker workflow

The host edits the bind-mounted repository. All QMK setup, lint, formatting, and compilation run in the container; no SSH or Codex process is required inside it.

Start the environment:

    docker compose up -d --build
    docker exec qmk-jackass git submodule update --init --recursive

Validate and compile:

    docker exec qmk-jackass qmk lint -kb jackass/mk1
    docker exec qmk-jackass qmk compile -kb jackass/mk1 -km default

For a clean verification build:

    docker exec qmk-jackass qmk clean -a
    docker exec qmk-jackass qmk compile -kb jackass/mk1 -km default

The generated firmware is `jackass_mk1_default.bin` at the repository root.

## Copying into a clean QMK tree

The board is self-contained. Starting with QMK `0.33.13` and initialized QMK submodules, copy only the vendor directory:

    cp -R keyboards/jackass /path/to/qmk_firmware/keyboards/

Then compile in that QMK tree:

    qmk compile -kb jackass/mk1 -km default

No `keyboards/keychron`, VIA definition, core patch, or external board-local file is required.

## Updating the tracking branches

Keep `qmk-master` as a fast-forward mirror of upstream QMK master:

    git fetch qmk master
    git switch qmk-master
    git merge --ff-only qmk/master

Likewise, refresh the Keychron reference branch without merging it into the port:

    git fetch keychron 2025q3
    git switch keychron-2025q3
    git merge --ff-only keychron/2025q3

Return to `master` before port work. A future rebase should repeat the exact-import/reduction method rather than merging either reference branch wholesale.
