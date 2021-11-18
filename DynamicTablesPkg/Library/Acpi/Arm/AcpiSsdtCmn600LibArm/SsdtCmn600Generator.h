/** @file

  Copyright (c) 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.0 Platform Design Document
**/

#ifndef SSDT_CMN600_GENERATOR_H_
#define SSDT_CMN600_GENERATOR_H_

/** PeriphBase maximum address length is 256MB (0x10000000)
    for a (X >= 4) || (Y >= 4) dimensions mesh.
*/
#define PERIPHBASE_MAX_ADDRESS_LENGTH  SIZE_256MB

/** PeriphBase minimum address length is 64MB (0x04000000)
    for a (X < 4) && (Y < 4) dimensions mesh.
*/
#define PERIPHBASE_MIN_ADDRESS_LENGTH  SIZE_64MB

/** RootNodeBase address length is 16KB (0x00004000).
*/
#define ROOTNODEBASE_ADDRESS_LENGTH  SIZE_16KB

/** Maximum number of CMN-600 Debug and Trace Logic Controllers (DTC).
*/
#define MAX_DTC_COUNT  4

/** Starting value for the UID to represent the CMN600 devices.
*/
#define CMN600_DEVICE_START_UID  0

/** Maximum CMN-600 devices supported by this generator.
    This generator supports a maximum of 16 CMN-600 devices.
    Note: This is not a hard limitation and can be extended if needed.
          Corresponding changes would be needed to support the Name and
          UID fields describing the serial port.

*/
#define MAX_CMN600_DEVICES_SUPPORTED  16

#endif // SSDT_CMN600_GENERATOR_H_
