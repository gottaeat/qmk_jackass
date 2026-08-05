# Jackass MK1

Jackass MK1 is a focused ANSI firmware target for the Keychron K2 HE hardware. It behaves as a conventional keyboard with two fixed Hall-effect actuation profiles while retaining the stock Keychron Bluetooth, 2.4 GHz, wired, battery, charging, sleep/wake, NKRO, and wear-leveling paths.

- Maintainer: [gottaeat](https://github.com/gottaeat)
- Hardware: Keychron K2 HE ANSI
- Firmware repository: [gottaeat/qmk_jackass](https://github.com/gottaeat/qmk_jackass)

## Default behavior

- Profile 1 (work): 2.5 mm actuation and a one-second full-board red confirmation.
- Profile 2 (play): 1.5 mm actuation and a one-second full-board yellow confirmation.
- The key between F12 and Delete cycles profiles.
- `JM_PROF_NEXT` is a bindable keyboard keycode. Whichever key actively resolves to it stays red for profile 1 or yellow for profile 2.
- Fn+B (`BAT_LVL`) turns off the board and shows the battery level in white across number keys 1-0 for three seconds while running wirelessly on battery.
- Lighting is solid white at the saved brightness. F5/F6 adjust brightness in Mac mode; Fn+F5/F6 do so in Windows mode. The key actively bound to `UG_TOGG` stays blue and toggles the backlight; it defaults to the top-right key. Caps Lock is red while active.
- Mac-mode F3 and F4 retain Keychron's Mission Control and Launchpad actions; Fn+F3/F4 send ordinary F3/F4.
- The physical Mac/Windows layer switch and Bluetooth/2.4 GHz/wired mode selector work through the retained Keychron paths.
- Bluetooth reconnect/pairing turns off the board and blinks the selected host key (Fn+1/2/3); 2.4 GHz does the same on number 4, with receiver pairing on a two-second Fn+4 hold.
- Cable mode keeps the backlight off until USB power is present.
- The left modifiers are Ctrl, Option/Alt, Meta/GUI in both OS modes.

There is no VIA, factory-test, game-controller, XInput, joystick, SOCD, rapid-trigger, actuation-toggle, lighting-effect, hue, saturation, or speed control support. Only static-white brightness and on/off control remain.

## Build

From the QMK repository root, with the project container running:

    docker exec qmk-jackass qmk compile -kb jackass/mk1 -km default

The output is `jackass_mk1_default.bin`.

To enter the STM32 DFU bootloader, disconnect USB, set the mode switch to Cable, hold Esc or the reset button under the space bar, and reconnect USB.

See the repository-level `PORTING_GUIDE.md` for provenance, Docker setup, portability, and validation details.
