/** @file
  SMBIOS Type 9 System Slots Table Generator.

  Copyright (c) 2024 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 9 System Slots Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSystemSlotInfo
  - EArchCommonObjSystemSlotPeerInfo
    Required only when a system slot provides a non-null
    PeerGroupListToken.
*/

#define SMBIOS_TYPE9_MAX_STRINGS         1
#define SMBIOS_TYPE9_BASE_LENGTH         OFFSET_OF (SMBIOS_TABLE_TYPE9, PeerGroups)
#define SMBIOS_TYPE9_EXTENDED_LENGTH     sizeof (SMBIOS_TABLE_TYPE9_EXTENDED)
#define SMBIOS_TYPE9_UNKNOWN_CHAR_BIT    BIT0
#define SMBIOS_TYPE9_DATA_BUS_WIDTH_MIN  1
#define SMBIOS_TYPE9_DATA_BUS_WIDTH_MAX  32

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemSlotInfo,
  CM_ARCH_COMMON_SYSTEM_SLOT_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemSlotPeerInfo,
  CM_ARCH_COMMON_SYSTEM_SLOT_PEER_INFO
  );

/** Check whether a slot type is defined by SMBIOS 3.9.0.

  @param [in] SlotType  Slot type to validate.

  @retval TRUE   The slot type is defined and is not Unknown.
  @retval FALSE  The slot type is reserved or Unknown.
**/
STATIC
BOOLEAN
IsSlotTypeValid (
  IN UINT8  SlotType
  )
{
  return ((SlotType == SlotTypeOther) ||
          ((SlotType >= SlotTypeIsa) && (SlotType <= SlotTypeOCPNICPriorto30)) ||
          (SlotType == SlotTypeCXLFlexbus10) ||
          ((SlotType >= SlotTypePC98C20) && (SlotType <= SlotTypePciExpressGen3X16)) ||
          ((SlotType >= SlotTypePciExpressGen4) &&
           (SlotType <= SlotTypeEnterpriseandDatacenter3E3FormFactorSlot)));
}

/** Check whether a slot type provides device presence detection.

  SMBIOS Specification v3.9.0, Section 6.2 and Annex A, Section 4.6.6,
  require Current Usage to be known for slots with presence detection.

  @param [in] SlotType  Slot type to check.

  @retval TRUE   The slot type provides device presence detection.
  @retval FALSE  The slot type does not provide device presence detection.
**/
STATIC
BOOLEAN
SlotTypeHasPresenceDetection (
  IN UINT8  SlotType
  )
{
  return ((SlotType == SlotTypePci) ||
          (SlotType == SlotTypePci66MhzCapable) ||
          ((SlotType >= SlotTypeAgp) && (SlotType <= SlotTypeAgp4X)) ||
          (SlotType == SlotTypePciX) ||
          (SlotType == SlotTypeAgp8X) ||
          ((SlotType >= SlotTypeM2Socket1_DP) && (SlotType <= SlotTypeOCPNICPriorto30)) ||
          (SlotType == SlotTypeCXLFlexbus10) ||
          ((SlotType >= SlotTypePciExpress) && (SlotType <= SlotTypePciExpressGen3X16)) ||
          ((SlotType >= SlotTypePciExpressGen4) &&
           (SlotType <= SlotTypeEnterpriseandDatacenter3E3FormFactorSlot)));
}

/** Validate a System Slot Information CM object.

  Validation follows SMBIOS Specification v3.9.0, Section 7.10 and Annex A.

  @param [in] SystemSlotInfo  System Slot Information to validate.

  @retval EFI_SUCCESS            The System Slot Information is valid.
  @retval EFI_INVALID_PARAMETER  The System Slot Information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemSlotInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_SLOT_INFO  *SystemSlotInfo
  )
{
  if ((SystemSlotInfo->SystemSlotInfoToken == CM_NULL_TOKEN) ||
      (SystemSlotInfo->SlotDesignation[0] == '\0') ||
      !IsSlotTypeValid (SystemSlotInfo->SlotType) ||
      (SystemSlotInfo->SlotDataBusWidth < SlotDataBusWidthOther) ||
      (SystemSlotInfo->SlotDataBusWidth > SlotDataBusWidth32X) ||
      (SystemSlotInfo->SlotDataBusWidth == SlotDataBusWidthUnknown) ||
      (SystemSlotInfo->CurrentUsage < SlotUsageOther) ||
      (SystemSlotInfo->CurrentUsage > SlotUsageUnavailable) ||
      (SystemSlotInfo->SlotLength < SlotLengthOther) ||
      (SystemSlotInfo->SlotLength > SlotLength3_5InchDriveFormFactor) ||
      ((SystemSlotInfo->SlotCharacteristics1 & SMBIOS_TYPE9_UNKNOWN_CHAR_BIT) != 0) ||
      (SystemSlotInfo->DataBusWidth < SMBIOS_TYPE9_DATA_BUS_WIDTH_MIN) ||
      (SystemSlotInfo->DataBusWidth > SMBIOS_TYPE9_DATA_BUS_WIDTH_MAX) ||
      (SystemSlotInfo->SlotPhysicalWidth < SlotPhysicalWidthOther) ||
      (SystemSlotInfo->SlotPhysicalWidth > SlotPhysicalWidth32X) ||
      (SystemSlotInfo->SlotHeight > SlotHeightLowProfile))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid System Slot Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemSlotInfo->CurrentUsage == SlotUsageUnknown) &&
      SlotTypeHasPresenceDetection (SystemSlotInfo->SlotType))
  {
    DEBUG ((DEBUG_ERROR, "%a: Current Usage is unknown for a presence-detectable slot\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((SystemSlotInfo->SlotType == SlotTypePCIExpressGen6andBeyond) &&
      (SystemSlotInfo->SlotInformation < Gen6))
  {
    DEBUG ((DEBUG_ERROR, "%a: PCIe Gen 6 slot requires Gen 6 or later SlotInformation\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Validate a System Slot Peer Information CM object.

  SMBIOS Specification v3.9.0, Section 7.10.9, requires the base device
  address to be lower than every peer device address.

  @param [in] SystemSlotInfo  System Slot Information containing the base device.
  @param [in] PeerInfo        System Slot Peer Information to validate.

  @retval EFI_SUCCESS            The peer information is valid.
  @retval EFI_INVALID_PARAMETER  The peer information is invalid.
**/
STATIC
EFI_STATUS
ValidateSystemSlotPeerInfo (
  IN CONST CM_ARCH_COMMON_SYSTEM_SLOT_INFO       *SystemSlotInfo,
  IN CONST CM_ARCH_COMMON_SYSTEM_SLOT_PEER_INFO  *PeerInfo
  )
{
  if ((PeerInfo->DataBusWidth < SMBIOS_TYPE9_DATA_BUS_WIDTH_MIN) ||
      (PeerInfo->DataBusWidth > SMBIOS_TYPE9_DATA_BUS_WIDTH_MAX))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid peer DataBusWidth 0x%x\n", __func__, PeerInfo->DataBusWidth));
    return EFI_INVALID_PARAMETER;
  }

  if ((PeerInfo->SegmentGroupNum < SystemSlotInfo->SegmentGroupNum) ||
      ((PeerInfo->SegmentGroupNum == SystemSlotInfo->SegmentGroupNum) &&
       (PeerInfo->BusNum < SystemSlotInfo->BusNum)) ||
      ((PeerInfo->SegmentGroupNum == SystemSlotInfo->SegmentGroupNum) &&
       (PeerInfo->BusNum == SystemSlotInfo->BusNum) &&
       (PeerInfo->DevFuncNum <= SystemSlotInfo->DevFuncNum)))
  {
    DEBUG ((DEBUG_ERROR, "%a: Peer device must follow the base device\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Construct SMBIOS Type 9 tables describing system slots.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No System Slot Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType9TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                            Status;
  CM_ARCH_COMMON_SYSTEM_SLOT_INFO       *SystemSlotInfo;
  CM_ARCH_COMMON_SYSTEM_SLOT_PEER_INFO  *PeerInfo;
  UINT32                                SystemSlotCount;
  UINT32                                PeerCount;
  SMBIOS_STRUCTURE                      **TableList;
  CM_OBJECT_TOKEN                       *CmObjectList;
  SMBIOS_TABLE_TYPE9                    *SmbiosRecord;
  SMBIOS_TABLE_TYPE9_EXTENDED           *Type9Extended;
  STRING_TABLE                          StrTable;
  BOOLEAN                               StringTableInitialized;
  SMBIOS_TABLE_STRING                   SlotDesignationRef;
  UINTN                                 FormattedLength;
  UINTN                                 Index;
  UINTN                                 PeerIndex;

  ASSERT (This != NULL);
  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) || (TableCount == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table                 = NULL;
  *CmObjectToken         = NULL;
  *TableCount            = 0;
  TableList              = NULL;
  CmObjectList           = NULL;
  SmbiosRecord           = NULL;
  StringTableInitialized = FALSE;

  Status = GetEArchCommonObjSystemSlotInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SystemSlotInfo,
             &SystemSlotCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get System Slot objects: %r\n", __func__, Status));
    return Status;
  }

  if (SystemSlotCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((SystemSlotInfo == NULL) ||
      (SystemSlotCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (SystemSlotCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid System Slot object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * SystemSlotCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * SystemSlotCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < SystemSlotCount; Index++) {
    Status = ValidateSystemSlotInfo (&SystemSlotInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    PeerInfo  = NULL;
    PeerCount = 0;
    if (SystemSlotInfo[Index].PeerGroupListToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjSystemSlotPeerInfo (
                 CfgMgrProtocol,
                 SystemSlotInfo[Index].PeerGroupListToken,
                 &PeerInfo,
                 &PeerCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to get peers for slot %u: %r\n", __func__, Index, Status));
        goto ErrorExit;
      }

      if (PeerCount == 0) {
        DEBUG ((DEBUG_ERROR, "%a: Peer list is empty for slot %u\n", __func__, Index));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }
    }

    if (PeerCount > MAX_UINT8) {
      DEBUG ((DEBUG_ERROR, "%a: Too many peers for slot %u: %u\n", __func__, Index, PeerCount));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    FormattedLength = SMBIOS_TYPE9_BASE_LENGTH +
                      (PeerCount * sizeof (MISC_SLOT_PEER_GROUP)) +
                      SMBIOS_TYPE9_EXTENDED_LENGTH;
    if (FormattedLength > MAX_UINT8) {
      DEBUG ((DEBUG_ERROR, "%a: Formatted length exceeds Hdr.Length for slot %u\n", __func__, Index));
      Status = EFI_INVALID_PARAMETER;
      goto ErrorExit;
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE9_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = StringTableAddString (
                               &StrTable,
                               SystemSlotInfo[Index].SlotDesignation,
                               &SlotDesignationRef
                               );
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (FormattedLength, &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type         = EFI_SMBIOS_TYPE_SYSTEM_SLOTS;
    SmbiosRecord->Hdr.Length       = (UINT8)FormattedLength;
    SmbiosRecord->SlotDesignation  = SlotDesignationRef;
    SmbiosRecord->SlotType         = SystemSlotInfo[Index].SlotType;
    SmbiosRecord->SlotDataBusWidth = SystemSlotInfo[Index].SlotDataBusWidth;
    SmbiosRecord->CurrentUsage     = SystemSlotInfo[Index].CurrentUsage;
    SmbiosRecord->SlotLength       = SystemSlotInfo[Index].SlotLength;
    WriteUnaligned16 (&SmbiosRecord->SlotID, SystemSlotInfo[Index].SlotId);
    *(UINT8 *) &SmbiosRecord->SlotCharacteristics1 = SystemSlotInfo[Index].SlotCharacteristics1;
    *(UINT8 *) &SmbiosRecord->SlotCharacteristics2 = SystemSlotInfo[Index].SlotCharacteristics2;
    WriteUnaligned16 (&SmbiosRecord->SegmentGroupNum, SystemSlotInfo[Index].SegmentGroupNum);
    SmbiosRecord->BusNum            = SystemSlotInfo[Index].BusNum;
    SmbiosRecord->DevFuncNum        = SystemSlotInfo[Index].DevFuncNum;
    SmbiosRecord->DataBusWidth      = SystemSlotInfo[Index].DataBusWidth;
    SmbiosRecord->PeerGroupingCount = (UINT8)PeerCount;

    if (PeerCount != 0) {
      if (PeerInfo == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: Peer list is NULL for slot %u\n", __func__, Index));
        Status = EFI_INVALID_PARAMETER;
        goto ErrorExit;
      }

      for (PeerIndex = 0; PeerIndex < PeerCount; PeerIndex++) {
        Status = ValidateSystemSlotPeerInfo (&SystemSlotInfo[Index], &PeerInfo[PeerIndex]);
        if (EFI_ERROR (Status)) {
          goto ErrorExit;
        }

        WriteUnaligned16 (
          &SmbiosRecord->PeerGroups[PeerIndex].SegmentGroupNum,
          PeerInfo[PeerIndex].SegmentGroupNum
          );
        SmbiosRecord->PeerGroups[PeerIndex].BusNum       = PeerInfo[PeerIndex].BusNum;
        SmbiosRecord->PeerGroups[PeerIndex].DevFuncNum   = PeerInfo[PeerIndex].DevFuncNum;
        SmbiosRecord->PeerGroups[PeerIndex].DataBusWidth = PeerInfo[PeerIndex].DataBusWidth;
      }
    }

    Type9Extended = (SMBIOS_TABLE_TYPE9_EXTENDED *)(
                                                    (UINT8 *)SmbiosRecord +
                                                    SMBIOS_TYPE9_BASE_LENGTH +
                                                    (PeerCount * sizeof (MISC_SLOT_PEER_GROUP))
                                                    );
    Type9Extended->SlotInformation   = SystemSlotInfo[Index].SlotInformation;
    Type9Extended->SlotPhysicalWidth = SystemSlotInfo[Index].SlotPhysicalWidth;
    WriteUnaligned16 (&Type9Extended->SlotPitch, SystemSlotInfo[Index].SlotPitch);
    Type9Extended->SlotHeight = SystemSlotInfo[Index].SlotHeight;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)SmbiosRecord + FormattedLength,
               StringTableGetStringSetSize (&StrTable)
               );
    StringTableFree (&StrTable);
    StringTableInitialized = FALSE;
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = SystemSlotInfo[Index].SystemSlotInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = SystemSlotCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < SystemSlotCount; Index++) {
      if (TableList[Index] != NULL) {
        FreePool (TableList[Index]);
      }
    }

    FreePool (TableList);
  }

  if (CmObjectList != NULL) {
    FreePool (CmObjectList);
  }

  return Status;
}

/** Free resources allocated when installing SMBIOS Type 9 tables.

  @param [in] This                  Pointer to the SMBIOS table generator.
  @param [in] TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in] SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in] CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [in] Table                 Pointer to the SMBIOS tables.
  @param [in] CmObjectToken         Pointer to the CM ObjectToken array.
  @param [in] TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType9TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL     *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO             *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL     *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                             ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                        **CmObjectToken,
  IN      CONST UINTN                                            TableCount
  )
{
  UINTN  Index;

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    for (Index = 0; Index < TableCount; Index++) {
      if ((*Table)[Index] != NULL) {
        FreePool ((*Table)[Index]);
      }
    }

    FreePool (*Table);
    *Table = NULL;
  }

  if (*CmObjectToken != NULL) {
    FreePool (*CmObjectToken);
    *CmObjectToken = NULL;
  }

  return EFI_SUCCESS;
}

/** The SMBIOS Type 9 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType9Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType09),
  L"SMBIOS.TYPE9.GENERATOR",
  EFI_SMBIOS_TYPE_SYSTEM_SLOTS,
  NULL,
  NULL,
  BuildSmbiosType9TableEx,
  FreeSmbiosType9TableEx
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is registered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_ALREADY_STARTED    The Generator is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType9LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType9Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 9: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType9LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType9Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 9: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
