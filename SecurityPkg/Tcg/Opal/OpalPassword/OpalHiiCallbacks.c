/** @file
  Callbacks required by the HII of the Opal UEFI Driver to help display
  Opal device information.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalHii.h"

/**
  Get the driver image handle.

  @retval  the driver image handle.

**/
EFI_HANDLE
HiiGetDriverImageHandleCB(
  VOID
  )
{
  return gImageHandle;
}

/**
  Returns the opaque pointer to a physical disk context.

  @param  DiskIndex       Input the disk index.

  @retval The device pointer.

**/
VOID *
HiiGetDiskContextCB(
  UINT8 DiskIndex
  )
{
  OPAL_DRIVER_DEVICE*                Dev;
  UINT8                              CurrentDisk;

  Dev = OpalDriverGetDeviceList();
  CurrentDisk = 0;

  if (DiskIndex >= GetDeviceCount()) {
    return NULL;
  }

  while (Dev != NULL) {
    if (CurrentDisk == DiskIndex) {
      return Dev;
    } else {
      Dev = Dev->Next;
      CurrentDisk++;
    }
  }

  return NULL;
}

/**
  Returns the opaque pointer to a physical disk context.

  @param  DiskIndex       Input the disk index.

  @retval The device pointer.

**/
OPAL_DISK*
HiiGetOpalDiskCB(
  UINT8 DiskIndex
  )
{
  VOID                           *Ctx;
  OPAL_DRIVER_DEVICE             *Tmp;

  Ctx = HiiGetDiskContextCB (DiskIndex);

  if (Ctx == NULL) {
    return NULL;
  }

  Tmp = (OPAL_DRIVER_DEVICE*) Ctx;

  return &Tmp->OpalDisk;
}

/**
  Returns the disk name.

  @param  DiskIndex       Input the disk index.

  @retval Returns the disk name.

**/
CHAR8*
HiiDiskGetNameCB(
  UINT8 DiskIndex
  )
{
  OPAL_DRIVER_DEVICE*                Ctx;

  Ctx = (OPAL_DRIVER_DEVICE*) HiiGetDiskContextCB (DiskIndex);

  if (Ctx != NULL) {
    if (Ctx->NameZ == NULL) {
      OpalDriverGetDriverDeviceName (Ctx);
    }
    return Ctx->NameZ;
  }
  return NULL;
}