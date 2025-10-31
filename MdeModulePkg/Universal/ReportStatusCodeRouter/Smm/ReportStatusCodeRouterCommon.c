/** @file
  Report Status Code Router Driver which produces SMM Report Stataus Code Handler Protocol
  and SMM Status Code Protocol.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "ReportStatusCodeRouterCommon.h"

LIST_ENTRY  mCallbackListHead = INITIALIZE_LIST_HEAD_VARIABLE (mCallbackListHead);

//
// Report operation nest status.
// If it is set, then the report operation has nested.
//
UINT32  mStatusCodeNestStatus = 0;

EFI_MM_STATUS_CODE_PROTOCOL  mSmmStatusCodeProtocol = {
  ReportDispatcher
};

EFI_MM_RSC_HANDLER_PROTOCOL  mSmmRscHandlerProtocol = {
  Register,
  Unregister
};

/**
  Register the callback function for ReportStatusCode() notification.

  When this function is called the function pointer is added to an internal list and any future calls to
  ReportStatusCode() will be forwarded to the Callback function.

  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is called
                                when a call to ReportStatusCode() occurs.

  @retval EFI_SUCCESS           Function was successfully registered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_OUT_OF_RESOURCES  The internal buffer ran out of space. No more functions can be
                                registered.
  @retval EFI_ALREADY_STARTED   The function was already registered. It can't be registered again.

**/
EFI_STATUS
EFIAPI
Register (
  IN EFI_MM_RSC_HANDLER_CALLBACK  Callback
  )
{
  LIST_ENTRY                     *Link;
  MM_RSC_HANDLER_CALLBACK_ENTRY  *CallbackEntry;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, MM_RSC_HANDLER_CALLBACK_ENTRY, Node, MM_RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
    if (CallbackEntry->RscHandlerCallback == Callback) {
      //
      // If the function was already registered. It can't be registered again.
      //
      return EFI_ALREADY_STARTED;
    }
  }

  CallbackEntry = (MM_RSC_HANDLER_CALLBACK_ENTRY *)AllocatePool (sizeof (MM_RSC_HANDLER_CALLBACK_ENTRY));
  if (CallbackEntry == NULL) {
    ASSERT (CallbackEntry != NULL);
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackEntry->Signature          = MM_RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE;
  CallbackEntry->RscHandlerCallback = Callback;

  InsertTailList (&mCallbackListHead, &CallbackEntry->Node);

  return EFI_SUCCESS;
}

/**
  Remove a previously registered callback function from the notification list.

  ReportStatusCode() messages will no longer be forwarded to the Callback function.

  @param[in] Callback           A pointer to a function of type EFI_PEI_RSC_HANDLER_CALLBACK that is to be
                                unregistered.

  @retval EFI_SUCCESS           The function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_NOT_FOUND         The callback function was not found to be unregistered.

**/
EFI_STATUS
EFIAPI
Unregister (
  IN EFI_MM_RSC_HANDLER_CALLBACK  Callback
  )
{
  LIST_ENTRY                     *Link;
  MM_RSC_HANDLER_CALLBACK_ENTRY  *CallbackEntry;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, MM_RSC_HANDLER_CALLBACK_ENTRY, Node, MM_RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
    if (CallbackEntry->RscHandlerCallback == Callback) {
      //
      // If the function is found in list, delete it and return.
      //
      RemoveEntryList (&CallbackEntry->Node);
      FreePool (CallbackEntry);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Provides an interface that a software module can call to report a status code.

  @param  This             EFI_MM_STATUS_CODE_PROTOCOL instance.
  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or software entity.
                           This included information about the class and subclass that is used to
                           classify the entity as well as an operation.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS           The function completed successfully
  @retval EFI_DEVICE_ERROR      The function should not be completed due to a device error.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN CONST EFI_MM_STATUS_CODE_PROTOCOL  *This,
  IN EFI_STATUS_CODE_TYPE               CodeType,
  IN EFI_STATUS_CODE_VALUE              Value,
  IN UINT32                             Instance,
  IN CONST EFI_GUID                     *CallerId,
  IN EFI_STATUS_CODE_DATA               *Data      OPTIONAL
  )
{
  LIST_ENTRY                     *Link;
  MM_RSC_HANDLER_CALLBACK_ENTRY  *CallbackEntry;

  //
  // Use atom operation to avoid the reentant of report.
  // If current status is not zero, then the function is reentrancy.
  //
  if (InterlockedCompareExchange32 (&mStatusCodeNestStatus, 0, 1) == 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link);) {
    CallbackEntry = CR (Link, MM_RSC_HANDLER_CALLBACK_ENTRY, Node, MM_RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
    //
    // The handler may remove itself, so get the next handler in advance.
    //
    Link = GetNextNode (&mCallbackListHead, Link);
    CallbackEntry->RscHandlerCallback (
                     CodeType,
                     Value,
                     Instance,
                     (EFI_GUID *)CallerId,
                     Data
                     );
  }

  //
  // Restore the nest status of report
  //
  InterlockedCompareExchange32 (&mStatusCodeNestStatus, 1, 0);

  return EFI_SUCCESS;
}

/**
  Entry point of Generic Status Code Driver.

  This function is the common entry point of MM Status Code Router.
  It produces MM Report Status Code Handler and Status Code protocol.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
GenericStatusCodeCommonEntry (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Handle = NULL;

  //
  // Install SmmRscHandler Protocol
  //
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gEfiMmRscHandlerProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmRscHandlerProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  //
  // Install SmmStatusCode Protocol
  //
  Status = gMmst->MmInstallProtocolInterface (
                    &Handle,
                    &gEfiMmStatusCodeProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mSmmStatusCodeProtocol
                    );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
