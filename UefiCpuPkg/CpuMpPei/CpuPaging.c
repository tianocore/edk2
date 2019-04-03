/** @file
  Basic paging support for the CPU to enable Stack Guard.

Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Cpuid.h>
#include <Register/Msr.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseLib.h>

#include "CpuMpPei.h"

#define IA32_PG_P             BIT0
#define IA32_PG_RW            BIT1
#define IA32_PG_U             BIT2
#define IA32_PG_A             BIT5
#define IA32_PG_D             BIT6
#define IA32_PG_PS            BIT7
#define IA32_PG_NX            BIT63

#define PAGE_ATTRIBUTE_BITS   (IA32_PG_RW | IA32_PG_P)
#define PAGE_PROGATE_BITS     (IA32_PG_D | IA32_PG_A | IA32_PG_NX | IA32_PG_U |\
                               PAGE_ATTRIBUTE_BITS)

#define PAGING_PAE_INDEX_MASK       0x1FF
#define PAGING_4K_ADDRESS_MASK_64   0x000FFFFFFFFFF000ull
#define PAGING_2M_ADDRESS_MASK_64   0x000FFFFFFFE00000ull
#define PAGING_1G_ADDRESS_MASK_64   0x000FFFFFC0000000ull
#define PAGING_512G_ADDRESS_MASK_64 0x000FFF8000000000ull

typedef enum {
  PageNone = 0,
  PageMin  = 1,
  Page4K   = PageMin,
  Page2M   = 2,
  Page1G   = 3,
  Page512G = 4,
  PageMax  = Page512G
} PAGE_ATTRIBUTE;

typedef struct {
  PAGE_ATTRIBUTE   Attribute;
  UINT64           Length;
  UINT64           AddressMask;
  UINTN            AddressBitOffset;
  UINTN            AddressBitLength;
} PAGE_ATTRIBUTE_TABLE;

PAGE_ATTRIBUTE_TABLE mPageAttributeTable[] = {
  {PageNone,          0,                           0,  0, 0},
  {Page4K,     SIZE_4KB,   PAGING_4K_ADDRESS_MASK_64, 12, 9},
  {Page2M,     SIZE_2MB,   PAGING_2M_ADDRESS_MASK_64, 21, 9},
  {Page1G,     SIZE_1GB,   PAGING_1G_ADDRESS_MASK_64, 30, 9},
  {Page512G, SIZE_512GB, PAGING_512G_ADDRESS_MASK_64, 39, 9},
};

EFI_PEI_NOTIFY_DESCRIPTOR  mPostMemNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemoryDiscoveredPpiNotifyCallback
  }
};

/**
  The function will check if IA32 PAE is supported.

  @retval TRUE      IA32 PAE is supported.
  @retval FALSE     IA32 PAE is not supported.

**/
BOOLEAN
IsIa32PaeSupported (
  VOID
  )
{
  UINT32                    RegEax;
  CPUID_VERSION_INFO_EDX    RegEdx;

  AsmCpuid (CPUID_SIGNATURE, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_VERSION_INFO) {
    AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &RegEdx.Uint32);
    if (RegEdx.Bits.PAE != 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This API provides a way to allocate memory for page table.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
AllocatePageTableMemory (
  IN UINTN           Pages
  )
{
  VOID      *Address;

  Address = AllocatePages(Pages);
  if (Address != NULL) {
    ZeroMem(Address, EFI_PAGES_TO_SIZE (Pages));
  }

  return Address;
}

/**
  Get the address width supported by current processor.

  @retval 32      If processor is in 32-bit mode.
  @retval 36-48   If processor is in 64-bit mode.

**/
UINTN
GetPhysicalAddressWidth (
  VOID
  )
{
  UINT32          RegEax;

  if (sizeof(UINTN) == 4) {
    return 32;
  }

  AsmCpuid(CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
  if (RegEax >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &RegEax, NULL, NULL, NULL);
    RegEax &= 0xFF;
    if (RegEax > 48) {
      return 48;
    }

    return (UINTN)RegEax;
  }

  return 36;
}

/**
  Get the type of top level page table.

  @retval Page512G  PML4 paging.
  @retval Page1G    PAE paing.

**/
PAGE_ATTRIBUTE
GetPageTableTopLevelType (
  VOID
  )
{
  MSR_IA32_EFER_REGISTER      MsrEfer;

  MsrEfer.Uint64 = AsmReadMsr64 (MSR_CORE_IA32_EFER);

  return (MsrEfer.Bits.LMA == 1) ? Page512G : Page1G;
}

/**
  Return page table entry matching the address.

  @param[in]   Address          The address to be checked.
  @param[out]  PageAttributes   The page attribute of the page entry.

  @return The page entry.
**/
VOID *
GetPageTableEntry (
  IN  PHYSICAL_ADDRESS                  Address,
  OUT PAGE_ATTRIBUTE                    *PageAttribute
  )
{
  INTN                  Level;
  UINTN                 Index;
  UINT64                *PageTable;
  UINT64                AddressEncMask;

  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask);
  PageTable = (UINT64 *)(UINTN)(AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64);
  for (Level = (INTN)GetPageTableTopLevelType (); Level > 0; --Level) {
    Index = (UINTN)RShiftU64 (Address, mPageAttributeTable[Level].AddressBitOffset);
    Index &= PAGING_PAE_INDEX_MASK;

    //
    // No mapping?
    //
    if (PageTable[Index] == 0) {
      *PageAttribute = PageNone;
      return NULL;
    }

    //
    // Page memory?
    //
    if ((PageTable[Index] & IA32_PG_PS) != 0 || Level == PageMin) {
      *PageAttribute = (PAGE_ATTRIBUTE)Level;
      return &PageTable[Index];
    }

    //
    // Page directory or table
    //
    PageTable = (UINT64 *)(UINTN)(PageTable[Index] &
                                  ~AddressEncMask &
                                  PAGING_4K_ADDRESS_MASK_64);
  }

  *PageAttribute = PageNone;
  return NULL;
}

/**
  This function splits one page entry to smaller page entries.

  @param[in]  PageEntry        The page entry to be splitted.
  @param[in]  PageAttribute    The page attribute of the page entry.
  @param[in]  SplitAttribute   How to split the page entry.
  @param[in]  Recursively      Do the split recursively or not.

  @retval RETURN_SUCCESS            The page entry is splitted.
  @retval RETURN_INVALID_PARAMETER  If target page attribute is invalid
  @retval RETURN_OUT_OF_RESOURCES   No resource to split page entry.
**/
RETURN_STATUS
SplitPage (
  IN  UINT64                            *PageEntry,
  IN  PAGE_ATTRIBUTE                    PageAttribute,
  IN  PAGE_ATTRIBUTE                    SplitAttribute,
  IN  BOOLEAN                           Recursively
  )
{
  UINT64            BaseAddress;
  UINT64            *NewPageEntry;
  UINTN             Index;
  UINT64            AddressEncMask;
  PAGE_ATTRIBUTE    SplitTo;

  if (SplitAttribute == PageNone || SplitAttribute >= PageAttribute) {
    ASSERT (SplitAttribute != PageNone);
    ASSERT (SplitAttribute < PageAttribute);
    return RETURN_INVALID_PARAMETER;
  }

  NewPageEntry = AllocatePageTableMemory (1);
  if (NewPageEntry == NULL) {
    ASSERT (NewPageEntry != NULL);
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // One level down each step to achieve more compact page table.
  //
  SplitTo = PageAttribute - 1;
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) &
                   mPageAttributeTable[SplitTo].AddressMask;
  BaseAddress    = *PageEntry &
                   ~PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) &
                   mPageAttributeTable[PageAttribute].AddressMask;
  for (Index = 0; Index < SIZE_4KB / sizeof(UINT64); Index++) {
    NewPageEntry[Index] = BaseAddress | AddressEncMask |
                          ((*PageEntry) & PAGE_PROGATE_BITS);

    if (SplitTo != PageMin) {
      NewPageEntry[Index] |= IA32_PG_PS;
    }

    if (Recursively && SplitTo > SplitAttribute) {
      SplitPage (&NewPageEntry[Index], SplitTo, SplitAttribute, Recursively);
    }

    BaseAddress += mPageAttributeTable[SplitTo].Length;
  }

  (*PageEntry) = (UINT64)(UINTN)NewPageEntry | AddressEncMask | PAGE_ATTRIBUTE_BITS;

  return RETURN_SUCCESS;
}

/**
  This function modifies the page attributes for the memory region specified
  by BaseAddress and Length from their current attributes to the attributes
  specified by Attributes.

  Caller should make sure BaseAddress and Length is at page boundary.

  @param[in]   BaseAddress      Start address of a memory region.
  @param[in]   Length           Size in bytes of the memory region.
  @param[in]   Attributes       Bit mask of attributes to modify.

  @retval RETURN_SUCCESS            The attributes were modified for the memory
                                    region.
  @retval RETURN_INVALID_PARAMETER  Length is zero; or,
                                    Attributes specified an illegal combination
                                    of attributes that cannot be set together; or
                                    Addressis not 4KB aligned.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify
                                    the attributes.
  @retval RETURN_UNSUPPORTED        Cannot modify the attributes of given memory.

**/
RETURN_STATUS
EFIAPI
ConvertMemoryPageAttributes (
  IN  PHYSICAL_ADDRESS                  BaseAddress,
  IN  UINT64                            Length,
  IN  UINT64                            Attributes
  )
{
  UINT64                            *PageEntry;
  PAGE_ATTRIBUTE                    PageAttribute;
  RETURN_STATUS                     Status;
  EFI_PHYSICAL_ADDRESS              MaximumAddress;

  if (Length == 0 ||
      (BaseAddress & (SIZE_4KB - 1)) != 0 ||
      (Length & (SIZE_4KB - 1)) != 0) {

    ASSERT (Length > 0);
    ASSERT ((BaseAddress & (SIZE_4KB - 1)) == 0);
    ASSERT ((Length & (SIZE_4KB - 1)) == 0);

    return RETURN_INVALID_PARAMETER;
  }

  MaximumAddress = (EFI_PHYSICAL_ADDRESS)MAX_UINT32;
  if (BaseAddress > MaximumAddress ||
      Length > MaximumAddress ||
      (BaseAddress > MaximumAddress - (Length - 1))) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Below logic is to check 2M/4K page to make sure we do not waste memory.
  //
  while (Length != 0) {
    PageEntry = GetPageTableEntry (BaseAddress, &PageAttribute);
    if (PageEntry == NULL) {
      return RETURN_UNSUPPORTED;
    }

    if (PageAttribute != Page4K) {
      Status = SplitPage (PageEntry, PageAttribute, Page4K, FALSE);
      if (RETURN_ERROR (Status)) {
        return Status;
      }
      //
      // Do it again until the page is 4K.
      //
      continue;
    }

    //
    // Just take care of 'present' bit for Stack Guard.
    //
    if ((Attributes & IA32_PG_P) != 0) {
      *PageEntry |= (UINT64)IA32_PG_P;
    } else {
      *PageEntry &= ~((UINT64)IA32_PG_P);
    }

    //
    // Convert success, move to next
    //
    BaseAddress += SIZE_4KB;
    Length -= SIZE_4KB;
  }

  return RETURN_SUCCESS;
}

/**
  Get maximum size of page memory supported by current processor.

  @param[in]   TopLevelType     The type of top level page entry.

  @retval Page1G     If processor supports 1G page and PML4.
  @retval Page2M     For all other situations.

**/
PAGE_ATTRIBUTE
GetMaxMemoryPage (
  IN  PAGE_ATTRIBUTE  TopLevelType
  )
{
  UINT32          RegEax;
  UINT32          RegEdx;

  if (TopLevelType == Page512G) {
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
      AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        return Page1G;
      }
    }
  }

  return Page2M;
}

/**
  Create PML4 or PAE page table.

  @return The address of page table.

**/
UINTN
CreatePageTable (
  VOID
  )
{
  RETURN_STATUS           Status;
  UINTN                   PhysicalAddressBits;
  UINTN                   NumberOfEntries;
  PAGE_ATTRIBUTE          TopLevelPageAttr;
  UINTN                   PageTable;
  PAGE_ATTRIBUTE          MaxMemoryPage;
  UINTN                   Index;
  UINT64                  AddressEncMask;
  UINT64                  *PageEntry;
  EFI_PHYSICAL_ADDRESS    PhysicalAddress;

  TopLevelPageAttr = (PAGE_ATTRIBUTE)GetPageTableTopLevelType ();
  PhysicalAddressBits = GetPhysicalAddressWidth ();
  NumberOfEntries = (UINTN)1 << (PhysicalAddressBits -
                                 mPageAttributeTable[TopLevelPageAttr].AddressBitOffset);

  PageTable = (UINTN) AllocatePageTableMemory (1);
  if (PageTable == 0) {
    return 0;
  }

  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask);
  AddressEncMask &= mPageAttributeTable[TopLevelPageAttr].AddressMask;
  MaxMemoryPage = GetMaxMemoryPage (TopLevelPageAttr);
  PageEntry = (UINT64 *)PageTable;

  PhysicalAddress = 0;
  for (Index = 0; Index < NumberOfEntries; ++Index) {
    *PageEntry = PhysicalAddress | AddressEncMask | PAGE_ATTRIBUTE_BITS;

    //
    // Split the top page table down to the maximum page size supported
    //
    if (MaxMemoryPage < TopLevelPageAttr) {
      Status = SplitPage(PageEntry, TopLevelPageAttr, MaxMemoryPage, TRUE);
      ASSERT_EFI_ERROR (Status);
    }

    if (TopLevelPageAttr == Page1G) {
      //
      // PDPTE[2:1] (PAE Paging) must be 0. SplitPage() might change them to 1.
      //
      *PageEntry &= ~(UINT64)(IA32_PG_RW | IA32_PG_U);
    }

    PageEntry += 1;
    PhysicalAddress += mPageAttributeTable[TopLevelPageAttr].Length;
  }


  return PageTable;
}

/**
  Setup page tables and make them work.

**/
VOID
EnablePaging (
  VOID
  )
{
  UINTN                       PageTable;

  PageTable = CreatePageTable ();
  ASSERT (PageTable != 0);
  if (PageTable != 0) {
    AsmWriteCr3(PageTable);
    AsmWriteCr4 (AsmReadCr4 () | BIT5);   // CR4.PAE
    AsmWriteCr0 (AsmReadCr0 () | BIT31);  // CR0.PG
  }
}

/**
  Get the base address of current AP's stack.

  This function is called in AP's context and assumes that whole calling stacks
  (till this function) consumed by AP's wakeup procedure will not exceed 4KB.

  PcdCpuApStackSize must be configured with value taking the Guard page into
  account.

  @param[in,out] Buffer  The pointer to private data buffer.

**/
VOID
EFIAPI
GetStackBase (
  IN OUT VOID *Buffer
  )
{
  EFI_PHYSICAL_ADDRESS    StackBase;

  StackBase = (EFI_PHYSICAL_ADDRESS)(UINTN)&StackBase;
  StackBase += BASE_4KB;
  StackBase &= ~((EFI_PHYSICAL_ADDRESS)BASE_4KB - 1);
  StackBase -= PcdGet32(PcdCpuApStackSize);

  *(EFI_PHYSICAL_ADDRESS *)Buffer = StackBase;
}

/**
  Setup stack Guard page at the stack base of each processor. BSP and APs have
  different way to get stack base address.

**/
VOID
SetupStackGuardPage (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_PHYSICAL_ADDRESS        StackBase;
  UINTN                       NumberOfProcessors;
  UINTN                       Bsp;
  UINTN                       Index;

  //
  // One extra page at the bottom of the stack is needed for Guard page.
  //
  if (PcdGet32(PcdCpuApStackSize) <= EFI_PAGE_SIZE) {
    DEBUG ((DEBUG_ERROR, "PcdCpuApStackSize is not big enough for Stack Guard!\n"));
    ASSERT (FALSE);
  }

  MpInitLibGetNumberOfProcessors(&NumberOfProcessors, NULL);
  MpInitLibWhoAmI (&Bsp);
  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    StackBase = 0;

    if (Index == Bsp) {
      Hob.Raw = GetHobList ();
      while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
        if (CompareGuid (&gEfiHobMemoryAllocStackGuid,
                         &(Hob.MemoryAllocationStack->AllocDescriptor.Name))) {
          StackBase = Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress;
          break;
        }
        Hob.Raw = GET_NEXT_HOB (Hob);
      }
    } else {
      //
      // Ask AP to return is stack base address.
      //
      MpInitLibStartupThisAP(GetStackBase, Index, NULL, 0, (VOID *)&StackBase, NULL);
    }
    ASSERT (StackBase != 0);
    //
    // Set Guard page at stack base address.
    //
    ConvertMemoryPageAttributes(StackBase, EFI_PAGE_SIZE, 0);
    DEBUG ((DEBUG_INFO, "Stack Guard set at %lx [cpu%lu]!\n",
            (UINT64)StackBase, (UINT64)Index));
  }

  //
  // Publish the changes of page table.
  //
  CpuFlushTlb ();
}

/**
  Enabl/setup stack guard for each processor if PcdCpuStackGuard is set to TRUE.

  Doing this in the memory-discovered callback is to make sure the Stack Guard
  feature to cover as most PEI code as possible.

  @param[in] PeiServices          General purpose services available to every PEIM.
  @param[in] NotifyDescriptor     The notification structure this PEIM registered on install.
  @param[in] Ppi                  The memory discovered PPI.  Not used.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval others                  There's error in MP initialization.
**/
EFI_STATUS
EFIAPI
MemoryDiscoveredPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS      Status;
  BOOLEAN         InitStackGuard;

  //
  // Paging must be setup first. Otherwise the exception TSS setup during MP
  // initialization later will not contain paging information and then fail
  // the task switch (for the sake of stack switch).
  //
  InitStackGuard = FALSE;
  if (IsIa32PaeSupported () && PcdGetBool (PcdCpuStackGuard)) {
    EnablePaging ();
    InitStackGuard = TRUE;
  }

  Status = InitializeCpuMpWorker ((CONST EFI_PEI_SERVICES **)PeiServices);
  ASSERT_EFI_ERROR (Status);

  if (InitStackGuard) {
    SetupStackGuardPage ();
  }

  return Status;
}

