/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard

  @par Reference(s):
  - Arm CoreLink DMC Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.0 Platform Design Document
**/

#pragma once

/** PeriphBase maximum address length is 512 bytes (0x200)
*/
#define DMC_PERIPHBASE_MAX_ADDRESS_LENGTH  0x200

/** Mask covering the DMC Address space
*/
#define DMC_REGISTER_SPACE_MASK  0x1FFF

/** Offset of the PMU registers in the DMC register space
*/
#define DMC_PMU_ADDRESS_OFFSET  0x0A00
