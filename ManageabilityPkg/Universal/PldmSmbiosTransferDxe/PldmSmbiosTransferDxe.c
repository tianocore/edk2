/** @file
  This file provides edk2 PLDM SMBIOS Transfer Protocol implementation.

  Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.<BR>
  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
   Platform Level Data Model (PLDM) for SMBIOS Data Transfer Specification
   Version 1.0.1
   https://www.dmtf.org/sites/default/files/standards/documents/DSP0246_1.0.1.pdf
**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BasePldmProtocolLib.h>
#include <Library/ManageabilityTransportHelperLib.h>
#include <IndustryStandard/PldmSmbiosTransfer.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/PldmSmbiosTransferProtocol.h>
#include <Protocol/Smbios.h>

UINT32  SetSmbiosStructureTableHandle;

/**
  This function sets PLDM SMBIOS transfer source and destination
  PLDM terminus ID.

  @param [in]   This           EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   SourceId       PLDM source teminus ID.
                               Set to PLDM_TERMINUS_ID_UNASSIGNED means use
                               platform default PLDM terminus ID.
                               (gManageabilityPkgTokenSpaceGuid.PcdPldmSourceTerminusId)
  @param [in]   DestinationId  PLDM destination teminus ID.
                               Set to PLDM_TERMINUS_ID_UNASSIGNED means use
                               platform default PLDM terminus ID.
                               (gManageabilityPkgTokenSpaceGuid.PcdPldmDestinationEndpointId)

  @retval       EFI_SUCCESS            Get SMBIOS table metadata Successfully.
  @retval       EFI_INVALID_PARAMETER  Invalid value of source or destination
                                       PLDM terminus ID.
**/
EFI_STATUS
EFIAPI
SetPldmSmbiosTransferTerminusId (
  IN  UINT8  SourceId,
  IN  UINT8  DestinationId
  )
{
  return PldmSetTerminus (SourceId, DestinationId);
}

/**
  Get the full size of SMBIOS structure including optional strings that follow the formatted structure.

  @param Head                   Pointer to the beginning of SMBIOS structure.
  @param NumberOfStrings        The returned number of optional strings that follow the formatted structure.

  @return Size                  The returned size.
**/
UINTN
GetSmbiosStructureSize (
  IN   EFI_SMBIOS_TABLE_HEADER  *Head,
  OUT  UINTN                    *NumberOfStrings
  )
{
  UINTN  Size;
  UINTN  StrLen;
  CHAR8  *CharInStr;
  UINTN  StringsNumber;

  CharInStr     = (CHAR8 *)Head + Head->Length;
  Size          = Head->Length;
  StringsNumber = 0;
  StrLen        = 0;
  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      Size += 1;
      CharInStr++;
    }

    for (StrLen = 0; StrLen < SMBIOS_STRING_MAX_LENGTH; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    if (StrLen == SMBIOS_STRING_MAX_LENGTH) {
      return 0;
    }

    //
    // forward the pointer
    //
    CharInStr     += StrLen;
    Size          += StrLen;
    StringsNumber += 1;
  }

  //
  // count ending two zeros.
  //
  Size += 2;

  if (NumberOfStrings != NULL) {
    *NumberOfStrings = StringsNumber;
  }

  return Size;
}

/**

  This function returns full SMBIOS table length.

  @param  TableAddress      SMBIOS table based address
  @param  TableMaximumSize  Maximum size of SMBIOS table

  @return SMBIOS table length

**/
UINTN
GetSmbiosTableLength (
  IN VOID   *TableAddress,
  IN UINTN  TableMaximumSize
  )
{
  VOID   *TableEntry;
  VOID   *TableAddressEnd;
  UINTN  TableEntryLength;

  TableAddressEnd = (VOID *)((UINTN)TableAddress + TableMaximumSize);
  TableEntry      = TableAddress;
  while (TableEntry < TableAddressEnd) {
    TableEntryLength = GetSmbiosStructureSize (TableEntry, NULL);
    if (TableEntryLength == 0) {
      break;
    }

    if (((SMBIOS_STRUCTURE *)TableEntry)->Type == 127) {
      TableEntry = (VOID *)((UINTN)TableEntry + TableEntryLength);
      break;
    }

    TableEntry = (VOID *)((UINTN)TableEntry + TableEntryLength);
  }

  return ((UINTN)TableEntry - (UINTN)TableAddress);
}

/**
  This function gets SMBIOS table metadata.

  @param [in]   This         EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [out]  Buffer       Buffer to receive the SMBIOS table metadata.

  @retval       EFI_SUCCESS            Get SMBIOS table metadata Successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to get SMBIOS table metadata.
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureTableMetaData (
  IN  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL   *This,
  OUT PLDM_SMBIOS_STRUCTURE_TABLE_METADATA  *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseSize;

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Set SMBIOS structure table metafile.\n", __func__));

  ResponseSize = sizeof (PLDM_SMBIOS_STRUCTURE_TABLE_METADATA);
  Status       = PldmSubmitCommand (
                   PLDM_TYPE_SMBIOS,
                   PLDM_GET_SMBIOS_STRUCTURE_TABLE_METADATA_COMMAND_CODE,
                   NULL,
                   0,
                   (UINT8 *)Buffer,
                   &ResponseSize
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fails to get SMBIOS structure table metafile.\n", __func__));
  }

  if (ResponseSize != 0) {
    HelperManageabilityDebugPrint (
      (VOID *)Buffer,
      ResponseSize,
      "SMBIOS structure table metafile got from BMC.\n"
      );
  }

  return Status;
}

/**
  This function sets SMBIOS table metadata.

  @param [in]   This         EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.

  @retval       EFI_SUCCESS            Set SMBIOS table metadata Successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to set SMBIOS table metadata.
**/
EFI_STATUS
EFIAPI
SetSmbiosStructureTableMetaData (
  IN  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL   *This,
  IN  PLDM_SMBIOS_STRUCTURE_TABLE_METADATA  *Buffer
  )
{
  EFI_STATUS  Status;
  UINT32      ResponseSize;
  UINT32      RequestSize;

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Get SMBIOS structure table metafile.\n", __func__));

  RequestSize  = sizeof (PLDM_SMBIOS_STRUCTURE_TABLE_METADATA);
  ResponseSize = 0;

  Status = PldmSubmitCommand (
             PLDM_TYPE_SMBIOS,
             PLDM_SET_SMBIOS_STRUCTURE_TABLE_METADATA_COMMAND_CODE,
             (UINT8 *)Buffer,
             RequestSize,
             (UINT8 *)NULL,
             &ResponseSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fails to set SMBIOS structure table metafile.\n", __func__));
  }

  return Status;
}

/**
  This function gets SMBIOS structure table.

  @param [in]   This        EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [out]  Buffer      Pointer to the returned SMBIOS structure table.
                            Caller has to free this memory block when it
                            is no longer needed.
  @param [out]  BufferSize  Size of the returned message payload in buffer.

  @retval       EFI_SUCCESS            Gets SMBIOS structure table successfully.
  @retval       EFI_UNSUPPORTED        The function is unsupported by this
                                       driver instance.
  @retval       Other values           Fail to get SMBIOS structure table.
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureTable (
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This,
  OUT  UINT8                                **Buffer,
  OUT  UINT32                               *BufferSize
  )
{
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Unsupported.\n", __func__));
  // Only support PLDM SMBIOS Transfer push mode.
  return EFI_UNSUPPORTED;
}

/**
  This function sets SMBIOS structure table.

  @param [in]   This        EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.

  @retval      EFI_SUCCESS            Successful
  @retval      EFI_NOT_FOUND          No SMBIOS record found on system.
  @retval      EFI_UNSUPPORTED        The function is unsupported by this
                                      driver instance.
  @retval      Other values           Fail to set SMBIOS structure table.
**/
EFI_STATUS
EFIAPI
SetSmbiosStructureTable (
  IN  EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This
  )
{
  EFI_STATUS                               Status;
  SMBIOS_TABLE_3_0_ENTRY_POINT             *SmbiosEntry;
  EFI_SMBIOS_HANDLE                        SmbiosHandle;
  EFI_SMBIOS_PROTOCOL                      *Smbios;
  UINT32                                   PaddingSize;
  UINT32                                   ResponseSize;
  UINT32                                   RequestSize;
  UINT8                                    *RequestBuffer;
  UINT8                                    *DataPointer;
  UINT32                                   Crc32;
  UINT16                                   TableLength;
  EFI_SMBIOS_TABLE_HEADER                  *Record;
  PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST  *PldmSetSmbiosStructureTable;

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Set SMBIOS structure table.\n", __func__));

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: No Efi SMBIOS Protocol installed.\n"));
    return EFI_UNSUPPORTED;
  }

  if (Smbios->MajorVersion < 3) {
    DEBUG ((DEBUG_ERROR, "%a: We don't support SMBIOS spec version earlier than v3.0.\n"));
    return EFI_UNSUPPORTED;
  }

  Status = EfiGetSystemConfigurationTable (
             &gEfiSmbios3TableGuid,
             (VOID **)&SmbiosEntry
             );
  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get system configuration table.\n"));
    return Status;
  }

  //
  // Print out SMBIOS table information.
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "PldmSetSmbiosStructureTable SmbiosTable:\n"));
  DEBUG ((
    DEBUG_MANAGEABILITY_INFO,
    "AnchorString                ------ '%c%c%c%c%c'\n",
    SmbiosEntry->AnchorString[0],
    SmbiosEntry->AnchorString[1],
    SmbiosEntry->AnchorString[2],
    SmbiosEntry->AnchorString[3],
    SmbiosEntry->AnchorString[4]
    ));

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "EntryPointStructureChecksum ------ 0x%02x\n", SmbiosEntry->EntryPointStructureChecksum));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "EntryPointLength            ------ 0x%02x\n", SmbiosEntry->EntryPointLength));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "MajorVersion                ------ 0x%02x\n", SmbiosEntry->MajorVersion));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "MinorVersion                ------ 0x%02x\n", SmbiosEntry->MinorVersion));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "DocRev                      ------ 0x%02x\n", SmbiosEntry->DocRev));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "MaxStructureSize            ------ 0x%08x\n", SmbiosEntry->TableMaximumSize));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "EntryPointRevision               - 0x%02x\n", SmbiosEntry->EntryPointRevision));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "TableMaximumSize                 - 0x%08x\n", SmbiosEntry->TableMaximumSize));
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "TableAddress                     - 0x%016lx\n", SmbiosEntry->TableAddress));

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  do {
    Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR (Status)) {
      break;
    }

    DEBUG ((DEBUG_MANAGEABILITY_INFO, "  SMBIOS type %d to BMC\n", Record->Type));
  } while (Status == EFI_SUCCESS);

  TableLength = (UINT16)GetSmbiosTableLength ((VOID *)(UINTN)SmbiosEntry->TableAddress, SmbiosEntry->TableMaximumSize);

  // Padding requirement (0 ~ 3 bytes)
  PaddingSize = (4 - (TableLength % 4)) % 4;

  // Total request buffer size = PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST + SMBIOS tables + padding + checksum
  RequestSize   = (UINT32)(sizeof (PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST) + TableLength + PaddingSize + sizeof (Crc32));
  RequestBuffer = (UINT8 *)AllocatePool (RequestSize);
  if (RequestBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No memory resource for sending SetSmbiosStructureTable.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Fill in smbios tables
  CopyMem (
    (VOID *)((UINT8 *)RequestBuffer + sizeof (PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST)),
    (VOID *)(UINTN)SmbiosEntry->TableAddress,
    TableLength
    );

  // Fill in padding
  DataPointer = RequestBuffer + sizeof (PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST) + TableLength;
  ZeroMem ((VOID *)DataPointer, PaddingSize);

  // Fill in checksum
  gBS->CalculateCrc32 (
         (VOID *)(RequestBuffer + sizeof (PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST)),
         TableLength + PaddingSize,
         &Crc32
         );
  DataPointer += PaddingSize;
  CopyMem ((VOID *)DataPointer, (VOID *)&Crc32, 4);

  PldmSetSmbiosStructureTable                     = (PLDM_SET_SMBIOS_STRUCTURE_TABLE_REQUEST *)RequestBuffer;
  PldmSetSmbiosStructureTable->DataTransferHandle = SetSmbiosStructureTableHandle;
  PldmSetSmbiosStructureTable->TransferFlag       = PLDM_TRANSFER_FLAG_START_AND_END;
  ResponseSize                                    = sizeof (SetSmbiosStructureTableHandle);

  Status = PldmSubmitCommand (
             PLDM_TYPE_SMBIOS,
             PLDM_SET_SMBIOS_STRUCTURE_TABLE_COMMAND_CODE,
             RequestBuffer,
             RequestSize,
             (UINT8 *)&SetSmbiosStructureTableHandle,
             &ResponseSize
             );
  if (RequestBuffer != NULL) {
    FreePool (RequestBuffer);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Set SMBIOS structure table.\n", __func__));
  }

  if ((ResponseSize != 0) && (ResponseSize <= sizeof (SetSmbiosStructureTableHandle))) {
    HelperManageabilityDebugPrint (
      (VOID *)&SetSmbiosStructureTableHandle,
      ResponseSize,
      "Set SMBIOS structure table response got from BMC.\n"
      );
  }

  return Status;
}

/**
  This function gets particular type of SMBIOS structure.

  @param [in]   This                 EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   TypeId               The type of SMBIOS structure.
  @param [in]   StructureInstanceId  The instance ID of particular type of SMBIOS structure.
  @param [out]  Buffer               Pointer to the returned SMBIOS structure.
                                     Caller has to free this memory block when it
                                     is no longer needed.
  @param [out]  BufferSize           Size of the returned message payload in buffer.

  @retval      EFI_SUCCESS           Gets particular type of SMBIOS structure successfully.
  @retval      EFI_UNSUPPORTED       The function is unsupported by this
                                     driver instance.
  @retval      Other values          Fail to set SMBIOS structure table.
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureByType (
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This,
  IN   UINT8                                TypeId,
  IN   UINT16                               StructureInstanceId,
  OUT  UINT8                                **Buffer,
  OUT  UINT32                               *BufferSize
  )
{
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Unsupported.\n", __func__));
  // Only support PLDM SMBIOS Transfer push mode.
  return EFI_UNSUPPORTED;
}

/**
  This function gets particular handle of SMBIOS structure.

  @param [in]   This                 EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL instance.
  @param [in]   Handle               The handle of SMBIOS structure.
  @param [out]  Buffer               Pointer to the returned SMBIOS structure.
                                     Caller has to free this memory block when it
                                     is no longer needed.
  @param [out]  BufferSize           Size of the returned message payload in buffer.

  @retval      EFI_SUCCESS           Gets particular handle of SMBIOS structure successfully.
  @retval      EFI_UNSUPPORTED       The function is unsupported by this
                                     driver instance.
  @retval      Other values          Fail to set SMBIOS structure table.
**/
EFI_STATUS
EFIAPI
GetSmbiosStructureByHandle (
  IN   EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  *This,
  IN   UINT16                               Handle,
  OUT  UINT8                                **Buffer,
  OUT  UINT32                               *BufferSize
  )
{
  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Unsupported.\n", __func__));
  // Only support PLDM SMBIOS Transfer push mode.
  return EFI_UNSUPPORTED;
}

EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_V1_0  mPldmSmbiosTransferProtocolV10 = {
  SetPldmSmbiosTransferTerminusId,
  GetSmbiosStructureTableMetaData,
  SetSmbiosStructureTableMetaData,
  GetSmbiosStructureTable,
  SetSmbiosStructureTable,
  GetSmbiosStructureByType,
  GetSmbiosStructureByHandle
};

EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL  mPldmSmbiosTransferProtocol;

/**
  The entry point of the PLDM SMBIOS Transfer DXE driver.

  @param[in] ImageHandle - Handle of this driver image
  @param[in] SystemTable - Table containing standard EFI services

  @retval EFI_SUCCESS    - IPMI Protocol is installed successfully.
  @retval Otherwise      - Other errors.
**/
EFI_STATUS
EFIAPI
DxePldmSmbiosTransferEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE  Handle;
  EFI_STATUS  Status;

  DEBUG ((DEBUG_MANAGEABILITY_INFO, "%a: Entry.\n", __func__));

  SetSmbiosStructureTableHandle = 0;

  Handle                                           = NULL;
  mPldmSmbiosTransferProtocol.ProtocolVersion      = EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL_VERSION;
  mPldmSmbiosTransferProtocol.Functions.Version1_0 = &mPldmSmbiosTransferProtocolV10;
  Status                                           = gBS->InstallProtocolInterface (
                                                            &Handle,
                                                            &gEdkiiPldmSmbiosTransferProtocolGuid,
                                                            EFI_NATIVE_INTERFACE,
                                                            (VOID **)&mPldmSmbiosTransferProtocol
                                                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to install EDKII_PLDM_SMBIOS_TRANSFER_PROTOCOL.\n", __func__));
  }

  return Status;
}

/**
  This is the unload handler of PLDM SMBIOS Transfer DXE driver.

  @param[in] ImageHandle           The driver's image handle.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
PldmSmbiosTransferUnloadImage (
  IN EFI_HANDLE  ImageHandle
  )
{
  return EFI_SUCCESS;
}
