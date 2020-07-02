/** @file
  Page table management support.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Uefi.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/SerialPortLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SmmBase2.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

#include "CpuDxe.h"
#include "CpuPageTable.h"

///
/// Page Table Entry
///
#define IA32_PG_P                   BIT0
#define IA32_PG_RW                  BIT1
#define IA32_PG_U                   BIT2
#define IA32_PG_WT                  BIT3
#define IA32_PG_CD                  BIT4
#define IA32_PG_A                   BIT5
#define IA32_PG_D                   BIT6
#define IA32_PG_PS                  BIT7
#define IA32_PG_PAT_2M              BIT12
#define IA32_PG_PAT_4K              IA32_PG_PS
#define IA32_PG_PMNT                BIT62
#define IA32_PG_NX                  BIT63

#define PAGE_ATTRIBUTE_BITS         (IA32_PG_D | IA32_PG_A | IA32_PG_U | IA32_PG_RW | IA32_PG_P)
//
// Bits 1, 2, 5, 6 are reserved in the IA32 PAE PDPTE
// X64 PAE PDPTE does not have such restriction
//
#define IA32_PAE_PDPTE_ATTRIBUTE_BITS    (IA32_PG_P)

#define PAGE_PROGATE_BITS           (IA32_PG_NX | PAGE_ATTRIBUTE_BITS)

#define PAGING_4K_MASK  0xFFF
#define PAGING_2M_MASK  0x1FFFFF
#define PAGING_1G_MASK  0x3FFFFFFF

#define PAGING_PAE_INDEX_MASK  0x1FF

#define PAGING_4K_ADDRESS_MASK_64 0x000FFFFFFFFFF000ull
#define PAGING_2M_ADDRESS_MASK_64 0x000FFFFFFFE00000ull
#define PAGING_1G_ADDRESS_MASK_64 0x000FFFFFC0000000ull

#define MAX_PF_ENTRY_COUNT        10
#define MAX_DEBUG_MESSAGE_LENGTH  0x100
#define IA32_PF_EC_ID             BIT4

typedef enum {
  PageNone,
  Page4K,
  Page2M,
  Page1G,
} PAGE_ATTRIBUTE;

typedef struct {
  PAGE_ATTRIBUTE   Attribute;
  UINT64           Length;
  UINT64           AddressMask;
} PAGE_ATTRIBUTE_TABLE;

typedef enum {
  PageActionAssign,
  PageActionSet,
  PageActionClear,
} PAGE_ACTION;

PAGE_ATTRIBUTE_TABLE mPageAttributeTable[] = {
  {Page4K,  SIZE_4KB, PAGING_4K_ADDRESS_MASK_64},
  {Page2M,  SIZE_2MB, PAGING_2M_ADDRESS_MASK_64},
  {Page1G,  SIZE_1GB, PAGING_1G_ADDRESS_MASK_64},
};

PAGE_TABLE_POOL                   *mPageTablePool = NULL;
BOOLEAN                           mPageTablePoolLock = FALSE;
PAGE_TABLE_LIB_PAGING_CONTEXT     mPagingContext;
EFI_SMM_BASE2_PROTOCOL            *mSmmBase2 = NULL;

//
// Record the page fault exception count for one instruction execution.
//
UINTN                     *mPFEntryCount;
UINT64                    *(*mLastPFEntryPointer)[MAX_PF_ENTRY_COUNT];

/**
 Check if current execution environment is in SMM mode or not, via
 EFI_SMM_BASE2_PROTOCOL.

 This is necessary because of the fact that MdePkg\Library\SmmMemoryAllocationLib
 supports to free memory outside SMRAM. The library will call gBS->FreePool() or
 gBS->FreePages() and then SetMemorySpaceAttributes interface in turn to change
 memory paging attributes during free operation, if some memory related features
 are enabled (like Heap Guard).

 This means that SetMemorySpaceAttributes() has chance to run in SMM mode. This
 will cause incorrect result because SMM mode always loads its own page tables,
 which are usually different from DXE. This function can be used to detect such
 situation and help to avoid further misoperations.

  @retval TRUE    In SMM mode.
  @retval FALSE   Not in SMM mode.
**/
BOOLEAN
IsInSmm (
  VOID
  )
{
  BOOLEAN                 InSmm;

  InSmm = FALSE;
  if (mSmmBase2 == NULL) {
    gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, (VOID **)&mSmmBase2);
  }

  if (mSmmBase2 != NULL) {
    mSmmBase2->InSmm (mSmmBase2, &InSmm);
  }

  //
  // mSmmBase2->InSmm() can only detect if the caller is running in SMRAM
  // or from SMM driver. It cannot tell if the caller is running in SMM mode.
  // Check page table base address to guarantee that because SMM mode willl
  // load its own page table.
  //
  return (InSmm &&
          mPagingContext.ContextData.X64.PageTableBase != (UINT64)AsmReadCr3());
}

/**
  Return current paging context.

  @param[in,out]  PagingContext     The paging context.
**/
VOID
GetCurrentPagingContext (
  IN OUT PAGE_TABLE_LIB_PAGING_CONTEXT     *PagingContext
  )
{
  UINT32                          RegEax;
  CPUID_EXTENDED_CPU_SIG_EDX      RegEdx;
  MSR_IA32_EFER_REGISTER          MsrEfer;
  IA32_CR4                        Cr4;
  IA32_CR0                        Cr0;
  UINT32                          *Attributes;
  UINTN                           *PageTableBase;

  //
  // Don't retrieve current paging context from processor if in SMM mode.
  //
  if (!IsInSmm ()) {
    ZeroMem (&mPagingContext, sizeof(mPagingContext));
    if (sizeof(UINTN) == sizeof(UINT64)) {
      mPagingContext.MachineType = IMAGE_FILE_MACHINE_X64;
    } else {
      mPagingContext.MachineType = IMAGE_FILE_MACHINE_I386;
    }

    GetPagingDetails (&mPagingContext.ContextData, &PageTableBase, &Attributes);

    Cr0.UintN = AsmReadCr0 ();
    Cr4.UintN = AsmReadCr4 ();

    if (Cr0.Bits.PG != 0) {
      *PageTableBase = (AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64);
    } else {
      *PageTableBase = 0;
    }
    if (Cr0.Bits.WP  != 0) {
      *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_WP_ENABLE;
    }
    if (Cr4.Bits.PSE != 0) {
      *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PSE;
    }
    if (Cr4.Bits.PAE != 0) {
      *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PAE;
    }
    if (Cr4.Bits.LA57 != 0) {
      *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_5_LEVEL;
    }

    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
      AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx.Uint32);

      if (RegEdx.Bits.NX != 0) {
        // XD supported
        MsrEfer.Uint64 = AsmReadMsr64(MSR_CORE_IA32_EFER);
        if (MsrEfer.Bits.NXE != 0) {
          // XD activated
          *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_XD_ACTIVATED;
        }
      }

      if (RegEdx.Bits.Page1GB != 0) {
        *Attributes |= PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PAGE_1G_SUPPORT;
      }
    }
  }

  //
  // This can avoid getting SMM paging context if in SMM mode. We cannot assume
  // SMM mode shares the same paging context as DXE.
  //
  CopyMem (PagingContext, &mPagingContext, sizeof (mPagingContext));
}

/**
  Return length according to page attributes.

  @param[in]  PageAttributes   The page attribute of the page entry.

  @return The length of page entry.
**/
UINTN
PageAttributeToLength (
  IN PAGE_ATTRIBUTE  PageAttribute
  )
{
  UINTN  Index;
  for (Index = 0; Index < sizeof(mPageAttributeTable)/sizeof(mPageAttributeTable[0]); Index++) {
    if (PageAttribute == mPageAttributeTable[Index].Attribute) {
      return (UINTN)mPageAttributeTable[Index].Length;
    }
  }
  return 0;
}

/**
  Return address mask according to page attributes.

  @param[in]  PageAttributes   The page attribute of the page entry.

  @return The address mask of page entry.
**/
UINTN
PageAttributeToMask (
  IN PAGE_ATTRIBUTE  PageAttribute
  )
{
  UINTN  Index;
  for (Index = 0; Index < sizeof(mPageAttributeTable)/sizeof(mPageAttributeTable[0]); Index++) {
    if (PageAttribute == mPageAttributeTable[Index].Attribute) {
      return (UINTN)mPageAttributeTable[Index].AddressMask;
    }
  }
  return 0;
}

/**
  Return page table entry to match the address.

  @param[in]  PagingContext     The paging context.
  @param[in]  Address           The address to be checked.
  @param[out] PageAttributes    The page attribute of the page entry.

  @return The page entry.
**/
VOID *
GetPageTableEntry (
  IN  PAGE_TABLE_LIB_PAGING_CONTEXT     *PagingContext,
  IN  PHYSICAL_ADDRESS                  Address,
  OUT PAGE_ATTRIBUTE                    *PageAttribute
  )
{
  UINTN                 Index1;
  UINTN                 Index2;
  UINTN                 Index3;
  UINTN                 Index4;
  UINTN                 Index5;
  UINT64                *L1PageTable;
  UINT64                *L2PageTable;
  UINT64                *L3PageTable;
  UINT64                *L4PageTable;
  UINT64                *L5PageTable;
  UINT64                AddressEncMask;

  ASSERT (PagingContext != NULL);

  Index5 = ((UINTN)RShiftU64 (Address, 48)) & PAGING_PAE_INDEX_MASK;
  Index4 = ((UINTN)RShiftU64 (Address, 39)) & PAGING_PAE_INDEX_MASK;
  Index3 = ((UINTN)Address >> 30) & PAGING_PAE_INDEX_MASK;
  Index2 = ((UINTN)Address >> 21) & PAGING_PAE_INDEX_MASK;
  Index1 = ((UINTN)Address >> 12) & PAGING_PAE_INDEX_MASK;

  // Make sure AddressEncMask is contained to smallest supported address field.
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  if (PagingContext->MachineType == IMAGE_FILE_MACHINE_X64) {
    if ((PagingContext->ContextData.X64.Attributes & PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_5_LEVEL) != 0) {
      L5PageTable = (UINT64 *)(UINTN)PagingContext->ContextData.X64.PageTableBase;
      if (L5PageTable[Index5] == 0) {
        *PageAttribute = PageNone;
        return NULL;
      }

      L4PageTable = (UINT64 *)(UINTN)(L5PageTable[Index5] & ~AddressEncMask & PAGING_4K_ADDRESS_MASK_64);
    } else {
      L4PageTable = (UINT64 *)(UINTN)PagingContext->ContextData.X64.PageTableBase;
    }
    if (L4PageTable[Index4] == 0) {
      *PageAttribute = PageNone;
      return NULL;
    }

    L3PageTable = (UINT64 *)(UINTN)(L4PageTable[Index4] & ~AddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  } else {
    ASSERT((PagingContext->ContextData.Ia32.Attributes & PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PAE) != 0);
    L3PageTable = (UINT64 *)(UINTN)PagingContext->ContextData.Ia32.PageTableBase;
  }
  if (L3PageTable[Index3] == 0) {
    *PageAttribute = PageNone;
    return NULL;
  }
  if ((L3PageTable[Index3] & IA32_PG_PS) != 0) {
    // 1G
    *PageAttribute = Page1G;
    return &L3PageTable[Index3];
  }

  L2PageTable = (UINT64 *)(UINTN)(L3PageTable[Index3] & ~AddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  if (L2PageTable[Index2] == 0) {
    *PageAttribute = PageNone;
    return NULL;
  }
  if ((L2PageTable[Index2] & IA32_PG_PS) != 0) {
    // 2M
    *PageAttribute = Page2M;
    return &L2PageTable[Index2];
  }

  // 4k
  L1PageTable = (UINT64 *)(UINTN)(L2PageTable[Index2] & ~AddressEncMask & PAGING_4K_ADDRESS_MASK_64);
  if ((L1PageTable[Index1] == 0) && (Address != 0)) {
    *PageAttribute = PageNone;
    return NULL;
  }
  *PageAttribute = Page4K;
  return &L1PageTable[Index1];
}

/**
  Return memory attributes of page entry.

  @param[in]  PageEntry        The page entry.

  @return Memory attributes of page entry.
**/
UINT64
GetAttributesFromPageEntry (
  IN  UINT64                            *PageEntry
  )
{
  UINT64  Attributes;
  Attributes = 0;
  if ((*PageEntry & IA32_PG_P) == 0) {
    Attributes |= EFI_MEMORY_RP;
  }
  if ((*PageEntry & IA32_PG_RW) == 0) {
    Attributes |= EFI_MEMORY_RO;
  }
  if ((*PageEntry & IA32_PG_NX) != 0) {
    Attributes |= EFI_MEMORY_XP;
  }
  return Attributes;
}

/**
  Modify memory attributes of page entry.

  @param[in]  PagingContext    The paging context.
  @param[in]  PageEntry        The page entry.
  @param[in]  Attributes       The bit mask of attributes to modify for the memory region.
  @param[in]  PageAction       The page action.
  @param[out] IsModified       TRUE means page table modified. FALSE means page table not modified.
**/
VOID
ConvertPageEntryAttribute (
  IN  PAGE_TABLE_LIB_PAGING_CONTEXT     *PagingContext,
  IN  UINT64                            *PageEntry,
  IN  UINT64                            Attributes,
  IN  PAGE_ACTION                       PageAction,
  OUT BOOLEAN                           *IsModified
  )
{
  UINT64  CurrentPageEntry;
  UINT64  NewPageEntry;
  UINT32  *PageAttributes;

  CurrentPageEntry = *PageEntry;
  NewPageEntry = CurrentPageEntry;
  if ((Attributes & EFI_MEMORY_RP) != 0) {
    switch (PageAction) {
    case PageActionAssign:
    case PageActionSet:
      NewPageEntry &= ~(UINT64)IA32_PG_P;
      break;
    case PageActionClear:
      NewPageEntry |= IA32_PG_P;
      break;
    }
  } else {
    switch (PageAction) {
    case PageActionAssign:
      NewPageEntry |= IA32_PG_P;
      break;
    case PageActionSet:
    case PageActionClear:
      break;
    }
  }
  if ((Attributes & EFI_MEMORY_RO) != 0) {
    switch (PageAction) {
    case PageActionAssign:
    case PageActionSet:
      NewPageEntry &= ~(UINT64)IA32_PG_RW;
      break;
    case PageActionClear:
      NewPageEntry |= IA32_PG_RW;
      break;
    }
  } else {
    switch (PageAction) {
    case PageActionAssign:
      NewPageEntry |= IA32_PG_RW;
      break;
    case PageActionSet:
    case PageActionClear:
      break;
    }
  }

  GetPagingDetails (&PagingContext->ContextData, NULL, &PageAttributes);

  if ((*PageAttributes & PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_XD_ACTIVATED) != 0) {
    if ((Attributes & EFI_MEMORY_XP) != 0) {
      switch (PageAction) {
      case PageActionAssign:
      case PageActionSet:
        NewPageEntry |= IA32_PG_NX;
        break;
      case PageActionClear:
        NewPageEntry &= ~IA32_PG_NX;
        break;
      }
    } else {
      switch (PageAction) {
      case PageActionAssign:
        NewPageEntry &= ~IA32_PG_NX;
        break;
      case PageActionSet:
      case PageActionClear:
        break;
      }
    }
  }
  *PageEntry = NewPageEntry;
  if (CurrentPageEntry != NewPageEntry) {
    *IsModified = TRUE;
    DEBUG ((DEBUG_VERBOSE, "ConvertPageEntryAttribute 0x%lx", CurrentPageEntry));
    DEBUG ((DEBUG_VERBOSE, "->0x%lx\n", NewPageEntry));
  } else {
    *IsModified = FALSE;
  }
}

/**
  This function returns if there is need to split page entry.

  @param[in]  BaseAddress      The base address to be checked.
  @param[in]  Length           The length to be checked.
  @param[in]  PageEntry        The page entry to be checked.
  @param[in]  PageAttribute    The page attribute of the page entry.

  @retval SplitAttributes on if there is need to split page entry.
**/
PAGE_ATTRIBUTE
NeedSplitPage (
  IN  PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                            Length,
  IN  UINT64                            *PageEntry,
  IN  PAGE_ATTRIBUTE                    PageAttribute
  )
{
  UINT64                PageEntryLength;

  PageEntryLength = PageAttributeToLength (PageAttribute);

  if (((BaseAddress & (PageEntryLength - 1)) == 0) && (Length >= PageEntryLength)) {
    return PageNone;
  }

  if (((BaseAddress & PAGING_2M_MASK) != 0) || (Length < SIZE_2MB)) {
    return Page4K;
  }

  return Page2M;
}

/**
  This function splits one page entry to small page entries.

  @param[in]  PageEntry         The page entry to be splitted.
  @param[in]  PageAttribute     The page attribute of the page entry.
  @param[in]  SplitAttribute    How to split the page entry.
  @param[in]  AllocatePagesFunc If page split is needed, this function is used to allocate more pages.

  @retval RETURN_SUCCESS            The page entry is splitted.
  @retval RETURN_UNSUPPORTED        The page entry does not support to be splitted.
  @retval RETURN_OUT_OF_RESOURCES   No resource to split page entry.
**/
RETURN_STATUS
SplitPage (
  IN  UINT64                            *PageEntry,
  IN  PAGE_ATTRIBUTE                    PageAttribute,
  IN  PAGE_ATTRIBUTE                    SplitAttribute,
  IN  PAGE_TABLE_LIB_ALLOCATE_PAGES     AllocatePagesFunc
  )
{
  UINT64   BaseAddress;
  UINT64   *NewPageEntry;
  UINTN    Index;
  UINT64   AddressEncMask;

  ASSERT (PageAttribute == Page2M || PageAttribute == Page1G);

  ASSERT (AllocatePagesFunc != NULL);

  // Make sure AddressEncMask is contained to smallest supported address field.
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  if (PageAttribute == Page2M) {
    //
    // Split 2M to 4K
    //
    ASSERT (SplitAttribute == Page4K);
    if (SplitAttribute == Page4K) {
      NewPageEntry = AllocatePagesFunc (1);
      DEBUG ((DEBUG_VERBOSE, "Split - 0x%x\n", NewPageEntry));
      if (NewPageEntry == NULL) {
        return RETURN_OUT_OF_RESOURCES;
      }
      BaseAddress = *PageEntry & ~AddressEncMask & PAGING_2M_ADDRESS_MASK_64;
      for (Index = 0; Index < SIZE_4KB / sizeof(UINT64); Index++) {
        NewPageEntry[Index] = (BaseAddress + SIZE_4KB * Index) | AddressEncMask | ((*PageEntry) & PAGE_PROGATE_BITS);
      }
      (*PageEntry) = (UINT64)(UINTN)NewPageEntry | AddressEncMask | ((*PageEntry) & PAGE_ATTRIBUTE_BITS);
      return RETURN_SUCCESS;
    } else {
      return RETURN_UNSUPPORTED;
    }
  } else if (PageAttribute == Page1G) {
    //
    // Split 1G to 2M
    // No need support 1G->4K directly, we should use 1G->2M, then 2M->4K to get more compact page table.
    //
    ASSERT (SplitAttribute == Page2M || SplitAttribute == Page4K);
    if ((SplitAttribute == Page2M || SplitAttribute == Page4K)) {
      NewPageEntry = AllocatePagesFunc (1);
      DEBUG ((DEBUG_VERBOSE, "Split - 0x%x\n", NewPageEntry));
      if (NewPageEntry == NULL) {
        return RETURN_OUT_OF_RESOURCES;
      }
      BaseAddress = *PageEntry & ~AddressEncMask  & PAGING_1G_ADDRESS_MASK_64;
      for (Index = 0; Index < SIZE_4KB / sizeof(UINT64); Index++) {
        NewPageEntry[Index] = (BaseAddress + SIZE_2MB * Index) | AddressEncMask | IA32_PG_PS | ((*PageEntry) & PAGE_PROGATE_BITS);
      }
      (*PageEntry) = (UINT64)(UINTN)NewPageEntry | AddressEncMask | ((*PageEntry) & PAGE_ATTRIBUTE_BITS);
      return RETURN_SUCCESS;
    } else {
      return RETURN_UNSUPPORTED;
    }
  } else {
    return RETURN_UNSUPPORTED;
  }
}

/**
 Check the WP status in CR0 register. This bit is used to lock or unlock write
 access to pages marked as read-only.

  @retval TRUE    Write protection is enabled.
  @retval FALSE   Write protection is disabled.
**/
BOOLEAN
IsReadOnlyPageWriteProtected (
  VOID
  )
{
  IA32_CR0  Cr0;
  //
  // To avoid unforseen consequences, don't touch paging settings in SMM mode
  // in this driver.
  //
  if (!IsInSmm ()) {
    Cr0.UintN = AsmReadCr0 ();
    return (BOOLEAN) (Cr0.Bits.WP != 0);
  }
  return FALSE;
}

/**
 Disable Write Protect on pages marked as read-only.
**/
VOID
DisableReadOnlyPageWriteProtect (
  VOID
  )
{
  IA32_CR0  Cr0;
  //
  // To avoid unforseen consequences, don't touch paging settings in SMM mode
  // in this driver.
  //
  if (!IsInSmm ()) {
    Cr0.UintN = AsmReadCr0 ();
    Cr0.Bits.WP = 0;
    AsmWriteCr0 (Cr0.UintN);
  }
}

/**
 Enable Write Protect on pages marked as read-only.
**/
VOID
EnableReadOnlyPageWriteProtect (
  VOID
  )
{
  IA32_CR0  Cr0;
  //
  // To avoid unforseen consequences, don't touch paging settings in SMM mode
  // in this driver.
  //
  if (!IsInSmm ()) {
    Cr0.UintN = AsmReadCr0 ();
    Cr0.Bits.WP = 1;
    AsmWriteCr0 (Cr0.UintN);
  }
}

/**
  This function modifies the page attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  Caller should make sure BaseAddress and Length is at page boundary.

  @param[in]  PagingContext     The paging context. NULL means get page table from current CPU context.
  @param[in]  BaseAddress       The physical address that is the start address of a memory region.
  @param[in]  Length            The size in bytes of the memory region.
  @param[in]  Attributes        The bit mask of attributes to modify for the memory region.
  @param[in]  PageAction        The page action.
  @param[in]  AllocatePagesFunc If page split is needed, this function is used to allocate more pages.
                                NULL mean page split is unsupported.
  @param[out] IsSplitted        TRUE means page table splitted. FALSE means page table not splitted.
  @param[out] IsModified        TRUE means page table modified. FALSE means page table not modified.

  @retval RETURN_SUCCESS           The attributes were modified for the memory region.
  @retval RETURN_ACCESS_DENIED     The attributes for the memory resource range specified by
                                   BaseAddress and Length cannot be modified.
  @retval RETURN_INVALID_PARAMETER Length is zero.
                                   Attributes specified an illegal combination of attributes that
                                   cannot be set together.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                   the memory resource range.
  @retval RETURN_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                   resource range specified by BaseAddress and Length.
                                   The bit mask of attributes is not support for the memory resource
                                   range specified by BaseAddress and Length.
**/
RETURN_STATUS
ConvertMemoryPageAttributes (
  IN  PAGE_TABLE_LIB_PAGING_CONTEXT     *PagingContext OPTIONAL,
  IN  PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                            Length,
  IN  UINT64                            Attributes,
  IN  PAGE_ACTION                       PageAction,
  IN  PAGE_TABLE_LIB_ALLOCATE_PAGES     AllocatePagesFunc OPTIONAL,
  OUT BOOLEAN                           *IsSplitted,  OPTIONAL
  OUT BOOLEAN                           *IsModified   OPTIONAL
  )
{
  PAGE_TABLE_LIB_PAGING_CONTEXT     CurrentPagingContext;
  UINT64                            *PageEntry;
  PAGE_ATTRIBUTE                    PageAttribute;
  UINTN                             PageEntryLength;
  PAGE_ATTRIBUTE                    SplitAttribute;
  RETURN_STATUS                     Status;
  BOOLEAN                           IsEntryModified;
  BOOLEAN                           IsWpEnabled;

  if ((BaseAddress & (SIZE_4KB - 1)) != 0) {
    DEBUG ((DEBUG_ERROR, "BaseAddress(0x%lx) is not aligned!\n", BaseAddress));
    return EFI_UNSUPPORTED;
  }
  if ((Length & (SIZE_4KB - 1)) != 0) {
    DEBUG ((DEBUG_ERROR, "Length(0x%lx) is not aligned!\n", Length));
    return EFI_UNSUPPORTED;
  }
  if (Length == 0) {
    DEBUG ((DEBUG_ERROR, "Length is 0!\n"));
    return RETURN_INVALID_PARAMETER;
  }

  if ((Attributes & ~EFI_MEMORY_ATTRIBUTE_MASK) != 0) {
    DEBUG ((DEBUG_ERROR, "Attributes(0x%lx) has unsupported bit\n", Attributes));
    return EFI_UNSUPPORTED;
  }

  if (PagingContext == NULL) {
    GetCurrentPagingContext (&CurrentPagingContext);
  } else {
    CopyMem (&CurrentPagingContext, PagingContext, sizeof(CurrentPagingContext));
  }
  switch(CurrentPagingContext.MachineType) {
  case IMAGE_FILE_MACHINE_I386:
    if (CurrentPagingContext.ContextData.Ia32.PageTableBase == 0) {
      if (Attributes == 0) {
        return EFI_SUCCESS;
      } else {
        DEBUG ((DEBUG_ERROR, "PageTable is 0!\n"));
        return EFI_UNSUPPORTED;
      }
    }
    if ((CurrentPagingContext.ContextData.Ia32.Attributes & PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PAE) == 0) {
      DEBUG ((DEBUG_ERROR, "Non-PAE Paging!\n"));
      return EFI_UNSUPPORTED;
    }
    if ((BaseAddress + Length) > BASE_4GB) {
      DEBUG ((DEBUG_ERROR, "Beyond 4GB memory in 32-bit mode!\n"));
      return EFI_UNSUPPORTED;
    }
    break;
  case IMAGE_FILE_MACHINE_X64:
    ASSERT (CurrentPagingContext.ContextData.X64.PageTableBase != 0);
    break;
  default:
    ASSERT(FALSE);
    return EFI_UNSUPPORTED;
    break;
  }

//  DEBUG ((DEBUG_ERROR, "ConvertMemoryPageAttributes(%x) - %016lx, %016lx, %02lx\n", IsSet, BaseAddress, Length, Attributes));

  if (IsSplitted != NULL) {
    *IsSplitted = FALSE;
  }
  if (IsModified != NULL) {
    *IsModified = FALSE;
  }
  if (AllocatePagesFunc == NULL) {
    AllocatePagesFunc = AllocatePageTableMemory;
  }

  //
  // Make sure that the page table is changeable.
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  //
  // Below logic is to check 2M/4K page to make sure we do not waste memory.
  //
  Status = EFI_SUCCESS;
  while (Length != 0) {
    PageEntry = GetPageTableEntry (&CurrentPagingContext, BaseAddress, &PageAttribute);
    if (PageEntry == NULL) {
      Status = RETURN_UNSUPPORTED;
      goto Done;
    }
    PageEntryLength = PageAttributeToLength (PageAttribute);
    SplitAttribute = NeedSplitPage (BaseAddress, Length, PageEntry, PageAttribute);
    if (SplitAttribute == PageNone) {
      ConvertPageEntryAttribute (&CurrentPagingContext, PageEntry, Attributes, PageAction, &IsEntryModified);
      if (IsEntryModified) {
        if (IsModified != NULL) {
          *IsModified = TRUE;
        }
      }
      //
      // Convert success, move to next
      //
      BaseAddress += PageEntryLength;
      Length -= PageEntryLength;
    } else {
      if (AllocatePagesFunc == NULL) {
        Status = RETURN_UNSUPPORTED;
        goto Done;
      }
      Status = SplitPage (PageEntry, PageAttribute, SplitAttribute, AllocatePagesFunc);
      if (RETURN_ERROR (Status)) {
        Status = RETURN_UNSUPPORTED;
        goto Done;
      }
      if (IsSplitted != NULL) {
        *IsSplitted = TRUE;
      }
      if (IsModified != NULL) {
        *IsModified = TRUE;
      }
      //
      // Just split current page
      // Convert success in next around
      //
    }
  }

Done:
  //
  // Restore page table write protection, if any.
  //
  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }
  return Status;
}

/**
  This function assigns the page attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  Caller should make sure BaseAddress and Length is at page boundary.

  Caller need guarantee the TPL <= TPL_NOTIFY, if there is split page request.

  @param[in]  PagingContext     The paging context. NULL means get page table from current CPU context.
  @param[in]  BaseAddress       The physical address that is the start address of a memory region.
  @param[in]  Length            The size in bytes of the memory region.
  @param[in]  Attributes        The bit mask of attributes to set for the memory region.
  @param[in]  AllocatePagesFunc If page split is needed, this function is used to allocate more pages.
                                NULL mean page split is unsupported.

  @retval RETURN_SUCCESS           The attributes were cleared for the memory region.
  @retval RETURN_ACCESS_DENIED     The attributes for the memory resource range specified by
                                   BaseAddress and Length cannot be modified.
  @retval RETURN_INVALID_PARAMETER Length is zero.
                                   Attributes specified an illegal combination of attributes that
                                   cannot be set together.
  @retval RETURN_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                   the memory resource range.
  @retval RETURN_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                   resource range specified by BaseAddress and Length.
                                   The bit mask of attributes is not support for the memory resource
                                   range specified by BaseAddress and Length.
**/
RETURN_STATUS
EFIAPI
AssignMemoryPageAttributes (
  IN  PAGE_TABLE_LIB_PAGING_CONTEXT     *PagingContext OPTIONAL,
  IN  PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                            Length,
  IN  UINT64                            Attributes,
  IN  PAGE_TABLE_LIB_ALLOCATE_PAGES     AllocatePagesFunc OPTIONAL
  )
{
  RETURN_STATUS  Status;
  BOOLEAN        IsModified;
  BOOLEAN        IsSplitted;

//  DEBUG((DEBUG_INFO, "AssignMemoryPageAttributes: 0x%lx - 0x%lx (0x%lx)\n", BaseAddress, Length, Attributes));
  Status = ConvertMemoryPageAttributes (PagingContext, BaseAddress, Length, Attributes, PageActionAssign, AllocatePagesFunc, &IsSplitted, &IsModified);
  if (!EFI_ERROR(Status)) {
    if ((PagingContext == NULL) && IsModified) {
      //
      // Flush TLB as last step.
      //
      // Note: Since APs will always init CR3 register in HLT loop mode or do
      // TLB flush in MWAIT loop mode, there's no need to flush TLB for them
      // here.
      //
      CpuFlushTlb();
    }
  }

  return Status;
}

/**
 Check if Execute Disable feature is enabled or not.
**/
BOOLEAN
IsExecuteDisableEnabled (
  VOID
  )
{
  MSR_CORE_IA32_EFER_REGISTER    MsrEfer;

  MsrEfer.Uint64 = AsmReadMsr64 (MSR_IA32_EFER);
  return (MsrEfer.Bits.NXE == 1);
}

/**
  Update GCD memory space attributes according to current page table setup.
**/
VOID
RefreshGcdMemoryAttributesFromPaging (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;
  PAGE_TABLE_LIB_PAGING_CONTEXT       PagingContext;
  PAGE_ATTRIBUTE                      PageAttribute;
  UINT64                              *PageEntry;
  UINT64                              PageLength;
  UINT64                              MemorySpaceLength;
  UINT64                              Length;
  UINT64                              BaseAddress;
  UINT64                              PageStartAddress;
  UINT64                              Attributes;
  UINT64                              Capabilities;
  UINT64                              NewAttributes;
  UINTN                               Index;

  //
  // Assuming that memory space map returned is sorted already; otherwise sort
  // them in the order of lowest address to highest address.
  //
  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  ASSERT_EFI_ERROR (Status);

  GetCurrentPagingContext (&PagingContext);

  Attributes      = 0;
  NewAttributes   = 0;
  BaseAddress     = 0;
  PageLength      = 0;

  if (IsExecuteDisableEnabled ()) {
    Capabilities = EFI_MEMORY_RO | EFI_MEMORY_RP | EFI_MEMORY_XP;
  } else {
    Capabilities = EFI_MEMORY_RO | EFI_MEMORY_RP;
  }

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }

    //
    // Sync the actual paging related capabilities back to GCD service first.
    // As a side effect (good one), this can also help to avoid unnecessary
    // memory map entries due to the different capabilities of the same type
    // memory, such as multiple RT_CODE and RT_DATA entries in memory map,
    // which could cause boot failure of some old Linux distro (before v4.3).
    //
    Status = gDS->SetMemorySpaceCapabilities (
                    MemorySpaceMap[Index].BaseAddress,
                    MemorySpaceMap[Index].Length,
                    MemorySpaceMap[Index].Capabilities | Capabilities
                    );
    if (EFI_ERROR (Status)) {
      //
      // If we cannot update the capabilities, we cannot update its
      // attributes either. So just simply skip current block of memory.
      //
      DEBUG ((
        DEBUG_WARN,
        "Failed to update capability: [%lu] %016lx - %016lx (%016lx -> %016lx)\r\n",
        (UINT64)Index, MemorySpaceMap[Index].BaseAddress,
        MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - 1,
        MemorySpaceMap[Index].Capabilities,
        MemorySpaceMap[Index].Capabilities | Capabilities
        ));
      continue;
    }

    if (MemorySpaceMap[Index].BaseAddress >= (BaseAddress + PageLength)) {
      //
      // Current memory space starts at a new page. Resetting PageLength will
      // trigger a retrieval of page attributes at new address.
      //
      PageLength = 0;
    } else {
      //
      // In case current memory space is not adjacent to last one
      //
      PageLength -= (MemorySpaceMap[Index].BaseAddress - BaseAddress);
    }

    //
    // Sync actual page attributes to GCD
    //
    BaseAddress       = MemorySpaceMap[Index].BaseAddress;
    MemorySpaceLength = MemorySpaceMap[Index].Length;
    while (MemorySpaceLength > 0) {
      if (PageLength == 0) {
        PageEntry = GetPageTableEntry (&PagingContext, BaseAddress, &PageAttribute);
        if (PageEntry == NULL) {
          break;
        }

        //
        // Note current memory space might start in the middle of a page
        //
        PageStartAddress  = (*PageEntry) & (UINT64)PageAttributeToMask(PageAttribute);
        PageLength        = PageAttributeToLength (PageAttribute) - (BaseAddress - PageStartAddress);
        Attributes        = GetAttributesFromPageEntry (PageEntry);
      }

      Length = MIN (PageLength, MemorySpaceLength);
      if (Attributes != (MemorySpaceMap[Index].Attributes &
                         EFI_MEMORY_ATTRIBUTE_MASK)) {
        NewAttributes = (MemorySpaceMap[Index].Attributes &
                         ~EFI_MEMORY_ATTRIBUTE_MASK) | Attributes;
        Status = gDS->SetMemorySpaceAttributes (
                        BaseAddress,
                        Length,
                        NewAttributes
                        );
        ASSERT_EFI_ERROR (Status);
        DEBUG ((
          DEBUG_VERBOSE,
          "Updated memory space attribute: [%lu] %016lx - %016lx (%016lx -> %016lx)\r\n",
          (UINT64)Index, BaseAddress, BaseAddress + Length - 1,
          MemorySpaceMap[Index].Attributes,
          NewAttributes
          ));
      }

      PageLength        -= Length;
      MemorySpaceLength -= Length;
      BaseAddress       += Length;
    }
  }

  FreePool (MemorySpaceMap);
}

/**
  Initialize a buffer pool for page table use only.

  To reduce the potential split operation on page table, the pages reserved for
  page table should be allocated in the times of PAGE_TABLE_POOL_UNIT_PAGES and
  at the boundary of PAGE_TABLE_POOL_ALIGNMENT. So the page pool is always
  initialized with number of pages greater than or equal to the given PoolPages.

  Once the pages in the pool are used up, this method should be called again to
  reserve at least another PAGE_TABLE_POOL_UNIT_PAGES. Usually this won't happen
  often in practice.

  @param[in] PoolPages      The least page number of the pool to be created.

  @retval TRUE    The pool is initialized successfully.
  @retval FALSE   The memory is out of resource.
**/
BOOLEAN
InitializePageTablePool (
  IN  UINTN                           PoolPages
  )
{
  VOID                      *Buffer;
  BOOLEAN                   IsModified;

  //
  // Do not allow re-entrance.
  //
  if (mPageTablePoolLock) {
    return FALSE;
  }

  mPageTablePoolLock = TRUE;
  IsModified = FALSE;

  //
  // Always reserve at least PAGE_TABLE_POOL_UNIT_PAGES, including one page for
  // header.
  //
  PoolPages += 1;   // Add one page for header.
  PoolPages = ((PoolPages - 1) / PAGE_TABLE_POOL_UNIT_PAGES + 1) *
              PAGE_TABLE_POOL_UNIT_PAGES;
  Buffer = AllocateAlignedPages (PoolPages, PAGE_TABLE_POOL_ALIGNMENT);
  if (Buffer == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Out of aligned pages\r\n"));
    goto Done;
  }

  DEBUG ((
    DEBUG_INFO,
    "Paging: added %lu pages to page table pool\r\n",
    (UINT64)PoolPages
    ));

  //
  // Link all pools into a list for easier track later.
  //
  if (mPageTablePool == NULL) {
    mPageTablePool = Buffer;
    mPageTablePool->NextPool = mPageTablePool;
  } else {
    ((PAGE_TABLE_POOL *)Buffer)->NextPool = mPageTablePool->NextPool;
    mPageTablePool->NextPool = Buffer;
    mPageTablePool = Buffer;
  }

  //
  // Reserve one page for pool header.
  //
  mPageTablePool->FreePages  = PoolPages - 1;
  mPageTablePool->Offset = EFI_PAGES_TO_SIZE (1);

  //
  // Mark the whole pool pages as read-only.
  //
  ConvertMemoryPageAttributes (
    NULL,
    (PHYSICAL_ADDRESS)(UINTN)Buffer,
    EFI_PAGES_TO_SIZE (PoolPages),
    EFI_MEMORY_RO,
    PageActionSet,
    AllocatePageTableMemory,
    NULL,
    &IsModified
    );
  ASSERT (IsModified == TRUE);

Done:
  mPageTablePoolLock = FALSE;
  return IsModified;
}

/**
  This API provides a way to allocate memory for page table.

  This API can be called more than once to allocate memory for page tables.

  Allocates the number of 4KB pages and returns a pointer to the allocated
  buffer. The buffer returned is aligned on a 4KB boundary.

  If Pages is 0, then NULL is returned.
  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePageTableMemory (
  IN UINTN           Pages
  )
{
  VOID                            *Buffer;

  if (Pages == 0) {
    return NULL;
  }

  //
  // Renew the pool if necessary.
  //
  if (mPageTablePool == NULL ||
      Pages > mPageTablePool->FreePages) {
    if (!InitializePageTablePool (Pages)) {
      return NULL;
    }
  }

  Buffer = (UINT8 *)mPageTablePool + mPageTablePool->Offset;

  mPageTablePool->Offset     += EFI_PAGES_TO_SIZE (Pages);
  mPageTablePool->FreePages  -= Pages;

  return Buffer;
}

/**
  Special handler for #DB exception, which will restore the page attributes
  (not-present). It should work with #PF handler which will set pages to
  'present'.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
DebugExceptionHandler (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  UINTN     CpuIndex;
  UINTN     PFEntry;
  BOOLEAN   IsWpEnabled;

  MpInitLibWhoAmI (&CpuIndex);

  //
  // Clear last PF entries
  //
  IsWpEnabled = IsReadOnlyPageWriteProtected ();
  if (IsWpEnabled) {
    DisableReadOnlyPageWriteProtect ();
  }

  for (PFEntry = 0; PFEntry < mPFEntryCount[CpuIndex]; PFEntry++) {
    if (mLastPFEntryPointer[CpuIndex][PFEntry] != NULL) {
      *mLastPFEntryPointer[CpuIndex][PFEntry] &= ~(UINT64)IA32_PG_P;
    }
  }

  if (IsWpEnabled) {
    EnableReadOnlyPageWriteProtect ();
  }

  //
  // Reset page fault exception count for next page fault.
  //
  mPFEntryCount[CpuIndex] = 0;

  //
  // Flush TLB
  //
  CpuFlushTlb ();

  //
  // Clear TF in EFLAGS
  //
  if (mPagingContext.MachineType == IMAGE_FILE_MACHINE_I386) {
    SystemContext.SystemContextIa32->Eflags &= (UINT32)~BIT8;
  } else {
    SystemContext.SystemContextX64->Rflags &= (UINT64)~BIT8;
  }
}

/**
  Special handler for #PF exception, which will set the pages which caused
  #PF to be 'present'. The attribute of those pages should be restored in
  the subsequent #DB handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
PageFaultExceptionHandler (
  IN EFI_EXCEPTION_TYPE   ExceptionType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  EFI_STATUS                      Status;
  UINT64                          PFAddress;
  PAGE_TABLE_LIB_PAGING_CONTEXT   PagingContext;
  PAGE_ATTRIBUTE                  PageAttribute;
  UINT64                          Attributes;
  UINT64                          *PageEntry;
  UINTN                           Index;
  UINTN                           CpuIndex;
  UINTN                           PageNumber;
  BOOLEAN                         NonStopMode;

  PFAddress = AsmReadCr2 () & ~EFI_PAGE_MASK;
  if (PFAddress < BASE_4KB) {
    NonStopMode = NULL_DETECTION_NONSTOP_MODE ? TRUE : FALSE;
  } else {
    NonStopMode = HEAP_GUARD_NONSTOP_MODE ? TRUE : FALSE;
  }

  if (NonStopMode) {
    MpInitLibWhoAmI (&CpuIndex);
    GetCurrentPagingContext (&PagingContext);
    //
    // Memory operation cross page boundary, like "rep mov" instruction, will
    // cause infinite loop between this and Debug Trap handler. We have to make
    // sure that current page and the page followed are both in PRESENT state.
    //
    PageNumber = 2;
    while (PageNumber > 0) {
      PageEntry = GetPageTableEntry (&PagingContext, PFAddress, &PageAttribute);
      ASSERT(PageEntry != NULL);

      if (PageEntry != NULL) {
        Attributes = GetAttributesFromPageEntry (PageEntry);
        if ((Attributes & EFI_MEMORY_RP) != 0) {
          Attributes &= ~EFI_MEMORY_RP;
          Status = AssignMemoryPageAttributes (&PagingContext, PFAddress,
                                               EFI_PAGE_SIZE, Attributes, NULL);
          if (!EFI_ERROR(Status)) {
            Index = mPFEntryCount[CpuIndex];
            //
            // Re-retrieve page entry because above calling might update page
            // table due to table split.
            //
            PageEntry = GetPageTableEntry (&PagingContext, PFAddress, &PageAttribute);
            mLastPFEntryPointer[CpuIndex][Index++] = PageEntry;
            mPFEntryCount[CpuIndex] = Index;
          }
        }
      }

      PFAddress += EFI_PAGE_SIZE;
      --PageNumber;
    }
  }

  //
  // Initialize the serial port before dumping.
  //
  SerialPortInitialize ();
  //
  // Display ExceptionType, CPU information and Image information
  //
  DumpCpuContext (ExceptionType, SystemContext);
  if (NonStopMode) {
    //
    // Set TF in EFLAGS
    //
    if (mPagingContext.MachineType == IMAGE_FILE_MACHINE_I386) {
      SystemContext.SystemContextIa32->Eflags |= (UINT32)BIT8;
    } else {
      SystemContext.SystemContextX64->Rflags |= (UINT64)BIT8;
    }
  } else {
    CpuDeadLoop ();
  }
}

/**
  Initialize the Page Table lib.
**/
VOID
InitializePageTableLib (
  VOID
  )
{
  PAGE_TABLE_LIB_PAGING_CONTEXT     CurrentPagingContext;
  UINT32                            *Attributes;
  UINTN                             *PageTableBase;

  GetCurrentPagingContext (&CurrentPagingContext);

  GetPagingDetails (&CurrentPagingContext.ContextData, &PageTableBase, &Attributes);

  //
  // Reserve memory of page tables for future uses, if paging is enabled.
  //
  if ((*PageTableBase != 0) &&
      (*Attributes & PAGE_TABLE_LIB_PAGING_CONTEXT_IA32_X64_ATTRIBUTES_PAE) != 0) {
    DisableReadOnlyPageWriteProtect ();
    InitializePageTablePool (1);
    EnableReadOnlyPageWriteProtect ();
  }

  if (HEAP_GUARD_NONSTOP_MODE || NULL_DETECTION_NONSTOP_MODE) {
    mPFEntryCount = (UINTN *)AllocateZeroPool (sizeof (UINTN) * mNumberOfProcessors);
    ASSERT (mPFEntryCount != NULL);

    mLastPFEntryPointer = (UINT64 *(*)[MAX_PF_ENTRY_COUNT])
                          AllocateZeroPool (sizeof (mLastPFEntryPointer[0]) * mNumberOfProcessors);
    ASSERT (mLastPFEntryPointer != NULL);
  }

  DEBUG ((DEBUG_INFO, "CurrentPagingContext:\n"));
  DEBUG ((DEBUG_INFO, "  MachineType   - 0x%x\n", CurrentPagingContext.MachineType));
  DEBUG ((DEBUG_INFO, "  PageTableBase - 0x%Lx\n", (UINT64)*PageTableBase));
  DEBUG ((DEBUG_INFO, "  Attributes    - 0x%x\n", *Attributes));

  return ;
}

