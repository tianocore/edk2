/** @file
  Capsule update PEIM for UEFI2.0

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Capsule.h"

#define DEFAULT_SG_LIST_HEADS  (20)

#ifdef MDE_CPU_IA32
//
// Global Descriptor Table (GDT)
//
GLOBAL_REMOVE_IF_UNREFERENCED IA32_SEGMENT_DESCRIPTOR  mGdtEntries[] = {
  /* selector { Global Segment Descriptor                              } */
  /* 0x00 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // null descriptor
  /* 0x08 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear data segment descriptor
  /* 0x10 */ {
    { 0xffff, 0, 0, 0xf, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // linear code segment descriptor
  /* 0x18 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x20 */ {
    { 0xffff, 0, 0, 0xb, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x28 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
  /* 0x30 */ {
    { 0xffff, 0, 0, 0x3, 1, 0, 1, 0xf, 0, 0, 1, 1, 0 }
  },                                                                      // system data segment descriptor
  /* 0x38 */ {
    { 0xffff, 0, 0, 0xb, 1, 0, 1, 0xf, 0, 1, 0, 1, 0 }
  },                                                                      // system code segment descriptor
  /* 0x40 */ {
    { 0,      0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 0 }
  },                                                                      // spare segment descriptor
};

//
// IA32 Gdt register
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST IA32_DESCRIPTOR  mGdt = {
  sizeof (mGdtEntries) - 1,
  (UINTN)mGdtEntries
};

/**
  The function will check if 1G page is supported.

  @retval TRUE   1G page is supported.
  @retval FALSE  1G page is not supported.

**/
BOOLEAN
IsPage1GSupport (
  VOID
  )
{
  UINT32   RegEax;
  UINT32   RegEdx;
  BOOLEAN  Page1GSupport;

  Page1GSupport = FALSE;
  if (PcdGetBool (PcdUse1GPageTable)) {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000001) {
      AsmCpuid (0x80000001, NULL, NULL, NULL, &RegEdx);
      if ((RegEdx & BIT26) != 0) {
        Page1GSupport = TRUE;
      }
    }
  }

  return Page1GSupport;
}

/**
  Calculate the total size of page table.

  @param[in] Page1GSupport      1G page support or not.

  @return The size of page table.

**/
UINTN
CalculatePageTableSize (
  IN BOOLEAN  Page1GSupport
  )
{
  UINTN   ExtraPageTablePages;
  UINTN   TotalPagesNum;
  UINT8   PhysicalAddressBits;
  UINT32  NumberOfPml4EntriesNeeded;
  UINT32  NumberOfPdpEntriesNeeded;

  //
  // Create 4G page table by default,
  // and let PF handler to handle > 4G request.
  //
  PhysicalAddressBits = 32;
  ExtraPageTablePages = EXTRA_PAGE_TABLE_PAGES;

  //
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded  = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded  = 512;
  }

  if (!Page1GSupport) {
    TotalPagesNum = (NumberOfPdpEntriesNeeded + 1) * NumberOfPml4EntriesNeeded + 1;
  } else {
    TotalPagesNum = NumberOfPml4EntriesNeeded + 1;
  }

  TotalPagesNum += ExtraPageTablePages;

  return EFI_PAGES_TO_SIZE (TotalPagesNum);
}

/**
  Allocates and fills in the Page Directory and Page Table Entries to
  establish a 4G page table.

  @param[in] PageTablesAddress  The base address of page table.
  @param[in] Page1GSupport      1G page support or not.

**/
VOID
Create4GPageTables (
  IN EFI_PHYSICAL_ADDRESS  PageTablesAddress,
  IN BOOLEAN               Page1GSupport
  )
{
  UINT8                           PhysicalAddressBits;
  EFI_PHYSICAL_ADDRESS            PageAddress;
  UINTN                           IndexOfPml4Entries;
  UINTN                           IndexOfPdpEntries;
  UINTN                           IndexOfPageDirectoryEntries;
  UINT32                          NumberOfPml4EntriesNeeded;
  UINT32                          NumberOfPdpEntriesNeeded;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMapLevel4Entry;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageMap;
  PAGE_MAP_AND_DIRECTORY_POINTER  *PageDirectoryPointerEntry;
  PAGE_TABLE_ENTRY                *PageDirectoryEntry;
  UINTN                           BigPageAddress;
  PAGE_TABLE_1G_ENTRY             *PageDirectory1GEntry;
  UINT64                          AddressEncMask;

  //
  // Make sure AddressEncMask is contained to smallest supported address field.
  //
  AddressEncMask = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  //
  // Create 4G page table by default,
  // and let PF handler to handle > 4G request.
  //
  PhysicalAddressBits = 32;

  //
  // Calculate the table entries needed.
  //
  if (PhysicalAddressBits <= 39 ) {
    NumberOfPml4EntriesNeeded = 1;
    NumberOfPdpEntriesNeeded  = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 30));
  } else {
    NumberOfPml4EntriesNeeded = (UINT32)LShiftU64 (1, (PhysicalAddressBits - 39));
    NumberOfPdpEntriesNeeded  = 512;
  }

  //
  // Pre-allocate big pages to avoid later allocations.
  //
  BigPageAddress = (UINTN)PageTablesAddress;

  //
  // By architecture only one PageMapLevel4 exists - so lets allocate storage for it.
  //
  PageMap         = (VOID *)BigPageAddress;
  BigPageAddress += SIZE_4KB;

  PageMapLevel4Entry = PageMap;
  PageAddress        = 0;
  for (IndexOfPml4Entries = 0; IndexOfPml4Entries < NumberOfPml4EntriesNeeded; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    //
    // Each PML4 entry points to a page of Page Directory Pointer entires.
    // So lets allocate space for them and fill them in in the IndexOfPdpEntries loop.
    //
    PageDirectoryPointerEntry = (VOID *)BigPageAddress;
    BigPageAddress           += SIZE_4KB;

    //
    // Make a PML4 Entry
    //
    PageMapLevel4Entry->Uint64         = (UINT64)(UINTN)PageDirectoryPointerEntry | AddressEncMask;
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present   = 1;

    if (Page1GSupport) {
      PageDirectory1GEntry = (VOID *)PageDirectoryPointerEntry;

      for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectory1GEntry++, PageAddress += SIZE_1GB) {
        //
        // Fill in the Page Directory entries
        //
        PageDirectory1GEntry->Uint64         = (UINT64)PageAddress | AddressEncMask;
        PageDirectory1GEntry->Bits.ReadWrite = 1;
        PageDirectory1GEntry->Bits.Present   = 1;
        PageDirectory1GEntry->Bits.MustBe1   = 1;
      }
    } else {
      for (IndexOfPdpEntries = 0; IndexOfPdpEntries < NumberOfPdpEntriesNeeded; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        //
        // Each Directory Pointer entries points to a page of Page Directory entires.
        // So allocate space for them and fill them in in the IndexOfPageDirectoryEntries loop.
        //
        PageDirectoryEntry = (VOID *)BigPageAddress;
        BigPageAddress    += SIZE_4KB;

        //
        // Fill in a Page Directory Pointer Entries
        //
        PageDirectoryPointerEntry->Uint64         = (UINT64)(UINTN)PageDirectoryEntry | AddressEncMask;
        PageDirectoryPointerEntry->Bits.ReadWrite = 1;
        PageDirectoryPointerEntry->Bits.Present   = 1;

        for (IndexOfPageDirectoryEntries = 0; IndexOfPageDirectoryEntries < 512; IndexOfPageDirectoryEntries++, PageDirectoryEntry++, PageAddress += SIZE_2MB) {
          //
          // Fill in the Page Directory entries
          //
          PageDirectoryEntry->Uint64         = (UINT64)PageAddress | AddressEncMask;
          PageDirectoryEntry->Bits.ReadWrite = 1;
          PageDirectoryEntry->Bits.Present   = 1;
          PageDirectoryEntry->Bits.MustBe1   = 1;
        }
      }

      for ( ; IndexOfPdpEntries < 512; IndexOfPdpEntries++, PageDirectoryPointerEntry++) {
        ZeroMem (
          PageDirectoryPointerEntry,
          sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
          );
      }
    }
  }

  //
  // For the PML4 entries we are not using fill in a null entry.
  //
  for ( ; IndexOfPml4Entries < 512; IndexOfPml4Entries++, PageMapLevel4Entry++) {
    ZeroMem (
      PageMapLevel4Entry,
      sizeof (PAGE_MAP_AND_DIRECTORY_POINTER)
      );
  }
}

/**
  Return function from long mode to 32-bit mode.

  @param  EntrypointContext  Context for mode switching
  @param  ReturnContext      Context for mode switching

**/
VOID
ReturnFunction (
  SWITCH_32_TO_64_CONTEXT  *EntrypointContext,
  SWITCH_64_TO_32_CONTEXT  *ReturnContext
  )
{
  //
  // Restore original GDT
  //
  AsmWriteGdtr (&ReturnContext->Gdtr);

  //
  // return to original caller
  //
  LongJump ((BASE_LIBRARY_JUMP_BUFFER  *)(UINTN)EntrypointContext->JumpBuffer, 1);

  //
  // never be here
  //
  ASSERT (FALSE);
}

/**
  Thunk function from 32-bit protection mode to long mode.

  @param  PageTableAddress  Page table base address
  @param  Context           Context for mode switching
  @param  ReturnContext     Context for mode switching

  @retval EFI_SUCCESS  Function successfully executed.

**/
EFI_STATUS
Thunk32To64 (
  EFI_PHYSICAL_ADDRESS     PageTableAddress,
  SWITCH_32_TO_64_CONTEXT  *Context,
  SWITCH_64_TO_32_CONTEXT  *ReturnContext
  )
{
  UINTN       SetJumpFlag;
  EFI_STATUS  Status;

  //
  // Save return address, LongJump will return here then
  //
  SetJumpFlag = SetJump ((BASE_LIBRARY_JUMP_BUFFER  *)(UINTN)Context->JumpBuffer);

  if (SetJumpFlag == 0) {
    //
    // Build 4G Page Tables.
    //
    Create4GPageTables (PageTableAddress, Context->Page1GSupport);

    //
    // Create 64-bit GDT
    //
    AsmWriteGdtr (&mGdt);

    //
    // Write CR3
    //
    AsmWriteCr3 ((UINTN)PageTableAddress);

    DEBUG ((
      DEBUG_INFO,
      "%a() Stack Base: 0x%lx, Stack Size: 0x%lx\n",
      __func__,
      Context->StackBufferBase,
      Context->StackBufferLength
      ));

    //
    // Disable interrupt of Debug timer, since the IDT table cannot work in long mode
    //
    SaveAndSetDebugTimerInterrupt (FALSE);
    //
    // Transfer to long mode
    //
    AsmEnablePaging64 (
      0x38,
      (UINT64)Context->EntryPoint,
      (UINT64)(UINTN)Context,
      (UINT64)(UINTN)ReturnContext,
      Context->StackBufferBase + Context->StackBufferLength
      );
  }

  //
  // Convert to 32-bit Status and return
  //
  Status = EFI_SUCCESS;
  if ((UINTN)ReturnContext->ReturnStatus != 0) {
    Status = ENCODE_ERROR ((UINTN)ReturnContext->ReturnStatus);
  }

  return Status;
}

/**
  If in 32 bit protection mode, and coalesce image is of X64, switch to long mode.

  @param  LongModeBuffer            The context of long mode.
  @param  CoalesceEntry             Entry of coalesce image.
  @param  BlockListAddr             Address of block list.
  @param  MemoryResource            Pointer to the buffer of memory resource descriptor.
  @param  MemoryBase                Base of memory range.
  @param  MemorySize                Size of memory range.

  @retval EFI_SUCCESS               Successfully switched to long mode and execute coalesce.
  @retval Others                    Failed to execute coalesce in long mode.

**/
EFI_STATUS
ModeSwitch (
  IN EFI_CAPSULE_LONG_MODE_BUFFER  *LongModeBuffer,
  IN COALESCE_ENTRY                CoalesceEntry,
  IN EFI_PHYSICAL_ADDRESS          BlockListAddr,
  IN MEMORY_RESOURCE_DESCRIPTOR    *MemoryResource,
  IN OUT VOID                      **MemoryBase,
  IN OUT UINTN                     *MemorySize
  )
{
  EFI_STATUS                Status;
  EFI_PHYSICAL_ADDRESS      MemoryBase64;
  UINT64                    MemorySize64;
  EFI_PHYSICAL_ADDRESS      MemoryEnd64;
  SWITCH_32_TO_64_CONTEXT   Context;
  SWITCH_64_TO_32_CONTEXT   ReturnContext;
  BASE_LIBRARY_JUMP_BUFFER  JumpBuffer;
  EFI_PHYSICAL_ADDRESS      ReservedRangeBase;
  EFI_PHYSICAL_ADDRESS      ReservedRangeEnd;
  BOOLEAN                   Page1GSupport;

  ZeroMem (&Context, sizeof (SWITCH_32_TO_64_CONTEXT));
  ZeroMem (&ReturnContext, sizeof (SWITCH_64_TO_32_CONTEXT));

  MemoryBase64 = (UINT64)(UINTN)*MemoryBase;
  MemorySize64 = (UINT64)(UINTN)*MemorySize;
  MemoryEnd64  = MemoryBase64 + MemorySize64;

  Page1GSupport = IsPage1GSupport ();

  //
  // Merge memory range reserved for stack and page table
  //
  if (LongModeBuffer->StackBaseAddress < LongModeBuffer->PageTableAddress) {
    ReservedRangeBase = LongModeBuffer->StackBaseAddress;
    ReservedRangeEnd  = LongModeBuffer->PageTableAddress + CalculatePageTableSize (Page1GSupport);
  } else {
    ReservedRangeBase = LongModeBuffer->PageTableAddress;
    ReservedRangeEnd  = LongModeBuffer->StackBaseAddress + LongModeBuffer->StackSize;
  }

  //
  // Check if memory range reserved is overlap with MemoryBase ~ MemoryBase + MemorySize.
  // If they are overlapped, get a larger range to process capsule data.
  //
  if (ReservedRangeBase <= MemoryBase64) {
    if (ReservedRangeEnd < MemoryEnd64) {
      MemoryBase64 = ReservedRangeEnd;
    } else {
      DEBUG ((DEBUG_ERROR, "Memory is not enough to process capsule!\n"));
      return EFI_OUT_OF_RESOURCES;
    }
  } else if (ReservedRangeBase < MemoryEnd64) {
    if ((ReservedRangeEnd < MemoryEnd64) &&
        (ReservedRangeBase - MemoryBase64 < MemoryEnd64 - ReservedRangeEnd))
    {
      MemoryBase64 = ReservedRangeEnd;
    } else {
      MemorySize64 = (UINT64)(UINTN)(ReservedRangeBase - MemoryBase64);
    }
  }

  //
  // Initialize context jumping to 64-bit enviroment
  //
  Context.JumpBuffer        = (EFI_PHYSICAL_ADDRESS)(UINTN)&JumpBuffer;
  Context.StackBufferBase   = LongModeBuffer->StackBaseAddress;
  Context.StackBufferLength = LongModeBuffer->StackSize;
  Context.EntryPoint        = (EFI_PHYSICAL_ADDRESS)(UINTN)CoalesceEntry;
  Context.BlockListAddr     = BlockListAddr;
  Context.MemoryResource    = (EFI_PHYSICAL_ADDRESS)(UINTN)MemoryResource;
  Context.MemoryBase64Ptr   = (EFI_PHYSICAL_ADDRESS)(UINTN)&MemoryBase64;
  Context.MemorySize64Ptr   = (EFI_PHYSICAL_ADDRESS)(UINTN)&MemorySize64;
  Context.Page1GSupport     = Page1GSupport;
  Context.AddressEncMask    = PcdGet64 (PcdPteMemoryEncryptionAddressOrMask) & PAGING_1G_ADDRESS_MASK_64;

  //
  // Prepare data for return back
  //
  ReturnContext.ReturnCs         = 0x10;
  ReturnContext.ReturnEntryPoint = (EFI_PHYSICAL_ADDRESS)(UINTN)ReturnFunction;
  //
  // Will save the return status of processing capsule
  //
  ReturnContext.ReturnStatus = 0;

  //
  // Save original GDT
  //
  AsmReadGdtr ((IA32_DESCRIPTOR *)&ReturnContext.Gdtr);

  Status = Thunk32To64 (LongModeBuffer->PageTableAddress, &Context, &ReturnContext);

  if (!EFI_ERROR (Status)) {
    *MemoryBase = (VOID *)(UINTN)MemoryBase64;
    *MemorySize = (UINTN)MemorySize64;
  }

  return Status;
}

/**
  Locates the coalesce image entry point, and detects its machine type.

  @param CoalesceImageEntryPoint   Pointer to coalesce image entry point for output.
  @param CoalesceImageMachineType  Pointer to machine type of coalesce image.

  @retval EFI_SUCCESS     Coalesce image successfully located.
  @retval Others          Failed to locate the coalesce image.

**/
EFI_STATUS
FindCapsuleCoalesceImage (
  OUT EFI_PHYSICAL_ADDRESS  *CoalesceImageEntryPoint,
  OUT UINT16                *CoalesceImageMachineType
  )
{
  EFI_STATUS             Status;
  UINTN                  Instance;
  EFI_PEI_LOAD_FILE_PPI  *LoadFile;
  EFI_PEI_FV_HANDLE      VolumeHandle;
  EFI_PEI_FILE_HANDLE    FileHandle;
  EFI_PHYSICAL_ADDRESS   CoalesceImageAddress;
  UINT64                 CoalesceImageSize;
  UINT32                 AuthenticationState;

  Instance = 0;

  while (TRUE) {
    Status = PeiServicesFfsFindNextVolume (Instance++, &VolumeHandle);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = PeiServicesFfsFindFileByName (PcdGetPtr (PcdCapsuleCoalesceFile), VolumeHandle, &FileHandle);
    if (!EFI_ERROR (Status)) {
      Status = PeiServicesLocatePpi (&gEfiPeiLoadFilePpiGuid, 0, NULL, (VOID **)&LoadFile);
      ASSERT_EFI_ERROR (Status);

      Status = LoadFile->LoadFile (
                           LoadFile,
                           FileHandle,
                           &CoalesceImageAddress,
                           &CoalesceImageSize,
                           CoalesceImageEntryPoint,
                           &AuthenticationState
                           );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Unable to find PE32 section in CapsuleX64 image ffs %r!\n", Status));
        return Status;
      }

      *CoalesceImageMachineType = PeCoffLoaderGetMachineType ((VOID *)(UINTN)CoalesceImageAddress);
      break;
    } else {
      continue;
    }
  }

  return Status;
}

/**
  Gets the reserved long mode buffer.

  @param  LongModeBuffer  Pointer to the long mode buffer for output.

  @retval EFI_SUCCESS     Long mode buffer successfully retrieved.
  @retval Others          Variable storing long mode buffer not found.

**/
EFI_STATUS
GetLongModeContext (
  OUT EFI_CAPSULE_LONG_MODE_BUFFER  *LongModeBuffer
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PPIVariableServices;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&PPIVariableServices
             );
  ASSERT_EFI_ERROR (Status);

  Size   = sizeof (EFI_CAPSULE_LONG_MODE_BUFFER);
  Status = PPIVariableServices->GetVariable (
                                  PPIVariableServices,
                                  EFI_CAPSULE_LONG_MODE_BUFFER_NAME,
                                  &gEfiCapsuleVendorGuid,
                                  NULL,
                                  &Size,
                                  LongModeBuffer
                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error Get LongModeBuffer variable %r!\n", Status));
  }

  return Status;
}

#endif

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)

/**
  Get physical address bits.

  @return Physical address bits.

**/
UINT8
GetPhysicalAddressBits (
  VOID
  )
{
  UINT32  RegEax;
  UINT8   PhysicalAddressBits;
  VOID    *Hob;

  //
  // Get physical address bits supported.
  //
  Hob = GetFirstHob (EFI_HOB_TYPE_CPU);
  if (Hob != NULL) {
    PhysicalAddressBits = ((EFI_HOB_CPU *)Hob)->SizeOfMemorySpace;
  } else {
    AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);
    if (RegEax >= 0x80000008) {
      AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);
      PhysicalAddressBits = (UINT8)RegEax;
    } else {
      PhysicalAddressBits = 36;
    }
  }

  //
  // IA-32e paging translates 48-bit linear addresses to 52-bit physical addresses.
  //
  ASSERT (PhysicalAddressBits <= 52);
  if (PhysicalAddressBits > 48) {
    PhysicalAddressBits = 48;
  }

  return PhysicalAddressBits;
}

#endif

/**
  Sort memory resource entries based upon PhysicalStart, from low to high.

  @param[in, out] MemoryResource    A pointer to the memory resource entry buffer.

**/
VOID
SortMemoryResourceDescriptor (
  IN OUT MEMORY_RESOURCE_DESCRIPTOR  *MemoryResource
  )
{
  MEMORY_RESOURCE_DESCRIPTOR  *MemoryResourceEntry;
  MEMORY_RESOURCE_DESCRIPTOR  *NextMemoryResourceEntry;
  MEMORY_RESOURCE_DESCRIPTOR  TempMemoryResource;

  MemoryResourceEntry     = MemoryResource;
  NextMemoryResourceEntry = MemoryResource + 1;
  while (MemoryResourceEntry->ResourceLength != 0) {
    while (NextMemoryResourceEntry->ResourceLength != 0) {
      if (MemoryResourceEntry->PhysicalStart > NextMemoryResourceEntry->PhysicalStart) {
        CopyMem (&TempMemoryResource, MemoryResourceEntry, sizeof (MEMORY_RESOURCE_DESCRIPTOR));
        CopyMem (MemoryResourceEntry, NextMemoryResourceEntry, sizeof (MEMORY_RESOURCE_DESCRIPTOR));
        CopyMem (NextMemoryResourceEntry, &TempMemoryResource, sizeof (MEMORY_RESOURCE_DESCRIPTOR));
      }

      NextMemoryResourceEntry = NextMemoryResourceEntry + 1;
    }

    MemoryResourceEntry     = MemoryResourceEntry + 1;
    NextMemoryResourceEntry = MemoryResourceEntry + 1;
  }
}

/**
  Merge continous memory resource entries.

  @param[in, out] MemoryResource    A pointer to the memory resource entry buffer.

**/
VOID
MergeMemoryResourceDescriptor (
  IN OUT MEMORY_RESOURCE_DESCRIPTOR  *MemoryResource
  )
{
  MEMORY_RESOURCE_DESCRIPTOR  *MemoryResourceEntry;
  MEMORY_RESOURCE_DESCRIPTOR  *NewMemoryResourceEntry;
  MEMORY_RESOURCE_DESCRIPTOR  *NextMemoryResourceEntry;
  MEMORY_RESOURCE_DESCRIPTOR  *MemoryResourceEnd;

  MemoryResourceEntry    = MemoryResource;
  NewMemoryResourceEntry = MemoryResource;
  while (MemoryResourceEntry->ResourceLength != 0) {
    CopyMem (NewMemoryResourceEntry, MemoryResourceEntry, sizeof (MEMORY_RESOURCE_DESCRIPTOR));
    NextMemoryResourceEntry = MemoryResourceEntry + 1;

    while ((NextMemoryResourceEntry->ResourceLength != 0) &&
           (NextMemoryResourceEntry->PhysicalStart == (MemoryResourceEntry->PhysicalStart + MemoryResourceEntry->ResourceLength)))
    {
      MemoryResourceEntry->ResourceLength += NextMemoryResourceEntry->ResourceLength;
      if (NewMemoryResourceEntry != MemoryResourceEntry) {
        NewMemoryResourceEntry->ResourceLength += NextMemoryResourceEntry->ResourceLength;
      }

      NextMemoryResourceEntry = NextMemoryResourceEntry + 1;
    }

    MemoryResourceEntry    = NextMemoryResourceEntry;
    NewMemoryResourceEntry = NewMemoryResourceEntry + 1;
  }

  //
  // Set NULL terminate memory resource descriptor after merging.
  //
  MemoryResourceEnd = NewMemoryResourceEntry;
  ZeroMem (MemoryResourceEnd, sizeof (MEMORY_RESOURCE_DESCRIPTOR));
}

/**
  Build memory resource descriptor from resource descriptor in HOB list.

  @return Pointer to the buffer of memory resource descriptor.
          NULL if no memory resource descriptor reported in HOB list
          before capsule Coalesce.

**/
MEMORY_RESOURCE_DESCRIPTOR *
BuildMemoryResourceDescriptor (
  VOID
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  UINTN                        Index;
  EFI_HOB_RESOURCE_DESCRIPTOR  *ResourceDescriptor;
  MEMORY_RESOURCE_DESCRIPTOR   *MemoryResource;
  EFI_STATUS                   Status;

  //
  // Get the count of memory resource descriptor.
  //
  Index   = 0;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    if (ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      Index++;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  if (Index == 0) {
    DEBUG ((DEBUG_INFO | DEBUG_WARN, "No memory resource descriptor reported in HOB list before capsule Coalesce\n"));
 #if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
    //
    // Allocate memory to hold memory resource descriptor,
    // include extra one NULL terminate memory resource descriptor.
    //
    Status = PeiServicesAllocatePool ((1 + 1) * sizeof (MEMORY_RESOURCE_DESCRIPTOR), (VOID **)&MemoryResource);
    ASSERT_EFI_ERROR (Status);
    ZeroMem (MemoryResource, (1 + 1) * sizeof (MEMORY_RESOURCE_DESCRIPTOR));

    MemoryResource[0].PhysicalStart  = 0;
    MemoryResource[0].ResourceLength = LShiftU64 (1, GetPhysicalAddressBits ());
    DEBUG ((
      DEBUG_INFO,
      "MemoryResource[0x0] - Start(0x%0lx) Length(0x%0lx)\n",
      MemoryResource[0x0].PhysicalStart,
      MemoryResource[0x0].ResourceLength
      ));
    return MemoryResource;
 #else
    return NULL;
 #endif
  }

  //
  // Allocate memory to hold memory resource descriptor,
  // include extra one NULL terminate memory resource descriptor.
  //
  Status = PeiServicesAllocatePool ((Index + 1) * sizeof (MEMORY_RESOURCE_DESCRIPTOR), (VOID **)&MemoryResource);
  ASSERT_EFI_ERROR (Status);
  ZeroMem (MemoryResource, (Index + 1) * sizeof (MEMORY_RESOURCE_DESCRIPTOR));

  //
  // Get the content of memory resource descriptor.
  //
  Index   = 0;
  Hob.Raw = GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while (Hob.Raw != NULL) {
    ResourceDescriptor = (EFI_HOB_RESOURCE_DESCRIPTOR *)Hob.Raw;
    if (ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      DEBUG ((
        DEBUG_INFO,
        "MemoryResource[0x%x] - Start(0x%0lx) Length(0x%0lx)\n",
        Index,
        ResourceDescriptor->PhysicalStart,
        ResourceDescriptor->ResourceLength
        ));
      MemoryResource[Index].PhysicalStart  = ResourceDescriptor->PhysicalStart;
      MemoryResource[Index].ResourceLength = ResourceDescriptor->ResourceLength;
      Index++;
    }

    Hob.Raw = GET_NEXT_HOB (Hob);
    Hob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, Hob.Raw);
  }

  SortMemoryResourceDescriptor (MemoryResource);
  MergeMemoryResourceDescriptor (MemoryResource);

  DEBUG ((DEBUG_INFO, "Dump MemoryResource[] after sorted and merged\n"));
  for (Index = 0; MemoryResource[Index].ResourceLength != 0; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "  MemoryResource[0x%x] - Start(0x%0lx) Length(0x%0lx)\n",
      Index,
      MemoryResource[Index].PhysicalStart,
      MemoryResource[Index].ResourceLength
      ));
  }

  return MemoryResource;
}

/**
  Check if the capsules are staged.

  @retval TRUE              The capsules are staged.
  @retval FALSE             The capsules are not staged.

**/
BOOLEAN
AreCapsulesStaged (
  VOID
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PPIVariableServices;
  EFI_PHYSICAL_ADDRESS             CapsuleDataPtr64;

  CapsuleDataPtr64 = 0;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&PPIVariableServices
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to find ReadOnlyVariable2PPI\n"));
    return FALSE;
  }

  //
  // Check for Update capsule
  //
  Size   = sizeof (CapsuleDataPtr64);
  Status = PPIVariableServices->GetVariable (
                                  PPIVariableServices,
                                  EFI_CAPSULE_VARIABLE_NAME,
                                  &gEfiCapsuleVendorGuid,
                                  NULL,
                                  &Size,
                                  (VOID *)&CapsuleDataPtr64
                                  );

  if (!EFI_ERROR (Status)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check all the variables for SG list heads and get the count and addresses.

  @param ListLength               A pointer would return the SG list length.
  @param HeadList                 A ponter to the capsule SG list.

  @retval EFI_SUCCESS             a valid capsule is present
  @retval EFI_NOT_FOUND           if a valid capsule is not present
  @retval EFI_INVALID_PARAMETER   the input parameter is invalid
  @retval EFI_OUT_OF_RESOURCES    fail to allocate memory

**/
EFI_STATUS
GetScatterGatherHeadEntries (
  OUT UINTN                 *ListLength,
  OUT EFI_PHYSICAL_ADDRESS  **HeadList
  )
{
  EFI_STATUS                       Status;
  UINTN                            Size;
  UINTN                            Index;
  UINTN                            TempIndex;
  UINTN                            ValidIndex;
  BOOLEAN                          Flag;
  CHAR16                           CapsuleVarName[30];
  CHAR16                           *TempVarName;
  EFI_PHYSICAL_ADDRESS             CapsuleDataPtr64;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *PPIVariableServices;
  EFI_PHYSICAL_ADDRESS             *TempList;
  EFI_PHYSICAL_ADDRESS             *EnlargedTempList;
  UINTN                            TempListLength;

  Index             = 0;
  TempVarName       = NULL;
  CapsuleVarName[0] = 0;
  ValidIndex        = 0;
  CapsuleDataPtr64  = 0;

  if ((ListLength == NULL) || (HeadList == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a Invalid parameters.  Inputs can't be NULL\n", __func__));
    ASSERT (ListLength != NULL);
    ASSERT (HeadList != NULL);
    return EFI_INVALID_PARAMETER;
  }

  *ListLength = 0;
  *HeadList   = NULL;

  Status = PeiServicesLocatePpi (
             &gEfiPeiReadOnlyVariable2PpiGuid,
             0,
             NULL,
             (VOID **)&PPIVariableServices
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to find ReadOnlyVariable2PPI\n"));
    return Status;
  }

  //
  // Allocate memory for sg list head
  //
  TempListLength = DEFAULT_SG_LIST_HEADS * sizeof (EFI_PHYSICAL_ADDRESS);
  TempList       = AllocateZeroPool (TempListLength);
  if (TempList == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // setup var name buffer for update capsules
  //
  StrCpyS (CapsuleVarName, sizeof (CapsuleVarName) / sizeof (CHAR16), EFI_CAPSULE_VARIABLE_NAME);
  TempVarName = CapsuleVarName + StrLen (CapsuleVarName);
  while (TRUE) {
    if (Index != 0) {
      UnicodeValueToStringS (
        TempVarName,
        (sizeof (CapsuleVarName) - ((UINTN)TempVarName - (UINTN)CapsuleVarName)),
        0,
        Index,
        0
        );
    }

    Size   = sizeof (CapsuleDataPtr64);
    Status = PPIVariableServices->GetVariable (
                                    PPIVariableServices,
                                    CapsuleVarName,
                                    &gEfiCapsuleVendorGuid,
                                    NULL,
                                    &Size,
                                    (VOID *)&CapsuleDataPtr64
                                    );

    if (EFI_ERROR (Status)) {
      if (Status != EFI_NOT_FOUND) {
        DEBUG ((DEBUG_ERROR, "Unexpected error getting Capsule Update variable.  Status = %r\n", Status));
      }

      break;
    }

    //
    // If this BlockList has been linked before, skip this variable
    //
    Flag = FALSE;
    for (TempIndex = 0; TempIndex < ValidIndex; TempIndex++) {
      if (TempList[TempIndex] == CapsuleDataPtr64) {
        Flag = TRUE;
        break;
      }
    }

    if (Flag) {
      Index++;
      continue;
    }

    //
    // The TempList is full, enlarge it
    //
    if ((ValidIndex + 1) >= TempListLength) {
      EnlargedTempList = AllocateZeroPool (TempListLength * 2);
      if (EnlargedTempList == NULL) {
        DEBUG ((DEBUG_ERROR, "Fail to allocate memory!\n"));
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (EnlargedTempList, TempList, TempListLength);
      FreePool (TempList);
      TempList        = EnlargedTempList;
      TempListLength *= 2;
    }

    //
    // add it to the cached list
    //
    TempList[ValidIndex++] = CapsuleDataPtr64;
    Index++;
  }

  if (ValidIndex == 0) {
    DEBUG ((DEBUG_ERROR, "%a didn't find any SG lists in variables\n", __func__));
    return EFI_NOT_FOUND;
  }

  *HeadList = AllocateZeroPool ((ValidIndex + 1) * sizeof (EFI_PHYSICAL_ADDRESS));
  if (*HeadList == NULL) {
    DEBUG ((DEBUG_ERROR, "Failed to allocate memory\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*HeadList, TempList, (ValidIndex) * sizeof (EFI_PHYSICAL_ADDRESS));
  *ListLength = ValidIndex;

  return EFI_SUCCESS;
}

/**
  Capsule PPI service to coalesce a fragmented capsule in memory.

  @param PeiServices  General purpose services available to every PEIM.
  @param MemoryBase   Pointer to the base of a block of memory that we can walk
                      all over while trying to coalesce our buffers.
                      On output, this variable will hold the base address of
                      a coalesced capsule.
  @param MemorySize   Size of the memory region pointed to by MemoryBase.
                      On output, this variable will contain the size of the
                      coalesced capsule.

  @retval EFI_NOT_FOUND   if we can't determine the boot mode
                          if the boot mode is not flash-update
                          if we could not find the capsule descriptors

  @retval EFI_BUFFER_TOO_SMALL
                          if we could not coalesce the capsule in the memory
                          region provided to us

  @retval EFI_SUCCESS     if there's no capsule, or if we processed the
                          capsule successfully.
**/
EFI_STATUS
EFIAPI
CapsuleCoalesce (
  IN     EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID              **MemoryBase,
  IN OUT UINTN             *MemorySize
  )
{
  EFI_STATUS                  Status;
  EFI_BOOT_MODE               BootMode;
  UINTN                       ListLength;
  EFI_PHYSICAL_ADDRESS        *VariableArrayAddress;
  MEMORY_RESOURCE_DESCRIPTOR  *MemoryResource;

 #ifdef MDE_CPU_IA32
  UINT16                        CoalesceImageMachineType;
  EFI_PHYSICAL_ADDRESS          CoalesceImageEntryPoint;
  COALESCE_ENTRY                CoalesceEntry;
  EFI_CAPSULE_LONG_MODE_BUFFER  LongModeBuffer;
 #endif

  ListLength           = 0;
  VariableArrayAddress = NULL;

  //
  // Someone should have already ascertained the boot mode. If it's not
  // capsule update, then return normally.
  //
  Status = PeiServicesGetBootMode (&BootMode);
  if (EFI_ERROR (Status) || (BootMode != BOOT_ON_FLASH_UPDATE)) {
    DEBUG ((DEBUG_ERROR, "Boot mode is not correct for capsule update path.\n"));
    Status = EFI_NOT_FOUND;
    goto Done;
  }

  //
  // Get SG list entries
  //
  Status = GetScatterGatherHeadEntries (&ListLength, &VariableArrayAddress);
  if (EFI_ERROR (Status) || (VariableArrayAddress == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a failed to get Scatter Gather List Head Entries.  Status = %r\n", __func__, Status));
    goto Done;
  }

  MemoryResource = BuildMemoryResourceDescriptor ();

 #ifdef MDE_CPU_IA32
  if (FeaturePcdGet (PcdDxeIplSwitchToLongMode)) {
    //
    // Switch to 64-bit mode to process capsule data when:
    // 1. When DXE phase is 64-bit
    // 2. When the buffer for 64-bit transition exists
    // 3. When Capsule X64 image is built in BIOS image
    // In 64-bit mode, we can process capsule data above 4GB.
    //
    CoalesceImageEntryPoint = 0;
    Status                  = GetLongModeContext (&LongModeBuffer);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Fail to find the variable for long mode context!\n"));
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    Status = FindCapsuleCoalesceImage (&CoalesceImageEntryPoint, &CoalesceImageMachineType);
    if ((EFI_ERROR (Status)) || (CoalesceImageMachineType != EFI_IMAGE_MACHINE_X64)) {
      DEBUG ((DEBUG_ERROR, "Fail to find CapsuleX64 module in FV!\n"));
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    ASSERT (CoalesceImageEntryPoint != 0);
    CoalesceEntry = (COALESCE_ENTRY)(UINTN)CoalesceImageEntryPoint;
    Status        = ModeSwitch (&LongModeBuffer, CoalesceEntry, (EFI_PHYSICAL_ADDRESS)(UINTN)VariableArrayAddress, MemoryResource, MemoryBase, MemorySize);
  } else {
    //
    // Capsule is processed in IA32 mode.
    //
    Status = CapsuleDataCoalesce (PeiServices, (EFI_PHYSICAL_ADDRESS *)(UINTN)VariableArrayAddress, MemoryResource, MemoryBase, MemorySize);
  }

 #else
  //
  // Process capsule directly.
  //
  Status = CapsuleDataCoalesce (PeiServices, (EFI_PHYSICAL_ADDRESS *)(UINTN)VariableArrayAddress, MemoryResource, MemoryBase, MemorySize);
 #endif

  DEBUG ((DEBUG_INFO, "Capsule Coalesce Status = %r!\n", Status));

  if (Status == EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "There is not enough memory to process capsule!\n"));
  }

  if (Status == EFI_NOT_FOUND) {
    DEBUG ((DEBUG_ERROR, "Fail to parse capsule descriptor in memory!\n"));
    REPORT_STATUS_CODE (
      EFI_ERROR_CODE | EFI_ERROR_MAJOR,
      (EFI_SOFTWARE_PEI_MODULE | EFI_SW_PEI_EC_INVALID_CAPSULE_DESCRIPTOR)
      );
  }

Done:
  return Status;
}

/**
  Determine if we're in capsule update boot mode.

  @param PeiServices  PEI services table

  @retval EFI_SUCCESS   if we have a capsule available
  @retval EFI_NOT_FOUND no capsule detected

**/
EFI_STATUS
EFIAPI
CheckCapsuleUpdate (
  IN EFI_PEI_SERVICES  **PeiServices
  )
{
  if (AreCapsulesStaged ()) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}

/**
  This function will look at a capsule and determine if it's a test pattern.
  If it is, then it will verify it and emit an error message if corruption is detected.

  @param PeiServices   Standard pei services pointer
  @param CapsuleBase   Base address of coalesced capsule, which is preceeded
                       by private data. Very implementation specific.

  @retval TRUE    Capsule image is the test image
  @retval FALSE   Capsule image is not the test image.

**/
BOOLEAN
CapsuleTestPattern (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN VOID              *CapsuleBase
  )
{
  UINT32   *TestPtr;
  UINT32   TestCounter;
  UINT32   TestSize;
  BOOLEAN  RetValue;

  RetValue = FALSE;

  //
  // Look at the capsule data and determine if it's a test pattern. If it
  // is, then test it now.
  //
  TestPtr = (UINT32 *)CapsuleBase;
  //
  // 0x54534554 "TEST"
  //
  if (*TestPtr == 0x54534554) {
    RetValue = TRUE;
    DEBUG ((DEBUG_INFO, "Capsule test pattern mode activated...\n"));
    TestSize = TestPtr[1] / sizeof (UINT32);
    //
    // Skip over the signature and the size fields in the pattern data header
    //
    TestPtr    += 2;
    TestCounter = 0;
    while (TestSize > 0) {
      if (*TestPtr != TestCounter) {
        DEBUG ((DEBUG_INFO, "Capsule test pattern mode FAILED: BaseAddr/FailAddr 0x%X 0x%X\n", (UINT32)(UINTN)(EFI_CAPSULE_PEIM_PRIVATE_DATA *)CapsuleBase, (UINT32)(UINTN)TestPtr));
        return TRUE;
      }

      TestPtr++;
      TestCounter++;
      TestSize--;
    }

    DEBUG ((DEBUG_INFO, "Capsule test pattern mode SUCCESS\n"));
  }

  return RetValue;
}

/**
  Capsule PPI service that gets called after memory is available. The
  capsule coalesce function, which must be called first, returns a base
  address and size, which can be anything actually. Once the memory init
  PEIM has discovered memory, then it should call this function and pass in
  the base address and size returned by the coalesce function. Then this
  function can create a capsule HOB and return.

  @param PeiServices   standard pei services pointer
  @param CapsuleBase   address returned by the capsule coalesce function. Most
                       likely this will actually be a pointer to private data.
  @param CapsuleSize   value returned by the capsule coalesce function.

  @retval EFI_VOLUME_CORRUPTED  CapsuleBase does not appear to point to a
                                coalesced capsule
  @retval EFI_SUCCESS           if all goes well.
**/
EFI_STATUS
EFIAPI
CreateState (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN VOID              *CapsuleBase,
  IN UINTN             CapsuleSize
  )
{
  EFI_STATUS                     Status;
  EFI_CAPSULE_PEIM_PRIVATE_DATA  *PrivateData;
  UINTN                          Size;
  EFI_PHYSICAL_ADDRESS           NewBuffer;
  UINTN                          CapsuleNumber;
  UINTN                          Index;
  EFI_PHYSICAL_ADDRESS           BaseAddress;
  UINT64                         Length;

  PrivateData = (EFI_CAPSULE_PEIM_PRIVATE_DATA *)CapsuleBase;
  if (PrivateData->Signature != EFI_CAPSULE_PEIM_PRIVATE_DATA_SIGNATURE) {
    return EFI_VOLUME_CORRUPTED;
  }

  if (PrivateData->CapsuleAllImageSize >= MAX_ADDRESS) {
    DEBUG ((DEBUG_ERROR, "CapsuleAllImageSize too big - 0x%lx\n", PrivateData->CapsuleAllImageSize));
    return EFI_OUT_OF_RESOURCES;
  }

  if (PrivateData->CapsuleNumber >= MAX_ADDRESS) {
    DEBUG ((DEBUG_ERROR, "CapsuleNumber too big - 0x%lx\n", PrivateData->CapsuleNumber));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Capsule Number and Capsule Offset is in the tail of Capsule data.
  //
  Size          = (UINTN)PrivateData->CapsuleAllImageSize;
  CapsuleNumber = (UINTN)PrivateData->CapsuleNumber;
  //
  // Allocate the memory so that it gets preserved into DXE
  //
  Status = PeiServicesAllocatePages (
             EfiRuntimeServicesData,
             EFI_SIZE_TO_PAGES (Size),
             &NewBuffer
             );

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "AllocatePages Failed!\n"));
    return Status;
  }

  //
  // Copy to our new buffer for DXE
  //
  DEBUG ((DEBUG_INFO, "Capsule copy from 0x%8X to 0x%8X with size 0x%8X\n", (UINTN)((UINT8 *)PrivateData + sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64)), (UINTN)NewBuffer, Size));
  CopyMem ((VOID *)(UINTN)NewBuffer, (VOID *)(UINTN)((UINT8 *)PrivateData + sizeof (EFI_CAPSULE_PEIM_PRIVATE_DATA) + (CapsuleNumber - 1) * sizeof (UINT64)), Size);
  //
  // Check for test data pattern. If it is the test pattern, then we'll
  // test it and still create the HOB so that it can be used to verify
  // that capsules don't get corrupted all the way into BDS. BDS will
  // still try to turn it into a firmware volume, but will think it's
  // corrupted so nothing will happen.
  //
  DEBUG_CODE (
    CapsuleTestPattern (PeiServices, (VOID *)(UINTN)NewBuffer);
    );

  //
  // Build the UEFI Capsule Hob for each capsule image.
  //
  for (Index = 0; Index < CapsuleNumber; Index++) {
    BaseAddress = NewBuffer + PrivateData->CapsuleOffset[Index];
    Length      = ((EFI_CAPSULE_HEADER *)((UINTN)BaseAddress))->CapsuleImageSize;

    BuildCvHob (BaseAddress, Length);
  }

  return EFI_SUCCESS;
}

CONST EFI_PEI_CAPSULE_PPI  mCapsulePpi = {
  CapsuleCoalesce,
  CheckCapsuleUpdate,
  CreateState
};

CONST EFI_PEI_PPI_DESCRIPTOR  mUefiPpiListCapsule = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiCapsulePpiGuid,
  (EFI_PEI_CAPSULE_PPI *)&mCapsulePpi
};

/**
  Entry point function for the PEIM

  @param FileHandle      Handle of the file being invoked.
  @param PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS    If we installed our PPI

**/
EFI_STATUS
EFIAPI
CapsuleMain (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  //
  // Just produce our PPI
  //
  return PeiServicesInstallPpi (&mUefiPpiListCapsule);
}
