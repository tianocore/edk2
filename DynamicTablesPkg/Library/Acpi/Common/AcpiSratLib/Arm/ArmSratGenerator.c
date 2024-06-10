/** @file
  Arm SRAT Table Generator

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.3 Specification, January 2019

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

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
  ARM standard SRAT Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EArmObjGicCInfo (REQUIRED)
    - EArmObjGicItsInfo (OPTIONAL)
*/

/** This macro expands to a function that retrieves the GIC
    CPU interface Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/** This macro expands to a function that retrieves the GIC
    Interrupt Translation Service Information from the
    Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicItsInfo,
  CM_ARM_GIC_ITS_INFO
  );

/** Enum of the Arm specific CM objects required to
    build the arch specific information of the SRAT table.
*/
typedef enum ArmSratSubTableType {
  EArmGicCSubTableType,
  EArmGicItsSubTableType,
  EArmSubTableTypeMax,
} EARM_SRAT_SUB_TABLE_TYPE;

typedef struct SratSubTable {
  /// Start offset of the arch specific sub-table.
  UINT32    Offset;

  /// Count
  UINT32    Count;

  /// Array of CmInfo objects of the relevant type.
  VOID      *CmInfo;
} SRAT_SUB_TABLE;

STATIC SRAT_SUB_TABLE  mSratSubTable[EArmSubTableTypeMax];

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

  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             (CM_ARM_GICC_INFO **)&mSratSubTable[EArmGicCSubTableType].CmInfo,
             &mSratSubTable[EArmGicCSubTableType].Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (mSratSubTable[EArmGicCSubTableType].Count == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: GIC CPU Interface information not provided.\n"
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArmObjGicItsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             (CM_ARM_GIC_ITS_INFO **)&mSratSubTable[EArmGicItsSubTableType].CmInfo,
             &mSratSubTable[EArmGicItsSubTableType].Count
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Failed to get GIC ITS Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  mSratSubTable[EArmGicCSubTableType].Offset = *ArchOffset;
  *ArchOffset                               += (sizeof (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE) *
                                                mSratSubTable[EArmGicCSubTableType].Count);

  if (mSratSubTable[EArmGicItsSubTableType].Count != 0) {
    mSratSubTable[EArmGicItsSubTableType].Offset = *ArchOffset;
    *ArchOffset                                 += (sizeof (EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE) *
                                                    mSratSubTable[EArmGicItsSubTableType].Count);
  }

  return EFI_SUCCESS;
}

/** Add the GICC Affinity Structures in the SRAT Table.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.
**/
STATIC
VOID
EFIAPI
AddGICCAffinity (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  )
{
  EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE  *GicCAff;
  CM_ARM_GICC_INFO                      *GicCInfo;

  GicCInfo = mSratSubTable[EArmGicCSubTableType].CmInfo;
  GicCAff  = (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE *)((UINT8 *)Srat +
                                                      mSratSubTable[EArmGicCSubTableType].Offset);

  while (mSratSubTable[EArmGicCSubTableType].Count-- != 0) {
    DEBUG ((DEBUG_INFO, "SRAT: GicCAff = 0x%p\n", GicCAff));

    GicCAff->Type             = EFI_ACPI_6_3_GICC_AFFINITY;
    GicCAff->Length           = sizeof (EFI_ACPI_6_3_GICC_AFFINITY_STRUCTURE);
    GicCAff->ProximityDomain  = GicCInfo->ProximityDomain;
    GicCAff->AcpiProcessorUid = GicCInfo->AcpiProcessorUid;
    GicCAff->Flags            = GicCInfo->AffinityFlags;
    GicCAff->ClockDomain      = GicCInfo->ClockDomain;

    // Next
    GicCAff++;
    GicCInfo++;
  }// while
}

/** Add the GIC ITS Affinity Structures in the SRAT Table.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.
**/
STATIC
VOID
EFIAPI
AddGICItsAffinity (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  )
{
  EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE  *GicItsAff;
  CM_ARM_GIC_ITS_INFO                      *GicItsInfo;

  GicItsInfo = mSratSubTable[EArmGicItsSubTableType].CmInfo;
  GicItsAff  = (EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE *)((UINT8 *)Srat +
                                                           mSratSubTable[EArmGicItsSubTableType].Offset);

  while (mSratSubTable[EArmGicItsSubTableType].Count-- != 0) {
    DEBUG ((DEBUG_INFO, "SRAT: GicItsAff = 0x%p\n", GicItsAff));

    GicItsAff->Type            = EFI_ACPI_6_3_GIC_ITS_AFFINITY;
    GicItsAff->Length          = sizeof (EFI_ACPI_6_3_GIC_ITS_AFFINITY_STRUCTURE);
    GicItsAff->ProximityDomain = GicItsInfo->ProximityDomain;
    GicItsAff->Reserved[0]     = EFI_ACPI_RESERVED_BYTE;
    GicItsAff->Reserved[1]     = EFI_ACPI_RESERVED_BYTE;
    GicItsAff->ItsId           = GicItsInfo->GicItsId;

    // Next
    GicItsAff++;
    GicItsInfo++;
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

  AddGICCAffinity (CfgMgrProtocol, Srat);

  if (mSratSubTable[EArmGicCSubTableType].Count != 0) {
    AddGICItsAffinity (CfgMgrProtocol, Srat);
  }

  return EFI_SUCCESS;
}
