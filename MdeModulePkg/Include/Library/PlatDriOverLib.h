/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatDriOverLib.h

Abstract:


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
  @retval Other                    Returned by InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
InstallPlatformDriverOverrideProtocol (
  EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL *gPlatformDriverOverride
  );

/**
  Free all the mapping database memory resource and initialize the mapping list entry

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
  VariableLayout{
  //
  // NotEnd indicate whether the variable is the last one, and has no subsequent variable need to load.
  // Each variable has MaximumVariableSize limitation, so  we maybe need multi variables to store
  // large mapping infos.
  // The variable(s) name rule is PlatDriOver, PlatDriOver1, PlatDriOver2, ....
  //
  UINT32                         NotEnd;
  //
  // The entry which contains the mapping that Controller Device Path to a set of Driver Device Paths
  // There are often multi mapping entries in a variable.
  //
  UINT32                         SIGNATURE;            //EFI_SIGNATURE_32('p','d','o','i')
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  UINT32                         SIGNATURE;
  UINT32                         DriverNum;
  EFI_DEVICE_PATH_PROTOCOL       ControllerDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  EFI_DEVICE_PATH_PROTOCOL       DriverDevicePath[];
  ......
  }
  typedef struct _PLATFORM_OVERRIDE_ITEM{
  UINTN                          Signature;                  //EFI_SIGNATURE_32('p','d','o','i')
  LIST_ENTRY                     Link;
  UINT32                         DriverInfoNum;
  EFI_DEVICE_PATH_PROTOCOL       *ControllerDevicePath;
  LIST_ENTRY                     DriverInfoList;         //DRIVER_IMAGE_INFO List
  } PLATFORM_OVERRIDE_ITEM;
  typedef struct _DRIVER_IMAGE_INFO{
  UINTN                          Signature;                  //EFI_SIGNATURE_32('p','d','i','i')
  LIST_ENTRY                     Link;
  EFI_HANDLE                     ImageHandle;
  EFI_DEVICE_PATH_PROTOCOL       *DriverImagePath;
  BOOLEAN                        UnLoadable;
  BOOLEAN                        UnStartable;
  } DRIVER_IMAGE_INFO;

  @param  MappingDataBase          Mapping database list entry pointer

  @retval EFI_INVALID_PARAMETER    MappingDataBase pointer is null
  @retval EFI_NOT_FOUND            Cannot find the 'PlatDriOver' NV variable
  @retval EFI_VOLUME_CORRUPTED     The found NV variable is corrupted
  @retval EFI_SUCCESS              Create the mapping database in memory
                                   successfully

**/
EFI_STATUS
EFIAPI
InitOverridesMapping (
  OUT  LIST_ENTRY            *MappingDataBase
  );

/**
  Save the memory mapping database into NV environment variable(s)

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

  @param  This                     A pointer to the
                                   EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL instance.
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

EFI_STATUS
EFIAPI
DeleteOverridesVariables (
  VOID
  );

/**
  Check mapping database whether already has the  mapping info which
  records the input Controller to input DriverImage.
  If has, the controller's total override driver number and input DriverImage's order number is return.

  @param  ControllerDevicePath     The controller device path need to add a
                                   override driver image item
  @param  DriverImageDevicePath    The driver image device path need to be insert
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverInfoNum            the controller's total override driver number
  @param  DriverImageNO            The inserted order number

  @return EFI_INVALID_PARAMETER
  @return EFI_NOT_FOUND
  @return EFI_SUCCESS

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
  @param  DriverImageNO            The inserted order number

  @return EFI_INVALID_PARAMETER
  @return EFI_ALREADY_STARTED
  @return EFI_SUCCESS

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

  @param  ControllerDevicePath     The controller device path need to add a
                                   override driver image item
  @param  DriverImageDevicePath    The driver image device path need to be insert
  @param  MappingDataBase          Mapping database list entry pointer
  @param  DriverImageNO            The inserted order number

  @return EFI_INVALID_PARAMETER
  @return EFI_NOT_FOUND
  @return EFI_SUCCESS

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

  @param  Image          Image handle

  @return Pointer into the Binding Protocol interface

**/
EFI_DRIVER_BINDING_PROTOCOL *
EFIAPI
GetBindingProtocolFromImageHandle (
  IN  EFI_HANDLE   ImageHandle,
  OUT EFI_HANDLE   *BindingHandle
  );

#endif
