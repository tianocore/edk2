/** @file

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecMain.h"
#include "SecFsp.h"

EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI gSecTemporaryRamSupportPpi = {
  SecTemporaryRamSupport
};

EFI_PEI_PPI_DESCRIPTOR            mPeiSecPlatformInformationPpi[] = {
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
  @param[in] TempRamBase        Base address of tempory ram
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

  //
  // Process all libraries constructor function linked to SecCore.
  //
  ProcessLibraryConstructorList ();

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();


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
  // |      Heap         |    PeiTemporayRamSize
  // |                   |
  // |                   |
  // |-------------------|---->  TempRamBase
  IdtTableInStack.PeiService  = NULL;
  ExceptionHandler = FspGetExceptionHandler(mIdtEntryTemplate);
  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index ++) {
    CopyMem ((VOID*)&IdtTableInStack.IdtTable[Index], (VOID*)&ExceptionHandler, sizeof (UINT64));
  }

  IdtDescriptor.Base  = (UINTN) &IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

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
  SecCoreData.TemporaryRamBase       = (VOID*)(UINTN) TempRamBase;
  SecCoreData.TemporaryRamSize       = SizeOfRam;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SizeOfRam >> 1;
  SecCoreData.StackBase              = (VOID*)(UINTN)(TempRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = SizeOfRam >> 1;

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

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + CopySize / 2);

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + CopySize / 2);
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize / 2);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, CopySize / 2);


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
  // immediatly, also we need fixup the stack change caused by new call into
  // permenent memory.
  //
  SecSwitchStack (
    (UINT32) (UINTN) OldStack,
    (UINT32) (UINTN) NewStack
    );

  return EFI_SUCCESS;
}
