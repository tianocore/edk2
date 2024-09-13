/** @file
  Generic SEC driver for ARM platforms

  Copyright (c) 2011 - 2022, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Sec.h"

/**
  This service of the EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI that migrates temporary
  RAM into permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.
  @param TemporaryMemoryBase    Source Address in temporary memory from which
                                the SEC or PEIM will copy the Temporary RAM
                                contents.
  @param PermanentMemoryBase    Destination Address in permanent memory into
                                which the SEC or PEIM will copy the Temporary
                                RAM contents.
  @param CopySize               Amount of memory to migrate from temporary to
                                permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize >
                                TemporaryMemoryBase when TemporaryMemoryBase >
                                PermanentMemoryBase.

**/
STATIC
EFI_STATUS
EFIAPI
SecTemporaryRamSupport (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  )
{
  VOID   *OldHeap;
  VOID   *NewHeap;
  VOID   *OldStack;
  VOID   *NewStack;
  UINTN  HeapSize;

  HeapSize = ALIGN_VALUE (CopySize / 2, CPU_STACK_ALIGNMENT);

  OldHeap = (VOID *)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID *)((UINTN)PermanentMemoryBase + (CopySize - HeapSize));

  OldStack = (VOID *)((UINTN)TemporaryMemoryBase + HeapSize);
  NewStack = (VOID *)(UINTN)PermanentMemoryBase;

  //
  // Migrate the temporary memory stack to permanent memory stack.
  //
  CopyMem (NewStack, OldStack, CopySize - HeapSize);

  //
  // Migrate the temporary memory heap to permanent memory heap.
  //
  CopyMem (NewHeap, OldHeap, HeapSize);

  SecSwitchStack ((UINTN)NewStack - (UINTN)OldStack);

  return EFI_SUCCESS;
}

STATIC CONST EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI  mTemporaryRamSupportPpi = {
  SecTemporaryRamSupport
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  gCommonPpiTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    (VOID *)&mTemporaryRamSupportPpi
  }
};

/**
  Construct a PPI list from the PPIs provided in this file and the ones
  provided by the platform code.

  @param[out]   PpiListSize   Size of the PPI list in bytes
  @param[out]   PpiList       Pointer to the constructed PPI list
**/
STATIC
VOID
CreatePpiList (
  OUT UINTN                   *PpiListSize,
  OUT EFI_PEI_PPI_DESCRIPTOR  **PpiList
  )
{
  EFI_PEI_PPI_DESCRIPTOR  *PlatformPpiList;
  UINTN                   PlatformPpiListSize;
  UINTN                   ListBase;
  EFI_PEI_PPI_DESCRIPTOR  *LastPpi;

  // Get the Platform PPIs
  PlatformPpiListSize = 0;
  ArmPlatformGetPlatformPpiList (&PlatformPpiListSize, &PlatformPpiList);

  // Copy the Common and Platform PPis in Temporary Memory
  ListBase = PcdGet64 (PcdCPUCoresStackBase);
  CopyMem ((VOID *)ListBase, gCommonPpiTable, sizeof (gCommonPpiTable));
  CopyMem ((VOID *)(ListBase + sizeof (gCommonPpiTable)), PlatformPpiList, PlatformPpiListSize);

  // Set the Terminate flag on the last PPI entry
  LastPpi = (EFI_PEI_PPI_DESCRIPTOR *)ListBase +
            ((sizeof (gCommonPpiTable) + PlatformPpiListSize) / sizeof (EFI_PEI_PPI_DESCRIPTOR)) - 1;
  LastPpi->Flags |= EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;

  *PpiList     = (EFI_PEI_PPI_DESCRIPTOR *)ListBase;
  *PpiListSize = sizeof (gCommonPpiTable) + PlatformPpiListSize;
}

/**

 Prints firmware version and build time to serial console.

**/
STATIC
VOID
PrintFirmwareVersion (
  VOID
  )
{
  CHAR8  Buffer[100];
  UINTN  CharCount;

  CharCount = AsciiSPrint (
                Buffer,
                sizeof (Buffer),
                "UEFI firmware (version %s built at %a on %a)\n\r",
                (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString),
                __TIME__,
                __DATE__
                );

  // Because we are directly bit banging the serial port instead of going through the DebugLib, we need to make sure
  // the serial port is initialized before we write to it
  SerialPortInitialize ();
  SerialPortWrite ((UINT8 *)Buffer, CharCount);
}

/**
  SEC main routine.

  @param[in]  PeiCoreEntryPoint   Address in ram of the entrypoint of the PEI
                                  core
**/
STATIC
VOID
EFIAPI
SecMain (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  EFI_SEC_PEI_HAND_OFF    SecCoreData;
  UINTN                   PpiListSize;
  EFI_PEI_PPI_DESCRIPTOR  *PpiList;
  UINTN                   TemporaryRamBase;
  UINTN                   TemporaryRamSize;

  CreatePpiList (&PpiListSize, &PpiList);

  // Adjust the Temporary Ram as the new Ppi List (Common + Platform Ppi Lists) is created at
  // the base of the primary core stack
  PpiListSize      = ALIGN_VALUE (PpiListSize, CPU_STACK_ALIGNMENT);
  TemporaryRamBase = (UINTN)PcdGet64 (PcdCPUCoresStackBase) + PpiListSize;
  TemporaryRamSize = (UINTN)PcdGet32 (PcdCPUCorePrimaryStackSize) - PpiListSize;

  //
  // Bind this information into the SEC hand-off state
  // Note: this must be in sync with the stuff in the asm file
  // Note also:  HOBs (pei temp ram) MUST be above stack
  //
  SecCoreData.DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = (VOID *)(UINTN)PcdGet64 (PcdFvBaseAddress);
  SecCoreData.BootFirmwareVolumeSize = PcdGet32 (PcdFvSize);
  SecCoreData.TemporaryRamBase       = (VOID *)TemporaryRamBase; // We run on the primary core (and so we use the first stack)
  SecCoreData.TemporaryRamSize       = TemporaryRamSize;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = ALIGN_VALUE (SecCoreData.TemporaryRamSize / 2, CPU_STACK_ALIGNMENT);
  SecCoreData.StackBase              = (VOID *)((UINTN)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = (TemporaryRamBase + TemporaryRamSize) - (UINTN)SecCoreData.StackBase;

  // Jump to PEI core entry point
  (PeiCoreEntryPoint)(&SecCoreData, PpiList);
}

/**
  Module C entrypoint.

  @param[in]  PeiCoreEntryPoint   Address in ram of the entrypoint of the PEI
                                  core
**/
VOID
CEntryPoint (
  IN  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint
  )
{
  if (!ArmMmuEnabled ()) {
    // Data Cache enabled on Primary core when MMU is enabled.
    ArmDisableDataCache ();
    // Invalidate instruction cache
    ArmInvalidateInstructionCache ();
    // Enable Instruction Caches on all cores.
    ArmEnableInstructionCache ();

    InvalidateDataCacheRange (
      (VOID *)(UINTN)PcdGet64 (PcdCPUCoresStackBase),
      PcdGet32 (PcdCPUCorePrimaryStackSize)
      );
  }

  // Write VBAR - The Exception Vector table must be aligned to its requirement
  // Note: The AArch64 Vector table must be 2k-byte aligned - if this assertion fails ensure
  // 'Align=4K' is defined into your FDF for this module.
  ASSERT (((UINTN)PeiVectorTable & ARM_VECTOR_TABLE_ALIGNMENT) == 0);
  ArmWriteVBar ((UINTN)PeiVectorTable);

  // Enable Floating Point
  if (FixedPcdGet32 (PcdVFPEnabled)) {
    ArmEnableVFP ();
  }

  // Invoke "ProcessLibraryConstructorList" to have all library constructors
  // called.
  ProcessLibraryConstructorList ();

  PrintFirmwareVersion ();

  // Initialize the Debug Agent for Source Level Debugging
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);

  // Initialize the platform specific controllers
  ArmPlatformInitialize (ArmReadMpidr ());

  // Goto primary Main.
  SecMain (PeiCoreEntryPoint);

  // PEI Core should always load and never return
  ASSERT (FALSE);
}
