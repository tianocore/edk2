/** @file

  Copyright (c) 2018 - 2022, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#pragma once

#define IWB_DEV_NAME_SIZE  (AML_NAME_SEG_SIZE + 1)
#define IWB_DEV_PATH_SIZE  (IWB_DEV_NAME_SIZE + 6)      /* "\\_SB_." */

#pragma pack(1)

/** A structure that describes the Node indexer
    used for indexing the IORT nodes.
*/
typedef struct IortNodeIndexer {
  /// Index token for the Node
  CM_OBJECT_TOKEN    Token;
  /// Pointer to the node
  VOID               *Object;
  /// Node offset from the start of the IORT table
  UINT32             Offset;

  /// Unique identifier for the Node
  UINT32             Identifier;
} IORT_NODE_INDEXER;

typedef struct AcpiIortGenerator {
  /// ACPI Table generator header
  ACPI_TABLE_GENERATOR    Header;

  // IORT Generator private data

  /// IORT node count
  UINT32                  IortNodeCount;
  /// Pointer to the node indexer array
  IORT_NODE_INDEXER       *NodeIndexer;
} ACPI_IORT_GENERATOR;

/** Build a SSDT table describing the IWB device.

  The table created by this function must be freed by FreeSimpleIwbDeviceTable.

  @param [in]  IwbInfo          IWB device info to describe in the SSDT table.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildIwbDeviceTable (
  IN  CONST CM_ARM_GIC_IWB_INFO          *IwbInfo,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  );

/** Free an Iwb device table previously created by
    the BuildIwbDeviceTable function.

  @param [in] Table   Pointer to a Iwb Device table allocated by
                      the BuildIwbDeviceTable function.

**/
VOID
EFIAPI
FreeIwbDeviceTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  );

#pragma pack()
