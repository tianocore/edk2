/** @file
  Public Header file of HII library used by Opal UEFI Driver.
  Defines required callbacks of Opal HII library.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _OPAL_HII_H_
#define _OPAL_HII_H_

#include <Library/OpalPasswordSupportLib.h>
#include <OpalDriverPrivate.h>

#define DEFAULT_RESPONSE_SIZE 200

/**
  Get the driver image handle.

  @retval  the driver image handle.

**/
EFI_HANDLE
HiiGetDriverImageHandleCB(
  VOID
  );

/**
  Install the HII form and string packages.

  @retval  EFI_SUCCESS           Install all the resources success.
  @retval  EFI_OUT_OF_RESOURCES  Out of resource error.
**/
EFI_STATUS
OpalHiiAddPackages(
  VOID
  );

/**
  Check whether enable feature or not.

  @retval  Return the disk number.

**/
UINT8
HiiGetNumConfigRequiredOpalDisksCB(
  VOID
  );

/**
  Returns the driver name.

  @retval Returns the driver name.

**/
CHAR16*
HiiGetDriverNameCB(
  VOID
  );

/**
  Returns the opaque pointer to a physical disk context.

  @param  DiskIndex       Input the disk index.

  @retval The device pointer.

**/
OPAL_DISK*
HiiGetOpalDiskCB(
  UINT8 DiskIndex
  );

/**
  Returns the disk name.

  @param  DiskIndex       Input the disk index.

  @retval Returns the disk name.

**/
CHAR8*
HiiDiskGetNameCB(
  UINT8 DiskIndex
  );

/**
  Set a string Value in a form.

  @param      DestStringId   The stringid which need to update.
  @param      SrcAsciiStr    The string nned to update.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Other error occur.

**/
EFI_STATUS
HiiSetFormString(
  EFI_STRING_ID       DestStringId,
  CHAR8               *SrcAsciiStr
  );

/**
  Install the HII related resources.

  @retval  EFI_SUCCESS        Install all the resources success.
  @retval  other              Error occur when install the resources.
**/
EFI_STATUS
HiiInstall(
  VOID
  );

/**
  Uninstall the HII capability.

  @retval  EFI_SUCCESS           Uninstall all the resources success.
  @retval  others                Other errors occur when unistall the hii resource.
**/
EFI_STATUS
HiiUninstall(
  VOID
  );

/**
  Initialize the Opal disk base on the hardware info get from device.

  @param Dev                  The Opal device.

  @retval EFI_SUCESS          Initialize the device success.
  @retval EFI_DEVICE_ERROR    Get info from device failed.

**/
EFI_STATUS
OpalDiskInitialize (
  IN OPAL_DRIVER_DEVICE          *Dev
  );

#endif // _HII_H_
