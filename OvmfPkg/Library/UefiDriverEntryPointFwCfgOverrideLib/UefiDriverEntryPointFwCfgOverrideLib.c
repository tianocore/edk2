/** @file
  Entry point to a EFI/DXE driver. This version is specific to QEMU, and ties
  dispatch of the driver in question on the value of a QEMU fw_cfg boolean
  variable which is referenced by name via a fixed pointer PCD.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2022, Google LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Protocol/LoadedImage.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

/**
  Unloads an image from memory.

  This function is a callback that a driver registers to do cleanup
  when the UnloadImage boot service function is called.

  @param  ImageHandle The handle to the image to unload.

  @return Status returned by all unload().

**/
STATIC
EFI_STATUS
EFIAPI
_DriverUnloadHandler (
  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  //
  // If an UnloadImage() handler is specified, then call it
  //
  Status = ProcessModuleUnloadList (ImageHandle);

  //
  // If the driver specific unload handler does not return an error, then call
  // all of the library destructors.  If the unload handler returned an error,
  // then the driver can not be unloaded, and the library destructors should
  // not be called
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
  The entry point of PE/COFF Image for a DXE Driver, DXE Runtime Driver, or
  UEFI Driver.

  @param  ImageHandle                The image handle of the DXE Driver, DXE
                                     Runtime Driver, or UEFI Driver.
  @param  SystemTable                A pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, or
                                     UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than
                                     SystemTable->Hdr.Revision.
  @retval  Other                     Return value from
                                     ProcessModuleEntryPointList().

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
  RETURN_STATUS              RetStatus;
  BOOLEAN                    Enabled;

  if (_gUefiDriverRevision != 0) {
    //
    // Make sure that the EFI/UEFI spec revision of the platform is >= EFI/UEFI
    // spec revision of the driver
    //
    if (SystemTable->Hdr.Revision < _gUefiDriverRevision) {
      return EFI_INCOMPATIBLE_VERSION;
    }
  }

  //
  // Call constructor for all libraries
  //
  ProcessLibraryConstructorList (ImageHandle, SystemTable);

  //
  //  Install unload handler...
  //
  if (_gDriverUnloadImageCount != 0) {
    Status = gBS->HandleProtocol (
                    ImageHandle,
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);
    LoadedImage->Unload = _DriverUnloadHandler;
  }

  RetStatus = QemuFwCfgParseBool (
                FixedPcdGetPtr (PcdEntryPointOverrideFwCfgVarName),
                &Enabled
                );
  if (!RETURN_ERROR (RetStatus) && !Enabled) {
    //
    // The QEMU fw_cfg variable tells us not to load this image.  So abort.
    //
    Status = EFI_ABORTED;
  } else {
    //
    // Call the driver entry point
    //
    Status = ProcessModuleEntryPointList (ImageHandle, SystemTable);
  }

  //
  // If all of the drivers returned errors, or we if are aborting, then invoke
  // all of the library destructors
  //
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (ImageHandle, SystemTable);
  }

  //
  // Return the cumulative return status code from all of the driver entry
  // points
  //
  return Status;
}
