/** @file

  Copyright (c) 2020 - 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.3 Platform Design Document
**/

#pragma once

/** CMN ACPI Hardware IDs. */
#define ACPI_HID_CMN_600  "ARMHC600"
#define ACPI_HID_CMN_650  "ARMHC650"
#define ACPI_HID_CMN_700  "ARMHC700"
#define ACPI_HID_CMN_S3   "ARMHC003"

/** PeriphBase maximum address length is 256MB (0x10000000)
    for a (X >= 4) || (Y >= 4) dimensions mesh.
*/
#define CMN600_PERIPHBASE_MAX_ADDRESS_LENGTH  SIZE_256MB

/** Required alignment of the CMN configuration region base address.
*/
#define CMN_PERIPHBASE_ADDRESS_ALIGNMENT  SIZE_64MB

/** RootNodeBase address length is 16KB (0x00004000).
*/
#define CMN600_ROOTNODEBASE_ADDRESS_LENGTH  SIZE_16KB

/** Maximum number of CMN Debug and Trace Logic Controllers (DTC).
*/
#define MAX_CMN_DTC_COUNT  4

/** Maximum CMN devices supported by this generator.
    This generator supports a maximum of 16 CMN devices.
    Note: This is not a hard limitation and can be extended if needed.
          Corresponding changes would be needed to support the Name and
          UID fields describing the CMN device.

*/
#define MAX_CMN_DEVICES_SUPPORTED  16
