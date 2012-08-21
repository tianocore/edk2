/**@file
  This file holds the common functions used by xen paravirtualization.

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <Library/XenHypercallLib.h>
#include <Library/XenLib.h>

/**
  Get shared info page.

  @return A pointer to shared info page.

**/
VOID *
EFIAPI
GetSharedInfo (
  VOID
  )
{
  static EFI_XEN_SHARED_INFO         *SharedInfoPtr = NULL;
  XEN_ADD_TO_PHYSMAP                 AddPhysmap;

  if (SharedInfoPtr != NULL) {
    return (VOID *) SharedInfoPtr;
  }

  AddPhysmap.DomId  = DOMID_SELF; 
  AddPhysmap.Space  = XENMAPSPACE_SHARED_INFO;
  AddPhysmap.Index  = 0;

  SharedInfoPtr   = (EFI_XEN_SHARED_INFO *) AllocatePages (EFI_PAGE_SIZE);
  AddPhysmap.Gpfn = (UINTN)(SharedInfoPtr) >> EFI_PAGE_SHIFT;

  if (HypervisorMemoryOp(XENMEM_ADD_TO_PHYSMAP, &AddPhysmap)) {
    DEBUG ((EFI_D_ERROR, "Hypercall Error: can't get the shared info page.\n"));
    return NULL;
  }

  return (VOID *) SharedInfoPtr;
}

/**
  Test and clear one bit at a gavin address.
  TODO The method needs to be written as a assembly function.

  @param IN BitNum   The number of bit to operate.
  @param IN Addr     The address to operate.

  @return  The old value of the bit.

**/
UINTN
EFIAPI
SyncTestClearBit (
  IN     UINTN                    BitNum,
  IN     VOID                     *Addr
  )
{
  UINTN             OldBit;

  __asm__ __volatile__ (
    "lock          \n\t"
    "btr  %2,  %1  \n\t"
    "sbb  %0,  %0  \n\t"
    : "=r" (OldBit), "=m" (*(volatile long *)Addr)
    : "Ir" (BitNum), "m" (*(volatile long *)Addr) 
    : "memory"
    );

  return OldBit;
}

