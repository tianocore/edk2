/** @file
  X64 SRAT Table Generator

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
  This macro is used to get the object information for the Local APIC X2APIC
  Affinity object.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjLocalApicX2ApicAffinityInfo,
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO
  );

/** Enum of the X64 specific CM objects required to
    build the arch specific information of the SRAT table.
*/
typedef enum X64SratSubTableType {
  EX64LocalApicX2ApicAffinityTableType,
  EX64SubTableTypeMax
} EX64_SRAT_SUB_TABLE_TYPE;

typedef struct SratSubTable {
  /// Start offset of the arch specific sub-table.
  UINT32    Offset;

  /// Count
  UINT32    Count;

  /// Array of CmInfo objects of the relevant type.
  VOID      *CmInfo;
} SRAT_SUB_TABLE;

STATIC SRAT_SUB_TABLE  mSratSubTable[EX64SubTableTypeMax];

/** Reserve arch sub-tables space.

  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
  @param [in, out] ArchOffset  On input, contains the offset where arch specific
                               sub-tables can be written. It is expected that
                               there enough space to write all the arch specific
                               sub-tables from this offset onward.
                               On ouput, contains the ending offset of the arch
                               specific sub-tables.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_UNSUPPORTED       Not supported.
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
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO  *CmX2ApicAffinity;
  EFI_STATUS                              Status;
  LOCAL_APIC_MODE                         ApicMode;
  UINT32                                  CmCount;
  UINT32                                  Index;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ArchOffset != NULL);

  Status = GetEX64ObjLocalApicX2ApicAffinityInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CmX2ApicAffinity,
             &CmCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Failed to get Local Apic/X2Apic Affinity Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (CmCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SRAT: Local Apic/X2Apic Affinity information not provided.\n"
      ));
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  ApicMode = CmX2ApicAffinity[0].ApicMode;
  for (Index = 0; Index < CmCount; Index++) {
    if ((CmX2ApicAffinity[Index].Flags &
         ~EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_ENABLED) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SRAT: Invalid Flags. Flags = 0x%x\n",
        CmX2ApicAffinity[Index].Flags
        ));
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      return EFI_INVALID_PARAMETER;
    }

    if (CmX2ApicAffinity[Index].ApicMode == LocalApicModeXApic) {
      if (CmX2ApicAffinity[Index].ApicId > MAX_UINT8) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SRAT: Local Apic Id is invalid. ApicId = 0x%x\n",
          CmX2ApicAffinity[Index].ApicId
          ));
        ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
        return EFI_INVALID_PARAMETER;
      }
    }

    if (CmX2ApicAffinity[Index].ApicMode != ApicMode) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SRAT: Mixed Apic Modes are not supported.\n"
        ));
      ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
      return EFI_INVALID_PARAMETER;
    }
  }

  mSratSubTable[EX64LocalApicX2ApicAffinityTableType].CmInfo = CmX2ApicAffinity;
  mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Count  = CmCount;
  mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Offset = *ArchOffset;

  if (CmX2ApicAffinity[0].ApicMode == LocalApicModeX2Apic) {
    *ArchOffset += sizeof (EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE) * CmCount;
  } else {
    *ArchOffset += sizeof (EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE) * CmCount;
  }

  return EFI_SUCCESS;
}

/** Add the arch specific sub-tables to the SRAT table.

  These sub-tables are written in the space reserved beforehand.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_UNSUPPORTED       Not supported.
**/
EFI_STATUS
EFIAPI
AddArchObjects (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  )
{
  CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO                      *CmX2ApicAffinity;
  EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE  *ApicAffinity;
  EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE      *X2ApicAffinity;

  CmX2ApicAffinity = (CM_X64_LOCAL_APIC_X2APIC_AFFINITY_INFO *)
                     mSratSubTable[EX64LocalApicX2ApicAffinityTableType].CmInfo;

  if (CmX2ApicAffinity->ApicMode == LocalApicModeX2Apic) {
    X2ApicAffinity = (EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE *)
                     ((UINT8 *)Srat +
                      mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Offset);
    while (mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Count-- != 0) {
      X2ApicAffinity->Type            = EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY;
      X2ApicAffinity->Length          = sizeof (EFI_ACPI_6_3_PROCESSOR_LOCAL_X2APIC_AFFINITY_STRUCTURE);
      X2ApicAffinity->Reserved1[0]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity->Reserved1[1]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity->ProximityDomain = CmX2ApicAffinity->ProximityDomain;
      X2ApicAffinity->X2ApicId        = CmX2ApicAffinity->ApicId;
      X2ApicAffinity->Flags           = CmX2ApicAffinity->Flags;
      X2ApicAffinity->ClockDomain     = CmX2ApicAffinity->ClockDomain;
      X2ApicAffinity->Reserved2[0]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity->Reserved2[1]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity->Reserved2[2]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity->Reserved2[3]    = EFI_ACPI_RESERVED_BYTE;
      X2ApicAffinity++;
      // Next
      CmX2ApicAffinity++;
    }
  } else {
    ApicAffinity = (EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE *)
                   ((UINT8 *)Srat +
                    mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Offset);
    while (mSratSubTable[EX64LocalApicX2ApicAffinityTableType].Count-- != 0) {
      ApicAffinity->Type                    = EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY;
      ApicAffinity->Length                  = sizeof (EFI_ACPI_6_3_PROCESSOR_LOCAL_APIC_SAPIC_AFFINITY_STRUCTURE);
      ApicAffinity->ProximityDomain7To0     = (CmX2ApicAffinity->ProximityDomain & MAX_UINT8);
      ApicAffinity->ApicId                  = CmX2ApicAffinity->ApicId & MAX_UINT8;
      ApicAffinity->Flags                   = CmX2ApicAffinity->Flags;
      ApicAffinity->LocalSapicEid           = 0;
      ApicAffinity->ProximityDomain31To8[0] = (CmX2ApicAffinity->ProximityDomain >> 8) & MAX_UINT8;
      ApicAffinity->ProximityDomain31To8[0] = (CmX2ApicAffinity->ProximityDomain >> 16) & MAX_UINT8;
      ApicAffinity->ProximityDomain31To8[0] = (CmX2ApicAffinity->ProximityDomain >> 24) & MAX_UINT8;
      ApicAffinity->ClockDomain             = CmX2ApicAffinity->ClockDomain;
      ApicAffinity++;
      CmX2ApicAffinity++;
    } // while
  }

  return EFI_SUCCESS;
}
