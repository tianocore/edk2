/** @file  SmmStoreFvbRuntime.c

  Copyright (c) 2022, 9elements GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmStoreLib.h>

#include "SmmStoreFvbRuntime.h"

STATIC EFI_EVENT  mSmmStoreVirtualAddrChangeEvent;

//
// Global variable declarations
//
SMMSTORE_INSTANCE  *mSmmStoreInstance;

SMMSTORE_INSTANCE  mSmmStoreInstanceTemplate = {
  SMMSTORE_SIGNATURE, // Signature
  NULL,               // Handle ... NEED TO BE FILLED
  {
    FvbGetAttributes,      // GetAttributes
    FvbSetAttributes,      // SetAttributes
    FvbGetPhysicalAddress, // GetPhysicalAddress
    FvbGetBlockSize,       // GetBlockSize
    FvbRead,               // Read
    FvbWrite,              // Write
    FvbEraseBlocks,        // EraseBlocks
    NULL,                  // ParentHandle
  }, //  FvbProtoccol
  0, // BlockSize ... NEED TO BE FILLED
  0, // LastBlock ... NEED TO BE FILLED
  0, // MmioAddress ... NEED TO BE FILLED
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_MEMMAP_DP,
        {
          (UINT8)(sizeof (MEMMAP_DEVICE_PATH)),
          (UINT8)(sizeof (MEMMAP_DEVICE_PATH) >> 8)
        }
      },
      EfiMemoryMappedIO,
      (EFI_PHYSICAL_ADDRESS)0, // NEED TO BE FILLED
      (EFI_PHYSICAL_ADDRESS)0, // NEED TO BE FILLED
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  } // DevicePath
};

/**
  Initialize the SmmStore instance.


  @param[in]      FvBase         The physical MMIO base address of the FV containing
                                 the variable store.

  @param[in]      NumberofBlocks Number of blocks within the FV.
  @param[in]      BlockSize      The size in bytes of one block within the FV.
  @param[in, out] Instance       The SmmStore instace to initialize

**/
STATIC
EFI_STATUS
SmmStoreInitInstance (
  IN EFI_PHYSICAL_ADDRESS   FvBase,
  IN UINTN                  NumberofBlocks,
  IN UINTN                  BlockSize,
  IN OUT SMMSTORE_INSTANCE  *Instance
  )
{
  EFI_STATUS             Status;
  FV_MEMMAP_DEVICE_PATH  *FvDevicePath;

  ASSERT (Instance != NULL);

  Instance->BlockSize   = BlockSize;
  Instance->LastBlock   = NumberofBlocks - 1;
  Instance->MmioAddress = FvBase;

  FvDevicePath                                = &Instance->DevicePath;
  FvDevicePath->MemMapDevPath.StartingAddress = FvBase;
  FvDevicePath->MemMapDevPath.EndingAddress   = FvBase + BlockSize * NumberofBlocks - 1;

  Status = FvbInitialize (Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->Handle,
                  &gEfiDevicePathProtocolGuid,
                  &Instance->DevicePath,
                  &gEfiFirmwareVolumeBlockProtocolGuid,
                  &Instance->FvbProtocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: Created a new instance\n", __FUNCTION__));

  return Status;
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
SmmStoreVirtualNotifyEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // Convert Fvb
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.EraseBlocks);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.GetAttributes);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.GetBlockSize);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.GetPhysicalAddress);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.Read);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.SetAttributes);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->FvbProtocol.Write);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance->MmioAddress);
  EfiConvertPointer (0x0, (VOID **)&mSmmStoreInstance);

  return;
}

/**
  The user Entry Point for module SmmStoreFvbRuntimeDxe. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmStoreInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  MmioAddress;
  UINTN                 BlockSize;
  UINTN                 BlockCount;
  UINT32                NvStorageBase;
  UINT32                NvStorageSize;
  UINT32                NvVariableSize;
  UINT32                FtwWorkingSize;
  UINT32                FtwSpareSize;

  Status = SmmStoreLibInitialize ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to initialize SmmStoreLib\n", __FUNCTION__));
    return Status;
  }

  Status = SmmStoreLibGetMmioAddress (&MmioAddress);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get SmmStore MMIO address\n", __FUNCTION__));
    SmmStoreLibDeinitialize ();
    return Status;
  }

  Status = SmmStoreLibGetNumBlocks (&BlockCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get SmmStore No. blocks\n", __FUNCTION__));
    SmmStoreLibDeinitialize ();
    return Status;
  }

  Status = SmmStoreLibGetBlockSize (&BlockSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get SmmStore block size\n", __FUNCTION__));
    SmmStoreLibDeinitialize ();
    return Status;
  }

  NvStorageSize = BlockCount * BlockSize;
  NvStorageBase = MmioAddress;

  FtwSpareSize   = (BlockCount / 2) * BlockSize;
  FtwWorkingSize = BlockSize;
  NvVariableSize = NvStorageSize - FtwSpareSize - FtwWorkingSize;
  DEBUG ((DEBUG_INFO, "NvStorageBase:0x%x, NvStorageSize:0x%x\n", NvStorageBase, NvStorageSize));

  if (NvVariableSize >= 0x80000000) {
    SmmStoreLibDeinitialize ();
    return EFI_INVALID_PARAMETER;
  }

  Status = PcdSet32S (PcdFlashNvStorageVariableSize, NvVariableSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageVariableBase, NvStorageBase);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageVariableBase64, NvStorageBase);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingSize, FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwWorkingBase, NvStorageBase + NvVariableSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageFtwWorkingBase64, NvStorageBase + NvVariableSize);
  ASSERT_EFI_ERROR (Status);

  Status = PcdSet32S (PcdFlashNvStorageFtwSpareSize, FtwSpareSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet32S (PcdFlashNvStorageFtwSpareBase, NvStorageBase + NvVariableSize + FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);
  Status = PcdSet64S (PcdFlashNvStorageFtwSpareBase64, NvStorageBase + NvVariableSize + FtwWorkingSize);
  ASSERT_EFI_ERROR (Status);

  mSmmStoreInstance = AllocateRuntimeCopyPool (sizeof (SMMSTORE_INSTANCE), &mSmmStoreInstanceTemplate);
  if (mSmmStoreInstance == NULL) {
    SmmStoreLibDeinitialize ();
    DEBUG ((DEBUG_ERROR, "%a: Out of resources\n", __FUNCTION__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SmmStoreInitInstance (
             MmioAddress,
             BlockCount,
             BlockSize,
             mSmmStoreInstance
             );
  if (EFI_ERROR (Status)) {
    DEBUG (
      (
       DEBUG_ERROR,
       "%a: Fail to create instance for SmmStore\n",
       __FUNCTION__
      )
      );
    FreePool (mSmmStoreInstance);
    SmmStoreLibDeinitialize ();
    return Status;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SmmStoreVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSmmStoreVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
