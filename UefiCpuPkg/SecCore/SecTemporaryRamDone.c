/** @file
  SEC platform information(2) PPI.

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/CpuPageTableLib.h>
#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>
#include "SecMain.h"

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
  Get Paging Mode

  @retval  Paging Mode.
**/
PAGING_MODE
GetPagingMode (
  VOID
  )
{
  IA32_CR4                    Cr4;
  BOOLEAN                     Page5LevelSupport;
  UINT32                      RegEax;
  CPUID_EXTENDED_CPU_SIG_EDX  RegEdx;
  BOOLEAN                     Page1GSupport;
  PAGING_MODE                 PagingMode;

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

  return PagingMode;
}

/**
  Get max physical address supported by specific page mode

  @param[in]  PagingMode           The paging mode.

  @retval  Max Address.
**/
UINT32
GetMaxAddress (
  IN PAGING_MODE  PagingMode
  )
{
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX  VirPhyAddressSize;
  UINT32                          MaxExtendedFunctionId;
  UINT32                          MaxAddressBits;

  VirPhyAddressSize.Uint32 = 0;

  //
  // Get Maximum Physical Address Bits
  // Get the number of address lines; Maximum Physical Address is 2^PhysicalAddressBits - 1.
  // If CPUID does not supported, then use a max value of 36 as per SDM 3A, 4.1.4.
  //
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunctionId, NULL, NULL, NULL);
  if (MaxExtendedFunctionId >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
    MaxAddressBits = VirPhyAddressSize.Bits.PhysicalAddressBits;
  } else {
    MaxAddressBits = 36;
  }

  if ((PagingMode == Paging4Level1GB) || (PagingMode == Paging4Level)) {
    //
    // The max liner address bits is 48 for 4 level page table.
    //
    MaxAddressBits = MIN (VirPhyAddressSize.Bits.PhysicalAddressBits, 48);
  }

  return MaxAddressBits;
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
  PAGING_MODE             PagingMode;
  UINT32                  MaxAddressBits;
  UINTN                   PageTable;
  EFI_PHYSICAL_ADDRESS    Buffer;
  UINTN                   BufferSize;
  UINT64                  Length;
  UINT64                  Address;
  IA32_MAP_ATTRIBUTE      MapAttribute;
  IA32_MAP_ATTRIBUTE      MapMask;

  PageTable                   = 0;
  BufferSize                  = 0;
  MapAttribute.Uint64         = 0;
  MapAttribute.Bits.Present   = 1;
  MapAttribute.Bits.ReadWrite = 1;
  MapMask.Uint64              = MAX_UINT64;

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

    //
    // Get PagingMode & MaxAddressBits.
    //
    PagingMode     = GetPagingMode ();
    MaxAddressBits = GetMaxAddress (PagingMode);
    DEBUG ((DEBUG_INFO, "SecTemporaryRamDone: PagingMode = 0x%lx, MaxAddressBits = %d\n", PagingMode, MaxAddressBits));

    //
    // Create page table to cover the max mapping address in physical memory before Temp
    // Ram Exit. The max mapping address is defined by PcdMaxMappingAddressBeforeTempRamExit.
    //
    Length = FixedPcdGet64 (PcdMaxMappingAddressBeforeTempRamExit);
    Length = MIN (LShiftU64 (1, MaxAddressBits), Length);
    if (Length != 0) {
      Status = PageTableMap (&PageTable, PagingMode, 0, &BufferSize, 0, Length, &MapAttribute, &MapMask, NULL);
      ASSERT (Status == EFI_BUFFER_TOO_SMALL);
      if (Status != EFI_BUFFER_TOO_SMALL) {
        return Status;
      }

      Status = PeiServicesAllocatePages (
                 EfiBootServicesData,
                 EFI_SIZE_TO_PAGES (BufferSize),
                 &Buffer
                 );
      if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = PageTableMap (&PageTable, PagingMode, (VOID *)(UINTN)Buffer, &BufferSize, 0, Length, &MapAttribute, &MapMask, NULL);
      ASSERT (BufferSize == 0);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "SecTemporaryRamDone: Failed to create page table in physical memory before Temp Ram Exit: %r.\n", Status));
        CpuDeadLoop ();
      }

      AsmWriteCr3 (PageTable);
    }
  }

  //
  // Disable Temporary RAM after Stack and Heap have been migrated at this point.
  //
  SecPlatformDisableTemporaryMemory ();

  //
  // Expanding the page table to cover the entire memory space since the physical memory is WB after TempRamExit.
  //
  if ((Cr0.Bits.PG != 0) && (Length < LShiftU64 (1, MaxAddressBits))) {
    Address = Length;
    Length  = LShiftU64 (1, MaxAddressBits) - Length;

    MapAttribute.Uint64         = Address;
    MapAttribute.Bits.Present   = 1;
    MapAttribute.Bits.ReadWrite = 1;

    Status = PageTableMap (&PageTable, PagingMode, 0, &BufferSize, Address, Length, &MapAttribute, &MapMask, NULL);
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      return Status;
    }

    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               EFI_SIZE_TO_PAGES (BufferSize),
               &Buffer
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = PageTableMap (&PageTable, PagingMode, (VOID *)(UINTN)Buffer, &BufferSize, Address, Length, &MapAttribute, &MapMask, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "SecTemporaryRamDone: Failed to create full range page table in physical memory after Temp Ram Exit: %r.\n", Status));
      CpuDeadLoop ();
    }

    AsmWriteCr3 (PageTable);
  }

  //
  // Restore original interrupt state
  //
  SetInterruptState (State);

  return EFI_SUCCESS;
}
