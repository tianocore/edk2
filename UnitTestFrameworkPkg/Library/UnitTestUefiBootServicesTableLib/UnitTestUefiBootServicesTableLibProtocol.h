/** @file
  An internal header file for the Unit Test instance of the UEFI Boot Services Table Library.

  This file includes common header files, defines internal structure and functions used by
  the library implementation.

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UEFI_BOOT_SERVICES_TABLE_LIB_UNIT_TEST_PROTOCOL_H_
#define UEFI_BOOT_SERVICES_TABLE_LIB_UNIT_TEST_PROTOCOL_H_

#include "UnitTestUefiBootServicesTableLib.h"

#define EFI_HANDLE_SIGNATURE  SIGNATURE_32('h','n','d','l')

///
/// IHANDLE - contains a list of protocol handles
///
typedef struct {
  UINTN         Signature;
  /// All handles list of IHANDLE
  LIST_ENTRY    AllHandles;
  /// List of PROTOCOL_INTERFACE's for this handle
  LIST_ENTRY    Protocols;
  UINTN         LocateRequest;
  /// The Handle Database Key value when this handle was last created or modified
  UINT64        Key;
} IHANDLE;

#define ASSERT_IS_HANDLE(a)  ASSERT((a)->Signature == EFI_HANDLE_SIGNATURE)

#define PROTOCOL_ENTRY_SIGNATURE  SIGNATURE_32('p','r','t','e')

///
/// PROTOCOL_ENTRY - each different protocol has 1 entry in the protocol
/// database.  Each handler that supports this protocol is listed, along
/// with a list of registered notifies.
///
typedef struct {
  UINTN         Signature;
  /// Link Entry inserted to mProtocolDatabase
  LIST_ENTRY    AllEntries;
  /// ID of the protocol
  EFI_GUID      ProtocolID;
  /// All protocol interfaces
  LIST_ENTRY    Protocols;
  /// Registered notification handlers
  LIST_ENTRY    Notify;
} PROTOCOL_ENTRY;

#define PROTOCOL_INTERFACE_SIGNATURE  SIGNATURE_32('p','i','f','c')

///
/// PROTOCOL_INTERFACE - each protocol installed on a handle is tracked
/// with a protocol interface structure
///
typedef struct {
  UINTN             Signature;
  /// Link on IHANDLE.Protocols
  LIST_ENTRY        Link;
  /// Back pointer
  IHANDLE           *Handle;
  /// Link on PROTOCOL_ENTRY.Protocols
  LIST_ENTRY        ByProtocol;
  /// The protocol ID
  PROTOCOL_ENTRY    *Protocol;
  /// The interface value
  VOID              *Interface;
  /// OPEN_PROTOCOL_DATA list
  LIST_ENTRY        OpenList;
  UINTN             OpenListCount;
} PROTOCOL_INTERFACE;

#define OPEN_PROTOCOL_DATA_SIGNATURE  SIGNATURE_32('p','o','d','l')

typedef struct {
  UINTN         Signature;
  /// Link on PROTOCOL_INTERFACE.OpenList
  LIST_ENTRY    Link;

  EFI_HANDLE    AgentHandle;
  EFI_HANDLE    ControllerHandle;
  UINT32        Attributes;
  UINT32        OpenCount;
} OPEN_PROTOCOL_DATA;

#define PROTOCOL_NOTIFY_SIGNATURE  SIGNATURE_32('p','r','t','n')

///
/// PROTOCOL_NOTIFY - used for each register notification for a protocol
///
typedef struct {
  UINTN             Signature;
  PROTOCOL_ENTRY    *Protocol;
  /// All notifications for this protocol
  LIST_ENTRY        Link;
  /// Event to notify
  EFI_EVENT         Event;
  /// Last position notified
  LIST_ENTRY        *Position;
} PROTOCOL_NOTIFY;

typedef struct {
  EFI_GUID          *Protocol;
  VOID              *SearchKey;
  LIST_ENTRY        *Position;
  PROTOCOL_ENTRY    *ProtEntry;
} LOCATE_POSITION;

typedef
IHANDLE *
(*UNIT_TEST_GET_NEXT) (
  IN OUT LOCATE_POSITION  *Position,
  OUT VOID                **Interface
  );

#endif
