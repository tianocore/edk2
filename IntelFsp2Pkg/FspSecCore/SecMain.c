/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"
#include "SecFsp.h"

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI gSecTemporaryRamSupportPpi = {
  SecTemporaryRamSupport
};

EFI_PEI_PPI_DESCRIPTOR            mPeiSecPlatformInformationPpi[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gFspInApiModePpiGuid,
    NULL
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamSupportPpiGuid,
    &gSecTemporaryRamSupportPpi
  }
};

//
// These are IDT entries pointing to 08:FFFFFFE4h.
//
UINT64  mIdtEntryTemplate = 0xffff8e000008ffe4ULL;

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param[in] SizeOfRam          Size of the temporary memory available for use.
  @param[in] TempRamBase        Base address of temporary ram
  @param[in] BootFirmwareVolume Base address of the Boot Firmware Volume.
  @param[in] PeiCore            PeiCore entry point.
  @param[in] BootLoaderStack    BootLoader stack.
  @param[in] ApiIdx             the index of API.

  @return This function never returns.

**/
VOID
EFIAPI
SecStartup (
  IN UINT32                   SizeOfRam,
  IN UINT32                   TempRamBase,
  IN VOID                    *BootFirmwareVolume,
  IN PEI_CORE_ENTRY           PeiCore,
  IN UINT32                   BootLoaderStack,
  IN UINT32                   ApiIdx
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  IA32_DESCRIPTOR             IdtDescriptor;
  SEC_IDT_TABLE               IdtTableInStack;
  UINT32                      Index;
  FSP_GLOBAL_DATA             PeiFspData;
  UINT64                      ExceptionHandler;
  UINTN                       IdtSize;

  //
  // Process all libraries constructor function linked to SecCore.
  //
  ProcessLibraryConstructorList ();

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();

  //
  // Scenario 1 memory map when running on bootloader stack
  //
  // |-------------------|---->
  // |Idt Table          |
  // |-------------------|
  // |PeiService Pointer |
  // |-------------------|
  // |                   |
  // |                   |
  // |      Heap         |
  // |                   |
  // |                   |
  // |-------------------|---->  TempRamBase
  //
  //
  // |-------------------|
  // |Bootloader stack   |----> somewhere in memory, FSP will share this stack.
  // |-------------------|

  //
  // Scenario 2 memory map when running FSP on a separate stack
  //
  // |-------------------|---->
  // |Idt Table          |
  // |-------------------|
  // |PeiService Pointer |    PeiStackSize
  // |-------------------|
  // |                   |
  // |      Stack        |
  // |-------------------|---->
  // |                   |
  // |                   |
  // |      Heap         |    PeiTemporaryRamSize
  // |                   |
  // |                   |
  // |-------------------|---->  TempRamBase
  IdtTableInStack.PeiService = 0;
  AsmReadIdtr (&IdtDescriptor);
  if (IdtDescriptor.Base == 0) {
    ExceptionHandler = FspGetExceptionHandler(mIdtEntryTemplate);
    for (Index = 0; Index < FixedPcdGet8(PcdFspMaxInterruptSupported); Index ++) {
      CopyMem ((VOID*)&IdtTableInStack.IdtTable[Index], (VOID*)&ExceptionHandler, sizeof (UINT64));
    }
    IdtSize = sizeof (IdtTableInStack.IdtTable);
  } else {
    IdtSize = IdtDescriptor.Limit + 1;
    if (IdtSize > sizeof (IdtTableInStack.IdtTable)) {
      //
      // ERROR: IDT table size from boot loader is larger than FSP can support, DeadLoop here!
      //
      CpuDeadLoop();
    } else {
      CopyMem ((VOID *) (UINTN) &IdtTableInStack.IdtTable, (VOID *) IdtDescriptor.Base, IdtSize);
    }
  }
  IdtDescriptor.Base  = (UINTN) &IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(IdtSize - 1);

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Initialize the global FSP data region
  //
  FspGlobalDataInit (&PeiFspData, BootLoaderStack, (UINT8)ApiIdx);

  //
  // Update the base address and length of Pei temporary memory
  //
  SecCoreData.DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFirmwareVolume;
  SecCoreData.BootFirmwareVolumeSize = (UINT32)((EFI_FIRMWARE_VOLUME_HEADER *)BootFirmwareVolume)->FvLength;

  //
  // Support FSP reserved temporary memory from the whole temporary memory provided by bootloader.
  // FSP reserved temporary memory will not be given to PeiCore.
  //
  SecCoreData.TemporaryRamBase       = (UINT8 *)(UINTN) TempRamBase  + PcdGet32 (PcdFspPrivateTemporaryRamSize);
  SecCoreData.TemporaryRamSize       = SizeOfRam - PcdGet32 (PcdFspPrivateTemporaryRamSize);
  if (PcdGet8 (PcdFspHeapSizePercentage) == 0) {
    SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
    SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize;
    SecCoreData.StackBase              = (VOID *)GetFspEntryStack(); // Share the same boot loader stack
    SecCoreData.StackSize              = 0;
  } else {
    SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
    SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize * PcdGet8 (PcdFspHeapSizePercentage) / 100;
    SecCoreData.StackBase              = (VOID*)(UINTN)((UINTN)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize);
    SecCoreData.StackSize              = SecCoreData.TemporaryRamSize - SecCoreData.PeiTemporaryRamSize;
  }

  DEBUG ((DEBUG_INFO, "Fsp BootFirmwareVolumeBase - 0x%x\n", SecCoreData.BootFirmwareVolumeBase));
  DEBUG ((DEBUG_INFO, "Fsp BootFirmwareVolumeSize - 0x%x\n", SecCoreData.BootFirmwareVolumeSize));
  DEBUG ((DEBUG_INFO, "Fsp TemporaryRamBase       - 0x%x\n", SecCoreData.TemporaryRamBase));
  DEBUG ((DEBUG_INFO, "Fsp TemporaryRamSize       - 0x%x\n", SecCoreData.TemporaryRamSize));
  DEBUG ((DEBUG_INFO, "Fsp PeiTemporaryRamBase    - 0x%x\n", SecCoreData.PeiTemporaryRamBase));
  DEBUG ((DEBUG_INFO, "Fsp PeiTemporaryRamSize    - 0x%x\n", SecCoreData.PeiTemporaryRamSize));
  DEBUG ((DEBUG_INFO, "Fsp StackBase              - 0x%x\n", SecCoreData.StackBase));
  DEBUG ((DEBUG_INFO, "Fsp StackSize              - 0x%x\n", SecCoreData.StackSize));

  //
  // Call PeiCore Entry
  //
  PeiCore (&SecCoreData, mPeiSecPlatformInformationPpi);

  //
  // Should never be here
  //
  CpuDeadLoop ();
}

/**
  This service of the TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param[in] PeiServices            Pointer to the PEI Services Table.
  @param[in] TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param[in] PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param[in] CopySize               Amount of memory to migrate from temporary to permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize > TemporaryMemoryBase when
                                TemporaryMemoryBase > PermanentMemoryBase.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  IA32_DESCRIPTOR   IdtDescriptor;
  VOID*             OldHeap;
  VOID*             NewHeap;
  VOID*             OldStack;
  VOID*             NewStack;
  UINTN             HeapSize;
  UINTN             StackSize;

  UINTN             CurrentStack;
  UINTN             FspStackBase;

  //
  // Override OnSeparateStack to 1 because this function will switch stack to permanent memory
  // which makes FSP running on different stack from bootloader temporary ram stack.
  //
  GetFspGlobalDataPointer ()->OnSeparateStack = 1;

  if (PcdGet8 (PcdFspHeapSizePercentage) == 0) {

    CurrentStack = AsmReadEsp();
    FspStackBase = (UINTN)GetFspEntryStack();

    StackSize = FspStackBase - CurrentStack;
    HeapSize  = CopySize;

    OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
    NewHeap = (VOID*)((UINTN)PermanentMemoryBase);

    OldStack = (VOID*)CurrentStack;
    //
    //The old stack is copied at the end of the stack region because stack grows down.
    //
    NewStack = (VOID*)((UINTN)PermanentMemoryBase - StackSize);

  } else {
    HeapSize   = CopySize * PcdGet8 (PcdFspHeapSizePercentage) / 100 ;
    StackSize  = CopySize - HeapSize;

    OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
    NewHeap = (VOID*)((UINTN)PermanentMemoryBase + StackSize);

    OldStack = (VOID*)((UINTN)TemporaryMemoryBase + HeapSize);
    NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  }
  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, HeapSize);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, StackSize);


  //
  // We need *not* fix the return address because currently,
  // The PeiCore is executed in flash.
  //

  //
  // Rebase IDT table in permanent memory
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtDescriptor.Base = IdtDescriptor.Base - (UINTN)OldStack + (UINTN)NewStack;

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Fixed the FSP data pointer
  //
  FspDataPointerFixUp ((UINTN)NewStack - (UINTN)OldStack);

  //
  // SecSwitchStack function must be invoked after the memory migration
  // immediately, also we need fixup the stack change caused by new call into
  // permanent memory.
  //
  SecSwitchStack (
    (UINT32) (UINTN) OldStack,
    (UINT32) (UINTN) NewStack
    );

  return EFI_SUCCESS;
}
