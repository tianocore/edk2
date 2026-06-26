/** @file
  Helper library to map mmio memory regions.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Ensure a range is present in the GCD memory space map as MMIO.

  The input range must already be page-aligned. The function walks the current
  GCD memory space map and adds every overlapping EfiGcdMemoryTypeNonExistent
  descriptor as EfiGcdMemoryTypeMemoryMappedIo with the requested attributes.
  Existing EfiGcdMemoryTypeMemoryMappedIo descriptors are accepted only when
  their capabilities contain all requested attributes. Existing descriptors of
  any other type, or MMIO descriptors without the requested attributes, are
  treated as conflicts.

  This function only ensures that MMIO GCD descriptors exist. It does not set
  memory space attributes.

  @param[in] Base        The page-aligned base address of the MMIO range.
  @param[in] Length      The page-aligned size of the MMIO range, in bytes.
  @param[in] Attributes  The GCD memory space attributes required for the MMIO
                         range.

  @retval EFI_SUCCESS      The range is backed by compatible MMIO descriptors.
  @retval EFI_UNSUPPORTED  The range overlaps an existing non-MMIO descriptor,
                           or an MMIO descriptor without the requested
                           attributes.
  @retval EFI_ABORTED      An existing GCD descriptor is malformed.
  @retval Others           The GCD memory services returned an error.
**/
STATIC
EFI_STATUS
AddMmioMemorySpace (
  IN  UINT64  Base,
  IN  UINT64  Length,
  IN  UINT64  Attributes
  )
{
  EFI_STATUS                       Status;
  UINTN                            Index;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor;
  UINT64                           IntersectionBase;
  UINT64                           IntersectionEnd;

  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    Descriptor = &MemorySpaceMap[Index];

    if (Descriptor->BaseAddress > (MAX_UINT64 - Descriptor->Length)) {
      Status = EFI_ABORTED;
      break;
    }

    IntersectionBase = MAX (Base, Descriptor->BaseAddress);
    IntersectionEnd  = MIN (
                         Base + Length,
                         Descriptor->BaseAddress + Descriptor->Length
                         );
    if (IntersectionBase >= IntersectionEnd) {
      //
      // The descriptor and the aperture don't overlap.
      //
      continue;
    }

    if (Descriptor->GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      Status = gDS->AddMemorySpace (
                      EfiGcdMemoryTypeMemoryMappedIo,
                      IntersectionBase,
                      IntersectionEnd - IntersectionBase,
                      Attributes
                      );

      DEBUG ((
        EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_VERBOSE,
        "%a: %a: add [%Lx, %Lx): %r\n",
        gEfiCallerBaseName,
        __func__,
        IntersectionBase,
        IntersectionEnd,
        Status
        ));
      if (EFI_ERROR (Status)) {
        break;
      }

      continue;
    }

    if ((Descriptor->GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo) ||
        ((Descriptor->Capabilities & Attributes) != Attributes))
    {
      Status = EFI_UNSUPPORTED;
      break;
    }
  } // for

  FreePool (MemorySpaceMap);
  return Status;
}

/**
  Map a range as MMIO in the GCD memory map.

  The requested range is expanded to page boundaries before it is processed.
  Missing GCD memory space descriptors are added as
  EfiGcdMemoryTypeMemoryMappedIo. Existing MMIO descriptors are accepted only
  when their capabilities contain the requested attributes. Existing descriptors
  of any other type are treated as conflicts.

  After the range is backed by compatible MMIO descriptors, the requested GCD
  memory space attributes are applied to the normalized full range.

  If this function fails after adding new GCD MMIO descriptors, the descriptors
  are not rolled back. Callers are expected to treat failures from this function
  as fatal to the current boot path.

  @param[in] Base        The base address of the requested MMIO range.
  @param[in] Length      The size of the requested MMIO range, in bytes.
  @param[in] Attributes  The GCD memory space attributes to apply to the MMIO
                         range.

  @retval EFI_SUCCESS            The full range was mapped as MMIO and
                                 configured with the requested attributes.
  @retval EFI_INVALID_PARAMETER  Length is zero, or the normalized range
                                 overflows the physical address space.
  @retval EFI_UNSUPPORTED        The range overlaps an existing non-MMIO
                                 descriptor, or an MMIO descriptor without the
                                 requested attributes.
  @retval EFI_ABORTED            An existing GCD descriptor is malformed.
  @retval Others                 The GCD memory services returned an error.
**/
EFI_STATUS
EFIAPI
MapMmioMemory (
  IN EFI_PHYSICAL_ADDRESS  Base,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  RegionEnd;

  DEBUG ((
    DEBUG_INFO,
    "Map MMIO Memory: 0x%08lx - 0x%08lx : 0x%08lx\n",
    Base,
    Length,
    Attributes
    ));

  if (Length == 0) {
    return EFI_INVALID_PARAMETER;
  }

  // Check if RegionsBase + Length would overflow
  if ((Base > (MAX_UINT64 - Length))) {
    return EFI_INVALID_PARAMETER;
  }

  RegionEnd = Base + Length;

  // Check if aligning RegionEnd would overflow
  if (RegionEnd > MAX_UINT64 - ALIGN_VALUE_ADDEND (RegionEnd, EFI_PAGE_SIZE)) {
    return EFI_INVALID_PARAMETER;
  }

  RegionEnd = ALIGN_VALUE (RegionEnd, EFI_PAGE_SIZE);

  // Align down Base to page boundary
  Base = Base & ~(EFI_PAGE_SIZE - 1);

  // Calculate the total region size.
  Length = RegionEnd - Base;

  Status = AddMmioMemorySpace (Base, Length, Attributes);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return gDS->SetMemorySpaceAttributes (Base, Length, Attributes);
}
