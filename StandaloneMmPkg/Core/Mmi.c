/** @file
  MMI management.

  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

//
// MM_HANDLER_STATE_NOTIFIER
//

//
// MM_HANDLER - used for each MM handler
//

#define MMI_ENTRY_SIGNATURE  SIGNATURE_32('m','m','i','e')

typedef struct {
  UINTN       Signature;
  LIST_ENTRY  AllEntries;  // All entries

  EFI_GUID    HandlerType; // Type of interrupt
  LIST_ENTRY  MmiHandlers; // All handlers
} MMI_ENTRY;

#define MMI_HANDLER_SIGNATURE  SIGNATURE_32('m','m','i','h')

typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;        // Link on MMI_ENTRY.MmiHandlers
  EFI_MM_HANDLER_ENTRY_POINT    Handler;     // The mm handler's entry point
  MMI_ENTRY                     *MmiEntry;
} MMI_HANDLER;

LIST_ENTRY  mRootMmiHandlerList = INITIALIZE_LIST_HEAD_VARIABLE (mRootMmiHandlerList);
LIST_ENTRY  mMmiEntryList       = INITIALIZE_LIST_HEAD_VARIABLE (mMmiEntryList);

/**
  Finds the MMI entry for the requested handler type.

  @param  HandlerType            The type of the interrupt
  @param  Create                 Create a new entry if not found

  @return MMI entry

**/
MMI_ENTRY  *
EFIAPI
MmCoreFindMmiEntry (
  IN EFI_GUID  *HandlerType,
  IN BOOLEAN   Create
  )
{
  LIST_ENTRY  *Link;
  MMI_ENTRY   *Item;
  MMI_ENTRY   *MmiEntry;

  //
  // Search the MMI entry list for the matching GUID
  //
  MmiEntry = NULL;
  for (Link = mMmiEntryList.ForwardLink;
       Link != &mMmiEntryList;
       Link = Link->ForwardLink) {

    Item = CR (Link, MMI_ENTRY, AllEntries, MMI_ENTRY_SIGNATURE);
    if (CompareGuid (&Item->HandlerType, HandlerType)) {
      //
      // This is the MMI entry
      //
      MmiEntry = Item;
      break;
    }
  }

  //
  // If the protocol entry was not found and Create is TRUE, then
  // allocate a new entry
  //
  if ((MmiEntry == NULL) && Create) {
    MmiEntry = AllocatePool (sizeof (MMI_ENTRY));
    if (MmiEntry != NULL) {
      //
      // Initialize new MMI entry structure
      //
      MmiEntry->Signature = MMI_ENTRY_SIGNATURE;
      CopyGuid ((VOID *)&MmiEntry->HandlerType, HandlerType);
      InitializeListHead (&MmiEntry->MmiHandlers);

      //
      // Add it to MMI entry list
      //
      InsertTailList (&mMmiEntryList, &MmiEntry->AllEntries);
    }
  }
  return MmiEntry;
}

/**
  Manage MMI of a particular type.

  @param  HandlerType    Points to the handler type or NULL for root MMI handlers.
  @param  Context        Points to an optional context buffer.
  @param  CommBuffer     Points to the optional communication buffer.
  @param  CommBufferSize Points to the size of the optional communication buffer.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING  Interrupt source was processed successfully but not quiesced.
  @retval EFI_INTERRUPT_PENDING              One or more MMI sources could not be quiesced.
  @retval EFI_NOT_FOUND                      Interrupt source was not handled or quiesced.
  @retval EFI_SUCCESS                        Interrupt source was handled and quiesced.

**/
EFI_STATUS
EFIAPI
MmiManage (
  IN     CONST EFI_GUID  *HandlerType,
  IN     CONST VOID      *Context         OPTIONAL,
  IN OUT VOID            *CommBuffer      OPTIONAL,
  IN OUT UINTN           *CommBufferSize  OPTIONAL
  )
{
  LIST_ENTRY   *Link;
  LIST_ENTRY   *Head;
  MMI_ENTRY    *MmiEntry;
  MMI_HANDLER  *MmiHandler;
  BOOLEAN      SuccessReturn;
  EFI_STATUS   Status;

  Status = EFI_NOT_FOUND;
  SuccessReturn = FALSE;
  if (HandlerType == NULL) {
    //
    // Root MMI handler
    //

    Head = &mRootMmiHandlerList;
  } else {
    //
    // Non-root MMI handler
    //
    MmiEntry = MmCoreFindMmiEntry ((EFI_GUID *) HandlerType, FALSE);
    if (MmiEntry == NULL) {
      //
      // There is no handler registered for this interrupt source
      //
      return Status;
    }

    Head = &MmiEntry->MmiHandlers;
  }

  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    MmiHandler = CR (Link, MMI_HANDLER, Link, MMI_HANDLER_SIGNATURE);

    Status = MmiHandler->Handler (
               (EFI_HANDLE) MmiHandler,
               Context,
               CommBuffer,
               CommBufferSize
               );

    switch (Status) {
    case EFI_INTERRUPT_PENDING:
      //
      // If a handler returns EFI_INTERRUPT_PENDING and HandlerType is not NULL then
      // no additional handlers will be processed and EFI_INTERRUPT_PENDING will be returned.
      //
      if (HandlerType != NULL) {
        return EFI_INTERRUPT_PENDING;
      }
      break;

    case EFI_SUCCESS:
      //
      // If at least one of the handlers returns EFI_SUCCESS then the function will return
      // EFI_SUCCESS. If a handler returns EFI_SUCCESS and HandlerType is not NULL then no
      // additional handlers will be processed.
      //
      if (HandlerType != NULL) {
        return EFI_SUCCESS;
      }
      SuccessReturn = TRUE;
      break;

    case EFI_WARN_INTERRUPT_SOURCE_QUIESCED:
      //
      // If at least one of the handlers returns EFI_WARN_INTERRUPT_SOURCE_QUIESCED
      // then the function will return EFI_SUCCESS.
      //
      SuccessReturn = TRUE;
      break;

    case EFI_WARN_INTERRUPT_SOURCE_PENDING:
      //
      // If all the handlers returned EFI_WARN_INTERRUPT_SOURCE_PENDING
      // then EFI_WARN_INTERRUPT_SOURCE_PENDING will be returned.
      //
      break;

    default:
      //
      // Unexpected status code returned.
      //
      ASSERT (FALSE);
      break;
    }
  }

  if (SuccessReturn) {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Registers a handler to execute within MM.

  @param  Handler        Handler service funtion pointer.
  @param  HandlerType    Points to the handler type or NULL for root MMI handlers.
  @param  DispatchHandle On return, contains a unique handle which can be used to later unregister the handler function.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Handler or DispatchHandle is NULL.

**/
EFI_STATUS
EFIAPI
MmiHandlerRegister (
  IN  EFI_MM_HANDLER_ENTRY_POINT    Handler,
  IN  CONST EFI_GUID                *HandlerType  OPTIONAL,
  OUT EFI_HANDLE                    *DispatchHandle
  )
{
  MMI_HANDLER  *MmiHandler;
  MMI_ENTRY    *MmiEntry;
  LIST_ENTRY   *List;

  if (Handler == NULL || DispatchHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MmiHandler = AllocateZeroPool (sizeof (MMI_HANDLER));
  if (MmiHandler == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MmiHandler->Signature = MMI_HANDLER_SIGNATURE;
  MmiHandler->Handler = Handler;

  if (HandlerType == NULL) {
    //
    // This is root MMI handler
    //
    MmiEntry = NULL;
    List = &mRootMmiHandlerList;
  } else {
    //
    // None root MMI handler
    //
    MmiEntry = MmCoreFindMmiEntry ((EFI_GUID *) HandlerType, TRUE);
    if (MmiEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    List = &MmiEntry->MmiHandlers;
  }

  MmiHandler->MmiEntry = MmiEntry;
  InsertTailList (List, &MmiHandler->Link);

  *DispatchHandle = (EFI_HANDLE) MmiHandler;

  return EFI_SUCCESS;
}

/**
  Unregister a handler in MM.

  @param  DispatchHandle  The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS           Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER DispatchHandle does not refer to a valid handle.

**/
EFI_STATUS
EFIAPI
MmiHandlerUnRegister (
  IN EFI_HANDLE  DispatchHandle
  )
{
  MMI_HANDLER  *MmiHandler;
  MMI_ENTRY    *MmiEntry;

  MmiHandler = (MMI_HANDLER *) DispatchHandle;

  if (MmiHandler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MmiHandler->Signature != MMI_HANDLER_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  MmiEntry = MmiHandler->MmiEntry;

  RemoveEntryList (&MmiHandler->Link);
  FreePool (MmiHandler);

  if (MmiEntry == NULL) {
    //
    // This is root MMI handler
    //
    return EFI_SUCCESS;
  }

  if (IsListEmpty (&MmiEntry->MmiHandlers)) {
    //
    // No handler registered for this interrupt now, remove the MMI_ENTRY
    //
    RemoveEntryList (&MmiEntry->AllEntries);

    FreePool (MmiEntry);
  }

  return EFI_SUCCESS;
}
