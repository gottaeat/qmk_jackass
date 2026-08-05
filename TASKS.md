# Jackass MK1 tasks

## Completed in source

- [x] Base `master` on QMK `0.33.13`.
- [x] Add `qmk-master` tracking `qmk/master` and `keychron-2025q3` tracking `keychron/2025q3`.
- [x] Import the Keychron K2 HE `2025q3` sources exactly before reduction.
- [x] Move the ANSI target to `keyboards/jackass/mk1` with Jackass metadata.
- [x] Keep all board and compatibility code inside `keyboards/jackass/mk1`.
- [x] Reduce Hall behavior to regular triggering with two persisted profiles: 2.5 mm and 1.5 mm.
- [x] Add the bindable `JM_PROF_NEXT` profile-cycling action and bind it to the screenshot key.
- [x] Make the active `JM_PROF_NEXT` key stay red/yellow with the selected profile without hardcoding an LED index.
- [x] Bind battery level to Fn+B.
- [x] Restore Keychron's Mac Mission Control and Launchpad actions on F3/F4.
- [x] Restore only static-white brightness and backlight toggle controls, without restoring effects or color controls.
- [x] Make the active `UG_TOGG` key stay blue without hardcoding an LED index.
- [x] Retain Mac/Windows layers, shortcut macros, Ctrl-Option/Alt-Meta/GUI order, NKRO, wear-leveling, battery, charging, and low-power behavior.
- [x] Retain Keychron LKBT51 Bluetooth, 2.4 GHz, wired transport switching, reports, pairing, reconnection, and wake handling.
- [x] Port Keychron/ChibiOS's `BOARD_OTG_NOVBUSSENS` fix locally so PA9 remains the K2 HE mode-select input under QMK 0.33.13.
- [x] Retain the K2 HE Bluetooth host 1/2/3 and 2.4 GHz number-4 connection beacons with the rest of the backlight off.
- [x] Reduce lighting to white, red Caps Lock, white wireless/battery indication, and red/yellow profile confirmations.
- [x] Force cable-mode backlighting off until USB power is present.
- [x] Replay the SNLED PWM shadow after transport-change driver initialization so wired mode immediately restores the complete keyboard.
- [x] Remove non-ANSI layouts, VIA/raw HID, factory testing, retail/demo lighting, game-controller, joystick, XInput, SOCD, OKMC, rapid-trigger, and toggle sources.
- [x] Add Docker build files and the porting guide.
- [x] Audit the generated dependency/object manifest and confirm every board-local C file is consumed by the build.
- [x] Pass QMK keyboard lint in Docker.
- [x] Produce a QMK firmware binary in Docker.

## Hardware validation required

- [ ] Flash `jackass_mk1_default.bin` onto a K2 HE ANSI board and verify boot/reset behavior.
- [ ] Confirm every ANSI matrix position and Hall sensor calibrates and reports correctly.
- [ ] Measure profile 1 at 2.5 mm and profile 2 at 1.5 mm on representative switches.
- [ ] Confirm the screenshot key cycles only profiles 1 and 2 and persists the selected profile across power loss.
- [ ] Confirm the full board shows red for one second for profile 1 and yellow for one second for profile 2, then returns to white.
- [ ] Confirm Caps Lock is red and returns correctly after a battery/profile indication.
- [ ] Confirm Fn+B in battery-powered Bluetooth and 2.4 GHz modes turns off non-number LEDs, shows an accurate number-row battery gauge, and restores the prior backlight state after three seconds.
- [ ] Confirm Mac F3/F4 invoke Mission Control/Launchpad and Fn+F3/F4 send ordinary function keys.
- [ ] Confirm the top-right key toggles the backlight and F5/F6 brightness adjustment works in Mac mode and through Fn in Windows mode; verify both settings persist.
- [ ] Test USB typing, NKRO, suspend, and remote wake.
- [ ] Pair and reconnect all three Bluetooth host slots; confirm the selected number 1/2/3 beacon blinks with all other LEDs off, then test keyboard, consumer, system, mouse-wake, and NKRO reports.
- [ ] Pair and reconnect the 2.4 GHz receiver; confirm number 4 blinks with all other LEDs off, then test the same report paths.
- [ ] Exercise the physical Bluetooth/2.4 GHz/wired selector repeatedly, including transport changes under load.
- [ ] In cable mode, confirm the keyboard/backlight stays off while unplugged and the entire keyboard—not a partial LED-driver region—starts normally after USB is connected.
- [ ] In cable mode, hold Esc while connecting USB and confirm the keyboard enters STM32 DFU.
- [ ] Test sleep/wake and idle power behavior on battery in both wireless modes.
- [ ] Confirm the physical Mac/Windows switch selects the intended layers and all retained macros work on both operating systems.
- [ ] Confirm the bottom-left order is Ctrl, Option/Alt, Meta/GUI in both modes.

Hardware items are intentionally left open: compilation validates interfaces and linkage, not the electrical behavior of the board or LKBT51 module.
