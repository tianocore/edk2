/** @file
  Xen Hypercall Library implementation for Intel architecture

Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/CpuLib.h>

INTN            UseVmmCall = -1;
static BOOLEAN  HypercallAvail;

//
// Interface exposed by the ASM implementation of the core hypercall
//
INTN
EFIAPI
__XenVmmcall2 (
  IN     INTN  HypercallNum,
  IN OUT INTN  Arg1,
  IN OUT INTN  Arg2
  );

INTN
EFIAPI
__XenVmcall2 (
  IN     INTN  HypercallNum,
  IN OUT INTN  Arg1,
  IN OUT INTN  Arg2
  );

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
  return HypercallAvail;
}

/**
  Library constructor: Check for gEfiXenInfoGuid HOB
**/
RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;
  CHAR8              sig[13];

  GuidHob = GetFirstGuidHob (&gEfiXenInfoGuid);
  if (GuidHob == NULL) {
    return RETURN_NOT_FOUND;
  }

  sig[12] = '\0';
  AsmCpuid (
    0,
    NULL,
    (UINT32 *)&sig[0],
    (UINT32 *)&sig[8],
    (UINT32 *)&sig[4]
    );

  DEBUG ((DEBUG_ERROR, "Detected CPU \"%12a\"\n", sig));

  if ((AsciiStrCmp ("AuthenticAMD", sig) == 0) ||
      (AsciiStrCmp ("HygonGenuine", sig) == 0))
  {
    UseVmmCall = TRUE;
  } else if ((AsciiStrCmp ("GenuineIntel", sig) == 0) ||
             (AsciiStrCmp ("CentaurHauls", sig) == 0) ||
             (AsciiStrCmp ("  Shanghai  ", sig) == 0))
  {
    UseVmmCall = FALSE;
  } else {
    return RETURN_NOT_FOUND;
  }

  HypercallAvail = TRUE;

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
  if (UseVmmCall) {
    return __XenVmmcall2 (HypercallID, Arg1, Arg2);
  } else {
    return __XenVmcall2 (HypercallID, Arg1, Arg2);
  }
}
