/** @file
  This file implement EfiMain() for library class DxeSmmDriverEntryPoint.
  EfiMain() is common driver entry point for all SMM driver who uses DxeSmmDriverEntryPoint
  library class.

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <FrameworkSmm.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/SmmBase.h>
#include <Protocol/DevicePath.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>

/**
  This function returns the size, in bytes,
  of the device path data structure specified by DevicePath.
  If DevicePath is NULL, then 0 is returned.

  @param  DevicePath A pointer to a device path data structure.

  @return The size of a device path in bytes.

**/
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
  while (!IsDevicePathEnd (DevicePath)) {
    DevicePath = NextDevicePathNode (DevicePath);
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

  Status = gBS->AllocatePool (EfiBootServicesData, Size, (VOID **) &NewDevicePath);

  if (EFI_SUCCESS == Status) {
    //
    // CopyMem in gBS is used as this service should always be ready. We didn't choose
    // to use a BaseMemoryLib function as such library instance may have constructor.
    //
    gBS->CopyMem ((VOID *) NewDevicePath, (VOID *) FirstDevicePath, Size1);
    //
    // Over write Src1 EndNode and do the copy
    //
    DevicePath2 = (EFI_DEVICE_PATH_PROTOCOL *) ((CHAR8 *) NewDevicePath + (Size1 - sizeof (EFI_DEVICE_PATH_PROTOCOL)));
    gBS->CopyMem ((VOID *) DevicePath2, (VOID *) SecondDevicePath, Size2);
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
  //
  // Call the unload handlers for all the modules.
  // 
  // Note: All libraries were constructed in SMM space, 
  // therefore we can not destruct them in Unload 
  // handler.
  //
  return ProcessModuleUnloadList (ImageHandle);
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
  gBS = SystemTable->BootServices;

  //
  // Retrieve SMM Base Protocol
  //
  Status = gBS->LocateProtocol (
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
    // Retrieve the Loaded Image Protocol
    //
    Status = gBS->HandleProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID*)&LoadedImage
                  );
    ASSERT_EFI_ERROR (Status);
    //
    // Retrieve the Device Path Protocol from the DeviceHandle from which this driver was loaded
    //
    Status = gBS->HandleProtocol (
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
    Status = SmmBase->Register (SmmBase, CompleteFilePath, LoadedImage->ImageBase, 0, &Handle, FALSE);
    ASSERT_EFI_ERROR (Status);
    //
    // Optionally install the unload handler
    //
    if (_gDriverUnloadImageCount > 0) {
      Status = gBS->HandleProtocol (
                      ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
      ASSERT_EFI_ERROR (Status);
      LoadedImage->Unload = _DriverUnloadHandler;
    }

    return Status;
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

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
