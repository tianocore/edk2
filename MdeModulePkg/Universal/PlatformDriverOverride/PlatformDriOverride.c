/** @file

Copyright (c) 2007 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PlatformDriverOverrideLib.h>
#include <Protocol/PlatformDriverOverride.h>

LIST_ENTRY      mMappingDataBase = INITIALIZE_LIST_HEAD_VARIABLE (mMappingDataBase);
BOOLEAN         mEnvironmentVariableRead = FALSE;
EFI_HANDLE      mCallerImageHandle = NULL;

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
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     *DriverImageHandle
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
    if (EFI_ERROR (Status)){
      DEBUG ((DEBUG_ERROR, "The status to Get Platform Driver Override Variable is %r\n", Status));
      InitializeListHead (&mMappingDataBase);
      return EFI_NOT_FOUND;
    }
  }

  //
  // if the environment variable does not exist, just return not found
  //
  if (IsListEmpty (&mMappingDataBase)) {
    return EFI_NOT_FOUND;
  }

  return GetDriverFromMapping (
            ControllerHandle,
            DriverImageHandle,
            &mMappingDataBase,
            mCallerImageHandle
            );
}

/**
  Retrieves the device path of the platform override driver for a controller in the system.
  This driver doesn't support this API.

  @param  This                  A pointer to the EFI_PLATFORM_DRIVER_OVERRIDE_
                                PROTOCOL instance.                            
  @param  ControllerHandle      The device handle of the controller to check if a driver override
                                exists.                                                          
  @param  DriverImagePath       On input, a pointer to the previous driver device path returned by
                                GetDriverPath(). On output, a pointer to the next driver
                                device path. Passing in a pointer to NULL, will return the first
                                driver device path for ControllerHandle.
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetDriverPath (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              *This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                       **DriverImagePath
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Used to associate a driver image handle with a device path that was returned on a prior call to the
  GetDriverPath() service. This driver image handle will then be available through the               
  GetDriver() service. This driver doesn't support this API.

  @param  This                  A pointer to the EFI_PLATFORM_DRIVER_OVERRIDE_
                                PROTOCOL instance.                            
  @param  ControllerHandle      The device handle of the controller.                                                             
  @param  DriverImagePath       A pointer to the driver device path that was returned in a prior
                                call to GetDriverPath().                                                                        
  @param  DriverImageHandle     The driver image handle that was returned by LoadImage()
                                when the driver specified by DriverImagePath was loaded 
                                into memory. 
  
  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
DriverLoaded (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL          *This,
  IN EFI_HANDLE                                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DriverImagePath,
  IN EFI_HANDLE                                     DriverImageHandle
  )
{
  return EFI_UNSUPPORTED;
}

EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL mPlatformDriverOverride = {
  GetDriver,
  GetDriverPath,
  DriverLoaded
};

/**
  Platform Driver Override driver entry point, install the Platform Driver Override Protocol

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS               The DXE Driver, DXE Runtime Driver, DXE SMM Driver,
                                     or UEFI Driver exited normally.
  @retval  EFI_ALREADY_STARTED       A protocol instance has been installed. Not need install again.
**/
EFI_STATUS
EFIAPI
PlatformDriverOverrideEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HANDLE          Handle;
  EFI_STATUS          Status;
  VOID                *Instance;

  mCallerImageHandle = ImageHandle;

  //
  // According to UEFI spec, there can be at most a single instance
  // in the system of the EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL.
  // So here we check the existence.
  //
  Status = gBS->LocateProtocol (
                  &gEfiPlatformDriverOverrideProtocolGuid,
                  NULL,
                  &Instance
                  );
  //
  // If there was no error, assume there is an installation and return error
  //
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Install platform driver override protocol
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiPlatformDriverOverrideProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPlatformDriverOverride
                  );
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}
