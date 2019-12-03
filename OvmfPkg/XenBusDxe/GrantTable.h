/** @file
  Grant Table function declaration.

  Grant Table are used to grant access to certain page of the current
  VM to an other VM.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __GNTTAB_H__
#define __GNTTAB_H__

#include <IndustryStandard/Xen/grant_table.h>

/**
  Initialize the Grant Table at the address MmioAddr.

  @param Dev      A pointer to XENBUS_DEVICE.
  @param MmioAddr An address where the grant table can be mapped into
                  the guest.
**/
VOID
XenGrantTableInit (
  IN XENBUS_DEVICE  *Dev
  );

/**
  Desinitilize the Grant Table.
**/
VOID
XenGrantTableDeinit (
  IN XENBUS_DEVICE  *Dev
  );

/**
  Grant access to the page Frame to the domain DomainId.

  @param This       A pointer to XENBUS_PROTOCOL instance.
  @param DomainId   ID of the domain to grant acces to.
  @param Frame      Frame Number of the page to grant access to.
  @param ReadOnly   Provide read-only or read-write access.
  @param RefPtr     Reference number of the grant will be written to this pointer.
**/
EFI_STATUS
EFIAPI
XenBusGrantAccess (
  IN  XENBUS_PROTOCOL *This,
  IN  domid_t         DomainId,
  IN  UINTN           Frame, // MFN
  IN  BOOLEAN         ReadOnly,
  OUT grant_ref_t     *RefPtr
  );

/**
  End access to grant Ref, previously return by XenBusGrantAccess.

  @param This       A pointer to XENBUS_PROTOCOL instance.
  @param Ref        Reference numeber of a grant previously returned by
                    XenBusGrantAccess.
**/
EFI_STATUS
EFIAPI
XenBusGrantEndAccess (
  IN XENBUS_PROTOCOL  *This,
  IN grant_ref_t      Ref
  );

#endif /* !__GNTTAB_H__ */
