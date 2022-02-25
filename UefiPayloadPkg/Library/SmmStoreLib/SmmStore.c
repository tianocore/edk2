/** @file  SmmStore.c

  Copyright (c) 2022, 9elements GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/SmmStoreLib.h>
#include "SmmStore.h"

/*
 * A memory buffer to place arguments in.
 */
STATIC SMM_STORE_COM_BUF     *mArgComBuf;
STATIC EFI_PHYSICAL_ADDRESS  mArgComBufPhys;

/*
 * Metadata provided by the first stage bootloader.
 */
STATIC SMMSTORE_INFO  *mSmmStoreInfo;

STATIC EFI_EVENT  mSmmStoreLibVirtualAddrChangeEvent;

/**
  Calls into SMM to use the SMMSTOREv2 implementation for persistent storage.

  @param Cmd     The command to write into the APM port. This allows to enter the
                 Smi special command handler.
  @param SubCmd  The subcommand to execute in the Smi handler.
  @param Arg     Optional argument to pass to the Smi handler. Typically a pointer
                 in 'flat' memory mode, which points to read only memory.

  @retval EFI_NO_RESPONSE       The SmmStore is not present or didn't response.
  @retval EFI_UNSUPPORTED       The request isn't suppored.
  @retval EFI_DEVICE_ERROR      An error occured while executing the request.
  @retval EFI_SUCCESS           The operation was executed successfully.
**/
STATIC
EFI_STATUS
CallSmm (
  UINT8  Cmd,
  UINT8  SubCmd,
  UINTN  Arg
  )
{
  CONST UINTN  Rax = ((SubCmd << 8) | Cmd);
  CONST UINTN  Rbx = Arg;
  UINTN        Result;

  Result = TriggerSmi (Rax, Rbx, 5);
  if (Result == Rax) {
    return EFI_NO_RESPONSE;
  } else if (Result == SMMSTORE_RET_SUCCESS) {
    return EFI_SUCCESS;
  } else if (Result == SMMSTORE_RET_UNSUPPORTED) {
    return EFI_UNSUPPORTED;
  }

  return EFI_DEVICE_ERROR;
}

/**
  Get the SmmStore block size

  @param BlockSize    The pointer to store the block size in.

**/
EFI_STATUS
SmmStoreLibGetBlockSize (
  OUT UINTN  *BlockSize
  )
{
  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (BlockSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *BlockSize = mSmmStoreInfo->BlockSize;

  return EFI_SUCCESS;
}

/**
  Get the SmmStore number of blocks

  @param NumBlocks    The pointer to store the number of blocks in.

**/
EFI_STATUS
SmmStoreLibGetNumBlocks (
  OUT UINTN  *NumBlocks
  )
{
  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (NumBlocks == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *NumBlocks = mSmmStoreInfo->NumBlocks;

  return EFI_SUCCESS;
}

/**
  Get the SmmStore MMIO address

  @param MmioAddress    The pointer to store the address in.

**/
EFI_STATUS
SmmStoreLibGetMmioAddress (
  OUT EFI_PHYSICAL_ADDRESS  *MmioAddress
  )
{
  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (MmioAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *MmioAddress = mSmmStoreInfo->MmioAddress;

  return EFI_SUCCESS;
}

/**
  Read from SmmStore

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
SmmStoreLibRead (
  IN        EFI_LBA  Lba,
  IN        UINTN    Offset,
  IN        UINTN    *NumBytes,
  IN        UINT8    *Buffer
  )
{
  EFI_STATUS  Status;

  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (Lba >= mSmmStoreInfo->NumBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  if (((*NumBytes + Offset) > mSmmStoreInfo->BlockSize) ||
      ((*NumBytes + Offset) > mSmmStoreInfo->ComBufferSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  mArgComBuf->Read.BufSize   = *NumBytes;
  mArgComBuf->Read.BufOffset = Offset;
  mArgComBuf->Read.BlockId   = Lba;

  Status = CallSmm (mSmmStoreInfo->ApmCmd, SMMSTORE_CMD_RAW_READ, mArgComBufPhys);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Buffer, (VOID *)(UINTN)(mSmmStoreInfo->ComBuffer + Offset), *NumBytes);

  return EFI_SUCCESS;
}

/**
  Write to SmmStore

  @param[in] Lba      The starting logical block index to write to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes On input, indicates the requested write size. On
                      output, indicates the actual number of bytes written
  @param[in] Buffer   Pointer to the data to write.

**/
EFI_STATUS
SmmStoreLibWrite (
  IN        EFI_LBA  Lba,
  IN        UINTN    Offset,
  IN        UINTN    *NumBytes,
  IN        UINT8    *Buffer
  )
{
  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (Lba >= mSmmStoreInfo->NumBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  if (((*NumBytes + Offset) > mSmmStoreInfo->BlockSize) ||
      ((*NumBytes + Offset) > mSmmStoreInfo->ComBufferSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  mArgComBuf->Write.BufSize   = *NumBytes;
  mArgComBuf->Write.BufOffset = Offset;
  mArgComBuf->Write.BlockId   = Lba;

  CopyMem ((VOID *)(UINTN)(mSmmStoreInfo->ComBuffer + Offset), Buffer, *NumBytes);

  return CallSmm (mSmmStoreInfo->ApmCmd, SMMSTORE_CMD_RAW_WRITE, mArgComBufPhys);
}

/**
  Erase a SmmStore block

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SmmStoreLibEraseBlock (
  IN   EFI_LBA  Lba
  )
{
  if (mSmmStoreInfo == NULL) {
    return EFI_NO_MEDIA;
  }

  if (Lba >= mSmmStoreInfo->NumBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  mArgComBuf->Clear.BlockId = Lba;

  return CallSmm (mSmmStoreInfo->ApmCmd, SMMSTORE_CMD_RAW_CLEAR, mArgComBufPhys);
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
STATIC
VOID
EFIAPI
SmmStoreLibVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mArgComBuf);
  if (mSmmStoreInfo != NULL) {
    EfiConvertPointer (0x0, (VOID **)&mSmmStoreInfo->ComBuffer);
    EfiConvertPointer (0x0, (VOID **)&mSmmStoreInfo);
  }

  return;
}

/**
  Initializes SmmStore support

  @retval EFI_WRITE_PROTECTED   The SmmStore is not present.
  @retval EFI_OUT_OF_RESOURCES  Run out of memory.
  @retval EFI_SUCCESS           The SmmStore is supported.

**/
EFI_STATUS
SmmStoreLibInitialize (
  VOID
  )
{
  EFI_STATUS                       Status;
  VOID                             *GuidHob;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;

  //
  // Find the SmmStore information guid hob
  //
  GuidHob = GetFirstGuidHob (&gEfiSmmStoreInfoHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_WARN, "SmmStore not supported! Skipping driver init.\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Place SmmStore information hob in a runtime buffer
  //
  mSmmStoreInfo = AllocateRuntimePool (GET_GUID_HOB_DATA_SIZE (GuidHob));
  if (mSmmStoreInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (mSmmStoreInfo, GET_GUID_HOB_DATA (GuidHob), GET_GUID_HOB_DATA_SIZE (GuidHob));

  //
  // Validate input
  //
  if ((mSmmStoreInfo->MmioAddress == 0) ||
      (mSmmStoreInfo->ComBuffer == 0) ||
      (mSmmStoreInfo->BlockSize == 0) ||
      (mSmmStoreInfo->NumBlocks == 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid data in SmmStore Info hob\n", __FUNCTION__));
    FreePool (mSmmStoreInfo);
    mSmmStoreInfo = NULL;
    return EFI_WRITE_PROTECTED;
  }

  //
  // Allocate Communication Buffer for arguments to pass to SMM.
  // The argument com buffer is only read by SMM, but never written.
  // The FVB data send/retrieved will be placed in a separate bootloader
  // pre-allocated memory region, the ComBuffer.
  //
  if (mSmmStoreInfo->ComBuffer < BASE_4GB) {
    //
    // Assume that SMM handler is running in 32-bit mode when ComBuffer is
    // is placed below BASE_4GB.
    //
    mArgComBufPhys = BASE_4GB - 1;
  } else {
    mArgComBufPhys = BASE_8EB - 1;
  }

  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiRuntimeServicesData,
                  EFI_SIZE_TO_PAGES (sizeof (SMM_STORE_COM_BUF)),
                  &mArgComBufPhys
                  );

  if (EFI_ERROR (Status)) {
    FreePool (mSmmStoreInfo);
    mSmmStoreInfo = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  mArgComBuf = (VOID *)mArgComBufPhys;

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SmmStoreLibVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSmmStoreLibVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Finally mark the SMM communication buffer provided by CB or SBL as runtime memory
  //
  Status = gDS->GetMemorySpaceDescriptor (mSmmStoreInfo->ComBuffer, &GcdDescriptor);
  if (EFI_ERROR (Status) || (GcdDescriptor.GcdMemoryType != EfiGcdMemoryTypeReserved)) {
    DEBUG (
      (
       DEBUG_INFO,
       "%a: No memory space descriptor for com buffer found\n",
       __FUNCTION__
      )
      );

    //
    // Add a new entry if not covered by existing mapping
    //
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeReserved,
                    mSmmStoreInfo->ComBuffer,
                    mSmmStoreInfo->ComBufferSize,
                    EFI_MEMORY_WB | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Mark as runtime service
  //
  Status = gDS->SetMemorySpaceAttributes (
                  mSmmStoreInfo->ComBuffer,
                  mSmmStoreInfo->ComBufferSize,
                  EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Mark the memory mapped store as MMIO memory
  //
  Status = gDS->GetMemorySpaceDescriptor (mSmmStoreInfo->MmioAddress, &GcdDescriptor);
  if (EFI_ERROR (Status) || (GcdDescriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo)) {
    DEBUG (
      (
       DEBUG_INFO,
       "%a: No memory space descriptor for com buffer found\n",
       __FUNCTION__
      )
      );

    //
    // Add a new entry if not covered by existing mapping
    //
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    mSmmStoreInfo->MmioAddress,
                    mSmmStoreInfo->NumBlocks * mSmmStoreInfo->BlockSize,
                    EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                    );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Mark as runtime service
  //
  Status = gDS->SetMemorySpaceAttributes (
                  mSmmStoreInfo->MmioAddress,
                  mSmmStoreInfo->NumBlocks * mSmmStoreInfo->BlockSize,
                  EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Denitializes SmmStore support by freeing allocated memory and unregistering
  the virtual address change event.
**/
VOID
EFIAPI
SmmStoreLibDeinitialize (
  VOID
  )
{
  if (mArgComBuf != NULL) {
    gBS->FreePages (mArgComBufPhys, EFI_SIZE_TO_PAGES (sizeof (SMM_STORE_COM_BUF)));
    mArgComBuf = NULL;
  }

  if (mSmmStoreInfo != NULL) {
    FreePool (mSmmStoreInfo);
    mSmmStoreInfo = NULL;
  }

  if (mSmmStoreLibVirtualAddrChangeEvent != NULL) {
    gBS->CloseEvent (mSmmStoreLibVirtualAddrChangeEvent);
    mSmmStoreLibVirtualAddrChangeEvent = NULL;
  }
}
