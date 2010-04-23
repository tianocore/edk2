/** @file
  Produces the Legacy Region Protocol.

  This generic implementation of the Legacy Region Protocol does not actually 
  perform any lock/unlock operations.  This module may be used on platforms 
  that do not provide HW locking of the legacy memory regions.  It can also 
  be used as a template driver for implementing the Legacy Region Protocol on
  a platform that does support HW locking of the legacy memory regions.

Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/LegacyRegion.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Sets hardware to decode or not decode a region.

  @param  This                  Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  @param  Start                 Start of region to decode.
  @param  Length                Size in bytes of the region.
  @param  On                    Decode/nondecode flag.

  @retval EFI_SUCCESS           Decode range successfully changed.

**/
EFI_STATUS
EFIAPI
LegacyRegionDecode (
  IN EFI_LEGACY_REGION_PROTOCOL  *This,
  IN UINT32                      Start,
  IN UINT32                      Length,
  IN BOOLEAN                     *On
  )
{
  return EFI_SUCCESS;
}

/**
  Sets a region to read only.

  @param  This                  Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  @param  Start                 Start of region to lock.
  @param  Length                Size in bytes of the region.
  @param  Granularity           Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS           The region was made read only.

**/
EFI_STATUS
EFIAPI
LegacyRegionLock (
  IN  EFI_LEGACY_REGION_PROTOCOL  *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  OUT UINT32                      *Granularity OPTIONAL
  )
{
  return EFI_SUCCESS;
}

/**
  Sets a region to read only and ensures that flash is locked from being
  inadvertently modified.

  @param  This                  Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  @param  Start                 Start of region to lock.
  @param  Length                Size in bytes of the region.
  @param  Granularity           Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS           The region was made read only and flash is locked.

**/
EFI_STATUS
EFIAPI
LegacyRegionBootLock (
  IN  EFI_LEGACY_REGION_PROTOCOL  *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  OUT UINT32                      *Granularity OPTIONAL
  )
{
  return EFI_SUCCESS;
}

/**
  Sets a region to read-write.

  @param  This                  Indicates the EFI_LEGACY_REGION_PROTOCOL instance
  @param  Start                 Start of region to lock.
  @param  Length                Size in bytes of the region.
  @param  Granularity           Lock attribute affects this granularity in bytes.

  @retval EFI_SUCCESS           The region was successfully made read-write.

**/
EFI_STATUS
EFIAPI
LegacyRegionUnlock (
  IN  EFI_LEGACY_REGION_PROTOCOL  *This,
  IN  UINT32                      Start,
  IN  UINT32                      Length,
  OUT UINT32                      *Granularity OPTIONAL
  )
{
  return EFI_SUCCESS;
}

//
// Module global for the handle the Legacy Region Protocol is installed
//
EFI_HANDLE                  mLegacyRegionHandle = NULL;

//
// Module global for the Legacy Region Protocol instance that is installed onto
// mLegacyRegionHandle
//
EFI_LEGACY_REGION_PROTOCOL  mLegacyRegion = {
  LegacyRegionDecode,
  LegacyRegionLock,
  LegacyRegionBootLock,
  LegacyRegionUnlock
};

/**
  The user Entry Point for module LegacyRegionDxe.  The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
LegacyRegionInstall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  
  //
  // Make sure the Legacy Region Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiLegacyRegionProtocolGuid);
  
  //
  // Install the protocol on a new handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mLegacyRegionHandle,
                  &gEfiLegacyRegionProtocolGuid, &mLegacyRegion,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
