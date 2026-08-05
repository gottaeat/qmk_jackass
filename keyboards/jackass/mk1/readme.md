# Jackass MK1

Jackass MK1 is a focused ANSI firmware target for the Keychron K2 HE hardware. It behaves as a conventional keyboard with two fixed Hall-effect actuation profiles while retaining the stock Keychron Bluetooth, 2.4 GHz, wired, battery, charging, sleep/wake, NKRO, and wear-leveling paths.

- Maintainer: [gottaeat](https://github.com/gottaeat)
- Hardware: Keychron K2 HE ANSI
- Firmware repository: [gottaeat/qmk_jackass](https://github.com/gottaeat/qmk_jackass)

## Default behavior

- Profile 1 (work): 2.5 mm actuation and a one-second full-board red confirmation.
- Profile 2 (play): 1.5 mm actuation and a one-second full-board yellow confirmation.
- The key between F12 and Delete cycles profiles.
- `JM_PROF1`, `JM_PROF2`, and `JM_PROF_NEXT` are bindable keyboard keycodes.
- Fn+B (`BAT_LVL`) shows the battery level.
- Lighting is solid white. Caps Lock is red while active.
- The physical Mac/Windows layer switch and Bluetooth/2.4 GHz/wired mode selector work through the retained Keychron paths.
- The left modifiers are Ctrl, Option/Alt, Meta/GUI in both OS modes.

There is no VIA, factory-test, game-controller, XInput, joystick, SOCD, rapid-trigger, actuation-toggle, lighting-effect, or lighting-control support.

## Build

From the QMK repository root, with the project container running:

    docker exec qmk-jackass qmk compile -kb jackass/mk1 -km default

The output is `jackass_mk1_default.bin`.

To enter the STM32 DFU bootloader, disconnect USB, set the mode switch to Cable, hold Esc or the reset button under the space bar, and reconnect USB.

See the repository-level `PORTING_GUIDE.md` for provenance, Docker setup, portability, and validation details.
