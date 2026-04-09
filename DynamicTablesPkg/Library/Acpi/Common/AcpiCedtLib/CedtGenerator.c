/** @file
  CEDT Table Generator

  Copyright (c) 2025, Google, Inc. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Compute Express Link Specification - Revision 3.0, August 1, 2022.

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include <IndustryStandard/Cxl.h>
#include <IndustryStandard/Acpi64.h>
#include "Base.h"
#include "IndustryStandard/Acpi10.h"
#include "Library/BaseLib.h"

/** ARM standard CEDT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjCxlHostBridgeInfo
  - EArchCommonObjCxlFixedMemoryWindowInfo
*/

/** Retrieve the CXL host bridge info.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCxlHostBridgeInfo,
  CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO
  );

/** Retrieve the CXL fixed memory window info.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCxlFixedMemoryWindowInfo,
  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO
  );

#define EFI_ACPI_CEDT_CXL_VERSION_RCH_LENGTH_BYTES  SIZE_8KB
#define EFI_ACPI_CEDT_CXL_VERSION_HB_LENGTH_BYTES   SIZE_64KB

#define INVALID_INTERLEAVE_WAYS  (0xFF)
#define INVALID_GRANULARITY      (0xFF)

#define CFMWS_ALIGNMENT  SIZE_256MB

/** Add the CXL Host Bridge Info to the CEDT table

  @param [in,out]  WritePointer    Address of a write pointer for the CEDT table.
                                   On return, this will point to the next write
                                   location.
  @param [in]  HostBridgeList      Pointer to the CXL host bridge list.
  @param [in]  HostBridgeCount     Count of CXL host bridges.
**/
STATIC
VOID
AddCxlHostBridgeList (
  IN OUT   UINT8                                **WritePointer,
  IN CONST CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO  *HostBridgeList,
  IN       UINT32                               HostBridgeCount
  )
{
  EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE  *Chbs;
  UINT32                                       Index;

  ASSERT (WritePointer != NULL);
  ASSERT (*WritePointer != NULL);

  for (Index = 0; Index < HostBridgeCount; Index++) {
    Chbs               = (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE *)*WritePointer;
    Chbs->Type         = EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_HOST_BRIDGE_STRUCTURE;
    Chbs->RecordLength = sizeof (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE);
    Chbs->CxlVersion   = HostBridgeList[Index].Version;
    Chbs->Uid          = HostBridgeList[Index].Uid;
    Chbs->Base         = HostBridgeList[Index].ComponentRegisterBase;
    Chbs->Length       = (Chbs->CxlVersion == EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE_CXL_VERSION_HB) ?
                         EFI_ACPI_CEDT_CXL_VERSION_HB_LENGTH_BYTES :
                         EFI_ACPI_CEDT_CXL_VERSION_RCH_LENGTH_BYTES;
    Chbs->Reserved0 = EFI_ACPI_RESERVED_BYTE;
    Chbs->Reserved1 = EFI_ACPI_RESERVED_DWORD;

    *WritePointer += Chbs->RecordLength;
  }
}

/** Converts the literal number of interleave ways to register field encoding.

  @param [in]  NumberOfInterleaveWays Number of ways the memory is interleaved.

  @retval The register field encoding for the number of interleave ways.
**/
STATIC
UINT8
GetEncodedNumberOfInterleaveWays (
  IN UINT8  NumberOfInterleaveWays
  )
{
  switch (NumberOfInterleaveWays) {
    case 1:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_NONE;
    case 2:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_2_WAY;
    case 3:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_3_WAY;
    case 4:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_4_WAY;
    case 6:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_6_WAY;
    case 8:  return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_8_WAY;
    case 12: return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_12_WAY;
    case 16: return EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_WAYS_16_WAY;
    default: return INVALID_INTERLEAVE_WAYS;
  }
}

/** Convert the literal interleave granularity to the register field encoding.

  @param [in]  InterleaveGranularity Granularity of the interleave in bytes.

  @retval The register field encoding for the interleave granularity.
**/
STATIC
UINT8
GetEncodedInterleaveGranularity (
  IN UINT32  InterleaveGranularity
  )
{
  switch (InterleaveGranularity) {
    case 256:   return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_256B;
    case 512:   return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_512B;
    case 1024:  return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_1024B;
    case 2048:  return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_2048B;
    case 4096:  return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_4096B;
    case 8192:  return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_8192B;
    case 16384: return EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_INTERLEAVE_GRANULARITY_16384B;
    default:    return INVALID_GRANULARITY;
  }
}

/** Add the CXL Fixed Memory Window Info to the CEDT table

  @param [in,out]  WritePointer    Address of a write pointer for the CEDT table.
                                   On return, this will point to the next write
                                   location.
  @param [in]  WindowList          Pointer to the CXL fixed memory window info
                                   list.
  @param [in]  WindowCount         Count of CXL fixed memory windows.
  @param [in]  CfgMgrProtocol      ConfigManagerProtocol to query data from.
**/
STATIC
EFI_STATUS
AddCxlFixedMemoryWindowList (
  IN OUT   UINT8                                          **WritePointer,
  IN CONST CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO    *WindowList,
  IN       UINT32                                         WindowCount,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE  *Cfmws;
  UINT32                                               Index;
  UINT32                                               InterleaveTargetIndex;
  UINT64                                               Remainder;

  ASSERT (WritePointer != NULL);
  ASSERT (*WritePointer != NULL);

  for (Index = 0; Index < WindowCount; Index++) {
    Cfmws               = (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE *)*WritePointer;
    Cfmws->Type         = EFI_ACPI_6_4_CEDT_STRUCTURE_TYPE_CXL_FIXED_MEMORY_WINDOW_STRUCTURE;
    Cfmws->RecordLength =
      sizeof (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE) +
      (sizeof (UINT32) * WindowList[Index].NumberOfInterleaveWays);

    Cfmws->BaseHpa = WindowList[Index].BaseHostPhysicalAddress;
    DivU64x64Remainder (
      Cfmws->BaseHpa,
      CFMWS_ALIGNMENT,
      &Remainder
      );
    if (Remainder != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CEDT: Unaligned window address: 0x%lx",
        Cfmws->BaseHpa
        ));
      return EFI_INVALID_PARAMETER;
    }

    Cfmws->WindowSize = WindowList[Index].WindowSizeBytes;
    DivU64x64Remainder (
      Cfmws->WindowSize,
      CFMWS_ALIGNMENT * WindowList[Index].NumberOfInterleaveWays,
      &Remainder
      );
    if (Remainder != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CEDT: Unaligned window size: 0x%lx",
        Cfmws->WindowSize
        ));
      return EFI_INVALID_PARAMETER;
    }

    Cfmws->EncodedNumberOfInterleaveWays =
      GetEncodedNumberOfInterleaveWays (WindowList[Index].NumberOfInterleaveWays);

    Cfmws->InterleaveArithmetic = WindowList[Index].InterleaveArithmetic;
    if (Cfmws->InterleaveArithmetic > EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_INTERLEAVE_ARITHMETIC_MODULO_XOR) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CEDT: Invalid interleave arithmetic: 0x%d",
        Cfmws->InterleaveArithmetic
        ));
      return EFI_INVALID_PARAMETER;
    }

    Cfmws->WindowRestrictions = WindowList[Index].WindowRestrictions;
    if (Cfmws->WindowRestrictions &
        ~((EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE_WINDOW_RESTRICTIONS_FIXED_DEVICE_CONFIGURATION << 1)-1))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CEDT: Invalid window restrictions: 0x%x",
        Cfmws->WindowRestrictions
        ));
      return EFI_INVALID_PARAMETER;
    }

    Cfmws->QtgId                           = WindowList[Index].QtgId;
    Cfmws->HostBridgeInterleaveGranularity =
      GetEncodedInterleaveGranularity (WindowList[Index].HostBridgeInterleaveGranularity);
    if (Cfmws->HostBridgeInterleaveGranularity == INVALID_GRANULARITY) {
      return EFI_INVALID_PARAMETER;
    }

    Cfmws->Reserved0 = EFI_ACPI_RESERVED_BYTE;
    Cfmws->Reserved1 = EFI_ACPI_RESERVED_DWORD;
    Cfmws->Reserved2 = EFI_ACPI_RESERVED_WORD;

    for (InterleaveTargetIndex = 0;
         InterleaveTargetIndex < WindowList[Index].NumberOfInterleaveWays;
         InterleaveTargetIndex++)
    {
      UINT32                               HostBridgeCount;
      CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO  *HostBridge;
      EFI_STATUS                           Status;

      Status = GetEArchCommonObjCxlHostBridgeInfo (
                 CfgMgrProtocol,
                 WindowList[Index].InterleaveTargetTokens[InterleaveTargetIndex],
                 &HostBridge,
                 &HostBridgeCount
                 );
      if (EFI_ERROR (Status) || (HostBridgeCount != 1)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: CEDT: Failed to find host bridge with token: %x",
          WindowList[Index].InterleaveTargetTokens[InterleaveTargetIndex]
          ));
        return Status;
      }

      Cfmws->InterleaveTargetList[InterleaveTargetIndex] = HostBridge->Uid;
    }

    *WritePointer += Cfmws->RecordLength;
  }

  return EFI_SUCCESS;
}

/** Construct the CEDT ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildCedtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                           Status;
  UINT32                               TableSize;
  UINT32                               Index;
  UINT32                               TotalInterleaveTargets;
  UINT8                                *WritePointer;
  UINT32                               HostBridgeCount;
  CM_ARCH_COMMON_CXL_HOST_BRIDGE_INFO  *HostBridgeList;

  UINT32                                       WindowCount;
  CM_ARCH_COMMON_CXL_FIXED_MEMORY_WINDOW_INFO  *WindowList;

  EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE  *CedtTable;

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
      "ERROR: CEDT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  // Get CHBS objects.

  Status = GetEArchCommonObjCxlHostBridgeInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HostBridgeList,
             &HostBridgeCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CEDT: Failed to get CXL Host Bridge Information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "CEDT: Host Bridge Count = %d\n",
    HostBridgeCount
    ));

  // Get CFMWS objects
  Status = GetEArchCommonObjCxlFixedMemoryWindowInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &WindowList,
             &WindowCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CEDT: Failed to get CXL Fixed Memory Window Information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "CEDT: Window Count = %d\n",
    WindowCount
    ));

  // Calculate the CEDT Table Size
  TableSize =
    sizeof (EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE) +
    ((sizeof (EFI_ACPI_6_4_CEDT_CXL_HOST_BRIDGE_STRUCTURE) * HostBridgeCount));

  // CFMWS does not have a fixed size because the number of InterleaveTargets
  // can vary. Its size is given by 36 + (4*NumberOfInterleaveWays).
  //
  // Also validate that the provided interleave ways is compliant.
  TotalInterleaveTargets = 0;
  for (Index = 0; Index < WindowCount; Index++) {
    UINT8  InterleaveWays;
    InterleaveWays = WindowList[Index].NumberOfInterleaveWays;
    if (GetEncodedNumberOfInterleaveWays (InterleaveWays) == INVALID_INTERLEAVE_WAYS) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CEDT: Invalid interleave ways: %d",
        InterleaveWays
        ));
      Status = EFI_INVALID_PARAMETER;
      goto error_handler;
    }

    TotalInterleaveTargets += InterleaveWays;
  }

  TableSize += (TotalInterleaveTargets * sizeof (UINT32)) +
               (WindowCount * sizeof (EFI_ACPI_6_4_CEDT_CXL_FIXED_MEMORY_WINDOW_STRUCTURE));

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CEDT: Failed to allocate memory for CEDT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  // Add ACPI header.
  CedtTable = (EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &CedtTable->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CEDT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  WritePointer = (UINT8 *)&CedtTable->CedtStructure;

  // Populate CHBS entries.
  AddCxlHostBridgeList (
    &WritePointer,
    HostBridgeList,
    HostBridgeCount
    );

  // Populate CFMWS entries.
  Status = AddCxlFixedMemoryWindowList (
             &WritePointer,
             WindowList,
             WindowCount,
             CfgMgrProtocol
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: CEDT: Failed to add CFMWS. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  return Status;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free any resources allocated for constructing the CEDT

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeCedtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: CEDT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the CEDT Table Generator revision.
*/
#define CEDT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the CEDT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  CedtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdCedt),
  // Generator Description
  L"ACPI.STD.CEDT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_4_CXL_EARLY_DISCOVERY_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_4_CEDT_CXL_EARLY_DISCOVERY_TABLE_REVISION_01,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_4_CEDT_CXL_EARLY_DISCOVERY_TABLE_REVISION_01,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  CEDT_GENERATOR_REVISION,
  // Build Table function
  BuildCedtTable,
  // Free Resource function
  FreeCedtTableResources,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiCedtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&CedtGenerator);
  DEBUG ((DEBUG_INFO, "CEDT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiCedtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&CedtGenerator);
  DEBUG ((DEBUG_INFO, "CEDT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
