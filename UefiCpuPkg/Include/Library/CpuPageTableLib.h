/** @file
  Public include file for PageTableLib library.

  Copyright (c) 2022 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PAGE_TABLE_LIB_H_
#define PAGE_TABLE_LIB_H_

typedef union {
  struct {
    UINT32    Present                  : 1;  // 0 = Not present in memory, 1 = Present in memory
    UINT32    ReadWrite                : 1;  // 0 = Read-Only, 1= Read/Write
    UINT32    UserSupervisor           : 1;  // 0 = Supervisor, 1=User
    UINT32    WriteThrough             : 1;  // 0 = Write-Back caching, 1=Write-Through caching
    UINT32    CacheDisabled            : 1;  // 0 = Cached, 1=Non-Cached
    UINT32    Accessed                 : 1;  // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT32    Dirty                    : 1;  // 0 = Not dirty, 1 = Dirty (set by CPU)
    UINT32    Pat                      : 1;  // PAT
    UINT32    Global                   : 1;  // 0 = Not global, 1 = Global (if CR4.PGE = 1)
    UINT32    Reserved1                : 3;  // Ignored
    UINT32    PageTableBaseAddressLow  : 20; // Page Table Base Address Low

    UINT32    PageTableBaseAddressHigh : 20; // Page Table Base Address High
    UINT32    Reserved2                : 7;  // Ignored
    UINT32    ProtectionKey            : 4;  // Protection key
    UINT32    Nx                       : 1;  // No Execute bit
  } Bits;
  UINT64    Uint64;
} IA32_MAP_ATTRIBUTE;

#define IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK  0xFFFFFFFFFF000ull
#define IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS(pa)  ((pa)->Uint64 & IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK)
#define IA32_MAP_ATTRIBUTE_ATTRIBUTES(pa)               ((pa)->Uint64 & ~IA32_MAP_ATTRIBUTE_PAGE_TABLE_BASE_ADDRESS_MASK)

//
// Below enum follows "4.1.1 Four Paging Modes" in Chapter 4 Paging of SDM Volume 3.
// Page1GB is only supported in 4-level and 5-level.
//
typedef enum {
  Paging32bit,

  //
  // High byte in paging mode indicates the max levels of the page table.
  // Low byte in paging mode indicates the max level that can be a leaf entry.
  //
  PagingPae4KB = 0x0301,
  PagingPae2MB = 0x0302,
  PagingPae    = 0x0302,

  Paging4Level4KB = 0x0401,
  Paging4Level2MB = 0x0402,
  Paging4Level    = 0x0402,
  Paging4Level1GB = 0x0403,

  Paging5Level4KB = 0x0501,
  Paging5Level2MB = 0x0502,
  Paging5Level    = 0x0502,
  Paging5Level1GB = 0x0503,

  PagingModeMax
} PAGING_MODE;

/**
  Create or update page table to map [LinearAddress, LinearAddress + Length) with specified attribute.

  @param[in, out] PageTable      The pointer to the page table to update, or pointer to NULL if a new page table is to be created.
                                 If not pointer to NULL, the value it points to won't be changed in this function.
  @param[in]      PagingMode     The paging mode.
  @param[in]      Buffer         The free buffer to be used for page table creation/updating.
  @param[in, out] BufferSize     The buffer size.
                                 On return, the remaining buffer size.
                                 The free buffer is used from the end so caller can supply the same Buffer pointer with an updated
                                 BufferSize in the second call to this API.
  @param[in]      LinearAddress  The start of the linear address range.
  @param[in]      Length         The length of the linear address range.
  @param[in]      Attribute      The attribute of the linear address range.
                                 All non-reserved fields in IA32_MAP_ATTRIBUTE are supported to set in the page table.
                                 Page table entries that map the linear address range are reset to 0 before set to the new attribute
                                 when a new physical base address is set.
  @param[in]      Mask           The mask used for attribute. The corresponding field in Attribute is ignored if that in Mask is 0.
  @param[out]     IsModified     TRUE means page table is modified by software or hardware. FALSE means page table is not modified by software.
                                 If the output IsModified is FALSE, there is possibility that the page table is changed by hardware. It is ok
                                 because page table can be changed by hardware anytime, and caller don't need to Flush TLB.

  @retval RETURN_UNSUPPORTED        PagingMode is not supported.
  @retval RETURN_INVALID_PARAMETER  PageTable, BufferSize, Attribute or Mask is NULL.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 1 but some other attributes are not provided.
  @retval RETURN_INVALID_PARAMETER  For non-present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  For present range, Mask->Bits.Present is 1, Attribute->Bits.Present is 0 but some other attributes are provided.
  @retval RETURN_INVALID_PARAMETER  *BufferSize is not multiple of 4KB.
  @retval RETURN_BUFFER_TOO_SMALL   The buffer is too small for page table creation/updating.
                                    BufferSize is updated to indicate the expected buffer size.
                                    Caller may still get RETURN_BUFFER_TOO_SMALL with the new BufferSize.
  @retval RETURN_SUCCESS            PageTable is created/updated successfully or the input Length is 0.
**/
RETURN_STATUS
EFIAPI
PageTableMap (
  IN OUT UINTN               *PageTable  OPTIONAL,
  IN     PAGING_MODE         PagingMode,
  IN     VOID                *Buffer,
  IN OUT UINTN               *BufferSize,
  IN     UINT64              LinearAddress,
  IN     UINT64              Length,
  IN     IA32_MAP_ATTRIBUTE  *Attribute,
  IN     IA32_MAP_ATTRIBUTE  *Mask,
  OUT    BOOLEAN             *IsModified   OPTIONAL
  );

typedef struct {
  UINT64                LinearAddress;
  UINT64                Length;
  IA32_MAP_ATTRIBUTE    Attribute;
} IA32_MAP_ENTRY;

/**
  Parse page table.

  @param[in]      PageTable  Pointer to the page table.
  @param[in]      PagingMode The paging mode.
  @param[out]     Map        Return an array that describes multiple linear address ranges.
  @param[in, out] MapCount   On input, the maximum number of entries that Map can hold.
                             On output, the number of entries in Map.

  @retval RETURN_UNSUPPORTED       PageLevel is not 5 or 4.
  @retval RETURN_INVALID_PARAMETER MapCount is NULL.
  @retval RETURN_INVALID_PARAMETER *MapCount is not 0 but Map is NULL.
  @retval RETURN_BUFFER_TOO_SMALL  *MapCount is too small.
  @retval RETURN_SUCCESS           Page table is parsed successfully.
**/
RETURN_STATUS
EFIAPI
PageTableParse (
  IN     UINTN           PageTable,
  IN     PAGING_MODE     PagingMode,
  IN     IA32_MAP_ENTRY  *Map,
  IN OUT UINTN           *MapCount
  );

/**
  Retrieve page table entry.

  @param[in]      PageTable      The pointer to the page table to use.
  @param[in]      PagingMode     The paging mode.
  @param[in]      LinearAddress  The linear address to use to walk the page table.
  @param[in, out] Entry          The page table map attribute for the linear address.
  @param[in, out] Level          The page table entry level for the linear address.

  @retval RETURN_SUCCESS            Page table entry and level returned.
  @retval RETURN_UNSUPPORTED        PagingMode is not supported.
  @retval RETURN_INVALID_PARAMETER  PageTable is 0.
  @retval RETURN_INVALID_PARAMETER  Entry or Level is NULL.
  @retval RETURN_INVALID_PARAMETER  LinearAddress exceeds the maximum linear address
                                    for the PagingMode.
**/
RETURN_STATUS
EFIAPI
PageTableGetMapAttribute (
  IN     UINTN               PageTable,
  IN     PAGING_MODE         PagingMode,
  IN     UINT64              LinearAddress,
  IN OUT IA32_MAP_ATTRIBUTE  *Entry,
  IN OUT UINTN               *Level
  );

#endif
