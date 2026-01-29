/** @file

  Copyright (c) 2023, Google LLC. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuDxe.h"

/**
  Check whether the provided memory range is covered by a single entry of type
  EfiGcdSystemMemory in the GCD memory map.

  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.

  @return Whether the region is system memory or not.
**/
STATIC
BOOLEAN
RegionIsSystemMemory (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  )
{
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  GcdDescriptor;
  EFI_PHYSICAL_ADDRESS             GcdEndAddress;
  EFI_STATUS                       Status;

  Status = gDS->GetMemorySpaceDescriptor (BaseAddress, &GcdDescriptor);
  if (EFI_ERROR (Status) ||
      (GcdDescriptor.GcdMemoryType != EfiGcdMemoryTypeSystemMemory))
  {
    return FALSE;
  }

  GcdEndAddress = GcdDescriptor.BaseAddress + GcdDescriptor.Length;

  //
  // Return TRUE if the GCD descriptor covers the range entirely
  //
  return GcdEndAddress >= (BaseAddress + Length);
}

/**
  This function retrieves the attributes of the memory region specified by
  BaseAddress and Length. If different attributes are obtained from different
  parts of the memory region, EFI_NO_MAPPING will be returned.

  @param  This              The EFI_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        Pointer to attributes returned.

  @retval EFI_SUCCESS           The attributes got for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes is NULL.
  @retval EFI_NO_MAPPING        Attributes are not consistent cross the memory
                                region.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.

**/
STATIC
EFI_STATUS
GetMemoryAttributes (
  IN  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS           BaseAddress,
  IN  UINT64                         Length,
  OUT UINT64                         *Attributes
  )
{
  UINTN       RegionAddress;
  UINTN       RegionLength;
  UINTN       RegionAttributes;
  UINTN       Union;
  UINTN       Intersection;
  EFI_STATUS  Status;

  if ((Length == 0) || (Attributes == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BaseAddress 0x%llx Length 0x%llx is zero or Attributes is NULL\n",
      __func__,
      BaseAddress,
      Length
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!RegionIsSystemMemory (BaseAddress, Length)) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: BaseAddress == 0x%lx, Length == 0x%lx\n",
    __func__,
    BaseAddress,
    Length
    ));

  Union        = 0;
  Intersection = MAX_UINTN;

  for (RegionAddress = (UINTN)BaseAddress;
       RegionAddress < (UINTN)(BaseAddress + Length);
       RegionAddress += RegionLength)
  {
    Status = GetMemoryRegion (
               &RegionAddress,
               &RegionLength,
               &RegionAttributes
               );

    DEBUG ((
      DEBUG_VERBOSE,
      "%a: RegionAddress == 0x%lx, RegionLength == 0x%lx, RegionAttributes == 0x%lx\n",
      __func__,
      (UINT64)RegionAddress,
      (UINT64)RegionLength,
      (UINT64)RegionAttributes
      ));

    if (EFI_ERROR (Status)) {
      return EFI_NO_MAPPING;
    }

    Union        |= RegionAttributes;
    Intersection &= RegionAttributes;
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: Union == %lx, Intersection == %lx\n",
    __func__,
    (UINT64)Union,
    (UINT64)Intersection
    ));

  if (Union != Intersection) {
    return EFI_NO_MAPPING;
  }

  *Attributes  = RegionAttributeToGcdAttribute (Union);
  *Attributes &= EFI_MEMORY_RP | EFI_MEMORY_RO | EFI_MEMORY_XP;
  return EFI_SUCCESS;
}

/**
  This function set given attributes of the memory region specified by
  BaseAddress and Length.

  The valid Attributes is EFI_MEMORY_RP, EFI_MEMORY_XP, and EFI_MEMORY_RO.

  @param  This              The EFI_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to set for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be set together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.
  @retval EFI_ACCESS_DENIED     Attributes for the requested memory region are
                                controlled by system firmware and cannot be
                                updated via the protocol.

**/
STATIC
EFI_STATUS
SetMemoryAttributes (
  IN  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS           BaseAddress,
  IN  UINT64                         Length,
  IN  UINT64                         Attributes
  )
{
  DEBUG ((
    DEBUG_INFO,
    "%a: BaseAddress == 0x%lx, Length == 0x%lx, Attributes == 0x%lx\n",
    __func__,
    (UINTN)BaseAddress,
    (UINTN)Length,
    (UINTN)Attributes
    ));

  if ((Length == 0) ||
      ((Attributes & ~(EFI_MEMORY_RO | EFI_MEMORY_RP | EFI_MEMORY_XP)) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BaseAddress 0x%llx Length is zero or Attributes (0x%llx) is invalid\n",
      __func__,
      BaseAddress,
      Attributes
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!RegionIsSystemMemory (BaseAddress, Length)) {
    return EFI_UNSUPPORTED;
  }

  return ArmSetMemoryAttributes (BaseAddress, Length, Attributes, Attributes);
}

/**
  This function clears given attributes of the memory region specified by
  BaseAddress and Length.

  The valid Attributes is EFI_MEMORY_RP, EFI_MEMORY_XP, and EFI_MEMORY_RO.

  @param  This              The EFI_MEMORY_ATTRIBUTE_PROTOCOL instance.
  @param  BaseAddress       The physical address that is the start address of
                            a memory region.
  @param  Length            The size in bytes of the memory region.
  @param  Attributes        The bit mask of attributes to clear for the memory
                            region.

  @retval EFI_SUCCESS           The attributes were cleared for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of
                                attributes that cannot be cleared together.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.
  @retval EFI_ACCESS_DENIED     Attributes for the requested memory region are
                                controlled by system firmware and cannot be
                                updated via the protocol.

**/
STATIC
EFI_STATUS
ClearMemoryAttributes (
  IN  EFI_MEMORY_ATTRIBUTE_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS           BaseAddress,
  IN  UINT64                         Length,
  IN  UINT64                         Attributes
  )
{
  DEBUG ((
    DEBUG_INFO,
    "%a: BaseAddress == 0x%lx, Length == 0x%lx, Attributes == 0x%lx\n",
    __func__,
    (UINTN)BaseAddress,
    (UINTN)Length,
    (UINTN)Attributes
    ));

  if ((Length == 0) ||
      ((Attributes & ~(EFI_MEMORY_RO | EFI_MEMORY_RP | EFI_MEMORY_XP)) != 0))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a: BaseAddress 0x%llx Length is zero or Attributes (0x%llx) is invalid\n",
      __func__,
      BaseAddress,
      Attributes
      ));
    return EFI_INVALID_PARAMETER;
  }

  if (!RegionIsSystemMemory (BaseAddress, Length)) {
    return EFI_UNSUPPORTED;
  }

  return ArmSetMemoryAttributes (BaseAddress, Length, 0, Attributes);
}

EFI_MEMORY_ATTRIBUTE_PROTOCOL  mMemoryAttribute = {
  GetMemoryAttributes,
  SetMemoryAttributes,
  ClearMemoryAttributes
};
