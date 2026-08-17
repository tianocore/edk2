/** @file
  Generic version of arch-specific functionality for DxeLoad.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright 2024 Google LLC

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>
#include <Library/ArmMmuLib.h>
#include <Library/ArmLib.h>

#include "UefiPayloadEntry.h"

#define STACK_SIZE  0x20000

//
// Upper bound on the number of mappable resource descriptor HOBs the
// bootloader may hand over.  Exceeding it is an error rather than a
// reason to map a subset of the address space; see
// ConfigureMmuFromHobs().
//
#define MAX_RESOURCE_HOBS  256

//
// Splitting N input ranges at every distinct boundary yields at most
// 2 * N - 1 elementary intervals, so an output table of 2 * N entries
// plus the zero-Length terminator can never overflow.
//
#define MAX_DESCRIPTORS  (2 * MAX_RESOURCE_HOBS)

//
// One mappable range collected from a resource descriptor HOB, before
// overlaps between HOBs are resolved.
//
typedef struct {
  UINT64                          Start;
  UINT64                          End;
  ARM_MEMORY_REGION_ATTRIBUTES    Attributes;
  UINTN                           Priority;
} MMU_INPUT_REGION;

STATIC ARM_MEMORY_REGION_DESCRIPTOR  mVirtualMemoryTable[MAX_DESCRIPTORS + 1];
STATIC MMU_INPUT_REGION              mInputRegions[MAX_RESOURCE_HOBS];
STATIC UINT64                        mBoundaries[2 * MAX_RESOURCE_HOBS];

/**
  Return the ARM memory attributes for a HOB resource descriptor.

  ResourceType is the primary discriminator: MEMORY_RESERVED covers
  firmware-reserved DRAM (ACPI NVS, the payload FV/HOB/stack), which
  must be write-back so that unaligned accesses do not fault and no
  mismatched-attribute alias of DRAM is created; MEMORY_MAPPED_IO is
  Device.  The cacheability bits in ResourceAttribute state which
  types the range supports, not which type is wanted (the payload's
  MemInfoCallbackMmio() advertises UC|WC|WT|WB on everything), so
  only an unambiguous UNCACHEABLE-only attribute overrides the type
  switch: that is how ECAM published as MEMORY_RESERVED under
  PcdPublishMcfgAsReservedMemory is mapped Device rather than cacheable.

  @param[in]  Resource  The HOB resource descriptor.
  @param[out] Attr      Returned ARM memory attributes.

  @retval TRUE   Descriptor should be mapped with the returned Attr.
  @retval FALSE  Descriptor should be skipped.
**/
STATIC
BOOLEAN
ArmAttributesForResourceHob (
  IN  EFI_HOB_RESOURCE_DESCRIPTOR   *Resource,
  OUT ARM_MEMORY_REGION_ATTRIBUTES  *Attr
  )
{
  EFI_RESOURCE_ATTRIBUTE_TYPE  Ra;

  Ra = Resource->ResourceAttribute;

  if (((Ra & EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE) != 0) &&
      ((Ra & (EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
              EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE)) == 0))
  {
    *Attr = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    return TRUE;
  }

  switch (Resource->ResourceType) {
    case EFI_RESOURCE_SYSTEM_MEMORY:
    case EFI_RESOURCE_MEMORY_RESERVED:
      *Attr = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
      return TRUE;
    case EFI_RESOURCE_MEMORY_MAPPED_IO:
      *Attr = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
      return TRUE;
    default:
      break;
  }

  return FALSE;
}

/**
  Return a priority for an ARM memory attribute, used to resolve
  overlaps between resource descriptor HOBs.

  A cacheable alias of a device aperture is a mismatched-attribute
  alias, and its failure mode is an abort or silent data corruption.
  Mapping a few pages of DRAM as Device only costs performance.  So
  where two descriptors overlap, the device/uncached attribute wins.

  @param[in] Attributes  ARM memory region attributes.

  @return  Priority.  The higher value wins where two ranges overlap.
**/
STATIC
UINTN
ArmAttributePriority (
  IN ARM_MEMORY_REGION_ATTRIBUTES  Attributes
  )
{
  switch (Attributes) {
    case ARM_MEMORY_REGION_ATTRIBUTE_DEVICE:
    case ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED:
      return 1;
    default:
      return 0;
  }
}

/**
  Configure the MMU from the HOB resource descriptors.

  Only called when the payload is entered with the MMU off, i.e.
  from a raw bootloader.  ArmConfigureMmu() then populates a fresh
  translation table, installs it in TTBR0 and enables the MMU.

  ChainloadApp instead installs its own tables (in Reserved pages)
  before ExitBootServices() and enters the payload with the MMU and
  caches on; HandOffToDxeCore() adopts that live translation and
  never reaches this function.  Calling ArmConfigureMmu() with the
  MMU already enabled and an unknown incoming TCR/MAIR is not safe:
  it programs TCR and MAIR while the outgoing TTBR0 is still live,
  and ArmSetTTBR0() performs no TLB invalidation, so stale entries
  from the previous tables stay usable afterwards.

  The bootloader's descriptors may overlap.  The payload emits a
  SYSTEM_MEMORY descriptor covering all of DRAM together with
  MEMORY_RESERVED carve-outs inside it, and some of those carve-outs
  have to be mapped Device rather than cacheable - ECAM published
  under PcdPublishMcfgAsReservedMemory, for one.  ArmConfigureMmu()
  applies the descriptor array in order and a later descriptor
  overwrites an earlier one, so handing it the HOB list as-is would
  make the attribute of an ECAM aperture depend on the order the
  bootloader happened to emit its HOBs in.  Instead the ranges are
  collected, split at every distinct boundary, and each resulting
  interval is given the highest-priority attribute among the ranges
  covering it.  The result has no overlapping entries at all, so it
  does not depend on HOB order.

  @retval EFI_SUCCESS           MMU configured.
  @retval EFI_OUT_OF_RESOURCES  More mappable resource descriptor HOBs
                                than MAX_RESOURCE_HOBS; the address
                                space cannot be mapped in full.
  @retval EFI_NOT_FOUND         No mappable resource descriptor HOB.
  @retval other                 ArmConfigureMmu() failure.
**/
STATIC
EFI_STATUS
ConfigureMmuFromHobs (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR   *Resource;
  ARM_MEMORY_REGION_ATTRIBUTES  Attr;
  ARM_MEMORY_REGION_ATTRIBUTES  IntervalAttr;
  VOID                          *TranslationTableBase;
  UINTN                         TranslationTableSize;
  UINTN                         RegionCount;
  UINTN                         BoundaryCount;
  UINTN                         Count;
  UINTN                         Index;
  UINTN                         Inner;
  UINTN                         Best;
  UINT64                        Base;
  UINT64                        End;
  UINT64                        Value;
  BOOLEAN                       Truncated;
  BOOLEAN                       Found;

  RegionCount = 0;
  Truncated   = FALSE;

  //
  // Collect every mappable range.  Overlaps are resolved below, so the
  // order the HOBs arrive in does not matter here.
  //
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    Resource = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;

    if (ArmAttributesForResourceHob (Resource, &Attr)) {
      if (RegionCount == MAX_RESOURCE_HOBS) {
        Truncated = TRUE;
        break;
      }

      Base = Resource->PhysicalStart & ~(UINT64)EFI_PAGE_MASK;
      End  = ALIGN_VALUE (
               Resource->PhysicalStart + Resource->ResourceLength,
               EFI_PAGE_SIZE
               );

      //
      // A sub-page-aligned MMIO region adjacent to RAM would otherwise
      // produce two entries covering the same page with different
      // attributes.  Warn on bootloader-supplied misalignment and align
      // outward (the end is already ALIGN_VALUE()'d up), rather than
      // asserting on data this code does not control.
      //
      if (Resource->PhysicalStart != Base) {
        DEBUG ((
          DEBUG_WARN,
          "%a: resource at 0x%Lx not page-aligned; aligning outward\n",
          __func__,
          Resource->PhysicalStart
          ));
      }

      if (End > Base) {
        mInputRegions[RegionCount].Start      = Base;
        mInputRegions[RegionCount].End        = End;
        mInputRegions[RegionCount].Attributes = Attr;
        mInputRegions[RegionCount].Priority   = ArmAttributePriority (Attr);
        RegionCount++;
      }
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  //
  // Truncating the HOB walk would leave regions the payload needs out
  // of the page tables, and the failure would surface much later as an
  // unrelated abort.  Fail here instead, so that HandOffToDxeCore()
  // reports the cause.
  //
  if (Truncated) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: more than %u mappable resource descriptor HOBs; "
      "refusing to map a truncated address space\n",
      __func__,
      (UINT32)MAX_RESOURCE_HOBS
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  if (RegionCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: no mappable resource descriptor HOB\n",
      __func__
      ));
    return EFI_NOT_FOUND;
  }

  //
  // Collect, sort and deduplicate the range boundaries.  Every interval
  // between two adjacent boundaries is covered by a constant set of
  // input ranges and so has one unambiguous attribute.
  //
  BoundaryCount = 0;
  for (Index = 0; Index < RegionCount; Index++) {
    mBoundaries[BoundaryCount++] = mInputRegions[Index].Start;
    mBoundaries[BoundaryCount++] = mInputRegions[Index].End;
  }

  for (Index = 1; Index < BoundaryCount; Index++) {
    Value = mBoundaries[Index];
    for (Inner = Index; (Inner > 0) && (mBoundaries[Inner - 1] > Value); Inner--) {
      mBoundaries[Inner] = mBoundaries[Inner - 1];
    }

    mBoundaries[Inner] = Value;
  }

  Count = 0;
  for (Index = 0; Index < BoundaryCount; Index++) {
    if ((Count == 0) || (mBoundaries[Count - 1] != mBoundaries[Index])) {
      mBoundaries[Count++] = mBoundaries[Index];
    }
  }

  BoundaryCount = Count;

  //
  // Emit one descriptor per interval, giving it the highest-priority
  // attribute among the ranges covering it, and coalescing adjacent
  // intervals that resolved to the same attribute.
  //
  Count = 0;
  for (Index = 0; (Index + 1) < BoundaryCount; Index++) {
    Base         = mBoundaries[Index];
    End          = mBoundaries[Index + 1];
    Found        = FALSE;
    Best         = 0;
    IntervalAttr = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

    for (Inner = 0; Inner < RegionCount; Inner++) {
      if ((mInputRegions[Inner].Start > Base) || (mInputRegions[Inner].End < End)) {
        continue;
      }

      if (!Found || (mInputRegions[Inner].Priority > Best)) {
        Best         = mInputRegions[Inner].Priority;
        IntervalAttr = mInputRegions[Inner].Attributes;
        Found        = TRUE;
      }
    }

    //
    // A gap between two input ranges stays unmapped.
    //
    if (!Found) {
      continue;
    }

    if ((Count > 0) &&
        (mVirtualMemoryTable[Count - 1].Attributes == IntervalAttr) &&
        (mVirtualMemoryTable[Count - 1].PhysicalBase +
         mVirtualMemoryTable[Count - 1].Length == Base))
    {
      mVirtualMemoryTable[Count - 1].Length += End - Base;
      continue;
    }

    ASSERT (Count < MAX_DESCRIPTORS);
    mVirtualMemoryTable[Count].PhysicalBase = Base;
    mVirtualMemoryTable[Count].VirtualBase  = Base;
    mVirtualMemoryTable[Count].Length       = End - Base;
    mVirtualMemoryTable[Count].Attributes   = IntervalAttr;
    Count++;
  }

  //
  // The table is built from a sorted, deduplicated boundary list, so no
  // two entries can overlap.  Assert it rather than assume it: a
  // violation would mean two descriptors with different attributes
  // cover the same page, which is the mismatched-attribute alias this
  // flattening exists to eliminate, and whose outcome would once again
  // depend on the order the descriptors are applied in.
  //
  for (Index = 1; Index < Count; Index++) {
    ASSERT (
      mVirtualMemoryTable[Index - 1].PhysicalBase +
      mVirtualMemoryTable[Index - 1].Length <=
      mVirtualMemoryTable[Index].PhysicalBase
      );
  }

  ZeroMem (&mVirtualMemoryTable[Count], sizeof (mVirtualMemoryTable[Count]));

  DEBUG ((
    DEBUG_INFO,
    "%a: mapping %u regions from %u resource HOBs\n",
    __func__,
    (UINT32)Count,
    (UINT32)RegionCount
    ));

  return ArmConfigureMmu (
           mVirtualMemoryTable,
           &TranslationTableBase,
           &TranslationTableSize
           );
}

/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.
   It also installs EFI_END_OF_PEI_PPI to signal the end of PEI phase.

   @param DxeCoreEntryPoint         The entry point of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  VOID        *BaseOfStack;
  VOID        *TopOfStack;
  EFI_STATUS  Status;

  if (ArmMmuEnabled ()) {
    //
    // ChainloadApp entered the payload with the MMU and caches on and
    // its own translation tables live: those tables are pinned by an
    // EfiBootServicesData memory-allocation HOB emitted from the
    // launcher's boot-time reservation list, so DXE cannot allocate
    // over them and the OS reclaims them once it has installed its
    // own translation.  ArmPkg's CpuDxe
    // (ArmPkg/Drivers/CpuDxe/AArch64/Mmu.c) begins with
    // ASSERT(ArmMmuEnabled()) and manages memory attributes by editing
    // the live tables via ArmSetMemoryAttributes(); it never needs a
    // freshly-built hierarchy.  Adopt the incoming translation as-is.
    //
    DEBUG ((DEBUG_INFO, "HandOffToDxeCore: MMU already enabled, adopting live translation\n"));
  } else {
    //
    // Raw-bootloader path: build our own page tables from the
    // resource-descriptor HOBs.  ArmConfigureMmu() populates a table
    // in payload-owned pages, installs it in TTBR0 and enables the
    // MMU.
    //
    Status = ConfigureMmuFromHobs ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "HandOffToDxeCore: Failed to enable MMU: %r\n", Status));
      CpuDeadLoop ();
    }
  }

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack, STACK_SIZE);

  //
  // Transfer the control to the entry point of DxeCore.
  //
  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    HobList.Raw,
    NULL,
    TopOfStack
    );
}
