/** @file
  Support for the optional VariableSmmRuntimeDxe hook protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <Guid/EventGroup.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include "VariableSmmRuntimeDxeHookInternal.h"

STATIC EDKII_VARIABLE_PRE_SET_VARIABLE   mPreSetVariable;
STATIC EDKII_VARIABLE_POST_SET_VARIABLE  mPostSetVariable;
STATIC EFI_EVENT                         mVariableRuntimeHookNotifyEvent;
STATIC EFI_EVENT                         mVariableRuntimeHookEndOfDxeEvent;
STATIC VOID                              *mVariableRuntimeHookRegistration;
STATIC BOOLEAN                           mVariableRuntimeHookDiscoveryEnded;

/**
  Caches the first valid variable runtime hook provider.

  @param[in] Protocol  Variable runtime hook protocol instance.

  @retval TRUE   The provider was valid and its callbacks were cached.
  @retval FALSE  The provider was invalid or provider discovery has ended.
**/
STATIC
BOOLEAN
CacheVariableRuntimeHook (
  IN EDKII_VARIABLE_RUNTIME_HOOK_PROTOCOL  *Protocol
  )
{
  if (mVariableRuntimeHookDiscoveryEnded ||
      (mPreSetVariable != NULL) ||
      (Protocol == NULL) ||
      (Protocol->PreSetVariable == NULL) ||
      (Protocol->PostSetVariable == NULL))
  {
    return FALSE;
  }

  mPreSetVariable  = Protocol->PreSetVariable;
  mPostSetVariable = Protocol->PostSetVariable;
  return TRUE;
}

/**
  Closes an event and clears its cached event handle.

  @param[in,out] Event  Address of the cached event handle.
**/
STATIC
VOID
CloseVariableRuntimeHookEvent (
  IN OUT EFI_EVENT  *Event
  )
{
  EFI_EVENT  EventToClose;

  EventToClose = *Event;
  *Event       = NULL;
  if (EventToClose != NULL) {
    gBS->CloseEvent (EventToClose);
  }
}

/**
  Closes the variable runtime hook protocol notification event and clears its
  registration key.
**/
STATIC
VOID
CloseVariableRuntimeHookNotifyEvent (
  VOID
  )
{
  CloseVariableRuntimeHookEvent (&mVariableRuntimeHookNotifyEvent);
  mVariableRuntimeHookRegistration = NULL;
}

/**
  Notification function invoked when the variable runtime hook protocol is
  installed.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function context.
**/
STATIC
VOID
EFIAPI
VariableRuntimeHookInstalled (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                            Status;
  EDKII_VARIABLE_RUNTIME_HOOK_PROTOCOL  *Protocol;

  if (mVariableRuntimeHookDiscoveryEnded || (mPreSetVariable != NULL)) {
    return;
  }

  do {
    Protocol = NULL;
    Status   = gBS->LocateProtocol (
                      &gEdkiiVariableSmmRuntimeDxeHookProtocolGuid,
                      mVariableRuntimeHookRegistration,
                      (VOID **)&Protocol
                      );
    if (EFI_ERROR (Status)) {
      return;
    }
  } while (!CacheVariableRuntimeHook (Protocol));

  CloseVariableRuntimeHookNotifyEvent ();
}

/**
  Stops variable runtime hook provider discovery and closes discovery events.
**/
VOID
VariableRuntimeHookStopDiscovery (
  VOID
  )
{
  mVariableRuntimeHookDiscoveryEnded = TRUE;
  CloseVariableRuntimeHookNotifyEvent ();
  CloseVariableRuntimeHookEvent (&mVariableRuntimeHookEndOfDxeEvent);
}

/**
  EndOfDxe notification function that accepts any pending provider and then
  stops provider discovery.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function context.
**/
STATIC
VOID
EFIAPI
VariableRuntimeHookEndOfDxe (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if ((mPreSetVariable == NULL) && (mVariableRuntimeHookNotifyEvent != NULL)) {
    VariableRuntimeHookInstalled (mVariableRuntimeHookNotifyEvent, NULL);
  }

  VariableRuntimeHookStopDiscovery ();
}

/**
  Discovers the first valid variable runtime hook provider and registers for
  notification if a provider is not yet available.

  @retval EFI_SUCCESS  Discovery was initialized or a provider was cached.
  @retval Others       Event or protocol notification registration failed.
**/
EFI_STATUS
VariableRuntimeHookInitialize (
  VOID
  )
{
  EFI_STATUS                            Status;
  EDKII_VARIABLE_RUNTIME_HOOK_PROTOCOL  *Protocol;

  if (mVariableRuntimeHookDiscoveryEnded || (mPreSetVariable != NULL)) {
    return EFI_SUCCESS;
  }

  Protocol = NULL;
  Status   = gBS->LocateProtocol (
                    &gEdkiiVariableSmmRuntimeDxeHookProtocolGuid,
                    NULL,
                    (VOID **)&Protocol
                    );
  if (!EFI_ERROR (Status) && CacheVariableRuntimeHook (Protocol)) {
    return EFI_SUCCESS;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  VariableRuntimeHookEndOfDxe,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &mVariableRuntimeHookEndOfDxeEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  VariableRuntimeHookInstalled,
                  NULL,
                  &mVariableRuntimeHookNotifyEvent
                  );
  if (EFI_ERROR (Status)) {
    VariableRuntimeHookStopDiscovery ();
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEdkiiVariableSmmRuntimeDxeHookProtocolGuid,
                  mVariableRuntimeHookNotifyEvent,
                  &mVariableRuntimeHookRegistration
                  );
  if (EFI_ERROR (Status)) {
    VariableRuntimeHookStopDiscovery ();
    return Status;
  }

  //
  // Process providers installed between the initial lookup and registration.
  //
  VariableRuntimeHookInstalled (mVariableRuntimeHookNotifyEvent, NULL);
  return EFI_SUCCESS;
}

/**
  Converts cached variable runtime hook callback pointers to virtual addresses.
**/
VOID
VariableRuntimeHookConvertPointers (
  VOID
  )
{
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mPreSetVariable);
  EfiConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mPostSetVariable);
}

/**
  Invokes the cached pre-hook for an OS runtime SetVariable() request.

  @param[in]  VariableName  Name of the variable.
  @param[in]  VendorGuid    Variable vendor GUID.
  @param[in]  Attributes    Variable attributes.
  @param[in]  DataSize      Size of Data in bytes.
  @param[in]  Data          Variable data.
  @param[out] HookInvoked   TRUE if the pre-hook returned success.

  @retval EFI_SUCCESS  Continue with MM communication.
  @retval Others       The pre-hook rejected or deferred the operation.
**/
EFI_STATUS
VariableRuntimeHookPreSetVariable (
  IN  CONST CHAR16    *VariableName,
  IN  CONST EFI_GUID  *VendorGuid,
  IN        UINT32    Attributes,
  IN        UINTN     DataSize,
  IN  CONST VOID      *Data,
  OUT       BOOLEAN   *HookInvoked
  )
{
  EFI_STATUS  Status;

  *HookInvoked = FALSE;
  if (!EfiAtRuntime () || (mPreSetVariable == NULL)) {
    return EFI_SUCCESS;
  }

  Status = mPreSetVariable (
             VariableName,
             VendorGuid,
             Attributes,
             DataSize,
             Data
             );
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  *HookInvoked = TRUE;
  return EFI_SUCCESS;
}

/**
  Invokes the cached post-hook after MM communication completes.

  @param[in] HookInvoked        TRUE if the pre-hook returned success.
  @param[in] SetVariableStatus  Authoritative MM variable service result.
**/
VOID
VariableRuntimeHookPostSetVariable (
  IN BOOLEAN     HookInvoked,
  IN EFI_STATUS  SetVariableStatus
  )
{
  if (!HookInvoked) {
    return;
  }

  mPostSetVariable (SetVariableStatus);
}
