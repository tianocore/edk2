/** @file
  Arm Error Source Table (AEST) generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for the Armv8-A RAS Extension and RAS System Architecture,
    DEN0085 2.0 EAC2.
    (https://support.arm.com/documentation/den0085/0202/?lang=en)
**/

#include <IndustryStandard/ArmErrorSourceTable.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/MetadataHandlerLib.h>
#include <Library/TableHelperLib.h>
#include <MetadataHelpers.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "AestGenerator.h"

/** AEST node groups and their Configuration Manager lists.
*/
STATIC AEST_NODE_GROUP  mNodeGroups[] = {
  { EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR, sizeof (CM_ARM_AEST_PROCESSOR_NODE),         TRUE,  NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY,    sizeof (CM_ARM_AEST_MEMORY_NODE),            TRUE,  NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_SMMU,      sizeof (CM_ARM_AEST_SMMU_NODE),              FALSE, NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_VENDOR,    sizeof (CM_ARM_AEST_VENDOR_NODE),            FALSE, NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_GIC,       sizeof (CM_ARM_AEST_GIC_NODE),               FALSE, NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_PCIE,      sizeof (CM_ARM_AEST_PCIE_ROOT_COMPLEX_NODE), FALSE, NULL, 0 },
  { EFI_ACPI_AEST_2_0_NODE_TYPE_PROXY,     sizeof (CM_ARM_AEST_PROXY_NODE),             FALSE, NULL, 0 }
};

STATIC BOOLEAN  mSratPresent;

/** Retrieve AEST processor nodes from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjAestProcessorNode,
  CM_ARM_AEST_PROCESSOR_NODE
  );

/** Retrieve AEST memory controller nodes from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjAestMemoryNode,
  CM_ARM_AEST_MEMORY_NODE
  );

/** Retrieve one of the variable-length AEST interface bitmap arrays.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjAestBitmapArray,
  CM_ARM_AEST_BITMAP_WORD
  );

/** Retrieve an AEST node interrupt array.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjAestInterruptArray,
  CM_ARM_AEST_INTERRUPT
  );

/** Retrieve the requested ACPI table list.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjAcpiTableList,
  CM_STD_OBJ_ACPI_TABLE_INFO
  );

/** Retrieve a PPTT processor hierarchy object.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProcHierarchyInfo,
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  );

/** Retrieve a GICC object that owns an ACPI processor UID.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/** Retrieve a PPTT cache object that owns a Cache ID.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCacheInfo,
  CM_ARCH_COMMON_CACHE_INFO
  );

/** Retrieve the owner of a proximity domain identifier.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProximityDomainInfo,
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO
  );

/** Retrieve an AEST node list for the requested node type.

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  NodeType        AEST node type.
  @param [out] List            Node list.
  @param [out] Count           Number of nodes.

  @retval EFI_SUCCESS      The list was retrieved.
  @retval EFI_UNSUPPORTED  The node type is unknown.
  @retval Others           Configuration Manager error.
**/
STATIC
EFI_STATUS
GetAestNodeList (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        UINT8                                         NodeType,
  OUT CONST VOID                                          **List,
  OUT       UINT32                                        *Count
  )
{
  *List  = NULL;
  *Count = 0;
  switch (NodeType) {
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR:
      return GetEArmObjAestProcessorNode (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               (CM_ARM_AEST_PROCESSOR_NODE **)List,
               Count
               );
    case EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY:
      return GetEArmObjAestMemoryNode (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               (CM_ARM_AEST_MEMORY_NODE **)List,
               Count
               );
    default:
      DEBUG ((DEBUG_ERROR, "AEST: Unknown node type 0x%x\n", NodeType));
      return EFI_UNSUPPORTED;
  }
}

/** Check the requested PPTT revision used by AEST.

  Processor nodes use PPTT to identify processors. This AEST revision works
  with PPTT revision 3 or later. Cf. DEN0085 2.0 EAC2, Section 2.1.1.

  Record SRAT presence for memory controller node validation.

  @param [in] CfgMgrProtocol  Configuration Manager protocol.

  @retval EFI_SUCCESS      PPTT revision 3 or later is requested.
  @retval EFI_UNSUPPORTED  The requested PPTT revision is too old.
  @retval EFI_NOT_FOUND    The required PPTT is missing.
  @retval Others           Configuration Manager or validation error.
**/
STATIC
EFI_STATUS
CheckTableDependencies (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  CM_STD_OBJ_ACPI_TABLE_INFO  *TableList;
  EFI_STATUS                  Status;
  UINT32                      TableCount;
  UINT32                      Index;
  BOOLEAN                     PpttPresent;

  mSratPresent = FALSE;

  Status = GetEStdObjAcpiTableList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &TableList,
             &TableCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((TableCount != 0) && (TableList == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: ACPI table list has %u entries but no data\n",
      TableCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  PpttPresent = FALSE;
  for (Index = 0; Index < TableCount; Index++) {
    if (TableList[Index].AcpiTableSignature == EFI_ACPI_6_7_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_STRUCTURE_SIGNATURE) {
      PpttPresent = TRUE;
      if (TableList[Index].AcpiTableRevision <
          EFI_ACPI_6_4_PROCESSOR_PROPERTIES_TOPOLOGY_TABLE_REVISION)
      {
        DEBUG ((
          DEBUG_ERROR,
          "AEST: PPTT revision %u is not supported by AEST 2.0\n",
          TableList[Index].AcpiTableRevision
          ));
        return EFI_UNSUPPORTED;
      }
    } else if (TableList[Index].AcpiTableSignature == EFI_ACPI_6_7_SYSTEM_RESOURCE_AFFINITY_TABLE_SIGNATURE) {
      mSratPresent = TRUE;
    }
  }

  if (!PpttPresent) {
    DEBUG ((DEBUG_ERROR, "AEST: PPTT table not found\n"));
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/** Resolve the processor or container UID for an AEST node interface.

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  Interface       Node interface CM data.
  @param [out] InterfaceInfo   Interface information to receive the UID.

  @retval EFI_SUCCESS           Interface affinity was resolved.
  @retval EFI_INVALID_PARAMETER The hierarchy or interface affinity is invalid.
  @retval Others                Configuration Manager or validation error.
**/
STATIC
EFI_STATUS
ResolveProcessorAffinity (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARM_AEST_INTERFACE                 *CONST  Interface,
  OUT       AEST_INTERFACE_INFO                   *CONST  InterfaceInfo
  )
{
  CM_ARCH_COMMON_PROC_HIERARCHY_INFO  *Hierarchy;
  CM_ARM_GICC_INFO                    *Gicc;
  METADATA_OBJ_UID                    MetadataUid;
  EFI_STATUS                          Status;
  UINT32                              Count;
  BOOLEAN                             IsContainer;
  CM_OBJECT_TOKEN                     Token;

  Token = Interface->ProcessorHierarchyToken;

  if (Token == CM_NULL_TOKEN) {
    DEBUG ((DEBUG_ERROR, "AEST: Processor affinity requires a PPTT hierarchy token\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjProcHierarchyInfo (
             CfgMgrProtocol,
             Token,
             &Hierarchy,
             &Count
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  IsContainer = (Interface->Flags &
                 EFI_ACPI_AEST_2_0_INTERFACE_FLAG_AFFINITY_TYPE) != 0;

  if ((Count != 1) || (Hierarchy == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Expected one non-null PPTT hierarchy object: count: %u\n",
      Count
      ));
    return EFI_INVALID_PARAMETER;
  }

  // The PPTT ACPI Processor ID must be valid.
  if ((Hierarchy->Flags & BIT1) == 0) {
    DEBUG ((DEBUG_ERROR, "AEST: PPTT hierarchy ACPI Processor ID is not valid\n"));
    return EFI_INVALID_PARAMETER;
  }

  // The PPTT leaf flag must agree with the interface affinity type.
  if (IsContainer == ((Hierarchy->Flags & BIT3) != 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: PPTT hierarchy is %a, but interface affinity is %a\n",
      ((Hierarchy->Flags & BIT3) != 0) ? "a processor leaf" : "a container",
      IsContainer ? "container" : "processor"
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Only processor leaves have an ACPI ID object token.
  if (IsContainer != (Hierarchy->AcpiIdObjectToken == CM_NULL_TOKEN)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: PPTT %a hierarchy has %a ACPI ID object token\n",
      IsContainer ? "container" : "processor leaf",
      IsContainer ? "an unexpected" : "no"
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!IsContainer) {
    Status = GetEArmObjGicCInfo (
               CfgMgrProtocol,
               Hierarchy->AcpiIdObjectToken,
               &Gicc,
               &Count
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Count != 1) || (Gicc == NULL)) {
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Invalid GICC reference for processor affinity: count %u, data %p\n",
        Count,
        Gicc
        ));
      return EFI_INVALID_PARAMETER;
    }

    InterfaceInfo->ProcessorAffinity = Gicc->AcpiProcessorUid;
  } else if (Hierarchy->OverrideNameUidEnabled) {
    InterfaceInfo->ProcessorAffinity = Hierarchy->OverrideUid;
  } else {
    ZeroMem (&MetadataUid, sizeof (MetadataUid));
    AsciiStrCpyS (MetadataUid.NameId, sizeof (MetadataUid.NameId), "ACPI0010");
    Status = MetadataHandlerGenerate (
               GetMetadataRoot (),
               MetadataTypeUid,
               Token,
               NULL,
               &MetadataUid,
               sizeof (MetadataUid)
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    InterfaceInfo->ProcessorAffinity = MetadataUid.Uid;
  }

  return EFI_SUCCESS;
}

/** Resolve and validate an AEST node interface and its bitmaps.

  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in]      Interface       Node interface CM data.
  @param [in, out] InterfaceInfo   Interface information with resolved
                                   processor affinity; receives the remaining
                                   interface data.

  @retval EFI_SUCCESS           Interface and bitmaps are valid.
  @retval EFI_INVALID_PARAMETER Interface or bitmaps are invalid.
  @retval Others                Configuration Manager error.
**/
STATIC
EFI_STATUS
ResolveAndValidateInterfaceData (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARM_AEST_INTERFACE                *CONST   Interface,
  IN OUT    AEST_INTERFACE_INFO                   *CONST  InterfaceInfo
  )
{
  CM_OBJECT_TOKEN          BitMapToken[AEST_INTERFACE_BITMAP_COUNT];
  CM_ARM_AEST_BITMAP_WORD  *Bitmap;
  EFI_STATUS               Status;
  UINT32                   Count;
  UINT32                   Capacity;
  UINT32                   EndIndex;
  UINT32                   MapIndex;
  UINT32                   WordIndex;
  UINT32                   BitIndex;
  UINT32                   RecordIndex;

  if ((Interface->Type > EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO) ||
      ((Interface->Flags & ~EFI_ACPI_AEST_2_0_INTERFACE_FLAGS_MASK) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Invalid interface type %u or reserved flags 0x%x\n",
      Interface->Type,
      Interface->Flags & ~EFI_ACPI_AEST_2_0_INTERFACE_FLAGS_MASK
      ));
    return EFI_INVALID_PARAMETER;
  }

  switch (Interface->GroupFormat) {
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB:
      InterfaceInfo->BitmapWordCount = EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_BITMAP_WORD_COUNT;
      Capacity                       = EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB_MAX_ERROR_RECORDS;
      break;
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB:
      InterfaceInfo->BitmapWordCount = EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_BITMAP_WORD_COUNT;
      Capacity                       = EFI_ACPI_AEST_2_0_GROUP_FORMAT_16KB_MAX_ERROR_RECORDS;
      break;
    case EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB:
      InterfaceInfo->BitmapWordCount = EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_BITMAP_WORD_COUNT;
      Capacity                       = EFI_ACPI_AEST_2_0_GROUP_FORMAT_64KB_MAX_ERROR_RECORDS;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "AEST: Invalid interface group format= %u\n", Interface->GroupFormat));
      return EFI_INVALID_PARAMETER;
  }

  if ((Interface->StartErrorRecordIndex >= Capacity) ||
      (Interface->NumberErrorRecords >
       (Capacity - Interface->StartErrorRecordIndex)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Invalid error record range: start= %u, count= %u, capacity= %u\n",
      Interface->StartErrorRecordIndex,
      Interface->NumberErrorRecords,
      Capacity
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((Interface->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO) &&
      ((Interface->GroupFormat != EFI_ACPI_AEST_2_0_GROUP_FORMAT_4KB) ||
       (Interface->StartErrorRecordIndex != 0) ||
       (Interface->NumberErrorRecords != 1)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Single-record MMIO requires 4KB format, start error index= 0, error record count= 1\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  EndIndex                                    = Interface->StartErrorRecordIndex + Interface->NumberErrorRecords;
  BitMapToken[AEST_IMPLEMENTED_BITMAP_INDEX]  = Interface->ImplementedBitmapToken;
  BitMapToken[AEST_GROUP_STATUS_BITMAP_INDEX] = Interface->GroupStatusBitmapToken;
  BitMapToken[AEST_ADDRESS_MODE_BITMAP_INDEX] = Interface->AddressModeBitmapToken;

  for (MapIndex = 0; MapIndex < ARRAY_SIZE (BitMapToken); MapIndex++) {
    if (BitMapToken[MapIndex] == CM_NULL_TOKEN) {
      DEBUG ((DEBUG_ERROR, "AEST: Bitmap array %u has a null CM token\n", MapIndex));
      return EFI_INVALID_PARAMETER;
    }

    Status = GetEArmObjAestBitmapArray (
               CfgMgrProtocol,
               BitMapToken[MapIndex],
               &Bitmap,
               &Count
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((Count != InterfaceInfo->BitmapWordCount) || (Bitmap == NULL)) {
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Invalid bitmap array %u: count= %u, expected= %u, data= 0x%p\n",
        MapIndex,
        Count,
        InterfaceInfo->BitmapWordCount,
        Bitmap
        ));
      return EFI_INVALID_PARAMETER;
    }

    InterfaceInfo->Bitmaps[MapIndex] = Bitmap;
    for (WordIndex = 0; WordIndex < Count; WordIndex++) {
      for (BitIndex = 0; BitIndex < AEST_BITMAP_WORD_BIT_COUNT; BitIndex++) {
        RecordIndex = (WordIndex * AEST_BITMAP_WORD_BIT_COUNT) + BitIndex;
        if (((RecordIndex < Interface->StartErrorRecordIndex) ||
             (RecordIndex >= EndIndex)) &&
            (((Bitmap[WordIndex].Word >> BitIndex) & 1) != 0))
        {
          DEBUG ((
            DEBUG_ERROR,
            "AEST: Bitmap array %u sets record %u outside node range: start= %u, count= %u\n",
            MapIndex,
            RecordIndex,
            Interface->StartErrorRecordIndex,
            Interface->NumberErrorRecords
            ));
          return EFI_INVALID_PARAMETER;
        }
      }
    }
  }

  if ((Interface->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SINGLE_MMIO) &&
      (InterfaceInfo->Bitmaps[AEST_IMPLEMENTED_BITMAP_INDEX][0].Word != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Single-record MMIO requires a zero implemented-record bitmap, got 0x%llx\n",
      InterfaceInfo->Bitmaps[AEST_IMPLEMENTED_BITMAP_INDEX][0].Word
      ));
    return EFI_INVALID_PARAMETER;
  }

  InterfaceInfo->Source = Interface;
  return EFI_SUCCESS;
}

/** Resolve and validate interrupt array.

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  Common          Common AEST node inputs.
  @param [out] InterruptArray  Interrupt array, or NULL when absent.

  @retval EFI_SUCCESS           Interrupt entries are valid or absent.
  @retval EFI_INVALID_PARAMETER The array or an entry is invalid.
  @retval Others                Configuration Manager error.
**/
STATIC
EFI_STATUS
ResolveAndValidateInterruptData (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN  CONST CM_ARM_AEST_COMMON                     *CONST  Common,
  OUT CONST CM_ARM_AEST_INTERRUPT                          **InterruptArray
  )
{
  CM_ARM_AEST_INTERRUPT  *Interrupts;
  EFI_STATUS             Status;
  UINT32                 Count;
  UINT32                 Index;

  *InterruptArray = NULL;
  if (Common->InterruptCount == 0) {
    return EFI_SUCCESS;
  }

  if (Common->InterruptArrayToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Interrupt count %u requires a non-null interrupt array token\n",
      Common->InterruptCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArmObjAestInterruptArray (
             CfgMgrProtocol,
             Common->InterruptArrayToken,
             &Interrupts,
             &Count
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Count != Common->InterruptCount) || (Interrupts == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Invalid interrupt array: count= %u, expected= %u, data= 0x%p\n",
      Count,
      Common->InterruptCount,
      Interrupts
      ));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < Count; Index++) {
    if ((Interrupts[Index].Type > EFI_ACPI_AEST_2_0_INTERRUPT_TYPE_ERI) ||
        ((Interrupts[Index].Flags & ~EFI_ACPI_AEST_2_0_INTERRUPT_FLAGS_MASK) != 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Invalid interrupt %u: type= %u, flags= 0x%x\n",
        Index,
        Interrupts[Index].Type,
        (UINT32)Interrupts[Index].Flags
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  *InterruptArray = Interrupts;
  return EFI_SUCCESS;
}

/** Get the serialized size of an AEST node's type-specific data.

  @param [in] NodeInfo  Resolved node information.

  @return The size of the node-specific data in bytes.
**/
STATIC
UINT32
GetAestNodeDataLength (
  IN CONST AEST_NODE_INFO  *CONST  NodeInfo
  )
{
  switch (NodeInfo->Type) {
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR:
      switch (NodeInfo->Data.Processor.ResourceType) {
        case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_CACHE:
          return sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_DATA) +
                 sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_CACHE_RESOURCE);
        case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_TLB:
          return sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_DATA) +
                 sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_TLB_RESOURCE);
        case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_GENERIC:
          return sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_DATA) +
                 sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_GENERIC_RESOURCE);
        default:
          ASSERT (FALSE);
          return 0;
      }

    case EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY:
      return sizeof (EFI_ACPI_AEST_2_0_MEMORY_DATA);
    case EFI_ACPI_AEST_2_0_NODE_TYPE_SMMU:
      return sizeof (EFI_ACPI_AEST_2_0_SMMU_DATA);
    case EFI_ACPI_AEST_2_0_NODE_TYPE_VENDOR:
      return sizeof (EFI_ACPI_AEST_2_0_VENDOR_DATA);
    case EFI_ACPI_AEST_2_0_NODE_TYPE_GIC:
      return sizeof (EFI_ACPI_AEST_2_0_GIC_DATA);
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PCIE:
      return sizeof (EFI_ACPI_AEST_2_0_PCIE_DATA);
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROXY:
      return sizeof (EFI_ACPI_AEST_2_0_PROXY_DATA);
    default:
      ASSERT (FALSE);
      return 0;
  }
}

/** Resolve and validate processor-specific node data.

  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in]      Source          Processor node CM object.
  @param [in, out] NodeInfo        Node information with resolved processor
                                   affinity; receives processor-specific data.

  @retval EFI_SUCCESS           Processor data is valid.
  @retval EFI_INVALID_PARAMETER Processor data or cache reference is invalid.
  @retval Others                Configuration Manager error.
**/
STATIC
EFI_STATUS
ResolveAndValidateProcessorData (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARM_AEST_PROCESSOR_NODE            *CONST  Source,
  IN OUT    AEST_NODE_INFO                        *CONST  NodeInfo
  )
{
  CM_ARCH_COMMON_CACHE_INFO  *Cache;
  EFI_STATUS                 Status;
  UINT32                     Count;

  // Reject reserved processor flag bits.
  if ((Source->Flags & ~EFI_ACPI_AEST_2_0_PROCESSOR_FLAGS_MASK) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Processor flags contain reserved bits 0x%x\n",
      (UINT32)(Source->Flags & ~EFI_ACPI_AEST_2_0_PROCESSOR_FLAGS_MASK)
      ));
    return EFI_INVALID_PARAMETER;
  }

  NodeInfo->Data.Processor.ResourceType = Source->ResourceType;
  NodeInfo->Data.Processor.Flags        = Source->Flags;
  switch (Source->ResourceType) {
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_CACHE:
      if (Source->Resource.CacheToken == CM_NULL_TOKEN) {
        DEBUG ((DEBUG_ERROR, "AEST: Cache processor resource requires a non-null cache token\n"));
        return EFI_INVALID_PARAMETER;
      }

      Cache  = NULL;
      Status = GetEArchCommonObjCacheInfo (
                 CfgMgrProtocol,
                 Source->Resource.CacheToken,
                 &Cache,
                 &Count
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((Count != 1) || (Cache == NULL) || (Cache->CacheId == 0)) {
        DEBUG ((
          DEBUG_ERROR,
          "AEST: Invalid cache reference: count= %u, data= 0x%p, CacheId= 0x%x\n",
          Count,
          Cache,
          (Cache == NULL) ? 0 : Cache->CacheId
          ));
        return EFI_INVALID_PARAMETER;
      }

      NodeInfo->Data.Processor.Resource.CacheId = Cache->CacheId;
      break;
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_TLB:
      NodeInfo->Data.Processor.Resource.TlbLevel = Source->Resource.TlbLevel;
      break;
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_GENERIC:
      NodeInfo->Data.Processor.Resource.GenericData = Source->Resource.GenericData;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "AEST: Invalid processor resource type %u\n", Source->ResourceType));
      return EFI_INVALID_PARAMETER;
  }

  // Cf. DEN0085 2.0 EAC2, Section 2.1.1, Table 5: Global or Shared nodes
  // require a zero processor ID. This generator uses the interface affinity
  // UID for a dedicated node.
  if ((Source->Flags & EFI_ACPI_AEST_2_0_PROCESSOR_FLAGS_MASK) == 0) {
    NodeInfo->Data.Processor.AcpiProcessorId = NodeInfo->Interface.ProcessorAffinity;
  } else {
    NodeInfo->Data.Processor.AcpiProcessorId = 0;
  }

  return EFI_SUCCESS;
}

/** Resolve and validate memory controller-specific node data.

  Cf. DEN0085 2.0 EAC2, Section 2.1.2, Table 10: the proximity domain
  must be zero when SRAT is absent. When SRAT is present, the CM token
  must identify one proximity domain object. Generated domain IDs are
  resolved through the shared metadata database.

  @param [in]      CfgMgrProtocol  Configuration Manager protocol.
  @param [in]      Source          Memory controller node CM object.
  @param [in, out] NodeInfo        Node information receiving memory data.

  @retval EFI_SUCCESS           Memory controller data was resolved.
  @retval EFI_NOT_FOUND         The proximity domain token was not found.
  @retval EFI_INVALID_PARAMETER The proximity domain token or object is invalid.
  @retval Others                Configuration Manager or metadata error.
**/
STATIC
EFI_STATUS
ResolveAndValidateMemoryData (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_ARM_AEST_MEMORY_NODE               *CONST  Source,
  IN OUT    AEST_NODE_INFO                        *CONST  NodeInfo
  )
{
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO  *ProximityDomain;
  METADATA_OBJ_PROXIMITY_DOMAIN         Metadata;
  EFI_STATUS                            Status;
  UINT32                                Count;

  NodeInfo->Data.Memory.ProximityDomain = 0;
  if (!mSratPresent) {
    return EFI_SUCCESS;
  }

  if (Source->ProximityDomainToken == CM_NULL_TOKEN) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Memory node requires a non-null proximity domain token "
      "when SRAT is requested\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjProximityDomainInfo (
             CfgMgrProtocol,
             Source->ProximityDomainToken,
             &ProximityDomain,
             &Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Failed to resolve memory proximity domain: token 0x%p, status %r\n",
      Source->ProximityDomainToken,
      Status
      ));
    return Status;
  }

  if ((Count != 1) || (ProximityDomain == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Invalid memory proximity domain reference: token 0x%p, count %u, data 0x%p\n",
      Source->ProximityDomainToken,
      Count,
      ProximityDomain
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (ProximityDomain->GenerateDomainId) {
    Status = MetadataHandlerGenerate (
               GetMetadataRoot (),
               MetadataTypeProximityDomain,
               Source->ProximityDomainToken,
               NULL,
               &Metadata,
               sizeof (Metadata)
               );
  } else {
    Metadata.Id = ProximityDomain->DomainId;
    Status      = MetadataAdd (
                    GetMetadataRoot (),
                    MetadataTypeProximityDomain,
                    Source->ProximityDomainToken,
                    &Metadata,
                    sizeof (Metadata)
                    );
    if (Status == EFI_ALREADY_STARTED) {
      Status = EFI_SUCCESS;
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Failed to resolve memory proximity domain metadata: token 0x%p, status %r\n",
      Source->ProximityDomainToken,
      Status
      ));
    return Status;
  }

  NodeInfo->Data.Memory.ProximityDomain = Metadata.Id;
  return EFI_SUCCESS;
}

/** Resolve, validate, and size one AEST node.

  @param [in]  CfgMgrProtocol  Configuration Manager protocol.
  @param [in]  NodeType        Type of the CM node object.
  @param [in]  Source          Common first member of the typed CM node.
  @param [out] NodeInfo        Resulting node information.

  @retval EFI_SUCCESS           Node information is complete.
  @retval EFI_UNSUPPORTED       Node type is not supported.
  @retval EFI_BAD_BUFFER_SIZE   Node is too large for its length field.
  @retval Others                Configuration Manager or validation error.
**/
STATIC
EFI_STATUS
ResolveAndValidateAestNode (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        UINT8                                         NodeType,
  IN  CONST CM_ARM_AEST_COMMON                    *CONST  Source,
  OUT       AEST_NODE_INFO                        *CONST  NodeInfo
  )
{
  EFI_STATUS  Status;
  UINT64      Length;

  NodeInfo->Type   = NodeType;
  NodeInfo->Source = Source;

  Status = ResolveProcessorAffinity (
             CfgMgrProtocol,
             &Source->Interface,
             &NodeInfo->Interface
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (NodeType) {
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR:
      Status = ResolveAndValidateProcessorData (
                 CfgMgrProtocol,
                 (CONST CM_ARM_AEST_PROCESSOR_NODE *)Source,
                 NodeInfo
                 );
      break;
    case EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY:
      Status = ResolveAndValidateMemoryData (
                 CfgMgrProtocol,
                 (CONST CM_ARM_AEST_MEMORY_NODE *)Source,
                 NodeInfo
                 );
      break;
    default:
      DEBUG ((DEBUG_ERROR, "AEST: Unsupported node type 0x%x\n", NodeType));
      return EFI_UNSUPPORTED;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ResolveAndValidateInterfaceData (
             CfgMgrProtocol,
             &Source->Interface,
             &NodeInfo->Interface
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Length = sizeof (EFI_ACPI_AEST_2_0_NODE) +
           GetAestNodeDataLength (NodeInfo) +
           sizeof (EFI_ACPI_AEST_2_0_INTERFACE_PREFIX) +
           (AEST_INTERFACE_BITMAP_COUNT * NodeInfo->Interface.BitmapWordCount * sizeof (UINT64)) +
           sizeof (EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX) +
           ((UINT64)Source->InterruptCount *
            sizeof (EFI_ACPI_AEST_2_0_INTERRUPT));
  if (Length > MAX_UINT16) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Node type 0x%x length %Lu exceeds the 16-bit node length field\n",
      NodeType,
      Length
      ));
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = ResolveAndValidateInterruptData (
             CfgMgrProtocol,
             Source,
             &NodeInfo->Interrupts
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NodeInfo->Length = (UINT16)Length;
  return EFI_SUCCESS;
}

/** Write the node-specific data for a validated processor node.

  @param [in]      NodeInfo  Resolved node information.
  @param [in, out] Buffer    Zeroed buffer for the node-specific data.
**/
STATIC
VOID
WriteProcessorNodeData (
  IN  CONST AEST_NODE_INFO  *CONST  NodeInfo,
  IN OUT    UINT8           *CONST  Buffer
  )
{
  EFI_ACPI_AEST_2_0_PROCESSOR_DATA  *Processor;
  UINT32                            Offset;

  Processor                  = (EFI_ACPI_AEST_2_0_PROCESSOR_DATA *)Buffer;
  Processor->AcpiProcessorId = NodeInfo->Data.Processor.AcpiProcessorId;
  Processor->ResourceType    = NodeInfo->Data.Processor.ResourceType;
  Processor->Flags           = NodeInfo->Data.Processor.Flags;
  Offset                     = sizeof (EFI_ACPI_AEST_2_0_PROCESSOR_DATA);
  switch (NodeInfo->Data.Processor.ResourceType) {
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_CACHE:
      ((EFI_ACPI_AEST_2_0_PROCESSOR_CACHE_RESOURCE *)(Buffer + Offset))->CacheReference =
        NodeInfo->Data.Processor.Resource.CacheId;
      break;
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_TLB:
      ((EFI_ACPI_AEST_2_0_PROCESSOR_TLB_RESOURCE *)(Buffer + Offset))->TlbLevel =
        NodeInfo->Data.Processor.Resource.TlbLevel;
      break;
    case EFI_ACPI_AEST_2_0_PROCESSOR_RESOURCE_GENERIC:
      ((EFI_ACPI_AEST_2_0_PROCESSOR_GENERIC_RESOURCE *)(Buffer + Offset))->Data =
        NodeInfo->Data.Processor.Resource.GenericData;
      break;
    default:
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Unsupported processor resource type %u\n",
        NodeInfo->Data.Processor.ResourceType
        ));
      ASSERT (FALSE);
  }
}

/** Write a validated AEST node interface.

  @param [in]      InterfaceInfo  Resolved interface information.
  @param [in, out] Buffer         Zeroed buffer for the interface.

  @return The size of the serialized interface in bytes.
**/
STATIC
UINT32
WriteAestInterface (
  IN  CONST AEST_INTERFACE_INFO  *CONST  InterfaceInfo,
  IN OUT    UINT8                *CONST  Buffer
  )
{
  CONST CM_ARM_AEST_INTERFACE         *CmInterface;
  EFI_ACPI_AEST_2_0_INTERFACE_PREFIX  *Interface;
  EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX  *Suffix;
  UINT32                              Offset;
  UINT32                              MapIndex;
  UINT32                              WordIndex;

  CmInterface            = InterfaceInfo->Source;
  Interface              = (EFI_ACPI_AEST_2_0_INTERFACE_PREFIX *)Buffer;
  Interface->Type        = CmInterface->Type;
  Interface->GroupFormat = CmInterface->GroupFormat;
  Interface->Flags       = CmInterface->Flags;

  if (CmInterface->Type != EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SR) {
    Interface->BaseAddress = CmInterface->BaseAddress;
  }

  Interface->StartErrorRecordIndex = CmInterface->StartErrorRecordIndex;
  Interface->NumberErrorRecords    = CmInterface->NumberErrorRecords;
  Offset                           = sizeof (EFI_ACPI_AEST_2_0_INTERFACE_PREFIX);
  for (MapIndex = 0; MapIndex < ARRAY_SIZE (InterfaceInfo->Bitmaps); MapIndex++) {
    for (WordIndex = 0; WordIndex < InterfaceInfo->BitmapWordCount; WordIndex++) {
      CopyMem (
        Buffer + Offset,
        &InterfaceInfo->Bitmaps[MapIndex][WordIndex].Word,
        sizeof (UINT64)
        );
      Offset += sizeof (UINT64);
    }
  }

  Suffix = (EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX *)(Buffer + Offset);
  if ((CmInterface->Flags & EFI_ACPI_AEST_2_0_INTERFACE_FLAG_DEVICE_VALID) != 0) {
    Suffix->ArmErrorNodeDeviceUid = CmInterface->ArmErrorNodeDeviceUid;
  }

  Suffix->ProcessorAffinity = InterfaceInfo->ProcessorAffinity;
  if ((CmInterface->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_MMIO) &&
      ((CmInterface->Flags & EFI_ACPI_AEST_2_0_INTERFACE_FLAG_GROUP_ADDRESS_VALID) != 0))
  {
    Suffix->ErrorGroupRegistersBaseAddress = CmInterface->GroupStatusRegistersBaseAddress;
  }

  if ((CmInterface->Type != EFI_ACPI_AEST_2_0_INTERFACE_TYPE_SR) &&
      ((CmInterface->Flags & EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INJECTION_VALID) != 0))
  {
    Suffix->FaultInjectionRegistersBaseAddress = CmInterface->FaultInjectionRegistersBaseAddress;
  }

  if ((CmInterface->Type == EFI_ACPI_AEST_2_0_INTERFACE_TYPE_MMIO) &&
      ((CmInterface->Flags & EFI_ACPI_AEST_2_0_INTERFACE_FLAG_INTERRUPT_CFG_VALID) != 0))
  {
    Suffix->InterruptConfigurationRegistersBaseAddress =
      CmInterface->InterruptConfigurationRegistersBaseAddress;
  }

  return Offset + sizeof (EFI_ACPI_AEST_2_0_INTERFACE_SUFFIX);
}

/** Write the validated interrupt array for an AEST node.

  @param [in]      Interrupts  Resolved interrupt array.
  @param [in]      Count       Number of interrupt entries.
  @param [in, out] Buffer      Zeroed buffer for the interrupt array.
**/
STATIC
VOID
WriteAestInterrupts (
  IN  CONST CM_ARM_AEST_INTERRUPT  *CONST  Interrupts,
  IN        UINT32                         Count,
  IN OUT    UINT8                  *CONST  Buffer
  )
{
  EFI_ACPI_AEST_2_0_INTERRUPT  *Interrupt;
  UINT32                       Index;

  Interrupt = (EFI_ACPI_AEST_2_0_INTERRUPT *)Buffer;
  for (Index = 0; Index < Count; Index++) {
    Interrupt[Index].Type  = Interrupts[Index].Type;
    Interrupt[Index].Flags = Interrupts[Index].Flags;
    Interrupt[Index].Gsiv  = Interrupts[Index].Gsiv;
  }
}

/** Write a validated AEST node into its table allocation.

  @param [in]      NodeInfo  Resolved node information.
  @param [in, out] Buffer    Zeroed buffer of NodeInfo->Length bytes.
**/
STATIC
VOID
WriteAestNode (
  IN  CONST AEST_NODE_INFO  *CONST  NodeInfo,
  IN OUT    UINT8           *CONST  Buffer
  )
{
  CONST CM_ARM_AEST_COMMON       *Source;
  EFI_ACPI_AEST_2_0_NODE         *Node;
  EFI_ACPI_AEST_2_0_MEMORY_DATA  *Memory;
  UINT32                         Offset;

  Source                            = NodeInfo->Source;
  Node                              = (EFI_ACPI_AEST_2_0_NODE *)Buffer;
  Node->Type                        = NodeInfo->Type;
  Node->Length                      = NodeInfo->Length;
  Node->DataOffset                  = sizeof (EFI_ACPI_AEST_2_0_NODE);
  Node->InterfaceOffset             = Node->DataOffset + GetAestNodeDataLength (NodeInfo);
  Node->InterruptArrayCount         = Source->InterruptCount;
  Node->TimestampRate               = Source->TimestampRate;
  Node->TimebaseOffset              = Source->TimebaseOffset;
  Node->ErrorInjectionCountdownRate = Source->ErrorInjectionCountdownRate;

  switch (NodeInfo->Type) {
    case EFI_ACPI_AEST_2_0_NODE_TYPE_PROCESSOR:
      WriteProcessorNodeData (NodeInfo, Buffer + Node->DataOffset);
      break;
    case EFI_ACPI_AEST_2_0_NODE_TYPE_MEMORY:
      Memory                  = (EFI_ACPI_AEST_2_0_MEMORY_DATA *)(Buffer + Node->DataOffset);
      Memory->ProximityDomain = NodeInfo->Data.Memory.ProximityDomain;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "AEST: Unsupported node type 0x%x\n", NodeInfo->Type));
      ASSERT (FALSE);
      return;
  }

  Offset  = Node->InterfaceOffset;
  Offset += WriteAestInterface (&NodeInfo->Interface, Buffer + Offset);
  if (Source->InterruptCount != 0) {
    Node->InterruptArrayOffset = Offset;
    WriteAestInterrupts (NodeInfo->Interrupts, Source->InterruptCount, Buffer + Offset);
    Offset += Source->InterruptCount * sizeof (EFI_ACPI_AEST_2_0_INTERRUPT);
  }

  ASSERT (Offset == NodeInfo->Length);
}

/** Construct an AEST table from the configured error node groups.

    @param [in]  This           Pointer to the table generator.
    @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
    @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                                Protocol Interface.
    @param [out] Table          Pointer to the constructed ACPI Table.

    @retval EFI_SUCCESS           The table was generated successfully.
    @retval EFI_INVALID_PARAMETER A parameter or CM object is invalid.
    @retval EFI_UNSUPPORTED       A requested node or revision is unsupported.
    @retval EFI_BAD_BUFFER_SIZE   A node or the whole table is too large.
    @retval Others                Configuration Manager or allocation error.
**/
STATIC
EFI_STATUS
EFIAPI
BuildAestTable (
  IN  CONST ACPI_TABLE_GENERATOR                          *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   **Table
  )
{
  CONST CM_ARM_AEST_COMMON  *Source;
  AEST_NODE_GROUP           *NodeGroup;
  AEST_NODE_INFO            *NodeInfos;
  EFI_ACPI_AEST_2_0_TABLE   *Aest;
  EFI_STATUS                Status;
  UINTN                     NodeGroupIndex;
  UINT32                    GroupNodeIndex;
  UINTN                     NodeInfoIndex;
  UINTN                     TotalNodeCount;
  UINTN                     NodeInfoAllocationSize;
  UINT32                    Offset;
  UINT64                    TableSize;

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
      "ERROR: AEST: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  NodeInfos      = NULL;
  TotalNodeCount = 0;
  for (NodeGroupIndex = 0; NodeGroupIndex < ARRAY_SIZE (mNodeGroups); NodeGroupIndex++) {
    NodeGroup = &mNodeGroups[NodeGroupIndex];
    if (!NodeGroup->Supported) {
      continue;
    }

    Status = GetAestNodeList (CfgMgrProtocol, NodeGroup->NodeType, &NodeGroup->List, &NodeGroup->Count);
    if (Status == EFI_NOT_FOUND) {
      NodeGroup->List  = NULL;
      NodeGroup->Count = 0;
      continue;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    if ((NodeGroup->Count != 0) && (NodeGroup->List == NULL)) {
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Node type 0x%x has %u nodes but a null object list\n",
        NodeGroup->NodeType,
        NodeGroup->Count
        ));
      return EFI_INVALID_PARAMETER;
    }

    TotalNodeCount += NodeGroup->Count;
  }

  Status = CheckTableDependencies (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NodeInfoAllocationSize = TotalNodeCount * sizeof (*NodeInfos);
  if (TotalNodeCount != 0) {
    NodeInfos = AllocateZeroPool (NodeInfoAllocationSize);
    if (NodeInfos == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "AEST: Failed to allocate node information\n"
        ));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  TableSize     = sizeof (EFI_ACPI_AEST_2_0_TABLE);
  NodeInfoIndex = 0;
  for (NodeGroupIndex = 0; NodeGroupIndex < ARRAY_SIZE (mNodeGroups); NodeGroupIndex++) {
    NodeGroup = &mNodeGroups[NodeGroupIndex];
    for (GroupNodeIndex = 0; GroupNodeIndex < NodeGroup->Count; GroupNodeIndex++) {
      Source = (CONST CM_ARM_AEST_COMMON *)((CONST UINT8 *)NodeGroup->List +
                                            (GroupNodeIndex * NodeGroup->ObjectSize));
      Status = ResolveAndValidateAestNode (
                 CfgMgrProtocol,
                 NodeGroup->NodeType,
                 Source,
                 &NodeInfos[NodeInfoIndex]
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "AEST: Failed to resolve or validate node type 0x%x, index %u: %r\n",
          NodeGroup->NodeType,
          GroupNodeIndex,
          Status
          ));
        goto Done;
      }

      TableSize += NodeInfos[NodeInfoIndex].Length;
      if (TableSize > MAX_UINT32) {
        Status = EFI_BAD_BUFFER_SIZE;
        goto Done;
      }

      NodeInfoIndex++;
    }
  }

  Aest = AllocateZeroPool (TableSize);
  if (Aest == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "AEST: Failed to allocate %u bytes for table\n",
      (UINT32)TableSize
      ));
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Aest->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    FreePool (Aest);
    goto Done;
  }

  Offset = sizeof (EFI_ACPI_AEST_2_0_TABLE);
  for (NodeInfoIndex = 0; NodeInfoIndex < TotalNodeCount; NodeInfoIndex++) {
    WriteAestNode (&NodeInfos[NodeInfoIndex], (UINT8 *)Aest + Offset);
    Offset += NodeInfos[NodeInfoIndex].Length;
  }

  ASSERT (Offset == TableSize);
  *Table = &Aest->Header;

Done:
  if (NodeInfos != NULL) {
    FreePool (NodeInfos);
  }

  return Status;
}

/** Free resources allocated for the AEST table.

    @param [in]      This           Pointer to the table generator.
    @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
    @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                    Protocol Interface.
    @param [in, out] Table          Pointer to the ACPI Table.

    @retval EFI_SUCCESS           The table was freed or no table was allocated.
    @retval EFI_INVALID_PARAMETER A parameter is NULL.
**/
STATIC
EFI_STATUS
EFIAPI
FreeAestTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                          *This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  if ((This == NULL) ||
      (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return EFI_SUCCESS;
}

/** The interface for the AEST Table Generator.
*/
STATIC ACPI_TABLE_GENERATOR  mAestGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdAest),
  // Generator Description
  L"ACPI.STD.AEST.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_AEST_2_0_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_AEST_2_0_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_AEST_2_0_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  AEST_GENERATOR_REVISION,
  // Build Table function
  BuildAestTable,
  // Free Resource function
  FreeAestTableResources,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the AEST generator with the ACPI table factory.

    @param [in]  ImageHandle  The handle to the image.
    @param [in]  SystemTable  Pointer to the System Table.

    @retval EFI_SUCCESS           The generator is registered.
    @retval EFI_INVALID_PARAMETER The generator is invalid.
    @retval EFI_ALREADY_STARTED   The generator for the table ID is already
                                  registered.
**/
EFI_STATUS
EFIAPI
AcpiAestLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mAestGenerator);
  DEBUG ((DEBUG_INFO, "AEST: Register generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the AEST generator from the ACPI table factory.

    @param [in]  ImageHandle  The handle to the image.
    @param [in]  SystemTable  Pointer to the System Table.

    @retval EFI_SUCCESS           The generator is deregistered.
    @retval EFI_INVALID_PARAMETER The generator is invalid.
    @retval EFI_NOT_FOUND         The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiAestLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mAestGenerator);
  DEBUG ((DEBUG_INFO, "AEST: Deregister generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
