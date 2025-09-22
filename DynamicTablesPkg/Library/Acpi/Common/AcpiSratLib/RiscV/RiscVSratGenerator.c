/** @file
  RISC-V SRAT Table Generator

  Copyright (c) 2024 Ventana Micro Systems Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.6 Specification

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#include <IndustryStandard/Acpi.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SratGenerator.h"

/**
  RISC-V standard SRAT Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - ERiscVObjRintcInfo (REQUIRED)
*/

/** This macro expands to a function that retrieves the RINTC
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjRintcInfo,
  CM_RISCV_RINTC_INFO
  );

typedef struct SratSubTable {
  /// Start offset of the arch specific sub-table.
  UINT32    Offset;

  /// Count
  UINT32    Count;

  /// Array of CmInfo objects of the relevant type.
  VOID      *CmInfo;
} SRAT_SUB_TABLE;

STATIC SRAT_SUB_TABLE  mSratSubTable;

/** Reserve arch sub-tables space.

  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
  @param [in, out] ArchOffset  On input, contains the offset where arch specific
                               sub-tables can be written. It is expected that
                               there enough space to write all the arch specific
                               sub-tables from this offset onward.
                               On ouput, contains the ending offset of the arch
                               specific sub-tables.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
ArchReserveOffsets (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT UINT32                                           *ArchOffset
  )
{
  EFI_STATUS  Status;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ArchOffset != NULL);

  Status = GetERiscVObjRintcInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             (CM_RISCV_RINTC_INFO **)&mSratSubTable.CmInfo,
             &mSratSubTable.Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Failed to get RINTC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (mSratSubTable.Count == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: RINTC information not provided.\n"
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  mSratSubTable.Offset = *ArchOffset;
  *ArchOffset         += (sizeof (EFI_ACPI_6_6_RINTC_AFFINITY_STRUCTURE) * mSratSubTable.Count);

  return EFI_SUCCESS;
}

/** Add the RINTC Affinity Structures in the SRAT Table.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.
**/
STATIC
VOID
EFIAPI
AddRINTCAffinity (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  )
{
  EFI_ACPI_6_6_RINTC_AFFINITY_STRUCTURE  *RintcAff;
  CM_RISCV_RINTC_INFO                    *RintcInfo;

  RintcInfo = mSratSubTable.CmInfo;
  RintcAff  = (EFI_ACPI_6_6_RINTC_AFFINITY_STRUCTURE *)((UINT8 *)Srat + mSratSubTable.Offset);

  while (mSratSubTable.Count-- != 0) {
    DEBUG ((DEBUG_INFO, "SRAT: RintcAff = 0x%p\n", RintcAff));

    RintcAff->Type             = EFI_ACPI_6_6_RINTC_AFFINITY;
    RintcAff->Length           = sizeof (EFI_ACPI_6_6_RINTC_AFFINITY_STRUCTURE);
    RintcAff->ProximityDomain  = RintcInfo->ProximityDomain;
    RintcAff->AcpiProcessorUid = RintcInfo->AcpiProcessorUid;
    RintcAff->Flags            = RintcInfo->AffinityFlags;
    RintcAff->ClockDomain      = RintcInfo->ClockDomain;

    // Next
    RintcAff++;
    RintcInfo++;
  }// while
}

/** Add the arch specific sub-tables to the SRAT table.

  These sub-tables are written in the space reserved beforehand.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.

  @retval EFI_SUCCESS           Table generated successfully.
**/
EFI_STATUS
EFIAPI
AddArchObjects (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Srat != NULL);

  AddRINTCAffinity (CfgMgrProtocol, Srat);
  return EFI_SUCCESS;
}
