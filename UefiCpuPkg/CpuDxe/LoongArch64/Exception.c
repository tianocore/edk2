/** @file Exception.c

  CPU DXE Module initialization exception instance.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CpuDxe.h"
#include <Guid/VectorHandoffTable.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Register/LoongArch64/Csr.h>

VOID
ExceptionEntryStart (
  VOID
  );

VOID
ExceptionEntryEnd (
  VOID
  );

/**
  This function registers and enables the handler specified by InterruptHandler for a processor
  interrupt or exception type specified by InterruptType. If InterruptHandler is NULL, then the
  handler for the processor interrupt or exception type specified by InterruptType is uninstalled.
  The installed handler is called once for each processor interrupt or exception.

  @param  InterruptType    A pointer to the processor's current interrupt state. Set to TRUE if interrupts
                           are enabled and FALSE if interrupts are disabled.
  @param  InterruptHandler A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                           when a processor interrupt occurs. If this parameter is NULL, then the handler
                           will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported.

**/
EFI_STATUS
RegisterInterruptHandler (
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  return (EFI_STATUS)RegisterCpuInterruptHandler (InterruptType, InterruptHandler);
}

/**
  Update the exception start entry code.

  @retval EFI_SUCCESS           Update the exception start entry code down.
  @retval EFI_OUT_OF_RESOURCES  The start entry code size out of bounds.

**/
EFI_STATUS
EFIAPI
UpdateExceptionStartEntry (
  VOID
  )
{
  EFI_PHYSICAL_ADDRESS  ExceptionStartEntry;
  UINTN                 VectorLength;
  UINTN                 MaxLength;
  UINTN                 MaxSizeOfVector;

  VectorLength = (UINTN)ExceptionEntryEnd - (UINTN)ExceptionEntryStart;

  //
  // A vector is up to 512 bytes.
  //
  MaxSizeOfVector = 512;
  MaxLength       = (MAX_LOONGARCH_EXCEPTION + MAX_LOONGARCH_INTERRUPT) * MaxSizeOfVector;

  if (VectorLength > MaxLength) {
    return EFI_OUT_OF_RESOURCES;
  }

  ExceptionStartEntry = PcdGet64 (PcdLoongArchExceptionVectorBaseAddress);

  InvalidateInstructionCacheRange ((VOID *)ExceptionStartEntry, VectorLength);
  CopyMem ((VOID *)ExceptionStartEntry, (VOID *)ExceptionEntryStart, VectorLength);
  InvalidateInstructionCacheRange ((VOID *)ExceptionStartEntry, VectorLength);
  InvalidateDataCache ();

  //
  // If PcdLoongArchExceptionVectorBaseAddress is not used during SEC and PEI stages, the exception
  // base addres is set to PcdLoongArchExceptionVectorBaseAddress.
  //
  if (CsrRead (LOONGARCH_CSR_EBASE) != ExceptionStartEntry) {
    SetExceptionBaseAddress (ExceptionStartEntry);
  }

  return EFI_SUCCESS;
}

/**
  Initialize interrupt handling for DXE phase.

  @param  Cpu A pointer of EFI_CPU_ARCH_PROTOCOL instance.

  @return VOID.

**/
VOID
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL  *Cpu
  )
{
  EFI_STATUS               Status;
  EFI_VECTOR_HANDOFF_INFO  *VectorInfoList;
  EFI_VECTOR_HANDOFF_INFO  *VectorInfo;
  BOOLEAN                  IrqEnabled;

  VectorInfo = (EFI_VECTOR_HANDOFF_INFO *)NULL;
  Status     = EfiGetSystemConfigurationTable (&gEfiVectorHandoffTableGuid, (VOID **)&VectorInfoList);

  if ((Status == EFI_SUCCESS) && (VectorInfoList != NULL)) {
    VectorInfo = VectorInfoList;
  }

  //
  // Disable interrupts
  //
  Cpu->GetInterruptState (Cpu, &IrqEnabled);
  if (IrqEnabled) {
    Cpu->DisableInterrupt (Cpu);
  }

  //
  // Update the Exception Start Entry code to point into CpuDxe.
  //
  Status = UpdateExceptionStartEntry ();
  if (EFI_ERROR (Status)) {
    DebugPrint (DEBUG_ERROR, "[%a]: Exception start entry code out of bounds!\n", __func__);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Intialize the CpuExceptionHandlerLib so we take over the exception vector table from the DXE Core
  //
  Status = InitializeCpuExceptionHandlers (VectorInfo);
  ASSERT_EFI_ERROR (Status);

  //
  // Enable interrupts
  //
  DebugPrint (DEBUG_INFO, "InitializeExceptions,IrqEnabled = %x\n", IrqEnabled);
  if (!IrqEnabled) {
    Status = Cpu->EnableInterrupt (Cpu);
  }

  ASSERT_EFI_ERROR (Status);
}
