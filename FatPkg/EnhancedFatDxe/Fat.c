/** @file
  Fat File System driver routines that support EFI driver model.

Copyright (c) 2005 - 2014, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  Register Driver Binding protocol for this driver.

  @param  ImageHandle           - Handle for the image of this driver.
  @param  SystemTable           - Pointer to the EFI System Table.

  @retval EFI_SUCCESS           - Driver loaded.
  @return other                 - Driver not loaded.

**/
EFI_STATUS
EFIAPI
FatEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**

  Unload function for this image. Uninstall DriverBinding protocol.

  @param ImageHandle           - Handle for the image of this driver.

  @retval EFI_SUCCESS           - Driver unloaded successfully.
  @return other                 - Driver can not unloaded.

**/
EFI_STATUS
EFIAPI
FatUnload (
  IN EFI_HANDLE  ImageHandle
  );

/**

  Test to see if this driver can add a file system to ControllerHandle.
  ControllerHandle must support both Disk IO and Block IO protocols.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to test.
  @param  RemainingDevicePath   - Not used.

  @retval EFI_SUCCESS           - This driver supports this device.
  @retval EFI_ALREADY_STARTED   - This driver is already running on this device.
  @return other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**

  Start this driver on ControllerHandle by opening a Block IO and Disk IO
  protocol, reading Device Path. Add a Simple File System protocol to
  ControllerHandle if the media contains a valid file system.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to bind driver to.
  @param  RemainingDevicePath   - Not used.

  @retval EFI_SUCCESS           - This driver is added to DeviceHandle.
  @retval EFI_ALREADY_STARTED   - This driver is already running on DeviceHandle.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @return other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**

  Stop this driver on ControllerHandle.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to stop driver on.
  @param  NumberOfChildren      - Not used.
  @param  ChildHandleBuffer     - Not used.

  @retval EFI_SUCCESS           - This driver is removed DeviceHandle.
  @return other                 - This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

//
// DriverBinding protocol instance
//
EFI_DRIVER_BINDING_PROTOCOL  gFatDriverBinding = {
  FatDriverBindingSupported,
  FatDriverBindingStart,
  FatDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**

  Register Driver Binding protocol for this driver.

  @param  ImageHandle           - Handle for the image of this driver.
  @param  SystemTable           - Pointer to the EFI System Table.

  @retval EFI_SUCCESS           - Driver loaded.
  @return other                 - Driver not loaded.

**/
EFI_STATUS
EFIAPI
FatEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Initialize the EFI Driver Library
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gFatDriverBinding,
             ImageHandle,
             &gFatComponentName,
             &gFatComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**

  Unload function for this image. Uninstall DriverBinding protocol.

  @param ImageHandle           - Handle for the image of this driver.

  @retval EFI_SUCCESS           - Driver unloaded successfully.
  @return other                 - Driver can not unloaded.

**/
EFI_STATUS
EFIAPI
FatUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *DeviceHandleBuffer;
  UINTN       DeviceHandleCount;
  UINTN       Index;
  VOID        *ComponentName;
  VOID        *ComponentName2;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = EfiTestManagedDevice (DeviceHandleBuffer[Index], ImageHandle, &gEfiDiskIoProtocolGuid);
    if (!EFI_ERROR (Status)) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      ImageHandle,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        break;
      }
    }
  }

  if (Index == DeviceHandleCount) {
    //
    // Driver is stopped successfully.
    //
    Status = gBS->HandleProtocol (ImageHandle, &gEfiComponentNameProtocolGuid, &ComponentName);
    if (EFI_ERROR (Status)) {
      ComponentName = NULL;
    }

    Status = gBS->HandleProtocol (ImageHandle, &gEfiComponentName2ProtocolGuid, &ComponentName2);
    if (EFI_ERROR (Status)) {
      ComponentName2 = NULL;
    }

    if (ComponentName == NULL) {
      if (ComponentName2 == NULL) {
        Status = gBS->UninstallMultipleProtocolInterfaces (
                        ImageHandle,
                        &gEfiDriverBindingProtocolGuid,
                        &gFatDriverBinding,
                        NULL
                        );
      } else {
        Status = gBS->UninstallMultipleProtocolInterfaces (
                        ImageHandle,
                        &gEfiDriverBindingProtocolGuid,
                        &gFatDriverBinding,
                        &gEfiComponentName2ProtocolGuid,
                        ComponentName2,
                        NULL
                        );
      }
    } else {
      if (ComponentName2 == NULL) {
        Status = gBS->UninstallMultipleProtocolInterfaces (
                        ImageHandle,
                        &gEfiDriverBindingProtocolGuid,
                        &gFatDriverBinding,
                        &gEfiComponentNameProtocolGuid,
                        ComponentName,
                        NULL
                        );
      } else {
        Status = gBS->UninstallMultipleProtocolInterfaces (
                        ImageHandle,
                        &gEfiDriverBindingProtocolGuid,
                        &gFatDriverBinding,
                        &gEfiComponentNameProtocolGuid,
                        ComponentName,
                        &gEfiComponentName2ProtocolGuid,
                        ComponentName2,
                        NULL
                        );
      }
    }
  }

  if (DeviceHandleBuffer != NULL) {
    FreePool (DeviceHandleBuffer);
  }

  return Status;
}

/**

  Test to see if this driver can add a file system to ControllerHandle.
  ControllerHandle must support both Disk IO and Block IO protocols.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to test.
  @param  RemainingDevicePath   - Not used.

  @retval EFI_SUCCESS           - This driver supports this device.
  @retval EFI_ALREADY_STARTED   - This driver is already running on this device.
  @return other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDiskIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}

/**

  Start this driver on ControllerHandle by opening a Block IO and Disk IO
  protocol, reading Device Path. Add a Simple File System protocol to
  ControllerHandle if the media contains a valid file system.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to bind driver to.
  @param  RemainingDevicePath   - Not used.

  @retval EFI_SUCCESS           - This driver is added to DeviceHandle.
  @retval EFI_ALREADY_STARTED   - This driver is already running on DeviceHandle.
  @retval EFI_OUT_OF_RESOURCES  - Can not allocate the memory.
  @return other                 - This driver does not support this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  EFI_DISK_IO_PROTOCOL   *DiskIo;
  EFI_DISK_IO2_PROTOCOL  *DiskIo2;
  BOOLEAN                LockedByMe;

  LockedByMe = FALSE;
  //
  // Acquire the lock.
  // If caller has already acquired the lock, cannot lock it again.
  //
  Status = FatAcquireLockOrFail ();
  if (!EFI_ERROR (Status)) {
    LockedByMe = TRUE;
  }

  Status = InitializeUnicodeCollationSupport (This->DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Open our required BlockIo and DiskIo
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIo2ProtocolGuid,
                  (VOID **)&DiskIo2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DiskIo2 = NULL;
  }

  //
  // Allocate Volume structure. In FatAllocateVolume(), Resources
  // are allocated with protocol installed and cached initialized
  //
  Status = FatAllocateVolume (ControllerHandle, DiskIo, DiskIo2, BlockIo);

  //
  // When the media changes on a device it will Reinstall the BlockIo interface.
  // This will cause a call to our Stop(), and a subsequent reentrant call to our
  // Start() successfully. We should leave the device open when this happen.
  //
  if (EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiDiskIoProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );
      gBS->CloseProtocol (
             ControllerHandle,
             &gEfiDiskIo2ProtocolGuid,
             This->DriverBindingHandle,
             ControllerHandle
             );
    }
  }

Exit:
  //
  // Unlock if locked by myself.
  //
  if (LockedByMe) {
    FatReleaseLock ();
  }

  return Status;
}

/**

  Stop this driver on ControllerHandle.

  @param  This                  - Protocol instance pointer.
  @param  ControllerHandle      - Handle of device to stop driver on.
  @param  NumberOfChildren      - Not used.
  @param  ChildHandleBuffer     - Not used.

  @retval EFI_SUCCESS           - This driver is removed DeviceHandle.
  @return other                 - This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
FatDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *FileSystem;
  FAT_VOLUME                       *Volume;
  EFI_DISK_IO2_PROTOCOL            *DiskIo2;

  DiskIo2 = NULL;
  //
  // Get our context back
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&FileSystem,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Volume  = VOLUME_FROM_VOL_INTERFACE (FileSystem);
    DiskIo2 = Volume->DiskIo2;
    Status  = FatAbandonVolume (Volume);
  }

  if (!EFI_ERROR (Status)) {
    if (DiskIo2 != NULL) {
      Status = gBS->CloseProtocol (
                      ControllerHandle,
                      &gEfiDiskIo2ProtocolGuid,
                      This->DriverBindingHandle,
                      ControllerHandle
                      );
      ASSERT_EFI_ERROR (Status);
    }

    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiDiskIoProtocolGuid,
                    This->DriverBindingHandle,
                    ControllerHandle
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}
