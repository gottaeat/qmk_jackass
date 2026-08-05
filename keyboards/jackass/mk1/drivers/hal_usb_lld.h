/* Copyright 2025 @ lokher (https://www.keychron.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/*
 * Keychron/ChibiOS commit ba10f3a80 fixes BOARD_OTG_NOVBUSSENS for the
 * STM32 OTGv1 driver. QMK 0.33.13's ChibiOS lacks that fix and enables
 * hardware VBUS sensing whenever USB is connected, taking PA9 away from
 * the K2 HE's P24G_MODE_SELECT_PIN. Include QMK's driver header first,
 * then apply Keychron's exact no-op connect/disconnect behavior locally.
 */
#include_next <hal_usb_lld.h>

#if (STM32_OTG_STEPPING == 1) && defined(BOARD_OTG_NOVBUSSENS)
#    undef usb_lld_connect_bus
#    define usb_lld_connect_bus(usbp)
#    undef usb_lld_disconnect_bus
#    define usb_lld_disconnect_bus(usbp)
#endif
