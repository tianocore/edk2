/** @file
  Basic paging support for the CPU to enable Stack Guard.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseLib.h>
#include <Guid/MigratedFvInfo.h>

#include "CpuMpPei.h"
#define PAGING_4K_ADDRESS_MASK_64  0x000FFFFFFFFFF000ull

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
  UINT32                  RegEax;
  CPUID_VERSION_INFO_EDX  RegEdx;

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
  IN UINTN  Pages
  )
{
  VOID  *Address;

  Address = AllocatePages (Pages);
  if (Address != NULL) {
    ZeroMem (Address, EFI_PAGES_TO_SIZE (Pages));
  }

  return Address;
}

/**
  This function modifies the page attributes for the memory region specified
  by BaseAddress and Length to not present. This function only change page
  table, but not flush TLB. Caller have the responsbility to flush TLB.

  Caller should make sure BaseAddress and Length is at page boundary.

  @param[in]   BaseAddress      Start address of a memory region.
  @param[in]   Length           Size in bytes of the memory region.

  @retval RETURN_SUCCESS            The memory region is changed to not present.
  @retval RETURN_OUT_OF_RESOURCES   There are not enough system resources to modify
                                    the attributes.
  @retval RETURN_UNSUPPORTED        Cannot modify the attributes of given memory.

**/
RETURN_STATUS
ConvertMemoryPageToNotPresent (
  IN  PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64            Length
  )
{
  EFI_STATUS                  Status;
  UINTN                       PageTable;
  EFI_PHYSICAL_ADDRESS        Buffer;
  UINTN                       BufferSize;
  IA32_MAP_ATTRIBUTE          MapAttribute;
  IA32_MAP_ATTRIBUTE          MapMask;
  PAGING_MODE                 PagingMode;
  IA32_CR4                    Cr4;
  BOOLEAN                     Page5LevelSupport;
  UINT32                      RegEax;
  BOOLEAN                     Page1GSupport;
  CPUID_EXTENDED_CPU_SIG_EDX  RegEdx;

  if (sizeof (UINTN) == sizeof (UINT64)) {
    //
    // Check Page5Level Support or not.
    //
    Cr4.UintN         = AsmReadCr4 ();
    Page5LevelSupport = (Cr4.Bits.LA57 ? TRUE : FALSE);

    //
    // Check Page1G Support or not.
    //
    Page1GSupport = FALSE;
    AsmCpuid (CPUID_EXTENDED_FUNCTION, &RegEax, NULL, NULL, NULL);
    if (RegEax >= CPUID_EXTENDED_CPU_SIG) {
      AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &RegEdx.Uint32);
      if (RegEdx.Bits.Page1GB != 0) {
        Page1GSupport = TRUE;
      }
    }

    //
    // Decide Paging Mode according Page5LevelSupport & Page1GSupport.
    //
    if (Page5LevelSupport) {
      PagingMode = Page1GSupport ? Paging5Level1GB : Paging5Level;
    } else {
      PagingMode = Page1GSupport ? Paging4Level1GB : Paging4Level;
    }
  } else {
    PagingMode = PagingPae;
  }

  MapAttribute.Uint64  = 0;
  MapMask.Uint64       = 0;
  MapMask.Bits.Present = 1;
  PageTable            = AsmReadCr3 () & PAGING_4K_ADDRESS_MASK_64;
  BufferSize           = 0;

  //
  // Get required buffer size for the pagetable that will be created.
  //
  Status = PageTableMap (&PageTable, PagingMode, 0, &BufferSize, BaseAddress, Length, &MapAttribute, &MapMask, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate required Buffer.
    //
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               EFI_SIZE_TO_PAGES (BufferSize),
               &Buffer
               );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = PageTableMap (&PageTable, PagingMode, (VOID *)(UINTN)Buffer, &BufferSize, BaseAddress, Length, &MapAttribute, &MapMask, NULL);
  }

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Enable PAE Page Table.

  @retval   EFI_SUCCESS           The PAE Page Table was enabled successfully.
  @retval   EFI_OUT_OF_RESOURCES  The PAE Page Table could not be enabled due to lack of available memory.

**/
EFI_STATUS
EnablePaePageTable (
  VOID
  )
{
  EFI_STATUS  Status;

  UINTN               PageTable;
  VOID                *Buffer;
  UINTN               BufferSize;
  IA32_MAP_ATTRIBUTE  MapAttribute;
  IA32_MAP_ATTRIBUTE  MapMask;

  PageTable                   = 0;
  Buffer                      = NULL;
  BufferSize                  = 0;
  MapAttribute.Uint64         = 0;
  MapMask.Uint64              = MAX_UINT64;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;

  //
  // 1:1 map 4GB in 32bit mode
  //
  Status = PageTableMap (&PageTable, PagingPae, 0, &BufferSize, 0, SIZE_4GB, &MapAttribute, &MapMask, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate required Buffer.
  //
  Buffer = AllocatePageTableMemory (EFI_SIZE_TO_PAGES (BufferSize));
  ASSERT (Buffer != NULL);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PageTableMap (&PageTable, PagingPae, Buffer, &BufferSize, 0, SIZE_4GB, &MapAttribute, &MapMask, NULL);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status) || (PageTable == 0)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Write the Pagetable to CR3.
  //
  AsmWriteCr3 (PageTable);

  //
  // Enable CR4.PAE
  //
  AsmWriteCr4 (AsmReadCr4 () | BIT5);

  //
  // Enable CR0.PG
  //
  AsmWriteCr0 (AsmReadCr0 () | BIT31);

  DEBUG ((
    DEBUG_INFO,
    "EnablePaePageTable: Created PageTable = 0x%x, BufferSize = %x\n",
    PageTable,
    BufferSize
    ));

  return Status;
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
  IN OUT VOID  *Buffer
  )
{
  EFI_PHYSICAL_ADDRESS  StackBase;
  UINTN                 Index;

  MpInitLibWhoAmI (&Index);
  StackBase  = (EFI_PHYSICAL_ADDRESS)(UINTN)&StackBase;
  StackBase += BASE_4KB;
  StackBase &= ~((EFI_PHYSICAL_ADDRESS)BASE_4KB - 1);
  StackBase -= PcdGet32 (PcdCpuApStackSize);

  *((EFI_PHYSICAL_ADDRESS *)Buffer + Index) = StackBase;
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
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_PHYSICAL_ADDRESS  *StackBase;
  UINTN                 NumberOfProcessors;
  UINTN                 Bsp;
  UINTN                 Index;
  EFI_STATUS            Status;

  //
  // One extra page at the bottom of the stack is needed for Guard page.
  //
  if (PcdGet32 (PcdCpuApStackSize) <= EFI_PAGE_SIZE) {
    DEBUG ((DEBUG_ERROR, "PcdCpuApStackSize is not big enough for Stack Guard!\n"));
    ASSERT (FALSE);
  }

  Status = MpInitLibGetNumberOfProcessors (&NumberOfProcessors, NULL);
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    NumberOfProcessors = 1;
  }

  StackBase = (EFI_PHYSICAL_ADDRESS *)AllocatePages (EFI_SIZE_TO_PAGES (sizeof (EFI_PHYSICAL_ADDRESS) * NumberOfProcessors));
  ASSERT (StackBase != NULL);
  if (StackBase == NULL) {
    return;
  }

  ZeroMem (StackBase, sizeof (EFI_PHYSICAL_ADDRESS) * NumberOfProcessors);
  MpInitLibStartupAllAPs (GetStackBase, FALSE, NULL, 0, (VOID *)StackBase, NULL);
  MpInitLibWhoAmI (&Bsp);
  Hob.Raw = GetHobList ();
  while ((Hob.Raw = GetNextHob (EFI_HOB_TYPE_MEMORY_ALLOCATION, Hob.Raw)) != NULL) {
    if (CompareGuid (
          &gEfiHobMemoryAllocStackGuid,
          &(Hob.MemoryAllocationStack->AllocDescriptor.Name)
          ))
    {
      StackBase[Bsp] = Hob.MemoryAllocationStack->AllocDescriptor.MemoryBaseAddress;
      break;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
  }

  for (Index = 0; Index < NumberOfProcessors; ++Index) {
    ASSERT (StackBase[Index] != 0);
    //
    // Set Guard page at stack base address.
    //
    ConvertMemoryPageToNotPresent (StackBase[Index], EFI_PAGE_SIZE);
    DEBUG ((
      DEBUG_INFO,
      "Stack Guard set at %lx [cpu%lu]!\n",
      (UINT64)StackBase[Index],
      (UINT64)Index
      ));
  }

  FreePages (StackBase, EFI_SIZE_TO_PAGES (sizeof (EFI_PHYSICAL_ADDRESS) * NumberOfProcessors));
  //
  // Publish the changes of page table.
  //
  CpuFlushTlb ();
}

/**
  Enable/setup stack guard for each processor if PcdCpuStackGuard is set to TRUE.

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
  EFI_STATUS              Status;
  BOOLEAN                 InitStackGuard;
  EDKII_MIGRATED_FV_INFO  *MigratedFvInfo;
  EFI_PEI_HOB_POINTERS    Hob;
  IA32_CR0                Cr0;

  //
  // Paging must be setup first. Otherwise the exception TSS setup during MP
  // initialization later will not contain paging information and then fail
  // the task switch (for the sake of stack switch).
  //
  InitStackGuard = FALSE;
  Hob.Raw        = NULL;
  if (IsIa32PaeSupported ()) {
    Hob.Raw        = GetFirstGuidHob (&gEdkiiMigratedFvInfoGuid);
    InitStackGuard = PcdGetBool (PcdCpuStackGuard);
  }

  //
  // Some security features depend on the page table enabling. So, here
  // is to enable paging if it is not enabled (only in 32bit mode).
  //
  Cr0.UintN = AsmReadCr0 ();
  if ((Cr0.Bits.PG == 0) && (InitStackGuard || (Hob.Raw != NULL))) {
    ASSERT (sizeof (UINTN) == sizeof (UINT32));

    Status = EnablePaePageTable ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MemoryDiscoveredPpiNotifyCallback: Failed to enable PAE page table: %r.\n", Status));
      CpuDeadLoop ();
    }
  }

  Status = InitializeCpuMpWorker ((CONST EFI_PEI_SERVICES **)PeiServices);
  ASSERT_EFI_ERROR (Status);

  if (InitStackGuard) {
    SetupStackGuardPage ();
  }

  while (Hob.Raw != NULL) {
    MigratedFvInfo = GET_GUID_HOB_DATA (Hob);

    //
    // Enable #PF exception, so if the code access SPI after disable NEM, it will generate
    // the exception to avoid potential vulnerability.
    //
    ConvertMemoryPageToNotPresent (MigratedFvInfo->FvOrgBase, MigratedFvInfo->FvLength);

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextGuidHob (&gEdkiiMigratedFvInfoGuid, Hob.Raw);
  }

  CpuFlushTlb ();

  return Status;
}
