/** @file
  PEI Services Table Pointer Library.

  Copyright (c) 2019 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/RiscVCpuLib.h>
#include <Library/RiscVFirmwareContextLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_platform.h>

/**
  Caches a pointer PEI Services Table.

  Caches the pointer to the PEI Services Table specified by PeiServicesTablePointer
  in a CPU specific manner as specified in the CPU binding section of the Platform Initialization
  Pre-EFI Initialization Core Interface Specification.

  If PeiServicesTablePointer is NULL, then ASSERT().

  @param    PeiServicesTablePointer   The address of PeiServices pointer.
**/
VOID
EFIAPI
SetPeiServicesTablePointer (
  IN CONST EFI_PEI_SERVICES  **PeiServicesTablePointer
  )
{
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContext;

  GetFirmwareContextPointer (&FirmwareContext);
  FirmwareContext->PeiServiceTable = (VOID *)(UINTN)PeiServicesTablePointer;

  DEBUG ((
    DEBUG_INFO,
    "Set PEI Service 0x%x at OpenSBI Firmware Context at 0x%x\n",
    PeiServicesTablePointer,
    FirmwareContext
    ));
}

/**
  Retrieves the cached value of the PEI Services Table pointer.

  Returns the cached value of the PEI Services Table pointer in a CPU specific manner
  as specified in the CPU binding section of the Platform Initialization Pre-EFI
  Initialization Core Interface Specification.

  If the cached PEI Services Table pointer is NULL, then ASSERT().

  @return  The pointer to PeiServices.

**/
CONST EFI_PEI_SERVICES **
EFIAPI
GetPeiServicesTablePointer (
  VOID
  )
{
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContext;

  GetFirmwareContextPointer (&FirmwareContext);
  return (CONST EFI_PEI_SERVICES **)FirmwareContext->PeiServiceTable;
}

/**
  The constructor function caches the pointer to PEI services.

  The constructor function caches the pointer to PEI services.
  It will always return EFI_SUCCESS.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiServicesTablePointerLibOpenSbiConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  SetPeiServicesTablePointer (PeiServices);
  return EFI_SUCCESS;
}

/**
  Perform CPU specific actions required to migrate the PEI Services Table
  pointer from temporary RAM to permanent RAM.

  For IA32 CPUs, the PEI Services Table pointer is stored in the 4 bytes
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For X64 CPUs, the PEI Services Table pointer is stored in the 8 bytes
  immediately preceding the Interrupt Descriptor Table (IDT) in memory.
  For Itanium, ARM or RISC-V CPUs, a the PEI Services Table Pointer is stored in
  a dedicated CPU register.  This means that there is no memory storage
  associated with storing the PEI Services Table pointer, so no additional
  migration actions are required for Itanium, ARM or RISC-V CPUs.

**/
VOID
EFIAPI
MigratePeiServicesTablePointer (
  VOID
  )
{
  //
  //  PEI Services Table pointer is cached in the global variable. No additional
  //  migration actions are required.
  //
  return;
}
