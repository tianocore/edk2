/** @file
  Xen Hypercall Library implementation for ARM architecture

  Copyright (C) 2015, Red Hat, Inc.
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

/**
  Check if the Xen Hypercall library is able to make calls to the Xen
  hypervisor.

  Client code should call further functions in this library only if, and after,
  this function returns TRUE.

  @retval TRUE   Hypercalls are available.
  @retval FALSE  Hypercalls are not available.
**/
BOOLEAN
EFIAPI
XenHypercallIsAvailable (
  VOID
  )
{
  return TRUE;
}

RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  )
{
  return RETURN_SUCCESS;
}
