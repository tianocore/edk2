/** @file
  Get information about Xen

  This library simply allow to find out if OVMF is running under Xen and
  allow to get more information when it is the case.

  Copyright (c) 2019, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _XEN_PLATFORM_LIB_H_
#define _XEN_PLATFORM_LIB_H_

#include <Guid/XenInfo.h>

/**
  This function detects if OVMF is running on Xen.

  @retval TRUE    OVMF is running on Xen
  @retval FALSE   Xen has not been detected
**/
BOOLEAN
EFIAPI
XenDetected (
  VOID
  );

/**
  This function detect if OVMF have started via the PVH entry point.

  @retval TRUE  PVH entry point as been used
  @retval FALSE OVMF have started via the HVM route
**/
BOOLEAN
EFIAPI
XenPvhDetected (
  VOID
  );

/**
  This function return a pointer to the XenInfo HOB.

  @return  XenInfo pointer or NULL if not available
**/
EFI_XEN_INFO *
EFIAPI
XenGetInfoHOB (
  VOID
  );

#endif
