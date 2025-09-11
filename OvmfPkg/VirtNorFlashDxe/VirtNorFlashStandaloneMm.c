/** @file  VirtNorFlashStandaloneMm.c

  Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2020, Linaro, Ltd. All rights reserved.<BR>
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MmServicesTableLib.h>
#include <Library/VirtNorFlashDeviceLib.h>

#include "VirtNorFlashDxe.h"

//
// Global variable declarations
//
NOR_FLASH_INSTANCE  **mNorFlashInstances;
UINT32              mNorFlashDeviceCount;
UINTN               mFlashNvStorageVariableBase;

NOR_FLASH_INSTANCE  mNorFlashInstanceTemplate = {
  NOR_FLASH_SIGNATURE, // Signature
  NULL,                // Handle ... NEED TO BE FILLED

  0, // DeviceBaseAddress ... NEED TO BE FILLED
  0, // RegionBaseAddress ... NEED TO BE FILLED
  0, // Size ... NEED TO BE FILLED
  0, // StartLba
  0, // LastBlock
  0, // BlockSize

  {
    FvbGetAttributes,      // GetAttributes
    FvbSetAttributes,      // SetAttributes
    FvbGetPhysicalAddress, // GetPhysicalAddress
    FvbGetBlockSize,       // GetBlockSize
    FvbRead,               // Read
    FvbWrite,              // Write
    FvbEraseBlocks,        // EraseBlocks
    NULL,                  // ParentHandle
  },    //  FvbProtoccol;
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
      { 0x0,                               0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
      },                                                             // GUID ... NEED TO BE FILLED
    },
    0, // Index
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  }   // DevicePath
};

EFI_STATUS
NorFlashCreateInstance (
  IN UINTN                NorFlashDeviceBase,
  IN UINTN                NorFlashRegionBase,
  IN UINTN                NorFlashSize,
  IN UINT32               Index,
  IN UINT32               BlockSize,
  IN BOOLEAN              SupportFvb,
  OUT NOR_FLASH_INSTANCE  **NorFlashInstance
  )
{
  EFI_STATUS          Status;
  NOR_FLASH_INSTANCE  *Instance;

  ASSERT (NorFlashInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof (NOR_FLASH_INSTANCE), &mNorFlashInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->DeviceBaseAddress = NorFlashDeviceBase;
  Instance->RegionBaseAddress = NorFlashRegionBase;
  Instance->Size              = NorFlashSize;
  Instance->BlockSize         = BlockSize;
  Instance->LastBlock         = (NorFlashSize / BlockSize) - 1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)Index;

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);
  if (Instance->ShadowBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (SupportFvb) {
    NorFlashFvbInitialize (Instance);

    Status = gMmst->MmInstallProtocolInterface (
                      &Instance->Handle,
                      &gEfiSmmFirmwareVolumeBlockProtocolGuid,
                      EFI_NATIVE_INTERFACE,
                      &Instance->FvbProtocol
                      );
    if (EFI_ERROR (Status)) {
      FreePool (Instance);
      return Status;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "standalone MM NOR Flash driver only support FVB.\n"));
    FreePool (Instance);
    return EFI_UNSUPPORTED;
  }

  *NorFlashInstance = Instance;
  return Status;
}

EFI_STATUS
EFIAPI
NorFlashInitialise (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS                  Status;
  UINT32                      Index;
  VIRT_NOR_FLASH_DESCRIPTION  *NorFlashDevices;
  BOOLEAN                     ContainVariableStorage;

  Status = VirtNorFlashPlatformInitialization ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to initialize Nor Flash devices\n"));
    return Status;
  }

  Status = VirtNorFlashPlatformGetDevices (&NorFlashDevices, &mNorFlashDeviceCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to get Nor Flash devices\n"));
    return Status;
  }

  mNorFlashInstances = AllocatePool (sizeof (NOR_FLASH_INSTANCE *) * mNorFlashDeviceCount);

  for (Index = 0; Index < mNorFlashDeviceCount; Index++) {
    // Check if this NOR Flash device contain the variable storage region

    if (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) != 0) {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= FixedPcdGet64 (PcdFlashNvStorageVariableBase64)) &&
        (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
         NorFlashDevices[Index].RegionBaseAddress + NorFlashDevices[Index].Size);
    } else {
      ContainVariableStorage =
        (NorFlashDevices[Index].RegionBaseAddress <= FixedPcdGet32 (PcdFlashNvStorageVariableBase)) &&
        (FixedPcdGet32 (PcdFlashNvStorageVariableBase) + FixedPcdGet32 (PcdFlashNvStorageVariableSize) <=
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
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "NorFlashInitialise: Fail to create instance for NorFlash[%d]\n", Index));
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
NorFlashFvbInitialize (
  IN NOR_FLASH_INSTANCE  *Instance
  )
{
  EFI_STATUS  Status;
  UINT32      FvbNumLba;

  ASSERT ((Instance != NULL));

  mFlashNvStorageVariableBase = (FixedPcdGet64 (PcdFlashNvStorageVariableBase64) != 0) ?
                                FixedPcdGet64 (PcdFlashNvStorageVariableBase64) : FixedPcdGet32 (PcdFlashNvStorageVariableBase);
  // Set the index of the first LBA for the FVB
  Instance->StartLba = (mFlashNvStorageVariableBase - Instance->RegionBaseAddress) / Instance->BlockSize;

  // Determine if there is a valid header at the beginning of the NorFlash
  Status = ValidateFvHeader (Instance);

  // Install the Default FVB header if required
  if (EFI_ERROR (Status)) {
    // There is no valid header, so time to install one.
    DEBUG ((DEBUG_INFO, "%a: The FVB Header is not valid.\n", __func__));
    DEBUG ((
      DEBUG_INFO,
      "%a: Installing a correct one for this volume.\n",
      __func__
      ));

    // Erase all the NorFlash that is reserved for variable storage
    FvbNumLba = (PcdGet32 (PcdFlashNvStorageVariableSize) + PcdGet32 (PcdFlashNvStorageFtwWorkingSize) + PcdGet32 (PcdFlashNvStorageFtwSpareSize)) / Instance->BlockSize;

    Status = FvbEraseBlocks (&Instance->FvbProtocol, (EFI_LBA)0, FvbNumLba, EFI_LBA_LIST_TERMINATOR);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Install all appropriate headers
    Status = InitializeFvAndVariableStoreHeaders (Instance);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return Status;
}
