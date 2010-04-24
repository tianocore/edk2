/** @file
  Support functions for UEFI protocol notification infrastructure.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PiSmmCore.h"

/**
  Signal event for every protocol in protocol entry.

  @param  Prot                   Protocol interface

**/
VOID
SmmNotifyProtocol (
  IN PROTOCOL_INTERFACE  *Prot
  )
{
  PROTOCOL_ENTRY   *ProtEntry;
  PROTOCOL_NOTIFY  *ProtNotify;
  LIST_ENTRY       *Link;

  ProtEntry = Prot->Protocol;
  for (Link=ProtEntry->Notify.ForwardLink; Link != &ProtEntry->Notify; Link=Link->ForwardLink) {
    ProtNotify = CR(Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);
    ProtNotify->Function (&ProtEntry->ProtocolID, Prot->Interface, Prot->Handle);
  }
}

/**
  Removes Protocol from the protocol list (but not the handle list).

  @param  Handle                 The handle to remove protocol on.
  @param  Protocol               GUID of the protocol to be moved
  @param  Interface              The interface of the protocol

  @return Protocol Entry

**/
PROTOCOL_INTERFACE *
SmmRemoveInterfaceFromProtocol (
  IN IHANDLE   *Handle,
  IN EFI_GUID  *Protocol,
  IN VOID      *Interface
  )
{
  PROTOCOL_INTERFACE  *Prot;
  PROTOCOL_NOTIFY     *ProtNotify;
  PROTOCOL_ENTRY      *ProtEntry;
  LIST_ENTRY          *Link;

  Prot = SmmFindProtocolInterface (Handle, Protocol, Interface);
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

/**
  Add a new protocol notification record for the request protocol.

  @param  Protocol               The requested protocol to add the notify
                                 registration
  @param  Function               Points to the notification function
  @param  Registration           Returns the registration record

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully returned the registration record
                                 that has been added

**/
EFI_STATUS
EFIAPI
SmmRegisterProtocolNotify (
  IN  CONST EFI_GUID     *Protocol,
  IN  EFI_SMM_NOTIFY_FN  Function,
  OUT VOID               **Registration
  )
{
  PROTOCOL_ENTRY   *ProtEntry;
  PROTOCOL_NOTIFY  *ProtNotify;
  LIST_ENTRY       *Link;
  EFI_STATUS       Status;

  if ((Protocol == NULL) || (Function == NULL) || (Registration == NULL))  {
    return EFI_INVALID_PARAMETER;
  }

  ProtNotify = NULL;

  //
  // Get the protocol entry to add the notification too
  //
  ProtEntry = SmmFindProtocolEntry ((EFI_GUID *) Protocol, TRUE);
  if (ProtEntry != NULL) {
    //
    // Find whether notification already exist
    //
    for (Link = ProtEntry->Notify.ForwardLink;
         Link != &ProtEntry->Notify;
         Link = Link->ForwardLink) {

      ProtNotify = CR(Link, PROTOCOL_NOTIFY, Link, PROTOCOL_NOTIFY_SIGNATURE);
      if (CompareGuid (&ProtNotify->Protocol->ProtocolID, Protocol) &&
          (ProtNotify->Function == Function)) {

        //
        // Notification already exist
        //
        *Registration = ProtNotify;

        return EFI_SUCCESS;
      }
    }

    //
    // Allocate a new notification record
    //
    ProtNotify = AllocatePool (sizeof(PROTOCOL_NOTIFY));
    if (ProtNotify != NULL) {
      ProtNotify->Signature = PROTOCOL_NOTIFY_SIGNATURE;
      ProtNotify->Protocol = ProtEntry;
      ProtNotify->Function = Function;
      //
      // Start at the ending
      //
      ProtNotify->Position = ProtEntry->Protocols.BackLink;

      InsertTailList (&ProtEntry->Notify, &ProtNotify->Link);
    }
  }

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
