/** @file -- MediaSanitizeUnitTest.c
  Placeholder/framework for developing a Media Sanitize unit test package.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Protocol/BlockIo.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/MediaSanitize.h>

#include "../NvmExpress.h"
#include "../NvmExpressBlockIo.h"
#include "../NvmExpressMediaSanitize.h"
#include "../NvmExpressHci.h"

/**
  Helper function for Nvme pass thru.

  @param[in]     This        Private Data.
  @param[in]     NamespaceId Name Space Id.
  @param[in,out] Packet      Transfer Buffer.
  @param[in]     Event       Event handle.

 **/
EFI_STATUS
EFIAPI
NvmeDeviceUnitTestPassthru (
  IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL        *This,
  IN     UINT32                                    NamespaceId,
  IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  *Packet,
  IN     EFI_EVENT                                 Event OPTIONAL
  )
{
  //
  // Parse command packet for unit testing
  //
  EFI_NVM_EXPRESS_COMMAND     *Command;
  EFI_NVM_EXPRESS_COMPLETION  *Completion;
  NVME_CQ                     *Cqe;
  NVME_ADMIN_FORMAT_NVM       FormatNvmCdw10;
  NVME_ADMIN_SANITIZE         SanitizeCdw1011;

  ASSERT (This);
  ASSERT (Packet);

  Command    = Packet->NvmeCmd;
  Completion = Packet->NvmeCompletion;
  Cqe        = (NVME_CQ *)Completion;

  ZeroMem (&FormatNvmCdw10, sizeof (NVME_ADMIN_FORMAT_NVM));
  ZeroMem (&SanitizeCdw1011, sizeof (NVME_ADMIN_SANITIZE));

  switch (Command->Cdw0.Opcode) {
    case NVME_ADMIN_FORMAT_NVM_CMD:
      UT_LOG_VERBOSE ("%a: Opcode = NVME_ADMIN_FORMAT_NVM_CMD\n", __func__);

      CopyMem (&FormatNvmCdw10, &Command->Cdw10, sizeof (NVME_ADMIN_FORMAT_NVM));

      //
      // FormatNVM Check 1: Validate SES parameter
      //
      if (FormatNvmCdw10.Ses > 0x2) {
        Cqe->Sct = NVME_CQE_SCT_GENERIC_CMD_STATUS;
        Cqe->Sc  = NVME_CQE_SC_INVALID_FIELD_IN_CMD;

        return EFI_INVALID_PARAMETER;
      }

      //
      // FormatNVM Check 2: Validate LbaIndex parameter
      //
      if (FormatNvmCdw10.Lbaf > 0x1) {
        Cqe->Sct = NVME_CQE_SCT_GENERIC_CMD_STATUS;
        Cqe->Sc  = NVME_CQE_SC_INVALID_FIELD_IN_CMD;

        return EFI_INVALID_PARAMETER;
      }

      break;
    case NVME_ADMIN_SANITIZE_CMD:
      UT_LOG_VERBOSE ("%a: Opcode = NVME_ADMIN_SANITIZE_CMD\n", __func__);

      CopyMem (&SanitizeCdw1011, &Command->Cdw10, sizeof (NVME_ADMIN_SANITIZE));

      //
      // Sanitize Check 1: Validate Sanitize Action parameter
      //
      if (SanitizeCdw1011.Sanact > 0x4) {
        Cqe->Sct = NVME_CQE_SCT_GENERIC_CMD_STATUS;
        Cqe->Sc  = NVME_CQE_SC_INVALID_FIELD_IN_CMD;

        return EFI_INVALID_PARAMETER;
      }

      //
      // Sanitize Check 2: Validate overwrite action with non-NULL overwrite pattern
      //
      if (((SanitizeCdw1011.Sanact == SANITIZE_ACTION_OVERWRITE) && (SanitizeCdw1011.Ovrpat != 0xDEADBEEF)) ||
          ((SanitizeCdw1011.Sanact != SANITIZE_ACTION_OVERWRITE) && (SanitizeCdw1011.Ovrpat != 0)))
      {
        Cqe->Sct = NVME_CQE_SCT_GENERIC_CMD_STATUS;
        Cqe->Sc  = NVME_CQE_SC_INVALID_FIELD_IN_CMD;

        return EFI_INVALID_PARAMETER;
      }

      break;
    default:
      UT_LOG_VERBOSE ("%a: Invalid Opcode = 0x%x!!!\n", __func__, Command->Cdw0.Opcode);
      break;
  }

  //
  // Populate CQE (completion queue entry based on opcode and parameters
  //

  return EFI_SUCCESS;
}

/**
  Helper function to simulate read.

  @param[in]   Private     Private Data.
  @param[in]   NamespaceId Name Space Id.
  @param[in]   Buffer      Transfer Buffer.

 **/
EFI_STATUS
NvmeIdentifyNamespace (
  IN NVME_CONTROLLER_PRIVATE_DATA  *Private,
  IN UINT32                        NamespaceId,
  IN VOID                          *Buffer
  )
{
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET  CommandPacket;
  EFI_NVM_EXPRESS_COMMAND                   Command;
  EFI_NVM_EXPRESS_COMPLETION                Completion;

  ZeroMem (&CommandPacket, sizeof (EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET));
  ZeroMem (&Command, sizeof (EFI_NVM_EXPRESS_COMMAND));
  ZeroMem (&Completion, sizeof (EFI_NVM_EXPRESS_COMPLETION));

  CommandPacket.NvmeCmd        = &Command;
  CommandPacket.NvmeCompletion = &Completion;
  Command.Cdw0.Opcode          = NVME_ADMIN_IDENTIFY_CMD;
  Command.Nsid                 = NamespaceId;
  CommandPacket.TransferBuffer = Buffer;
  CommandPacket.TransferLength = sizeof (NVME_ADMIN_NAMESPACE_DATA);
  CommandPacket.CommandTimeout = NVME_GENERIC_TIMEOUT;
  CommandPacket.QueueType      = NVME_ADMIN_QUEUE;

  //
  // Set bit 0 (Cns bit) to 1 to identify a namespace
  //
  CommandPacket.NvmeCmd->Cdw10 = 0;
  CommandPacket.NvmeCmd->Flags = CDW10_VALID;

  return EFI_SUCCESS;
}

/**
  Helper function to simulate read.

  @param[in]   Device  Private Data.
  @param[out]  Buffer  Buffer to read into.
  @param[in]   Lba     Logical Block Addess to read from.
  @param[in]   Blocks  Number of blocks.

 **/
EFI_STATUS
NvmeUnitTestRead (
  IN     NVME_DEVICE_PRIVATE_DATA  *Device,
  OUT VOID                         *Buffer,
  IN     UINT64                    Lba,
  IN     UINTN                     Blocks
  )
{
  UT_ASSERT_NOT_NULL (Device);
  Buffer = NULL;
  Lba    = 0;
  Blocks = 0;

  return EFI_SUCCESS;
}

/**
  Helper function to simulate write.

  @param[in]     Device  Private Data.
  @param[in]     Buffer  Buffer to write.
  @param[in]     Lba     Logical Block Addess to write.
  @param[in]     Blocks  Number of blocks.

 **/
EFI_STATUS
NvmeUnitTestWrite (
  IN NVME_DEVICE_PRIVATE_DATA  *Device,
  IN VOID                      *Buffer,
  IN UINT64                    Lba,
  IN UINTN                     Blocks
  )
{
  UT_ASSERT_NOT_NULL (Device);
  Buffer = NULL;
  Lba    = 0;
  Blocks = 0;

  return EFI_SUCCESS;
}

/**
  Simulated BlockIo read block function.

  @param[in]     This        BlockIo Protocol.
  @param[in]     MediaId     Id of the media.
  @param[in]     Lba         Logical Block Address.
  @param[in]     BufferSize  Size of Buffer.
  @param[out]    Buffer      Actual buffer to use to read.

 **/
EFI_STATUS
EFIAPI
NvmeBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_STATUS                Status;
  EFI_BLOCK_IO_MEDIA        *Media;
  UINTN                     BlockSize;
  UINTN                     NumberOfBlocks;
  UINTN                     IoAlign;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Media = This->Media;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if ((IoAlign > 0) && (((UINTN)Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO (This);
  Status = NvmeUnitTestRead (Device, Buffer, Lba, NumberOfBlocks);

  return Status;
}

/**
  Simulated BlockIo write block function.

  @param[in]     This        BlockIo Protocol.
  @param[in]     MediaId     Id of the media.
  @param[in]     Lba         Logical Block Address.
  @param[in]     BufferSize  Size of Buffer.
  @param[in]     Buffer      Actual buffer to use to write.

 **/
EFI_STATUS
EFIAPI
NvmeBlockIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  IN  VOID                   *Buffer
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_STATUS                Status;
  EFI_BLOCK_IO_MEDIA        *Media;
  UINTN                     BlockSize;
  UINTN                     NumberOfBlocks;
  UINTN                     IoAlign;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Media = This->Media;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if ((IoAlign > 0) && (((UINTN)Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO (This);
  Status = NvmeUnitTestWrite (Device, Buffer, Lba, NumberOfBlocks);

  return Status;
}

/**
  Simulated BlockIo read block ex function.

  @param[in]     This        BlockIo2 Protocol.
  @param[in]     MediaId     Id of the media.
  @param[in]     Lba         Logical Block Address.
  @param[in,out] Token       Block Io2 token.

  @param[in]     BufferSize  Size of Buffer.
  @param[out]    Buffer      Actual buffer to use to read.

 **/
EFI_STATUS
EFIAPI
NvmeBlockIoReadBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  OUT VOID                       *Buffer
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_BLOCK_IO_MEDIA        *Media;
  UINTN                     BlockSize;
  UINTN                     NumberOfBlocks;
  UINTN                     IoAlign;
  EFI_STATUS                Status;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Media = This->Media;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if ((IoAlign > 0) && (((UINTN)Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO2 (This);
  Status = NvmeUnitTestRead (Device, Buffer, Lba, NumberOfBlocks);

  return Status;
}

/**
  Simulated BlockIo write block ex function.

  @param[in]     This        BlockIo2 Protocol.
  @param[in]     MediaId     Id of the media.
  @param[in]     Lba         Logical Block Address.
  @param[in,out] Token       Block Io2 token.
  @param[in]     BufferSize  Size of Buffer.

  @param[in]     Buffer      Actual buffer to use to write.

 **/
EFI_STATUS
EFIAPI
NvmeBlockIoWriteBlocksEx (
  IN     EFI_BLOCK_IO2_PROTOCOL  *This,
  IN     UINT32                  MediaId,
  IN     EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN     *Token,
  IN     UINTN                   BufferSize,
  IN     VOID                    *Buffer
  )
{
  NVME_DEVICE_PRIVATE_DATA  *Device;
  EFI_BLOCK_IO_MEDIA        *Media;
  UINTN                     BlockSize;
  UINTN                     NumberOfBlocks;
  UINTN                     IoAlign;
  EFI_STATUS                Status;

  //
  // Check parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Media = This->Media;

  if (MediaId != Media->MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BlockSize = Media->BlockSize;
  if ((BufferSize % BlockSize) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }

  NumberOfBlocks = BufferSize / BlockSize;
  if ((Lba + NumberOfBlocks - 1) > Media->LastBlock) {
    return EFI_INVALID_PARAMETER;
  }

  IoAlign = Media->IoAlign;
  if ((IoAlign > 0) && (((UINTN)Buffer & (IoAlign - 1)) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Device = NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO2 (This);
  Status = NvmeUnitTestWrite (Device, Buffer, Lba, NumberOfBlocks);

  return Status;
}

/**
  MediaSanitizePurgeUnitTest to initialize a Private Namespace instance.

  @param[in]  ppDevice  Nvme Private Data structure to destory and free.
 **/
UNIT_TEST_STATUS
EFIAPI
NvmeDestroyDeviceInstance (
  NVME_DEVICE_PRIVATE_DATA  **ppDevice
  )
{
  //
  // Free in following order to to avoid dangling pointers:
  //
  // 1 - NVME_ADMIN_CONTROLLER_DATA
  // 2 - NVME_CONTROLLER_PRIVATE_DATA
  // 3 - NVME_DEVICE_PRIVATE_DATA
  //
  FreePool ((*ppDevice)->Controller->ControllerData);
  (*ppDevice)->Controller->ControllerData = NULL;

  FreePool ((*ppDevice)->Controller);
  (*ppDevice)->Controller = NULL;

  FreePool ((*ppDevice));
  *ppDevice = NULL;

  return UNIT_TEST_PASSED;
}

/**
  MediaSanitizePurgeUnitTest to initialize a Private Namespace instance.

  @param[in]  ppDevice  Nvme Private Data structure to initialize.
 **/
UNIT_TEST_STATUS
EFIAPI
NvmeCreateDeviceInstance (
  NVME_DEVICE_PRIVATE_DATA  **ppDevice
  )
{
  NVME_ADMIN_NAMESPACE_DATA     *NamespaceData;
  NVME_CONTROLLER_PRIVATE_DATA  *Private;
  NVME_DEVICE_PRIVATE_DATA      *Device;

  Private = AllocateZeroPool (sizeof (NVME_CONTROLLER_PRIVATE_DATA));

  Private->Signature     = NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE;
  Private->Cid[0]        = 0;
  Private->Cid[1]        = 0;
  Private->Cid[2]        = 0;
  Private->Pt[0]         = 0;
  Private->Pt[1]         = 0;
  Private->Pt[2]         = 0;
  Private->SqTdbl[0].Sqt = 0;
  Private->SqTdbl[1].Sqt = 0;
  Private->SqTdbl[2].Sqt = 0;
  Private->CqHdbl[0].Cqh = 0;
  Private->CqHdbl[1].Cqh = 0;
  Private->CqHdbl[2].Cqh = 0;
  Private->AsyncSqHead   = 0;

  Private->ControllerData = (NVME_ADMIN_CONTROLLER_DATA *)AllocateZeroPool (sizeof (NVME_ADMIN_CONTROLLER_DATA));

  UT_LOG_VERBOSE ("%a: Allocated and Initialized NVME_CONTROLLER_PRIVATE_DATA\n", __func__);
  UT_LOG_VERBOSE ("%a: Allocated and Initialized NVME_ADMIN_CONTROLLER_DATA\n", __func__);

  Private->ControllerData->Nn          = 1; // One namespace
  Private->ControllerData->Sanicap.Bes = 1; // Block Erase Supported
  Private->ControllerData->Sanicap.Ces = 1; // Crypto Erase Supported
  Private->ControllerData->Sanicap.Ows = 1; // Overwrite Supported

  NamespaceData = AllocateZeroPool (sizeof (NVME_ADMIN_NAMESPACE_DATA));
  UT_LOG_VERBOSE ("%a: Allocated and Initialized NVME_ADMIN_NAMESPACE_DATA\n", __func__);

  Device = (NVME_DEVICE_PRIVATE_DATA *)(AllocateZeroPool (sizeof (NVME_DEVICE_PRIVATE_DATA)));

  //
  // Initialize SSD namespace instance data
  //
  Device->Signature     = NVME_DEVICE_PRIVATE_DATA_SIGNATURE;
  Device->NamespaceId   = 0;
  Device->NamespaceUuid = 1;

  Device->Controller = Private;

  //
  // Build BlockIo media structure
  //
  Device->Media.MediaId          = 0;
  Device->Media.RemovableMedia   = FALSE;
  Device->Media.MediaPresent     = TRUE;
  Device->Media.LogicalPartition = FALSE;
  Device->Media.ReadOnly         = FALSE;
  Device->Media.WriteCaching     = FALSE;
  Device->Media.BlockSize        = (UINT32)(1 << 9); // 512 byte sector size

  Device->Media.LastBlock                     = 0x4000; // NamespaceData=>Nsze
  Device->Media.LogicalBlocksPerPhysicalBlock = 1;
  Device->Media.LowestAlignedLba              = 1;

  Device->BlockIo.Revision    = EFI_BLOCK_IO_PROTOCOL_REVISION2;
  Device->BlockIo.Media       = &Device->Media;
  Device->BlockIo.ReadBlocks  = NvmeBlockIoReadBlocks;
  Device->BlockIo.WriteBlocks = NvmeBlockIoWriteBlocks;

  Device->BlockIo2.Media         = &Device->Media;
  Device->BlockIo2.ReadBlocksEx  = NvmeBlockIoReadBlocksEx;
  Device->BlockIo2.WriteBlocksEx = NvmeBlockIoWriteBlocksEx;

  Device->MediaSanitize.Revision    = MEDIA_SANITIZE_PROTOCOL_REVISION;
  Device->MediaSanitize.Media       = &Device->Media;
  Device->MediaSanitize.MediaClear  = NvmExpressMediaClear;
  Device->MediaSanitize.MediaPurge  = NvmExpressMediaPurge;
  Device->MediaSanitize.MediaFormat = NvmExpressMediaFormat;

  Device->Controller->Passthru.Mode             = 0;
  Device->Controller->Passthru.PassThru         = NvmeDeviceUnitTestPassthru;
  Device->Controller->Passthru.BuildDevicePath  = NULL;
  Device->Controller->Passthru.GetNamespace     = NULL;
  Device->Controller->Passthru.GetNextNamespace = NULL;

  CopyMem (&Device->NamespaceData, NamespaceData, sizeof (NVME_ADMIN_NAMESPACE_DATA));
  *ppDevice = Device;

  UT_LOG_VERBOSE ("%a: Allocated and Initialized NVME_DEVICE_PRIVATE_DATA\n", __func__);

  return UNIT_TEST_PASSED;
}

/**
  MediaSanitizePurgeUnitTest to Test calls to NvmExpressMediaPurge.

  @param[in]  Context  Unit test case context
 **/
UNIT_TEST_STATUS
EFIAPI
MediaSanitizePurgeUnitTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32                    PurgeAction;
  UINT32                    OverwritePattern;
  UNIT_TEST_STATUS          UnitTestStatus;
  NVME_DEVICE_PRIVATE_DATA  *NvmeDevice;
  EFI_STATUS                Status;

  UnitTestStatus = UNIT_TEST_PASSED;
  NvmeDevice     = NULL;
  Status         = EFI_SUCCESS;

  UnitTestStatus = NvmeCreateDeviceInstance (&NvmeDevice);

  UT_ASSERT_STATUS_EQUAL (UnitTestStatus, UNIT_TEST_PASSED);
  UT_ASSERT_NOT_NULL (NvmeDevice);

  UT_LOG_VERBOSE ("%a: Create Device Instance Status = 0x%x\n", __func__, UnitTestStatus);
  UT_LOG_VERBOSE ("%a: Device = 0x%x\n", __func__, NvmeDevice);
  UT_LOG_VERBOSE ("%a: Device->BlockIo = 0x%x\n", __func__, NvmeDevice->BlockIo);
  UT_LOG_VERBOSE ("%a: Device->Signature = 0x%x\n", __func__, NvmeDevice->Signature);

  //
  // Case 1: Block Erase
  //
  PurgeAction      = SANITIZE_ACTION_BLOCK_ERASE;
  OverwritePattern = 0;

  Status = NvmExpressMediaPurge (
             &NvmeDevice->MediaSanitize,
             NvmeDevice->Media.MediaId,
             PurgeAction,
             OverwritePattern
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  UnitTestStatus = NvmeDestroyDeviceInstance (&NvmeDevice);

  return UNIT_TEST_PASSED;
}

/**
  NvmeSanitizeUnitTest to Test calls to NvmExpressSanitize.

  @param[in]  Context  Unit test case context
 **/
UNIT_TEST_STATUS
EFIAPI
NvmeSanitizeUnitTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32                    NamespaceId;
  UINT32                    SanitizeAction;
  UINT32                    NoDeallocateAfterSanitize;
  UINT32                    OverwritePattern;
  UNIT_TEST_STATUS          UnitTestStatus;
  NVME_DEVICE_PRIVATE_DATA  *NvmeDevice;
  EFI_STATUS                Status;

  NamespaceId               = 0;
  UnitTestStatus            = UNIT_TEST_PASSED;
  NvmeDevice                = NULL;
  Status                    = EFI_SUCCESS;
  SanitizeAction            = SANITIZE_ACTION_BLOCK_ERASE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0;

  UnitTestStatus = NvmeCreateDeviceInstance (&NvmeDevice);

  UT_ASSERT_STATUS_EQUAL (UnitTestStatus, UNIT_TEST_PASSED);
  UT_ASSERT_NOT_NULL (NvmeDevice);

  UT_LOG_VERBOSE ("%a: Create Device Instance Status = 0x%x\n", __func__, UnitTestStatus);
  UT_LOG_VERBOSE ("%a: Device = 0x%x\n", __func__, NvmeDevice);
  UT_LOG_VERBOSE ("%a: Device->BlockIo = 0x%x\n", __func__, NvmeDevice->BlockIo);
  UT_LOG_VERBOSE ("%a: Device->Signature = 0x%x\n", __func__, NvmeDevice->Signature);

  //
  // Case 1: Block Erase
  //
  SanitizeAction            = SANITIZE_ACTION_BLOCK_ERASE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0;

  Status = NvmExpressSanitize (
             &NvmeDevice->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocateAfterSanitize,
             OverwritePattern
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Case 2: Crypto Erase
  //
  SanitizeAction            = SANITIZE_ACTION_CRYPTO_ERASE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0;

  Status = NvmExpressSanitize (
             &NvmeDevice->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocateAfterSanitize,
             OverwritePattern
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Case 3: Overwrite
  //
  SanitizeAction            = SANITIZE_ACTION_OVERWRITE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0xDEADBEEF;

  Status = NvmExpressSanitize (
             &NvmeDevice->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocateAfterSanitize,
             OverwritePattern
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Case 4: Block Erase (invalid overwrite pattern)
  //
  SanitizeAction            = SANITIZE_ACTION_BLOCK_ERASE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0xDEADBEEF;

  Status = NvmExpressSanitize (
             &NvmeDevice->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocateAfterSanitize,
             OverwritePattern
             );

  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Case 5: Overwrite (invalid overwrite pattern)
  //
  SanitizeAction            = SANITIZE_ACTION_OVERWRITE;
  NoDeallocateAfterSanitize = 0;
  OverwritePattern          = 0;

  Status = NvmExpressSanitize (
             &NvmeDevice->BlockIo,
             NamespaceId,
             SanitizeAction,
             NoDeallocateAfterSanitize,
             OverwritePattern
             );

  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  UnitTestStatus = NvmeDestroyDeviceInstance (&NvmeDevice);

  return UNIT_TEST_PASSED;
}

/**
  NvmeFormatNvmUnitTest to Test calls to NvmExpressFormatNvm.

  @param[in]  Context  Unit test case context
 **/
UNIT_TEST_STATUS
EFIAPI
NvmeFormatNvmUnitTest (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32                    NamespaceId;
  UINT32                    Ses;
  UINT32                    Flbas;
  NVME_DEVICE_PRIVATE_DATA  *NvmeDevice;
  UNIT_TEST_STATUS          UnitTestStatus;
  EFI_STATUS                Status;

  NamespaceId    = 0;
  NvmeDevice     = NULL;
  UnitTestStatus = UNIT_TEST_PASSED;
  Status         = EFI_SUCCESS;

  UnitTestStatus = NvmeCreateDeviceInstance (&NvmeDevice);

  UT_ASSERT_STATUS_EQUAL (UnitTestStatus, UNIT_TEST_PASSED);
  UT_ASSERT_NOT_NULL (NvmeDevice);

  UT_LOG_VERBOSE ("%a: Create Device Instance Status = 0x%x\n", __func__, UnitTestStatus);
  UT_LOG_VERBOSE ("%a: Device = 0x%x\n", __func__, NvmeDevice);
  UT_LOG_VERBOSE ("%a: Device->BlockIo = 0x%x\n", __func__, NvmeDevice->BlockIo);
  UT_LOG_VERBOSE ("%a: Device->Signature = 0x%x\n", __func__, NvmeDevice->Signature);

  //
  // Case 1: User Data Erase (Flbas = 0)
  //
  Ses    = SES_USER_DATA_ERASE;
  Flbas  = 0;
  Status = NvmExpressFormatNvm (
             &NvmeDevice->BlockIo,
             NamespaceId,
             Ses,
             Flbas
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Case 2: Crypto Erase (Flbas = 0)
  //
  Ses    = SES_CRYPTO_ERASE;
  Flbas  = 0;
  Status = NvmExpressFormatNvm (
             &NvmeDevice->BlockIo,
             NamespaceId,
             Ses,
             Flbas
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // Case 3: User Data Erase (Invalid Flbas = 3)
  //
  Ses    = SES_USER_DATA_ERASE;
  Flbas  = 3;
  Status = NvmExpressFormatNvm (
             &NvmeDevice->BlockIo,
             NamespaceId,
             Ses,
             Flbas
             );

  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  //
  // Case 4: Invalid SES (Flba = 0)
  //
  Ses    = 0xFF;
  Flbas  = 0;
  Status = NvmExpressFormatNvm (
             &NvmeDevice->BlockIo,
             NamespaceId,
             Ses,
             Flbas
             );

  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);

  UnitTestStatus = NvmeDestroyDeviceInstance (&NvmeDevice);

  return UNIT_TEST_PASSED;
}

/**
  Baseline Unit Test.

  @param[in]  Context  Unit test case context
 **/
UNIT_TEST_STATUS
EFIAPI
UnitTestBaseline (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  UINT32  A;
  UINT32  B;
  UINT32  C;

  A = 1;
  B = 1;
  C = A + B;

  UT_ASSERT_EQUAL (C, 2);
  UT_ASSERT_NOT_EQUAL (0, 1);

  return UNIT_TEST_PASSED;
}

/**
  Test Case that locks a variable using the Variable Policy Protocol with a
  policy other than LOCK_NOW then attempts to lock the same variable using the
  Variable Lock Protocol.  The call to Variable Policy is expected to succeed
  and the call to Variable Lock is expected to fail.

  @retval EFI_SUCCES  Success
  @retval Other       Error
  **/
EFI_STATUS
EFIAPI
MediaSanitizeUnitTestEntry (
  VOID
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      NvmeFormatNvmTestSuite;
  UNIT_TEST_SUITE_HANDLE      NvmeSanitizeTestSuite;
  UNIT_TEST_SUITE_HANDLE      MediaSanitizeProtocolTestSuite;

  Framework = NULL;

  #define UNIT_TEST_NAME     "Media Sanitize Protocol Unit Test"
  #define UNIT_TEST_VERSION  "1.0"

  DEBUG ((DEBUG_INFO, "%a v%a\n", UNIT_TEST_NAME, UNIT_TEST_VERSION));

  //
  // Start setting up the test framework for running the tests.
  //
  Status = InitUnitTestFramework (
             &Framework,
             UNIT_TEST_NAME,
             gEfiCallerBaseName,
             UNIT_TEST_VERSION
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in InitUnitTestFramework. Status = %r\n", Status));
    goto EXIT;
  }

  //
  // Populate the NVM Express Format NVM Unit Test Suite.
  //
  Status = CreateUnitTestSuite (
             &NvmeFormatNvmTestSuite,
             Framework,
             "NVM Express Format NVM Test Suite",
             "Nvm.Express.Format.Nvm",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for NvmeFormatNvmTestSuite. Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Add baseline sanity test case
  //
  AddTestCase (
    NvmeFormatNvmTestSuite,          // Test Suite Handle
    "Baseline Format NVM Unit Test", // Test Description
    "FormatNVM",                     // Test Class
    UnitTestBaseline,                // UNIT_TEST_FUNCTION()
    NULL,                            // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                            // (Optional) UNIT_TEST_CLEANUP()
    NULL                             // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Add test case for NvmExpressFormatNvm()
  //
  AddTestCase (
    NvmeFormatNvmTestSuite,               // Test Suite Handle
    "Admin Format NVM Command Unit Test", // Test Description
    "FormatNVM",                          // Test Class
    NvmeFormatNvmUnitTest,                // UNIT_TEST_FUNCTION()
    NULL,                                 // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                                 // (Optional) UNIT_TEST_CLEANUP()
    NULL                                  // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Populate the NVM Express Sanitize Unit Test Suite.
  //
  Status = CreateUnitTestSuite (
             &NvmeSanitizeTestSuite,
             Framework,
             "NVM Express Sanitize Test Suite",
             "Nvm.Express.Sanitize",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for NvmeSanitizTestSuite. Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Add baseline sanity test
  //
  AddTestCase (
    NvmeSanitizeTestSuite,         // Test Suite Handle
    "Baseline Sanitize Unit Test", // Test Description
    "Sanitize",                    // Test Class
    UnitTestBaseline,              // UNIT_TEST_FUNCTION()
    NULL,                          // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                          // (Optional) UNIT_TEST_CLEANUP()
    NULL                           // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Add test case for NvmExressSanitize()
  //
  AddTestCase (
    NvmeSanitizeTestSuite,              // Test Suite Handle
    "Admin Sanitize Command Unit Test", // Test Description
    "Sanitize",                         // Test Class
    NvmeSanitizeUnitTest,               // UNIT_TEST_FUNCTION()
    NULL,                               // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                               // (Optional) UNIT_TEST_CLEANUP()
    NULL                                // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Populate the Media Sanitize Protocol Unit Test Suite.
  //
  Status = CreateUnitTestSuite (
             &MediaSanitizeProtocolTestSuite,
             Framework,
             "Media Sanitize Protocol Test Suite",
             "Media.Sanitize.Protocol",
             NULL,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed in CreateUnitTestSuite for MediaSanitizeProtocolTestSuite. Status = %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Add test case for Media Purge
  //
  AddTestCase (
    MediaSanitizeProtocolTestSuite,     // Test Suite Handle
    "Baseline MediaSanitize Unit Test", // Test Description
    "MediaSanitize",                    // Test Class
    UnitTestBaseline,                   // UNIT_TEST_FUNCTION()
    NULL,                               // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                               // (Optional) UNIT_TEST_CLEANUP()
    NULL                                // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Add test case for Media Purge
  //
  AddTestCase (
    MediaSanitizeProtocolTestSuite,      // Test Suite Handle
    "Protocol Media Sanitize Unit Test", // Test Description
    "MediaPurge",                        // Test Class
    MediaSanitizePurgeUnitTest,          // UNIT_TEST_FUNCTION()
    NULL,                                // (Optional) UNIT_TEST_PREREQUISITE()
    NULL,                                // (Optional) UNIT_TEST_CLEANUP()
    NULL                                 // (Optional) UNIT_TEST_CONTEXT
    );

  //
  // Execute the tests.
  //
  Status = RunAllTestSuites (Framework);

EXIT:
  if (Framework) {
    FreeUnitTestFramework (Framework);
  }

  return Status;
}

///
/// Avoid ECC error for function name that starts with lower case letter
///
#define MediaSanitizeUnitTestMain  main

/**
  Standard POSIX C entry point for host based unit test execution.

  @param[in] Argc  Number of arguments
  @param[in] Argv  Array of pointers to arguments

  @retval 0      Success
  @retval other  Error
**/
INT32
MediaSanitizeUnitTestMain (
  IN INT32  Argc,
  IN CHAR8  *Argv[]
  )
{
  return MediaSanitizeUnitTestEntry ();
}
