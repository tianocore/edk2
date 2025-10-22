/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard

  @par Reference(s):
  - Arm CoreLink DMC-620 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.0 Platform Design Document
**/

#ifndef SSDT_DMC620_GENERATOR_H_
#define SSDT_DMC620_GENERATOR_H_

/** PeriphBase maximum address length is 512 bytes (0x200)
*/
#define PERIPHBASE_MAX_ADDRESS_LENGTH  0x200

/** Mask covering the DMC620 Address space
*/
#define DMC620_REGISTER_SPACE_MASK  0x1FFF

/** Offset of the PMU registers in the DMC620 register space
*/
#define DMC620_PMU_ADDRESS_OFFSET  0x0A00

#endif // SSDT_DMC620_GENERATOR_H_
