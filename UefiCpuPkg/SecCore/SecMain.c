/** @file
  C functions in SEC

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"

EFI_PEI_TEMPORARY_RAM_DONE_PPI gSecTemporaryRamDonePpi = {
  SecTemporaryRamDone
};

EFI_SEC_PLATFORM_INFORMATION_PPI  mSecPlatformInformationPpi = { SecPlatformInformation };

EFI_PEI_PPI_DESCRIPTOR            mPeiSecPlatformInformationPpi[] = {
  {
    //
    // SecPerformance PPI notify descriptor.
    //
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gPeiSecPerformancePpiGuid,
    (VOID *) (UINTN) SecPerformancePpiCallBack
  },
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamDonePpiGuid,
    &gSecTemporaryRamDonePpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiSecPlatformInformationPpiGuid,
    &mSecPlatformInformationPpi
  }
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
NORETURN
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  );

/**
  Entry point of the notification callback function itself within the PEIM.
  It is to get SEC performance data and build HOB to convey the SEC performance
  data to DXE phase.

  @param  PeiServices      Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor Address of the notification descriptor data structure.
  @param  Ppi              Address of the PPI that was installed.

  @return Status of the notification.
          The status code returned from this function is ignored.
**/
EFI_STATUS
EFIAPI
SecPerformancePpiCallBack (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                    Status;
  PEI_SEC_PERFORMANCE_PPI       *SecPerf;
  FIRMWARE_SEC_PERFORMANCE      Performance;

  SecPerf = (PEI_SEC_PERFORMANCE_PPI *) Ppi;
  Status = SecPerf->GetPerformance ((CONST EFI_PEI_SERVICES **) PeiServices, SecPerf, &Performance);
  if (!EFI_ERROR (Status)) {
    BuildGuidDataHob (
      &gEfiFirmwarePerformanceGuid,
      &Performance,
      sizeof (FIRMWARE_SEC_PERFORMANCE)
    );
    DEBUG ((DEBUG_INFO, "FPDT: SEC Performance Hob ResetEnd = %ld\n", Performance.ResetEnd));
  }

  return Status;
}

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of temporary ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.
**/
VOID
NORETURN
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
  EFI_STATUS                  Status;

  //
  // Report Status Code to indicate entering SEC core
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_SEC | EFI_SW_SEC_PC_ENTRY_POINT
    );

  PeiStackSize = PcdGet32 (PcdPeiTemporaryRamStackSize);
  if (PeiStackSize == 0) {
    PeiStackSize = (SizeOfRam >> 1);
  }

  ASSERT (PeiStackSize < SizeOfRam);

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
  // |IDT Table          |
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
  // Setup the default exception handlers
  //
  Status = InitializeCpuExceptionHandlers (NULL);
  ASSERT_EFI_ERROR (Status);

  //
  // Update the base address and length of Pei temporary memory
  //
  SecCoreData.DataSize               = (UINT16) sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFirmwareVolume;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)((EFI_FIRMWARE_VOLUME_HEADER *) BootFirmwareVolume)->FvLength;
  SecCoreData.TemporaryRamBase       = (VOID*)(UINTN) TempRamBase;
  SecCoreData.TemporaryRamSize       = SizeOfRam;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SizeOfRam - PeiStackSize;
  SecCoreData.StackBase              = (VOID*)(UINTN)(TempRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = PeiStackSize;

  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2);

  //
  // Should not come here.
  //
  UNREACHABLE ();
}

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
NORETURN
EFIAPI
SecStartupPhase2(
  IN VOID                     *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_PEI_PPI_DESCRIPTOR      *PpiList;
  UINT32                      Index;
  EFI_PEI_PPI_DESCRIPTOR      *AllSecPpiList;
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;

  PeiCoreEntryPoint = NULL;
  SecCoreData   = (EFI_SEC_PEI_HAND_OFF *) Context;
  AllSecPpiList = (EFI_PEI_PPI_DESCRIPTOR *) SecCoreData->PeiTemporaryRamBase;

  //
  // Perform platform specific initialization before entering PeiCore.
  //
  PpiList = SecPlatformMain (SecCoreData);
  //
  // Find Pei Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  if (PpiList != NULL) {
    for (Index = 0;
      (PpiList[Index].Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) != EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
      Index++) {
      if (CompareGuid (PpiList[Index].Guid, &gEfiPeiCoreFvLocationPpiGuid) &&
          (((EFI_PEI_CORE_FV_LOCATION_PPI *) PpiList[Index].Ppi)->PeiCoreFvLocation != 0)
         ) {
        //
        // In this case, SecCore is in BFV but PeiCore is in another FV reported by PPI.
        //
        FindAndReportEntryPoints (
          (EFI_FIRMWARE_VOLUME_HEADER *) SecCoreData->BootFirmwareVolumeBase,
          (EFI_FIRMWARE_VOLUME_HEADER *) ((EFI_PEI_CORE_FV_LOCATION_PPI *) PpiList[Index].Ppi)->PeiCoreFvLocation,
          &PeiCoreEntryPoint
          );
        if (PeiCoreEntryPoint != NULL) {
          break;
        } else {
          //
          // PeiCore not found
          //
          CpuDeadLoop ();
        }
      }
    }
  }
  //
  // If EFI_PEI_CORE_FV_LOCATION_PPI not found, try to locate PeiCore from BFV.
  //
  if (PeiCoreEntryPoint == NULL) {
    //
    // Both SecCore and PeiCore are in BFV.
    //
    FindAndReportEntryPoints (
      (EFI_FIRMWARE_VOLUME_HEADER *) SecCoreData->BootFirmwareVolumeBase,
      (EFI_FIRMWARE_VOLUME_HEADER *) SecCoreData->BootFirmwareVolumeBase,
      &PeiCoreEntryPoint
      );
    if (PeiCoreEntryPoint == NULL) {
      CpuDeadLoop ();
    }
  }

  if (PpiList != NULL) {
    //
    // Remove the terminal flag from the terminal PPI
    //
    CopyMem (AllSecPpiList, mPeiSecPlatformInformationPpi, sizeof (mPeiSecPlatformInformationPpi));
    Index = sizeof (mPeiSecPlatformInformationPpi) / sizeof (EFI_PEI_PPI_DESCRIPTOR) - 1;
    AllSecPpiList[Index].Flags = AllSecPpiList[Index].Flags & (~EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);

    //
    // Append the platform additional PPI list
    //
    Index += 1;
    while (((PpiList->Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) != EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST)) {
      CopyMem (&AllSecPpiList[Index], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));
      Index++;
      PpiList++;
    }

    //
    // Add the terminal PPI
    //
    CopyMem (&AllSecPpiList[Index ++], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));

    //
    // Set PpiList to the total PPI
    //
    PpiList = AllSecPpiList;

    //
    // Adjust PEI TEMP RAM Range.
    //
    ASSERT (SecCoreData->PeiTemporaryRamSize > Index * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    SecCoreData->PeiTemporaryRamBase = (VOID *)((UINTN) SecCoreData->PeiTemporaryRamBase + Index * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    SecCoreData->PeiTemporaryRamSize = SecCoreData->PeiTemporaryRamSize - Index * sizeof (EFI_PEI_PPI_DESCRIPTOR);
    //
    // Adjust the Base and Size to be 8-byte aligned as HOB which has 8byte aligned requirement
    // will be built based on them in PEI phase.
    //
    SecCoreData->PeiTemporaryRamBase = (VOID *)(((UINTN)SecCoreData->PeiTemporaryRamBase + 7) & ~0x07);
    SecCoreData->PeiTemporaryRamSize &= ~(UINTN)0x07;
  } else {
    //
    // No addition PPI, PpiList directly point to the common PPI list.
    //
    PpiList = &mPeiSecPlatformInformationPpi[0];
  }

  DEBUG ((
    DEBUG_INFO,
    "%a() Stack Base: 0x%p, Stack Size: 0x%x\n",
    __FUNCTION__,
    SecCoreData->StackBase,
    (UINT32) SecCoreData->StackSize
    ));

  //
  // Report Status Code to indicate transferring to PEI core
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_SEC | EFI_SW_SEC_PC_HANDOFF_TO_NEXT
    );

  //
  // Transfer the control to the PEI core
  //
  ASSERT (PeiCoreEntryPoint != NULL);
  (*PeiCoreEntryPoint) (SecCoreData, PpiList);

  //
  // Should not come here.
  //
  UNREACHABLE ();
}

/**
  TemporaryRamDone() disables the use of Temporary RAM. If present, this service is invoked
  by the PEI Foundation after the EFI_PEI_PERMANANT_MEMORY_INSTALLED_PPI is installed.

  @retval EFI_SUCCESS           Use of Temporary RAM was disabled.
  @retval EFI_INVALID_PARAMETER Temporary RAM could not be disabled.

**/
EFI_STATUS
EFIAPI
SecTemporaryRamDone (
  VOID
  )
{
  BOOLEAN  State;

  //
  // Republish Sec Platform Information(2) PPI
  //
  RepublishSecPlatformInformationPpi ();

  //
  // Migrate DebugAgentContext.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);

  //
  // Disable interrupts and save current interrupt state
  //
  State = SaveAndDisableInterrupts();

  //
  // Disable Temporary RAM after Stack and Heap have been migrated at this point.
  //
  SecPlatformDisableTemporaryMemory ();

  //
  // Restore original interrupt state
  //
  SetInterruptState (State);

  return EFI_SUCCESS;
}
