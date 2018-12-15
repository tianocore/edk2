/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef STANDARD_NAMESPACE_OBJECTS_H_
#define STANDARD_NAMESPACE_OBJECTS_H_

#include <AcpiTableGenerator.h>
#include <SmbiosTableGenerator.h>

#pragma pack(1)

/** A macro defining a reserved zero/NULL token value that
    does not identify any object.
*/
#define CM_NULL_TOKEN  0

/** A reference token that the Configuration Manager can use
    to identify a Configuration Manager object.

  This can be used to differentiate between instances of
  objects of the same types. The identification scheme is
  implementation defined and is defined by the Configuration
  Manager.

  Typically the token is used to identify a specific instance
  from a set of objects in a call to the GetObject()/SetObject(),
  implemented by the Configuration Manager protocol.

  Note: The token value 0 is reserved for a NULL token and does
        not identify any object.
**/
typedef UINTN   CM_OBJECT_TOKEN;

/** The ESTD_OBJECT_ID enum describes the Object IDs
    in the Standard Namespace.
*/
typedef enum StdObjectID {
  EStdObjCfgMgrInfo = 0x00000000, ///< 0 - Configuration Manager Info
  EStdObjAcpiTableList,           ///< 1 - ACPI table Info List
  EStdObjSmbiosTableList,         ///< 2 - SMBIOS table Info List
  EStdObjMax
} ESTD_OBJECT_ID;

/** A structure that describes the Configuration Manager Information.
*/
typedef struct CmStdObjConfigurationManagerInfo {
  /// The Configuration Manager Revision.
  UINT32  Revision;

  /** The OEM ID. This information is used to
      populate the ACPI table header information.
  */
  UINT8   OemId[6];
} CM_STD_OBJ_CONFIGURATION_MANAGER_INFO;

/** A structure used to describe the ACPI table generators to be invoked.

  The AcpiTableData member of this structure may be used to directly provide
  the binary ACPI table data which is required by the following standard
  generators:
    - RAW
    - DSDT
    - SSDT

  Providing the ACPI table data is optional and depends on the generator
  that is being invoked. If unused, set AcpiTableData to NULL.
*/
typedef struct CmAStdObjAcpiTableInfo {
  /// The signature of the ACPI Table to be installed
  UINT32                         AcpiTableSignature;

  /// The ACPI table revision
  UINT32                         AcpiTableRevision;

  /// The ACPI Table Generator ID
  ACPI_TABLE_GENERATOR_ID        TableGeneratorId;

  /// Optional pointer to the ACPI table data
  EFI_ACPI_DESCRIPTION_HEADER  * AcpiTableData;

} CM_STD_OBJ_ACPI_TABLE_INFO;

/** A structure used to describe the SMBIOS table generators to be invoked.

  The SmbiosTableData member of this structure is used to provide
  the SMBIOS table data which is required by the following standard
  generator(s):
    - RAW

  Providing the SMBIOS table data is optional and depends on the
  generator that is being invoked. If unused, set the SmbiosTableData
  to NULL.
*/
typedef struct CmStdObjSmbiosTableInfo {
  /// The SMBIOS Table Generator ID
  SMBIOS_TABLE_GENERATOR_ID   TableGeneratorId;

  /// Optional pointer to the SMBIOS table data
  SMBIOS_STRUCTURE           * SmbiosTableData;
} CM_STD_OBJ_SMBIOS_TABLE_INFO;

#pragma pack()

#endif // STANDARD_NAMESPACE_OBJECTS_H_
