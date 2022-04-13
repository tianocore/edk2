/** @file
  UEFI handle & protocol handling.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include "Handle.h"

//
// mProtocolDatabase     - A list of all protocols in the system.  (simple list for now)
// gHandleList           - A list of all the handles in the system
// gProtocolDatabaseLock - Lock to protect the mProtocolDatabase
// gHandleDatabaseKey    -  The Key to show that the handle has been created/modified
//
LIST_ENTRY  mProtocolDatabase     = INITIALIZE_LIST_HEAD_VARIABLE (mProtocolDatabase);
LIST_ENTRY  gHandleList           = INITIALIZE_LIST_HEAD_VARIABLE (gHandleList);
EFI_LOCK    gProtocolDatabaseLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_NOTIFY);
UINT64      gHandleDatabaseKey    = 0;

/**
  Acquire lock on gProtocolDatabaseLock.

**/
VOID
CoreAcquireProtocolLock (
  VOID
  )
{
  CoreAcquireLock (&gProtocolDatabaseLock);
}

/**
  Release lock on gProtocolDatabaseLock.

**/
VOID
CoreReleaseProtocolLock (
  VOID
  )
{
  CoreReleaseLock (&gProtocolDatabaseLock);
}

/**
  Check whether a handle is a valid EFI_HANDLE
  The gProtocolDatabaseLock must be owned

  @param  UserHandle             The handle to check

  @retval EFI_INVALID_PARAMETER  The handle is NULL or not a valid EFI_HANDLE.
  @retval EFI_SUCCESS            The handle is valid EFI_HANDLE.

**/
EFI_STATUS
CoreValidateHandle (
  IN  EFI_HANDLE  UserHandle
  )
{
  IHANDLE     *Handle;
  LIST_ENTRY  *Link;

  if (UserHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ASSERT_LOCKED (&gProtocolDatabaseLock);

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
  The gProtocolDatabaseLock must be owned

  @param  Protocol               The ID of the protocol
  @param  Create                 Create a new entry if not found

  @return Protocol entry

**/
PROTOCOL_ENTRY  *
CoreFindProtocolEntry (
  IN EFI_GUID  *Protocol,
  IN BOOLEAN   Create
  )
{
  LIST_ENTRY      *Link;
  PROTOCOL_ENTRY  *Item;
  PROTOCOL_ENTRY  *ProtEntry;

  ASSERT_LOCKED (&gProtocolDatabaseLock);

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
CoreFindProtocolInterface (
  IN IHANDLE   *Handle,
  IN EFI_GUID  *Protocol,
  IN VOID      *Interface
  )
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  ASSERT_LOCKED (&gProtocolDatabaseLock);
  Prot = NULL;

  //
  // Lookup the protocol entry for this protocol ID
  //

  ProtEntry = CoreFindProtocolEntry (Protocol, FALSE);
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
  Removes an event from a register protocol notify list on a protocol.

  @param  Event                  The event to search for in the protocol
                                 database.

  @return EFI_SUCCESS   if the event was found and removed.
  @return EFI_NOT_FOUND if the event was not found in the protocl database.

**/
EFI_STATUS
CoreUnregisterProtocolNotifyEvent (
  IN EFI_EVENT  Event
  )
{
  LIST_ENTRY       *Link;
  PROTOCOL_ENTRY   *ProtEntry;
  LIST_ENTRY       *NotifyLink;
  PROTOCOL_NOTIFY  *ProtNotify;

  CoreAcquireProtocolLock ();

  for ( Link =  mProtocolDatabase.ForwardLink;
        Link != &mProtocolDatabase;
        Link =  Link->ForwardLink)
  {
    ProtEntry = CR (Link, PROTOCOL_ENTRY, AllEntries, PROTOCOL_ENTRY_SIGNATURE);

    for ( NotifyLink =  ProtEntry->Notify.ForwardLink;
          NotifyLink != &ProtEntry->Notify;
          NotifyLink =  NotifyLink->ForwardLink)
    {
      ProtNotify = CR (NotifyLink, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);

      if (ProtNotify->Event == Event) {
        RemoveEntryList (&ProtNotify->Link);
        CoreFreePool (ProtNotify);
        CoreReleaseProtocolLock ();
        return EFI_SUCCESS;
      }
    }
  }

  CoreReleaseProtocolLock ();
  return EFI_NOT_FOUND;
}

/**
  Removes all the events in the protocol database that match Event.

  @param  Event                  The event to search for in the protocol
                                 database.

  @return EFI_SUCCESS when done searching the entire database.

**/
EFI_STATUS
CoreUnregisterProtocolNotify (
  IN EFI_EVENT  Event
  )
{
  EFI_STATUS  Status;

  do {
    Status = CoreUnregisterProtocolNotifyEvent (Event);
  } while (!EFI_ERROR (Status));

  return EFI_SUCCESS;
}

/**
  Wrapper function to CoreInstallProtocolInterfaceNotify.  This is the public API which
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
CoreInstallProtocolInterface (
  IN OUT EFI_HANDLE      *UserHandle,
  IN EFI_GUID            *Protocol,
  IN EFI_INTERFACE_TYPE  InterfaceType,
  IN VOID                *Interface
  )
{
  return CoreInstallProtocolInterfaceNotify (
           UserHandle,
           Protocol,
           InterfaceType,
           Interface,
           TRUE
           );
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
CoreInstallProtocolInterfaceNotify (
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
  DEBUG ((DEBUG_INFO, "InstallProtocolInterface: %g %p\n", Protocol, Interface));

  Status = EFI_OUT_OF_RESOURCES;
  Prot   = NULL;
  Handle = NULL;

  if (*UserHandle != NULL) {
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
    Status = CoreValidateHandle (Handle);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "InstallProtocolInterface: input handle at 0x%x is invalid\n", Handle));
      goto Done;
    }
  }

  //
  // Each interface that is added must be unique
  //
  ASSERT (CoreFindProtocolInterface (Handle, Protocol, Interface) == NULL);

  //
  // Initialize the protocol interface structure
  //
  Prot->Signature = PROTOCOL_INTERFACE_SIGNATURE;
  Prot->Handle    = Handle;
  Prot->Protocol  = ProtEntry;
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

    DEBUG ((DEBUG_ERROR, "InstallProtocolInterface: %g %p failed with %r\n", Protocol, Interface, Status));
  }

  return Status;
}

/**
  Installs a list of protocol interface into the boot services environment.
  This function calls InstallProtocolInterface() in a loop. If any error
  occures all the protocols added by this function are removed. This is
  basically a lib function to save space.

  @param  Handle                 The pointer to a handle to install the new
                                 protocol interfaces on, or a pointer to NULL
                                 if a new handle is to be allocated.
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
CoreInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE  *Handle,
  ...
  )
{
  VA_LIST                   Args;
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
  OldTpl    = CoreRaiseTpl (TPL_NOTIFY);
  OldHandle = *Handle;

  //
  // Check for duplicate device path and install the protocol interfaces
  //
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
    // Make sure you are installing on top a device path that has already been added.
    //
    if (CompareGuid (Protocol, &gEfiDevicePathProtocolGuid)) {
      DeviceHandle = NULL;
      DevicePath   = Interface;
      Status       = CoreLocateDevicePath (&gEfiDevicePathProtocolGuid, &DevicePath, &DeviceHandle);
      if (!EFI_ERROR (Status) && (DeviceHandle != NULL) && IsDevicePathEnd (DevicePath)) {
        Status = EFI_ALREADY_STARTED;
        continue;
      }
    }

    //
    // Install it
    //
    Status = CoreInstallProtocolInterface (Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
  }

  VA_END (Args);

  //
  // If there was an error, remove all the interfaces that were installed without any errors
  //
  if (EFI_ERROR (Status)) {
    //
    // Reset the va_arg back to the first argument.
    //
    VA_START (Args, Handle);
    for ( ; Index > 1; Index--) {
      Protocol  = VA_ARG (Args, EFI_GUID *);
      Interface = VA_ARG (Args, VOID *);
      CoreUninstallProtocolInterface (*Handle, Protocol, Interface);
    }

    VA_END (Args);

    *Handle = OldHandle;
  }

  //
  // Done
  //
  CoreRestoreTpl (OldTpl);
  return Status;
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
CoreDisconnectControllersUsingProtocolInterface (
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
        CoreReleaseProtocolLock ();
        Status = CoreDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
        CoreAcquireProtocolLock ();
        if (!EFI_ERROR (Status)) {
          ItemFound = TRUE;
        }

        break;
      }
    }
  } while (ItemFound);

  if (!EFI_ERROR (Status)) {
    //
    // Attempt to remove BY_HANDLE_PROTOOCL and GET_PROTOCOL and TEST_PROTOCOL Open List items
    //
    for (Link = Prot->OpenList.ForwardLink; Link != &Prot->OpenList;) {
      OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);
      if ((OpenData->Attributes &
           (EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL | EFI_OPEN_PROTOCOL_GET_PROTOCOL | EFI_OPEN_PROTOCOL_TEST_PROTOCOL)) != 0)
      {
        Link = RemoveEntryList (&OpenData->Link);
        Prot->OpenListCount--;
        CoreFreePool (OpenData);
      } else {
        Link = Link->ForwardLink;
      }
    }
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
CoreUninstallProtocolInterface (
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
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Check that UserHandle is a valid handle
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

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

/**
  Uninstalls a list of protocol interface in the boot services environment.
  This function calls UninstallProtocolInterface() in a loop. This is
  basically a lib function to save space.

  If any errors are generated while the protocol interfaces are being
  uninstalled, then the protocol interfaces uninstalled prior to the error will
  be reinstalled and EFI_INVALID_PARAMETER will be returned.

  @param  Handle                 The handle to uninstall the protocol interfaces
                                 from.
  @param  ...                    EFI_GUID followed by protocol instance. A NULL
                                 terminates the list. The pairs are the
                                 arguments to UninstallProtocolInterface(). All
                                 the protocols are added to Handle.

  @retval EFI_SUCCESS            if all protocol interfaces where uninstalled.
  @retval EFI_INVALID_PARAMETER  if any protocol interface could not be
                                 uninstalled and an attempt was made to
                                 reinstall previously uninstalled protocol
                                 interfaces.
**/
EFI_STATUS
EFIAPI
CoreUninstallMultipleProtocolInterfaces (
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
    Status = CoreUninstallProtocolInterface (Handle, Protocol, Interface);
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
      CoreInstallProtocolInterface (&Handle, Protocol, EFI_NATIVE_INTERFACE, Interface);
    }

    VA_END (Args);
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Locate a certain GUID protocol interface in a Handle's protocols.

  @param  UserHandle             The handle to obtain the protocol interface on
  @param  Protocol               The GUID of the protocol

  @return The requested protocol interface for the handle

**/
PROTOCOL_INTERFACE  *
CoreGetProtocolInterface (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol
  )
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
    Prot      = CR (Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    ProtEntry = Prot->Protocol;
    if (CompareGuid (&ProtEntry->ProtocolID, Protocol)) {
      return Prot;
    }
  }

  return NULL;
}

/**
  Queries a handle to determine if it supports a specified protocol.

  @param  UserHandle             The handle being queried.
  @param  Protocol               The published unique identifier of the protocol.
  @param  Interface              Supplies the address where a pointer to the
                                 corresponding Protocol Interface is returned.

  @retval EFI_SUCCESS            The interface information for the specified protocol was returned.
  @retval EFI_UNSUPPORTED        The device does not support the specified protocol.
  @retval EFI_INVALID_PARAMETER  Handle is NULL..
  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_INVALID_PARAMETER  Interface is NULL.

**/
EFI_STATUS
EFIAPI
CoreHandleProtocol (
  IN EFI_HANDLE  UserHandle,
  IN EFI_GUID    *Protocol,
  OUT VOID       **Interface
  )
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
CoreOpenProtocol (
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
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Check for invalid UserHandle
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Check for invalid Attributes
  //
  switch (Attributes) {
    case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER:
      Status = CoreValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      Status = CoreValidateHandle (ControllerHandle);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      if (UserHandle == ControllerHandle) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_DRIVER:
    case EFI_OPEN_PROTOCOL_BY_DRIVER | EFI_OPEN_PROTOCOL_EXCLUSIVE:
      Status = CoreValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      Status = CoreValidateHandle (ControllerHandle);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      break;
    case EFI_OPEN_PROTOCOL_EXCLUSIVE:
      Status = CoreValidateHandle (ImageHandle);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      break;
    case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:
    case EFI_OPEN_PROTOCOL_GET_PROTOCOL:
    case EFI_OPEN_PROTOCOL_TEST_PROTOCOL:
      break;
    default:
      Status = EFI_INVALID_PARAMETER;
      goto Done;
  }

  //
  // Look at each protocol interface for a match
  //
  Prot = CoreGetProtocolInterface (UserHandle, Protocol);
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
              CoreReleaseProtocolLock ();
              Status = CoreDisconnectController (UserHandle, OpenData->AgentHandle, NULL);
              CoreAcquireProtocolLock ();
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

  //
  // Done. Release the database lock and return
  //
  CoreReleaseProtocolLock ();
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
CoreCloseProtocol (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol,
  IN  EFI_HANDLE  AgentHandle,
  IN  EFI_HANDLE  ControllerHandle
  )
{
  EFI_STATUS          Status;
  PROTOCOL_INTERFACE  *ProtocolInterface;
  LIST_ENTRY          *Link;
  OPEN_PROTOCOL_DATA  *OpenData;

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Check for invalid parameters
  //
  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = CoreValidateHandle (AgentHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (ControllerHandle != NULL) {
    Status = CoreValidateHandle (ControllerHandle);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  if (Protocol == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // Look at each protocol interface for a match
  //
  Status            = EFI_NOT_FOUND;
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
    Link     = Link->ForwardLink;
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

/**
  Return information about Opened protocols in the system

  @param  UserHandle             The handle to close the protocol interface on
  @param  Protocol               The ID of the protocol
  @param  EntryBuffer            A pointer to a buffer of open protocol information in the
                                 form of EFI_OPEN_PROTOCOL_INFORMATION_ENTRY structures.
  @param  EntryCount             Number of EntryBuffer entries

  @retval EFI_SUCCESS            The open protocol information was returned in EntryBuffer,
                                 and the number of entries was returned EntryCount.
  @retval EFI_NOT_FOUND          Handle does not support the protocol specified by Protocol.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to allocate EntryBuffer.

**/
EFI_STATUS
EFIAPI
CoreOpenProtocolInformation (
  IN  EFI_HANDLE                           UserHandle,
  IN  EFI_GUID                             *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  **EntryBuffer,
  OUT UINTN                                *EntryCount
  )
{
  EFI_STATUS                           Status;
  PROTOCOL_INTERFACE                   *ProtocolInterface;
  LIST_ENTRY                           *Link;
  OPEN_PROTOCOL_DATA                   *OpenData;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *Buffer;
  UINTN                                Count;
  UINTN                                Size;

  *EntryBuffer = NULL;
  *EntryCount  = 0;

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Look at each protocol interface for a match
  //
  Status            = EFI_NOT_FOUND;
  ProtocolInterface = CoreGetProtocolInterface (UserHandle, Protocol);
  if (ProtocolInterface == NULL) {
    goto Done;
  }

  //
  // Count the number of Open Entries
  //
  for ( Link = ProtocolInterface->OpenList.ForwardLink, Count = 0;
        (Link != &ProtocolInterface->OpenList);
        Link = Link->ForwardLink  )
  {
    Count++;
  }

  ASSERT (Count == ProtocolInterface->OpenListCount);

  if (Count == 0) {
    Size = sizeof (EFI_OPEN_PROTOCOL_INFORMATION_ENTRY);
  } else {
    Size = Count * sizeof (EFI_OPEN_PROTOCOL_INFORMATION_ENTRY);
  }

  Buffer = AllocatePool (Size);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Status = EFI_SUCCESS;
  for ( Link = ProtocolInterface->OpenList.ForwardLink, Count = 0;
        (Link != &ProtocolInterface->OpenList);
        Link = Link->ForwardLink, Count++  )
  {
    OpenData = CR (Link, OPEN_PROTOCOL_DATA, Link, OPEN_PROTOCOL_DATA_SIGNATURE);

    Buffer[Count].AgentHandle      = OpenData->AgentHandle;
    Buffer[Count].ControllerHandle = OpenData->ControllerHandle;
    Buffer[Count].Attributes       = OpenData->Attributes;
    Buffer[Count].OpenCount        = OpenData->OpenCount;
  }

  *EntryBuffer = Buffer;
  *EntryCount  = Count;

Done:
  //
  // Done. Release the database lock.
  //
  CoreReleaseProtocolLock ();
  return Status;
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
CoreProtocolsPerHandle (
  IN EFI_HANDLE  UserHandle,
  OUT EFI_GUID   ***ProtocolBuffer,
  OUT UINTN      *ProtocolBufferCount
  )
{
  EFI_STATUS          Status;
  IHANDLE             *Handle;
  PROTOCOL_INTERFACE  *Prot;
  LIST_ENTRY          *Link;
  UINTN               ProtocolCount;
  EFI_GUID            **Buffer;

  if (ProtocolBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ProtocolBufferCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ProtocolBufferCount = 0;

  ProtocolCount = 0;

  CoreAcquireProtocolLock ();

  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Handle = (IHANDLE *)UserHandle;

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

  Buffer = AllocatePool (sizeof (EFI_GUID *) * ProtocolCount);
  if (Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  *ProtocolBuffer      = Buffer;
  *ProtocolBufferCount = ProtocolCount;

  for ( Link = Handle->Protocols.ForwardLink, ProtocolCount = 0;
        Link != &Handle->Protocols;
        Link = Link->ForwardLink, ProtocolCount++)
  {
    Prot                  = CR (Link, PROTOCOL_INTERFACE, Link, PROTOCOL_INTERFACE_SIGNATURE);
    Buffer[ProtocolCount] = &(Prot->Protocol->ProtocolID);
  }

  Status = EFI_SUCCESS;

Done:
  CoreReleaseProtocolLock ();
  return Status;
}

/**
  return handle database key.


  @return Handle database key.

**/
UINT64
CoreGetHandleDatabaseKey (
  VOID
  )
{
  return gHandleDatabaseKey;
}

/**
  Go connect any handles that were created or modified while a image executed.

  @param  Key                    The Key to show that the handle has been
                                 created/modified

**/
VOID
CoreConnectHandlesByKey (
  UINT64  Key
  )
{
  UINTN       Count;
  LIST_ENTRY  *Link;
  EFI_HANDLE  *HandleBuffer;
  IHANDLE     *Handle;
  UINTN       Index;

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

  HandleBuffer = AllocatePool (Count * sizeof (EFI_HANDLE));
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

  CoreFreePool (HandleBuffer);
}
