/** @file
  UDF/ECMA-167 file system driver.

  Copyright (C) 2014-2017 Paulo Alcantara <pcacjr@zytor.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Udf.h"

//
// UDF filesystem driver's Global Variables.
//
EFI_DRIVER_BINDING_PROTOCOL  gUdfDriverBinding = {
  UdfDriverBindingSupported,
  UdfDriverBindingStart,
  UdfDriverBindingStop,
  0x10,
  NULL,
  NULL
};

EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  gUdfSimpleFsTemplate = {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION,
  UdfOpenVolume
};

/**
  Test to see if this driver supports ControllerHandle. Any ControllerHandle
  than contains a BlockIo and DiskIo protocol or a BlockIo2 protocol can be
  supported.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific
                                  child device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_ALREADY_STARTED This driver is already running on this device.
  @retval other               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UdfDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  EFI_DISK_IO_PROTOCOL  *DiskIo;

  //
  // Open DiskIo protocol on ControllerHandle
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
  // Close DiskIo protocol on ControllerHandle
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDiskIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  //
  // Test whether ControllerHandle supports BlockIo protocol
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
  Start this driver on ControllerHandle by opening a Block IO or a Block IO2
  or both, and Disk IO protocol, reading Device Path, and creating a child
  handle with a Disk IO and device path protocol.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific
                                   child device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on
                               ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UdfDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_TPL                     OldTpl;
  EFI_STATUS                  Status;
  EFI_BLOCK_IO_PROTOCOL       *BlockIo;
  EFI_DISK_IO_PROTOCOL        *DiskIo;
  PRIVATE_UDF_SIMPLE_FS_DATA  *PrivFsData;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Open BlockIo protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Open DiskIo protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDiskIoProtocolGuid,
                  (VOID **)&DiskIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Check if ControllerHandle supports an UDF file system
  //
  Status = SupportUdfFileSystem (This, ControllerHandle);
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Initialize private file system structure
  //
  PrivFsData =
    (PRIVATE_UDF_SIMPLE_FS_DATA *)
    AllocateZeroPool (sizeof (PRIVATE_UDF_SIMPLE_FS_DATA));
  if (PrivFsData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Create new child handle
  //
  PrivFsData->Signature = PRIVATE_UDF_SIMPLE_FS_DATA_SIGNATURE;
  PrivFsData->BlockIo   = BlockIo;
  PrivFsData->DiskIo    = DiskIo;
  PrivFsData->Handle    = ControllerHandle;

  //
  // Set up SimpleFs protocol
  //
  CopyMem (
    (VOID *)&PrivFsData->SimpleFs,
    (VOID *)&gUdfSimpleFsTemplate,
    sizeof (EFI_SIMPLE_FILE_SYSTEM_PROTOCOL)
    );

  //
  // Install child handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &PrivFsData->Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  &PrivFsData->SimpleFs,
                  NULL
                  );

Exit:
  if (EFI_ERROR (Status)) {
    //
    // Close DiskIo protocol on ControllerHandle
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    //
    // Close BlockIo protocol on ControllerHandle
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  gBS->RestoreTPL (OldTpl);

  return Status;
}

/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
UdfDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  PRIVATE_UDF_SIMPLE_FS_DATA       *PrivFsData;
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;

  //
  // Open SimpleFs protocol on ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFs,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    PrivFsData = PRIVATE_UDF_SIMPLE_FS_DATA_FROM_THIS (SimpleFs);

    //
    // Uninstall child handle
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    PrivFsData->Handle,
                    &gEfiSimpleFileSystemProtocolGuid,
                    &PrivFsData->SimpleFs,
                    NULL
                    );

    FreePool ((VOID *)PrivFsData);
  }

  if (!EFI_ERROR (Status)) {
    //
    // Close DiskIo protocol on ControllerHandle
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiDiskIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
    //
    // Close BlockIo protocol on ControllerHandle
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiBlockIoProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  return Status;
}

/**
  The user Entry Point for UDF file system driver. The user code starts with
  this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUdf (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUdfDriverBinding,
             ImageHandle,
             &gUdfComponentName,
             &gUdfComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
