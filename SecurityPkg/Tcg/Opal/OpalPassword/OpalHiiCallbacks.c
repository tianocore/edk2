/** @file
  Callbacks required by the HII of the Opal UEFI Driver to help display
  Opal device information.

Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalHii.h"

/**
  Get Opal var name.
  The return Value must be freed by caller if not NULL

  @param      OpalDisk       The disk.
  @param      Prefix         The prefix string.

  @retval  The var name string.

**/
CHAR16*
OpalDriverGetOpalVarName(
  OPAL_DISK        *OpalDisk,
  const CHAR16     *Prefix
  )
{
  OPAL_DRIVER_DEVICE*          Dev;
  UINTN                        PrefixLen;
  UINTN                        NameLen;
  UINTN                        VarNameLen;
  CHAR16*                      VarName;

  Dev = DRIVER_DEVICE_FROM_OPALDISK(OpalDisk);
  if (Dev == NULL) {
    return NULL;
  }

  PrefixLen = StrLen(Prefix);

  NameLen = 0;
  if (Dev->Name16 != NULL) {
    NameLen = StrLen(Dev->Name16);
  }

  VarNameLen = PrefixLen + NameLen;

  VarName = (CHAR16*)AllocateZeroPool((VarNameLen + 1) * sizeof(CHAR16));
  if (VarName == NULL) {
    return NULL;
  }

  CopyMem(VarName, Prefix, PrefixLen * sizeof(CHAR16));
  if (Dev->Name16 != NULL) {
    CopyMem(VarName + PrefixLen, Dev->Name16, NameLen * sizeof(CHAR16));
  }
  VarName[VarNameLen] = 0;

  return VarName;
}

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
  Check whether enable feature or not.

  @retval  Return the disk number.

**/
UINT8
HiiGetNumConfigRequiredOpalDisksCB(
  VOID
  )
{
  UINT8                        NumDisks;
  UINT8                        NumLockedOpalDisks;
  OPAL_DISK                    *OpalDisk;
  UINT8                        Index;

  NumLockedOpalDisks = 0;

  NumDisks = GetDeviceCount();

  for (Index = 0; Index < NumDisks; Index++) {
    OpalDisk = HiiGetOpalDiskCB(Index);

    if (OpalDisk != NULL) {
      if (!OpalFeatureEnabled (&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature)) {
        DEBUG ((DEBUG_INFO, "Ignoring disk %u because feature is disabled or health has already been inspected\n", Index));
      } else if (OpalDeviceLocked (&OpalDisk->SupportedAttributes, &OpalDisk->LockingFeature)) {
        NumLockedOpalDisks++;
      }
    }
  }

  return NumLockedOpalDisks;
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

/**
  Returns the driver name.

  @retval Returns the driver name.

**/
CHAR16*
HiiGetDriverNameCB(
  VOID
  )
{
  return (CHAR16*)EFI_DRIVER_NAME_UNICODE;
}
