/** @file

  This library provides basic platform driver override functions.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLAT_DRI_OVER_LIB_H_
#define _PLAT_DRI_OVER_LIB_H_

#include <PiDxe.h>
#include <Protocol/PlatformDriverOverride.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Library/BaseLib.h>

#include <VariableFormat.h>

/**
  Install the Platform Driver Override Protocol, and ensure there is only one Platform Driver Override Protocol
  in the system.

  @param  gPlatformDriverOverride  PlatformDriverOverride protocol interface which
                                   needs to be installed

  @retval EFI_ALREADY_STARTED      There has been a Platform Driver Override
                                   Protocol in the system, cannot install it again.
  @retval EFI_SUCCESS              The protocol is installed successfully.

**/
EFI_STATUS
EFIAPI
InstallPlatformDriverOverrideProtocol (
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL *gPlatformDriverOverride
  );

/**
  Free all the mapping database memory resource and initialize the mapping list entry.

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    mapping database list entry is NULL
  @retval EFI_SUCCESS              Free success

**/
EFI_STATUS
EFIAPI
FreeMappingDatabase (
  IN  OUT  LIST_ENTRY            *MappingDataBase
  );

/**
  Read the environment variable(s) that contain the override mappings from Controller Device Path to
  a set of Driver Device Paths, and create the mapping database in memory with those variable info.

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    MappingDataBase pointer is null
  @retval EFI_NOT_FOUND            Cannot find the 'PlatDriOver' NV variable
  @retval EFI_VOLUME_CORRUPTED     The found NV variable is corrupted
  @retval EFI_SUCCESS              Create the mapping database in memory successfully

**/
EFI_STATUS
EFIAPI
InitOverridesMapping (
  OUT  LIST_ENTRY            *MappingDataBase
  );

/**
  Save the memory mapping database into NV environment variable(s).

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    MappingDataBase pointer is null
  @retval EFI_SUCCESS              Save memory mapping database successfully

**/
EFI_STATUS
EFIAPI
SaveOverridesMapping (
  IN  LIST_ENTRY              *MappingDataBase
  );

/**
  Retrieves the image handle of the platform override driver for a controller in the system from the memory mapping database.

  @param  This                     A pointer to the EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
  @param  ControllerHandle         The device handle of the controller to check if
                                   a driver override exists.
  @param  DriverImageHandle        On output, a pointer to the next driver handle.
                                   Passing in a pointer to NULL, will return the
                                   first driver handle for ControllerHandle.
  @param  MappingDataBase          MappingDataBase - Mapping database list entry
                                   pointer
  @param  CallerImageHandle        The caller driver's image handle, for
                                   UpdateFvFileDevicePath use.

  @retval EFI_INVALID_PARAMETER    The handle specified by ControllerHandle is not
                                   a valid handle.  Or DriverImagePath is not a
                                   device path that was returned on a previous call
                                   to GetDriverPath().
  @retval EFI_NOT_FOUND            A driver override for ControllerHandle was not
                                   found.
  @retval EFI_UNSUPPORTED          The operation is not supported.
  @retval EFI_SUCCESS              The driver override for ControllerHandle was
                                   returned in DriverImagePath.

**/
EFI_STATUS
EFIAPI
GetDriverFromMapping (
  IN EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN     EFI_HANDLE                                     ControllerHandle,
  IN OUT EFI_HANDLE                                     * DriverImageHandle,
  IN     LIST_ENTRY                                     * MappingDataBase,
  IN     EFI_HANDLE                                     CallerImageHandle
  );

/**
  Deletes all environment variable(s) that contain the override mappings from Controller Device Path to
  a set of Driver Device Paths.

  @retval EFI_SUCCESS  Delete all variable(s) successfully.
**/
EFI_STATUS
EFIAPI
DeleteOverridesVariables (
  VOID
  );

/**
  Check mapping database whether already has the  mapping info which
  records the input Controller to input DriverImage.

  @param  ControllerDevicePath     The controller device path is to be check.
  @param  DriverImageDevicePath    The driver image device path is to be check.
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverInfoNum            the controller's total override driver number
  @param  DriverImageNO            The driver order number for the input DriverImage.
                                   If the DriverImageDevicePath is NULL, DriverImageNO is not set.

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath or MappingDataBase is NULL.
  @retval EFI_NOT_FOUND            ControllerDevicePath is not found in MappingDataBase or
                                   DriverImageDevicePath is not found in the found DriverImage Info list. 
  @retval EFI_SUCCESS              The controller's total override driver number and 
                                   input DriverImage's order number is correctly return.
**/
EFI_STATUS
EFIAPI
CheckMapping (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath,
  IN     LIST_ENTRY                                     * MappingDataBase,
  OUT    UINT32                                         *DriverInfoNum,
  OUT    UINT32                                         *DriverImageNO
  );

/**
  Insert a driver image as a controller's override driver into the mapping database.
  The driver image's order number is indicated by DriverImageNO.

  @param  ControllerDevicePath     The controller device path need to add a
                                   override driver image item
  @param  DriverImageDevicePath    The driver image device path need to be insert
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverImageNO            The inserted order number. If this number is taken, 
                                   the larger available number will be used.

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath is NULL, or DriverImageDevicePath is NULL
                                   or MappingDataBase is NULL
  @retval EFI_ALREADY_STARTED      The input Controller to input DriverImage has been 
                                   recorded into the mapping database.
  @retval EFI_SUCCESS              The Controller and DriverImage are inserted into 
                                   the mapping database successfully.

**/
EFI_STATUS
EFIAPI
InsertDriverImage (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath,
  IN     LIST_ENTRY                                     *MappingDataBase,
  IN     UINT32                                         DriverImageNO
  );

/**
  Delete a controller's override driver from the mapping database.

  @param  ControllerDevicePath     The controller device path will be deleted 
                                   when all drivers images on it are removed.
  @param  DriverImageDevicePath    The driver image device path will be delete.
                                   If NULL, all driver image will be delete.
  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    ControllerDevicePath is NULL, or MappingDataBase is NULL
  @retval EFI_NOT_FOUND            ControllerDevicePath is not found in MappingDataBase or
                                   DriverImageDevicePath is not found in the found DriverImage Info list. 
  @retval EFI_SUCCESS              Delete the specified driver successfully.

**/
EFI_STATUS
EFIAPI
DeleteDriverImage (
  IN     EFI_DEVICE_PATH_PROTOCOL                       *ControllerDevicePath,
  IN     EFI_DEVICE_PATH_PROTOCOL                       *DriverImageDevicePath,
  IN     LIST_ENTRY                                     *MappingDataBase
  );

/**
  Get the first Binding protocol which has the specific image handle

  @param  ImageHandle          The Image handle
  @param  BindingHandle        The BindingHandle of the found Driver Binding protocol.
                               If Binding protocol is not found, it is set to NULL. 

  @return                      Pointer into the Binding Protocol interface
  @retval NULL                 The paramter is not valid or the binding protocol is not found.

**/
EFI_DRIVER_BINDING_PROTOCOL *
EFIAPI
GetBindingProtocolFromImageHandle (
  IN  EFI_HANDLE   ImageHandle,
  OUT EFI_HANDLE   *BindingHandle
  );

#endif
