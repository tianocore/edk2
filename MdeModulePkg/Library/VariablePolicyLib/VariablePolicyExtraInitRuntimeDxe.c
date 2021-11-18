/** @file -- VariablePolicyExtraInitRuntimeDxe.c
This file contains extra init and deinit routines that register and unregister
VariableAddressChange callbacks.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

extern EFI_GET_VARIABLE  mGetVariableHelper;
extern UINT8             *mPolicyTable;
STATIC BOOLEAN           mIsVirtualAddrConverted;
STATIC EFI_EVENT         mVariablePolicyLibVirtualAddressChangeEvent = NULL;

/**
  For the RuntimeDxe version of this lib, convert internal pointer addresses to virtual addresses.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
STATIC
VOID
EFIAPI
VariablePolicyLibVirtualAddressCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  gRT->ConvertPointer (0, (VOID **)&mPolicyTable);
  gRT->ConvertPointer (0, (VOID **)&mGetVariableHelper);
  mIsVirtualAddrConverted = TRUE;
}

/**
  An extra init hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with init.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraInit (
  VOID
  )
{
  return gBS->CreateEventEx (
                EVT_NOTIFY_SIGNAL,
                TPL_NOTIFY,
                VariablePolicyLibVirtualAddressCallback,
                NULL,
                &gEfiEventVirtualAddressChangeGuid,
                &mVariablePolicyLibVirtualAddressChangeEvent
                );
}

/**
  An extra deinit hook that enables the RuntimeDxe library instance to
  register VirtualAddress change callbacks. Among other things.

  @retval     EFI_SUCCESS   Everything is good. Continue with deinit.
  @retval     Others        Uh... don't continue.

**/
EFI_STATUS
VariablePolicyExtraDeinit (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
  if (mIsVirtualAddrConverted) {
    Status = gBS->CloseEvent (mVariablePolicyLibVirtualAddressChangeEvent);
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}
