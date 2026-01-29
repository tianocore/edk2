/** @file
  SMM handle & protocol handling.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016 - 2018, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "StandaloneMmCore.h"

//
// mProtocolDatabase     - A list of all protocols in the system.  (simple list for now)
// gHandleList           - A list of all the handles in the system
//
LIST_ENTRY  mProtocolDatabase = INITIALIZE_LIST_HEAD_VARIABLE (mProtocolDatabase);
LIST_ENTRY  gHandleList       = INITIALIZE_LIST_HEAD_VARIABLE (gHandleList);

/**
  Check whether a handle is a valid EFI_HANDLE

  @param  UserHandle             The handle to check

  @retval EFI_INVALID_PARAMETER  The handle is NULL or not a valid EFI_HANDLE.
  @retval EFI_SUCCESS            The handle is valid EFI_HANDLE.

**/
EFI_STATUS
MmValidateHandle (
  IN EFI_HANDLE  UserHandle
  )
{
  IHANDLE  *Handle;

  Handle = (IHANDLE *)UserHandle;
  if (Handle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Handle->Signature != EFI_HANDLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Finds the protocol entry for the requested protocol.

  @param  Protocol               The ID of the protocol
  @param  Create                 Create a new entry if not found

  @return Protocol entry

**/
PROTOCOL_ENTRY  *
MmFindProtocolEntry (
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
MmFindProtocolInterface (
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
  ProtEntry = MmFindProtocolEntry (Protocol, FALSE);
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
  Wrapper function to MmInstallProtocolInterfaceNotify.  This is the public API which
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
MmInstallProtocolInterface (
  IN OUT EFI_HANDLE      *UserHandle,
  IN EFI_GUID            *Protocol,
  IN EFI_INTERFACE_TYPE  InterfaceType,
  IN VOID                *Interface
  )
{
  return MmInstallProtocolInterfaceNotify (
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
MmInstallProtocolInterfaceNotify (
  IN OUT EFI_HANDLE          *UserHandle,
  IN     EFI_GUID            *Protocol,
  IN     EFI_INTERFACE_TYPE  InterfaceType,
  IN     VOID                *Interface,
  IN     BOOLEAN             Notify
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
  DEBUG ((DEBUG_LOAD | DEBUG_INFO, "MmInstallProtocolInterface: %g %p\n", Protocol, Interface));

  Status = EFI_OUT_OF_RESOURCES;
  Prot   = NULL;
  Handle = NULL;

  if (*UserHandle != NULL) {
    Status = MmHandleProtocol (*UserHandle, Protocol, (VOID **)&ExistingInterface);
    if (!EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Lookup the Protocol Entry for the requested protocol
  //
  ProtEntry = MmFindProtocolEntry (Protocol, TRUE);
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
    // Add this handle to the list global list of all handles
    // in the system
    //
    InsertTailList (&gHandleList, &Handle->AllHandles);
  }

  Status = MmValidateHandle (Handle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Each interface that is added must be unique
  //
  ASSERT (MmFindProtocolInterface (Handle, Protocol, Interface) == NULL);

  //
  // Initialize the protocol interface structure
  //
  Prot->Signature = PROTOCOL_INTERFACE_SIGNATURE;
  Prot->Handle    = Handle;
  Prot->Protocol  = ProtEntry;
  Prot->Interface = Interface;

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
    MmNotifyProtocol (Prot);
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
      FreePool (Prot);
    }
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
MmUninstallProtocolInterface (
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
  Status = MmValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Check that Protocol exists on UserHandle, and Interface matches the interface in the database
  //
  Prot = MmFindProtocolInterface (UserHandle, Protocol, Interface);
  if (Prot == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Remove the protocol interface from the protocol
  //
  Status = EFI_NOT_FOUND;
  Handle = (IHANDLE *)UserHandle;
  Prot   = MmRemoveInterfaceFromProtocol (Handle, Protocol, Interface);

  if (Prot != NULL) {
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

  return Status;
}

/**
  Locate a certain GUID protocol interface in a Handle's protocols.

  @param  UserHandle             The handle to obtain the protocol interface on
  @param  Protocol               The GUID of the protocol

  @return The requested protocol interface for the handle

**/
PROTOCOL_INTERFACE  *
MmGetProtocolInterface (
  IN EFI_HANDLE  UserHandle,
  IN EFI_GUID    *Protocol
  )
{
  EFI_STATUS          Status;
  PROTOCOL_ENTRY      *ProtEntry;
  PROTOCOL_INTERFACE  *Prot;
  IHANDLE             *Handle;
  LIST_ENTRY          *Link;

  Status = MmValidateHandle (UserHandle);
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
  @retval EFI_INVALID_PARAMETER  Handle is not a valid EFI_HANDLE..
  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_INVALID_PARAMETER  Interface is NULL.

**/
EFI_STATUS
EFIAPI
MmHandleProtocol (
  IN  EFI_HANDLE  UserHandle,
  IN  EFI_GUID    *Protocol,
  OUT VOID        **Interface
  )
{
  EFI_STATUS          Status;
  PROTOCOL_INTERFACE  *Prot;

  //
  // Check for invalid Protocol
  //
  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for invalid Interface
  //
  if (Interface == NULL) {
    return EFI_INVALID_PARAMETER;
  } else {
    *Interface = NULL;
  }

  //
  // Check for invalid UserHandle
  //
  Status = MmValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Look at each protocol interface for a match
  //
  Prot = MmGetProtocolInterface (UserHandle, Protocol);
  if (Prot == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // This is the protocol interface entry for this protocol
  //
  *Interface = Prot->Interface;

  return EFI_SUCCESS;
}
