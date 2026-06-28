/** @file
  ACPI Maximum System Characteristics Table (MSCT) Generator

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>
#include <Library/SortLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/CmObjHelperLib.h>

#include "MsctGenerator.h"

/**
  Standard MSCT Generator

  Requirements:
    The following Configuration Manager Object(s) are used by this Generator:
    - EArchCommonObjMsctMaxPhysicalAddrInfo (REQUIRED)
    - EArchCommonObjMemoryAffinityInfo (OPTIONAL)
    - EArmObjGicCInfo (OPTIONAL)
    - EX64ObjLocalApicX2ApicAffinityInfo (OPTIONAL)
*/

/**
  This macro expands to a function that retrieves the maximum physical address
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMsctMaxPhysicalAddrInfo,
  CM_ARCH_COMMON_MSCT_MAX_PHYSICAL_ADDR_INFO
  );

/**
  This macro expands to a function that retrieves the Memory Affinity
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryAffinityInfo,
  CM_ARCH_COMMON_MEMORY_AFFINITY_INFO
  );

/** Comparison function for sorting Maximum Proximity Domain.

  @param [in]  Left    Pointer to the left Proximity Domain structure.
  @param [in]  Right   Pointer to the right Proximity Domain structure.

  @retval -1 If Left proximity domain < Right proximity domain
  @retval  0 If Left proximity domain == Right proximity domain
  @retval  1 If Left proximity domain > Right proximity domain
**/
INTN
EFIAPI
SortByProximityDomainRange (
  IN  CONST VOID  *Left,
  IN  CONST VOID  *Right
  )
{
  CONST EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *LeftInfo;
  CONST EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *RightInfo;

  LeftInfo  = (CONST EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE *)Left;
  RightInfo = (CONST EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE *)Right;

  if (LeftInfo->ProximityDomainRangeLow < RightInfo->ProximityDomainRangeLow) {
    return -1;
  } else if (LeftInfo->ProximityDomainRangeLow > RightInfo->ProximityDomainRangeLow) {
    return 1;
  } else {
    // ProximityDomainRangeLow are equal, compare ProximityDomainRangeHigh
    if (LeftInfo->ProximityDomainRangeHigh < RightInfo->ProximityDomainRangeHigh) {
      return -1;
    } else if (LeftInfo->ProximityDomainRangeHigh > RightInfo->ProximityDomainRangeHigh) {
      return 1;
    } else {
      return 0;
    }
  }
}

/** Get processor domain information.

  @param [in]      CfgMgrProtocol       Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     ProcDomainInfo       Pointer to the processor domain information.
  @param [out]     ProcDomainInfoCount  Pointer to the count of processor domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetProcessorDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST                 CfgMgrProtocol,
  OUT       EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  **ProcDomainInfo,
  OUT       UINT32                                                       *ProcDomainInfoCount
  )
{
  EFI_STATUS  Status;
  UINT32      *ProcDomainArch;
  UINT32      ProcDomainArchCount;
  UINT32      MaxProcDomain;
  UINT32      Index;
  UINT32      ProcDomainIndex;
  UINT32      ProcDomainCount;

  EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *ProcDomain;

  if ((CfgMgrProtocol == NULL) ||
      (ProcDomainInfo == NULL) ||
      (ProcDomainInfoCount == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  ProcDomain          = NULL;
  ProcDomainArch      = NULL;
  ProcDomainArchCount = 0;

  Status = GetArchProcessorDomainInfo (
             CfgMgrProtocol,
             &ProcDomainArch,
             &ProcDomainArchCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((ProcDomainArchCount == 0) || (ProcDomainArch == NULL)) {
    return EFI_NOT_FOUND;
  }

  ProcDomainCount = 0;
  ProcDomain      = NULL;
  MaxProcDomain   = 0;
  for (Index = 0; Index < ProcDomainArchCount; Index++) {
    for (ProcDomainIndex = 0; ProcDomainIndex < ProcDomainCount; ProcDomainIndex++) {
      if ((ProcDomain != NULL) &&
          (ProcDomain[ProcDomainIndex].ProximityDomainRangeLow == ProcDomainArch[Index]))
      {
        ProcDomain[ProcDomainIndex].MaximumProcessorCapacity++;
        break;
      }
    }

    if (ProcDomainIndex == ProcDomainCount) {
      ProcDomain = ReallocatePool (
                     sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE) * ProcDomainCount,
                     sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE) * (ProcDomainCount + 1),
                     ProcDomain
                     );
      if (ProcDomain == NULL) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: MSCT: Failed to reallocate memory for Processor Domain Info.\n"
          ));
        Status = EFI_OUT_OF_RESOURCES;
        ASSERT_EFI_ERROR (Status);
        goto error_handler;
      }

      ProcDomain[ProcDomainCount].Revision                 = EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_REVISION;
      ProcDomain[ProcDomainCount].Length                   = sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE);
      ProcDomain[ProcDomainCount].ProximityDomainRangeLow  = ProcDomainArch[Index];
      ProcDomain[ProcDomainCount].ProximityDomainRangeHigh = ProcDomainArch[Index];
      ProcDomain[ProcDomainCount].MaximumProcessorCapacity = 1;
      ProcDomainCount++;
      MaxProcDomain = MAX (MaxProcDomain, ProcDomainArch[Index]);
    }
  }

  *ProcDomainInfo      = ProcDomain;
  *ProcDomainInfoCount = ProcDomainCount;
  return EFI_SUCCESS;

error_handler:
  if (ProcDomain != NULL) {
    FreePool (ProcDomain);
  }

  return Status;
}

/** Get memory domain information.

  @param [in]      CfgMgrProtocol      Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     MemDomainInfo       Pointer to the memory domain information.
  @param [out]     MemDomainInfoCount  Pointer to the count of memory domain
                                        information structures.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
GetMemoryDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST                 CfgMgrProtocol,
  OUT       EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  **MemDomainInfo,
  OUT       UINT32                                                       *MemDomainInfoCount
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  UINT32      MemAffCount;
  UINT32      ProximityDomain;

  CM_ARCH_COMMON_MEMORY_AFFINITY_INFO                          *MemAffInfo;
  EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *MemDomain;

  if ((CfgMgrProtocol == NULL) ||
      (MemDomainInfo == NULL) ||
      (MemDomainInfoCount == NULL))
  {
    ASSERT_EFI_ERROR (EFI_INVALID_PARAMETER);
    return EFI_INVALID_PARAMETER;
  }

  MemDomain  = NULL;
  MemAffInfo = NULL;

  /// Get Memory Affinity Information
  Status = GetEArchCommonObjMemoryAffinityInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MemAffInfo,
             &MemAffCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to get Memory Affinity Info. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((MemAffInfo == NULL) || (MemAffCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Invalid Memory Affinity Info data.\n"
      ));
    Status = EFI_NOT_FOUND;
    return Status;
  }

  MemDomain = AllocateZeroPool (
                MemAffCount * sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE)
                );
  if (MemDomain == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to allocate memory for Memory Domain Info. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < MemAffCount; Index++) {
    Status = GetProximityDomainId (
               CfgMgrProtocol,
               MemAffInfo[Index].ProximityDomain,
               MemAffInfo[Index].ProximityDomainToken,
               &ProximityDomain
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "MSCT: Cannot obtain memory Proximity ID. Status = %r\n",
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      goto return_handler;
    }

    MemDomain[Index].Revision = EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_REVISION;
    MemDomain[Index].Length   = sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE);

    MemDomain[Index].ProximityDomainRangeLow  = ProximityDomain;
    MemDomain[Index].ProximityDomainRangeHigh = ProximityDomain;
    MemDomain[Index].MaximumMemoryCapacity    = MemAffInfo[Index].Length;
  }

  PerformQuickSort (
    MemDomain,
    MemAffCount,
    sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE),
    SortByProximityDomainRange
    );

  *MemDomainInfo      = MemDomain;
  *MemDomainInfoCount = MemAffCount;
  return EFI_SUCCESS;

return_handler:
  if (MemDomain != NULL) {
    FreePool (MemDomain);
  }

  return Status;
}

/** Update the MSCT Table with configuration data.

  @param [in]      CfgMgrProtocol      Pointer to the Configuration Manager
                                        Protocol.
  @param [out]     AcpiMsctTable       Pointer to the ACPI MSCT table.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Other                 Other EFI_STATUS error from called functions.
**/
EFI_STATUS
EFIAPI
UpdateMsctTable (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST              CfgMgrProtocol,
  OUT       EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER  **AcpiMsctTable
  )
{
  EFI_STATUS  Status;
  UINT32      MaxClockDomain;
  UINT32      MaxProximityDomain;
  UINT32      DomainInfoCount;
  UINT32      DomainInfoIndex;
  UINT32      MemDomainInfoCount;
  UINT32      MemIndex;
  UINT32      ProcDomainInfoCount;
  UINT32      ProcIndex;
  UINT32      TableSize;
  UINT8       *DomainInfoPtr;

  CM_ARCH_COMMON_MSCT_MAX_PHYSICAL_ADDR_INFO                   *MaxPhysAddrInfo;
  EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *DomainInfo;
  EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *MemDomainInfo;
  EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE  *ProcDomainInfo;
  EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER     *MsctTable;

  MsctTable       = NULL;
  MemDomainInfo   = NULL;
  ProcDomainInfo  = NULL;
  DomainInfo      = NULL;
  DomainInfoPtr   = NULL;
  MaxPhysAddrInfo = NULL;

  MemDomainInfoCount  = 0;
  ProcDomainInfoCount = 0;
  DomainInfoCount     = 0;
  DomainInfoIndex     = 0;
  MaxProximityDomain  = 0;

  Status = GetEArchCommonObjMsctMaxPhysicalAddrInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MaxPhysAddrInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to get Max Physical Address Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  MaxClockDomain = 0;
  Status         = GetArchClockDomainInfo (
                     CfgMgrProtocol,
                     &MaxClockDomain
                     );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    goto error_handler;
  }

  Status = GetMemoryDomainInfo (
             CfgMgrProtocol,
             &MemDomainInfo,
             &MemDomainInfoCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    goto error_handler;
  }

  Status = GetProcessorDomainInfo (
             CfgMgrProtocol,
             &ProcDomainInfo,
             &ProcDomainInfoCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    goto error_handler;
  }

  DomainInfoCount = MemDomainInfoCount + ProcDomainInfoCount;
  if (DomainInfoCount == 0) {
    DEBUG ((
      DEBUG_INFO,
      "MSCT: No Memory/Processor Domain information found.\n"
      ));
    goto generate_msct_table;
  }

  DomainInfo = AllocateZeroPool (
                 sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE) *
                 DomainInfoCount
                 );
  if (DomainInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_handler;
  }

  /// Get the maximum memory capacity per proximity domain.
  for (MemIndex = 0; MemIndex + 1 < MemDomainInfoCount; MemIndex++) {
    if (MemDomainInfo[MemIndex].ProximityDomainRangeLow == MAX_UINT32) {
      continue;
    }

    if (MemDomainInfo[MemIndex].ProximityDomainRangeLow ==
        MemDomainInfo[MemIndex + 1].ProximityDomainRangeLow)
    {
      /// Maximum memory capacity per domain.
      MemDomainInfo[MemIndex].MaximumMemoryCapacity +=
        MemDomainInfo[MemIndex + 1].MaximumMemoryCapacity;
      /// Invalidate the record
      MemDomainInfo[MemIndex + 1].ProximityDomainRangeLow  = MAX_UINT32;
      MemDomainInfo[MemIndex + 1].ProximityDomainRangeHigh = MAX_UINT32;
    }
  }

  /// Perform union of MemDomainInfo and ProcDomainInfo for same proximity domain
  DomainInfoIndex    = 0;
  MaxProximityDomain = 0;
  for (ProcIndex = 0; ProcIndex < ProcDomainInfoCount; ProcIndex++) {
    if (ProcDomainInfo[ProcIndex].ProximityDomainRangeLow == MAX_UINT32) {
      continue;
    }

    MaxProximityDomain = MAX (
                           MaxProximityDomain,
                           ProcDomainInfo[ProcIndex].ProximityDomainRangeHigh
                           );

    for (MemIndex = 0; MemIndex < MemDomainInfoCount; MemIndex++) {
      if (MemDomainInfo[MemIndex].ProximityDomainRangeLow == MAX_UINT32) {
        continue;
      }

      if (ProcDomainInfo[ProcIndex].ProximityDomainRangeLow ==
          MemDomainInfo[MemIndex].ProximityDomainRangeLow)
      {
        CopyMem (
          &DomainInfo[DomainInfoIndex],
          &ProcDomainInfo[ProcIndex],
          sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE)
          );
        DomainInfo[DomainInfoIndex].MaximumMemoryCapacity = MemDomainInfo[MemIndex].MaximumMemoryCapacity;
        DomainInfoIndex++;

        /// Invalidate the records
        ProcDomainInfo[ProcIndex].ProximityDomainRangeLow  = MAX_UINT32;
        ProcDomainInfo[ProcIndex].ProximityDomainRangeHigh = MAX_UINT32;
        MemDomainInfo[MemIndex].ProximityDomainRangeLow    = MAX_UINT32;
        MemDomainInfo[MemIndex].ProximityDomainRangeHigh   = MAX_UINT32;
        break;
      }
    }
  }

  /// Club the symmetric continuous proximity domains.
  if (ProcDomainInfoCount > 1) {
    PerformQuickSort (
      ProcDomainInfo,
      ProcDomainInfoCount,
      sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE),
      SortByProximityDomainRange
      );
    for (ProcIndex = ProcDomainInfoCount - 1; ProcIndex > 0; ProcIndex--) {
      if (ProcDomainInfo[ProcIndex].ProximityDomainRangeLow == MAX_UINT32) {
        continue;
      }

      if ((ProcDomainInfo[ProcIndex].ProximityDomainRangeLow ==
           (ProcDomainInfo[ProcIndex - 1].ProximityDomainRangeLow + 1)) &&
          (ProcDomainInfo[ProcIndex].MaximumProcessorCapacity ==
           (ProcDomainInfo[ProcIndex - 1].MaximumProcessorCapacity)))
      {
        ProcDomainInfo[ProcIndex - 1].ProximityDomainRangeHigh =
          ProcDomainInfo[ProcIndex].ProximityDomainRangeHigh;

        ProcDomainInfo[ProcIndex].ProximityDomainRangeLow  = MAX_UINT32;
        ProcDomainInfo[ProcIndex].ProximityDomainRangeHigh = MAX_UINT32;
      }
    }
  }

  /// Copy the processor proximity record
  for (ProcIndex = 0; ProcIndex < ProcDomainInfoCount; ProcIndex++) {
    if (ProcDomainInfo[ProcIndex].ProximityDomainRangeLow == MAX_UINT32) {
      continue;
    }

    CopyMem (
      &DomainInfo[DomainInfoIndex],
      &ProcDomainInfo[ProcIndex],
      sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE)
      );
    DomainInfoIndex++;
  }

  /// Copy remaining memory record
  for (MemIndex = 0; MemIndex < MemDomainInfoCount; MemIndex++) {
    if (MemDomainInfo[MemIndex].ProximityDomainRangeLow == MAX_UINT32) {
      continue;
    }

    CopyMem (
      &DomainInfo[DomainInfoIndex],
      &MemDomainInfo[MemIndex],
      sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE)
      );
    DomainInfoIndex++;
  }

  /// Sort the final list
  PerformQuickSort (
    DomainInfo,
    DomainInfoIndex,
    sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE),
    SortByProximityDomainRange
    );

generate_msct_table:
  /// Calculate the size needed for the MSCT table
  TableSize = sizeof (EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER) +
              (sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE) * DomainInfoIndex);

  /// Allocate the Buffer for MSCT table
  MsctTable = (EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER *)AllocateZeroPool (TableSize);
  if (MsctTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: Failed to allocate memory for MSCT Table, Size = %d, Status = %r\n",
      TableSize,
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  if (DomainInfoIndex > 0) {
    DomainInfoPtr = (UINT8 *)MsctTable + sizeof (EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER);
    CopyMem (
      DomainInfoPtr,
      (UINT8 *)DomainInfo,
      sizeof (EFI_ACPI_6_5_MAXIMUM_PROXIMITY_DOMAIN_INFORMATION_STRUCTURE) * DomainInfoIndex
      );
    MsctTable->OffsetProxDomInfo = sizeof (EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER);
  }

  MsctTable->Header.Length = TableSize;

  ///
  /// As per the specification maximum number of proximity domain
  /// should be -1 of actual domain value.
  /// If there is no domain information then it will be concidered as single domain.
  ///
  if (MaxProximityDomain > 0) {
    MsctTable->MaximumNumberOfProximityDomains = MaxProximityDomain - 1;
  } else {
    MsctTable->MaximumNumberOfProximityDomains = 0;
  }

  if (MaxClockDomain > 0) {
    MsctTable->MaximumNumberOfClockDomains = MaxClockDomain - 1;
  } else {
    MsctTable->MaximumNumberOfClockDomains = 0;
  }

  MsctTable->MaximumPhysicalAddress = MaxPhysAddrInfo->MaxPhysicalAddress;
  *AcpiMsctTable                    = MsctTable;

  if (MemDomainInfo != NULL) {
    FreePool (MemDomainInfo);
  }

  if (ProcDomainInfo != NULL) {
    FreePool (ProcDomainInfo);
  }

  if (DomainInfo != NULL) {
    FreePool (DomainInfo);
  }

  return EFI_SUCCESS;

error_handler:
  if (MsctTable != NULL) {
    FreePool (MsctTable);
  }

  return Status;
}

/** Build the ACPI MSCT Table.

@param [in]      This           Pointer to the table generator.
@param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
@param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                Protocol Interface.
@param [out]     Table          Pointer to the constructed ACPI Table.

@retval EFI_SUCCESS             Table generated successfully.
@retval EFI_INVALID_PARAMETER   A parameter is invalid.
@retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
@retval Other                   Other EFI_STATUS error from called functions.

**/
STATIC
EFI_STATUS
EFIAPI
BuildMsctTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST   This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST   AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  EFI_STATUS  Status;

  EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_HEADER  *MsctTable;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table    = NULL;
  MsctTable = NULL;

  Status = UpdateMsctTable (
             CfgMgrProtocol,
             &MsctTable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MSCT: Failed to update MSCT table. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  // Add ACPI header
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &MsctTable->Header,
             AcpiTableInfo,
             MsctTable->Header.Length
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    goto error_handler;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)MsctTable;
  return EFI_SUCCESS;

error_handler:
  if (MsctTable != NULL) {
    FreePool (MsctTable);
  }

  return Status;
}

/** This macro defines the MSCT Table Generator revision.
*/
#define MSCT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MSCT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  MsctGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMsct),
  // Generator Description
  L"ACPI.STD.MSCT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_5_MAXIMUM_SYSTEM_CHARACTERISTICS_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  MSCT_GENERATOR_REVISION,
  // Build Table function
  BuildMsctTable,
  // Free Resource function
  NULL,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** AcpiMsctLib constructor

@param[in]  ImageHandle   The firmware allocated handle for the EFI image.
@param[in]  SystemTable   A pointer to the EFI System Table.

@retval EFI_SUCCESS       The constructor always returns EFI_SUCCESS.
@retval Other             Other EFI_STATUS error from called functions.

**/
EFI_STATUS
EFIAPI
AcpiMsctLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&MsctGenerator);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: Register Generator Failed. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((
      DEBUG_INFO,
      "MSCT: Register Generator. Status = %r\n",
      Status
      ));
  }

  return Status;
}

/** AcpiMsctLib destructor

@param[in]  ImageHandle   The firmware allocated handle for the EFI image.
@param[in]  SystemTable   A pointer to the EFI System Table.

@retval EFI_SUCCESS       The destructor always returns EFI_SUCCESS.
@retval Other             Other EFI_STATUS error from called functions.

**/
EFI_STATUS
EFIAPI
AcpiMsctLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&MsctGenerator);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "MSCT: Deregister Generator Failed. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((
      DEBUG_INFO,
      "MSCT: Deregister Generator. Status = %r\n",
      Status
      ));
  }

  return Status;
}
