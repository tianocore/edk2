/**@file
  Functions related to the Firmware Volume Block service whose
  implementation is specific to the runtime DXE driver build.

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Guid/EventGroup.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include "FwBlockService.h"
#include "QemuFlash.h"

VOID
InstallProtocolInterfaces (
  IN EFI_FW_VOL_BLOCK_DEVICE *FvbDevice
  )
{
  EFI_STATUS                         Status;
  EFI_HANDLE                         FwbHandle;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL *OldFwbInterface;

  ASSERT (!FeaturePcdGet (PcdSmmSmramRequire));

  //
  // Find a handle with a matching device path that has supports FW Block
  // protocol
  //
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid,
                  &FvbDevice->DevicePath, &FwbHandle);
  if (EFI_ERROR (Status)) {
    //
    // LocateDevicePath fails so install a new interface and device path
    //
    FwbHandle = NULL;
    DEBUG ((DEBUG_INFO, "Installing QEMU flash FVB\n"));
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    &FvbDevice->FwVolBlockInstance,
                    &gEfiDevicePathProtocolGuid,
                    FvbDevice->DevicePath,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  } else if (IsDevicePathEnd (FvbDevice->DevicePath)) {
    //
    // Device already exists, so reinstall the FVB protocol
    //
    Status = gBS->HandleProtocol (
                    FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    (VOID**)&OldFwbInterface
                    );
    ASSERT_EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO, "Reinstalling FVB for QEMU flash region\n"));
    Status = gBS->ReinstallProtocolInterface (
                    FwbHandle,
                    &gEfiFirmwareVolumeBlockProtocolGuid,
                    OldFwbInterface,
                    &FvbDevice->FwVolBlockInstance
                    );
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // There was a FVB protocol on an End Device Path node
    //
    ASSERT (FALSE);
  }
}


STATIC
VOID
EFIAPI
FvbVirtualAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

  Routine Description:

    Fixup internal data so that EFI and SAL can be call in virtual mode.
    Call the passed in Child Notify event and convert the mFvbModuleGlobal
    date items to there virtual address.

  Arguments:

    (Standard EFI notify event - EFI_EVENT_NOTIFY)

  Returns:

    None

--*/
{
  EFI_FW_VOL_INSTANCE *FwhInstance;
  UINTN               Index;

  FwhInstance = mFvbModuleGlobal->FvInstance;
  EfiConvertPointer (0x0, (VOID **) &mFvbModuleGlobal->FvInstance);

  //
  // Convert the base address of all the instances
  //
  Index       = 0;
  while (Index < mFvbModuleGlobal->NumFv) {
    EfiConvertPointer (0x0, (VOID **) &FwhInstance->FvBase);
    FwhInstance = (EFI_FW_VOL_INSTANCE *)
      (
        (UINTN) ((UINT8 *) FwhInstance) +
        FwhInstance->VolumeHeader.HeaderLength +
        (sizeof (EFI_FW_VOL_INSTANCE) - sizeof (EFI_FIRMWARE_VOLUME_HEADER))
      );
    Index++;
  }

  EfiConvertPointer (0x0, (VOID **) &mFvbModuleGlobal);
  QemuFlashConvertPointers ();
}


VOID
InstallVirtualAddressChangeHandler (
  VOID
  )
{
  EFI_STATUS Status;
  EFI_EVENT  VirtualAddressChangeEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  FvbVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);
}

EFI_STATUS
MarkIoMemoryRangeForRuntimeAccess (
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINTN                               Length
  )
{
  EFI_STATUS                          Status;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     GcdDescriptor;

  //
  // Mark flash region as runtime memory
  //
  Status = gDS->RemoveMemorySpace (
                  BaseAddress,
                  Length
                  );

  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  BaseAddress,
                  Length,
                  EFI_MEMORY_UC | EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  0,
                  Length,
                  &BaseAddress,
                  gImageHandle,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  ASSERT_EFI_ERROR (Status);

  Status = gDS->SetMemorySpaceAttributes (
                  BaseAddress,
                  Length,
                  GcdDescriptor.Attributes | EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // When SEV is active, AmdSevDxe mapped the BaseAddress with C=0 but
  // SetMemorySpaceAttributes() remaps the range with C=1. Let's restore
  // the mapping so that both guest and hyervisor can access the flash
  // memory range.
  //
  if (MemEncryptSevIsEnabled ()) {
    Status = MemEncryptSevClearPageEncMask (
               0,
               BaseAddress,
               EFI_SIZE_TO_PAGES (Length),
               FALSE
               );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

VOID
SetPcdFlashNvStorageBaseAddresses (
  VOID
  )
{
  RETURN_STATUS PcdStatus;

  //
  // Set several PCD values to point to flash
  //
  PcdStatus = PcdSet64S (
    PcdFlashNvStorageVariableBase64,
    (UINTN) PcdGet32 (PcdOvmfFlashNvStorageVariableBase)
    );
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (
    PcdFlashNvStorageFtwWorkingBase,
    PcdGet32 (PcdOvmfFlashNvStorageFtwWorkingBase)
    );
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (
    PcdFlashNvStorageFtwSpareBase,
    PcdGet32 (PcdOvmfFlashNvStorageFtwSpareBase)
    );
  ASSERT_RETURN_ERROR (PcdStatus);
}
