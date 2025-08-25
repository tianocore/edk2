/** @file
  RISC-V CPU DXE driver.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuDxe.h"
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>

//
// Global Variables
//
STATIC BOOLEAN           mInterruptState = FALSE;
STATIC EFI_HANDLE        mCpuHandle      = NULL;
STATIC UINTN             mBootHartId;
RISCV_EFI_BOOT_PROTOCOL  gRiscvBootProtocol;

/**
  Get the boot hartid

  @param  This                   Protocol instance structure
  @param  BootHartId             Pointer to the Boot Hart ID variable

  @retval EFI_SUCCESS            If BootHartId is returned
  @retval EFI_INVALID_PARAMETER  Either "BootHartId" is NULL or "This" is not
                                 a valid RISCV_EFI_BOOT_PROTOCOL instance.

**/
EFI_STATUS
EFIAPI
RiscvGetBootHartId (
  IN RISCV_EFI_BOOT_PROTOCOL  *This,
  OUT UINTN                   *BootHartId
  )
{
  if ((This != &gRiscvBootProtocol) || (BootHartId == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *BootHartId = mBootHartId;
  return EFI_SUCCESS;
}

RISCV_EFI_BOOT_PROTOCOL  gRiscvBootProtocol = {
  RISCV_EFI_BOOT_PROTOCOL_LATEST_VERSION,
  RiscvGetBootHartId
};

EFI_CPU_ARCH_PROTOCOL  gCpu = {
  CpuFlushCpuDataCache,
  CpuEnableInterrupt,
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler,
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  1,                          // NumberOfTimers
  4                           // DmaBufferAlignment
};

//
// CPU Arch Protocol Functions
//

/**
  Flush CPU data cache. If the instruction cache is fully coherent
  with all DMA operations then function can just return EFI_SUCCESS.

  @param  This              Protocol instance structure
  @param  Start             Physical address to start flushing from.
  @param  Length            Number of bytes to flush. Round up to chipset
                            granularity.
  @param  FlushType         Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS       If cache was flushed
  @retval EFI_UNSUPPORTED   If flush type is not supported.
  @retval EFI_DEVICE_ERROR  If requested range could not be flushed.

**/
EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   Start,
  IN UINT64                 Length,
  IN EFI_CPU_FLUSH_TYPE     FlushType
  )
{
  switch (FlushType) {
    case EfiCpuFlushTypeWriteBack:
      WriteBackDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    case EfiCpuFlushTypeInvalidate:
      InvalidateDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    case EfiCpuFlushTypeWriteBackInvalidate:
      WriteBackInvalidateDataCacheRange ((VOID *)(UINTN)Start, (UINTN)Length);
      break;
    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Enables CPU interrupts.

  @param  This              Protocol instance structure

  @retval EFI_SUCCESS       If interrupts were enabled in the CPU
  @retval EFI_DEVICE_ERROR  If interrupts could not be enabled on the CPU.

**/
EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
{
  EnableInterrupts ();
  mInterruptState = TRUE;
  return EFI_SUCCESS;
}

/**
  Disables CPU interrupts.

  @param  This              Protocol instance structure

  @retval EFI_SUCCESS       If interrupts were disabled in the CPU.
  @retval EFI_DEVICE_ERROR  If interrupts could not be disabled on the CPU.

**/
EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
{
  DisableInterrupts ();
  mInterruptState = FALSE;
  return EFI_SUCCESS;
}

/**
  Return the state of interrupts.

  @param  This                   Protocol instance structure
  @param  State                  Pointer to the CPU's current interrupt state

  @retval EFI_SUCCESS            If interrupts were disabled in the CPU.
  @retval EFI_INVALID_PARAMETER  State is NULL.

**/
EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL  *This,
  OUT BOOLEAN                *State
  )
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = mInterruptState;
  return EFI_SUCCESS;
}

/**
  Generates an INIT to the CPU.

  @param  This              Protocol instance structure
  @param  InitType          Type of CPU INIT to perform

  @retval EFI_SUCCESS       If CPU INIT occurred. This value should never be
                            seen.
  @retval EFI_DEVICE_ERROR  If CPU INIT failed.
  @retval EFI_UNSUPPORTED   Requested type of CPU INIT not supported.

**/
EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_CPU_INIT_TYPE      InitType
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Registers a function to be called from the CPU interrupt handler.

  @param  This                   Protocol instance structure
  @param  InterruptType          Defines which interrupt to hook. IA-32
                                 valid range is 0x00 through 0xFF
  @param  InterruptHandler       A pointer to a function of type
                                 EFI_CPU_INTERRUPT_HANDLER that is called
                                 when a processor interrupt occurs.  A null
                                 pointer is an error condition.

  @retval EFI_SUCCESS            If handler installed or uninstalled.
  @retval EFI_ALREADY_STARTED    InterruptHandler is not NULL, and a handler
                                 for InterruptType was previously installed.
  @retval EFI_INVALID_PARAMETER  InterruptHandler is NULL, and a handler for
                                 InterruptType was not previously installed.
  @retval EFI_UNSUPPORTED        The interrupt specified by InterruptType
                                 is not supported.

**/
EFI_STATUS
EFIAPI
CpuRegisterInterruptHandler (
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_EXCEPTION_TYPE         InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER  InterruptHandler
  )
{
  return RegisterCpuInterruptHandler (InterruptType, InterruptHandler);
}

/**
  Returns a timer value from one of the CPU's internal timers. There is no
  inherent time interval between ticks but is a function of the CPU frequency.

  @param  This                - Protocol instance structure.
  @param  TimerIndex          - Specifies which CPU timer is requested.
  @param  TimerValue          - Pointer to the returned timer value.
  @param  TimerPeriod         - A pointer to the amount of time that passes
                                in femtoseconds (10-15) for each increment
                                of TimerValue. If TimerValue does not
                                increment at a predictable rate, then 0 is
                                returned.  The amount of time that has
                                passed between two calls to GetTimerValue()
                                can be calculated with the formula
                                (TimerValue2 - TimerValue1) * TimerPeriod.
                                This parameter is optional and may be NULL.

  @retval EFI_SUCCESS           - If the CPU timer count was returned.
  @retval EFI_UNSUPPORTED       - If the CPU does not have any readable timers.
  @retval EFI_DEVICE_ERROR      - If an error occurred while reading the timer.
  @retval EFI_INVALID_PARAMETER - TimerIndex is not valid or TimerValue is NULL.

**/
EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL  *This,
  IN  UINT32                 TimerIndex,
  OUT UINT64                 *TimerValue,
  OUT UINT64                 *TimerPeriod OPTIONAL
  )
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = (UINT64)RiscVReadTimer ();
  if (TimerPeriod != NULL) {
    *TimerPeriod = DivU64x32 (
                     1000000000000000u,
                     GetPerformanceCounterProperties (NULL, NULL)
                     );
  }

  return EFI_SUCCESS;
}

/**
  Implementation of SetMemoryAttributes() service of CPU Architecture Protocol.

  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  Attributes       The bit mask of attributes to set for the memory region.

  @retval EFI_SUCCESS           The attributes were set for the memory region.
  @retval EFI_ACCESS_DENIED     The attributes for the memory resource range specified by
                                BaseAddress and Length cannot be modified.
  @retval EFI_INVALID_PARAMETER Length is zero.
                                Attributes specified an illegal combination of attributes that
                                cannot be set together.
  @retval EFI_OUT_OF_RESOURCES  There are not enough system resources to modify the attributes of
                                the memory resource range.
  @retval EFI_UNSUPPORTED       The processor does not support one or more bytes of the memory
                                resource range specified by BaseAddress and Length.
                                The bit mask of attributes is not support for the memory resource
                                range specified by BaseAddress and Length.

**/
EFI_STATUS
EFIAPI
CpuSetMemoryAttributes (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 Attributes
  )
{
  return RiscVSetMemoryAttributes (BaseAddress, Length, Attributes);
}

/**
  Initialize the state information for the CPU Architectural Protocol.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS           Thread can be successfully created
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Cannot create the thread

**/
EFI_STATUS
EFIAPI
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  VOID                    *Hob;
  RISCV_SEC_HANDOFF_DATA  *SecData;
  const EFI_GUID          SecHobDataGuid = RISCV_SEC_HANDOFF_HOB_GUID;

  Hob = GetFirstGuidHob (&SecHobDataGuid);
  if (Hob != NULL) {
    SecData     = GET_GUID_HOB_DATA (Hob);
    mBootHartId = SecData->BootHartId;
  } else {
    CONST EFI_HOB_GUID_TYPE  *FdtHob = GetFirstGuidHob (&gFdtHobGuid);

    ASSERT (FdtHob != NULL);

    CONST VOID  *DeviceTreeBase =
      (CONST VOID *)(UINTN)*(CONST UINT64 *)GET_GUID_HOB_DATA (FdtHob);

    ASSERT (FdtCheckHeader (DeviceTreeBase) == 0);

    //
    // /chosen node
    //
    INT32  Node = FdtSubnodeOffsetNameLen (
                    DeviceTreeBase,
                    0,
                    "chosen",
                    sizeof ("chosen") - 1
                    );

    ASSERT (Node >= 0);

    //
    // boot-hartid
    //
    INT32               Len;
    CONST FDT_PROPERTY  *Prop =
      FdtGetProperty (DeviceTreeBase, Node, "boot-hartid", &Len);

    ASSERT (Prop != NULL && Len == sizeof (UINT32));

    //
    // Device-tree cells are big-endian
    //
    UINTN  BootHartId = SwapBytes32 (*(CONST UINT32 *)Prop->Data);

    mBootHartId = BootHartId;
  }

  DEBUG ((DEBUG_INFO, " %a: mBootHartId = 0x%x.\n", __func__, mBootHartId));

  InitializeCpuExceptionHandlers (NULL);

  //
  // Make sure interrupts are disabled
  //
  DisableInterrupts ();

  //
  // Enable MMU
  //
  Status = RiscVConfigureMmu ();
  ASSERT_EFI_ERROR (Status);

  //
  // Initialize FPU
  //
  InitializeFloatingPointUnits ();

  //
  // Install Boot protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gRiscVEfiBootProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gRiscvBootProtocol
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install CPU Architectural Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuHandle,
                  &gEfiCpuArchProtocolGuid,
                  &gCpu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
