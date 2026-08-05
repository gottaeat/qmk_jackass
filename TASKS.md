# Jackass MK1 tasks

## Completed in source

- [x] Base `master` on QMK `0.33.13`.
- [x] Add `qmk-master` tracking `qmk/master` and `keychron-2025q3` tracking `keychron/2025q3`.
- [x] Import the Keychron K2 HE `2025q3` sources exactly before reduction.
- [x] Move the ANSI target to `keyboards/jackass/mk1` with Jackass metadata.
- [x] Keep all board and compatibility code inside `keyboards/jackass/mk1`.
- [x] Reduce Hall behavior to regular triggering with two persisted profiles: 2.5 mm and 1.5 mm.
- [x] Add bindable direct/cycling profile actions and bind profile cycling to the screenshot key.
- [x] Bind battery level to Fn+B.
- [x] Retain Mac/Windows layers, shortcut macros, Ctrl-Option/Alt-Meta/GUI order, NKRO, wear-leveling, battery, charging, and low-power behavior.
- [x] Retain Keychron LKBT51 Bluetooth, 2.4 GHz, wired transport switching, reports, pairing, reconnection, and wake handling.
- [x] Reduce lighting to white, red Caps Lock, battery gauge, and red/yellow profile confirmations.
- [x] Remove non-ANSI layouts, VIA/raw HID, factory testing, retail/demo lighting, game-controller, joystick, XInput, SOCD, OKMC, rapid-trigger, and toggle sources.
- [x] Add Docker build files and the porting guide.
- [x] Pass QMK keyboard lint in Docker.
- [x] Produce a QMK firmware binary in Docker.

## Hardware validation required

- [ ] Flash `jackass_mk1_default.bin` onto a K2 HE ANSI board and verify boot/reset behavior.
- [ ] Confirm every ANSI matrix position and Hall sensor calibrates and reports correctly.
- [ ] Measure profile 1 at 2.5 mm and profile 2 at 1.5 mm on representative switches.
- [ ] Confirm the screenshot key cycles only profiles 1 and 2 and persists the selected profile across power loss.
- [ ] Confirm the full board shows red for one second for profile 1 and yellow for one second for profile 2, then returns to white.
- [ ] Confirm Caps Lock is red and returns correctly after a battery/profile indication.
- [ ] Confirm Fn+B shows an accurate battery gauge and charging/full/critical-battery handling remains correct.
- [ ] Test USB typing, NKRO, suspend, and remote wake.
- [ ] Pair and reconnect all three Bluetooth host slots; test keyboard, consumer, system, mouse-wake, and NKRO reports.
- [ ] Pair and reconnect the 2.4 GHz receiver and test the same report paths.
- [ ] Exercise the physical Bluetooth/2.4 GHz/wired selector repeatedly, including transport changes under load.
- [ ] Test sleep/wake and idle power behavior on battery in both wireless modes.
- [ ] Confirm the physical Mac/Windows switch selects the intended layers and all retained macros work on both operating systems.
- [ ] Confirm the bottom-left order is Ctrl, Option/Alt, Meta/GUI in both modes.

Hardware items are intentionally left open: compilation validates interfaces and linkage, not the electrical behavior of the board or LKBT51 module.
