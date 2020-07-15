/** @file
  Support functions for managing protocol.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef  _HAND_H_
#define  _HAND_H_


#define EFI_HANDLE_SIGNATURE            SIGNATURE_32('h','n','d','l')

///
/// IHANDLE - contains a list of protocol handles
///
typedef struct {
  UINTN               Signature;
  /// All handles list of IHANDLE
  LIST_ENTRY          AllHandles;
  /// List of PROTOCOL_INTERFACE's for this handle
  LIST_ENTRY          Protocols;
  UINTN               LocateRequest;
  /// The Handle Database Key value when this handle was last created or modified
  UINT64              Key;
} IHANDLE;

#define ASSERT_IS_HANDLE(a)  ASSERT((a)->Signature == EFI_HANDLE_SIGNATURE)

#define PROTOCOL_ENTRY_SIGNATURE        SIGNATURE_32('p','r','t','e')

///
/// PROTOCOL_ENTRY - each different protocol has 1 entry in the protocol
/// database.  Each handler that supports this protocol is listed, along
/// with a list of registered notifies.
///
typedef struct {
  UINTN               Signature;
  /// Link Entry inserted to mProtocolDatabase
  LIST_ENTRY          AllEntries;
  /// ID of the protocol
  EFI_GUID            ProtocolID;
  /// All protocol interfaces
  LIST_ENTRY          Protocols;
  /// Registerd notification handlers
  LIST_ENTRY          Notify;
} PROTOCOL_ENTRY;


#define PROTOCOL_INTERFACE_SIGNATURE  SIGNATURE_32('p','i','f','c')

///
/// PROTOCOL_INTERFACE - each protocol installed on a handle is tracked
/// with a protocol interface structure
///
typedef struct {
  UINTN                       Signature;
  /// Link on IHANDLE.Protocols
  LIST_ENTRY                  Link;
  /// Back pointer
  IHANDLE                     *Handle;
  /// Link on PROTOCOL_ENTRY.Protocols
  LIST_ENTRY                  ByProtocol;
  /// The protocol ID
  PROTOCOL_ENTRY              *Protocol;
  /// The interface value
  VOID                        *Interface;
  /// OPEN_PROTOCOL_DATA list
  LIST_ENTRY                  OpenList;
  UINTN                       OpenListCount;

} PROTOCOL_INTERFACE;

#define OPEN_PROTOCOL_DATA_SIGNATURE  SIGNATURE_32('p','o','d','l')

typedef struct {
  UINTN                       Signature;
  ///Link on PROTOCOL_INTERFACE.OpenList
  LIST_ENTRY                  Link;

  EFI_HANDLE                  AgentHandle;
  EFI_HANDLE                  ControllerHandle;
  UINT32                      Attributes;
  UINT32                      OpenCount;
} OPEN_PROTOCOL_DATA;


#define PROTOCOL_NOTIFY_SIGNATURE       SIGNATURE_32('p','r','t','n')

///
/// PROTOCOL_NOTIFY - used for each register notification for a protocol
///
typedef struct {
  UINTN               Signature;
  PROTOCOL_ENTRY      *Protocol;
  /// All notifications for this protocol
  LIST_ENTRY          Link;
  /// Event to notify
  EFI_EVENT           Event;
  /// Last position notified
  LIST_ENTRY          *Position;
} PROTOCOL_NOTIFY;



/**
  Finds the protocol entry for the requested protocol.
  The gProtocolDatabaseLock must be owned

  @param  Protocol               The ID of the protocol
  @param  Create                 Create a new entry if not found

  @return Protocol entry

**/
PROTOCOL_ENTRY  *
CoreFindProtocolEntry (
  IN EFI_GUID   *Protocol,
  IN BOOLEAN    Create
  );


/**
  Signal event for every protocol in protocol entry.

  @param  ProtEntry              Protocol entry

**/
VOID
CoreNotifyProtocolEntry (
  IN PROTOCOL_ENTRY   *ProtEntry
  );


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
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  );


/**
  Removes Protocol from the protocol list (but not the handle list).

  @param  Handle                 The handle to remove protocol on.
  @param  Protocol               GUID of the protocol to be moved
  @param  Interface              The interface of the protocol

  @return Protocol Entry

**/
PROTOCOL_INTERFACE *
CoreRemoveInterfaceFromProtocol (
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  );


/**
  Connects a controller to a driver.

  @param  ControllerHandle                      Handle of the controller to be
                                                connected.
  @param  ContextDriverImageHandles             DriverImageHandle A pointer to an
                                                ordered list of driver image
                                                handles.
  @param  RemainingDevicePath                   RemainingDevicePath A pointer to
                                                the device path that specifies a
                                                child  of the controller
                                                specified by ControllerHandle.

  @retval EFI_SUCCESS                           One or more drivers were
                                                connected to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES                  No enough system resources to
                                                complete the request.
  @retval EFI_NOT_FOUND                         No drivers were connected to
                                                ControllerHandle.

**/
EFI_STATUS
CoreConnectSingleController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *ContextDriverImageHandles OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath       OPTIONAL
  );

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
  IN EFI_HANDLE           UserHandle,
  IN PROTOCOL_INTERFACE   *Prot
  );


/**
  Acquire lock on gProtocolDatabaseLock.

**/
VOID
CoreAcquireProtocolLock (
  VOID
  );


/**
  Release lock on gProtocolDatabaseLock.

**/
VOID
CoreReleaseProtocolLock (
  VOID
  );


/**
  Check whether a handle is a valid EFI_HANDLE

  @param  UserHandle             The handle to check

  @retval EFI_INVALID_PARAMETER  The handle is NULL or not a valid EFI_HANDLE.
  @retval EFI_SUCCESS            The handle is valid EFI_HANDLE.

**/
EFI_STATUS
CoreValidateHandle (
  IN  EFI_HANDLE                UserHandle
  );

//
// Externs
//
extern EFI_LOCK         gProtocolDatabaseLock;
extern LIST_ENTRY       gHandleList;
extern UINT64           gHandleDatabaseKey;

#endif
