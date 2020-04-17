/** @file  BlSMMStoreDxe.c

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/SMMStoreLib.h>
#include <Library/HobLib.h>

#include "BlSMMStoreDxe.h"

STATIC EFI_EVENT mSMMStoreVirtualAddrChangeEvent;

//
// Global variable declarations
//
SMMSTORE_INSTANCE *mSMMStoreInstance;

SMMSTORE_INSTANCE  mSMMStoreInstanceTemplate = {
  SMMSTORE_SIGNATURE, // Signature
  NULL, // Handle ... NEED TO BE FILLED
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
    FvbGetAttributes, // GetAttributes
    FvbSetAttributes, // SetAttributes
    FvbGetPhysicalAddress,  // GetPhysicalAddress
    FvbGetBlockSize,  // GetBlockSize
    FvbRead,  // Read
    FvbWrite, // Write
    FvbEraseBlocks, // EraseBlocks
    NULL, //ParentHandle
  }, //  FvbProtoccol;
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

STATIC
EFI_STATUS
SMMStoreCreateInstance (
  IN UINTN                  NumberofBlocks,
  IN UINTN                  BlockSize,
  OUT SMMSTORE_INSTANCE**  SMMStoreInstance
  )
{
  EFI_STATUS Status;
  SMMSTORE_INSTANCE* Instance;

  ASSERT(SMMStoreInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof(SMMSTORE_INSTANCE),&mSMMStoreInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Media.MediaId = 0;
  Instance->Media.BlockSize = BlockSize;
  Instance->Media.LastBlock = NumberofBlocks - 1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, &gEfiCallerIdGuid);
  Instance->DevicePath.Index = (UINT8)0;

  Status = SMMStoreFvbInitialize (Instance);
  if (EFI_ERROR(Status)) {
    FreePool (Instance);
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                &Instance->Handle,
                &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
                &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
                NULL
                );
  if (EFI_ERROR(Status)) {
    FreePool (Instance);
    return Status;
  }

  DEBUG((DEBUG_INFO, "%a: Created a new instance\n", __FUNCTION__));

  *SMMStoreInstance = Instance;
  return Status;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
BlSMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  // Convert Fvb
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.EraseBlocks);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetBlockSize);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.GetPhysicalAddress);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.Read);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.SetAttributes);
  EfiConvertPointer (0x0, (VOID**)&mSMMStoreInstance->FvbProtocol.Write);

  SMMStoreVirtualNotifyEvent (Event, Context);

  return;
}

EFI_STATUS
EFIAPI
BlSMMSTOREInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                              Status;
  VOID                                    *ComBuf;
  VOID                                    *GuidHob;
  SMMSTORE_INFO                           *SMMStoreInfoHob;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR         GcdDescriptor;

  if (PcdGetBool (PcdEmuVariableNvModeEnable)) {
    DEBUG ((DEBUG_WARN, "Variable emulation is active! Skipping driver init.\n"));
    return EFI_SUCCESS;
  }

  //
  // Find the SMMSTORE information guid hob
  //
  GuidHob = GetFirstGuidHob (&gEfiSMMSTOREInfoHobGuid);
  if (GuidHob == NULL) {
    DEBUG ((DEBUG_WARN, "SMMSTORE not supported! Skipping driver init.\n"));
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_SUCCESS;
  }

  //
  // Allocate Communication Buffer for arguments to pass to SMM
  //
  ComBuf = AllocateRuntimePool (SMMSTORE_COMBUF_SIZE);
  if (!ComBuf) {
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Place SMMSTORE information hob in a runtime buffer
  //
  SMMStoreInfoHob = AllocateRuntimePool (GET_GUID_HOB_DATA_SIZE(GuidHob));
  if (!SMMStoreInfoHob) {
    FreePool(ComBuf);
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem(SMMStoreInfoHob, GET_GUID_HOB_DATA (GuidHob), GET_GUID_HOB_DATA_SIZE(GuidHob));

  if (!SMMStoreInfoHob->MmioAddress ||
      !SMMStoreInfoHob->ComBuffer ||
      !SMMStoreInfoHob->BlockSize ||
      !SMMStoreInfoHob->NumBlocks) {
    DEBUG((DEBUG_ERROR, "%a: Invalid data in SMMStore Info hob\n", __FUNCTION__));
    FreePool(ComBuf);
    FreePool(SMMStoreInfoHob);
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_WRITE_PROTECTED;
  }

  //
  // Update PCDs for VariableRuntimeDxe
  // Can't do it later as VariableRuntimeDxe has no Depex
  //
  PcdSet32S (PcdFlashNvStorageVariableBase,
    PcdGet32 (PcdFlashNvStorageVariableBase) + SMMStoreInfoHob->MmioAddress);
  PcdSet32S (PcdFlashNvStorageFtwWorkingBase,
    PcdGet32 (PcdFlashNvStorageFtwWorkingBase) + SMMStoreInfoHob->MmioAddress);
  PcdSet32S (PcdFlashNvStorageFtwSpareBase,
    PcdGet32 (PcdFlashNvStorageFtwSpareBase) + SMMStoreInfoHob->MmioAddress);

  Status = SMMStoreInitialize(ComBuf, SMMStoreInfoHob);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR,"%a: Failed to initialize SMMStore\n",
      __FUNCTION__));
    FreePool(ComBuf);
    FreePool(SMMStoreInfoHob);
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  mSMMStoreInstance = AllocateRuntimePool (sizeof(SMMSTORE_INSTANCE*));
  if (!mSMMStoreInstance) {
    DEBUG((DEBUG_ERROR, "%a: Out of resources\n", __FUNCTION__));
    FreePool(ComBuf);
    FreePool(SMMStoreInfoHob);
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = SMMStoreCreateInstance (
    SMMStoreInfoHob->NumBlocks,
    SMMStoreInfoHob->BlockSize,
    &mSMMStoreInstance
  );
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "%a: Fail to create instance for SMMStore\n",
      __FUNCTION__));
    FreePool(ComBuf);
    FreePool(SMMStoreInfoHob);
    PcdSetBoolS (PcdEmuVariableNvModeEnable, TRUE);
    return Status;
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  BlSMMStoreVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSMMStoreVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Finally mark the SMM communication buffer provided by CB or SBL as runtime memory
  //
  Status      = gDS->GetMemorySpaceDescriptor (SMMStoreInfoHob->ComBuffer, &GcdDescriptor);
  if (EFI_ERROR (Status) || GcdDescriptor.GcdMemoryType != EfiGcdMemoryTypeReserved) {
    DEBUG((DEBUG_INFO, "%a: No memory space descriptor for com buffer found\n",
      __FUNCTION__));

    //
    // Add a new entry if not covered by existing mapping
    //
    Status = gDS->AddMemorySpace (
        EfiGcdMemoryTypeReserved,
        SMMStoreInfoHob->ComBuffer, SMMStoreInfoHob->ComBufferSize,
        EFI_MEMORY_WB | EFI_MEMORY_RUNTIME
        );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Mark as runtime service
  //
  Status = gDS->SetMemorySpaceAttributes (
                  SMMStoreInfoHob->ComBuffer,
                  SMMStoreInfoHob->ComBufferSize,
                  EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Mark the memory mapped store as MMIO memory
  //
  Status      = gDS->GetMemorySpaceDescriptor (SMMStoreInfoHob->MmioAddress, &GcdDescriptor);
  if (EFI_ERROR (Status) || GcdDescriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) {
    DEBUG((DEBUG_INFO, "%a: No memory space descriptor for com buffer found\n",
      __FUNCTION__));

    //
    // Add a new entry if not covered by existing mapping
    //
    Status = gDS->AddMemorySpace (
        EfiGcdMemoryTypeMemoryMappedIo,
        SMMStoreInfoHob->MmioAddress,
        SMMStoreInfoHob->NumBlocks * SMMStoreInfoHob->BlockSize,
        EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
        );
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Mark as runtime service
  //
  Status = gDS->SetMemorySpaceAttributes (
                  SMMStoreInfoHob->MmioAddress,
                  SMMStoreInfoHob->NumBlocks * SMMStoreInfoHob->BlockSize,
                  EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
