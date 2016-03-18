/** @file
  C functions in SEC

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "SecMain.h"

EFI_PEI_PPI_DESCRIPTOR            mPeiSecMainPpi[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gTopOfTemporaryRamPpiGuid,
    NULL // To be patched later.
  },
};

//
// These are IDT entries pointing to 10:FFFFFFE4h.
//
UINT64  mIdtEntryTemplate = 0xffff8e000010ffe4ULL;

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  );


/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] SizeOfRam           Size of the temporary memory available for use.
  @param[in] TempRamBase         Base address of tempory ram
  @param[in] BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
EFIAPI
SecStartup (
  IN UINT32                   SizeOfRam,
  IN UINT32                   TempRamBase,
  IN VOID                     *BootFirmwareVolume
  )
{
  EFI_SEC_PEI_HAND_OFF        SecCoreData;
  IA32_DESCRIPTOR             IdtDescriptor;
  SEC_IDT_TABLE               IdtTableInStack;
  UINT32                      Index;
  UINT32                      PeiStackSize;

  PeiStackSize = PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (PeiStackSize == 0) {
    PeiStackSize = (SizeOfRam >> 1);
  }

  ASSERT (PeiStackSize < SizeOfRam);

  //
  // Process all libraries constructor function linked to SecCore.
  //
  ProcessLibraryConstructorList ();

  DEBUG ((DEBUG_INFO, "SecCore - SecStartup\n"));

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

  IdtTableInStack.PeiService = 0;
  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index ++) {
    CopyMem ((VOID*)&IdtTableInStack.IdtTable[Index], (VOID*)&mIdtEntryTemplate, sizeof (UINT64));
  }

  IdtDescriptor.Base  = (UINTN) &IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Update the base address and length of Pei temporary memory
  //
  SecCoreData.DataSize               = (UINT16) sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFirmwareVolume;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)(SIZE_4GB - (UINTN) BootFirmwareVolume);
  SecCoreData.TemporaryRamBase       = (VOID*)(UINTN) TempRamBase;
  SecCoreData.TemporaryRamSize       = SizeOfRam;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SizeOfRam - PeiStackSize;
  SecCoreData.StackBase              = (VOID*)(UINTN)(TempRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = PeiStackSize;

  DEBUG ((DEBUG_INFO, "BootFirmwareVolumeBase - 0x%x\n", SecCoreData.BootFirmwareVolumeBase));
  DEBUG ((DEBUG_INFO, "BootFirmwareVolumeSize - 0x%x\n", SecCoreData.BootFirmwareVolumeSize));
  DEBUG ((DEBUG_INFO, "TemporaryRamBase       - 0x%x\n", SecCoreData.TemporaryRamBase));
  DEBUG ((DEBUG_INFO, "TemporaryRamSize       - 0x%x\n", SecCoreData.TemporaryRamSize));
  DEBUG ((DEBUG_INFO, "PeiTemporaryRamBase    - 0x%x\n", SecCoreData.PeiTemporaryRamBase));
  DEBUG ((DEBUG_INFO, "PeiTemporaryRamSize    - 0x%x\n", SecCoreData.PeiTemporaryRamSize));
  DEBUG ((DEBUG_INFO, "StackBase              - 0x%x\n", SecCoreData.StackBase));
  DEBUG ((DEBUG_INFO, "StackSize              - 0x%x\n", SecCoreData.StackSize));

  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2);

}

/**
  This API patch the TopOfTemporaryRam value in SecPpiList.

  @param[in,out] SecPpiList           PPI list to be patched.
  @param[in]     TopOfTemporaryRam    The top of CAR.

**/
VOID
PatchTopOfTemporaryRamPpi (
  IN OUT EFI_PEI_PPI_DESCRIPTOR *SecPpiList,
  IN VOID                       *TopOfTemporaryRam
  )
{
  SecPpiList[0].Ppi = TopOfTemporaryRam;
}

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_PEI_PPI_DESCRIPTOR      *PpiList;
  UINT32                      Index;
  EFI_PEI_PPI_DESCRIPTOR      LocalSecPpiList[sizeof(mPeiSecMainPpi)/sizeof(mPeiSecMainPpi[0])];
  EFI_PEI_PPI_DESCRIPTOR      AllSecPpiList[FixedPcdGet32(PcdSecCoreMaxPpiSupported)];
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;

  SecCoreData = (EFI_SEC_PEI_HAND_OFF *) Context;
  //
  // Find Pei Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  FindAndReportEntryPoints ((EFI_FIRMWARE_VOLUME_HEADER *) SecCoreData->BootFirmwareVolumeBase, &PeiCoreEntryPoint);
  if (PeiCoreEntryPoint == NULL)
  {
    CpuDeadLoop ();
  }

  CopyMem (LocalSecPpiList, mPeiSecMainPpi, sizeof(mPeiSecMainPpi));
  PatchTopOfTemporaryRamPpi (LocalSecPpiList, (VOID *)((UINTN)SecCoreData->TemporaryRamBase + SecCoreData->TemporaryRamSize));

  //
  // Perform platform specific initialization before entering PeiCore.
  //
  PpiList = SecPlatformMain (SecCoreData);
  if (PpiList != NULL) {
    //
    // Remove the terminal flag from the terminal Ppi
    //
    CopyMem (AllSecPpiList, LocalSecPpiList, sizeof (LocalSecPpiList));
    for (Index = 0; Index < PcdGet32 (PcdSecCoreMaxPpiSupported); Index ++) {
      if ((AllSecPpiList[Index].Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) == EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) {
        break;
      }
    }
    AllSecPpiList[Index].Flags = AllSecPpiList[Index].Flags & (~EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);

    //
    // Append the platform additional Ppi list
    //
    Index += 1;
    while (Index < PcdGet32 (PcdSecCoreMaxPpiSupported) &&
           ((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) != EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST)) {
      CopyMem (&AllSecPpiList[Index], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));
      Index++;
      PpiList++;
    }

    //
    // Check whether the total Ppis exceeds the max supported Ppi.
    //
    if (Index >= PcdGet32 (PcdSecCoreMaxPpiSupported)) {
      //
      // the total Ppi is larger than the supported Max
      // PcdSecCoreMaxPpiSupported can be enlarged to solve it.
      //
      CpuDeadLoop ();
    } else {
      //
      // Add the terminal Ppi
      //
      CopyMem (&AllSecPpiList[Index], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));
    }

    //
    // Set PpiList to the total Ppi
    //
    PpiList = &AllSecPpiList[0];
  } else {
    //
    // No addition Ppi, PpiList directly point to the common Ppi list.
    //
    PpiList = &LocalSecPpiList[0];
  }

  //
  // Transfer the control to the PEI core
  //
  ASSERT (PeiCoreEntryPoint != NULL);
  (*PeiCoreEntryPoint) (SecCoreData, PpiList);

  //
  // Should not come here.
  //
  return ;
}
