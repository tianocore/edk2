/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  hand.h

Abstract:

  EFI internal protocol definitions



Revision History

--*/

#ifndef  _HAND_H_
#define  _HAND_H_


//
// IHANDLE - contains a list of protocol handles
//

#define EFI_HANDLE_SIGNATURE            EFI_SIGNATURE_32('h','n','d','l')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          AllHandles;     // All handles list of IHANDLE
  LIST_ENTRY          Protocols;      // List of PROTOCOL_INTERFACE's for this handle
  UINTN               LocateRequest;  // 
  UINT64              Key;            // The Handle Database Key value when this handle was last created or modified
} IHANDLE;

#define ASSERT_IS_HANDLE(a)  ASSERT((a)->Signature == EFI_HANDLE_SIGNATURE)


//
// PROTOCOL_ENTRY - each different protocol has 1 entry in the protocol 
// database.  Each handler that supports this protocol is listed, along
// with a list of registered notifies.
//

#define PROTOCOL_ENTRY_SIGNATURE        EFI_SIGNATURE_32('p','r','t','e')
typedef struct {
  UINTN               Signature;
  LIST_ENTRY          AllEntries;             // All entries
  EFI_GUID            ProtocolID;             // ID of the protocol
  LIST_ENTRY          Protocols;              // All protocol interfaces
  LIST_ENTRY          Notify;                 // Registerd notification handlers
} PROTOCOL_ENTRY;

//
// PROTOCOL_INTERFACE - each protocol installed on a handle is tracked
// with a protocol interface structure
//

#define PROTOCOL_INTERFACE_SIGNATURE  EFI_SIGNATURE_32('p','i','f','c')
typedef struct {
  UINTN                       Signature;
  EFI_HANDLE                  Handle;     // Back pointer
  LIST_ENTRY                  Link;       // Link on IHANDLE.Protocols
  LIST_ENTRY                  ByProtocol; // Link on PROTOCOL_ENTRY.Protocols
  PROTOCOL_ENTRY              *Protocol;  // The protocol ID
  VOID                        *Interface; // The interface value
                                          
  LIST_ENTRY                  OpenList;       // OPEN_PROTOCOL_DATA list.
  UINTN                       OpenListCount;  
  
  EFI_HANDLE                  ControllerHandle;

} PROTOCOL_INTERFACE;

#define OPEN_PROTOCOL_DATA_SIGNATURE  EFI_SIGNATURE_32('p','o','d','l')

typedef struct {
  UINTN                       Signature;
  LIST_ENTRY                  Link;

  EFI_HANDLE                  AgentHandle;
  EFI_HANDLE                  ControllerHandle;
  UINT32                      Attributes;
  UINT32                      OpenCount;
} OPEN_PROTOCOL_DATA;


//
// PROTOCOL_NOTIFY - used for each register notification for a protocol
//

#define PROTOCOL_NOTIFY_SIGNATURE       EFI_SIGNATURE_32('p','r','t','n')
typedef struct {
  UINTN               Signature;
  PROTOCOL_ENTRY      *Protocol;
  LIST_ENTRY          Link;                   // All notifications for this protocol
  EFI_EVENT           Event;                  // Event to notify
  LIST_ENTRY          *Position;              // Last position notified
} PROTOCOL_NOTIFY;

//
// Internal prototypes
//


PROTOCOL_ENTRY  *
CoreFindProtocolEntry (
  IN EFI_GUID     *Protocol,
  IN BOOLEAN      Create
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
;

VOID
CoreNotifyProtocolEntry (
  IN PROTOCOL_ENTRY       *ProtEntry
  )
/*++

Routine Description:

  Signal event for every protocol in protocol entry.

Arguments:

  ProtEntry     - Protocol entry

Returns:

--*/
;

PROTOCOL_INTERFACE *
CoreFindProtocolInterface (
  IN IHANDLE              *Handle,
  IN EFI_GUID             *Protocol,
  IN VOID                 *Interface
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
;

PROTOCOL_INTERFACE *
CoreRemoveInterfaceFromProtocol (
  IN IHANDLE              *Handle,
  IN EFI_GUID             *Protocol,
  IN VOID                 *Interface
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
;

EFI_STATUS
CoreUnregisterProtocolNotify (
  IN EFI_EVENT            Event
  )
/*++

Routine Description:

  Removes all the events in the protocol database that match Event.

Arguments:
  
  Event   - The event to search for in the protocol database.

Returns:

  EFI_SUCCESS when done searching the entire database.

--*/
;

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
;

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
;

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
;

EFI_STATUS
CoreValidateHandle (
  IN  EFI_HANDLE                UserHandle
  )
/*++

Routine Description:

  Check whether a handle is a valid EFI_HANDLE
  
Arguments:

  UserHandle		-	The handle to check
  
Returns:

  EFI_INVALID_PARAMETER		-	The handle is NULL or not a valid EFI_HANDLE.
  
  EFI_SUCCESS							-	The handle is valid EFI_HANDLE.

--*/
;

//
// Externs
//

extern EFI_LOCK         gProtocolDatabaseLock;
extern LIST_ENTRY       gHandleList;
extern UINT64           gHandleDatabaseKey;

#endif
