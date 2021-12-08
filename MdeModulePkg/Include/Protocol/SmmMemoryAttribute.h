/** @file
  SMM Memory Attribute Protocol provides retrieval and update service
  for memory attributes in EFI SMM environment.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_MEMORYATTRIBUTE_H__
#define __SMM_MEMORYATTRIBUTE_H__

// {69B792EA-39CE-402D-A2A6-F721DE351DFE}
#define EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL_GUID \
  { \
    0x69b792ea, 0x39ce, 0x402d, { 0xa2, 0xa6, 0xf7, 0x21, 0xde, 0x35, 0x1d, 0xfe } \
  }

typedef struct _EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL;

/**
  This function set given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
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

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_SMM_SET_MEMORY_ATTRIBUTES)(
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL *This,
  IN  EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN  UINT64                              Length,
  IN  UINT64                              Attributes
  );

/**
  This function clears given attributes of the memory region specified by
  BaseAddress and Length.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
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

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_SMM_CLEAR_MEMORY_ATTRIBUTES)(
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL *This,
  IN  EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN  UINT64                              Length,
  IN  UINT64                              Attributes
  );

/**
  This function retrieves the attributes of the memory region specified by
  BaseAddress and Length. If different attributes are got from different part
  of the memory region, EFI_NO_MAPPING will be returned.

  @param  This              The EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL instance.
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
typedef
EFI_STATUS
(EFIAPI *EDKII_SMM_GET_MEMORY_ATTRIBUTES)(
  IN  EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL *This,
  IN  EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN  UINT64                              Length,
  OUT UINT64                              *Attributes
  );

///
/// SMM Memory Attribute Protocol provides services to retrieve or update
/// attribute of memory in the EFI SMM environment.
///
struct _EDKII_SMM_MEMORY_ATTRIBUTE_PROTOCOL {
  EDKII_SMM_GET_MEMORY_ATTRIBUTES      GetMemoryAttributes;
  EDKII_SMM_SET_MEMORY_ATTRIBUTES      SetMemoryAttributes;
  EDKII_SMM_CLEAR_MEMORY_ATTRIBUTES    ClearMemoryAttributes;
};

extern EFI_GUID  gEdkiiSmmMemoryAttributeProtocolGuid;

#endif
