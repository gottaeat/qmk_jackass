# Jackass MK1 porting guide

## Source contract

The `master` branch starts at QMK tag `0.33.13` (`332fa30e173e5b0ecc0c70ff166974b6db86525e`). The Keychron source is the `2025q3` branch at `07bfc38a4b11b8dac7ab758dfc5868b4229499ca`.

The port deliberately used two phases:

1. Import the Keychron K2 HE sources without modification. Commit `41e0fd9f66` records the exact K2 HE/common import, and commit `452b2d748f` records the exact Keychron SNLED27351 SPI driver import.
2. Reduce those imported sources in place for the K2 HE ANSI board and the requested feature set.

This preserves a reviewable Keychron source trail. The implementation is a reduction and board-local QMK compatibility adaptation, not a rewrite using analogous upstream QMK features.

The import was rechecked with Git object IDs: the source/import blobs for the K2 HE board file, LKBT51 module, wireless transport, and both SNLED27351 SPI driver files are byte-identical. A full tree comparison also reports no differences between Keychron's common subtree and the copy recorded by the exact-import commit.

Keychron 2025q3 also pins `Keychron/ChibiOS` commit `41e112ce20d85dd8be9362777fbf107586d1cbd2`, rather than QMK's ChibiOS. Its commit `ba10f3a80` fixes `BOARD_OTG_NOVBUSSENS` on STM32 OTGv1. That fix is required by the K2 HE because PA9 is the keyboard's mode-select input but is also STM32's hardware USB-VBUS sense pin. `drivers/hal_usb_lld.h` applies Keychron's no-op USB connect/disconnect macros as a board-local overlay; QMK core and its submodule remain unchanged.

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
- The stock Bluetooth host and 2.4 GHz connection-state beacons: number keys 1-3 for Bluetooth hosts and number key 4 for 2.4 GHz.
- Fn+B battery indication on the number row.
- Keychron's Mac Mission Control and Launchpad actions on F3/F4.
- Static-white brightness adjustment and backlight on/off state, persisted through QMK's RGB Matrix configuration.
- NKRO and Keychron's Mac/Windows shortcut macros.
- The K2 HE LED map and Keychron SNLED27351 SPI driver.

Removed behavior and source:

- ISO and JIS layouts and all non-default keymaps.
- VIA, raw HID, factory testing, and retail/demo paths.
- Joystick, gamepad, XInput, SOCD, OKMC, rapid trigger, and actuation toggle.
- RGB effects, hue/saturation/speed controls, and retail lighting.
- Generic board/MCU/driver branches not used by the K2 HE ANSI hardware.

The only visible lighting states are adjustable solid white, red Caps Lock, green/blue OS-layout status on the left GUI key, red/blue/green transport status on Esc, the persistent red/yellow profile key, white wireless beacons with the rest of the board off, and the white number-row battery gauge. The backlight can be toggled off, but no effects or color controls are exposed. Cable mode forces the backlight off while USB power is absent.

## K2 HE dependency graph audit

The reduction was checked from QMK's generated dependency files and final object manifest, not only from the handwritten makefiles. Every C file below `keyboards/jackass/mk1` is consumed by the build: 22 appear as board-path objects, `board.c` and `debounce.c` appear as QMK keyboard objects, and `keymaps/default/keymap.c` is consumed by QMK's generated keymap translation unit.

The retained paths are:

1. QMK startup calls bootmagic, which scans the custom matrix at the default row 0/column 0 position. That K2 HE ANSI position is Esc, so holding Esc while connecting USB enters the STM32 DFU bootloader.
2. `mk1.c` and the custom matrix callback drive Keychron's analog scan, Hall calibration, regular-trigger action, and external calibration EEPROM code. Profile selection feeds the scan's actuation point and persists through QMK embedded-flash wear-leveling.
3. `board.c` initializes Keychron common code. That initializes the LKBT51 module, report buffer, wireless state machine, battery measurement, RTC timer, and STM32F401 low-power implementation.
4. The A9/A10 physical selector is debounced by `wireless_pre_task()`. Its unchanged K2 mapping selects Bluetooth, 2.4 GHz, or USB and then calls Keychron's retained `set_transport()` path.
5. Bluetooth and 2.4 GHz commands and interrupt events pass through `lkbt51.c`, `wireless.c`, and `report_buffer.c`. Connection events feed the reduced-from-Keychron indicator state machine. Bluetooth host indices 1-3 map to LED indices 17-19; Keychron's 2.4 GHz host index 24 maps to LED index 20, the number 4 key.
6. Fn+1/2/3 and Fn+4 reach `keychron_wireless_common.c`. A Bluetooth host tap reconnects/selects that host; holding a Bluetooth host or the 2.4 GHz key for two seconds invokes Keychron pairing. Fn+B reaches the retained battery measurement and the focused number-row renderer while operating wirelessly on battery.
7. Each housekeeping pass runs the physical selector, LKBT51 event parser, indicator timer, pairing-hold timer, battery task, and low-power task. RGB rendering applies brightness-scaled static white, Caps Lock and the persistent key markers, wireless/battery indication, and finally the cable-unplugged blackout.

The closure audit also checked every board-local header against QMK's generated dependency files and every handwritten board-local object against the final linker map. Every header is included by the build, and the linker discards no non-empty section from those objects. QMK's generated weak LED-map fallback is discarded as expected because `mk1.c` provides the retained Keychron LED map. GCC 15.2's static analyzer reports no issues across the 25 board-local C translation units. A whole-tree analyzer build proceeds through all of them before stopping on an analyzer-only out-of-bounds report in QMK `0.33.13`'s `quantum/action.c`; the normal warning-as-error firmware build is clean.

Only two conditional compilation checks remain in the board source. The active STM32 OTGv1 compatibility check applies Keychron's required `BOARD_OTG_NOVBUSSENS` handling, and the LKBT51 driver asserts that SPI is enabled. There are no inactive feature branches in the retained keyboard code.

The clean-build compile flags contain the intentional board options for the USB startup path, Cortex idle behavior, and RGB brightness-off threshold, plus QMK's bootmagic, DIP-switch, NKRO, RGB Matrix, EEPROM, and embedded-flash wear-leveling feature defines. Dead Keychron feature-marker defines were removed. The flags do not contain VIA, factory-test, joystick, gamepad, XInput, SOCD, OKMC, rapid-trigger, dynamic-keymap, or toggle-feature defines.

## Board-local QMK 0.33.13 adaptations

Keychron's branch differs from QMK `0.33.13`. The compatibility changes are intentionally local:

- `keyboard.json` uses QMK's custom matrix, custom RGB Matrix driver, STM32F401, DFU, dip-switch, and embedded-flash wear-leveling declarations.
- `drivers/rgb_driver.c` adapts the retained Keychron SNLED27351 SPI calls to QMK's four-function `rgb_matrix_driver_t` interface.
- The retained SNLED27351 initialization now replays Keychron's software PWM shadow after the hardware PWM RAM is cleared. This makes the stock transport-change driver reinitialization restore both LED chips immediately instead of waiting for later indications to dirty them one at a time.
- `drivers/hal_usb_lld.h` carries Keychron/ChibiOS commit `ba10f3a80` locally so QMK 0.33.13 does not reclaim PA9 for USB-VBUS sensing and break the physical transport selector.
- `debounce.c` preserves Keychron's no-debounce copy behavior using QMK `0.33.13`'s custom debounce signature.
- Keychron GPIO calls use the equivalent QMK `0.33.13` GPIO names.
- The Hall scan writes its resulting rows through QMK's current custom-matrix callback.
- Keyboard EEPROM data is versioned and validated before profile data is used; valid external Hall calibration is restored after a keyboard-data reset.
- The EEPROM staging buffer uses fixed local storage instead of Keychron's unchecked startup heap allocation; its size, contents, and load order are unchanged.
- Profile changes preserve the reduced regular-trigger state and the active thresholds of pressed keys until release. Keychron's full implementation clears state because profiles can change action types; here that could strand a reported press, while changing thresholds mid-press could release and reactuate the profile key during one downstroke.

Do not replace these paths with similar QMK-native implementations when rebasing. Start from the newer Keychron source, import it exactly, and then replay the focused reductions and local interface adaptations.

## Profiles and bindable actions

The default profile is profile 1:

| Keycode | Action | Actuation | Confirmation |
| --- | --- | ---: | --- |
| `JM_PROF_NEXT` | Cycle profile 1/2 | Selected profile | Bound key stays in the selected profile color; the rest of the board is unchanged |
| `BAT_LVL` | Show battery gauge | n/a | Board off except white number-row gauge, 3 seconds |

`JM_PROF_NEXT` defaults to the physical screenshot key between F12 and Delete. Its persistent red/yellow marker follows the key's currently resolved matrix position, so a source keymap can bind it elsewhere without changing an LED index. `BAT_LVL` defaults to Fn+B. Both are normal keyboard keycodes in `common/keychron_common.h`.

## Stock switch and wireless behavior

- The top switch retains Keychron's single-DIP mapping: Mac selects layers 0/1 and Windows selects layers 2/3. Both base layers use Ctrl, Option/Alt, Meta/GUI on the bottom left. The active left GUI key is green in Mac mode and blue in Windows mode.
- The mode selector retains the K2 pin mapping and order for 2.4 GHz, cable, and Bluetooth. Esc is red in wired mode, blue in Bluetooth mode, and green in 2.4 GHz mode. In cable mode the backlight is forced off until USB power is present.
- The top-right lighting key toggles the backlight in either OS mode. Its blue marker dynamically follows whichever key actively resolves to `UG_TOGG`, without a hardcoded LED index. Mac F5/F6 adjust static-white brightness directly; Windows uses Fn+F5/F6. On/off and brightness are persisted, while mode, hue, saturation, and speed changes remain unavailable.
- Mac F3/F4 invoke Keychron's Mission Control and Launchpad consumer actions. Fn+F3/F4 retain ordinary F3/F4.
- Entering/reconnecting Bluetooth blacks out the board and blinks the selected host's number key. Fn+1/2/3 selects the three hosts; holding a host key for two seconds starts pairing.
- Entering/reconnecting 2.4 GHz blacks out the board and blinks number 4. Holding Fn+4 for two seconds retains Keychron's receiver-pairing action.
- Fn+B in either battery-powered wireless mode blacks out the board, lights one white number-row key per 10% battery for three seconds, and then restores the prior static-white on/off state.
- In cable mode, holding Esc while connecting USB triggers QMK bootmagic at matrix row 0/column 0 and jumps to STM32 DFU.

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
