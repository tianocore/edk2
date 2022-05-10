/** @file
  Implementation of protocol related services in the UEFI Boot Services table for use in unit tests.

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiBootServicesTableLibUnitTestProtocol.h"

STATIC LIST_ENTRY  mProtocolDatabase       = INITIALIZE_LIST_HEAD_VARIABLE (mProtocolDatabase);
STATIC LIST_ENTRY  gHandleList             = INITIALIZE_LIST_HEAD_VARIABLE (gHandleList);
STATIC UINT64      gHandleDatabaseKey      = 0;
STATIC UINTN       mEfiLocateHandleRequest = 0;

//
// Helper Functions
//

/**
  Check whether a handle is a valid EFI_HANDLE

  @param  UserHandle             The handle to check

  @retval EFI_INVALID_PARAMETER  The handle is NULL or not a valid EFI_HANDLE.
  @retval EFI_SUCCESS            The handle is valid EFI_HANDLE.

**/
EFI_STATUS
UnitTestValidateHandle (
  IN  EFI_HANDLE  UserHandle
  )
{
  IHANDLE     *Handle;
  LIST_ENTRY  *Link;

  if (UserHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  for (Link = gHandleList.BackLink; Link != &gHandleList; Link = Link->BackLink) {
    Handle = CR (Link, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
    if (Handle == (IHANDLE *)UserHandle) {
      return EFI_SUCCESS;
    }
  }

  return EFI_INVALID_PARAMETER;
}

/**
  Finds the protocol entry for the requested protocol.

  @param  Protocol               The ID of the protocol
  @param  Create                 Create a new entry if not found

  @return Protocol entry

**/
PROTOCOL_ENTRY  *
UnitTestFindProtocolEntry (
  IN EFI_GUID  *Protocol,
  IN BOOLEAN   Create
  )
{
  LIST_ENTRY      *Link;
  PROTOCOL_ENTRY  *Item;
  PROTOCOL_ENTRY  *ProtEntry;

  //
  // Search the database for the matching GUID
  //

  ProtEntry = NULL;
  for (Link = mProtocolDatabase.ForwardLink;
       Link != &mProtocolDatabase;
       Link = Link->ForwardLink)
  {
    Item = CR (Link, PROTOCOL_ENTRY, AllEntries, PROTOCOL_ENTRY_SIGNATURE);
    if (CompareGuid (&Item->ProtocolID, Protocol)) {
      //
      // This is the protocol entry
      //

      ProtEntry = Item;
      break;
    }
  }

  //
  // If the protocol entry was not found and Create is TRUE, then
  // allocate a new entry
  //
  if ((ProtEntry == NULL) && Create) {
    ProtEntry = AllocatePool (sizeof (PROTOCOL_ENTRY));

    if (ProtEntry != NULL) {
      //
      // Initialize new protocol entry structure
      //
      ProtEntry->Signature = PROTOCOL_ENTRY_SIGNATURE;
      CopyGuid ((VOID *)&ProtEntry->ProtocolID, Protocol);
      InitializeListHead (&ProtEntry->Protocols);
      InitializeListHead (&ProtEntry->Notify);

      //
      // Add it to protocol database
      //
      InsertTailList (&mProtocolDatabase, &ProtEntry->AllEntries);
    }
  }

  return ProtEntry;
}

/**
  Finds the protocol instance for the requested handle and protocol.
  Note: This function doesn't do parameters checking, it's caller's responsibility
  to pass in valid parameters.

  @param  Handle                 The handle to search the protocol on
  @param  Protocol               GUID of the protocol
  @param  Interface              The interface for the protocol being searched

  @return Protocol instance (NULL: Not found)

**/
PROTOCOL_INTERFACE *
UnitTestFindProtocolInterface (
  IN IHANDLE   *Handle,
  IN EFI_GUID  *Protocol,
  IN VOID      *Interface
  )
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  Prot = NULL;

  //
  // Lookup the protocol entry for this protocol ID
  //

  ProtEntry = UnitTestFindProtocolEntry (Protocol, FALSE);
  if (ProtEntry != NULL) {
    //
    // Look at each protocol interface for any matches
    //
    for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
      //
      // If this protocol interface matches, remove it
      //
      Prot = CR (Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      if ((Prot->Interface == Interface) && (Prot->Protocol == ProtEntry)) {
        break;
      }

      Prot = NULL;
    }
  }

  return Prot;
}

/**
  Signal event for every protocol in protocol entry.

  @param  ProtEntry              Protocol entry

**/
VOID
UnitTestNotifyProtocolEntry (
  IN PROTOCOL_ENTRY  *ProtEntry
  )
{
  PROTOCOL_NOTIFY  *ProtNotify;
  LIST_ENTRY       *Link;

  for (Link = ProtEntry->Notify.ForwardLink; Link != &ProtEntry->Notify; Link = Link->ForwardLink) {
    ProtNotify = CR (Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);
    UnitTestSignalEvent (ProtNotify->Event);
  }
}

/**
  Routine to get the next Handle, when you are searching for all handles.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
UnitTestGetNextLocateAllHandles (
  IN OUT LOCATE_POSITION  *Position,
  OUT VOID                **Interface
  )
{
  IHANDLE  *Handle;

  //
  // Next handle
  //
  Position->Position = Position->Position->ForwardLink;

  //
  // If not at the end of the list, get the handle
  //
  Handle     = NULL;
  *Interface = NULL;
  if (Position->Position != &gHandleList) {
    Handle = CR (Position->Position, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
  }

  return Handle;
}

/**
  Routine to get the next Handle, when you are searching for register protocol
  notifies.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
UnitTestGetNextLocateByRegisterNotify (
  IN OUT LOCATE_POSITION  *Position,
  OUT VOID                **Interface
  )
{
  IHANDLE             *Handle;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_INTERFACE  *Prot;
  LIST_ENTRY          *Link;

  Handle     = NULL;
  *Interface = NULL;
  ProtNotify = Position->SearchKey;

  //
  // If this is the first request, get the next handle
  //
  if (ProtNotify != NULL) {
    ASSERT (ProtNotify->Signature == PROTOCOL_NOTIFY_SIGNATURE);
    Position->SearchKey = NULL;

    //
    // If not at the end of the list, get the next handle
    //
    Link = ProtNotify->Position->ForwardLink;
    if (Link != &ProtNotify->Protocol->Protocols) {
      Prot       = CR (Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
      Handle     = Prot->Handle;
      *Interface = Prot->Interface;
    }
  }

  return Handle;
}

/**
  Routine to get the next Handle, when you are searching for a given protocol.

  @param  Position               Information about which Handle to seach for.
  @param  Interface              Return the interface structure for the matching
                                 protocol.

  @return An pointer to IHANDLE if the next Position is not the end of the list.
          Otherwise,NULL is returned.

**/
IHANDLE *
UnitTestGetNextLocateByProtocol (
  IN OUT LOCATE_POSITION  *Position,
  OUT VOID                **Interface
  )
{
  IHANDLE             *Handle;
  LIST_ENTRY          *Link;
  PROTOCOL_INTERFACE  *Prot;

  Handle     = NULL;
  *Interface = NULL;
  for ( ; ;) {
    //
    // Next entry
    //
    Link               = Position->Position->ForwardLink;
    Position->Position = Link;

    //
    // If not at the end, return the handle
    //
    if (Link == &Position->ProtEntry->Protocols) {
      Handle = NULL;
      break;
    }

    //
    // Get the handle
    //
    Prot       = CR (Link, PROTOCOL_INTERFACE, ByProtocol, PROTOCOL_INTERFACE_SIGNATURE);
    Handle     = Prot->Handle;
    *Interface = Prot->Interface;

    //
    // If this handle has not been returned this request, then
    // return it now
    //
    if (Handle->LocateRequest != mEfiLocateHandleRequest) {
      Handle->LocateRequest = mEfiLocateHandleRequest;
      break;
    }
  }

  return Handle;
}

/**
  Attempts to disconnect all drivers that are using the protocol interface being queried.
  If failed, reconnect all drivers disconnected.
  Note: This function doesn't do parameters checking, it's caller's responsibility
  to pass in valid parameters.

  @param  UserHandle             The handle on which the protocol is installed
  @param  Prot                   The protocol to disconnect drivers from

  @retval EFI_SUCCESS            Drivers using the protocol interface are all
                                 disconnected
  @retval EFI_ACCESS_DENIED      Failed to disconnect one or all of the drivers

**/
EFI_STATUS
UnitTestDisconnectControllersUsingProtocolInterface (
  IN EFI_HANDLE          UserHandle,
  IN PROTOCOL_INTERFACE  *Prot
  )
{
  EFI_STATUS          Status;
  BOOLEAN             ItemFound;
  LIST_ENTRY          *Link;
  OPEN_PROTOCOL_DATA  *OpenData;

  Status = EFI_SUCCESS;

  //
  // Attempt to disconnect all drivers from this protocol interface
  //
  do {
    ItemFound = FALSE;
    for (Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList; Link = Link->ForwardLink) {
      OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
      if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
        Status = UnitTestDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
        if (!EFI_ERROR (Status)) {
          ItemFound = TRUE;
        }

        break;
      }
    }
  } while (ItemFound);

  if (!EFI_ERROR (Status)) {
    //
    // Attempt to remove BY_HANDLE_PROTOCOL and GET_PROTOCOL and TEST_PROTOCOL Open List items
    //
    for (Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList;) {
      OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
      if ((OpenData->Attributes &
           (EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL | EFI_OPEN_PROTOCOL_GET_PROTOCOL | EFI_OPEN_PROTOCOL_TEST_PROTOCOL)) != 0)
      {
        Link = RemoveEntryList (&OpenData->Link);
        Prot->OpenListCount--;
        FreePool (OpenData);
      } else {
        Link = Link->ForwardLink;
      }
    }
  }

  //
  // If there are errors or still has open items in the list, then reconnect all the drivers and return an error
  //
  if (EFI_ERROR (Status) || (Prot->OpenListCount > 0)) {
    UnitTestConnectController (UserHandle, NULL, NULL, TRUE);
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}

/**
  Removes Protocol from the protocol list (but not the handle list).

  @param  Handle                 The handle to remove protocol on.
  @param  Protocol               GUID of the protocol to be moved
  @param  Interface              The interface of the protocol

  @return Protocol Entry

**/
PROTOCOL_INTERFACE *
UnitTestRemoveInterfaceFromProtocol (
  IN IHANDLE   *Handle,
  IN EFI_GUID  *Protocol,
  IN VOID      *Interface
  )
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  Prot = UnitTestFindProtocolInterface (Handle, Protocol, Interface);
  if (Prot != NULL) {
    ProtEntry = Prot->Protocol;

    //
    // If there's a protocol notify location pointing to this entry, back it up one
    //
    for (Link = ProtEntry->Notify.ForwardLink; Link != &ProtEntry->Notify; Link = Link->ForwardLink) {
      ProtNotify = CR (Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);

      if (ProtNotify->Position == &Prot->ByProtocol) {
        ProtNotify->Position = Prot->ByProtocol.BackLink;
      }
    }

    //
    // Remove the protocol interface entry
    //
    RemoveEntryList (&Prot->ByProtocol);
  }

  return Prot;
}

//
// Boot Services Function Implementation
//

/**
  Locate a certain GUID protocol interface in a Handle's protocols.

  @param  UserHandle             The handle to obtain the protocol interface on
  @param  Protocol               The GUID of the protocol

  @return The requested protocol interface for the handle

**/
PROTOCOL_INTERFACE  *
UnitTestGetProtocolInterface (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol
  )
{
  EFI_STATUS          Status;
  PROTOCOL_ENTRY      *ProtEntry;
  PROTOCOL_INTERFACE  *Prot;
  IHANDLE             *Handle;
  LIST_ENTRY          *Link;

  Status = UnitTestValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Handle = (IHANDLE *)UserHandle;

  //
  // Look at each protocol interface for a match
  //
  for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
    Prot      = CR (Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    ProtEntry = Prot->Protocol;
    if (CompareGuid (&ProtEntry->ProtocolID, Protocol)) {
      return Prot;
    }
  }

  return NULL;
}

/**
  Installs a protocol interface into the boot services environment.

  @param  UserHandle             The handle to install the protocol handler on,
                                 or NULL if a new handle is to be allocated
  @param  Protocol               The protocol to add to the handle
  @param  InterfaceType          Indicates whether Interface is supplied in
                                 native form.
  @param  Interface              The interface for the protocol being added
  @param  Notify                 indicates whether notify the notification list
                                 for this protocol

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   No enough buffer to allocate
  @retval EFI_SUCCESS            Protocol interface successfully installed

**/
EFI_STATUS
UnitTestInstallProtocolInterfaceNotify (
  IN OUT EFI_HANDLE      *UserHandle,
  IN EFI_GUID            *Protocol,
  IN EFI_INTERFACE_TYPE  InterfaceType,
  IN VOID                *Interface,
  IN BOOLEAN             Notify
  )
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_ENTRY      *ProtEntry;
  IHANDLE             *Handle;
  EFI_STATUS          Status;
  VOID                *ExistingInterface;

  //
  // returns EFI_INVALID_PARAMETER if InterfaceType is invalid.
  // Also added check for invalid UserHandle and Protocol pointers.
  //
  if ((UserHandle == NULL) || (Protocol == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterfaceType != EFI_NATIVE_INTERFACE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Print debug message
  //
  UT_LOG_INFO ("InstallProtocolInterface: %g %p\n", Protocol, Interface);

  Status = EFI_OUT_OF_RESOURCES;
  Prot   = NULL;
  Handle = NULL;

  if (*UserHandle != NULL) {
    Status = UnitTestHandleProtocol (*UserHandle, Protocol, (VOID **)&ExistingInterface);
    if (!EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Lookup the Protocol Entry for the requested protocol
  //
  ProtEntry = UnitTestFindProtocolEntry (Protocol, TRUE);
  if (ProtEntry == NULL) {
    goto Done;
  }

  //
  // Allocate a new protocol interface structure
  //
  Prot = AllocateZeroPool (sizeof (PROTOCOL_INTERFACE));
  if (Prot == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // If caller didn't supply a handle, allocate a new one
  //
  Handle = (IHANDLE *)*UserHandle;
  if (Handle == NULL) {
    Handle = AllocateZeroPool (sizeof (IHANDLE));
    if (Handle == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Initialize new handler structure
    //
    Handle->Signature = EFI_HANDLE_SIGNATURE;
    InitializeListHead (&Handle->Protocols);

    //
    // Initialize the Key to show that the handle has been created/modified
    //
    gHandleDatabaseKey++;
    Handle->Key = gHandleDatabaseKey;

    //
    // Add this handle to the list global list of all handles
    // in the system
    //
    InsertTailList (&gHandleList, &Handle->AllHandles);
  } else {
    Status =  UnitTestValidateHandle (Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "InstallProtocolInterface: input handle at 0x%x is invalid\n", Handle));
      goto Done;
    }
  }

  //
  // Each interface that is added must be unique
  //
  ASSERT (UnitTestFindProtocolInterface (Handle, Protocol, Interface) == NULL);

  //
  // Initialize the protocol interface structure
  //
  Prot->Signature = PROTOCOL_INTERFACE_SIGNATURE;
  Prot->Handle    = Handle;
  Prot->Protocol  = ProtEntry;
  Prot->Interface = Interface;

  //
  // Initialize OpenProtocol Data base
  //
  InitializeListHead (&Prot->OpenList);
  Prot->OpenListCount = 0;

  //
  // Add this protocol interface to the head of the supported
  // protocol list for this handle
  //
  InsertHeadList (&Handle->Protocols, &Prot->Link);

  //
  // Add this protocol interface to the tail of the
  // protocol entry
  //
  InsertTailList (&ProtEntry->Protocols, &Prot->ByProtocol);

  //
  // Notify the notification list for this protocol
  //
  if (Notify) {
    UnitTestNotifyProtocolEntry (ProtEntry);
  }

  Status = EFI_SUCCESS;

Done:
  if (!EFI_ERROR (Status)) {
    //
    // Return the new handle back to the caller
    //
    *UserHandle = Handle;
  } else {
    //
    // There was an error, clean up
    //
    if (Prot != NULL) {
      UnitTestFreePool (Prot);
    }

    DEBUG ((DEBUG_ERROR, "InstallProtocolInterface: %g %p failed with %r\n", Protocol, Interface, Status));
  }

  return Status;
}

/**
  Wrapper function to UnitTestInstallProtocolInterfaceNotify.  This is the public API which
  Calls the private one which contains a BOOLEAN parameter for notifications

  @param  UserHandle             The handle to install the protocol handler on,
                                 or NULL if a new handle is to be allocated
  @param  Protocol               The protocol to add to the handle
  @param  InterfaceType          Indicates whether Interface is supplied in
                                 native form.
  @param  Interface              The interface for the protocol being added

  @return Status code

**/
EFI_STATUS
EFIAPI
UnitTestInstallProtocolInterface (
  IN OUT EFI_HANDLE      *UserHandle,
  IN EFI_GUID            *Protocol,
  IN EFI_INTERFACE_TYPE  InterfaceType,
  IN VOID                *Interface
  )
{
  return UnitTestInstallProtocolInterfaceNotify (
           UserHandle,
           Protocol,
           InterfaceType,
           Interface,
           TRUE
           );
}

/**
  Reinstall a protocol interface on a device handle.  The OldInterface for Protocol is replaced by the NewInterface.

  @param  UserHandle             Handle on which the interface is to be
                                 reinstalled
  @param  Protocol               The numeric ID of the interface
  @param  OldInterface           A pointer to the old interface
  @param  NewInterface           A pointer to the new interface

  @retval EFI_SUCCESS            The protocol interface was installed
  @retval EFI_NOT_FOUND          The OldInterface on the handle was not found
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value

**/
EFI_STATUS
EFIAPI
UnitTestReinstallProtocolInterface (
  IN EFI_HANDLE  UserHandle,
  IN EFI_GUID    *Protocol,
  IN VOID        *OldInterface,
  IN VOID        *NewInterface
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Uninstalls all instances of a protocol:interfacer from a handle.
  If the last protocol interface is remove from the handle, the
  handle is freed.

  @param  UserHandle             The handle to remove the protocol handler from
  @param  Protocol               The protocol, of protocol:interface, to remove
  @param  Interface              The interface, of protocol:interface, to remove

  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_SUCCESS            Protocol interface successfully uninstalled.

**/
EFI_STATUS
EFIAPI
UnitTestUninstallProtocolInterface (
  IN EFI_HANDLE  UserHandle,
  IN EFI_GUID    *Protocol,
  IN VOID        *Interface
  )
{
  EFI_STATUS          Status;
  IHANDLE             *Handle;
  PROTOCOL_INTERFACE  *Prot;

  //
  // Check that Protocol is valid
  //
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check that UserHandle is a valid handle
  //
  Status = UnitTestValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check that Protocol exists on UserHandle, and Interface matches the interface in the database
  //
  Prot = UnitTestFindProtocolInterface (UserHandle, Protocol, Interface);
  if (Prot == NULL) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Attempt to disconnect all drivers that are using the protocol interface that is about to be removed
  //
  Status = UnitTestDisconnectControllersUsingProtocolInterface (
             UserHandle,
             Prot
             );
  if (EFI_ERROR (Status)) {
    //
    // One or more drivers refused to release, so return the error
    //
    goto Done;
  }

  //
  // Remove the protocol interface from the protocol
  //
  Status = EFI_NOT_FOUND;
  Handle = (IHANDLE *)UserHandle;
  Prot   = UnitTestRemoveInterfaceFromProtocol (Handle, Protocol, Interface);

  if (Prot != NULL) {
    //
    // Update the Key to show that the handle has been created/modified
    //
    gHandleDatabaseKey++;
    Handle->Key = gHandleDatabaseKey;

    //
    // Remove the protocol interface from the handle
    //
    RemoveEntryList (&Prot->Link);

    //
    // Free the memory
    //
    Prot->Signature = 0;
    FreePool (Prot);
    Status = EFI_SUCCESS;
  }

  //
  // If there are no more handlers for the handle, free the handle
  //
  if (IsListEmpty (&Handle->Protocols)) {
    Handle->Signature = 0;
    RemoveEntryList (&Handle->AllHandles);
    FreePool (Handle);
  }

Done:
  return Status;
}

/**
  Queries a handle to determine if it supports a specified protocol.

  @param  UserHandle             The handle being queried.
  @param  Protocol               The published unique identifier of the protocol.
  @param  Interface              Supplies the address where a pointer to the
                                 corresponding Protocol Interface is returned.

  @return The requested protocol interface for the handle

**/
EFI_STATUS
EFIAPI
UnitTestHandleProtocol (
  IN EFI_HANDLE  UserHandle,
  IN EFI_GUID    *Protocol,
  OUT VOID       **Interface
  )
{
  return UnitTestOpenProtocol (
           UserHandle,
           Protocol,
           Interface,
           gImageHandle,
           NULL,
           EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
           );
}

/**
  Add a new protocol notification record for the request protocol.

  @param  Protocol               The requested protocol to add the notify
                                 registration
  @param  Event                  The event to signal
  @param  Registration           Returns the registration record

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully returned the registration record
                                 that has been added

**/
EFI_STATUS
EFIAPI
UnitTestRegisterProtocolNotify (
  IN EFI_GUID   *Protocol,
  IN EFI_EVENT  Event,
  OUT  VOID     **Registration
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Locates the requested handle(s) and returns them in Buffer.

  @param  SearchType             The type of search to perform to locate the
                                 handles
  @param  Protocol               The protocol to search for
  @param  SearchKey              Dependant on SearchType
  @param  BufferSize             On input the size of Buffer.  On output the
                                 size of data returned.
  @param  Buffer                 The buffer to return the results in

  @retval EFI_BUFFER_TOO_SMALL   Buffer too small, required buffer size is
                                 returned in BufferSize.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully found the requested handle(s) and
                                 returns them in Buffer.

**/
EFI_STATUS
EFIAPI
UnitTestLocateHandle (
  IN EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN EFI_GUID                *Protocol   OPTIONAL,
  IN VOID                    *SearchKey  OPTIONAL,
  IN OUT UINTN               *BufferSize,
  OUT EFI_HANDLE             *Buffer
  )
{
  EFI_STATUS          Status;
  LOCATE_POSITION     Position;
  PROTOCOL_NOTIFY     *ProtNotify;
  UNIT_TEST_GET_NEXT  GetNext;
  UINTN               ResultSize;
  IHANDLE             *Handle;
  IHANDLE             **ResultBuffer;
  VOID                *Interface;

  if (BufferSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*BufferSize > 0) && (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  GetNext = NULL;

  //
  // Set initial position
  //
  Position.Protocol  = Protocol;
  Position.SearchKey = SearchKey;
  Position.Position  = &gHandleList;

  ResultSize   = 0;
  ResultBuffer = (IHANDLE **)Buffer;
  Status       = EFI_SUCCESS;

  //
  // Get the search function based on type
  //
  switch (SearchType) {
    case AllHandles:
      GetNext = UnitTestGetNextLocateAllHandles;
      break;

    case ByRegisterNotify:
      //
      // Must have SearchKey for locate ByRegisterNotify
      //
      if (SearchKey == NULL) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      GetNext = UnitTestGetNextLocateByRegisterNotify;
      break;

    case ByProtocol:
      GetNext = UnitTestGetNextLocateByProtocol;
      if (Protocol == NULL) {
        Status = EFI_INVALID_PARAMETER;
        break;
      }

      //
      // Look up the protocol entry and set the head pointer
      //
      Position.ProtEntry = UnitTestFindProtocolEntry (Protocol, FALSE);
      if (Position.ProtEntry == NULL) {
        Status = EFI_NOT_FOUND;
        break;
      }

      Position.Position = &Position.ProtEntry->Protocols;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (GetNext != NULL);
  //
  // Enumerate out the matching handles
  //
  mEfiLocateHandleRequest += 1;
  for ( ; ;) {
    //
    // Get the next handle.  If no more handles, stop
    //
    Handle = GetNext (&Position, &Interface);
    if (NULL == Handle) {
      break;
    }

    //
    // Increase the resulting buffer size, and if this handle
    // fits return it
    //
    ResultSize += sizeof (Handle);
    if (ResultSize <= *BufferSize) {
      *ResultBuffer = Handle;
      ResultBuffer += 1;
    }
  }

  //
  // If the result is a zero length buffer, then there were no
  // matching handles
  //
  if (ResultSize == 0) {
    Status = EFI_NOT_FOUND;
  } else {
    //
    // Return the resulting buffer size.  If it's larger than what
    // was passed, then set the error code
    //
    if (ResultSize > *BufferSize) {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = ResultSize;

    if ((SearchType == ByRegisterNotify) && !EFI_ERROR (Status)) {
      //
      // If this is a search by register notify and a handle was
      // returned, update the register notification position
      //
      ASSERT (SearchKey != NULL);
      ProtNotify           = SearchKey;
      ProtNotify->Position = ProtNotify->Position->ForwardLink;
    }
  }

  return Status;
}

/**
  Locates the handle to a device on the device path that best matches the specified protocol.

  @param  Protocol               The protocol to search for.
  @param  DevicePath             On input, a pointer to a pointer to the device
                                 path. On output, the device path pointer is
                                 modified to point to the remaining part of the
                                 devicepath.
  @param  Device                 A pointer to the returned device handle.

  @retval EFI_SUCCESS            The resulting handle was returned.
  @retval EFI_NOT_FOUND          No handles matched the search.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
UnitTestLocateDevicePath (
  IN EFI_GUID                      *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath,
  OUT EFI_HANDLE                   *Device
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Boot Service called to add, modify, or remove a system configuration table from
  the EFI System Table.

  @param  Guid           Pointer to the GUID for the entry to add, update, or
                         remove
  @param  Table          Pointer to the configuration table for the entry to add,
                         update, or remove, may be NULL.

  @return EFI_SUCCESS               Guid, Table pair added, updated, or removed.
  @return EFI_INVALID_PARAMETER     Input GUID not valid.
  @return EFI_NOT_FOUND             Attempted to delete non-existant entry
  @return EFI_OUT_OF_RESOURCES      Not enough memory available

**/
EFI_STATUS
EFIAPI
UnitTestInstallConfigurationTable (
  IN EFI_GUID  *Guid,
  IN VOID      *Table
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Locates the installed protocol handler for the handle, and
  invokes it to obtain the protocol interface. Usage information
  is registered in the protocol data base.

  @param  UserHandle             The handle to obtain the protocol interface on
  @param  Protocol               The ID of the protocol
  @param  Interface              The location to return the protocol interface
  @param  ImageHandle            The handle of the Image that is opening the
                                 protocol interface specified by Protocol and
                                 Interface.
  @param  ControllerHandle       The controller handle that is requiring this
                                 interface.
  @param  Attributes             The open mode of the protocol interface
                                 specified by Handle and Protocol.

  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_SUCCESS            Get the protocol interface.

**/
EFI_STATUS
EFIAPI
UnitTestOpenProtocol (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface OPTIONAL,
  IN  EFI_HANDLE  ImageHandle,
  IN  EFI_HANDLE  ControllerHandle,
  IN  UINT32      Attributes
  )
{
  EFI_STATUS          Status;
  PROTOCOL_INTERFACE  *Prot;
  LIST_ENTRY          *Link;
  OPEN_PROTOCOL_DATA  *OpenData;
  BOOLEAN             ByDriver;
  BOOLEAN             Exclusive;
  BOOLEAN             Disconnect;
  BOOLEAN             ExactMatch;

  //
  // Check for invalid Protocol
  //
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for invalid Interface
  //
  if ((Attributes != EFI_OPEN_PROTOCOL_TEST_PROTOCOL) && (Interface == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for invalid UserHandle
  //
  Status =  UnitTestValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check for invalid Attributes
  //
  switch (Attributes) {
    case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER:
      Status =  UnitTestValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status =  UnitTestValidateHandle (ControllerHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if (UserHandle == ControllerHandle) {
        return EFI_INVALID_PARAMETER;
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_DRIVER:
    case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE:
      Status =  UnitTestValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status =  UnitTestValidateHandle (ControllerHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;
    case EFI_OPEN_PROTOCOL_EXCLUSIVE:
      Status =  UnitTestValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:
    case EFI_OPEN_PROTOCOL_GET_PROTOCOL:
    case EFI_OPEN_PROTOCOL_TEST_PROTOCOL:
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // Look at each protocol interface for a match
  //
  Prot = UnitTestGetProtocolInterface (UserHandle, Protocol);
  if (Prot == NULL) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_SUCCESS;

  ByDriver  = FALSE;
  Exclusive = FALSE;
  for ( Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList; Link = Link->ForwardLink) {
    OpenData   = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
    ExactMatch =  (BOOLEAN)((OpenData->AgentHandle == ImageHandle) &&
                            (OpenData->Attributes == Attributes)  &&
                            (OpenData->ControllerHandle == ControllerHandle));
    if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
      ByDriver = TRUE;
      if (ExactMatch) {
        Status = EFI_ALREADY_STARTED;
        goto Done;
      }
    }

    if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_EXCLUSIVE) != 0) {
      Exclusive = TRUE;
    } else if (ExactMatch) {
      OpenData->OpenCount++;
      Status = EFI_SUCCESS;
      goto Done;
    }
  }

  //
  // ByDriver  TRUE  -> A driver is managing (UserHandle, Protocol)
  // ByDriver  FALSE -> There are no drivers managing (UserHandle, Protocol)
  // Exclusive TRUE  -> Something has exclusive access to (UserHandle, Protocol)
  // Exclusive FALSE -> Nothing has exclusive access to (UserHandle, Protocol)
  //

  switch (Attributes) {
    case EFI_OPEN_PROTOCOL_BY_DRIVER:
      if (Exclusive || ByDriver) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE:
    case EFI_OPEN_PROTOCOL_EXCLUSIVE:
      if (Exclusive) {
        Status = EFI_ACCESS_DENIED;
        goto Done;
      }

      if (ByDriver) {
        do {
          Disconnect = FALSE;
          for (Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList; Link = Link->ForwardLink) {
            OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
            if ((OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) != 0) {
              Disconnect = TRUE;
              Status     = UnitTestDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
              if (EFI_ERROR (Status)) {
                Status = EFI_ACCESS_DENIED;
                goto Done;
              } else {
                break;
              }
            }
          }
        } while (Disconnect);
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER:
    case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:
    case EFI_OPEN_PROTOCOL_GET_PROTOCOL:
    case EFI_OPEN_PROTOCOL_TEST_PROTOCOL:
      break;
  }

  if (ImageHandle == NULL) {
    Status = EFI_SUCCESS;
    goto Done;
  }

  //
  // Create new entry
  //
  OpenData = AllocatePool (sizeof (OPEN_PROTOCOL_DATA));
  if (OpenData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    OpenData->Signature        = OPEN_PROTOCOL_DATA_SIGNATURE;
    OpenData->AgentHandle      = ImageHandle;
    OpenData->ControllerHandle = ControllerHandle;
    OpenData->Attributes       = Attributes;
    OpenData->OpenCount        = 1;
    InsertTailList (&Prot->OpenList, &OpenData->Link);
    Prot->OpenListCount++;
    Status = EFI_SUCCESS;
  }

Done:

  if (Attributes != EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
    //
    // Keep Interface unmodified in case of any Error
    // except EFI_ALREADY_STARTED and EFI_UNSUPPORTED.
    //
    if (!EFI_ERROR (Status) || (Status == EFI_ALREADY_STARTED)) {
      //
      // According to above logic, if 'Prot' is NULL, then the 'Status' must be
      // EFI_UNSUPPORTED. Here the 'Status' is not EFI_UNSUPPORTED, so 'Prot'
      // must be not NULL.
      //
      // The ASSERT here is for addressing a false positive NULL pointer
      // dereference issue raised from static analysis.
      //
      ASSERT (Prot != NULL);
      //
      // EFI_ALREADY_STARTED is not an error for bus driver.
      // Return the corresponding protocol interface.
      //
      *Interface = Prot->Interface;
    } else if (Status == EFI_UNSUPPORTED) {
      //
      // Return NULL Interface if Unsupported Protocol.
      //
      *Interface = NULL;
    }
  }

  return Status;
}

/**
  Closes a protocol on a handle that was opened using OpenProtocol().

  @param  UserHandle             The handle for the protocol interface that was
                                 previously opened with OpenProtocol(), and is
                                 now being closed.
  @param  Protocol               The published unique identifier of the protocol.
                                 It is the caller's responsibility to pass in a
                                 valid GUID.
  @param  AgentHandle            The handle of the agent that is closing the
                                 protocol interface.
  @param  ControllerHandle       If the agent that opened a protocol is a driver
                                 that follows the EFI Driver Model, then this
                                 parameter is the controller handle that required
                                 the protocol interface. If the agent does not
                                 follow the EFI Driver Model, then this parameter
                                 is optional and may be NULL.

  @retval EFI_SUCCESS            The protocol instance was closed.
  @retval EFI_INVALID_PARAMETER  Handle, AgentHandle or ControllerHandle is not a
                                 valid EFI_HANDLE.
  @retval EFI_NOT_FOUND          Can not find the specified protocol or
                                 AgentHandle.

**/
EFI_STATUS
EFIAPI
UnitTestCloseProtocol (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol,
  IN  EFI_HANDLE  AgentHandle,
  IN  EFI_HANDLE  ControllerHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Return information about Opened protocols in the system

  @param  UserHandle             The handle to close the protocol interface on
  @param  Protocol               The ID of the protocol
  @param  EntryBuffer            A pointer to a buffer of open protocol
                                 information in the form of
                                 EFI_OPEN_PROTOCOL_INFORMATION_ENTRY structures.
  @param  EntryCount             Number of EntryBuffer entries

**/
EFI_STATUS
EFIAPI
UnitTestOpenProtocolInformation (
  IN  EFI_HANDLE                           UserHandle,
  IN  EFI_GUID                             *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  **EntryBuffer,
  OUT UINTN                                *EntryCount
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Retrieves the list of protocol interface GUIDs that are installed on a handle in a buffer allocated
  from pool.

  @param  UserHandle             The handle from which to retrieve the list of
                                 protocol interface GUIDs.
  @param  ProtocolBuffer         A pointer to the list of protocol interface GUID
                                 pointers that are installed on Handle.
  @param  ProtocolBufferCount    A pointer to the number of GUID pointers present
                                 in ProtocolBuffer.

  @retval EFI_SUCCESS            The list of protocol interface GUIDs installed
                                 on Handle was returned in ProtocolBuffer. The
                                 number of protocol interface GUIDs was returned
                                 in ProtocolBufferCount.
  @retval EFI_INVALID_PARAMETER  Handle is NULL.
  @retval EFI_INVALID_PARAMETER  Handle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER  ProtocolBuffer is NULL.
  @retval EFI_INVALID_PARAMETER  ProtocolBufferCount is NULL.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 results.

**/
EFI_STATUS
EFIAPI
UnitTestProtocolsPerHandle (
  IN EFI_HANDLE  UserHandle,
  OUT EFI_GUID   ***ProtocolBuffer,
  OUT UINTN      *ProtocolBufferCount
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from pool. This is a version of UnitTestLocateHandle()
  that allocates a buffer for the caller.

  @param  SearchType             Specifies which handle(s) are to be returned.
  @param  Protocol               Provides the protocol to search by.    This
                                 parameter is only valid for SearchType
                                 ByProtocol.
  @param  SearchKey              Supplies the search key depending on the
                                 SearchType.
  @param  NumberHandles          The number of handles returned in Buffer.
  @param  Buffer                 A pointer to the buffer to return the requested
                                 array of  handles that support Protocol.

  @retval EFI_SUCCESS            The result array of handles was returned.
  @retval EFI_NOT_FOUND          No handles match the search.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 matching results.
  @retval EFI_INVALID_PARAMETER  One or more parameters are not valid.

**/
EFI_STATUS
EFIAPI
UnitTestLocateHandleBuffer (
  IN EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN EFI_GUID                *Protocol OPTIONAL,
  IN VOID                    *SearchKey OPTIONAL,
  IN OUT UINTN               *NumberHandles,
  OUT EFI_HANDLE             **Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if (NumberHandles == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize     = 0;
  *NumberHandles = 0;
  *Buffer        = NULL;
  Status         = UnitTestLocateHandle (
                     SearchType,
                     Protocol,
                     SearchKey,
                     &BufferSize,
                     *Buffer
                     );
  //
  // LocateHandleBuffer() returns incorrect status code if SearchType is
  // invalid.
  //
  // Add code to correctly handle expected errors from UnitTestLocateHandle().
  //
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    if (Status != EFI_INVALID_PARAMETER) {
      Status = EFI_NOT_FOUND;
    }

    return Status;
  }

  *Buffer = AllocatePool (BufferSize);
  if (*Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UnitTestLocateHandle (
             SearchType,
             Protocol,
             SearchKey,
             &BufferSize,
             *Buffer
             );

  *NumberHandles = BufferSize / sizeof (EFI_HANDLE);
  if (EFI_ERROR (Status)) {
    *NumberHandles = 0;
  }

  return Status;
}

/**
  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is passed in, return a Protocol Instance that was just add
  to the system. If Registration is NULL return the first Protocol Interface
  you find.

  @param  Protocol               The protocol to search for
  @param  Registration           Optional Registration Key returned from
                                 RegisterProtocolNotify()
  @param  Interface              Return the Protocol interface (instance).

  @retval EFI_SUCCESS            If a valid Interface is returned
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Protocol interface not found

**/
EFI_STATUS
EFIAPI
UnitTestLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Installs a list of protocol interface into the boot services environment.
  This function calls InstallProtocolInterface() in a loop. If any error
  occurs all the protocols added by this function are removed. This is
  basically a lib function to save space.

  @param  Handle                 The handle to install the protocol handlers on,
                                 or NULL if a new handle is to be allocated
  @param  ...                    EFI_GUID followed by protocol instance. A NULL
                                 terminates the  list. The pairs are the
                                 arguments to InstallProtocolInterface(). All the
                                 protocols are added to Handle.

  @retval EFI_SUCCESS            All the protocol interface was installed.
  @retval EFI_OUT_OF_RESOURCES   There was not enough memory in pool to install all the protocols.
  @retval EFI_ALREADY_STARTED    A Device Path Protocol instance was passed in that is already present in
                                 the handle database.
  @retval EFI_INVALID_PARAMETER  Handle is NULL.
  @retval EFI_INVALID_PARAMETER  Protocol is already installed on the handle specified by Handle.

**/
EFI_STATUS
EFIAPI
UnitTestInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE  *Handle,
  ...
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Uninstalls a list of protocol interface in the boot services environment.
  This function calls UninstallProtocolInterface() in a loop. This is
  basically a lib function to save space.

  @param  Handle                 The handle to uninstall the protocol
  @param  ...                    EFI_GUID followed by protocol instance. A NULL
                                 terminates the  list. The pairs are the
                                 arguments to UninstallProtocolInterface(). All
                                 the protocols are added to Handle.

  @return Status code

**/
EFI_STATUS
EFIAPI
UnitTestUninstallMultipleProtocolInterfaces (
  IN EFI_HANDLE  Handle,
  ...
  )
{
  EFI_STATUS  Status;
  VA_LIST     Args;
  EFI_GUID    *Protocol;
  VOID        *Interface;
  UINTN       Index;

  VA_START (Args, Handle);
  for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
    //
    // If protocol is NULL, then it's the end of the list
    //
    Protocol = VA_ARG (Args, EFI_GUID *);
    if (Protocol == NULL) {
      break;
    }

    Interface = VA_ARG (Args, VOID *);

    //
    // Uninstall it
    //
    Status = UnitTestUninstallProtocolInterface (Handle, Protocol, Interface);
  }

  VA_END (Args);

  //
  // If there was an error, add all the interfaces that were
  // uninstalled without any errors
  //
  if (EFI_ERROR (Status)) {
    //
    // Reset the va_arg back to the first argument.
    //
    VA_START (Args, Handle);
    for ( ; Index > 1; Index--) {
      Protocol  = VA_ARG (Args, EFI_GUID *);
      Interface = VA_ARG (Args, VOID *);
      UnitTestInstallProtocolInterface (&Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
    }

    VA_END (Args);
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}
