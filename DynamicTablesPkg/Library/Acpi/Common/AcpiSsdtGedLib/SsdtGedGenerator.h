/** @file
  SSDT Generic Event Device Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI Specification, section 5.6.9,
    "Interrupt-signaled ACPI events".
**/

#pragma once

#define GED_HARDWARE_ID            "ACPI0013"
#define GED_OPERATION_REGION_NAME  "REG0"

#define GED_MAXIMUM_ID           0xF
#define GED_MAXIMUM_FIELD_COUNT  0x1000

#define SSDT_GED_GENERATOR_REVISION  CREATE_REVISION (1, 0)
