/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  Support.c

Abstract:

Revision History:

--*/
#include "EfiLdr.h"

EFI_STATUS
EfiAddMemoryDescriptor(
  UINTN                 *NoDesc,
  EFI_MEMORY_DESCRIPTOR *Desc,
  EFI_MEMORY_TYPE       Type,
  EFI_PHYSICAL_ADDRESS  BaseAddress,
  UINT64                NoPages,
  UINT64                Attribute
  )
{
  UINTN  NumberOfDesc;
  UINT64 Temp;
  UINTN  Index;

  if (NoPages == 0) {
    return EFI_SUCCESS;
  }

  //
  // See if the new memory descriptor needs to be carved out of an existing memory descriptor
  //

  NumberOfDesc = *NoDesc;
  for (Index = 0; Index < NumberOfDesc; Index++) {

    if (Desc[Index].Type == EfiConventionalMemory) {

      Temp = DivU64x32 ((BaseAddress - Desc[Index].PhysicalStart), EFI_PAGE_SIZE) + NoPages;

      if ((Desc[Index].PhysicalStart < BaseAddress) && (Desc[Index].NumberOfPages >= Temp)) {
        if (Desc[Index].NumberOfPages > Temp) {
          Desc[*NoDesc].Type          = EfiConventionalMemory;
          Desc[*NoDesc].PhysicalStart = BaseAddress + MultU64x32 (NoPages, EFI_PAGE_SIZE);
          Desc[*NoDesc].NumberOfPages = Desc[Index].NumberOfPages - Temp;
          Desc[*NoDesc].VirtualStart  = 0;
          Desc[*NoDesc].Attribute     = Desc[Index].Attribute;
          *NoDesc = *NoDesc + 1;
        }
        Desc[Index].NumberOfPages = Temp - NoPages;
      }

      if ((Desc[Index].PhysicalStart == BaseAddress) && (Desc[Index].NumberOfPages == NoPages)) {
        Desc[Index].Type      = Type;
        Desc[Index].Attribute = Attribute;
        return EFI_SUCCESS;
      }

      if ((Desc[Index].PhysicalStart == BaseAddress) && (Desc[Index].NumberOfPages > NoPages)) {
        Desc[Index].NumberOfPages -= NoPages;
        Desc[Index].PhysicalStart += MultU64x32 (NoPages, EFI_PAGE_SIZE);
      }
    }
  }

  //
  // Add the new memory descriptor
  //

  Desc[*NoDesc].Type          = Type;
  Desc[*NoDesc].PhysicalStart = BaseAddress;
  Desc[*NoDesc].NumberOfPages = NoPages;
  Desc[*NoDesc].VirtualStart  = 0;
  Desc[*NoDesc].Attribute     = Attribute;
  *NoDesc = *NoDesc + 1;

  return EFI_SUCCESS;
}

UINTN
FindSpace (
  UINTN                       NoPages,
  IN UINTN                    *NumberOfMemoryMapEntries,
  IN EFI_MEMORY_DESCRIPTOR    *EfiMemoryDescriptor,
  EFI_MEMORY_TYPE             Type,
  UINT64                      Attribute
  )
{
  EFI_PHYSICAL_ADDRESS        MaxPhysicalStart;
  UINT64                      MaxNoPages;
  UINTN                       Index;
  EFI_MEMORY_DESCRIPTOR       *CurrentMemoryDescriptor;

  MaxPhysicalStart = 0;
  MaxNoPages       = 0;
  CurrentMemoryDescriptor = NULL;
  for (Index = 0; Index < *NumberOfMemoryMapEntries; Index++) {
    if (EfiMemoryDescriptor[Index].PhysicalStart + LShiftU64(EfiMemoryDescriptor[Index].NumberOfPages, EFI_PAGE_SHIFT) <= 0x100000) {
      continue;
    }
    if ((EfiMemoryDescriptor[Index].Type == EfiConventionalMemory) && 
        (EfiMemoryDescriptor[Index].NumberOfPages >= NoPages)) {
      if (EfiMemoryDescriptor[Index].PhysicalStart > MaxPhysicalStart) {
        if (EfiMemoryDescriptor[Index].PhysicalStart + LShiftU64(EfiMemoryDescriptor[Index].NumberOfPages, EFI_PAGE_SHIFT) <= 0x100000000ULL) {
          MaxPhysicalStart = EfiMemoryDescriptor[Index].PhysicalStart;
          MaxNoPages       = EfiMemoryDescriptor[Index].NumberOfPages;
          CurrentMemoryDescriptor = &EfiMemoryDescriptor[Index];
        }
      }
    }
    if ((EfiMemoryDescriptor[Index].Type == EfiReservedMemoryType) ||
        (EfiMemoryDescriptor[Index].Type >= EfiACPIReclaimMemory) ) {
      continue;
    }
    if ((EfiMemoryDescriptor[Index].Type == EfiRuntimeServicesCode) ||
        (EfiMemoryDescriptor[Index].Type == EfiRuntimeServicesData)) {
      break;
    }
  }
 
  if (MaxPhysicalStart == 0) {
    return 0;
  }

  if (MaxNoPages != NoPages) {
    CurrentMemoryDescriptor->NumberOfPages = MaxNoPages - NoPages;
    EfiMemoryDescriptor[*NumberOfMemoryMapEntries].Type          = Type;
    EfiMemoryDescriptor[*NumberOfMemoryMapEntries].PhysicalStart = MaxPhysicalStart + LShiftU64(MaxNoPages - NoPages, EFI_PAGE_SHIFT);
    EfiMemoryDescriptor[*NumberOfMemoryMapEntries].NumberOfPages = NoPages;
    EfiMemoryDescriptor[*NumberOfMemoryMapEntries].VirtualStart  = 0;
    EfiMemoryDescriptor[*NumberOfMemoryMapEntries].Attribute     = Attribute;
    *NumberOfMemoryMapEntries = *NumberOfMemoryMapEntries + 1;
  } else {
    CurrentMemoryDescriptor->Type      = Type;
    CurrentMemoryDescriptor->Attribute = Attribute;
  }

  return (UINTN)(MaxPhysicalStart + LShiftU64(MaxNoPages - NoPages, EFI_PAGE_SHIFT));
}

VOID
GenMemoryMap (
  UINTN                 *NumberOfMemoryMapEntries,
  EFI_MEMORY_DESCRIPTOR *EfiMemoryDescriptor,
  BIOS_MEMORY_MAP       *BiosMemoryMap
  )
{
  UINT64                BaseAddress;
  UINT64                Length;
  EFI_MEMORY_TYPE       Type;
  UINTN                 Index;
  UINTN                 Attr;
  UINT64                Ceiling;

  Ceiling = 0xFFFFFFFF;
  for (Index = 0; Index < BiosMemoryMap->MemoryMapSize / sizeof(BIOS_MEMORY_MAP_ENTRY); Index++) {

    switch (BiosMemoryMap->MemoryMapEntry[Index].Type) { 
    case (INT15_E820_AddressRangeMemory):
      Type = EfiConventionalMemory;
      Attr = EFI_MEMORY_WB;
      break;
    case (INT15_E820_AddressRangeReserved):
      Type = EfiReservedMemoryType;
      Attr = EFI_MEMORY_UC;
      break;
    case (INT15_E820_AddressRangeACPI):
      Type = EfiACPIReclaimMemory;
      Attr = EFI_MEMORY_WB;
      break;
    case (INT15_E820_AddressRangeNVS):
      Type = EfiACPIMemoryNVS;
      Attr = EFI_MEMORY_UC;
      break;
    default:
      // We should not get here, according to ACPI 2.0 Spec.
      // BIOS behaviour of the Int15h, E820h
      Type = EfiReservedMemoryType;
      Attr = EFI_MEMORY_UC;
      break;
    }
    if (Type == EfiConventionalMemory) {
      BaseAddress = BiosMemoryMap->MemoryMapEntry[Index].BaseAddress;
      Length      = BiosMemoryMap->MemoryMapEntry[Index].Length;
      if (BaseAddress & EFI_PAGE_MASK) {
        Length      = Length + (BaseAddress & EFI_PAGE_MASK) - EFI_PAGE_SIZE;
        BaseAddress = LShiftU64 (RShiftU64 (BaseAddress, EFI_PAGE_SHIFT) + 1, EFI_PAGE_SHIFT);
      }
    } else {
      BaseAddress = BiosMemoryMap->MemoryMapEntry[Index].BaseAddress;
      Length      = BiosMemoryMap->MemoryMapEntry[Index].Length + (BaseAddress & EFI_PAGE_MASK);
      BaseAddress = LShiftU64 (RShiftU64 (BaseAddress, EFI_PAGE_SHIFT), EFI_PAGE_SHIFT);
      if (Length & EFI_PAGE_MASK) {
        Length = LShiftU64 (RShiftU64 (Length, EFI_PAGE_SHIFT) + 1, EFI_PAGE_SHIFT);
      }
      //
      // Update Memory Ceiling
      //
      if ((BaseAddress >= 0x100000) && (BaseAddress < 0x100000000ULL)) {
        if (Ceiling > BaseAddress) {
          Ceiling = BaseAddress;
        }
      }
    }
    EfiAddMemoryDescriptor (
      NumberOfMemoryMapEntries,
      EfiMemoryDescriptor,
      Type,
      (EFI_PHYSICAL_ADDRESS)BaseAddress,
      RShiftU64 (Length, EFI_PAGE_SHIFT),
      Attr
      );
  }

  //
  // Update MemoryMap according to Ceiling
  //
  for (Index = 0; Index < *NumberOfMemoryMapEntries; Index++) {
    if ((EfiMemoryDescriptor[Index].Type == EfiConventionalMemory) &&
        (EfiMemoryDescriptor[Index].PhysicalStart > 0x100000) && 
        (EfiMemoryDescriptor[Index].PhysicalStart < 0x100000000ULL)) {
      if (EfiMemoryDescriptor[Index].PhysicalStart >= Ceiling) {
        EfiMemoryDescriptor[Index].Type = EfiReservedMemoryType;
      }
    }
  }
}
