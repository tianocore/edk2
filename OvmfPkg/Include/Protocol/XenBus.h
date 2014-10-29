
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

#define XST_NIL ((XENSTORE_TRANSACTION) { 0 })

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

///
/// Function prototypes
///

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


///
/// Protocol structure
///
/// DISCLAIMER: the XENBUS_PROTOCOL introduced here is a work in progress, and
/// should not be used outside of the EDK II tree.
///
struct _XENBUS_PROTOCOL {
  XENBUS_GRANT_ACCESS           GrantAccess;
  XENBUS_GRANT_END_ACCESS       GrantEndAccess;
  //
  // Protocol data fields
  //
};

extern EFI_GUID gXenBusProtocolGuid;

#endif
