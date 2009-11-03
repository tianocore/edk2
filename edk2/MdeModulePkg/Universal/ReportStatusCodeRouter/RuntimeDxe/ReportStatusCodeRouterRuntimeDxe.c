/** @file
  Report Status Code Router Driver which produces Report Stataus Code Handler Protocol
  and Status Code Runtime Protocol.

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ReportStatusCodeRouterRuntimeDxe.h"

EFI_HANDLE   mHandle                    = NULL;
LIST_ENTRY   mCallbackListHead          = INITIALIZE_LIST_HEAD_VARIABLE (mCallbackListHead);
EFI_EVENT    mVirtualAddressChangeEvent = NULL;

//
// Report operation nest status.
// If it is set, then the report operation has nested.
//
UINT32       mStatusCodeNestStatus = 0;

EFI_STATUS_CODE_PROTOCOL  mStatusCodeProtocol  = {
  ReportDispatcher
};

EFI_RSC_HANDLER_PROTOCOL  mRscHandlerProtocol = {
  Register,
  Unregister
  };

/**
  Register the callback function for ReportStatusCode() notification.
  
  When this function is called the function pointer is added to an internal list and any future calls to
  ReportStatusCode() will be forwarded to the Callback function. During the bootservices,
  this is the callback for which this service can be invoked. The report status code router
  will create an event such that the callback function is only invoked at the TPL for which it was
  registered. The entity that registers for the callback should also register for an event upon
  generation of exit boot services and invoke the unregister service.
  If the handler does not have a TPL dependency, it should register for a callback at TPL high. The
  router infrastructure will support making callbacks at runtime, but the caller for runtime invocation
  must meet the following criteria:
  1. must be a runtime driver type so that its memory is not reclaimed
  2. not unregister at exit boot services so that the router will still have its callback address
  3. the caller must be self-contained (eg. Not call out into any boot-service interfaces) and be
  runtime safe, in general.
  
  @param[in] Callback   A pointer to a function of type EFI_RSC_HANDLER_CALLBACK that is called when
                        a call to ReportStatusCode() occurs.
  @param[in] Tpl        TPL at which callback can be safely invoked.   
  
  @retval  EFI_SUCCESS              Function was successfully registered.
  @retval  EFI_INVALID_PARAMETER    The callback function was NULL.
  @retval  EFI_OUT_OF_RESOURCES     The internal buffer ran out of space. No more functions can be
                                    registered.
  @retval  EFI_ALREADY_STARTED      The function was already registered. It can't be registered again.

**/
EFI_STATUS
EFIAPI
Register (
  IN EFI_RSC_HANDLER_CALLBACK   Callback,
  IN EFI_TPL                    Tpl
  )
{
  LIST_ENTRY                    *Link;
  RSC_HANDLER_CALLBACK_ENTRY    *CallbackEntry;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, RSC_HANDLER_CALLBACK_ENTRY, Node, RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
    if (CallbackEntry->RscHandlerCallback == Callback) {
      //
      // If the function was already registered. It can't be registered again.
      //
      return EFI_ALREADY_STARTED;
    }
  }

  CallbackEntry = AllocatePool (sizeof (RSC_HANDLER_CALLBACK_ENTRY));
  ASSERT (CallbackEntry != NULL);

  CallbackEntry->Signature          = RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE;
  CallbackEntry->RscHandlerCallback = Callback;
  CallbackEntry->Tpl                = Tpl;

  InsertTailList (&mCallbackListHead, &CallbackEntry->Node);

  return EFI_SUCCESS;
}

/**
  Remove a previously registered callback function from the notification list.
  
  A callback function must be unregistered before it is deallocated. It is important that any registered
  callbacks that are not runtime complaint be unregistered when ExitBootServices() is called.
  
  @param[in]  Callback  A pointer to a function of type EFI_RSC_HANDLER_CALLBACK that is to be
                        unregistered.
                        
  @retval EFI_SUCCESS           The function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER The callback function was NULL.
  @retval EFI_NOT_FOUND         The callback function was not found to be unregistered.

**/
EFI_STATUS
EFIAPI
Unregister (
  IN EFI_RSC_HANDLER_CALLBACK Callback
  )
{
  LIST_ENTRY                    *Link;
  RSC_HANDLER_CALLBACK_ENTRY    *CallbackEntry;

  if (Callback == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, RSC_HANDLER_CALLBACK_ENTRY, Node, RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
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
  Event callback function to invoke status code handler in list.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
RscHandlerNotification (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  RSC_EVENT_CONTEXT  *RscContext;

  RscContext = (RSC_EVENT_CONTEXT *) Context;

  RscContext->RscHandlerCallback (
                RscContext->Type,
                RscContext->Value,
                RscContext->Instance,
                RscContext->CallerId,
                RscContext->Data
                );
}

/**
  Provides an interface that a software module can call to report a status code.

  @param  Type             Indicates the type of status code being reported.
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
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  )
{
  LIST_ENTRY                    *Link;
  RSC_HANDLER_CALLBACK_ENTRY    *CallbackEntry;
  RSC_EVENT_CONTEXT             Context;
  EFI_EVENT                     Event;
  EFI_STATUS                    Status;

  //
  // Use atom operation to avoid the reentant of report.
  // If current status is not zero, then the function is reentrancy.
  //
  if (InterlockedCompareExchange32 (&mStatusCodeNestStatus, 0, 1) == 1) {
    return EFI_DEVICE_ERROR;
  }

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, RSC_HANDLER_CALLBACK_ENTRY, Node, RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);

    if ((CallbackEntry->Tpl == TPL_HIGH_LEVEL) || EfiAtRuntime ()) {
      CallbackEntry->RscHandlerCallback (
                       Type,
                       Value,
                       Instance,
                       CallerId,
                       Data
                       );
      continue;
    }

    Context.RscHandlerCallback = CallbackEntry->RscHandlerCallback;
    Context.Type               = Type;
    Context.Value              = Value;
    Context.Instance           = Instance;
    Context.CallerId           = CallerId;
    Context.Data               = Data;

    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    CallbackEntry->Tpl,
                    RscHandlerNotification,
                    &Context,
                    &Event
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->SignalEvent (Event);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->CloseEvent (Event);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Restore the nest status of report
  //
  InterlockedCompareExchange32 (&mStatusCodeNestStatus, 1, 0);

  return EFI_SUCCESS;
}

/**
  Virtual address change notification call back. It converts global pointer
  to virtual address.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
VirtualAddressChangeCallBack (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS					Status;
  LIST_ENTRY                    *Link;
  RSC_HANDLER_CALLBACK_ENTRY    *CallbackEntry;

  for (Link = GetFirstNode (&mCallbackListHead); !IsNull (&mCallbackListHead, Link); Link = GetNextNode (&mCallbackListHead, Link)) {
    CallbackEntry = CR (Link, RSC_HANDLER_CALLBACK_ENTRY, Node, RSC_HANDLER_CALLBACK_ENTRY_SIGNATURE);
    Status = EfiConvertFunctionPointer (0, (VOID **) &CallbackEntry->RscHandlerCallback);
    ASSERT_EFI_ERROR (Status);
  }

  Status = EfiConvertList (
             0,
             &mCallbackListHead
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Entry point of Generic Status Code Driver.

  This function is the entry point of this Generic Status Code Driver.
  It installs eport Stataus Code Handler Protocol  and Status Code Runtime Protocol,
  and registers event for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
GenericStatusCodeRuntimeDxeEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS     Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiRscHandlerProtocolGuid,
                  &mRscHandlerProtocol,
                  &gEfiStatusCodeRuntimeProtocolGuid,
                  &mStatusCodeProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtualAddressChangeCallBack,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
