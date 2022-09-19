/** @file
  Xen Hypercall Library implementation for Intel architecture

  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2022, Citrix Systems, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/EventGroup.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/XenHypercallLib.h>

STATIC VOID  *mHyperPage;

//
// Pointer to reserved page for Xen's hypercall page.
//
extern VOID  *XenHypercallPage;

//
// Virtual Address Change Event
//
// This is needed for runtime variable access.
//
EFI_EVENT  mXenHypercallLibAddrChangeEvent = NULL;

RETURN_STATUS
EFIAPI
XenHypercallRuntimeLibConstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  XenHypercallLibInit ();

  //
  // We don't fail library construction, since that has catastrophic
  // consequences for client modules (whereas those modules may easily be
  // running on a non-Xen platform). Instead, XenHypercallIsAvailable()
  // will return FALSE.
  //
  return RETURN_SUCCESS;
}

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
  return mHyperPage != NULL;
}

//
// Interface exposed by the ASM implementation of the core hypercall
//
INTN
EFIAPI
__XenHypercall2 (
  IN     VOID  *HypercallAddr,
  IN OUT INTN  Arg1,
  IN OUT INTN  Arg2
  );

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It converts pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context.

**/
STATIC
VOID
EFIAPI
XenHypercallLibAddrChangeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = EfiConvertFunctionPointer (0, &mHyperPage);
  ASSERT_EFI_ERROR (Status);
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
  Library constructor: populate hypercall page.
**/
RETURN_STATUS
EFIAPI
XenHypercallLibInit (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      TransferReg;
  UINT32      TransferPages;
  UINT32      XenLeaf;

  XenLeaf = XenCpuidLeaf ();

  if (XenLeaf == 0) {
    return RETURN_UNSUPPORTED;
  }

  AsmCpuid (XenLeaf + 2, &TransferPages, &TransferReg, NULL, NULL);

  //
  // Only populate the first page of the hypercall even if there's more
  // than one, that is even if TransferPages > 1.
  // We don't use hypercall id > 127.
  //
  AsmWriteMsr64 (TransferReg, (UINTN)&XenHypercallPage);

  mHyperPage = &XenHypercallPage;

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  XenHypercallLibAddrChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mXenHypercallLibAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

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
  ASSERT (mHyperPage != NULL);
  //
  // Hypercall must not use code beyong the first hypercall page.
  // Only the first page is populated by XenHypercallLibInit ()
  //
  ASSERT (HypercallID < EFI_PAGE_SIZE / 32);
  if (HypercallID >= EFI_PAGE_SIZE / 32) {
    return -38; // -ENOSYS
  }

  return __XenHypercall2 ((UINT8 *)mHyperPage + HypercallID * 32, Arg1, Arg2);
}
