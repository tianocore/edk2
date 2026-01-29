/** @file

Copyright (c) 2023, Google LLC. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_MEMORY_ATTRIBUTE_PPI_H_
#define EDKII_MEMORY_ATTRIBUTE_PPI_H_

#include <Uefi/UefiSpec.h>

///
/// Global ID for the EDKII_MEMORY_ATTRIBUTE_PPI.
///
#define EDKII_MEMORY_ATTRIBUTE_PPI_GUID \
  { \
    0x1be840de, 0x2d92, 0x41ec, { 0xb6, 0xd3, 0x19, 0x64, 0x13, 0x50, 0x51, 0xfb } \
  }

///
/// Forward declaration for the EDKII_MEMORY_ATTRIBUTE_PPI.
///
typedef struct _EDKII_MEMORY_ATTRIBUTE_PPI EDKII_MEMORY_ATTRIBUTE_PPI;

/**
  Set the requested memory permission attributes on a region of memory.

  BaseAddress and Length must be aligned to EFI_PAGE_SIZE.

  Attributes must contain a combination of EFI_MEMORY_RP, EFI_MEMORY_RO and
  EFI_MEMORY_XP, and specifies the attributes that must be set for the
  region in question. Attributes that are omitted will be cleared from the
  region only if they are set in AttributeMask.

  AttributeMask must contain a combination of EFI_MEMORY_RP, EFI_MEMORY_RO and
  EFI_MEMORY_XP, and specifies the attributes that the call will operate on.
  AttributeMask must not be 0x0, and must contain at least the bits set in
  Attributes.

  @param[in]  This              The protocol instance pointer.
  @param[in]  BaseAddress       The physical address that is the start address
                                of a memory region.
  @param[in]  Length            The size in bytes of the memory region.
  @param[in]  Attributes        Memory attributes to set or clear.
  @param[in]  AttributeMask     Mask of memory attributes to operate on.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                AttributeMask is zero.
                                AttributeMask lacks bits set in Attributes.
                                BaseAddress or Length is not suitably aligned.
  @retval EFI_UNSUPPORTED       The processor does not support one or more
                                bytes of the memory resource range specified
                                by BaseAddress and Length.
                                The bit mask of attributes is not supported for
                                the memory resource range specified by
                                BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES  Requested attributes cannot be applied due to
                                lack of system resources.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_MEMORY_ATTRIBUTE_SET_PERMISSIONS)(
  IN  EDKII_MEMORY_ATTRIBUTE_PPI  *This,
  IN  EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN  UINT64                      Length,
  IN  UINT64                      Attributes,
  IN  UINT64                      AttributeMask
  );

///
/// This PPI contains a set of services to manage memory permission attributes.
///
struct _EDKII_MEMORY_ATTRIBUTE_PPI {
  EDKII_MEMORY_ATTRIBUTE_SET_PERMISSIONS    SetPermissions;
};

extern EFI_GUID  gEdkiiMemoryAttributePpiGuid;

#endif
