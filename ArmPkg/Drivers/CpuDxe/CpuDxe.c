/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuDxe.h"

#include <Guid/IdleLoopEvent.h>

#include <Library/MemoryAllocationLib.h>

BOOLEAN  mIsFlushingGCD;

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
  @retval EFI_UNSUPPORTED       The processor does not support the cache flush type specified
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
  ArmEnableInterrupts ();

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
  ArmDisableInterrupts ();

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

  *State = ArmGetInterruptState ();
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

EFI_STATUS
EFIAPI
CpuGetTimerValue (
  IN  EFI_CPU_ARCH_PROTOCOL  *This,
  IN  UINT32                 TimerIndex,
  OUT UINT64                 *TimerValue,
  OUT UINT64                 *TimerPeriod   OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
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

//
// Globals used to initialize the protocol
//
EFI_HANDLE             mCpuHandle = NULL;
EFI_CPU_ARCH_PROTOCOL  mCpu       = {
  CpuFlushCpuDataCache,
  CpuEnableInterrupt,
  CpuDisableInterrupt,
  CpuGetInterruptState,
  CpuInit,
  CpuRegisterInterruptHandler,
  CpuGetTimerValue,
  CpuSetMemoryAttributes,
  0,          // NumberOfTimers
  2048,       // DmaBufferAlignment
};

STATIC
VOID
InitializeDma (
  IN OUT  EFI_CPU_ARCH_PROTOCOL  *CpuArchProtocol
  )
{
  CpuArchProtocol->DmaBufferAlignment = ArmCacheWritebackGranule ();
}

/**
  Map all EfiConventionalMemory regions in the memory map with NX
  attributes so that allocating or freeing EfiBootServicesData regions
  does not result in changes to memory permission attributes.

**/
STATIC
VOID
RemapUnusedMemoryNx (
  VOID
  )
{
  UINT64                 TestBit;
  UINTN                  MemoryMapSize;
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapEnd;
  EFI_STATUS             Status;

  TestBit = LShiftU64 (1, EfiBootServicesData);
  if ((PcdGet64 (PcdDxeNxMemoryProtectionPolicy) & TestBit) == 0) {
    return;
  }

  MemoryMapSize = 0;
  MemoryMap     = NULL;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  do {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);

  ASSERT_EFI_ERROR (Status);

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    if (MemoryMapEntry->Type == EfiConventionalMemory) {
      ArmSetMemoryAttributes (
        MemoryMapEntry->PhysicalStart,
        EFI_PAGES_TO_SIZE (MemoryMapEntry->NumberOfPages),
        EFI_MEMORY_XP,
        EFI_MEMORY_XP
        );
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
}

EFI_STATUS
CpuDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   IdleLoopEvent;

  InitializeExceptions (&mCpu);

  InitializeDma (&mCpu);

  //
  // Once we install the CPU arch protocol, the DXE core's memory
  // protection routines will invoke them to manage the permissions of page
  // allocations as they are created. Given that this includes pages
  // allocated for page tables by this driver, we must ensure that unused
  // memory is mapped with the same permissions as boot services data
  // regions. Otherwise, we may end up with unbounded recursion, due to the
  // fact that updating permissions on a newly allocated page table may trigger
  // a block entry split, which triggers a page table allocation, etc etc
  //
  if (FeaturePcdGet (PcdRemapUnusedMemoryNx)) {
    RemapUnusedMemoryNx ();
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mCpuHandle,
                  &gEfiCpuArchProtocolGuid,
                  &mCpu,
                  &gEfiMemoryAttributeProtocolGuid,
                  &mMemoryAttribute,
                  NULL
                  );

  //
  // Make sure GCD and MMU settings match. This API calls gDS->SetMemorySpaceAttributes ()
  // and that calls EFI_CPU_ARCH_PROTOCOL.SetMemoryAttributes, so this code needs to go
  // after the protocol is installed
  //
  mIsFlushingGCD = TRUE;
  SyncCacheConfig (&mCpu);
  mIsFlushingGCD = FALSE;

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

  return Status;
}
