/** @file
  SSDT PL050 AML Table Generator definitions.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ARM PrimeCell PS/2 Keyboard/Mouse Interface (PL050) Technical Reference
    Manual, ARM DDI 0143C
**/

#pragma once

#define PL050_KMICR_OFFSET    0x00
#define PL050_KMISTAT_OFFSET  0x04
#define PL050_KMIDATA_OFFSET  0x08

#define PL050_REGISTER_LENGTH  4

#define PL050_KEYBOARD_HID  "ARMH0501"
#define PL050_KEYBOARD_CID  "PL050_KBD"
#define PL050_MOUSE_HID     "ARMH0502"
#define PL050_MOUSE_CID     "PL050_MOUSE"
