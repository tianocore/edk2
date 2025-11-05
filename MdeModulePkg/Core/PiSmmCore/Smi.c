/** @file
  SMI management.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCore.h"

//
// mSmiManageCallingDepth is used to track the depth of recursive calls of SmiManage.
//
UINTN  mSmiManageCallingDepth = 0;

LIST_ENTRY  mSmiEntryList = INITIALIZE_LIST_HEAD_VARIABLE (mSmiEntryList);

SMI_ENTRY  mRootSmiEntry = {
  SMI_ENTRY_SIGNATURE,
  INITIALIZE_LIST_HEAD_VARIABLE (mRootSmiEntry.AllEntries),
  { 0 },
  INITIALIZE_LIST_HEAD_VARIABLE (mRootSmiEntry.SmiHandlers),
};

/**
  Finds the SMI entry for the requested handler type.

  @param  HandlerType            The type of the interrupt
  @param  Create                 Create a new entry if not found

  @return SMI entry

**/
SMI_ENTRY  *
EFIAPI
SmmCoreFindSmiEntry (
  IN EFI_GUID  *HandlerType,
  IN BOOLEAN   Create
  )
{
  LIST_ENTRY  *Link;
  SMI_ENTRY   *Item;
  SMI_ENTRY   *SmiEntry;

  //
  // Search the SMI entry list for the matching GUID
  //
  SmiEntry = NULL;
  for (Link = mSmiEntryList.ForwardLink;
       Link != &mSmiEntryList;
       Link = Link->ForwardLink)
  {
    Item = CR (Link, SMI_ENTRY, AllEntries, SMI_ENTRY_SIGNATURE);
    if (CompareGuid (&Item->HandlerType, HandlerType)) {
      //
      // This is the SMI entry
      //
      SmiEntry = Item;
      break;
    }
  }

  //
  // If the protocol entry was not found and Create is TRUE, then
  // allocate a new entry
  //
  if ((SmiEntry == NULL) && Create) {
    SmiEntry = AllocatePool (sizeof (SMI_ENTRY));
    if (SmiEntry != NULL) {
      //
      // Initialize new SMI entry structure
      //
      SmiEntry->Signature = SMI_ENTRY_SIGNATURE;
      CopyGuid ((VOID *)&SmiEntry->HandlerType, HandlerType);
      InitializeListHead (&SmiEntry->SmiHandlers);

      //
      // Add it to SMI entry list
      //
      InsertTailList (&mSmiEntryList, &SmiEntry->AllEntries);
    }
  }

  return SmiEntry;
}

/**
  Remove SmiHandler and free the memory it used.
  If SmiEntry is empty, remove SmiEntry and free the memory it used.

  @param  SmiHandler Points to SMI handler.
  @param  SmiEntry   Points to SMI Entry or NULL for root SMI handlers.

  @retval TRUE        SmiEntry is removed.
  @retval FALSE       SmiEntry is not removed.
**/
BOOLEAN
RemoveSmiHandler (
  IN SMI_HANDLER  *SmiHandler,
  IN SMI_ENTRY    *SmiEntry
  )
{
  ASSERT (SmiHandler->ToRemove);
  RemoveEntryList (&SmiHandler->Link);
  FreePool (SmiHandler);

  //
  // Remove the SMI_ENTRY if all handlers have been removed.
  //
  if (SmiEntry != NULL) {
    if (IsListEmpty (&SmiEntry->SmiHandlers)) {
      RemoveEntryList (&SmiEntry->AllEntries);
      FreePool (SmiEntry);
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Manage SMI of a particular type.

  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  Context        Points to an optional context buffer.
  @param  CommBuffer     Points to the optional communication buffer.
  @param  CommBufferSize Points to the size of the optional communication buffer.

  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING  Interrupt source was processed successfully but not quiesced.
  @retval EFI_INTERRUPT_PENDING              One or more SMI sources could not be quiesced.
  @retval EFI_NOT_FOUND                      Interrupt source was not handled or quiesced.
  @retval EFI_SUCCESS                        Interrupt source was handled and quiesced.

**/
EFI_STATUS
EFIAPI
SmiManage (
  IN     CONST EFI_GUID  *HandlerType,
  IN     CONST VOID      *Context         OPTIONAL,
  IN OUT VOID            *CommBuffer      OPTIONAL,
  IN OUT UINTN           *CommBufferSize  OPTIONAL
  )
{
  LIST_ENTRY   *Link;
  LIST_ENTRY   *Head;
  LIST_ENTRY   *EntryLink;
  SMI_ENTRY    *SmiEntry;
  SMI_HANDLER  *SmiHandler;
  EFI_STATUS   ReturnStatus;
  BOOLEAN      WillReturn;
  EFI_STATUS   Status;

  PERF_FUNCTION_BEGIN ();
  mSmiManageCallingDepth++;
  WillReturn   = FALSE;
  Status       = EFI_NOT_FOUND;
  ReturnStatus = Status;
  if (HandlerType == NULL) {
    //
    // Root SMI handler
    //
    SmiEntry = &mRootSmiEntry;
  } else {
    //
    // Non-root SMI handler
    //
    SmiEntry = SmmCoreFindSmiEntry ((EFI_GUID *)HandlerType, FALSE);
    if (SmiEntry == NULL) {
      //
      // There is no handler registered for this interrupt source
      //
      PERF_FUNCTION_END ();
      return Status;
    }
  }

  Head = &SmiEntry->SmiHandlers;

  for (Link = Head->ForwardLink; Link != Head; Link = Link->ForwardLink) {
    SmiHandler = CR (Link, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);

    Status = SmiHandler->Handler (
                           (EFI_HANDLE)SmiHandler,
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
          PERF_FUNCTION_END ();
          ReturnStatus = EFI_INTERRUPT_PENDING;
          WillReturn   = TRUE;
        } else {
          //
          // If any other handler's result sets ReturnStatus as EFI_SUCCESS, the return status
          // will be EFI_SUCCESS.
          //
          if (ReturnStatus != EFI_SUCCESS) {
            ReturnStatus = Status;
          }
        }

        break;

      case EFI_SUCCESS:
        //
        // If at least one of the handlers returns EFI_SUCCESS then the function will return
        // EFI_SUCCESS. If a handler returns EFI_SUCCESS and HandlerType is not NULL then no
        // additional handlers will be processed.
        //
        if (HandlerType != NULL) {
          PERF_FUNCTION_END ();
          WillReturn = TRUE;
        }

        ReturnStatus = EFI_SUCCESS;
        break;

      case EFI_WARN_INTERRUPT_SOURCE_QUIESCED:
        //
        // If at least one of the handlers returns EFI_WARN_INTERRUPT_SOURCE_QUIESCED
        // then the function will return EFI_SUCCESS.
        //
        ReturnStatus = EFI_SUCCESS;
        break;

      case EFI_WARN_INTERRUPT_SOURCE_PENDING:
        //
        // If all the handlers returned EFI_WARN_INTERRUPT_SOURCE_PENDING
        // then EFI_WARN_INTERRUPT_SOURCE_PENDING will be returned.
        //
        if (ReturnStatus != EFI_SUCCESS) {
          ReturnStatus = Status;
        }

        break;

      default:
        //
        // Unexpected status code returned.
        //
        ASSERT (FALSE);
        break;
    }

    if (WillReturn) {
      break;
    }
  }

  ASSERT (mSmiManageCallingDepth > 0);
  mSmiManageCallingDepth--;

  //
  // SmiHandlerUnRegister() calls from SMI handlers are deferred till this point.
  // Before returned from SmiManage, delete the SmiHandler which is
  // marked as ToRemove.
  // Note that SmiManage can be called recursively.
  //
  if (mSmiManageCallingDepth == 0) {
    //
    // Go through all SmiHandler in root SMI handlers
    //
    for ( Link = GetFirstNode (&mRootSmiEntry.SmiHandlers)
          ; !IsNull (&mRootSmiEntry.SmiHandlers, Link);
          )
    {
      //
      // SmiHandler might be removed in below, so cache the next link in Link
      //
      SmiHandler = CR (Link, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);
      Link       = GetNextNode (&mRootSmiEntry.SmiHandlers, Link);
      if (SmiHandler->ToRemove) {
        //
        // Remove SmiHandler if the ToRemove is set.
        //
        RemoveSmiHandler (SmiHandler, NULL);
      }
    }

    //
    // Go through all SmiHandler in non-root SMI handlers
    //
    for ( EntryLink = GetFirstNode (&mSmiEntryList)
          ; !IsNull (&mSmiEntryList, EntryLink);
          )
    {
      //
      // SmiEntry might be removed in below, so cache the next link in EntryLink
      //
      SmiEntry  = CR (EntryLink, SMI_ENTRY, AllEntries, SMI_ENTRY_SIGNATURE);
      EntryLink = GetNextNode (&mSmiEntryList, EntryLink);
      for ( Link = GetFirstNode (&SmiEntry->SmiHandlers)
            ; !IsNull (&SmiEntry->SmiHandlers, Link);
            )
      {
        //
        // SmiHandler might be removed in below, so cache the next link in Link
        //
        SmiHandler = CR (Link, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);
        Link       = GetNextNode (&SmiEntry->SmiHandlers, Link);
        if (SmiHandler->ToRemove) {
          if (RemoveSmiHandler (SmiHandler, SmiEntry)) {
            break;
          }
        }
      }
    }
  }

  PERF_FUNCTION_END ();
  return ReturnStatus;
}

/**
  Registers a handler to execute within SMM.

  @param  Handler        Handler service function pointer.
  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  DispatchHandle On return, contains a unique handle which can be used to later unregister the handler function.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Handler or DispatchHandle is NULL.

**/
EFI_STATUS
EFIAPI
SmiHandlerRegister (
  IN  EFI_SMM_HANDLER_ENTRY_POINT2  Handler,
  IN  CONST EFI_GUID                *HandlerType  OPTIONAL,
  OUT EFI_HANDLE                    *DispatchHandle
  )
{
  SMI_HANDLER  *SmiHandler;
  SMI_ENTRY    *SmiEntry;
  LIST_ENTRY   *List;

  if ((Handler == NULL) || (DispatchHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SmiHandler = AllocateZeroPool (sizeof (SMI_HANDLER));
  if (SmiHandler == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SmiHandler->Signature  = SMI_HANDLER_SIGNATURE;
  SmiHandler->Handler    = Handler;
  SmiHandler->CallerAddr = (UINTN)RETURN_ADDRESS (0);
  SmiHandler->ToRemove   = FALSE;

  if (HandlerType == NULL) {
    //
    // This is root SMI handler
    //
    SmiEntry = &mRootSmiEntry;
  } else {
    //
    // None root SMI handler
    //
    SmiEntry = SmmCoreFindSmiEntry ((EFI_GUID *)HandlerType, TRUE);
    if (SmiEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  List = &SmiEntry->SmiHandlers;

  SmiHandler->SmiEntry = SmiEntry;
  InsertTailList (List, &SmiHandler->Link);

  *DispatchHandle = (EFI_HANDLE)SmiHandler;

  return EFI_SUCCESS;
}

/**
  Unregister a handler in SMM.

  @param  DispatchHandle  The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS           Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER DispatchHandle does not refer to a valid handle.

**/
EFI_STATUS
EFIAPI
SmiHandlerUnRegister (
  IN EFI_HANDLE  DispatchHandle
  )
{
  SMI_HANDLER  *SmiHandler;
  SMI_ENTRY    *SmiEntry;
  LIST_ENTRY   *EntryLink;
  LIST_ENTRY   *HandlerLink;

  if (DispatchHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Look for it in root SMI handlers
  //
  SmiHandler = NULL;
  for ( HandlerLink = GetFirstNode (&mRootSmiEntry.SmiHandlers)
        ; !IsNull (&mRootSmiEntry.SmiHandlers, HandlerLink) && ((EFI_HANDLE)SmiHandler != DispatchHandle)
        ; HandlerLink = GetNextNode (&mRootSmiEntry.SmiHandlers, HandlerLink)
        )
  {
    SmiHandler = CR (HandlerLink, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);
  }

  //
  // Look for it in non-root SMI handlers
  //
  for ( EntryLink = GetFirstNode (&mSmiEntryList)
        ; !IsNull (&mSmiEntryList, EntryLink) && ((EFI_HANDLE)SmiHandler != DispatchHandle)
        ; EntryLink = GetNextNode (&mSmiEntryList, EntryLink)
        )
  {
    SmiEntry = CR (EntryLink, SMI_ENTRY, AllEntries, SMI_ENTRY_SIGNATURE);
    for ( HandlerLink = GetFirstNode (&SmiEntry->SmiHandlers)
          ; !IsNull (&SmiEntry->SmiHandlers, HandlerLink) && ((EFI_HANDLE)SmiHandler != DispatchHandle)
          ; HandlerLink = GetNextNode (&SmiEntry->SmiHandlers, HandlerLink)
          )
    {
      SmiHandler = CR (HandlerLink, SMI_HANDLER, Link, SMI_HANDLER_SIGNATURE);
    }
  }

  if (SmiHandler == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT ((EFI_HANDLE)SmiHandler == DispatchHandle);
  SmiHandler->ToRemove = TRUE;

  if (mSmiManageCallingDepth > 0) {
    //
    // This function is called from SmiManage()
    // Do not delete or remove SmiHandler or SmiEntry now.
    // SmiManage will handle it later
    //
    return EFI_SUCCESS;
  }

  SmiEntry = SmiHandler->SmiEntry;

  //
  // For root SMI handler, use NULL for SmiEntry
  //
  RemoveSmiHandler (SmiHandler, (SmiEntry == &mRootSmiEntry) ? NULL : SmiEntry);
  return EFI_SUCCESS;
}
