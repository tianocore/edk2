
/** @file
  XenBus protocol to be used between the XenBus bus driver and Xen PV devices.

  DISCLAIMER: the XENBUS_PROTOCOL introduced here is a work in progress, and
  should not be used outside of the EDK II tree.

  This protocol provide the necessary for a Xen PV driver frontend to
  communicate with the bus driver, and perform several task to
  initialize/shutdown a PV device and perform IO with a PV backend.

  Copyright (C) 2014, Citrix Ltd.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PROTOCOL_XENBUS_H__
#define __PROTOCOL_XENBUS_H__

#define XENBUS_PROTOCOL_GUID \
  {0x3d3ca290, 0xb9a5, 0x11e3, {0xb7, 0x5d, 0xb8, 0xac, 0x6f, 0x7d, 0x65, 0xe6}}

///
/// Forward declaration
///
typedef struct _XENBUS_PROTOCOL XENBUS_PROTOCOL;

typedef enum xenbus_state XenBusState;

typedef struct
{
  UINT32 Id;
} XENSTORE_TRANSACTION;

#define XST_NIL ((XENSTORE_TRANSACTION *) NULL)

typedef enum {
  XENSTORE_STATUS_SUCCESS = 0,
  XENSTORE_STATUS_FAIL,
  XENSTORE_STATUS_EINVAL,
  XENSTORE_STATUS_EACCES,
  XENSTORE_STATUS_EEXIST,
  XENSTORE_STATUS_EISDIR,
  XENSTORE_STATUS_ENOENT,
  XENSTORE_STATUS_ENOMEM,
  XENSTORE_STATUS_ENOSPC,
  XENSTORE_STATUS_EIO,
  XENSTORE_STATUS_ENOTEMPTY,
  XENSTORE_STATUS_ENOSYS,
  XENSTORE_STATUS_EROFS,
  XENSTORE_STATUS_EBUSY,
  XENSTORE_STATUS_EAGAIN,
  XENSTORE_STATUS_EISCONN,
  XENSTORE_STATUS_E2BIG
} XENSTORE_STATUS;


#include <IndustryStandard/Xen/grant_table.h>
#include <IndustryStandard/Xen/event_channel.h>

///
/// Function prototypes
///

/**
  Get the contents of the node Node of the PV device. Returns the contents in
  *Result which should be freed after use.

  @param This           A pointer to XENBUS_PROTOCOL instance.
  @param Transaction    The XenStore transaction covering this request.
  @param Node           The basename of the file to read.
  @param Result         The returned contents from this file.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.

  @note The results buffer is malloced and should be free'd by the
        caller.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_READ)(
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Result
  );

/**
  Get the contents of the node Node of the PV device's backend. Returns the
  contents in *Result which should be freed after use.

  @param This           A pointer to XENBUS_PROTOCOL instance.
  @param Transaction    The XenStore transaction covering this request.
  @param Node           The basename of the file to read.
  @param Result         The returned contents from this file.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.

  @note The results buffer is malloced and should be free'd by the
        caller.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_BACKEND_READ)(
  IN  XENBUS_PROTOCOL       *This,
  IN  CONST XENSTORE_TRANSACTION *Transaction,
  IN  CONST CHAR8           *Node,
  OUT VOID                  **Result
  );

/**
  Print formatted write to a XenStore node.

  @param This             A pointer to XENBUS_PROTOCOL instance.
  @param Transaction      The XenStore transaction covering this request.
  @param Directory        The dirname of the path to read.
  @param Node             The basename of the path to read.
  @param Format           AsciiSPrint format string followed by a variable number
                          of arguments.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_PRINTF) (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *Directory,
  IN CONST CHAR8            *Node,
  IN CONST CHAR8            *Format,
  ...
  );

/**
  Remove a node or directory (directories must be empty) of the PV driver's
  subdirectory.

  @param This           A pointer to XENBUS_PROTOCOL instance.
  @param Transaction    The XenStore transaction covering this request.
  @param Node           The basename of the node to remove.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_REMOVE) (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN CONST CHAR8            *Node
  );

/**
  Start a transaction.

  Changes by others will not be seen during the lifetime of this
  transaction, and changes will not be visible to others until it
  is committed (XsTransactionEnd).

  @param This         A pointer to XENBUS_PROTOCOL instance.
  @param Transaction  The returned transaction.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_TRANSACTION_START)(
  IN  XENBUS_PROTOCOL       *This,
  OUT XENSTORE_TRANSACTION  *Transaction
  );

/**
  End a transaction.

  @param This         A pointer to XENBUS_PROTOCOL instance.
  @param Transaction  The transaction to end/commit.
  @param Abort        If TRUE, the transaction is discarded
                      instead of committed.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_XS_TRANSACTION_END) (
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN BOOLEAN                Abort
  );

/**
  Set a new state for the frontend of the PV driver.

  @param This         A pointer to XENBUS_PROTOCOL instance.
  @param Transaction  The transaction to end/commit.
  @param State        The new state to apply.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_SET_STATE)(
  IN XENBUS_PROTOCOL        *This,
  IN CONST XENSTORE_TRANSACTION *Transaction,
  IN XenBusState            State
  );

/**
  Grant access to the page Frame to the domain DomainId.

  @param This       A pointer to XENBUS_PROTOCOL instance.
  @param DomainId   ID of the domain to grant acces to.
  @param Frame      Frame Number of the page to grant access to.
  @param ReadOnly   Provide read-only or read-write access.
  @param RefPtr     Reference number of the grant will be writen to this pointer.
**/
typedef
EFI_STATUS
(EFIAPI *XENBUS_GRANT_ACCESS)(
  IN  XENBUS_PROTOCOL *This,
  IN  domid_t         DomainId,
  IN  UINTN           Frame,
  IN  BOOLEAN         ReadOnly,
  OUT grant_ref_t     *refp
  );

/**
  End access to grant Ref, previously return by XenBusGrantAccess.

  @param This       A pointer to XENBUS_PROTOCOL instance.
  @param Ref        Reference numeber of a grant previously returned by
                    XenBusGrantAccess.
**/
typedef
EFI_STATUS
(EFIAPI *XENBUS_GRANT_END_ACCESS)(
  IN XENBUS_PROTOCOL  *This,
  IN grant_ref_t      Ref
  );

/**
  Allocate a port that can be bind from domain DomainId.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param DomainId   The domain ID that can bind the newly allocated port.
  @param Port       A pointer to a evtchn_port_t that will contain the newly
                    allocated port.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
typedef
UINT32
(EFIAPI *XENBUS_EVENT_CHANNEL_ALLOCATE) (
  IN  XENBUS_PROTOCOL *This,
  IN  domid_t         DomainId,
  OUT evtchn_port_t   *Port
  );

/**
  Send an event to the remote end of the channel whose local endpoint is Port.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Port       Local port to the the event from.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
typedef
UINT32
(EFIAPI *XENBUS_EVENT_CHANNEL_NOTIFY) (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  );

/**
  Close a local event channel Port.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Port       The event channel to close.

  @retval UINT32    The return value from the hypercall, 0 if success.
**/
typedef
UINT32
(EFIAPI *XENBUS_EVENT_CHANNEL_CLOSE) (
  IN XENBUS_PROTOCOL  *This,
  IN evtchn_port_t    Port
  );

/**
  Register a XenStore watch.

  XenStore watches allow a client to wait for changes to an object in the
  XenStore.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Node       The basename of the path to watch.
  @param Token      A token.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.  EEXIST errors from the
           XenStore are supressed, allowing multiple, physically different,
           xenbus_watch objects, to watch the same path in the XenStore.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_REGISTER_WATCH) (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  );

/**
  Register a XenStore watch on a backend's node.

  XenStore watches allow a client to wait for changes to an object in the
  XenStore.

  @param This       A pointer to the XENBUS_PROTOCOL.
  @param Node       The basename of the path to watch.
  @param Token      A token.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of write failure.  EEXIST errors from the
           XenStore are supressed, allowing multiple, physically different,
           xenbus_watch objects, to watch the same path in the XenStore.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_REGISTER_WATCH_BACKEND) (
  IN  XENBUS_PROTOCOL *This,
  IN  CONST CHAR8     *Node,
  OUT VOID            **Token
  );

/**
  Unregister a XenStore watch.

  @param This   A pointer to the XENBUS_PROTOCOL.
  @param Token  An token previously returned by a successful
                call to RegisterWatch ().
**/
typedef
VOID
(EFIAPI *XENBUS_UNREGISTER_WATCH) (
  IN XENBUS_PROTOCOL  *This,
  IN VOID             *Token
  );

/**
  Block until the node watch by Token change.

  @param This   A pointer to the XENBUS_PROTOCOL.
  @param Token  An token previously returned by a successful
                call to RegisterWatch or RegisterWatchBackend.

  @return  On success, XENSTORE_STATUS_SUCCESS. Otherwise an errno value
           indicating the type of failure.
**/
typedef
XENSTORE_STATUS
(EFIAPI *XENBUS_WAIT_FOR_WATCH) (
  IN XENBUS_PROTOCOL  *This,
  IN VOID             *Token
  );


///
/// Protocol structure
///
/// DISCLAIMER: the XENBUS_PROTOCOL introduced here is a work in progress, and
/// should not be used outside of the EDK II tree.
///
struct _XENBUS_PROTOCOL {
  XENBUS_XS_READ                XsRead;
  XENBUS_XS_BACKEND_READ        XsBackendRead;
  XENBUS_XS_PRINTF              XsPrintf;
  XENBUS_XS_REMOVE              XsRemove;
  XENBUS_XS_TRANSACTION_START   XsTransactionStart;
  XENBUS_XS_TRANSACTION_END     XsTransactionEnd;
  XENBUS_SET_STATE              SetState;

  XENBUS_GRANT_ACCESS           GrantAccess;
  XENBUS_GRANT_END_ACCESS       GrantEndAccess;

  XENBUS_EVENT_CHANNEL_ALLOCATE EventChannelAllocate;
  XENBUS_EVENT_CHANNEL_NOTIFY   EventChannelNotify;
  XENBUS_EVENT_CHANNEL_CLOSE    EventChannelClose;

  XENBUS_REGISTER_WATCH         RegisterWatch;
  XENBUS_REGISTER_WATCH_BACKEND RegisterWatchBackend;
  XENBUS_UNREGISTER_WATCH       UnregisterWatch;
  XENBUS_WAIT_FOR_WATCH         WaitForWatch;
  //
  // Protocol data fields
  //
  CONST CHAR8                   *Type;
  UINT16                        DeviceId;
  CONST CHAR8                   *Node;
  CONST CHAR8                   *Backend;
};

extern EFI_GUID gXenBusProtocolGuid;

#endif
