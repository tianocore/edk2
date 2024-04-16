/** @file CpuDxe.c

  CPU DXE Module to produce CPU ARCH Protocol.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CPU_DXE_H_
#define CPU_DXE_H_

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/CpuLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/MpInitLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Guid/DebugImageInfoTable.h>
#include <Protocol/Cpu.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/LoadedImage.h>

//
// For coding convenience, define the maximum valid
// LoongArch exception.
// Since UEFI V2.11, it will be present in DebugSupport.h.
//
#define MAX_LOONGARCH_EXCEPTION  64

/*
  This function flushes the range of addresses from Start to Start+Length
  from the processor's data cache. If Start is not aligned to a cache line
  boundary, then the bytes before Start to the preceding cache line boundary
  are also flushed. If Start+Length is not aligned to a cache line boundary,
  then the bytes past Start+Length to the end of the next cache line boundary
  are also flushed. The FlushType of EfiCpuFlushTypeWriteBackInvalidate must be
  supported. If the data cache is fully coherent with all DMA operations, then
  this function can just return EFI_SUCCESS. If the processor does not support
  flushing a range of the data cache, then the entire data cache can be flushed.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  Start            The beginning physical address to flush from the processor's data
                           cache.
  @param  Length           The number of bytes to flush from the processor's data cache. This
                           function may flush more bytes than Length specifies depending upon
                           the granularity of the flush operation that the processor supports.
  @param  FlushType        Specifies the type of flush operation to perform.

  @retval EFI_SUCCESS           The address range from Start to Start+Length was flushed from
                                the processor's data cache.
  @retval EFI_UNSUPPORTEDT      The processor does not support the cache flush type specified
                                by FlushType.
  @retval EFI_DEVICE_ERROR      The address range from Start to Start+Length could not be flushed
                                from the processor's data cache.

**/
EFI_STATUS
EFIAPI
CpuFlushCpuDataCache (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_PHYSICAL_ADDRESS   Start,
  IN UINT64                 Length,
  IN EFI_CPU_FLUSH_TYPE     FlushType
  );

/**
  This function enables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be enabled on the processor.

**/
EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  );

/**
  This function disables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are disabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be disabled on the processor.

**/
EFI_STATUS
EFIAPI
CpuDisableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  );

/**
  This function retrieves the processor's current interrupt state a returns it in
  State. If interrupts are currently enabled, then TRUE is returned. If interrupts
  are currently disabled, then FALSE is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  State            A pointer to the processor's current interrupt state. Set to TRUE if
                           interrupts are enabled and FALSE if interrupts are disabled.

  @retval EFI_SUCCESS           The processor's current interrupt state was returned in State.
  @retval EFI_INVALID_PARAMETER State is NULL.

**/
EFI_STATUS
EFIAPI
CpuGetInterruptState (
  IN  EFI_CPU_ARCH_PROTOCOL  *This,
  OUT BOOLEAN                *State
  );

/**
  This function generates an INIT on the processor. If this function succeeds, then the
  processor will be reset, and control will not be returned to the caller. If InitType is
  not supported by this processor, or the processor cannot programmatically generate an
  INIT without help from external hardware, then EFI_UNSUPPORTED is returned. If an error
  occurs attempting to generate an INIT, then EFI_DEVICE_ERROR is returned.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  InitType         The type of processor INIT to perform.

  @retval EFI_SUCCESS           The processor INIT was performed. This return code should never be seen.
  @retval EFI_UNSUPPORTED       The processor INIT operation specified by InitType is not supported
                                by this processor.
  @retval EFI_DEVICE_ERROR      The processor INIT failed.

**/
EFI_STATUS
EFIAPI
CpuInit (
  IN EFI_CPU_ARCH_PROTOCOL  *This,
  IN EFI_CPU_INIT_TYPE      InitType
  );

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
  );

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
  OUT UINT64                 *TimerPeriod   OPTIONAL
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
  );

/**
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
  );

/**
  Initialize interrupt handling for DXE phase.

  @param  Cpu A pointer of EFI_CPU_ARCH_PROTOCOL instance.

  @return VOID.

**/
VOID
InitializeExceptions (
  IN EFI_CPU_ARCH_PROTOCOL  *gCpu
  );

/**
  Converts EFI Attributes to corresponding architecture Attributes.

  @param[in]  EfiAttributes     Efi Attributes.

  @retval  Corresponding architecture attributes.
**/
UINTN
EFIAPI
EfiAttributeConverse (
  IN UINTN  EfiAttributes
  );

#endif // CPU_DXE_H_
