/** @file
  Xen Hypercall Library implementation for Intel architecture

Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Guid/XenInfo.h>

STATIC VOID    *HyperPage;

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
  return HyperPage != NULL;
}

//
// Interface exposed by the ASM implementation of the core hypercall
//
INTN
EFIAPI
__XenHypercall2 (
  IN     VOID *HypercallAddr,
  IN OUT INTN Arg1,
  IN OUT INTN Arg2
  );

/**
  Library constructor: retrieves the Hyperpage address
  from the gEfiXenInfoGuid HOB
**/

RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  )
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  EFI_XEN_INFO        *XenInfo;

  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    //
    // We don't fail library construction, since that has catastrophic
    // consequences for client modules (whereas those modules may easily be
    // running on a non-Xen platform). Instead, XenHypercallIsAvailable() above
    // will return FALSE.
    //
    return RETURN_SUCCESS;
  }
  XenInfo = (EFI_XEN_INFO *) GET_GUID_HOB_DATA (GuidHob);
  HyperPage = XenInfo->HyperPages;
  return RETURN_SUCCESS;
}

/**
  This function will put the two arguments in the right place (registers) and
  invoke the hypercall identified by HypercallID.

  @param HypercallID    The symbolic ID of the hypercall to be invoked
  @param Arg1           First argument.
  @param Arg2           Second argument.

  @return   Return 0 if success otherwise it return an errno.
**/
INTN
EFIAPI
XenHypercall2 (
  IN     UINTN  HypercallID,
  IN OUT INTN   Arg1,
  IN OUT INTN   Arg2
  )
{
  ASSERT (HyperPage != NULL);

  return __XenHypercall2 ((UINT8*)HyperPage + HypercallID * 32, Arg1, Arg2);
}
