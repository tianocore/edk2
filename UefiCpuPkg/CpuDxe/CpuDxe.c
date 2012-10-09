/** @file
  CPU DXE Module.

  Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuDxe.h"

//
// Global Variables
//
IA32_IDT_GATE_DESCRIPTOR  gIdtTable[INTERRUPT_VECTOR_NUMBER] = { 0 };

EFI_CPU_INTERRUPT_HANDLER ExternalVectorTable[0x100];
BOOLEAN                   InterruptState = FALSE;
EFI_HANDLE                mCpuHandle = NULL;
BOOLEAN                   mIsFlushingGCD;
UINT64                    mValidMtrrAddressMask = MTRR_LIB_CACHE_VALID_ADDRESS;
UINT64                    mValidMtrrBitsMask    = MTRR_LIB_MSR_VALID_MASK;
IA32_IDT_GATE_DESCRIPTOR  *mOrigIdtEntry        = NULL;
UINT16                    mOrigIdtEntryCount    = 0;

FIXED_MTRR    mFixedMtrrTable[] = {
  {
    MTRR_LIB_IA32_MTRR_FIX64K_00000,
    0,
    0x10000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_80000,
    0x80000,
    0x4000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX16K_A0000,
    0xA0000,
    0x4000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C0000,
    0xC0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_C8000,
    0xC8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D0000,
    0xD0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_D8000,
    0xD8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E0000,
    0xE0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_E8000,
    0xE8000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F0000,
    0xF0000,
    0x1000
  },
  {
    MTRR_LIB_IA32_MTRR_FIX4K_F8000,
    0xF8000,
    0x1000
  },
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
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
// bit 0 - exception 0
// bit 1 - exception 1
// etc.
//
UINT32 mErrorCodeFlag = 0x00027d00;

//
// Local function prototypes
//

/**
  Set Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.
  @param Handler      Handler address.

**/
VOID
SetInterruptDescriptorTableHandlerAddress (
  IN UINTN Index,
  IN VOID  *Handler  OPTIONAL
  );

//
// CPU Arch Protocol Functions
//


/**
  Common exception handler.

  @param  InterruptType  Exception type
  @param  SystemContext  EFI_SYSTEM_CONTEXT

**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
#if defined (MDE_CPU_IA32)
  DEBUG ((
    EFI_D_ERROR,
    "!!!! IA32 Exception Type - %08x !!!!\n",
    InterruptType
    ));
  if ((mErrorCodeFlag & (1 << InterruptType)) != 0) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %08x\n",
      SystemContext.SystemContextIa32->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "CS  - %04x,     EIP - %08x, EFL - %08x, SS  - %04x\n",
    SystemContext.SystemContextIa32->Cs,
    SystemContext.SystemContextIa32->Eip,
    SystemContext.SystemContextIa32->Eflags,
    SystemContext.SystemContextIa32->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DS  - %04x,     ES  - %04x,     FS  - %04x,     GS  - %04x\n",
    SystemContext.SystemContextIa32->Ds,
    SystemContext.SystemContextIa32->Es,
    SystemContext.SystemContextIa32->Fs,
    SystemContext.SystemContextIa32->Gs
    ));
  DEBUG ((
    EFI_D_ERROR,
    "EAX - %08x, EBX - %08x, ECX - %08x, EDX - %08x\n",
    SystemContext.SystemContextIa32->Eax,
    SystemContext.SystemContextIa32->Ebx,
    SystemContext.SystemContextIa32->Ecx,
    SystemContext.SystemContextIa32->Edx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "ESP - %08x, EBP - %08x, ESI - %08x, EDI - %08x\n",
    SystemContext.SystemContextIa32->Esp,
    SystemContext.SystemContextIa32->Ebp,
    SystemContext.SystemContextIa32->Esi,
    SystemContext.SystemContextIa32->Edi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDT - %08x  LIM - %04x,     IDT - %08x  LIM - %04x\n",
    SystemContext.SystemContextIa32->Gdtr[0],
    SystemContext.SystemContextIa32->Gdtr[1],
    SystemContext.SystemContextIa32->Idtr[0],
    SystemContext.SystemContextIa32->Idtr[1]
    ));
  DEBUG ((
    EFI_D_ERROR,
    "LDT - %08x, TR  - %08x\n",
    SystemContext.SystemContextIa32->Ldtr,
    SystemContext.SystemContextIa32->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %08x, CR2 - %08x, CR3 - %08x, CR4 - %08x\n",
    SystemContext.SystemContextIa32->Cr0,
    SystemContext.SystemContextIa32->Cr2,
    SystemContext.SystemContextIa32->Cr3,
    SystemContext.SystemContextIa32->Cr4
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %08x, DR1 - %08x, DR2 - %08x, DR3 - %08x\n",
    SystemContext.SystemContextIa32->Dr0,
    SystemContext.SystemContextIa32->Dr1,
    SystemContext.SystemContextIa32->Dr2,
    SystemContext.SystemContextIa32->Dr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR6 - %08x, DR7 - %08x\n",
    SystemContext.SystemContextIa32->Dr6,
    SystemContext.SystemContextIa32->Dr7
    ));
#elif defined (MDE_CPU_X64)
  DEBUG ((
    EFI_D_ERROR,
    "!!!! X64 Exception Type - %016lx !!!!\n",
    (UINT64)InterruptType
    ));
  if ((mErrorCodeFlag & (1 << InterruptType)) != 0) {
    DEBUG ((
      EFI_D_ERROR,
      "ExceptionData - %016lx\n",
      SystemContext.SystemContextX64->ExceptionData
      ));
  }
  DEBUG ((
    EFI_D_ERROR,
    "RIP - %016lx, RFL - %016lx\n",
    SystemContext.SystemContextX64->Rip,
    SystemContext.SystemContextX64->Rflags
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RAX - %016lx, RCX - %016lx, RDX - %016lx\n",
    SystemContext.SystemContextX64->Rax,
    SystemContext.SystemContextX64->Rcx,
    SystemContext.SystemContextX64->Rdx
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RBX - %016lx, RSP - %016lx, RBP - %016lx\n",
    SystemContext.SystemContextX64->Rbx,
    SystemContext.SystemContextX64->Rsp,
    SystemContext.SystemContextX64->Rbp
    ));
  DEBUG ((
    EFI_D_ERROR,
    "RSI - %016lx, RDI - %016lx\n",
    SystemContext.SystemContextX64->Rsi,
    SystemContext.SystemContextX64->Rdi
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R8  - %016lx, R9  - %016lx, R10 - %016lx\n",
    SystemContext.SystemContextX64->R8,
    SystemContext.SystemContextX64->R9,
    SystemContext.SystemContextX64->R10
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R11 - %016lx, R12 - %016lx, R13 - %016lx\n",
    SystemContext.SystemContextX64->R11,
    SystemContext.SystemContextX64->R12,
    SystemContext.SystemContextX64->R13
    ));
  DEBUG ((
    EFI_D_ERROR,
    "R14 - %016lx, R15 - %016lx\n",
    SystemContext.SystemContextX64->R14,
    SystemContext.SystemContextX64->R15
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CS  - %04lx, DS  - %04lx, ES  - %04lx, FS  - %04lx, GS  - %04lx, SS  - %04lx\n",
    SystemContext.SystemContextX64->Cs,
    SystemContext.SystemContextX64->Ds,
    SystemContext.SystemContextX64->Es,
    SystemContext.SystemContextX64->Fs,
    SystemContext.SystemContextX64->Gs,
    SystemContext.SystemContextX64->Ss
    ));
  DEBUG ((
    EFI_D_ERROR,
    "GDT - %016lx; %04lx,                   IDT - %016lx; %04lx\n",
    SystemContext.SystemContextX64->Gdtr[0],
    SystemContext.SystemContextX64->Gdtr[1],
    SystemContext.SystemContextX64->Idtr[0],
    SystemContext.SystemContextX64->Idtr[1]
    ));
  DEBUG ((
    EFI_D_ERROR,
    "LDT - %016lx, TR  - %016lx\n",
    SystemContext.SystemContextX64->Ldtr,
    SystemContext.SystemContextX64->Tr
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR0 - %016lx, CR2 - %016lx, CR3 - %016lx\n",
    SystemContext.SystemContextX64->Cr0,
    SystemContext.SystemContextX64->Cr2,
    SystemContext.SystemContextX64->Cr3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "CR4 - %016lx, CR8 - %016lx\n",
    SystemContext.SystemContextX64->Cr4,
    SystemContext.SystemContextX64->Cr8
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR0 - %016lx, DR1 - %016lx, DR2 - %016lx\n",
    SystemContext.SystemContextX64->Dr0,
    SystemContext.SystemContextX64->Dr1,
    SystemContext.SystemContextX64->Dr2
    ));
  DEBUG ((
    EFI_D_ERROR,
    "DR3 - %016lx, DR6 - %016lx, DR7 - %016lx\n",
    SystemContext.SystemContextX64->Dr3,
    SystemContext.SystemContextX64->Dr6,
    SystemContext.SystemContextX64->Dr7
    ));
#else
#error CPU type not supported for exception information dump!
#endif

  //
  // Hang the system with CpuSleep so the processor will enter a lower power
  // state.
  //
  while (TRUE) {
    CpuSleep ();
  };
}


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
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      Start,
  IN UINT64                    Length,
  IN EFI_CPU_FLUSH_TYPE        FlushType
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
  IN EFI_CPU_ARCH_PROTOCOL          *This
  )
{
  EnableInterrupts ();

  InterruptState = TRUE;
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
  IN EFI_CPU_ARCH_PROTOCOL     *This
  )
{
  DisableInterrupts ();

  InterruptState = FALSE;
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
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  OUT BOOLEAN                   *State
  )
{
  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *State = InterruptState;
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
  IN EFI_CPU_ARCH_PROTOCOL      *This,
  IN EFI_CPU_INIT_TYPE          InitType
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
  IN EFI_CPU_ARCH_PROTOCOL         *This,
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  )
{
  if (InterruptType < 0 || InterruptType > 0xff) {
    return EFI_UNSUPPORTED;
  }

  if (InterruptHandler == NULL && ExternalVectorTable[InterruptType] == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptHandler != NULL && ExternalVectorTable[InterruptType] != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (InterruptHandler != NULL) {
    SetInterruptDescriptorTableHandlerAddress ((UINTN)InterruptType, NULL);
  } else {
    //
    // Restore the original IDT handler address if InterruptHandler is NULL.
    //
    RestoreInterruptDescriptorTableHandlerAddress ((UINTN)InterruptType);
  }

  ExternalVectorTable[InterruptType] = InterruptHandler;
  return EFI_SUCCESS;
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
  IN  EFI_CPU_ARCH_PROTOCOL     *This,
  IN  UINT32                    TimerIndex,
  OUT UINT64                    *TimerValue,
  OUT UINT64                    *TimerPeriod OPTIONAL
  )
{
  if (TimerValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (TimerIndex != 0) {
    return EFI_INVALID_PARAMETER;
  }

  *TimerValue = AsmReadTsc ();

  if (TimerPeriod != NULL) {
      //
      // BugBug: Hard coded. Don't know how to do this generically
      //
      *TimerPeriod = 1000000000;
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
  IN EFI_CPU_ARCH_PROTOCOL     *This,
  IN EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN UINT64                    Length,
  IN UINT64                    Attributes
  )
{
  RETURN_STATUS             Status;
  MTRR_MEMORY_CACHE_TYPE    CacheType;

  if (!IsMtrrSupported ()) {
    return EFI_UNSUPPORTED;
  }

  //
  // If this function is called because GCD SetMemorySpaceAttributes () is called
  // by RefreshGcdMemoryAttributes (), then we are just synchronzing GCD memory
  // map with MTRR values. So there is no need to modify MTRRs, just return immediately
  // to avoid unnecessary computing.
  //
  if (mIsFlushingGCD) {
    DEBUG((EFI_D_ERROR, "  Flushing GCD\n"));
      return EFI_SUCCESS;
    }

  switch (Attributes) {
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

  case EFI_MEMORY_UCE:
  case EFI_MEMORY_RP:
  case EFI_MEMORY_XP:
  case EFI_MEMORY_RUNTIME:
    return EFI_UNSUPPORTED;

  default:
    return EFI_INVALID_PARAMETER;
  }
  //
  // call MTRR libary function
  //
  Status = MtrrSetMemoryAttribute (
             BaseAddress,
             Length,
             CacheType
             );

  return (EFI_STATUS) Status;
}

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

**/
VOID
InitializeMtrrMask (
  VOID
  )
{
  UINT32                              RegEax;
  UINT8                               PhysicalAddressBits;

  AsmCpuid (0x80000000, &RegEax, NULL, NULL, NULL);

  if (RegEax >= 0x80000008) {
    AsmCpuid (0x80000008, &RegEax, NULL, NULL, NULL);

    PhysicalAddressBits = (UINT8) RegEax;

    mValidMtrrBitsMask    = LShiftU64 (1, PhysicalAddressBits) - 1;
    mValidMtrrAddressMask = mValidMtrrBitsMask & 0xfffffffffffff000ULL;
  } else {
    mValidMtrrBitsMask    = MTRR_LIB_MSR_VALID_MASK;
    mValidMtrrAddressMask = MTRR_LIB_CACHE_VALID_ADDRESS;
  }
}

/**
  Gets GCD Mem Space type from MTRR Type.

  This function gets GCD Mem Space type from MTRR Type.

  @param  MtrrAttributes  MTRR memory type

  @return GCD Mem Space type

**/
UINT64
GetMemorySpaceAttributeFromMtrrType (
  IN UINT8                MtrrAttributes
  )
{
  switch (MtrrAttributes) {
  case MTRR_CACHE_UNCACHEABLE:
    return EFI_MEMORY_UC;
  case MTRR_CACHE_WRITE_COMBINING:
    return EFI_MEMORY_WC;
  case MTRR_CACHE_WRITE_THROUGH:
    return EFI_MEMORY_WT;
  case MTRR_CACHE_WRITE_PROTECTED:
    return EFI_MEMORY_WP;
  case MTRR_CACHE_WRITE_BACK:
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
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  OUT UINTN                              *StartIndex,
  OUT UINTN                              *EndIndex
  )
{
  UINTN           Index;

  *StartIndex = 0;
  *EndIndex   = 0;
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (BaseAddress >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
      *StartIndex = Index;
    }
    if (BaseAddress + Length - 1 >= MemorySpaceMap[Index].BaseAddress &&
        BaseAddress + Length - 1 < MemorySpaceMap[Index].BaseAddress + MemorySpaceMap[Index].Length) {
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
  IN EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap,
  IN UINTN                               NumberOfDescriptors,
  IN EFI_PHYSICAL_ADDRESS                BaseAddress,
  IN UINT64                              Length,
  IN UINT64                              Attributes
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
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) | (MemorySpaceMap[Index].Capabilities & Attributes)
           );
  }

  return EFI_SUCCESS;
}


/**
  Refreshes the GCD Memory Space attributes according to MTRRs.

  This function refreshes the GCD Memory Space attributes according to MTRRs.

**/
VOID
RefreshGcdMemoryAttributes (
  VOID
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  UINTN                               SubIndex;
  UINT64                              RegValue;
  EFI_PHYSICAL_ADDRESS                BaseAddress;
  UINT64                              Length;
  UINT64                              Attributes;
  UINT64                              CurrentAttributes;
  UINT8                               MtrrType;
  UINTN                               NumberOfDescriptors;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR     *MemorySpaceMap;
  UINT64                              DefaultAttributes;
  VARIABLE_MTRR                       VariableMtrr[MTRR_NUMBER_OF_VARIABLE_MTRR];
  MTRR_FIXED_SETTINGS                 MtrrFixedSettings;
  UINT32                              FirmwareVariableMtrrCount;
  UINT8                               DefaultMemoryType;

  if (!IsMtrrSupported ()) {
    return;
  }

  FirmwareVariableMtrrCount = GetFirmwareVariableMtrrCount ();
  ASSERT (FirmwareVariableMtrrCount <= MTRR_NUMBER_OF_VARIABLE_MTRR);

  mIsFlushingGCD = TRUE;
  MemorySpaceMap = NULL;

  //
  // Initialize the valid bits mask and valid address mask for MTRRs
  //
  InitializeMtrrMask ();

  //
  // Get the memory attribute of variable MTRRs
  //
  MtrrGetMemoryAttributeInVariableMtrr (
    mValidMtrrBitsMask,
    mValidMtrrAddressMask,
    VariableMtrr
    );

  //
  // Get the memory space map from GCD
  //
  Status = gDS->GetMemorySpaceMap (
                  &NumberOfDescriptors,
                  &MemorySpaceMap
                  );
  ASSERT_EFI_ERROR (Status);

  DefaultMemoryType = (UINT8) MtrrGetDefaultMemoryType ();
  DefaultAttributes = GetMemorySpaceAttributeFromMtrrType (DefaultMemoryType);

  //
  // Set default attributes to all spaces.
  //
  for (Index = 0; Index < NumberOfDescriptors; Index++) {
    if (MemorySpaceMap[Index].GcdMemoryType == EfiGcdMemoryTypeNonExistent) {
      continue;
    }
    gDS->SetMemorySpaceAttributes (
           MemorySpaceMap[Index].BaseAddress,
           MemorySpaceMap[Index].Length,
           (MemorySpaceMap[Index].Attributes & ~EFI_MEMORY_CACHETYPE_MASK) |
           (MemorySpaceMap[Index].Capabilities & DefaultAttributes)
           );
  }

  //
  // Go for variable MTRRs with WB attribute
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&
        VariableMtrr[Index].Type == MTRR_CACHE_WRITE_BACK) {
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        EFI_MEMORY_WB
        );
    }
  }

  //
  // Go for variable MTRRs with the attribute except for WB and UC attributes
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&                          
        VariableMtrr[Index].Type != MTRR_CACHE_WRITE_BACK &&
        VariableMtrr[Index].Type != MTRR_CACHE_UNCACHEABLE) {
      Attributes = GetMemorySpaceAttributeFromMtrrType ((UINT8) VariableMtrr[Index].Type);
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        Attributes
        );
    }
  }

  //
  // Go for variable MTRRs with UC attribute
  //
  for (Index = 0; Index < FirmwareVariableMtrrCount; Index++) {
    if (VariableMtrr[Index].Valid &&
        VariableMtrr[Index].Type == MTRR_CACHE_UNCACHEABLE) {
      SetGcdMemorySpaceAttributes (
        MemorySpaceMap,
        NumberOfDescriptors,
        VariableMtrr[Index].BaseAddress,
        VariableMtrr[Index].Length,
        EFI_MEMORY_UC
        );
    }
  }

  //
  // Go for fixed MTRRs
  //
  Attributes  = 0;
  BaseAddress = 0;
  Length      = 0;
  MtrrGetFixedMtrr (&MtrrFixedSettings);
  for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++) {
    RegValue = MtrrFixedSettings.Mtrr[Index];
    //
    // Check for continuous fixed MTRR sections
    //
    for (SubIndex = 0; SubIndex < 8; SubIndex++) {
      MtrrType = (UINT8) RShiftU64 (RegValue, SubIndex * 8);
      CurrentAttributes = GetMemorySpaceAttributeFromMtrrType (MtrrType);
      if (Length == 0) {
        //
        // A new MTRR attribute begins
        //
        Attributes = CurrentAttributes;
      } else {
        //
        // If fixed MTRR attribute changed, then set memory attribute for previous atrribute
        //
        if (CurrentAttributes != Attributes) {
          SetGcdMemorySpaceAttributes (
            MemorySpaceMap,
            NumberOfDescriptors,
            BaseAddress,
            Length,
            Attributes
            );
          BaseAddress = mFixedMtrrTable[Index].BaseAddress + mFixedMtrrTable[Index].Length * SubIndex;
          Length = 0;
          Attributes = CurrentAttributes;
        }
      }
      Length += mFixedMtrrTable[Index].Length;
    }
  }
  //
  // Handle the last fixed MTRR region
  //
  SetGcdMemorySpaceAttributes (
    MemorySpaceMap,
    NumberOfDescriptors,
    BaseAddress,
    Length,
    Attributes
    );

  //
  // Free memory space map allocated by GCD service GetMemorySpaceMap ()
  //
  if (MemorySpaceMap != NULL) {
    FreePool (MemorySpaceMap);
  }

  mIsFlushingGCD = FALSE;
}

/**
  Set Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.
  @param Handler      Handler address.

**/
VOID
SetInterruptDescriptorTableHandlerAddress (
  IN UINTN Index,
  IN VOID  *Handler  OPTIONAL
  )
{
  UINTN                     UintnHandler;

  if (Handler != NULL) {
    UintnHandler = (UINTN) Handler;
  } else {
    UintnHandler = ((UINTN) AsmIdtVector00) + (8 * Index);
  }

  gIdtTable[Index].Bits.OffsetLow   = (UINT16)UintnHandler;
  gIdtTable[Index].Bits.Reserved_0  = 0;
  gIdtTable[Index].Bits.GateType    = IA32_IDT_GATE_TYPE_INTERRUPT_32;
  gIdtTable[Index].Bits.OffsetHigh  = (UINT16)(UintnHandler >> 16);
#if defined (MDE_CPU_X64)
  gIdtTable[Index].Bits.OffsetUpper = (UINT32)(UintnHandler >> 32);
  gIdtTable[Index].Bits.Reserved_1  = 0;
#endif
}

/**
  Restore original Interrupt Descriptor Table Handler Address.

  @param Index        The Index of the interrupt descriptor table handle.

**/
VOID
RestoreInterruptDescriptorTableHandlerAddress (
  IN UINTN       Index
  )
{
  if (Index < mOrigIdtEntryCount) {
    gIdtTable[Index].Bits.OffsetLow   = mOrigIdtEntry[Index].Bits.OffsetLow;
    gIdtTable[Index].Bits.OffsetHigh  = mOrigIdtEntry[Index].Bits.OffsetHigh;
#if defined (MDE_CPU_X64)
    gIdtTable[Index].Bits.OffsetUpper = mOrigIdtEntry[Index].Bits.OffsetUpper;
#endif
  }
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
  IA32_DESCRIPTOR           OldIdtPtr;
  IA32_IDT_GATE_DESCRIPTOR  *OldIdt;
  UINTN                     OldIdtSize;
  VOID                      *IdtPtrAlignmentBuffer;
  IA32_DESCRIPTOR           *IdtPtr;
  UINTN                     Index;
  UINT16                    CurrentCs;
  VOID                      *IntHandler;

  SetMem (ExternalVectorTable, sizeof(ExternalVectorTable), 0);

  //
  // Get original IDT address and size.
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &OldIdtPtr);

  if ((OldIdtPtr.Base != 0) && ((OldIdtPtr.Limit & 7) == 7)) {
    OldIdt = (IA32_IDT_GATE_DESCRIPTOR*) OldIdtPtr.Base;
    OldIdtSize = (OldIdtPtr.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR);
    //
    // Save original IDT entry and IDT entry count.
    //
    mOrigIdtEntry = AllocateCopyPool (OldIdtPtr.Limit + 1, (VOID *) OldIdtPtr.Base);
    ASSERT (mOrigIdtEntry != NULL);
    mOrigIdtEntryCount = (UINT16) OldIdtSize;
  } else {
    OldIdt = NULL;
    OldIdtSize = 0;
  }

  //
  // Intialize IDT
  //
  CurrentCs = AsmReadCs();
  for (Index = 0; Index < INTERRUPT_VECTOR_NUMBER; Index ++) {
    //
    // If the old IDT had a handler for this interrupt, then
    // preserve it.
    //
    if (Index < OldIdtSize) {
      IntHandler = 
        (VOID*) (
          OldIdt[Index].Bits.OffsetLow +
          (OldIdt[Index].Bits.OffsetHigh << 16)
#if defined (MDE_CPU_X64)
            + (((UINTN) OldIdt[Index].Bits.OffsetUpper) << 32)
#endif
          );
    } else {
      IntHandler = NULL;
    }

    gIdtTable[Index].Bits.Selector    = CurrentCs;
    gIdtTable[Index].Bits.Reserved_0  = 0;
    gIdtTable[Index].Bits.GateType    = IA32_IDT_GATE_TYPE_INTERRUPT_32;
    SetInterruptDescriptorTableHandlerAddress (Index, IntHandler);
  }

  //
  // Load IDT Pointer
  //
  IdtPtrAlignmentBuffer = AllocatePool (sizeof (*IdtPtr) + 16);
  IdtPtr = ALIGN_POINTER (IdtPtrAlignmentBuffer, 16);
  IdtPtr->Base = (UINT32)(((UINTN)(VOID*) gIdtTable) & (BASE_4GB-1));
  IdtPtr->Limit = (UINT16) (sizeof (gIdtTable) - 1);

  AsmWriteIdtr (IdtPtr);

  FreePool (IdtPtrAlignmentBuffer);

  //
  // Initialize Exception Handlers
  //
  for (Index = OldIdtSize; Index < 32; Index++) {
    Status = CpuRegisterInterruptHandler (&gCpu, Index, CommonExceptionHandler);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Set the pointer to the array of C based exception handling routines.
  //
  InitializeExternalVectorTablePtr (ExternalVectorTable);

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
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  CpuSleep ();
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
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   IdleLoopEvent;

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
                  &gEfiCpuArchProtocolGuid, &gCpu,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Refresh GCD memory space map according to MTRR value.
  //
  RefreshGcdMemoryAttributes ();

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

