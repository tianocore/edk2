/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatformDriOverride.c

Abstract:


**/


#include "PlatformDriOverride.h"

EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL gPlatformDriverOverride = {
  GetDriver,
  GetDriverPath,
  DriverLoaded
};

LIST_ENTRY      mMappingDataBase = INITIALIZE_LIST_HEAD_VARIABLE (mMappingDataBase);
BOOLEAN         mEnvironmentVariableRead = FALSE;
EFI_HANDLE      mCallerImageHandle;

/**
  Platform Driver Override driver entry point, install the Platform Driver Override Protocol

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM Driver,
                                     or UEFI Driver exited normally.
  @retval  EFI_INCOMPATIBLE_VERSION  _gUefiDriverRevision is greater than SystemTable->Hdr.Revision.
  @retval  Other                     Return value from ProcessModuleEntryPointList().
**/
EFI_STATUS
EFIAPI
PlatformDriverOverrideEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mEnvironmentVariableRead = FALSE;
  mCallerImageHandle = ImageHandle;
  InitializeListHead (&mMappingDataBase);
  return InstallPlatformDriverOverrideProtocol (&gPlatformDriverOverride);
}


/**
  Retrieves the image handle of the platform override driver for a controller in the system.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImageHandle      On input, a pointer to the previous driver image
                                 handle returned by GetDriver().  On output, a
                                 pointer to the next driver image handle. Passing
                                 in a NULL,  will return the first driver image
                                 handle for ControllerHandle.

  @retval EFI_SUCCESS            The driver override for ControllerHandle was
                                 returned in DriverImageHandle.
  @retval EFI_NOT_FOUND          A driver override for ControllerHandle was not
                                 found.
  @retval EFI_INVALID_PARAMETER  The handle specified by ControllerHandle is not a
                                 valid handle. DriverImageHandle is not a handle
                                 that was returned on a previous  call to
                                 GetDriver().

**/
EFI_STATUS
EFIAPI
GetDriver (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     * DriverImageHandle
  )
{
  EFI_STATUS  Status;
  //
  // Check that ControllerHandle is a valid handle
  //
  if (ControllerHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the environment variable(s) that contain the override mappings from Controller Device Path to
  // a set of Driver Device Paths, and  initialize in memory database of the overrides that map Controller
  // Device Paths to an ordered set of Driver Device Paths and Driver Handles. This action is only performed
  // once and finished in first call.
  //
  if (!mEnvironmentVariableRead) {
    mEnvironmentVariableRead = TRUE;

    Status = InitOverridesMapping (&mMappingDataBase);
    if (Status == EFI_NOT_FOUND) {
      InitializeListHead (&mMappingDataBase);
      return EFI_NOT_FOUND;
    } else if (Status == EFI_VOLUME_CORRUPTED){
      DEBUG ((DEBUG_ERROR, "Platform Driver Override Variable is corrupt\n"));
      //
      // The environment variable(s) that contain the override mappings from Controller Device Path to
      //  a set of Driver Device Paths is corrupted,  platform code can use LibDeleteOverridesVariables to
      //  delete all orverride variables as a policy. Here can be IBV/OEM customized.
      //

      //LibDeleteOverridesVariables();
      InitializeListHead (&mMappingDataBase);
      return EFI_NOT_FOUND;
    } else if (EFI_ERROR (Status)){
      InitializeListHead (&mMappingDataBase);
      return EFI_NOT_FOUND;
    }
  }
  //
  // if the environment variable does not exist or the variable appears to be corrupt, just return not found
  //
  if (IsListEmpty (&mMappingDataBase)) {
    return EFI_NOT_FOUND;
  }

  return GetDriverFromMapping (
            This,
            ControllerHandle,
            DriverImageHandle,
            &mMappingDataBase,
            mCallerImageHandle
            );

}


/**
  For the use of the ControllerHandle parameter in the GetDriverPath()
  But this API is very difficult to use, so not support.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImagePath        The device path for this Image.
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetDriverPath (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  )
{
  return EFI_UNSUPPORTED;
}


/**
  For the use of the ControllerHandle parameter in the DriverLoaded()
  But this API is very difficult to use, so not support.

  @param  This                   A pointer to the
                                 EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle       The device handle of the controller to check if a
                                 driver override exists.
  @param  DriverImagePath        The device path for this Image.
  @param  DriverImageHandle      On input, a pointer to the previous driver image
                                 handle returned by GetDriver().  On output, a
                                 pointer to the next driver image handle. Passing
                                 in a NULL,  will return the first driver image
                                 handle for ControllerHandle.
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
DriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          * This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       * DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  )
{
  return EFI_UNSUPPORTED;
}
