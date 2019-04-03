/** @file
  Grant Table function implementation.

  Grant Table are used to grant access to certain page of the current
  VM to an other VM.

  Author: Steven Smith (sos22@cam.ac.uk)
  Changes: Grzegorz Milos (gm281@cam.ac.uk)
  Copyright (C) 2006, Cambridge University
  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "XenBusDxe.h"

#include <IndustryStandard/Xen/memory.h>

#include <Library/XenHypercallLib.h>
#include <Library/SynchronizationLib.h>

#include "GrantTable.h"

#define NR_RESERVED_ENTRIES 8

/* NR_GRANT_FRAMES must be less than or equal to that configured in Xen */
#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * EFI_PAGE_SIZE / sizeof(grant_entry_v1_t))

STATIC grant_entry_v1_t *GrantTable = NULL;
STATIC grant_ref_t GrantList[NR_GRANT_ENTRIES];
STATIC EFI_LOCK mGrantListLock;
#ifdef GNT_DEBUG
STATIC BOOLEAN GrantInUseList[NR_GRANT_ENTRIES];
#endif

STATIC
VOID
XenGrantTablePutFreeEntry (
  grant_ref_t Ref
  )
{
  EfiAcquireLock (&mGrantListLock);
#ifdef GNT_DEBUG
  ASSERT (GrantInUseList[Ref]);
  GrantInUseList[Ref] = FALSE;
#endif
  GrantList[Ref] = GrantList[0];
  GrantList[0] = Ref;
  EfiReleaseLock (&mGrantListLock);
}

STATIC
grant_ref_t
XenGrantTableGetFreeEntry (
  VOID
  )
{
  grant_ref_t Ref;

  EfiAcquireLock (&mGrantListLock);
  Ref = GrantList[0];
  ASSERT (Ref >= NR_RESERVED_ENTRIES && Ref < NR_GRANT_ENTRIES);
  GrantList[0] = GrantList[Ref];
#ifdef GNT_DEBUG
  ASSERT (!GrantInUseList[Ref]);
  GrantInUseList[Ref] = TRUE;
#endif
  EfiReleaseLock (&mGrantListLock);
  return Ref;
}

STATIC
grant_ref_t
XenGrantTableGrantAccess (
  IN domid_t  DomainId,
  IN UINTN    Frame,
  IN BOOLEAN  ReadOnly
  )
{
  grant_ref_t Ref;
  UINT16 Flags;

  ASSERT (GrantTable != NULL);
  Ref = XenGrantTableGetFreeEntry ();
  GrantTable[Ref].frame = (UINT32)Frame;
  GrantTable[Ref].domid = DomainId;
  MemoryFence ();
  Flags = GTF_permit_access;
  if (ReadOnly) {
    Flags |= GTF_readonly;
  }
  GrantTable[Ref].flags = Flags;

  return Ref;
}

STATIC
EFI_STATUS
XenGrantTableEndAccess (
  grant_ref_t Ref
  )
{
  UINT16 Flags, OldFlags;

  ASSERT (GrantTable != NULL);
  ASSERT (Ref >= NR_RESERVED_ENTRIES && Ref < NR_GRANT_ENTRIES);

  OldFlags = GrantTable[Ref].flags;
  do {
    if ((Flags = OldFlags) & (GTF_reading | GTF_writing)) {
      DEBUG ((EFI_D_WARN, "WARNING: g.e. still in use! (%x)\n", Flags));
      return EFI_NOT_READY;
    }
    OldFlags = InterlockedCompareExchange16 (&GrantTable[Ref].flags, Flags, 0);
  } while (OldFlags != Flags);

  XenGrantTablePutFreeEntry (Ref);
  return EFI_SUCCESS;
}

VOID
XenGrantTableInit (
  IN XENBUS_DEVICE  *Dev
  )
{
  xen_add_to_physmap_t Parameters;
  INTN Index;
  INTN ReturnCode;

#ifdef GNT_DEBUG
  SetMem(GrantInUseList, sizeof (GrantInUseList), 1);
#endif
  EfiInitializeLock (&mGrantListLock, TPL_NOTIFY);
  for (Index = NR_RESERVED_ENTRIES; Index < NR_GRANT_ENTRIES; Index++) {
    XenGrantTablePutFreeEntry ((grant_ref_t)Index);
  }

  GrantTable = (VOID*)(UINTN) Dev->XenIo->GrantTableAddress;
  for (Index = 0; Index < NR_GRANT_FRAMES; Index++) {
    Parameters.domid = DOMID_SELF;
    Parameters.idx = Index;
    Parameters.space = XENMAPSPACE_grant_table;
    Parameters.gpfn = (xen_pfn_t) ((UINTN) GrantTable >> EFI_PAGE_SHIFT) + Index;
    ReturnCode = XenHypercallMemoryOp (XENMEM_add_to_physmap, &Parameters);
    if (ReturnCode != 0) {
      DEBUG ((EFI_D_ERROR,
        "Xen GrantTable, add_to_physmap hypercall error: %Ld\n",
        (INT64)ReturnCode));
    }
  }
}

VOID
XenGrantTableDeinit (
  XENBUS_DEVICE *Dev
  )
{
  INTN ReturnCode, Index;
  xen_remove_from_physmap_t Parameters;

  if (GrantTable == NULL) {
    return;
  }

  for (Index = NR_GRANT_FRAMES - 1; Index >= 0; Index--) {
    Parameters.domid = DOMID_SELF;
    Parameters.gpfn = (xen_pfn_t) ((UINTN) GrantTable >> EFI_PAGE_SHIFT) + Index;
    DEBUG ((EFI_D_INFO, "Xen GrantTable, removing %Lx\n",
      (UINT64)Parameters.gpfn));
    ReturnCode = XenHypercallMemoryOp (XENMEM_remove_from_physmap, &Parameters);
    if (ReturnCode != 0) {
      DEBUG ((EFI_D_ERROR,
        "Xen GrantTable, remove_from_physmap hypercall error: %Ld\n",
        (INT64)ReturnCode));
    }
  }
  GrantTable = NULL;
}

EFI_STATUS
EFIAPI
XenBusGrantAccess (
  IN  XENBUS_PROTOCOL *This,
  IN  domid_t         DomainId,
  IN  UINTN           Frame, // MFN
  IN  BOOLEAN         ReadOnly,
  OUT grant_ref_t     *RefPtr
  )
{
  *RefPtr = XenGrantTableGrantAccess (DomainId, Frame, ReadOnly);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
XenBusGrantEndAccess (
  IN XENBUS_PROTOCOL  *This,
  IN grant_ref_t      Ref
  )
{
  return XenGrantTableEndAccess (Ref);
}
