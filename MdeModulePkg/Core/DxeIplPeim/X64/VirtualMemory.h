/** @file
  x64 Long Mode Virtual Memory Management Definitions

  References:
    1) IA-32 Intel(R) Architecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Architecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Architecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel
    4) AMD64 Architecture Programmer's Manual Volume 2: System Programming

Copyright (c) 2006 - 2023, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTUAL_MEMORY_H_
#define _VIRTUAL_MEMORY_H_

#define SYS_CODE64_SEL  0x38

#pragma pack(1)

typedef union {
  struct {
    UINT32    LimitLow    : 16;
    UINT32    BaseLow     : 16;
    UINT32    BaseMid     : 8;
    UINT32    Type        : 4;
    UINT32    System      : 1;
    UINT32    Dpl         : 2;
    UINT32    Present     : 1;
    UINT32    LimitHigh   : 4;
    UINT32    Software    : 1;
    UINT32    Reserved    : 1;
    UINT32    DefaultSize : 1;
    UINT32    Granularity : 1;
    UINT32    BaseHigh    : 8;
  } Bits;
  UINT64    Uint64;
} IA32_GDT;

typedef struct {
  IA32_IDT_GATE_DESCRIPTOR    Ia32IdtEntry;
  UINT32                      Offset32To63;
  UINT32                      Reserved;
} X64_IDT_GATE_DESCRIPTOR;

#pragma pack()

#define CR0_WP  BIT16

#define PAGING_1G_ADDRESS_MASK_64  0x000FFFFFC0000000ull

#define PAGE_TABLE_POOL_ALIGNMENT   BASE_2MB
#define PAGE_TABLE_POOL_UNIT_SIZE   SIZE_2MB
#define PAGE_TABLE_POOL_UNIT_PAGES  EFI_SIZE_TO_PAGES (PAGE_TABLE_POOL_UNIT_SIZE)
#define PAGE_TABLE_POOL_ALIGN_MASK  \
  (~(EFI_PHYSICAL_ADDRESS)(PAGE_TABLE_POOL_ALIGNMENT - 1))

typedef struct {
  VOID     *NextPool;
  UINTN    Offset;
  UINTN    FreePages;
} PAGE_TABLE_POOL;

/**
  Check if Execute Disable Bit (IA32_EFER.NXE) should be enabled or not.

  @retval TRUE    IA32_EFER.NXE should be enabled.
  @retval FALSE   IA32_EFER.NXE should not be enabled.

**/
BOOLEAN
IsEnableNonExecNeeded (
  VOID
  );

/**
  Enable Execute Disable Bit.

**/
VOID
EnableExecuteDisableBit (
  VOID
  );

/**
  Create IA32 PAE paging or 4-level/5-level paging for long mode to
  establish a 1:1 Virtual to Physical mapping.

  @param[in] Is64BitPageTable   Whether to create 64-bit page table.
  @param[in] StackBase          Stack base address.
  @param[in] StackSize          Stack size.
  @param[in] GhcbBase           GHCB page area base address.
  @param[in] GhcbSize           GHCB page area size.

  @return The address of page table.

**/
UINTN
CreateIdentityMappingPageTables (
  IN BOOLEAN               Is64BitPageTable,
  IN EFI_PHYSICAL_ADDRESS  StackBase,
  IN UINTN                 StackSize,
  IN EFI_PHYSICAL_ADDRESS  GhcbBase,
  IN UINTN                 GhcbSize
  );

/**

  Fix up the vector number in the vector code.

  @param VectorBase   Base address of the vector handler.
  @param VectorNum    Index of vector.

**/
VOID
EFIAPI
AsmVectorFixup (
  VOID   *VectorBase,
  UINT8  VectorNum
  );

/**

  Get the information of vector template.

  @param TemplateBase   Base address of the template code.

  @return               Size of the Template code.

**/
UINTN
EFIAPI
AsmGetVectorTemplatInfo (
  OUT   VOID  **TemplateBase
  );

/**
  Clear legacy memory located at the first 4K-page.

  This function traverses the whole HOB list to check if memory from 0 to 4095
  exists and has not been allocated, and then clear it if so.

  @param HobStart         The start of HobList passed to DxeCore.

**/
VOID
ClearFirst4KPage (
  IN  VOID  *HobStart
  );

/**
  Return configure status of NULL pointer detection feature.

  @return TRUE   NULL pointer detection feature is enabled
  @return FALSE  NULL pointer detection feature is disabled
**/
BOOLEAN
IsNullDetectionEnabled (
  VOID
  );

#endif
