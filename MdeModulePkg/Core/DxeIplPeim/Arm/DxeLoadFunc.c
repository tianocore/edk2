/** @file
  ARM specifc functionality for DxeLoad.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeIpl.h"

#include <Library/ArmMmuLib.h>
#include <Library/PeCoffLib.h>

/**
  Discover the code sections of the DXE core, and remap them read-only
  and executable.

  @param DxeCoreEntryPoint  The entrypoint of the DXE core executable.
  @param HobList            The list of HOBs passed to the DXE core from PEI.
**/
STATIC
VOID
RemapDxeCoreCodeReadOnly (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  EFI_PEI_HOB_POINTERS                 Hob;
  PE_COFF_LOADER_IMAGE_CONTEXT         ImageContext;
  RETURN_STATUS                        Status;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_SECTION_HEADER             *Section;
  UINTN                                Index;

  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;
  ImageContext.Handle    = NULL;

  //
  // Find the module HOB for the DXE core
  //
  for (Hob.Raw = HobList.Raw; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if ((GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_MEMORY_ALLOCATION) &&
        (CompareGuid (&Hob.MemoryAllocation->AllocDescriptor.Name, &gEfiHobMemoryAllocModuleGuid)) &&
        (Hob.MemoryAllocationModule->EntryPoint == DxeCoreEntryPoint))
    {
      ImageContext.Handle = (VOID *)(UINTN)Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress;
      break;
    }
  }

  ASSERT (ImageContext.Handle != NULL);

  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  ASSERT_RETURN_ERROR (Status);

  Hdr.Union = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((UINT8 *)ImageContext.Handle +
                                                  ImageContext.PeCoffHeaderOffset);
  ASSERT (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE);

  Section = (EFI_IMAGE_SECTION_HEADER *)((UINT8 *)Hdr.Union + sizeof (UINT32) +
                                         sizeof (EFI_IMAGE_FILE_HEADER) +
                                         Hdr.Pe32->FileHeader.SizeOfOptionalHeader
                                         );

  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    if ((Section[Index].Characteristics & EFI_IMAGE_SCN_CNT_CODE) != 0) {
      ArmSetMemoryRegionReadOnly (
        (UINTN)((UINT8 *)ImageContext.Handle + Section[Index].VirtualAddress),
        Section[Index].Misc.VirtualSize
        );

      ArmClearMemoryRegionNoExec (
        (UINTN)((UINT8 *)ImageContext.Handle + Section[Index].VirtualAddress),
        Section[Index].Misc.VirtualSize
        );
    }
  }
}

/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.
   It also installs EFI_END_OF_PEI_PPI to signal the end of PEI phase.

   @param DxeCoreEntryPoint         The entry point of DxeCore.
   @param HobList                   The start of HobList passed to DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS  DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS  HobList
  )
{
  VOID        *BaseOfStack;
  VOID        *TopOfStack;
  EFI_STATUS  Status;

  //
  // DRAM may be mapped with non-executable permissions by default, so
  // we'll need to map the DXE core code region executable explicitly.
  //
  RemapDxeCoreCodeReadOnly (DxeCoreEntryPoint, HobList);

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  if (PcdGetBool (PcdSetNxForStack)) {
    Status = ArmSetMemoryRegionNoExec ((UINTN)BaseOfStack, STACK_SIZE);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *)((UINTN)BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // End of PEI phase singal
  //
  Status = PeiServicesInstallPpi (&gEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Update the contents of BSP stack HOB to reflect the real stack info passed to DxeCore.
  //
  UpdateStackHob ((EFI_PHYSICAL_ADDRESS)(UINTN)BaseOfStack, STACK_SIZE);

  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    HobList.Raw,
    NULL,
    TopOfStack
    );
}
