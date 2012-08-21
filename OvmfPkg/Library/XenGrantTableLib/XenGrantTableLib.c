/**@file
  This file holds the Xen Grant Table methods, which can ... ... 
  //TODO more discription here

  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>
#include <Library/XenGrantTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define NR_GRANT_FRAMES 4
#define NR_GRANT_ENTRIES (NR_GRANT_FRAMES * EFI_PAGE_SIZE / sizeof(GRANT_ENTRY))
//
// External tools reserve first few grant table entries.
//
#define NR_RESERVED_ENTRIES 8

//
// Shared grant table between domains.
//
GRANT_ENTRY               *GrantTable;

XEN_GRANT_REF             GrantTableList[NR_GRANT_ENTRIES];
EFI_TPL                   OriginalTPL;
EFI_XEN_SHARED_INFO       *HypervisorSharedInfo;

PG_ENTRY                  DemandMapPgt;


/**
  Map an array of MFNs contiguous into virtual address space.

  Mapping a list of frames for storing grant entries. Frames parameter
  is used to store grant table address when grant table being setup,
  nr_gframes is the number of frames to map grant table. Returning
  GNTST_okay means success and negative value means failure.
**/
//EFI_STATUS
VOID *
EFIAPI
GrantTableMapFrames (
  IN     UINTN                          *Frames,
  IN     UINTN                          NumOfFrames
  )
{
  EFI_STATUS                            Status;
  UINTN                                 BitsOfAlignment;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  EFI_HANDLE                            mDriverImageHandle;
  XEN_ADD_TO_PHYSMAP                    AddPhysmap;
  UINTN                                 Index;

  //
  // Allocate the PCI Memory.
  //
  BitsOfAlignment    = 0;
  BaseAddress        = 0;
  mDriverImageHandle = gImageHandle;
  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAnySearchBottomUp, 
                  EfiGcdMemoryTypeMemoryMappedIo, 
                  BitsOfAlignment,
                  (UINT64) (EFI_PAGE_SIZE * NumOfFrames),
                  &BaseAddress,
                  mDriverImageHandle,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "[gbtest]Status = %r\n", Status));
    return NULL;
  }
    
  //
  // Get the grant table
  //
  Index = NumOfFrames - 1;
  do {
    AddPhysmap.DomId  = DOMID_SELF; 
    AddPhysmap.Space  = XENMAPSPACE_GRANT_TABLE;
    AddPhysmap.Index  = Index;

    AddPhysmap.Gpfn = ((UINTN)(&BaseAddress) >> EFI_PAGE_SHIFT) + 1;
    if (HypervisorMemoryOp(XENMEM_ADD_TO_PHYSMAP, &AddPhysmap)) {
      DEBUG ((EFI_D_ERROR, "Grant Table Error: can't add to physmap.\n"));
      return NULL;
    }
  } while (Index--);

  return (VOID *) (UINTN) BaseAddress;
}

/**
  TODO
**/
VOID
EFIAPI
LocalIrqSave (
  IN     UINTN            Flags
  )
{
  VCPU_INFO       *Vcpu;

  Vcpu = &HypervisorSharedInfo->VcpuInfo[0];
  Flags = Vcpu->EvtChnUpcallMask;
  Vcpu->EvtChnUpcallMask = 1;
  MemoryFence();
}

VOID
EFIAPI
LocalIrqRestore (
  IN     UINTN            Flags
  )
{
  VCPU_INFO       *Vcpu;

  MemoryFence();
  Vcpu = &HypervisorSharedInfo->VcpuInfo[0];
  Vcpu->EvtChnUpcallMask = Flags;
  if (Vcpu->EvtChnUpcallMask == 0) {
    MemoryFence();
  }
}

/**
  TODO
**/
VOID
EFIAPI
PutFreeEntry (
  IN     XEN_GRANT_REF            Ref
  )
{
  UINTN       Flags;

  Flags = 0;
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);
  LocalIrqSave(Flags);

  GrantTableList[Ref] = GrantTableList[0];
  GrantTableList[0] = Ref;
  LocalIrqRestore(Flags);
  gBS->RestoreTPL (OriginalTPL);
}

/**
  TODO
**/
XEN_GRANT_REF
EFIAPI
GetFreeEntry (
  VOID
  )
{
  UINT32      Ref;
  UINTN       Flags;

  //
  // We use the TPL Notify to perform an atomic operation.
  //
  Flags = 0;
  OriginalTPL = gBS->RaiseTPL (TPL_NOTIFY);
  LocalIrqSave(Flags);

  Ref = GrantTableList[0];
  ASSERT (Ref > NR_RESERVED_ENTRIES && Ref < NR_GRANT_ENTRIES);
  GrantTableList[0] = GrantTableList[Ref];

  LocalIrqRestore(Flags);
  gBS->RestoreTPL (OriginalTPL);

  return Ref;
}

/**
  Init the grant table
**/
VOID
EFIAPI
InitGrantTable (
  VOID
  )
{
  GRANTTABLE_SETUP_TABLE    Setup;
  UINTN                     Frames[NR_GRANT_FRAMES];
  UINTN                     Index;

  //
  // Link all the free grant table entries.
  //
  for (Index = NR_RESERVED_ENTRIES; Index < NR_GRANT_ENTRIES; Index++) {
    PutFreeEntry (Index);
  }

  Setup.Dom      = DOMID_SELF;
  Setup.NrFrames = NR_GRANT_FRAMES;
  Setup.FrameList= (UINTN) Frames;
  //
  // Create a grant table. 
  //
  HypervisorGrantTableOp (GNTTABOP_SETUP_TABLE, &Setup, 1);

  GrantTable = GrantTableMapFrames(Frames, NR_GRANT_FRAMES);
}

/**
  (gnttab_grant_foreign_access())...

  @param IN DomId     ...
  @param IN Frame     ...
  @param IN ReadOnly     ...

  @return  .
  TODO Need to implement now; Done;
**/
XEN_GRANT_REF
EFIAPI
GrantTableGrantAccess (
  IN     DOMID                    DomId,
  IN     UINTN                    Frame,
  IN     UINT32                   ReadOnly
  )
{
//TODO use the blkfront function to have a test
  XEN_GRANT_REF    Ref;

  Ref = GetFreeEntry();
  GrantTable[Ref].Frame = Frame;
  GrantTable[Ref].DomId = DomId;
  MemoryFence();

  ReadOnly *= GTF_READONLY;
  GrantTable[Ref].Flags = GTF_PERMIT_ACCESS | ReadOnly;

  return Ref;
}

/**
  @param IN Ref     ...

  @return  .
  TODO Need to implement now; Done;
**/
UINT32
EFIAPI
GrantTableEndAccess (
  IN     XEN_GRANT_REF            Ref
  )
{
  UINT32    Flags;
  UINT32    Nflags;

  ASSERT (Ref >= NR_GRANT_ENTRIES || Ref < NR_RESERVED_ENTRIES);

  Nflags = GrantTable[Ref].Flags;
  do {
    if ((Flags = Nflags) & (GTF_READING | GTF_WRITING)) {
      DEBUG ((EFI_D_WARN, "WARNING: Grant entry is still in use! (%x)\n", Flags));
      return 0;
    }
  } while ((Nflags = InterlockedCompareExchange32 (&GrantTable[Ref].Flags, Flags, 0)) != Flags);
  //} while ((Nflags = InternalSyncCompareExchange64 (&GrantTable[Ref].Flags, Flags, 0)) != Flags);

  PutFreeEntry(Ref);
  return 1;
}

/**
  ...
**/
VOID
EFIAPI
FinishGrantTable (
  VOID
  )
{
  GRANTTABLE_SETUP_TABLE    Setup;

  Setup.Dom      = DOMID_SELF;
  Setup.NrFrames = 0;

  HypervisorGrantTableOp (GNTTABOP_SETUP_TABLE, &Setup, 1);
}

