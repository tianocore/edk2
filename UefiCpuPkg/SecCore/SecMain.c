/** @file
  C functions in SEC

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"

EFI_PEI_TEMPORARY_RAM_DONE_PPI  gSecTemporaryRamDonePpi = {
  SecTemporaryRamDone
};

EFI_SEC_PLATFORM_INFORMATION_PPI  mSecPlatformInformationPpi = { SecPlatformInformation };

EFI_PEI_PPI_DESCRIPTOR  mPeiSecPlatformInformationPpi[] = {
  {
    //
    // SecPerformance PPI notify descriptor.
    //
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gPeiSecPerformancePpiGuid,
    (VOID *)(UINTN)SecPerformancePpiCallBack
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

/**
  Migrates the Global Descriptor Table (GDT) to permanent memory.

  @retval   EFI_SUCCESS           The GDT was migrated successfully.
  @retval   EFI_OUT_OF_RESOURCES  The GDT could not be migrated due to lack of available memory.

**/
EFI_STATUS
MigrateGdt (
  VOID
  )
{
  EFI_STATUS       Status;
  UINTN            GdtBufferSize;
  IA32_DESCRIPTOR  Gdtr;
  VOID             *GdtBuffer;

  AsmReadGdtr ((IA32_DESCRIPTOR *)&Gdtr);
  GdtBufferSize = sizeof (IA32_SEGMENT_DESCRIPTOR) -1 + Gdtr.Limit + 1;

  Status =  PeiServicesAllocatePool (
              GdtBufferSize,
              &GdtBuffer
              );
  ASSERT (GdtBuffer != NULL);
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  GdtBuffer = ALIGN_POINTER (GdtBuffer, sizeof (IA32_SEGMENT_DESCRIPTOR));
  CopyMem (GdtBuffer, (VOID *)Gdtr.Base, Gdtr.Limit + 1);
  Gdtr.Base = (UINTN)GdtBuffer;
  AsmWriteGdtr (&Gdtr);

  return EFI_SUCCESS;
}

/**
  Migrate page table to permanent memory mapping entire physical address space.

  @retval   EFI_SUCCESS           The PageTable was migrated successfully.
  @retval   EFI_UNSUPPORTED       Unsupport to migrate page table to permanent memory if IA-32e Mode not actived.
  @retval   EFI_OUT_OF_RESOURCES  The PageTable could not be migrated due to lack of available memory.

**/
EFI_STATUS
MigratePageTable (
  VOID
  )
{
  EFI_STATUS                      Status;
  IA32_CR4                        Cr4;
  BOOLEAN                         Page5LevelSupport;
  UINT32                          RegEax;
  CPUID_EXTENDED_CPU_SIG_EDX      RegEdx;
  BOOLEAN                         Page1GSupport;
  PAGING_MODE                     PagingMode;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;
  UINT32                          MaxExtendedFunctionId;
  UINTN                           PageTable;
  EFI_PHYSICAL_ADDRESS            Buffer;
  UINTN                           BufferSize;
  IA32_MAP_ATTRIBUTE              MapAttribute;
  IA32_MAP_ATTRIBUTE              MapMask;

  VirPhyAddressSize.Uint32    = 0;
  PageTable                   = 0;
  BufferSize                  = 0;
  MapAttribute.Uint64         = 0;
  MapMask.Uint64              = MAX_UINT64;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;

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

  //
  // Get Maximum Physical Address Bits
  // Get the number of address lines; Maximum Physical Address is 2^PhysicalAddressBits - 1.
  // If CPUID does not supported, then use a max value of 36 as per SDM 3A, 4.1.4.
  //
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunctionId, NULL, NULL, NULL);
  if (MaxExtendedFunctionId >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  if ((PagingMode == Paging4Level1GB) || (PagingMode == Paging4Level)) {
    //
    // The max lineaddress bits is 48 for 4 level page table.
    //
    VirPhyAddressSize.Bits.PhysicalAddressBits = MIN (VirPhyAddressSize.Bits.PhysicalAddressBits, 48);
  }

  //
  // Get required buffer size for the pagetable that will be created.
  //
  Status = PageTableMap (&PageTable, PagingMode, 0, &BufferSize, 0, LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits), &MapAttribute, &MapMask, NULL);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  //
  // Allocate required Buffer.
  //
  Status = PeiServicesAllocatePages (
             EfiBootServicesData,
             EFI_SIZE_TO_PAGES (BufferSize),
             &Buffer
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create PageTable in permanent memory.
  //
  Status = PageTableMap (&PageTable, PagingMode, (VOID *)(UINTN)Buffer, &BufferSize, 0, LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits), &MapAttribute, &MapMask, NULL);
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status) || (PageTable == 0)) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Write the Pagetable to CR3.
  //
  AsmWriteCr3 (PageTable);

  DEBUG ((
    DEBUG_INFO,
    "MigratePageTable: Created PageTable = 0x%lx, BufferSize = %x, PagingMode = 0x%lx, Support Max Physical Address Bits = %d\n",
    PageTable,
    BufferSize,
    (UINTN)PagingMode,
    VirPhyAddressSize.Bits.PhysicalAddressBits
    ));

  return Status;
}

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
SecStartupPhase2 (
  IN VOID  *Context
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
  EFI_STATUS                Status;
  PEI_SEC_PERFORMANCE_PPI   *SecPerf;
  FIRMWARE_SEC_PERFORMANCE  Performance;

  SecPerf = (PEI_SEC_PERFORMANCE_PPI *)Ppi;
  Status  = SecPerf->GetPerformance ((CONST EFI_PEI_SERVICES **)PeiServices, SecPerf, &Performance);
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
  IN UINT32  SizeOfRam,
  IN UINT32  TempRamBase,
  IN VOID    *BootFirmwareVolume
  )
{
  EFI_SEC_PEI_HAND_OFF  SecCoreData;
  IA32_DESCRIPTOR       IdtDescriptor;
  SEC_IDT_TABLE         IdtTableInStack;
  UINT32                Index;
  UINT32                PeiStackSize;
  EFI_STATUS            Status;

  //
  // Report Status Code to indicate entering SEC core
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    EFI_SOFTWARE_SEC | EFI_SW_SEC_PC_ENTRY_POINT
    );

  DEBUG ((
    DEBUG_INFO,
    "%a() TempRAM Base: 0x%x, TempRAM Size: 0x%x, BootFirmwareVolume 0x%x\n",
    __func__,
    TempRamBase,
    SizeOfRam,
    BootFirmwareVolume
    ));

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
  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index++) {
    ZeroMem ((VOID *)&IdtTableInStack.IdtTable[Index], sizeof (IA32_IDT_GATE_DESCRIPTOR));
    CopyMem ((VOID *)&IdtTableInStack.IdtTable[Index], (VOID *)&mIdtEntryTemplate, sizeof (UINT64));
  }

  IdtDescriptor.Base  = (UINTN)&IdtTableInStack.IdtTable;
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
  SecCoreData.DataSize               = (UINT16)sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFirmwareVolume;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)((EFI_FIRMWARE_VOLUME_HEADER *)BootFirmwareVolume)->FvLength;
  SecCoreData.TemporaryRamBase       = (VOID *)(UINTN)TempRamBase;
  SecCoreData.TemporaryRamSize       = SizeOfRam;
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SizeOfRam - PeiStackSize;
  SecCoreData.StackBase              = (VOID *)(UINTN)(TempRamBase + SecCoreData.PeiTemporaryRamSize);
  SecCoreData.StackSize              = PeiStackSize;

  DEBUG ((
    DEBUG_INFO,
    "%a() BFV Base: 0x%x, BFV Size: 0x%x, TempRAM Base: 0x%x, TempRAM Size: 0x%x, PeiTempRamBase: 0x%x, PeiTempRamSize: 0x%x, StackBase: 0x%x, StackSize: 0x%x\n",
    __func__,
    SecCoreData.BootFirmwareVolumeBase,
    SecCoreData.BootFirmwareVolumeSize,
    SecCoreData.TemporaryRamBase,
    SecCoreData.TemporaryRamSize,
    SecCoreData.PeiTemporaryRamBase,
    SecCoreData.PeiTemporaryRamSize,
    SecCoreData.StackBase,
    SecCoreData.StackSize
    ));

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
SecStartupPhase2 (
  IN VOID  *Context
  )
{
  EFI_SEC_PEI_HAND_OFF      *SecCoreData;
  EFI_PEI_PPI_DESCRIPTOR    *PpiList;
  UINT32                    Index;
  EFI_PEI_PPI_DESCRIPTOR    *AllSecPpiList;
  EFI_PEI_CORE_ENTRY_POINT  PeiCoreEntryPoint;

  PeiCoreEntryPoint = NULL;
  SecCoreData       = (EFI_SEC_PEI_HAND_OFF *)Context;

  //
  // Perform platform specific initialization before entering PeiCore.
  //
  PpiList = SecPlatformMain (SecCoreData);
  //
  // Find Pei Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  if (PpiList != NULL) {
    Index = 0;
    do {
      if (CompareGuid (PpiList[Index].Guid, &gEfiPeiCoreFvLocationPpiGuid) &&
          (((EFI_PEI_CORE_FV_LOCATION_PPI *)PpiList[Index].Ppi)->PeiCoreFvLocation != 0)
          )
      {
        //
        // In this case, SecCore is in BFV but PeiCore is in another FV reported by PPI.
        //
        FindAndReportEntryPoints (
          (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase,
          (EFI_FIRMWARE_VOLUME_HEADER *)((EFI_PEI_CORE_FV_LOCATION_PPI *)PpiList[Index].Ppi)->PeiCoreFvLocation,
          &PeiCoreEntryPoint
          );
        if (PeiCoreEntryPoint != NULL) {
          break;
        } else {
          //
          // Invalid PeiCore FV provided by platform
          //
          CpuDeadLoop ();
        }
      }
    } while ((PpiList[Index++].Flags & EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST) != EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  }

  //
  // If EFI_PEI_CORE_FV_LOCATION_PPI not found, try to locate PeiCore from BFV.
  //
  if (PeiCoreEntryPoint == NULL) {
    //
    // Both SecCore and PeiCore are in BFV.
    //
    FindAndReportEntryPoints (
      (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase,
      (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase,
      &PeiCoreEntryPoint
      );
    if (PeiCoreEntryPoint == NULL) {
      CpuDeadLoop ();
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "%a() PeiCoreEntryPoint: 0x%x\n",
    __func__,
    PeiCoreEntryPoint
    ));

  if (PpiList != NULL) {
    AllSecPpiList = (EFI_PEI_PPI_DESCRIPTOR *)SecCoreData->PeiTemporaryRamBase;

    //
    // Remove the terminal flag from the terminal PPI
    //
    CopyMem (AllSecPpiList, mPeiSecPlatformInformationPpi, sizeof (mPeiSecPlatformInformationPpi));
    Index                      = sizeof (mPeiSecPlatformInformationPpi) / sizeof (EFI_PEI_PPI_DESCRIPTOR) - 1;
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
    CopyMem (&AllSecPpiList[Index++], PpiList, sizeof (EFI_PEI_PPI_DESCRIPTOR));

    //
    // Set PpiList to the total PPI
    //
    PpiList = AllSecPpiList;

    //
    // Adjust PEI TEMP RAM Range.
    //
    ASSERT (SecCoreData->PeiTemporaryRamSize > Index * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    SecCoreData->PeiTemporaryRamBase = (VOID *)((UINTN)SecCoreData->PeiTemporaryRamBase + Index * sizeof (EFI_PEI_PPI_DESCRIPTOR));
    SecCoreData->PeiTemporaryRamSize = SecCoreData->PeiTemporaryRamSize - Index * sizeof (EFI_PEI_PPI_DESCRIPTOR);
    //
    // Adjust the Base and Size to be 8-byte aligned as HOB which has 8byte aligned requirement
    // will be built based on them in PEI phase.
    //
    SecCoreData->PeiTemporaryRamBase  = (VOID *)(((UINTN)SecCoreData->PeiTemporaryRamBase + 7) & ~0x07);
    SecCoreData->PeiTemporaryRamSize &= ~(UINTN)0x07;
    DEBUG ((
      DEBUG_INFO,
      "%a() PeiTemporaryRamBase: 0x%x, PeiTemporaryRamSize: 0x%x\n",
      __func__,
      SecCoreData->PeiTemporaryRamBase,
      SecCoreData->PeiTemporaryRamSize
      ));
  } else {
    //
    // No addition PPI, PpiList directly point to the common PPI list.
    //
    PpiList = &mPeiSecPlatformInformationPpi[0];
  }

  DEBUG ((
    DEBUG_INFO,
    "%a() Stack Base: 0x%p, Stack Size: 0x%x\n",
    __func__,
    SecCoreData->StackBase,
    (UINT32)SecCoreData->StackSize
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
  (*PeiCoreEntryPoint)(SecCoreData, PpiList);

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
  EFI_STATUS              Status;
  EFI_STATUS              Status2;
  UINTN                   Index;
  BOOLEAN                 State;
  EFI_PEI_PPI_DESCRIPTOR  *PeiPpiDescriptor;
  REPUBLISH_SEC_PPI_PPI   *RepublishSecPpiPpi;
  IA32_CR0                Cr0;

  //
  // Republish Sec Platform Information(2) PPI
  //
  RepublishSecPlatformInformationPpi ();

  //
  // Re-install SEC PPIs using a PEIM produced service if published
  //
  for (Index = 0, Status = EFI_SUCCESS; Status == EFI_SUCCESS; Index++) {
    Status = PeiServicesLocatePpi (
               &gRepublishSecPpiPpiGuid,
               Index,
               &PeiPpiDescriptor,
               (VOID **)&RepublishSecPpiPpi
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Calling RepublishSecPpi instance %d.\n", Index));
      Status2 = RepublishSecPpiPpi->RepublishSecPpis ();
      ASSERT_EFI_ERROR (Status2);
    }
  }

  //
  // Migrate DebugAgentContext.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);

  //
  // Disable interrupts and save current interrupt state
  //
  State = SaveAndDisableInterrupts ();

  //
  // Migrate GDT before NEM near down
  //
  if (PcdGetBool (PcdMigrateTemporaryRamFirmwareVolumes)) {
    Status = MigrateGdt ();
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Migrate page table to permanent memory mapping entire physical address space if CR0.PG is set.
  //
  Cr0.UintN = AsmReadCr0 ();
  if (Cr0.Bits.PG != 0) {
    //
    // Assume CPU runs in 64bit mode if paging is enabled.
    //
    ASSERT (sizeof (UINTN) == sizeof (UINT64));

    Status = MigratePageTable ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SecTemporaryRamDone: Failed to migrate page table to permanent memory: %r.\n", Status));
      CpuDeadLoop ();
    }
  }

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
