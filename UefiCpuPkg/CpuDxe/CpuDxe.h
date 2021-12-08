/** @file
  CPU DXE Module to produce CPU ARCH Protocol and CPU MP Protocol.

  Copyright (c) 2008 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_DXE_H_
#define _CPU_DXE_H_

#include <PiDxe.h>

#include <Protocol/Cpu.h>
#include <Protocol/MpService.h>
#include <Register/Intel/Msr.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/MtrrLib.h>
#include <Library/LocalApicLib.h>
#include <Library/UefiCpuLib.h>
#include <Library/UefiLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/HobLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MpInitLib.h>
#include <Library/TimerLib.h>

#include <Guid/IdleLoopEvent.h>
#include <Guid/VectorHandoffTable.h>

#define HEAP_GUARD_NONSTOP_MODE       \
        ((PcdGet8 (PcdHeapGuardPropertyMask) & (BIT6|BIT4|BIT1|BIT0)) > BIT6)

#define NULL_DETECTION_NONSTOP_MODE   \
        ((PcdGet8 (PcdNullPointerDetectionPropertyMask) & (BIT6|BIT0)) > BIT6)

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
  );

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
  );

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
  );

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
  );

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
  OUT UINT64                 *TimerPeriod OPTIONAL
  );

/**
  Set memory cacheability attributes for given range of memory.

  @param  This                   Protocol instance structure
  @param  BaseAddress            Specifies the start address of the
                                 memory range
  @param  Length                 Specifies the length of the memory range
  @param  Attributes             The memory cacheability for the memory range

  @retval EFI_SUCCESS            If the cacheability of that memory range is
                                 set successfully
  @retval EFI_UNSUPPORTED        If the desired operation cannot be done
  @retval EFI_INVALID_PARAMETER  The input parameter is not correct,
                                 such as Length = 0

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
  Initialize Global Descriptor Table.

**/
VOID
InitGlobalDescriptorTable (
  VOID
  );

/**
  Sets the code selector (CS).

  @param  Selector  Value of code selector.

**/
VOID
EFIAPI
SetCodeSelector (
  UINT16  Selector
  );

/**
  Sets the data selector (DS).

  @param  Selector  Value of data selector.

**/
VOID
EFIAPI
SetDataSelectors (
  UINT16  Selector
  );

/**
  Update GCD memory space attributes according to current page table setup.
**/
VOID
RefreshGcdMemoryAttributesFromPaging (
  VOID
  );

/**
  Special handler for #DB exception, which will restore the page attributes
  (not-present). It should work with #PF handler which will set pages to
  'present'.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
DebugExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

/**
  Special handler for #PF exception, which will set the pages which caused
  #PF to be 'present'. The attribute of those pages should be restored in
  the subsequent #DB handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.

**/
VOID
EFIAPI
PageFaultExceptionHandler (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

extern BOOLEAN  mIsAllocatingPageTable;
extern UINTN    mNumberOfProcessors;

#endif
