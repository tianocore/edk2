/** @file
  CPU DXE Module to produce CPU ARCH Protocol.

  Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuDxe.h"
#include "CpuMp.h"
#include "CpuPageTable.h"

#define CPU_INTERRUPT_NUM  256

//
// Global Variables
//
EFI_HANDLE  mCpuHandle = NULL;
BOOLEAN     mIsFlushingGCD;
BOOLEAN     mIsAllocatingPageTable = FALSE;
UINT64      mTimerPeriod           = 0;

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
  if (FlushType == EfiCpuFlushTypeWriteBackInvalidate) {
    AsmWbinvd ();
    return EFI_SUCCESS;
  } else if (FlushType == EfiCpuFlushTypeInvalidate) {
    AsmInvd ();
    return EFI_SUCCESS;
  } else {
    return EFI_UNSUPPORTED;
  }
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

  *State = GetInterruptState ();

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
  UINT64  BeginValue;
  UINT64  EndValue;

  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = AsmReadTsc ();

  if (TimerPeriod != NULL) {
    if (mTimerPeriod == 0) {
      //
      // Read time stamp counter before and after delay of 100 microseconds
      //
      BeginValue = AsmReadTsc ();
      MicroSecondDelay (100);
      EndValue = AsmReadTsc ();
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
  A minimal wrapper function that allows MtrrSetAllMtrrs() to be passed to
  EFI_MP_SERVICES_PROTOCOL.StartupAllAPs() as Procedure.

  @param[in] Buffer  Pointer to an MTRR_SETTINGS object, to be passed to
                     MtrrSetAllMtrrs().
**/
VOID
EFIAPI
SetMtrrsFromBuffer (
  IN VOID  *Buffer
  )
{
  MtrrSetAllMtrrs (Buffer);
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
  RETURN_STATUS             Status;
  MTRR_MEMORY_CACHE_TYPE    CacheType;
  EFI_STATUS                MpStatus;
  EFI_MP_SERVICES_PROTOCOL  *MpService;
  MTRR_SETTINGS             MtrrSettings;
  UINT64                    CacheAttributes;
  UINT64                    MemoryAttributes;
  MTRR_MEMORY_CACHE_TYPE    CurrentCacheType;

  //
  // If this function is called because GCD SetMemorySpaceAttributes () is called
  // by RefreshGcdMemoryAttributes (), then we are just synchronizing GCD memory
  // map with MTRR values. So there is no need to modify MTRRs, just return immediately
  // to avoid unnecessary computing.
  //
  if (mIsFlushingGCD) {
    DEBUG ((DEBUG_VERBOSE, "  Flushing GCD\n"));
    return EFI_SUCCESS;
  }

  //
  // During memory attributes updating, new pages may be allocated to setup
  // smaller granularity of page table. Page allocation action might then cause
  // another calling of CpuSetMemoryAttributes() recursively, due to memory
  // protection policy configured (such as PcdDxeNxMemoryProtectionPolicy).
  // Since this driver will always protect memory used as page table by itself,
  // there's no need to apply protection policy requested from memory service.
  // So it's safe to just return EFI_SUCCESS if this time of calling is caused
  // by page table memory allocation.
  //
  if (mIsAllocatingPageTable) {
    DEBUG ((DEBUG_VERBOSE, "  Allocating page table memory\n"));
    return EFI_SUCCESS;
  }

  CacheAttributes  = Attributes & EFI_CACHE_ATTRIBUTE_MASK;
  MemoryAttributes = Attributes & EFI_MEMORY_ATTRIBUTE_MASK;

  if (Attributes != (CacheAttributes | MemoryAttributes)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CacheAttributes != 0) {
    if (!IsMtrrSupported ()) {
      return EFI_UNSUPPORTED;
    }

    switch (CacheAttributes) {
      case EFI_MEMORY_UC:
        CacheType = CacheUncacheable;
        break;

      case EFI_MEMORY_WC:
        CacheType = CacheWriteCombining;
        break;

      case EFI_MEMORY_WT:
        CacheType = CacheWriteThrough;
        break;

      case EFI_MEMORY_WP:
        CacheType = CacheWriteProtected;
        break;

      case EFI_MEMORY_WB:
        CacheType = CacheWriteBack;
        break;

      default:
        return EFI_INVALID_PARAMETER;
    }

    CurrentCacheType = MtrrGetMemoryAttribute (BaseAddress);
    if (CurrentCacheType != CacheType) {
      //
      // call MTRR library function
      //
      Status = MtrrSetMemoryAttribute (
                 BaseAddress,
                 Length,
                 CacheType
                 );

      if (!RETURN_ERROR (Status)) {
        MpStatus = gBS->LocateProtocol (
                          &gEfiMpServiceProtocolGuid,
                          NULL,
                          (VOID **)&MpService
                          );
        //
        // Synchronize the update with all APs
        //
        if (!EFI_ERROR (MpStatus)) {
          MtrrGetAllMtrrs (&MtrrSettings);
          MpStatus = MpService->StartupAllAPs (
                                  MpService,          // This
                                  SetMtrrsFromBuffer, // Procedure
                                  FALSE,              // SingleThread
                                  NULL,               // WaitEvent
                                  0,                  // TimeoutInMicrosecsond
                                  &MtrrSettings,      // ProcedureArgument
                                  NULL                // FailedCpuList
                                  );
          ASSERT (MpStatus == EFI_SUCCESS || MpStatus == EFI_NOT_STARTED);
        }
      }

      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Set memory attribute by page table
  //
  return AssignMemoryPageAttributes (NULL, BaseAddress, Length, MemoryAttributes, NULL);
}

/**
  Gets GCD Mem Space type from MTRR Type.

  This function gets GCD Mem Space type from MTRR Type.

  @param  Type  MTRR memory type

  @return GCD Mem Space type

**/
UINT64
GetMemorySpaceAttributeFromMtrrType (
  IN MTRR_MEMORY_CACHE_TYPE  Type
  )
{
  switch (Type) {
    case CacheUncacheable:
      return EFI_MEMORY_UC;
    case CacheWriteCombining:
      return EFI_MEMORY_WC;
    case CacheWriteThrough:
      return EFI_MEMORY_WT;
    case CacheWriteProtected:
      return EFI_MEMORY_WP;
    case CacheWriteBack:
      return EFI_MEMORY_WB;
    default:
      return 0;
  }
}

/**
  Searches memory descriptors covered by given memory range.

  This function searches into the Gcd Memory Space for descriptors
  (from StartIndex to EndIndex) that contains the memory range
  specified by BaseAddress and Length.

  @param  MemorySpaceMap       Gcd Memory Space Map as array.
  @param  NumberOfDescriptors  Number of descriptors in map.
  @param  BaseAddress          BaseAddress for the requested range.
  @param  Length               Length for the requested range.
  @param  StartIndex           Start index into the Gcd Memory Space Map.
  @param  EndIndex             End index into the Gcd Memory Space Map.

  @retval EFI_SUCCESS          Search successfully.
  @retval EFI_NOT_FOUND        The requested descriptors does not exist.

**/
EFI_STATUS
SearchGcdMemorySpaces (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN UINTN                            NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS             BaseAddress,
  IN UINT64                           Length,
  OUT UINTN                           *StartIndex,
  OUT UINTN                           *EndIndex
  )
{
  UINTN  Index;

  *StartIndex = 0;
  *EndIndex   = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if ((BaseAddress >= MemorySpaceMap[Index].BaseAddress) &&
        (BaseAddress < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length))
    {
      *StartIndex = Index;
    }

    if ((BaseAddress + Length - 1 >= MemorySpaceMap[Index].BaseAddress) &&
        (BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length))
    {
      *EndIndex = Index;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Sets the attributes for a specified range in Gcd Memory Space Map.

  This function sets the attributes for a specified range in
  Gcd Memory Space Map.

  @param  MemorySpaceMap       Gcd Memory Space Map as array
  @param  NumberOfDescriptors  Number of descriptors in map
  @param  BaseAddress          BaseAddress for the range
  @param  Length               Length for the range
  @param  Attributes           Attributes to set

  @retval EFI_SUCCESS          Memory attributes set successfully
  @retval EFI_NOT_FOUND        The specified range does not exist in Gcd Memory Space

**/
EFI_STATUS
SetGcdMemorySpaceAttributes (
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap,
  IN UINTN                            NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS             BaseAddress,
  IN UINT64                           Length,
  IN UINT64                           Attributes
  )
{
  EFI_STATUS            Status;
  UINTN                 Index;
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  EFI_PHYSICAL_ADDRESS  RegionStart;
  UINT64                RegionLength;

  //
  // Get all memory descriptors covered by the memory range
  //
  Status = SearchGcdMemorySpaces (
             MemorySpaceMap,
             NumberOfDescriptors,
             BaseAddress,
             Length,
             &StartIndex,
             &EndIndex
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Go through all related descriptors and set attributes accordingly
  //
  for (Index = StartIndex; Index <= EndIndex; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }

    //
    // Calculate the start and end address of the overlapping range
    //
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress) {
      RegionStart = BaseAddress;
    } else {
      RegionStart = MemorySpaceMap[Index].BaseAddress;
    }

    if (BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      RegionLength = BaseAddress + Length - RegionStart;
    } else {
      RegionLength = MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length - RegionStart;
    }

    //
    // Set memory attributes according to MTRR attribute and the original attribute of descriptor
    //
    gDS->SetMemorySpaceAttributes (
           RegionStart,
           RegionLength,
           (MemorySpaceMap[Index].Attributes & ~EFI_CACHE_ATTRIBUTE_MASK) | (MemorySpaceMap[Index].Capabilities & Attributes)
           );
  }

  return EFI_SUCCESS;
}

/**
  Refreshes the GCD Memory Space attributes according to MTRRs.

  This function refreshes the GCD Memory Space attributes according to MTRRs.

**/
VOID
RefreshMemoryAttributesFromMtrr (
  VOID
  )
{
  EFI_STATUS                       Status;
  RETURN_STATUS                    ReturnStatus;
  UINTN                            Index;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;
  MTRR_MEMORY_RANGE                *Ranges;
  UINTN                            RangeCount;

  MemorySpaceMap = NULL;

  //
  // Get the memory space map from GCD
  //
  Status = gDS->GetMemorySpaceMap (
                  &NumberOfDescriptors,
                  &MemorySpaceMap
                  );
  ASSERT_EFI_ERROR (Status);

  RangeCount   = 0;
  ReturnStatus = MtrrGetMemoryAttributesInMtrrSettings (NULL, NULL, &RangeCount);
  ASSERT (ReturnStatus == RETURN_BUFFER_TOO_SMALL);
  Ranges = AllocatePool (sizeof (*Ranges) * RangeCount);
  ASSERT (Ranges != NULL);
  ReturnStatus = MtrrGetMemoryAttributesInMtrrSettings (NULL, Ranges, &RangeCount);
  ASSERT_RETURN_ERROR (ReturnStatus);

  for (Index = 0; Index < RangeCount; Index++) {
    SetGcdMemorySpaceAttributes (
      MemorySpaceMap,
      NumberOfDescriptors,
      Ranges[Index].BaseAddress,
      Ranges[Index].Length,
      GetMemorySpaceAttributeFromMtrrType (Ranges[Index].Type)
      );
  }

  //
  // Free memory space map allocated by GCD service GetMemorySpaceMap ()
  //
  if (MemorySpaceMap != NULL) {
    FreePool (MemorySpaceMap);
  }
}

/**
 Check if paging is enabled or not.
**/
BOOLEAN
IsPagingAndPageAddressExtensionsEnabled (
  VOID
  )
{
  IA32_CR0  Cr0;
  IA32_CR4  Cr4;

  Cr0.UintN = AsmReadCr0 ();
  Cr4.UintN = AsmReadCr4 ();

  return ((Cr0.Bits.PG != 0) && (Cr4.Bits.PAE != 0));
}

/**
  Refreshes the GCD Memory Space attributes according to MTRRs and Paging.

  This function refreshes the GCD Memory Space attributes according to MTRRs
  and page tables.

**/
VOID
RefreshGcdMemoryAttributes (
  VOID
  )
{
  mIsFlushingGCD = TRUE;

  if (IsMtrrSupported ()) {
    RefreshMemoryAttributesFromMtrr ();
  }

  if (IsPagingAndPageAddressExtensionsEnabled ()) {
    RefreshGcdMemoryAttributesFromPaging ();
  }

  mIsFlushingGCD = FALSE;
}

/**
  Initialize Interrupt Descriptor Table for interrupt handling.

**/
VOID
InitInterruptDescriptorTable (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_VECTOR_HANDOFF_INFO   *VectorInfoList;
  EFI_VECTOR_HANDOFF_INFO   *VectorInfo;
  IA32_IDT_GATE_DESCRIPTOR  *IdtTable;
  IA32_DESCRIPTOR           IdtDescriptor;
  UINTN                     IdtEntryCount;

  VectorInfo = NULL;
  Status     = EfiGetSystemConfigurationTable (&gEfiVectorHandoffTableGuid, (VOID **)&VectorInfoList);
  if ((Status == EFI_SUCCESS) && (VectorInfoList != NULL)) {
    VectorInfo = VectorInfoList;
  }

  AsmReadIdtr (&IdtDescriptor);
  IdtEntryCount = (IdtDescriptor.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR);
  if (IdtEntryCount < CPU_INTERRUPT_NUM) {
    //
    // Increase Interrupt Descriptor Table and Copy the old IDT table in
    //
    IdtTable = AllocateZeroPool (sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM);
    ASSERT (IdtTable != NULL);
    CopyMem (IdtTable, (VOID *)IdtDescriptor.Base, sizeof (IA32_IDT_GATE_DESCRIPTOR) * IdtEntryCount);

    //
    // Load Interrupt Descriptor Table
    //
    IdtDescriptor.Base  = (UINTN)IdtTable;
    IdtDescriptor.Limit = (UINT16)(sizeof (IA32_IDT_GATE_DESCRIPTOR) * CPU_INTERRUPT_NUM - 1);
    AsmWriteIdtr (&IdtDescriptor);
  }

  Status = InitializeCpuExceptionHandlers (VectorInfo);
  ASSERT_EFI_ERROR (Status);
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
  Ensure the compatibility of a memory space descriptor with the MMIO aperture.

  The memory space descriptor can come from the GCD memory space map, or it can
  represent a gap between two neighboring memory space descriptors. In the
  latter case, the GcdMemoryType field is expected to be
  EfiGcdMemoryTypeNonExistent.

  If the memory space descriptor already has type
  EfiGcdMemoryTypeMemoryMappedIo, and its capabilities are a superset of the
  required capabilities, then no action is taken -- it is by definition
  compatible with the aperture.

  Otherwise, the intersection of the memory space descriptor is calculated with
  the aperture. If the intersection is the empty set (no overlap), no action is
  taken; the memory space descriptor is compatible with the aperture.

  Otherwise, the type of the descriptor is investigated again. If the type is
  EfiGcdMemoryTypeNonExistent (representing a gap, or a genuine descriptor with
  such a type), then an attempt is made to add the intersection as MMIO space
  to the GCD memory space map, with the specified capabilities. This ensures
  continuity for the aperture, and the descriptor is deemed compatible with the
  aperture.

  Otherwise, the memory space descriptor is incompatible with the MMIO
  aperture.

  @param[in] Base         Base address of the aperture.
  @param[in] Length       Length of the aperture.
  @param[in] Capabilities Capabilities required by the aperture.
  @param[in] Descriptor   The descriptor to ensure compatibility with the
                          aperture for.

  @retval EFI_SUCCESS            The descriptor is compatible. The GCD memory
                                 space map may have been updated, for
                                 continuity within the aperture.
  @retval EFI_INVALID_PARAMETER  The descriptor is incompatible.
  @return                        Error codes from gDS->AddMemorySpace().
**/
EFI_STATUS
IntersectMemoryDescriptor (
  IN  UINT64                                 Base,
  IN  UINT64                                 Length,
  IN  UINT64                                 Capabilities,
  IN  CONST EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
  )
{
  UINT64      IntersectionBase;
  UINT64      IntersectionEnd;
  EFI_STATUS  Status;

  if ((Descriptor->GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo) &&
      ((Descriptor->Capabilities & Capabilities) == Capabilities))
  {
    return EFI_SUCCESS;
  }

  IntersectionBase = MAX (Base, Descriptor->BaseAddress);
  IntersectionEnd  = MIN (
                       Base + Length,
                       Descriptor->BaseAddress + Descriptor->Length
                       );
  if (IntersectionBase >= IntersectionEnd) {
    //
    // The descriptor and the aperture don't overlap.
    //
    return EFI_SUCCESS;
  }

  if (Descriptor->GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    IntersectionBase,
                    IntersectionEnd - IntersectionBase,
                    Capabilities
                    );

    DEBUG ((
      EFI_ERROR (Status) ? DEBUG_ERROR : DEBUG_VERBOSE,
      "%a: %a: add [%Lx, %Lx): %r\n",
      gEfiCallerBaseName,
      __func__,
      IntersectionBase,
      IntersectionEnd,
      Status
      ));
    return Status;
  }

  DEBUG ((
    DEBUG_ERROR,
    "%a: %a: desc [%Lx, %Lx) type %u cap %Lx conflicts "
    "with aperture [%Lx, %Lx) cap %Lx\n",
    gEfiCallerBaseName,
    __func__,
    Descriptor->BaseAddress,
    Descriptor->BaseAddress + Descriptor->Length,
    (UINT32)Descriptor->GcdMemoryType,
    Descriptor->Capabilities,
    Base,
    Base + Length,
    Capabilities
    ));
  return EFI_INVALID_PARAMETER;
}

/**
  Add MMIO space to GCD.
  The routine checks the GCD database and only adds those which are
  not added in the specified range to GCD.

  @param Base         Base address of the MMIO space.
  @param Length       Length of the MMIO space.
  @param Capabilities Capabilities of the MMIO space.

  @retval EFI_SUCCESS The MMIO space was added successfully.
**/
EFI_STATUS
AddMemoryMappedIoSpace (
  IN  UINT64  Base,
  IN  UINT64  Length,
  IN  UINT64  Capabilities
  )
{
  EFI_STATUS                       Status;
  UINTN                            Index;
  UINTN                            NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *MemorySpaceMap;

  Status = gDS->GetMemorySpaceMap (&NumberOfDescriptors, &MemorySpaceMap);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: %a: GetMemorySpaceMap(): %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
    return Status;
  }

  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    Status = IntersectMemoryDescriptor (
               Base,
               Length,
               Capabilities,
               &MemorySpaceMap[Index]
               );
    if (EFI_ERROR (Status)) {
      goto FreeMemorySpaceMap;
    }
  }

  DEBUG_CODE_BEGIN ();
  //
  // Make sure there are adjacent descriptors covering [Base, Base + Length).
  // It is possible that they have not been merged; merging can be prevented
  // by allocation and different capabilities.
  //
  UINT64                           CheckBase;
  EFI_STATUS                       CheckStatus;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR  Descriptor;

  for (CheckBase = Base;
       CheckBase < Base + Length;
       CheckBase = Descriptor.BaseAddress + Descriptor.Length)
  {
    CheckStatus = gDS->GetMemorySpaceDescriptor (CheckBase, &Descriptor);
    ASSERT_EFI_ERROR (CheckStatus);
    ASSERT (Descriptor.GcdMemoryType == EfiGcdMemoryTypeMemoryMappedIo);
    ASSERT ((Descriptor.Capabilities & Capabilities) == Capabilities);
  }

  DEBUG_CODE_END ();

FreeMemorySpaceMap:
  FreePool (MemorySpaceMap);

  return Status;
}

/**
  Add and allocate CPU local APIC memory mapped space.

  @param[in]ImageHandle     Image handle this driver.

**/
VOID
AddLocalApicMemorySpace (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  BaseAddress;

  BaseAddress = (EFI_PHYSICAL_ADDRESS)GetLocalApicBaseAddress ();
  Status      = AddMemoryMappedIoSpace (BaseAddress, SIZE_4KB, EFI_MEMORY_UC);
  ASSERT_EFI_ERROR (Status);

  //
  // Try to allocate APIC memory mapped space, does not check return
  // status because it may be allocated by other driver, or DXE Core if
  // this range is built into Memory Allocation HOB.
  //
  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  0,
                  SIZE_4KB,
                  &BaseAddress,
                  ImageHandle,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_INFO,
      "%a: %a: AllocateMemorySpace() Status - %r\n",
      gEfiCallerBaseName,
      __func__,
      Status
      ));
  }
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
  EFI_STATUS  Status;
  EFI_EVENT   IdleLoopEvent;

  InitializePageTableLib ();

  InitializeFloatingPointUnits ();

  //
  // Make sure interrupts are disabled
  //
  DisableInterrupts ();

  //
  // Init GDT for DXE
  //
  InitGlobalDescriptorTable ();

  //
  // Setup IDT pointer, IDT and interrupt entry points
  //
  InitInterruptDescriptorTable ();

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

  //
  // Install EFI memory attribute Protocol
  //
  InstallEfiMemoryAttributeProtocol (mCpuHandle);

  //
  // Refresh GCD memory space map according to MTRR value.
  //
  RefreshGcdMemoryAttributes ();

  //
  // Add and allocate local APIC memory mapped space
  //
  AddLocalApicMemorySpace (ImageHandle);

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
