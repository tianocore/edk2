/** @file
  Entry point to a EFI/DXE driver.

Copyright (c) 2006, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

EFI_BOOT_SERVICES  *mBS;

/**
  This function returns the size, in bytes, 
  of the device path data structure specified by DevicePath.
  If DevicePath is NULL, then 0 is returned.

  @param  DevicePath A pointer to a device path data structure.

  @return The size of a device path in bytes.

**/
STATIC
UINTN
EFIAPI
SmmGetDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;

  if (DevicePath == NULL) {
    return 0;
  }

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!EfiIsDevicePathEnd (DevicePath)) {
    DevicePath = EfiNextDevicePathNode (DevicePath);
  }

  //
  // Compute the size and add back in the size of the end device path structure
  //
  return ((UINTN) DevicePath - (UINTN) Start) + sizeof (EFI_DEVICE_PATH_PROTOCOL);
}

/**
  This function appends the device path SecondDevicePath
  to every device path instance in FirstDevicePath. 

  @param  FirstDevicePath A pointer to a device path data structure.
  
  @param  SecondDevicePath A pointer to a device path data structure.

  @return A pointer to the new device path is returned.
          NULL is returned if space for the new device path could not be allocated from pool.
          It is up to the caller to free the memory used by FirstDevicePath and SecondDevicePath
          if they are no longer needed.

**/
EFI_DEVICE_PATH_PROTOCOL *
EFIAPI
SmmAppendDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *FirstDevicePath,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *SecondDevicePath
  )
{
  EFI_STATUS                Status;
  UINTN                     Size;
  UINTN                     Size1;
  UINTN                     Size2;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath2;

  ASSERT (FirstDevicePath != NULL && SecondDevicePath != NULL);

  //
  // Allocate space for the combined device path. It only has one end node of
  // length EFI_DEVICE_PATH_PROTOCOL
  //
  Size1         = SmmGetDevicePathSize (FirstDevicePath);
  Size2         = SmmGetDevicePathSize (SecondDevicePath);
  Size          = Size1 + Size2 - sizeof (EFI_DEVICE_PATH_PROTOCOL);

  Status = mBS->AllocatePool (EfiBootServicesData, Size, (VOID **) &NewDevicePath);

  if (EFI_SUCCESS == Status) {
    mBS->CopyMem ((VOID *) NewDevicePath, (VOID *) FirstDevicePath, Size1);
    //
    // Over write Src1 EndNode and do the copy
    //
    DevicePath2 = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    mBS->CopyMem ((VOID *) DevicePath2, (VOID *) SecondDevicePath, Size2);
  }

  return NewDevicePath;
}

/**
  Unload function that is registered in the LoadImage protocol.  It un-installs
  protocols produced and deallocates pool used by the driver.  Called by the core
  when unloading the driver.

  @param  ImageHandle   ImageHandle of the unloaded driver

  @return Status of the ProcessModuleUnloadList.

**/
EFI_STATUS
EFIAPI
_DriverUnloadHandler (
  EFI_HANDLE ImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // Call the unload handlers for all the modules
  //
  Status = ProcessModuleUnloadList (ImageHandle);

  //
  // If the driver specific unload handler does not return an error, then call all of the
  // library destructors.  If the unload handler returned an error, then the driver can not be
  // unloaded, and the library destructors should not be called
  //
  if (!EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, gST);
  }

  //
  // Return the status from the driver specific unload handler
  //
  return Status;
}

/**
  Enrty point to DXE SMM Driver.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

**/
EFI_STATUS
EFIAPI
_ModuleEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  EFI_SMM_BASE_PROTOCOL      *SmmBase;
  BOOLEAN                    InSmm;
  EFI_DEVICE_PATH_PROTOCOL   *CompleteFilePath;
  EFI_DEVICE_PATH_PROTOCOL   *ImageDevicePath;
  EFI_HANDLE                 Handle;

  //
  // Cache a pointer to the Boot Services Table 
  //
  mBS = SystemTable->BootServices;

  //
  // Retrieve the Loaded Image Protocol
  //
  Status = mBS->HandleProtocol (
                  ImageHandle, 
                  &gEfiLoadedImageProtocolGuid,
                  (VOID*)&LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Retrieve SMM Base Protocol
  //
  Status = mBS->LocateProtocol (
                  &gEfiSmmBaseProtocolGuid, 
                  NULL, 
                  (VOID **) &SmmBase
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Check to see if we are already in SMM
  //
  SmmBase->InSmm (SmmBase, &InSmm);

  //
  //
  //
  if (!InSmm) {
    //
    // Retrieve the Device Path Protocol from the DeviceHandle tha this driver was loaded from
    //
    Status = mBS->HandleProtocol (
                    LoadedImage->DeviceHandle, 
                    &gEfiDevicePathProtocolGuid,
                    (VOID*)&ImageDevicePath
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Build the full device path to the currently execuing image
    //
    CompleteFilePath = SmmAppendDevicePath (ImageDevicePath, LoadedImage->FilePath);

    //
    // Load the image in memory to SMRAM; it will automatically generate the
    // SMI.
    //
    Status = SmmBase->Register (SmmBase, CompleteFilePath, NULL, 0, &Handle, FALSE);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  //
  // Optionally install the unload handler
  //
  if (_gDriverUnloadImageCount > 0) {
    Status = mBS->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);
    LoadedImage->Unload = _DriverUnloadHandler;
  }

  //
  // Call the list of driver entry points
  //
  Status = ProcessModuleEntryPointList (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, SystemTable);
  }

  return Status;
}

/**
  Enrty point wrapper of DXE SMM Driver.

  @param  ImageHandle ImageHandle of the loaded driver.
  @param  SystemTable Pointer to the EFI System Table.

  @retval  EFI_SUCCESS One or more of the drivers returned a success code.
  @retval  !EFI_SUCESS The return status from the last driver entry point in the list.

**/
EFI_STATUS
EFIAPI
EfiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return _ModuleEntryPoint (ImageHandle, SystemTable);
}
