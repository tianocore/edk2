/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  notify.c

Abstract:

  EFI notify infrastructure



Revision History

--*/

#include <DxeMain.h>

VOID
CoreNotifyProtocolEntry (
  IN PROTOCOL_ENTRY   *ProtEntry
  )
/*++

Routine Description:

  Signal event for every protocol in protocol entry.

Arguments:

  ProtEntry     - Protocol entry

Returns:

--*/
{
  PROTOCOL_NOTIFY     *ProtNotify;
  LIST_ENTRY          *Link;

  ASSERT_LOCKED (&gProtocolDatabaseLock);

  for (Link=ProtEntry->Notify.ForwardLink; Link != &ProtEntry->Notify; Link=Link->ForwardLink) {
    ProtNotify = CR(Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);
    CoreSignalEvent (ProtNotify->Event);
  }
}


PROTOCOL_INTERFACE *
CoreRemoveInterfaceFromProtocol (
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  )
/*++

Routine Description:

  Removes Protocol from the protocol list (but not the handle list).

Arguments:

  Handle -  The handle to remove protocol on.

  Protocol  -  GUID of the protocol to be moved

  Interface - The interface of the protocol

Returns:

  Protocol Entry

--*/
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  ASSERT_LOCKED (&gProtocolDatabaseLock);

  Prot = CoreFindProtocolInterface (Handle, Protocol, Interface);
  if (Prot != NULL) {

    ProtEntry = Prot->Protocol;

    //
    // If there's a protocol notify location pointing to this entry, back it up one
    //

    for(Link = ProtEntry->Notify.ForwardLink; Link != &ProtEntry->Notify; Link=Link->ForwardLink) {
      ProtNotify = CR(Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);

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



EFI_STATUS
EFIAPI
CoreRegisterProtocolNotify (
  IN EFI_GUID       *Protocol,
  IN EFI_EVENT      Event,
  OUT  VOID           **Registration
  )
/*++

Routine Description:

  Add a new protocol notification record for the request protocol.

Arguments:

  Protocol      - The requested protocol to add the notify registration

  Event         - The event to signal 

  Registration  - Returns the registration record


Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter

  EFI_SUCCESS                 - Successfully returned the registration record that has been added
  
--*/
{
  PROTOCOL_ENTRY      *ProtEntry;
  PROTOCOL_NOTIFY     *ProtNotify;
  EFI_STATUS        Status;
  
  if ((Protocol == NULL) || (Event == NULL) || (Registration == NULL))  {
    return EFI_INVALID_PARAMETER;
  }

  CoreAcquireProtocolLock ();

  ProtNotify = NULL;
  
  //
  // Get the protocol entry to add the notification too
  //

  ProtEntry = CoreFindProtocolEntry (Protocol, TRUE);
  if (ProtEntry != NULL) {

    //
    // Allocate a new notification record
    //

    ProtNotify = CoreAllocateBootServicesPool (sizeof(PROTOCOL_NOTIFY));

    if (ProtNotify != NULL) {
      
      ProtNotify->Signature = PROTOCOL_NOTIFY_SIGNATURE;
      ProtNotify->Protocol = ProtEntry;
      ProtNotify->Event = Event;
      //
      // start at the begining
      //
      ProtNotify->Position = &ProtEntry->Protocols; 

      InsertTailList (&ProtEntry->Notify, &ProtNotify->Link);
    }
  }

  CoreReleaseProtocolLock ();

  //
  // Done.  If we have a protocol notify entry, then return it.
  // Otherwise, we must have run out of resources trying to add one
  //

  Status = EFI_OUT_OF_RESOURCES;
  if (ProtNotify != NULL) {
    *Registration = ProtNotify;
    Status = EFI_SUCCESS;
  }

  return Status;
}



EFI_STATUS
EFIAPI
CoreReinstallProtocolInterface (
  IN EFI_HANDLE     UserHandle,
  IN EFI_GUID       *Protocol,
  IN VOID           *OldInterface,
  IN VOID           *NewInterface
  )
/*++

Routine Description:

  Reinstall a protocol interface on a device handle.  The OldInterface for Protocol is replaced by the NewInterface.

Arguments:

  UserHandle    - Handle on which the interface is to be reinstalled
  Protocol      - The numeric ID of the interface
  OldInterface  - A pointer to the old interface
  NewInterface  - A pointer to the new interface 


Returns:

  Status code.

  On EFI_SUCCESS            The protocol interface was installed
  On EFI_NOT_FOUND          The OldInterface on the handle was not found
  On EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  
--*/
{
  EFI_STATUS                Status;
  IHANDLE                   *Handle;
  PROTOCOL_INTERFACE        *Prot;
  PROTOCOL_ENTRY            *ProtEntry;

  Status = CoreValidateHandle (UserHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Protocol == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Handle = (IHANDLE *) UserHandle;

  //
  // Lock the protocol database
  //
  CoreAcquireProtocolLock ();

  //
  // Check that Protocol exists on UserHandle, and Interface matches the interface in the database
  //
  Prot = CoreFindProtocolInterface (UserHandle, Protocol, OldInterface);
  if (Prot == NULL) {
    CoreReleaseProtocolLock ();
    return EFI_NOT_FOUND;
  }

  //
  // Attempt to disconnect all drivers that are using the protocol interface that is about to be reinstalled
  //
  Status = CoreDisconnectControllersUsingProtocolInterface (
             UserHandle,
             Prot
             );
  if (EFI_ERROR (Status)) {
    //
    // One or more drivers refused to release, so return the error
    //
    CoreReleaseProtocolLock ();
    return Status;
  }

  //
  // Remove the protocol interface from the protocol
  //
  Prot = CoreRemoveInterfaceFromProtocol (Handle, Protocol, OldInterface);

  if (Prot == NULL) {
    CoreReleaseProtocolLock ();
    return EFI_NOT_FOUND;
  }

  ProtEntry = Prot->Protocol;

  //
  // Update the interface on the protocol
  //
  Prot->Interface = NewInterface;

  //
  // Add this protocol interface to the tail of the
  // protocol entry
  //
  InsertTailList (&ProtEntry->Protocols, &Prot->ByProtocol);

  //
  // Update the Key to show that the handle has been created/modified
  //
  gHandleDatabaseKey++;
  Handle->Key = gHandleDatabaseKey;

  //
  // Release the lock and connect all drivers to UserHandle
  //
  CoreReleaseProtocolLock ();
  Status = CoreConnectController (
                  UserHandle, 
                  NULL, 
                  NULL, 
                  TRUE
                  );
  CoreAcquireProtocolLock ();
  
  //
  // Notify the notification list for this protocol
  //
  CoreNotifyProtocolEntry (ProtEntry);

  CoreReleaseProtocolLock ();
  
  return EFI_SUCCESS;
}
