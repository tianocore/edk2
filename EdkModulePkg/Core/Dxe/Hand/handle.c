/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  handle.c

Abstract:

  EFI handle & protocol handling



Revision History

--*/

#include <DxeMain.h>


//
// mProtocolDatabase     - A list of all protocols in the system.  (simple list for now)
// gHandleList           - A list of all the handles in the system
// gProtocolDatabaseLock - Lock to protect the mProtocolDatabase
// gHandleDatabaseKey    -  The Key to show that the handle has been created/modified
//
static LIST_ENTRY      mProtocolDatabase     = INITIALIZE_LIST_HEAD_VARIABLE (mProtocolDatabase);
LIST_ENTRY             gHandleList           = INITIALIZE_LIST_HEAD_VARIABLE (gHandleList);
EFI_LOCK               gProtocolDatabaseLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
UINT64                 gHandleDatabaseKey    = 0;


VOID
CoreAcquireProtocolLock (
  VOID
  )
/*++

Routine Description:

  Acquire lock on gProtocolDatabaseLock.
  
Arguments:

  None
  
Returns:

  None

--*/
{
  CoreAcquireLock (&gProtocolDatabaseLock);
}


VOID
CoreReleaseProtocolLock (
  VOID
  )
/*++

Routine Description:

  Release lock on gProtocolDatabaseLock.
  
Arguments:

  None
  
Returns:

  None

--*/
{
  CoreReleaseLock (&gProtocolDatabaseLock);
}


EFI_STATUS
CoreValidateHandle (
  IN  EFI_HANDLE                UserHandle
  )
/*++

Routine Description:

  Check whether a handle is a valid EFI_HANDLE
  
Arguments:

  UserHandle    - The handle to check
  
Returns:

  EFI_INVALID_PARAMETER   - The handle is NULL or not a valid EFI_HANDLE.

  EFI_SUCCESS             - The handle is valid EFI_HANDLE.

--*/
{
  IHANDLE             *Handle;

  Handle = (IHANDLE *)UserHandle;
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if (Handle->Signature != EFI_HANDLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  return EFI_SUCCESS;
}


PROTOCOL_ENTRY  *
CoreFindProtocolEntry (
  IN EFI_GUID   *Protocol,
  IN BOOLEAN    Create
  )
/*++

Routine Description:

  Finds the protocol entry for the requested protocol.
  
  The gProtocolDatabaseLock must be owned

Arguments:
  
  Protocol  - The ID of the protocol 

  Create    - Create a new entry if not found

Returns:

  Protocol entry

--*/
{
  LIST_ENTRY          *Link;
  PROTOCOL_ENTRY      *Item;
  PROTOCOL_ENTRY      *ProtEntry;

  ASSERT_LOCKED(&gProtocolDatabaseLock);

  //
  // Search the database for the matching GUID
  //

  ProtEntry = NULL;
  for (Link = mProtocolDatabase.ForwardLink; 
       Link != &mProtocolDatabase; 
       Link = Link->ForwardLink) {

    Item = CR(Link, PROTOCOL_ENTRY, AllEntries, PROTOCOL_ENTRY_SIGNATURE);
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
    ProtEntry = CoreAllocateBootServicesPool (sizeof(PROTOCOL_ENTRY));
    
    if (ProtEntry != NULL) {
      //
      // Initialize new protocol entry structure
      //
      ProtEntry->Signature = PROTOCOL_ENTRY_SIGNATURE;
      CopyMem ((VOID *)&ProtEntry->ProtocolID, Protocol, sizeof (EFI_GUID));
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


PROTOCOL_INTERFACE *
CoreFindProtocolInterface (
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  )
/*++

Routine Description:

  Finds the protocol instance for the requested handle and protocol.
  
  Note: This function doesn't do parameters checking, it's caller's responsibility 
        to pass in valid parameters.
  
Arguments:
  
  Handle    - The handle to search the protocol on
  
  Protocol  - GUID of the protocol

  Interface - The interface for the protocol being searched

Returns:

  Protocol instance (NULL: Not found)

--*/  
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  ASSERT_LOCKED(&gProtocolDatabaseLock);
  Prot = NULL;

  //
  // Lookup the protocol entry for this protocol ID
  //

  ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
  if (ProtEntry != NULL) {

    //
    // Look at each protocol interface for any matches
    //
    for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link=Link->ForwardLink) {

      //
      // If this protocol interface matches, remove it
      //
      Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
      if (Prot->Interface == Interface && Prot->Protocol == ProtEntry) {
        break;
      }

      Prot = NULL;
    }
  }

  return Prot;
}

STATIC
EFI_STATUS
CoreUnregisterProtocolNotifyEvent (
  IN EFI_EVENT      Event
  )
/*++

Routine Description:

  Removes an event from a register protocol notify list on a protocol.

Arguments:
  
  Event   - The event to search for in the protocol database.

Returns:

  EFI_SUCCESS   if the event was found and removed.
  EFI_NOT_FOUND if the event was not found in the protocl database.

--*/
{
  LIST_ENTRY         *Link;
  PROTOCOL_ENTRY     *ProtEntry;
  LIST_ENTRY         *NotifyLink;
  PROTOCOL_NOTIFY    *ProtNotify;

  CoreAcquireProtocolLock ();

  for ( Link =  mProtocolDatabase.ForwardLink; 
        Link != &mProtocolDatabase; 
        Link =  Link->ForwardLink) {

    ProtEntry = CR(Link, PROTOCOL_ENTRY, AllEntries, PROTOCOL_ENTRY_SIGNATURE);

    for ( NotifyLink =  ProtEntry->Notify.ForwardLink; 
          NotifyLink != &ProtEntry->Notify; 
          NotifyLink =  NotifyLink->ForwardLink) {

      ProtNotify = CR(NotifyLink, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);

      if (ProtNotify->Event == Event) {
        RemoveEntryList(&ProtNotify->Link);
        CoreFreePool(ProtNotify);
        CoreReleaseProtocolLock ();
        return EFI_SUCCESS;
      }
    }
  }

  CoreReleaseProtocolLock ();
  return EFI_NOT_FOUND;
}


EFI_STATUS
CoreUnregisterProtocolNotify (
  IN EFI_EVENT      Event
  )
/*++

Routine Description:

  Removes all the events in the protocol database that match Event.

Arguments:
  
  Event   - The event to search for in the protocol database.

Returns:

  EFI_SUCCESS when done searching the entire database.

--*/
{
  EFI_STATUS       Status;

  do {
    Status = CoreUnregisterProtocolNotifyEvent (Event);
  } while (!EFI_ERROR (Status));

  return EFI_SUCCESS;
}



EFI_STATUS
EFIAPI
CoreInstallProtocolInterface (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface
  )
/*++

Routine Description:

  Wrapper function to CoreInstallProtocolInterfaceNotify.  This is the public API which
  Calls the private one which contains a BOOLEAN parameter for notifications

Arguments:

  UserHandle     - The handle to install the protocol handler on,
                    or NULL if a new handle is to be allocated

  Protocol       - The protocol to add to the handle

  InterfaceType  - Indicates whether Interface is supplied in native form.

  Interface      - The interface for the protocol being added

Returns:

  Status code    

--*/
{
  return CoreInstallProtocolInterfaceNotify (
            UserHandle, 
            Protocol, 
            InterfaceType, 
            Interface, 
            TRUE
            );
}

EFI_STATUS
CoreInstallProtocolInterfaceNotify (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface,
  IN BOOLEAN            Notify
  )
/*++

Routine Description:

  Installs a protocol interface into the boot services environment.

Arguments:

  UserHandle     - The handle to install the protocol handler on,
                   or NULL if a new handle is to be allocated

  Protocol       - The protocol to add to the handle

  InterfaceType  - Indicates whether Interface is supplied in native form.

  Interface      - The interface for the protocol being added

  Notify         - indicates whether notify the notification list 
                   for this protocol

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES       - No enough buffer to allocate
  
  EFI_SUCCESS               - Protocol interface successfully installed

--*/
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
  if (UserHandle == NULL || Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterfaceType != EFI_NATIVE_INTERFACE) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Print debug message
  //
  DEBUG((EFI_D_ERROR | EFI_D_INFO, "InstallProtocolInterface: %g %p\n", Protocol, Interface));

  Status = EFI_OUT_OF_RESOURCES;
  Prot = NULL;
  Handle = NULL;

  ASSERT (NULL != gDxeCoreBS);

  if (*UserHandle != NULL_HANDLE) {
    Status = CoreHandleProtocol (*UserHandle, Protocol, (VOID **)&ExistingInterface);
    if (!EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Lock the protocol database 
  //
  CoreAcquireProtocolLock ();

  //
  // Lookup the Protocol Entry for the requested protocol
  //
  ProtEntry = CoreFindProtocolEntry (Protocol, TRUE);
  if (ProtEntry == NULL) {
    goto Done;
  }

  //
  // Allocate a new protocol interface structure
  //
  Prot = CoreAllocateZeroBootServicesPool (sizeof(PROTOCOL_INTERFACE));
  if (Prot == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // If caller didn't supply a handle, allocate a new one
  //
  Handle = (IHANDLE *)*UserHandle;
  if (Handle == NULL) {
    Handle = CoreAllocateZeroBootServicesPool (sizeof(IHANDLE));
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
  } 

  Status = CoreValidateHandle (Handle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Each interface that is added must be unique
  //
  ASSERT (CoreFindProtocolInterface (Handle, Protocol, Interface) == NULL);

  //
  // Initialize the protocol interface structure
  //
  Prot->Signature = PROTOCOL_INTERFACE_SIGNATURE;
  Prot->Handle = Handle;
  Prot->Protocol = ProtEntry;
  Prot->Interface = Interface;

  //
  // Initalize OpenProtocol Data base
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
    CoreNotifyProtocolEntry (ProtEntry);
  }
  Status = EFI_SUCCESS;

Done:
  //
  // Done, unlock the database and return
  //
  CoreReleaseProtocolLock ();
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
      CoreFreePool (Prot);
    }
  }

  return Status;
}



EFI_STATUS
EFIAPI
CoreInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE           *Handle,
  ...
  )
/*++

Routine Description:

  Installs a list of protocol interface into the boot services environment.
  This function calls InstallProtocolInterface() in a loop. If any error
  occures all the protocols added by this function are removed. This is 
  basically a lib function to save space.

Arguments:

  Handle      - The handle to install the protocol handlers on,
                or NULL if a new handle is to be allocated
  ...         - EFI_GUID followed by protocol instance. A NULL terminates the 
                list. The pairs are the arguments to InstallProtocolInterface().
                All the protocols are added to Handle.

Returns:

  EFI_INVALID_PARAMETER       - Handle is NULL.
  
  EFI_SUCCESS                 - Protocol interfaces successfully installed.

--*/
{
  VA_LIST                   args;
  EFI_STATUS                Status;
  EFI_GUID                  *Protocol;
  VOID                      *Interface;
  EFI_TPL                   OldTpl;
  UINTN                     Index;
  EFI_HANDLE                OldHandle;
  EFI_HANDLE                DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Syncronize with notifcations. 
  // 
  OldTpl = CoreRaiseTpl (TPL_NOTIFY);
  OldHandle = *Handle;

  //
  // Check for duplicate device path and install the protocol interfaces
  //
  VA_START (args, Handle);
  for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
    //
    // If protocol is NULL, then it's the end of the list
    //
    Protocol = VA_ARG (args, EFI_GUID *);
    if (Protocol == NULL) {
      break;
    }

    Interface = VA_ARG (args, VOID *);

    //
    // Make sure you are installing on top a device path that has already been added.
    //
    if (CompareGuid (Protocol, &gEfiDevicePathProtocolGuid)) {
      DeviceHandle = NULL;
      DevicePath   = Interface;
      Status = CoreLocateDevicePath (&gEfiDevicePathProtocolGuid, &DevicePath, &DeviceHandle);
      if (!EFI_ERROR (Status) && (DeviceHandle != NULL_HANDLE) && IsDevicePathEnd(DevicePath)) {
        Status = EFI_ALREADY_STARTED;
        continue;
      }
    }
  
    //
    // Install it
    //
    Status = CoreInstallProtocolInterface (Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
  }
  
  //
  // If there was an error, remove all the interfaces that were installed without any errors
  //
  if (EFI_ERROR (Status)) {
    //
    // Reset the va_arg back to the first argument.
    //
    VA_START (args, Handle);
    for (; Index > 1; Index--) {
      Protocol = VA_ARG (args, EFI_GUID *);
      Interface = VA_ARG (args, VOID *);
      CoreUninstallProtocolInterface (*Handle, Protocol, Interface);
    }        
    *Handle = OldHandle;
  }

  //
  // Done
  //
  CoreRestoreTpl (OldTpl);
  return Status;
}

EFI_STATUS
CoreDisconnectControllersUsingProtocolInterface (
  IN EFI_HANDLE           UserHandle,
  IN PROTOCOL_INTERFACE   *Prot
  )
/*++

Routine Description:

  Attempts to disconnect all drivers that are using the protocol interface being queried.
  If failed, reconnect all drivers disconnected.
  
  Note: This function doesn't do parameters checking, it's caller's responsibility 
        to pass in valid parameters.

Arguments:

  UserHandle  - The handle on which the protocol is installed 
  Prot        - The protocol to disconnect drivers from

Returns:

  EFI_SUCCESS       - Drivers using the protocol interface are all disconnected
  EFI_ACCESS_DENIED - Failed to disconnect one or all of the drivers

--*/
{
  EFI_STATUS            Status;
  BOOLEAN               ItemFound;
  LIST_ENTRY            *Link;
  OPEN_PROTOCOL_DATA    *OpenData;

  Status = EFI_SUCCESS;
  
  //
  // Attempt to disconnect all drivers from this protocol interface
  //
  do {
    ItemFound = FALSE;
    for ( Link = Prot->OpenList.ForwardLink;
          (Link != &Prot->OpenList) && !ItemFound;
          Link = Link->ForwardLink ) {
      OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
      if (OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) {
        ItemFound = TRUE;
        CoreReleaseProtocolLock ();
        Status = CoreDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
        CoreAcquireProtocolLock ();
        if (EFI_ERROR (Status)) {
           ItemFound = FALSE;
           break;
        }
      }
    }
  } while (ItemFound);

  if (!EFI_ERROR (Status)) {
    //
    // Attempt to remove BY_HANDLE_PROTOOCL and GET_PROTOCOL and TEST_PROTOCOL Open List items
    //
    do {
      ItemFound = FALSE;
      for ( Link = Prot->OpenList.ForwardLink;
            (Link != &Prot->OpenList) && !ItemFound;
            Link = Link->ForwardLink ) {
        OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
        if (OpenData->Attributes & 
            (EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL | EFI_OPEN_PROTOCOL_GET_PROTOCOL | EFI_OPEN_PROTOCOL_TEST_PROTOCOL)) {
          ItemFound = TRUE;
          RemoveEntryList (&OpenData->Link);  
          Prot->OpenListCount--;
          CoreFreePool (OpenData);
        }
      }
    } while (ItemFound);
  }

  //
  // If there are errors or still has open items in the list, then reconnect all the drivers and return an error
  //
  if (EFI_ERROR (Status) || (Prot->OpenListCount > 0)) {
    CoreReleaseProtocolLock ();
    CoreConnectController (UserHandle, NULL, NULL, TRUE);
    CoreAcquireProtocolLock ();
    Status = EFI_ACCESS_DENIED;
  }

  return Status;
}


EFI_STATUS
EFIAPI
CoreUninstallProtocolInterface (
  IN EFI_HANDLE       UserHandle,
  IN EFI_GUID         *Protocol,
  IN VOID             *Interface
  )
/*++

Routine Description:

  Uninstalls all instances of a protocol:interfacer from a handle. 
  If the last protocol interface is remove from the handle, the 
  handle is freed.

Arguments:

  UserHandle      - The handle to remove the protocol handler from

  Protocol        - The protocol, of protocol:interface, to remove

  Interface       - The interface, of protocol:interface, to remove

Returns:

  EFI_INVALID_PARAMETER       - Protocol is NULL.
  
  EFI_SUCCESS                 - Protocol interface successfully uninstalled.

--*/
{
  EFI_STATUS            Status;
  IHANDLE               *Handle;
  PROTOCOL_INTERFACE    *Prot;

  //
  // Check that Protocol is valid
  //
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check that UserHandle is a valid handle
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Check that Protocol exists on UserHandle, and Interface matches the interface in the database
  //
  Prot = CoreFindProtocolInterface (UserHandle, Protocol, Interface);
  if (Prot == NULL) {
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Attempt to disconnect all drivers that are using the protocol interface that is about to be removed
  //
  Status = CoreDisconnectControllersUsingProtocolInterface (
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
  Prot   = CoreRemoveInterfaceFromProtocol (Handle, Protocol, Interface);

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
    CoreFreePool (Prot);
    Status = EFI_SUCCESS;
  }

  //
  // If there are no more handlers for the handle, free the handle
  //
  if (IsListEmpty (&Handle->Protocols)) {
    Handle->Signature = 0;
    RemoveEntryList (&Handle->AllHandles);
    CoreFreePool (Handle);
  }

Done:  
  //
  // Done, unlock the database and return
  //
  CoreReleaseProtocolLock ();
  return Status;
}



EFI_STATUS
EFIAPI
CoreUninstallMultipleProtocolInterfaces (
  IN EFI_HANDLE           Handle,
  ...
  )
/*++

Routine Description:

  Uninstalls a list of protocol interface in the boot services environment. 
  This function calls UnisatllProtocolInterface() in a loop. This is 
  basically a lib function to save space.

Arguments:

  Handle      - The handle to uninstall the protocol

  ...         - EFI_GUID followed by protocol instance. A NULL terminates the 
                list. The pairs are the arguments to UninstallProtocolInterface().
                All the protocols are added to Handle.

Returns:

  Status code    

--*/
{
  EFI_STATUS      Status;
  VA_LIST         args;
  EFI_GUID        *Protocol;
  VOID            *Interface;
  UINTN           Index;

  VA_START (args, Handle);
  for (Index = 0, Status = EFI_SUCCESS; !EFI_ERROR (Status); Index++) {
    //
    // If protocol is NULL, then it's the end of the list
    //
    Protocol = VA_ARG (args, EFI_GUID *);
    if (Protocol == NULL) {
      break;
    }

    Interface = VA_ARG (args, VOID *);

    //
    // Uninstall it
    //
    Status = CoreUninstallProtocolInterface (Handle, Protocol, Interface);
  }

  //
  // If there was an error, add all the interfaces that were
  // uninstalled without any errors
  //
  if (EFI_ERROR (Status)) {
    //
    // Reset the va_arg back to the first argument.
    //
    VA_START (args, Handle);
    for (; Index > 1; Index--) {
      Protocol = VA_ARG(args, EFI_GUID *);
      Interface = VA_ARG(args, VOID *);
      CoreInstallProtocolInterface (&Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
    }        
  }

  return Status;
}    

STATIC
PROTOCOL_INTERFACE  *
CoreGetProtocolInterface (
  IN  EFI_HANDLE                UserHandle,
  IN  EFI_GUID                  *Protocol
  )
/*++

Routine Description:

  Locate a certain GUID protocol interface in a Handle's protocols.

Arguments:

  UserHandle  - The handle to obtain the protocol interface on

  Protocol    - The GUID of the protocol 

Returns:

  The requested protocol interface for the handle
  
--*/  
{
  EFI_STATUS          Status;
  PROTOCOL_ENTRY      *ProtEntry;
  PROTOCOL_INTERFACE  *Prot;
  IHANDLE             *Handle;
  LIST_ENTRY          *Link;

  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  
  Handle = (IHANDLE *)UserHandle;

  //
  // Look at each protocol interface for a match
  //
  for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
    Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    ProtEntry = Prot->Protocol;
    if (CompareGuid (&ProtEntry->ProtocolID, Protocol)) {
      return Prot;
    }
  }
  return NULL;
}


EFI_STATUS
EFIAPI
CoreHandleProtocol (
  IN EFI_HANDLE       UserHandle,
  IN EFI_GUID         *Protocol,
  OUT VOID            **Interface
  )
/*++

Routine Description:

  Queries a handle to determine if it supports a specified protocol.

Arguments:

  UserHandle  - The handle being queried.

  Protocol    - The published unique identifier of the protocol.

  Interface   - Supplies the address where a pointer to the corresponding Protocol
               Interface is returned.

Returns:

  The requested protocol interface for the handle
  
--*/  
{
  return CoreOpenProtocol (
          UserHandle,     
          Protocol, 
          Interface, 
          gDxeCoreImageHandle, 
          NULL,     
          EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
          );
}


EFI_STATUS
EFIAPI
CoreOpenProtocol (
  IN  EFI_HANDLE                UserHandle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface OPTIONAL,
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  )
/*++

Routine Description:

  Locates the installed protocol handler for the handle, and
  invokes it to obtain the protocol interface. Usage information
  is registered in the protocol data base.

Arguments:

  UserHandle     - The handle to obtain the protocol interface on

  Protocol       - The ID of the protocol 

  Interface      - The location to return the protocol interface

  ImageHandle       - The handle of the Image that is opening the protocol interface
                    specified by Protocol and Interface.
  
  ControllerHandle  - The controller handle that is requiring this interface.

  Attributes     - The open mode of the protocol interface specified by Handle
                    and Protocol.

Returns:

  EFI_INVALID_PARAMETER       - Protocol is NULL.
  
  EFI_SUCCESS                 - Get the protocol interface.
  
--*/
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
  if (Attributes != EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
    if (Interface == NULL) {
      return EFI_INVALID_PARAMETER;
    } else {
      *Interface = NULL;
    }
  }
  
  //
  // Check for invalid UserHandle
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check for invalid Attributes
  //
  switch (Attributes) {
  case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER :
    Status = CoreValidateHandle (ImageHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = CoreValidateHandle (ControllerHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    if (UserHandle == ControllerHandle) {
      return EFI_INVALID_PARAMETER;
    }
    break;
  case EFI_OPEN_PROTOCOL_BY_DRIVER :
  case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE :
    Status = CoreValidateHandle (ImageHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Status = CoreValidateHandle (ControllerHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    break;
  case EFI_OPEN_PROTOCOL_EXCLUSIVE :
    Status = CoreValidateHandle (ImageHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    break;
  case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL :
  case EFI_OPEN_PROTOCOL_GET_PROTOCOL :
  case EFI_OPEN_PROTOCOL_TEST_PROTOCOL :
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Look at each protocol interface for a match
  //
  Prot = CoreGetProtocolInterface (UserHandle, Protocol);
  if (Prot == NULL) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  //
  // This is the protocol interface entry for this protocol
  //    
  if (Attributes != EFI_OPEN_PROTOCOL_TEST_PROTOCOL) {
    *Interface = Prot->Interface;
  }
  Status = EFI_SUCCESS;

  ByDriver        = FALSE;
  Exclusive       = FALSE;
  for ( Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList; Link = Link->ForwardLink) {
    OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
    ExactMatch =  (BOOLEAN)((OpenData->AgentHandle == ImageHandle) && 
                            (OpenData->Attributes == Attributes)  &&
                            (OpenData->ControllerHandle == ControllerHandle));
    if (OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) {
      ByDriver = TRUE;
      if (ExactMatch) {
        Status = EFI_ALREADY_STARTED;
        goto Done;
      }
    }
    if (OpenData->Attributes & EFI_OPEN_PROTOCOL_EXCLUSIVE) {
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
  case EFI_OPEN_PROTOCOL_BY_DRIVER :
    if (Exclusive || ByDriver) {
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }
    break;
  case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE :
  case EFI_OPEN_PROTOCOL_EXCLUSIVE :
    if (Exclusive) {
      Status = EFI_ACCESS_DENIED;
      goto Done;
    }
    if (ByDriver) {
      do {
        Disconnect = FALSE;
        for ( Link = Prot->OpenList.ForwardLink; (Link != &Prot->OpenList) && (!Disconnect); Link = Link->ForwardLink) {
          OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
          if (OpenData->Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) {
            Disconnect = TRUE;
            CoreReleaseProtocolLock ();
            Status = CoreDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
            CoreAcquireProtocolLock ();
            if (EFI_ERROR (Status)) {
              Status = EFI_ACCESS_DENIED;
              goto Done;
            }
          }
        }
      } while (Disconnect);
    } 
    break;
  case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER :
  case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL :
  case EFI_OPEN_PROTOCOL_GET_PROTOCOL :
  case EFI_OPEN_PROTOCOL_TEST_PROTOCOL :
    break;
  }

  if (ImageHandle == NULL) {
    Status = EFI_SUCCESS;
    goto Done;
  }
  //
  // Create new entry
  //
  OpenData = CoreAllocateBootServicesPool (sizeof(OPEN_PROTOCOL_DATA));
  if (OpenData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    OpenData->Signature         = OPEN_PROTOCOL_DATA_SIGNATURE;
    OpenData->AgentHandle       = ImageHandle;
    OpenData->ControllerHandle  = ControllerHandle;
    OpenData->Attributes        = Attributes;
    OpenData->OpenCount         = 1;
    InsertTailList (&Prot->OpenList, &OpenData->Link);
    Prot->OpenListCount++;
    Status = EFI_SUCCESS;
  }

Done:
  //
  // Done. Release the database lock are return
  //
  CoreReleaseProtocolLock ();
  return Status;
}


EFI_STATUS
EFIAPI
CoreCloseProtocol (
  IN  EFI_HANDLE                UserHandle,
  IN  EFI_GUID                  *Protocol,
  IN  EFI_HANDLE                AgentHandle,
  IN  EFI_HANDLE                ControllerHandle  
  )
/*++

Routine Description:

  Closes a protocol on a handle that was opened using OpenProtocol().

Arguments:

  UserHandle       -  The handle for the protocol interface that was previously opened
                      with OpenProtocol(), and is now being closed.
  Protocol         -  The published unique identifier of the protocol. It is the caller's
                      responsibility to pass in a valid GUID.
  AgentHandle      -  The handle of the agent that is closing the protocol interface.
  ControllerHandle -  If the agent that opened a protocol is a driver that follows the
                      EFI Driver Model, then this parameter is the controller handle
                      that required the protocol interface. If the agent does not follow
                      the EFI Driver Model, then this parameter is optional and may be NULL.

Returns:

  EFI_SUCCESS             - The protocol instance was closed.
  EFI_INVALID_PARAMETER   - Handle, AgentHandle or ControllerHandle is not a valid EFI_HANDLE. 
  EFI_NOT_FOUND           - Can not find the specified protocol or AgentHandle.
  
--*/
{
  EFI_STATUS          Status;
  PROTOCOL_INTERFACE  *ProtocolInterface;
  LIST_ENTRY          *Link;
  OPEN_PROTOCOL_DATA  *OpenData;

  //
  // Check for invalid parameters
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = CoreValidateHandle (AgentHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (ControllerHandle != NULL_HANDLE) {
    Status = CoreValidateHandle (ControllerHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Look at each protocol interface for a match
  //
  Status = EFI_NOT_FOUND;
  ProtocolInterface = CoreGetProtocolInterface (UserHandle, Protocol);
  if (ProtocolInterface == NULL) {
    goto Done;
  }

  //
  // Walk the Open data base looking for AgentHandle
  //
  Link = ProtocolInterface->OpenList.ForwardLink;
  while (Link != &ProtocolInterface->OpenList) {
    OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
    Link = Link->ForwardLink;
    if ((OpenData->AgentHandle == AgentHandle) && (OpenData->ControllerHandle == ControllerHandle)) {
        RemoveEntryList (&OpenData->Link);  
        ProtocolInterface->OpenListCount--;
        CoreFreePool (OpenData);
        Status = EFI_SUCCESS;
    }
  }

Done:
  //
  // Done. Release the database lock and return.
  //
  CoreReleaseProtocolLock ();
  return Status;
}



EFI_STATUS
EFIAPI
CoreOpenProtocolInformation (
  IN  EFI_HANDLE                          UserHandle,
  IN  EFI_GUID                            *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                               *EntryCount
  )
/*++

Routine Description:

  Return information about Opened protocols in the system

Arguments:

  UserHandle  - The handle to close the protocol interface on

  Protocol    - The ID of the protocol 

  EntryBuffer - A pointer to a buffer of open protocol information in the form of
                EFI_OPEN_PROTOCOL_INFORMATION_ENTRY structures.

  EntryCount  - Number of EntryBuffer entries

Returns:

  
--*/
{
  EFI_STATUS                          Status;
  PROTOCOL_INTERFACE                  *ProtocolInterface;
  LIST_ENTRY                          *Link;
  OPEN_PROTOCOL_DATA                  *OpenData;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *Buffer;
  UINTN                               Count;
  UINTN                               Size;

  *EntryBuffer = NULL;
  *EntryCount = 0;

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Look at each protocol interface for a match
  //
  Status = EFI_NOT_FOUND;
  ProtocolInterface = CoreGetProtocolInterface (UserHandle, Protocol);
  if (ProtocolInterface == NULL) {
    goto Done;
  }

  //
  // Count the number of Open Entries
  //
  for ( Link = ProtocolInterface->OpenList.ForwardLink, Count = 0; 
        (Link != &ProtocolInterface->OpenList) ;
        Link = Link->ForwardLink  ) {
    Count++;
  } 

  ASSERT (Count == ProtocolInterface->OpenListCount);

  if (Count == 0) {
    Size = sizeof(EFI_OPEN_PROTOCOL_INFORMATION_ENTRY);
  } else {
    Size = Count * sizeof(EFI_OPEN_PROTOCOL_INFORMATION_ENTRY);
  }

  Buffer = CoreAllocateBootServicesPool (Size);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = EFI_SUCCESS;
  for ( Link = ProtocolInterface->OpenList.ForwardLink, Count = 0; 
        (Link != &ProtocolInterface->OpenList);
        Link = Link->ForwardLink, Count++  ) {
    OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);

    Buffer[Count].AgentHandle      = OpenData->AgentHandle;
    Buffer[Count].ControllerHandle = OpenData->ControllerHandle;
    Buffer[Count].Attributes       = OpenData->Attributes;
    Buffer[Count].OpenCount        = OpenData->OpenCount;
  } 

  *EntryBuffer = Buffer;
  *EntryCount = Count;
        
Done:
  //
  // Done. Release the database lock are return
  //
  CoreReleaseProtocolLock ();
  return Status;
}



EFI_STATUS
EFIAPI
CoreProtocolsPerHandle (
  IN EFI_HANDLE       UserHandle,
  OUT EFI_GUID        ***ProtocolBuffer,
  OUT UINTN           *ProtocolBufferCount
  )
/*++

Routine Description:

  Retrieves the list of protocol interface GUIDs that are installed on a handle in a buffer allocated
 from pool.

Arguments:

  UserHandle           - The handle from which to retrieve the list of protocol interface
                          GUIDs.

  ProtocolBuffer       - A pointer to the list of protocol interface GUID pointers that are
                          installed on Handle.

  ProtocolBufferCount  - A pointer to the number of GUID pointers present in
                          ProtocolBuffer.

Returns:
  EFI_SUCCESS   -  The list of protocol interface GUIDs installed on Handle was returned in
                   ProtocolBuffer. The number of protocol interface GUIDs was
                   returned in ProtocolBufferCount.
  EFI_INVALID_PARAMETER   -  Handle is NULL.
  EFI_INVALID_PARAMETER   -  Handle is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER   -  ProtocolBuffer is NULL.
  EFI_INVALID_PARAMETER   -  ProtocolBufferCount is NULL.
  EFI_OUT_OF_RESOURCES    -  There is not enough pool memory to store the results.
  
--*/
{
  EFI_STATUS                          Status;
  IHANDLE                             *Handle;
  PROTOCOL_INTERFACE                  *Prot;
  LIST_ENTRY                          *Link;
  UINTN                               ProtocolCount;
  EFI_GUID                            **Buffer;

  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Handle = (IHANDLE *)UserHandle;

  if (ProtocolBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProtocolBufferCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ProtocolBufferCount = 0;

  ProtocolCount = 0;

  CoreAcquireProtocolLock ();
  
  for (Link = Handle->Protocols.ForwardLink; Link != &Handle->Protocols; Link = Link->ForwardLink) {
    ProtocolCount++;
  }

  //
  // If there are no protocol interfaces installed on Handle, then Handle is not a valid EFI_HANDLE
  //
  if (ProtocolCount == 0) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  Buffer = CoreAllocateBootServicesPool (sizeof (EFI_GUID *) * ProtocolCount);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  *ProtocolBuffer = Buffer;
  *ProtocolBufferCount = ProtocolCount;

  for ( Link = Handle->Protocols.ForwardLink, ProtocolCount = 0;
        Link != &Handle->Protocols; 
        Link = Link->ForwardLink, ProtocolCount++) {
    Prot = CR(Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    Buffer[ProtocolCount] = &(Prot->Protocol->ProtocolID);
  }
  Status = EFI_SUCCESS;

Done:
  CoreReleaseProtocolLock ();
  return Status;
}


UINT64
CoreGetHandleDatabaseKey (
  VOID
  )
/*++

Routine Description:

  return handle database key.

Arguments:

  None
  
Returns:
  
  Handle database key.
  
--*/
{
  return gHandleDatabaseKey;
}


VOID
CoreConnectHandlesByKey (
  UINT64  Key
  )
/*++

Routine Description:

  Go connect any handles that were created or modified while a image executed.

Arguments:

  Key  -  The Key to show that the handle has been created/modified

Returns:
  
  None
--*/
{
  UINTN           Count;
  LIST_ENTRY      *Link;
  EFI_HANDLE      *HandleBuffer;
  IHANDLE         *Handle;
  UINTN           Index;

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  for (Link = gHandleList.ForwardLink, Count = 0; Link != &gHandleList; Link = Link->ForwardLink) {
    Handle = CR (Link, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
    if (Handle->Key > Key) {
      Count++;
    }
  }

  HandleBuffer = CoreAllocateBootServicesPool (Count * sizeof (EFI_HANDLE));
  if (HandleBuffer == NULL) {
    CoreReleaseProtocolLock ();
    return;
  }
  
  for (Link = gHandleList.ForwardLink, Count = 0; Link != &gHandleList; Link = Link->ForwardLink) {
    Handle = CR (Link, IHANDLE, AllHandles, EFI_HANDLE_SIGNATURE);
    if (Handle->Key > Key) {
      HandleBuffer[Count++] = Handle;
    }
  }

  //
  // Unlock the protocol database
  //
  CoreReleaseProtocolLock ();

  //
  // Connect all handles whose Key value is greater than Key
  //
  for (Index = 0; Index < Count; Index++) {
    CoreConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
  }
  
  CoreFreePool(HandleBuffer);
}
