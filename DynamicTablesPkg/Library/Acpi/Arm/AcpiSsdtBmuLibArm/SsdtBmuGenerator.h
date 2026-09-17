/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard

  @par Reference(s):
  - ACPI for the Arm Components 1.3 Platform Design Document
    (https://support.arm.com/documentation/den0093/1-3alp0)
**/

#pragma once

/** PeriphBase maximum address length is 192 KiB or 256 KiB (0x30000 or 0x40000)
*/
#define BMU_PERIPHBASE_MAX_ADDRESS_LENGTH_1  0x30000
#define BMU_PERIPHBASE_MAX_ADDRESS_LENGTH_2  0x40000

/** Mask covering the BMU Address space
*/
#define BMU_REGISTER_SPACE_MASK  0xFFFFF

/** Offset of the BMU registers in the BMU register space
*/
#define BMU_ADDRESS_OFFSET_1  0x80000
#define BMU_ADDRESS_OFFSET_2  0xC0000
