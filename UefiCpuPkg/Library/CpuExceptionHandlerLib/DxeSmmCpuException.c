/** @file
  CPU Exception Library provides DXE/SMM CPU common exception handler.

Copyright (c) 2012 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuExceptionCommon.h"
#include <Library/DebugLib.h>

//
// Spinlock for CPU information display
//
SPIN_LOCK        mDisplayMessageSpinLock;

//
// Image align size for DXE/SMM
//
CONST UINTN      mImageAlignSize = SIZE_4KB;

RESERVED_VECTORS_DATA       mReservedVectorsData[CPU_EXCEPTION_NUM];
EFI_CPU_INTERRUPT_HANDLER   mExternalInterruptHandlerTable[CPU_EXCEPTION_NUM];
EFI_CPU_INTERRUPT_HANDLER   *mExternalInterruptHandler = NULL;
UINTN                       mEnabledInterruptNum = 0;

/**
  Common exception handler.

  @param ExceptionType  Exception type.
  @param SystemContext  Pointer to EFI_SYSTEM_CONTEXT.
**/
VOID
EFIAPI
CommonExceptionHandler (
  IN EFI_EXCEPTION_TYPE          ExceptionType, 
  IN EFI_SYSTEM_CONTEXT          SystemContext
  )
{
  EXCEPTION_HANDLER_CONTEXT      *ExceptionHandlerContext;

  ExceptionHandlerContext = (EXCEPTION_HANDLER_CONTEXT *) (UINTN) (SystemContext.SystemContextIa32);

  switch (mReservedVectors[ExceptionType].Attribute) {
  case EFI_VECTOR_HANDOFF_HOOK_BEFORE:
    //
    // Need to jmp to old IDT handler after this exception handler
    //
    ExceptionHandlerContext->ExceptionDataFlag = (mErrorCodeFlag & (1 << ExceptionType)) ? TRUE : FALSE;
    ExceptionHandlerContext->OldIdtHandler     = mReservedVectors[ExceptionType].ExceptonHandler;
    break;
  case EFI_VECTOR_HANDOFF_HOOK_AFTER:
    while (TRUE) {
      //
      // If if anyone has gotten SPIN_LOCK for owner running hook after
      //
      if (AcquireSpinLockOrFail (&mReservedVectors[ExceptionType].SpinLock)) {
        //
        // Need to execute old IDT handler before running this exception handler
        //
        mReservedVectors[ExceptionType].ApicId = GetApicId ();
        ArchSaveExceptionContext (ExceptionType, SystemContext);
        ExceptionHandlerContext->ExceptionDataFlag = (mErrorCodeFlag & (1 << ExceptionType)) ? TRUE : FALSE;
        ExceptionHandlerContext->OldIdtHandler     = mReservedVectors[ExceptionType].ExceptonHandler;
        return;
      }
      //
      // If failed to acquire SPIN_LOCK, check if it was locked by processor itself
      //
      if (mReservedVectors[ExceptionType].ApicId == GetApicId ()) {
        //
        // Old IDT handler has been executed, then retore CPU exception content to
        // run new exception handler.
        //
        ArchRestoreExceptionContext (ExceptionType, SystemContext);
        //
        // Rlease spin lock for ApicId
        //
        ReleaseSpinLock (&mReservedVectors[ExceptionType].SpinLock);
        break;
      }
      CpuPause ();
    }
    break;
  case 0xffffffff:
    break;
  default:
    //
    // It should never reach here
    //
    CpuDeadLoop ();
    break;
  }
  
  if (mExternalInterruptHandler[ExceptionType] != NULL) {
    (mExternalInterruptHandler[ExceptionType]) (ExceptionType, SystemContext);
  } else if (ExceptionType < CPU_EXCEPTION_NUM) {
    //
    // Get Spinlock to display CPU information
    //
    while (!AcquireSpinLockOrFail (&mDisplayMessageSpinLock)) {
      CpuPause ();
    }
    //
    // Display ExceptionType, CPU information and Image information
    //  
    DumpCpuContent (ExceptionType, SystemContext);
    //
    // Release Spinlock of output message
    //
    ReleaseSpinLock (&mDisplayMessageSpinLock);
    //
    // Enter a dead loop if needn't to execute old IDT handler further
    //
    if (mReservedVectors[ExceptionType].Attribute != EFI_VECTOR_HANDOFF_HOOK_BEFORE) {
      CpuDeadLoop ();
    }
  }
}

/**
  Internal worker function to update IDT entries accordling to vector attributes.

  @param[in] IdtTable       Pointer to IDT table.
  @param[in] TemplateMap    Pointer to a buffer where the address map is returned.
  @param[in] IdtEntryCount  IDT entries number to be updated.

**/
VOID
UpdateIdtTable (
  IN IA32_IDT_GATE_DESCRIPTOR        *IdtTable,
  IN EXCEPTION_HANDLER_TEMPLATE_MAP  *TemplateMap,
  IN UINTN                           IdtEntryCount
  )
{
  UINT16                             CodeSegment;
  UINTN                              Index;
  UINTN                              InterruptHandler;

  //
  // Use current CS as the segment selector of interrupt gate in IDT
  //
  CodeSegment = AsmReadCs ();

  for (Index = 0; Index < IdtEntryCount; Index ++) {
    IdtTable[Index].Bits.Selector = CodeSegment;
    //
    // Check reserved vectors attributes
    //
    switch (mReservedVectors[Index].Attribute) {
    case EFI_VECTOR_HANDOFF_DO_NOT_HOOK:
      //
      // Keep original IDT entry
      //
      continue;
    case EFI_VECTOR_HANDOFF_HOOK_AFTER:
      InitializeSpinLock (&mReservedVectors[Index].SpinLock);
      CopyMem (
        (VOID *) mReservedVectors[Index].HookAfterStubHeaderCode,
        (VOID *) TemplateMap->HookAfterStubHeaderStart,
        TemplateMap->ExceptionStubHeaderSize
        );
      AsmVectorNumFixup (
        (VOID *) mReservedVectors[Index].HookAfterStubHeaderCode,
        (UINT8) Index,
        (VOID *) TemplateMap->HookAfterStubHeaderStart
        );
      //
      // Go on the following code
      //
    case EFI_VECTOR_HANDOFF_HOOK_BEFORE:
      //
      // Save original IDT handler address
      //
      mReservedVectors[Index].ExceptonHandler = ArchGetIdtHandler (&IdtTable[Index]);
      //
      // Go on the following code
      //
    default:
      //
      // Update new IDT entry
      //
      InterruptHandler = TemplateMap->ExceptionStart + Index * TemplateMap->ExceptionStubHeaderSize;
      ArchUpdateIdtEntry (&IdtTable[Index], InterruptHandler);
      break;
    }
  }
 
  //
  // Save Interrupt number to global variable used for RegisterCpuInterruptHandler ()
  //
  mEnabledInterruptNum = IdtEntryCount;
}

/**
  Internal worker function to initialize exception handler.

  @param[in]  VectorInfo    Pointer to reserved vector list.
  
  @retval EFI_SUCCESS           CPU Exception Entries have been successfully initialized 
                                with default exception handlers.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.
  @retval EFI_UNSUPPORTED       This function is not supported.

**/
EFI_STATUS
InitializeCpuExceptionHandlersWorker (
  IN EFI_VECTOR_HANDOFF_INFO       *VectorInfo OPTIONAL
  )
{
  EFI_STATUS                       Status;
  IA32_DESCRIPTOR                  IdtDescriptor;
  UINTN                            IdtEntryCount;
  EXCEPTION_HANDLER_TEMPLATE_MAP   TemplateMap;
  IA32_IDT_GATE_DESCRIPTOR         *IdtTable;

  mReservedVectors = mReservedVectorsData;
  SetMem ((VOID *) mReservedVectors, sizeof (RESERVED_VECTORS_DATA) * CPU_EXCEPTION_NUM, 0xff);
  if (VectorInfo != NULL) {
    Status = ReadAndVerifyVectorInfo (VectorInfo, mReservedVectors, CPU_EXCEPTION_NUM);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }
  }
  InitializeSpinLock (&mDisplayMessageSpinLock);

  mExternalInterruptHandler = mExternalInterruptHandlerTable;
  //
  // Read IDT descriptor and calculate IDT size
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtEntryCount = (IdtDescriptor.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR);
  if (IdtEntryCount > CPU_EXCEPTION_NUM) {
    //
    // CPU exeption library only setup CPU_EXCEPTION_NUM exception handler at most
    //
    IdtEntryCount = CPU_EXCEPTION_NUM;
  }

  IdtTable = (IA32_IDT_GATE_DESCRIPTOR *) IdtDescriptor.Base;
  AsmGetTemplateAddressMap (&TemplateMap);
  ASSERT (TemplateMap.ExceptionStubHeaderSize <= HOOKAFTER_STUB_SIZE);
  UpdateIdtTable (IdtTable, &TemplateMap, IdtEntryCount);
  mEnabledInterruptNum = IdtEntryCount;
  return EFI_SUCCESS;
}

/**
  Registers a function to be called from the processor interrupt handler.

  @param[in]  InterruptType     Defines which interrupt or exception to hook.
  @param[in]  InterruptHandler  A pointer to a function of type EFI_CPU_INTERRUPT_HANDLER that is called
                                when a processor interrupt occurs. If this parameter is NULL, then the handler
                                will be uninstalled.

  @retval EFI_SUCCESS           The handler for the processor interrupt was successfully installed or uninstalled.
  @retval EFI_ALREADY_STARTED   InterruptHandler is not NULL, and a handler for InterruptType was
                                previously installed.
  @retval EFI_INVALID_PARAMETER InterruptHandler is NULL, and a handler for InterruptType was not
                                previously installed.
  @retval EFI_UNSUPPORTED       The interrupt specified by InterruptType is not supported,
                                or this function is not supported.
**/
EFI_STATUS
RegisterCpuInterruptHandlerWorker (
  IN EFI_EXCEPTION_TYPE            InterruptType,
  IN EFI_CPU_INTERRUPT_HANDLER     InterruptHandler
  )
{
  if (InterruptType < 0 || InterruptType >= (EFI_EXCEPTION_TYPE)mEnabledInterruptNum ||
      mReservedVectors[InterruptType].Attribute == EFI_VECTOR_HANDOFF_DO_NOT_HOOK) {
    return EFI_UNSUPPORTED;
  }

  if (InterruptHandler == NULL && mExternalInterruptHandler[InterruptType] == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (InterruptHandler != NULL && mExternalInterruptHandler[InterruptType] != NULL) {
    return EFI_ALREADY_STARTED;
  }

  mExternalInterruptHandler[InterruptType] = InterruptHandler;
  return EFI_SUCCESS;
}

