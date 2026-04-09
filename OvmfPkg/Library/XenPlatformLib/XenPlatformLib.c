/** @file
  Get information about Xen

  This library simply allow to find out if OVMF is running under Xen and
  allow to get more information when it is the case.

  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/HobLib.h>
#include <Library/XenPlatformLib.h>

/**
  This function return a pointer to the XenInfo HOB.

  @return  XenInfo pointer or NULL if not available
**/
EFI_XEN_INFO *
EFIAPI
XenGetInfoHOB (
  VOID
  )
{
  EFI_HOB_GUID_TYPE    *GuidHob;
  STATIC BOOLEAN       Cached = FALSE;
  STATIC EFI_XEN_INFO  *XenInfo;

  //
  // Return the cached result for the benefit of XenDetected that can be
  // called many times.
  //
  if (Cached) {
    return XenInfo;
  }

  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    XenInfo = NULL;
  } else {
    XenInfo = (EFI_XEN_INFO *)GET_GUID_HOB_DATA (GuidHob);
  }

  Cached = TRUE;
  return XenInfo;
}

/**
  This function detects if OVMF is running on Xen.

  @retval TRUE    OVMF is running on Xen
  @retval FALSE   Xen has not been detected
**/
BOOLEAN
EFIAPI
XenDetected (
  VOID
  )
{
  return (XenGetInfoHOB () != NULL);
}

/**
  This function detect if OVMF have started via the PVH entry point.

  @retval TRUE  PVH entry point as been used
  @retval FALSE OVMF have started via the HVM route
**/
BOOLEAN
EFIAPI
XenPvhDetected (
  VOID
  )
{
  EFI_XEN_INFO  *XenInfo;

  XenInfo = XenGetInfoHOB ();
  return (XenInfo != NULL && XenInfo->RsdpPvh != NULL);
}
