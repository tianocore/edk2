/** @file  NorFlashDxe.c

  Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>

#include "NorFlash.h"

STATIC EFI_EVENT mNorFlashVirtualAddrChangeEvent;

//
// Global variable declarations
//
NOR_FLASH_INSTANCE **mNorFlashInstances;
UINT32               mNorFlashDeviceCount;
UINTN                mFlashNvStorageVariableBase;
EFI_EVENT            mFvbVirtualAddrChangeEvent;

NOR_FLASH_INSTANCE  mNorFlashInstanceTemplate = {
  NOR_FLASH_SIGNATURE, // Signature
  NULL, // Handle ... NEED TO BE FILLED

  0, // DeviceBaseAddress ... NEED TO BE FILLED
  0, // RegionBaseAddress ... NEED TO BE FILLED
  0, // Size ... NEED TO BE FILLED
  0, // StartLba

  {
    EFI_BLOCK_IO_PROTOCOL_REVISION2, // Revision
    NULL, // Media ... NEED TO BE FILLED
    NorFlashBlockIoReset, // Reset;
    NorFlashBlockIoReadBlocks,          // ReadBlocks
    NorFlashBlockIoWriteBlocks,         // WriteBlocks
    NorFlashBlockIoFlushBlocks          // FlushBlocks
  }, // BlockIoProtocol

  {
    0, // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE, // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching;
    0, // BlockSize ... NEED TO BE FILLED
    4, //  IoAlign
    0, // LastBlock ... NEED TO BE FILLED
    0, // LowestAlignedLba
    1, // LogicalBlocksPerPhysicalBlock
  }, //Media;

  {
    EFI_DISK_IO_PROTOCOL_REVISION, // Revision
    NorFlashDiskIoReadDisk,        // ReadDisk
    NorFlashDiskIoWriteDisk        // WriteDisk
  },

  {
    FvbGetAttributes, // GetAttributes
    FvbSetAttributes, // SetAttributes
    FvbGetPhysicalAddress,  // GetPhysicalAddress
    FvbGetBlockSize,  // GetBlockSize
    FvbRead,  // Read
    FvbWrite, // Write
    FvbEraseBlocks, // EraseBlocks
    NULL, //ParentHandle
  }, //  FvbProtoccol;
  NULL, // ShadowBuffer
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End)),
          (UINT8)(OFFSET_OF (NOR_FLASH_DEVICE_PATH, End) >> 8)
        }
      },
      { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }, // GUID ... NEED TO BE FILLED
    },
    0, // Index
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
    } // DevicePath
};

EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                  NorFlashDeviceBase,
  IN UINTN                  NorFlashRegionBase,
  IN UINTN                  NorFlashSize,
  IN UINT32                 Index,
  IN UINT32                 BlockSize,
  IN BOOLEAN                SupportFvb,
  OUT NOR_FLASH_INSTANCE**  NorFlashInstance
  )
{
  EFI_STATUS Status;
  NOR_FLASH_INSTANCE* Instance;

  ASSERT(NorFlashInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof(NOR_FLASH_INSTANCE),&mNorFlashInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->DeviceBaseAddress = NorFlashDeviceBase;
  Instance->RegionBaseAddress = NorFlashRegionBase;
  Instance->Size = NorFlashSize;

  Instance->BlockIoProtocol.Media = &Instance->Media;
  Instance->Media.MediaId = Index;
  Instance->Media.BlockSize = BlockSize;
  Instance->Media.LastBlock = (NorFlashSize / BlockSize)-1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)Index;

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);;
  if (Instance->ShadowBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (SupportFvb) {
    NorFlashFvbInitialize (Instance);

    Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->Handle,
                  &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                  &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
                  &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
                  NULL
                  );
    if (EFI_ERROR(Status)) {
      FreePool (Instance);
      return Status;
    }
  } else {
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Instance->Handle,
                    &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                    &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
                    &gEfiDiskIoProtocolGuid, &Instance->DiskIoProtocol,
                    NULL
                    );
    if (EFI_ERROR(Status)) {
      FreePool (Instance);
      return Status;
    }
  }

  *NorFlashInstance = Instance;
  return Status;
}

/**
 * This function unlock and erase an entire NOR Flash block.
 **/
EFI_STATUS
NorFlashUnlockAndEraseSingleBlock (
  IN NOR_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
  EFI_STATUS      Status;
  UINTN           Index;
  EFI_TPL         OriginalTPL;

  if (!EfiAtRuntime ()) {
    // Raise TPL to TPL_HIGH to stop anyone from interrupting us.
    OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  } else {
    // This initialization is only to prevent the compiler to complain about the
    // use of uninitialized variables
    OriginalTPL = TPL_HIGH_LEVEL;
  }

  Index = 0;
  // The block erase might fail a first time (SW bug ?). Retry it ...
  do {
    // Unlock the block if we have to
    Status = NorFlashUnlockSingleBlockIfNecessary (Instance, BlockAddress);
    if (EFI_ERROR (Status)) {
      break;
    }
    Status = NorFlashEraseSingleBlock (Instance, BlockAddress);
    Index++;
  } while ((Index < NOR_FLASH_ERASE_RETRY) && (Status == EFI_WRITE_PROTECTED));

  if (Index == NOR_FLASH_ERASE_RETRY) {
    DEBUG((EFI_D_ERROR,"EraseSingleBlock(BlockAddress=0x%08x: Block Locked Error (try to erase %d times)\n", BlockAddress,Index));
  }

  if (!EfiAtRuntime ()) {
    // Interruptions can resume.
    gBS->RestoreTPL (OriginalTPL);
  }

  return Status;
}

EFI_STATUS
NorFlashWriteFullBlock (
  IN NOR_FLASH_INSTANCE     *Instance,
  IN EFI_LBA                Lba,
  IN UINT32                 *DataBuffer,
  IN UINT32                 BlockSizeInWords
  )
{
  EFI_STATUS    Status;
  UINTN         WordAddress;
  UINT32        WordIndex;
  UINTN         BufferIndex;
  UINTN         BlockAddress;
  UINTN         BuffersInBlock;
  UINTN         RemainingWords;
  EFI_TPL       OriginalTPL;
  UINTN         Cnt;

  Status = EFI_SUCCESS;

  // Get the physical address of the block
  BlockAddress = GET_NOR_BLOCK_ADDRESS (Instance->RegionBaseAddress, Lba, BlockSizeInWords * 4);

  // Start writing from the first address at the start of the block
  WordAddress = BlockAddress;

  if (!EfiAtRuntime ()) {
    // Raise TPL to TPL_HIGH to stop anyone from interrupting us.
    OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);
  } else {
    // This initialization is only to prevent the compiler to complain about the
    // use of uninitialized variables
    OriginalTPL = TPL_HIGH_LEVEL;
  }

  Status = NorFlashUnlockAndEraseSingleBlock (Instance, BlockAddress);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "WriteSingleBlock: ERROR - Failed to Unlock and Erase the single block at 0x%X\n", BlockAddress));
    goto EXIT;
  }

  // To speed up the programming operation, NOR Flash is programmed using the Buffered Programming method.

  // Check that the address starts at a 32-word boundary, i.e. last 7 bits must be zero
  if ((WordAddress & BOUNDARY_OF_32_WORDS) == 0x00) {

    // First, break the entire block into buffer-sized chunks.
    BuffersInBlock = (UINTN)(BlockSizeInWords * 4) / P30_MAX_BUFFER_SIZE_IN_BYTES;

    // Then feed each buffer chunk to the NOR Flash
    // If a buffer does not contain any data, don't write it.
    for(BufferIndex=0;
         BufferIndex < BuffersInBlock;
         BufferIndex++, WordAddress += P30_MAX_BUFFER_SIZE_IN_BYTES, DataBuffer += P30_MAX_BUFFER_SIZE_IN_WORDS
      ) {
      // Check the buffer to see if it contains any data (not set all 1s).
      for (Cnt = 0; Cnt < P30_MAX_BUFFER_SIZE_IN_WORDS; Cnt++) {
        if (~DataBuffer[Cnt] != 0 ) {
          // Some data found, write the buffer.
          Status = NorFlashWriteBuffer (Instance, WordAddress, P30_MAX_BUFFER_SIZE_IN_BYTES,
                                        DataBuffer);
          if (EFI_ERROR(Status)) {
            goto EXIT;
          }
          break;
        }
      }
    }

    // Finally, finish off any remaining words that are less than the maximum size of the buffer
    RemainingWords = BlockSizeInWords % P30_MAX_BUFFER_SIZE_IN_WORDS;

    if(RemainingWords != 0) {
      Status = NorFlashWriteBuffer (Instance, WordAddress, (RemainingWords * 4), DataBuffer);
      if (EFI_ERROR(Status)) {
        goto EXIT;
      }
    }

  } else {
    // For now, use the single word programming algorithm
    // It is unlikely that the NOR Flash will exist in an address which falls within a 32 word boundary range,
    // i.e. which ends in the range 0x......01 - 0x......7F.
    for(WordIndex=0; WordIndex<BlockSizeInWords; WordIndex++, DataBuffer++, WordAddress = WordAddress + 4) {
      Status = NorFlashWriteSingleWord (Instance, WordAddress, *DataBuffer);
      if (EFI_ERROR(Status)) {
        goto EXIT;
      }
    }
  }

EXIT:
  if (!EfiAtRuntime ()) {
    // Interruptions can resume.
    gBS->RestoreTPL (OriginalTPL);
  }

  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "NOR FLASH Programming [WriteSingleBlock] failed at address 0x%08x. Exit Status = \"%r\".\n", WordAddress, Status));
  }
  return Status;
}

EFI_STATUS
EFIAPI
NorFlashInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  NOR_FLASH_DESCRIPTION*  NorFlashDevices;
  BOOLEAN                 ContainVariableStorage;

  Status = NorFlashPlatformInitialization ();
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to initialize Nor Flash devices\n"));
    return Status;
  }

  Status = NorFlashPlatformGetDevices (&NorFlashDevices, &mNorFlashDeviceCount);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to get Nor Flash devices\n"));
    return Status;
  }

  mNorFlashInstances = AllocateRuntimePool (sizeof(NOR_FLASH_INSTANCE*) * mNorFlashDeviceCount);

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    // Check if this NOR Flash device contain the variable storage region

   if (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
     ContainVariableStorage =
       (NorFlashDevices[Index].RegionBaseAddress <= PcdGet64 (PcdFlashNvStorageVariableBase64)) &&
       (PcdGet64 (PcdFlashNvStorageVariableBase64) + PcdGet32 (PcdFlashNvStorageVariableSize) <=
        NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
   } else {
     ContainVariableStorage =
       (NorFlashDevices[Index].RegionBaseAddress <= PcdGet32 (PcdFlashNvStorageVariableBase)) &&
       (PcdGet32 (PcdFlashNvStorageVariableBase) + PcdGet32 (PcdFlashNvStorageVariableSize) <=
        NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
  }

    Status = NorFlashCreateInstance (
      NorFlashDevices[Index].DeviceBaseAddress,
      NorFlashDevices[Index].RegionBaseAddress,
      NorFlashDevices[Index].Size,
      Index,
      NorFlashDevices[Index].BlockSize,
      ContainVariableStorage,
      &mNorFlashInstances[Index]
    );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"NorFlashInitialise: Fail to create instance for NorFlash[%d]\n",Index));
    }
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  NorFlashVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mNorFlashVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE* Instance
  )
{
  EFI_STATUS  Status;
  UINT32      FvbNumLba;
  EFI_BOOT_MODE BootMode;
  UINTN       RuntimeMmioRegionSize;

  DEBUG((DEBUG_BLKIO,"NorFlashFvbInitialize\n"));
  ASSERT((Instance != NULL));

  //
  // Declare the Non-Volatile storage as EFI_MEMORY_RUNTIME
  //

  // Note: all the NOR Flash region needs to be reserved into the UEFI Runtime memory;
  //       even if we only use the small block region at the top of the NOR Flash.
  //       The reason is when the NOR Flash memory is set into program mode, the command
  //       is written as the base of the flash region (ie: Instance->DeviceBaseAddress)
  RuntimeMmioRegionSize = (Instance->RegionBaseAddress - Instance->DeviceBaseAddress) + Instance->Size;

  Status = gDS->AddMemorySpace (
      EfiGcdMemoryTypeMemoryMappedIo,
      Instance->DeviceBaseAddress, RuntimeMmioRegionSize,
      EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
      );
  ASSERT_EFI_ERROR (Status);

  Status = gDS->SetMemorySpaceAttributes (
      Instance->DeviceBaseAddress, RuntimeMmioRegionSize,
      EFI_MEMORY_UC | EFI_MEMORY_RUNTIME);
  ASSERT_EFI_ERROR (Status);

  mFlashNvStorageVariableBase = (PcdGet64 (PcdFlashNvStorageVariableBase64) != 0) ?
    PcdGet64 (PcdFlashNvStorageVariableBase64) : PcdGet32 (PcdFlashNvStorageVariableBase);

  // Set the index of the first LBA for the FVB
  Instance->StartLba = (mFlashNvStorageVariableBase - Instance->RegionBaseAddress) / Instance->Media.BlockSize;

  BootMode = GetBootModeHob ();
  if (BootMode == BOOT_WITH_DEFAULT_SETTINGS) {
    Status = EFI_INVALID_PARAMETER;
  } else {
    // Determine if there is a valid header at the beginning of the NorFlash
    Status = ValidateFvHeader (Instance);
  }

  // Install the Default FVB header if required
  if (EFI_ERROR(Status)) {
    // There is no valid header, so time to install one.
    DEBUG ((DEBUG_INFO, "%a: The FVB Header is not valid.\n", __FUNCTION__));
    DEBUG ((DEBUG_INFO, "%a: Installing a correct one for this volume.\n",
      __FUNCTION__));

    // Erase all the NorFlash that is reserved for variable storage
    FvbNumLba = (PcdGet32(PcdFlashNvStorageVariableSize) + PcdGet32(PcdFlashNvStorageFtwWorkingSize) + PcdGet32(PcdFlashNvStorageFtwSpareSize)) / Instance->Media.BlockSize;

    Status = FvbEraseBlocks (&Instance->FvbProtocol, (EFI_LBA)0, FvbNumLba, EFI_LBA_LIST_TERMINATOR);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    // Install all appropriate headers
    Status = InitializeFvAndVariableStoreHeaders (Instance);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //
  // The driver implementing the variable read service can now be dispatched;
  // the varstore headers are in place.
  //
  Status = gBS->InstallProtocolInterface (
                  &gImageHandle,
                  &gEdkiiNvVarStoreFormattedGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mFvbVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
