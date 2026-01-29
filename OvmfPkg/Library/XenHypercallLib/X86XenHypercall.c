/** @file
  Xen Hypercall Library implementation for Intel architecture

Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/CpuLib.h>

static INTN     mUseVmmCall = -1;
static BOOLEAN  mHypercallAvail;

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
  return mHypercallAvail;
}

STATIC
UINT32
XenCpuidLeaf (
  VOID
  )
{
  UINT8   Signature[13];
  UINT32  XenLeaf;

  Signature[12] = '\0';
  for (XenLeaf = 0x40000000; XenLeaf < 0x40010000; XenLeaf += 0x100) {
    AsmCpuid (
      XenLeaf,
      NULL,
      (UINT32 *)&Signature[0],
      (UINT32 *)&Signature[4],
      (UINT32 *)&Signature[8]
      );

    if (!AsciiStrCmp ((CHAR8 *)Signature, "XenVMMXenVMM")) {
      return XenLeaf;
    }
  }

  return 0;
}

/**
  Library constructor: Check for Xen leaf in CPUID
**/
RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  )
{
  UINT32  XenLeaf;
  CHAR8   sig[13];

  XenLeaf = XenCpuidLeaf ();
  if (XenLeaf == 0) {
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

  DEBUG ((DEBUG_INFO, "Detected CPU \"%12a\"\n", sig));

  if ((AsciiStrCmp ("AuthenticAMD", sig) == 0) ||
      (AsciiStrCmp ("HygonGenuine", sig) == 0))
  {
    mUseVmmCall = TRUE;
  } else if ((AsciiStrCmp ("GenuineIntel", sig) == 0) ||
             (AsciiStrCmp ("CentaurHauls", sig) == 0) ||
             (AsciiStrCmp ("  Shanghai  ", sig) == 0))
  {
    mUseVmmCall = FALSE;
  } else {
    DEBUG ((DEBUG_ERROR, "Unsupported CPU vendor\n"));
    return RETURN_NOT_FOUND;
  }

  mHypercallAvail = TRUE;

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
  if (mUseVmmCall) {
    return __XenVmmcall2 (HypercallID, Arg1, Arg2);
  } else {
    return __XenVmcall2 (HypercallID, Arg1, Arg2);
  }
}
