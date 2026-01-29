/** @file CpuDxe.c

  CPU DXE Module to produce CPU ARCH Protocol.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CpuDxe.h"
#include "CpuMp.h"
#include <Guid/IdleLoopEvent.h>
#include <Library/CpuMmuLib.h>
#include <Library/TimerLib.h>
#include <Register/LoongArch64/Csr.h>

UINT64  mTimerPeriod = 0;

/**
  IPI Interrupt Handler.

  @param InterruptType    The type of interrupt that occurred
  @param SystemContext    A pointer to the system context when the interrupt occurred
**/
VOID
EFIAPI
IpiInterruptHandler (
  IN EFI_EXCEPTION_TYPE  InterruptType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

//
// Globals used to initialize the protocol
//
EFI_HANDLE             mCpuHandle = NULL;
EFI_CPU_ARCH_PROTOCOL  gCpu       = {
  CpuFlushCpuDataCache,
  CpuEnableInterrupt,
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler,
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  0,          // NumberOfTimers
  4,          // DmaBufferAlignment
};

/**
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
  @retval EFI_INVALID_PARAMETER The processor does not support the cache flush type specified
                                by FlushType.

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
  This function enables interrupt processing by the processor.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.

  @retval EFI_SUCCESS           Interrupts are enabled on the processor.
  @retval EFI_DEVICE_ERROR      Interrupts could not be enabled on the processor.

**/
EFI_STATUS
EFIAPI
CpuEnableInterrupt (
  IN EFI_CPU_ARCH_PROTOCOL  *This
  )
{
  EnableInterrupts ();

  return EFI_SUCCESS;
}

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
  )
{
  DisableInterrupts ();

  return EFI_SUCCESS;
}

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
  )
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = GetInterruptState ();
  return EFI_SUCCESS;
}

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
  return RegisterInterruptHandler (InterruptType, InterruptHandler);
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
  OUT UINT64                 *TimerPeriod   OPTIONAL
  )
{
  UINT64  BeginValue;
  UINT64  EndValue;

  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = AsmReadStableCounter ();

  if (TimerPeriod != NULL) {
    if (mTimerPeriod == 0) {
      //
      // Read time stamp counter before and after delay of 100 microseconds
      //
      BeginValue = AsmReadStableCounter ();
      MicroSecondDelay (100);
      EndValue = AsmReadStableCounter ();
      //
      // Calculate the actual frequency
      //
      mTimerPeriod = DivU64x64Remainder (
                       MultU64x32 (
                         1000 * 1000 * 1000,
                         100
                         ),
                       EndValue - BeginValue,
                       NULL
                       );
    }

    *TimerPeriod = mTimerPeriod;
  }

  return EFI_SUCCESS;
}

/**
  This function modifies the attributes for the memory region specified by BaseAddress and
  Length from their current attributes to the attributes specified by Attributes.

  @param  This             The EFI_CPU_ARCH_PROTOCOL instance.
  @param  BaseAddress      The physical address that is the start address of a memory region.
  @param  Length           The size in bytes of the memory region.
  @param  EfiAttributes    The bit mask of attributes to set for the memory region.

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
  IN UINT64                 EfiAttributes
  )
{
  EFI_STATUS  Status;
  UINTN       PageTable;
  UINT64      PageWalkCfg;

  Status      = EFI_SUCCESS;
  PageTable   = CsrRead (LOONGARCH_CSR_PGDL);
  PageWalkCfg = ((UINT64)CsrRead (LOONGARCH_CSR_PWCTL1)) << 32 | CsrRead (LOONGARCH_CSR_PWCTL0);

  if ((BaseAddress & (EFI_PAGE_SIZE - 1)) != 0) {
    //
    // Minimum granularity is SIZE_4KB.
    //
    DEBUG ((
      DEBUG_INFO,
      "MemoryRegionMap(%lx, %lx, %lx, %lx, %lx): Minimum granularity is SIZE_4KB\n",
      &PageTable,
      PageWalkCfg,
      BaseAddress,
      Length,
      EfiAttributes
      ));

    Status = EFI_UNSUPPORTED;

    return Status;
  }

  Status = MemoryRegionMap (
             &PageTable,
             PageWalkCfg,
             BaseAddress,
             Length,
             EfiAttributes,
             0x0
             );

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Callback function for idle events.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               The pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
IdleLoopEventCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  CpuSleep ();
}

/**
  Refreshes the GCD Memory Space attributes according to Default Config

  This function refreshes the GCD Memory Space attributes according to DefaultConfig

  @retval EFI_SUCCESS  Refresh GCD success.

**/
EFI_STATUS
RefreshGcdMemoryAttributes (
  VOID
  )
{
  EFI_STATUS                       Status;
  UINT32                           Index;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;

  DEBUG ((DEBUG_PAGE, "RefreshGcdMemoryAttributes()\n"));

  //
  // Get the memory space map from GCD
  //
  MemorySpaceMap = NULL;
  Status         = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "RefreshGcdMemoryAttributes - GetMemorySpaceMap() failed! Status: %r\n",
      Status
      ));

    ASSERT_EFI_ERROR (Status);

    return Status;
  }

  for ( Index = 0; Index < NumberOfDescriptors; Index++ ) {
    //
    // If this is system memory, not a class resource like MMIO, and the capability
    // contains a Memory cacheability attributes and the attribute feature is set
    // to 0, we will set its attribute to the WriteBack memory of the LoongArch
    // architecture for the first time.
    //
    if (((MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeSystemMemory) &&
         MemorySpaceMap[Index].Capabilities & EFI_CACHE_ATTRIBUTE_MASK) &&
        (MemorySpaceMap[Index].Attributes == 0))
    {
      if (!(MemorySpaceMap[Index].Capabilities & EFI_MEMORY_WB)) {
        DEBUG ((
          DEBUG_WARN,
          "RefreshGcdMemoryAttributes - SystemMemory Capabilities should support EFI_MEMORY_WB ! \n"
          ));
      }

      //
      // Refresh or Sync Gcd's memory attributes according to Default Paging (CACHE_CC)
      //
      gDS->SetMemorySpaceAttributes (
             MemorySpaceMap[Index].BaseAddress,
             MemorySpaceMap[Index].Length,
             (MemorySpaceMap[Index].Attributes & ~EFI_CACHE_ATTRIBUTE_MASK) |
             (MemorySpaceMap[Index].Capabilities & EFI_MEMORY_WB)
             );
    }
  }

  return EFI_SUCCESS;
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
InitializeCpu (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   IdleLoopEvent;

  InitializeExceptions (&gCpu);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuHandle,
                  &gEfiCpuArchProtocolGuid,
                  &gCpu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Refresh GCD memory space map according to Default Paging.
  //
  RefreshGcdMemoryAttributes ();

  Status = gCpu.RegisterInterruptHandler (
                  &gCpu,
                  EXCEPT_LOONGARCH_INT_IPI,
                  IpiInterruptHandler
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Setup a callback for idle events
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IdleLoopEventCallback,
                  NULL,
                  &gIdleLoopEventGuid,
                  &IdleLoopEvent
                  );
  ASSERT_EFI_ERROR (Status);

  InitializeMpSupport ();

  return Status;
}
