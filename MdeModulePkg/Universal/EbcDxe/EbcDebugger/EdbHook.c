/** @file

Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "Edb.h"

/**

  Check the Hook flag, and trigger exception if match.

  @param  VmPtr        - EbcDebuggerCheckHookFlag
  @param  Flag         - Feature flag

**/
VOID
EbcDebuggerCheckHookFlag (
  IN VM_CONTEXT  *VmPtr,
  IN UINT32      Flag
  )
{
  if ((mDebuggerPrivate.FeatureFlags & Flag) == Flag) {
    mDebuggerPrivate.StatusFlags = Flag;
    EbcDebugSignalException (
      EXCEPT_EBC_BREAKPOINT,
      EXCEPTION_FLAG_NONE,
      VmPtr
      );
  }

  return;
}

/**

  It will record soruce address for Callstack entry.

  @param  SourceEntry  - Source address
  @param  Type         - Branch type

**/
VOID
EbcDebuggerPushCallstackSource (
  IN UINT64                    SourceEntry,
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  if (mDebuggerPrivate.CallStackEntryCount > EFI_DEBUGGER_CALLSTACK_MAX) {
    ASSERT (FALSE);
    mDebuggerPrivate.CallStackEntryCount = EFI_DEBUGGER_CALLSTACK_MAX;
  }

  //
  // Record the new callstack entry
  //
  mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].SourceAddress = SourceEntry;
  mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].Type          = Type;

  //
  // Do not change CallStackEntryCount
  //

  return;
}

/**

  It will record parameter for Callstack entry.

  @param  ParameterAddress - The address for the parameter
  @param  Type             - Branch type

**/
VOID
EbcDebuggerPushCallstackParameter (
  IN UINT64                    ParameterAddress,
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  if (mDebuggerPrivate.CallStackEntryCount > EFI_DEBUGGER_CALLSTACK_MAX) {
    ASSERT (FALSE);
    mDebuggerPrivate.CallStackEntryCount = EFI_DEBUGGER_CALLSTACK_MAX;
  }

  //
  // Record the new callstack parameter
  //
  mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].ParameterAddr = (UINTN)ParameterAddress;
  CopyMem (
    mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].Parameter,
    (VOID *)(UINTN)ParameterAddress,
    sizeof (mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].Parameter)
    );

  //
  // Do not change CallStackEntryCount
  //

  return;
}

/**

  It will record source address for callstack entry.

  @param  DestEntry    - Source address
  @param  Type         - Branch type

**/
VOID
EbcDebuggerPushCallstackDest (
  IN UINT64                    DestEntry,
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  UINTN  Index;

  if (mDebuggerPrivate.CallStackEntryCount < EFI_DEBUGGER_CALLSTACK_MAX) {
    //
    // If there is empty entry for callstack, add it
    //
    ASSERT (mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].Type == Type);
    mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].DestAddress = DestEntry;
    mDebuggerPrivate.CallStackEntryCount++;
  } else {
    //
    // If there is no empty entry for callstack, throw the oldest one
    //
    ASSERT (mDebuggerPrivate.CallStackEntry[EFI_DEBUGGER_TRACE_MAX].Type == Type);
    for (Index = 0; Index < EFI_DEBUGGER_CALLSTACK_MAX; Index++) {
      CopyMem (
        &mDebuggerPrivate.CallStackEntry[Index],
        &mDebuggerPrivate.CallStackEntry[Index + 1],
        sizeof (mDebuggerPrivate.CallStackEntry[Index])
        );
    }

    mDebuggerPrivate.CallStackEntry[EFI_DEBUGGER_CALLSTACK_MAX - 1].DestAddress = DestEntry;
    mDebuggerPrivate.CallStackEntryCount                                        = EFI_DEBUGGER_CALLSTACK_MAX;
  }

  return;
}

/**

  It will throw the newest Callstack entry.

**/
VOID
EbcDebuggerPopCallstack (
  VOID
  )
{
  if ((mDebuggerPrivate.CallStackEntryCount > 0) &&
      (mDebuggerPrivate.CallStackEntryCount <= EFI_DEBUGGER_CALLSTACK_MAX))
  {
    //
    // Throw the newest one
    //
    mDebuggerPrivate.CallStackEntryCount--;
    mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].SourceAddress = 0;
    mDebuggerPrivate.CallStackEntry[mDebuggerPrivate.CallStackEntryCount].DestAddress   = 0;
  } else if (mDebuggerPrivate.CallStackEntryCount == 0) {
    //
    // NOT assert here because it is reasonable, because when we start to build
    // callstack, we do not know how many function already called.
    //
  } else {
    ASSERT (FALSE);
  }

  return;
}

/**

  It will record source address for trace entry.

  @param  SourceEntry  - Source address
  @param  Type         - Branch type

**/
VOID
EbcDebuggerPushTraceSourceEntry (
  IN UINT64                    SourceEntry,
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  if (mDebuggerPrivate.TraceEntryCount > EFI_DEBUGGER_TRACE_MAX) {
    ASSERT (FALSE);
    mDebuggerPrivate.TraceEntryCount = EFI_DEBUGGER_TRACE_MAX;
  }

  //
  // Record the new trace entry
  //
  mDebuggerPrivate.TraceEntry[mDebuggerPrivate.TraceEntryCount].SourceAddress = SourceEntry;
  mDebuggerPrivate.TraceEntry[mDebuggerPrivate.TraceEntryCount].Type          = Type;

  //
  // Do not change TraceEntryCount
  //

  return;
}

/**

  It will record destination address for trace entry.

  @param  DestEntry    - Destination address
  @param  Type         - Branch type

**/
VOID
EbcDebuggerPushTraceDestEntry (
  IN UINT64                    DestEntry,
  IN EFI_DEBUGGER_BRANCH_TYPE  Type
  )
{
  UINTN  Index;

  if (mDebuggerPrivate.TraceEntryCount < EFI_DEBUGGER_TRACE_MAX) {
    //
    // If there is empty entry for trace, add it
    //
    ASSERT (mDebuggerPrivate.TraceEntry[mDebuggerPrivate.TraceEntryCount].Type == Type);
    mDebuggerPrivate.TraceEntry[mDebuggerPrivate.TraceEntryCount].DestAddress = DestEntry;
    mDebuggerPrivate.TraceEntryCount++;
  } else {
    //
    // If there is no empty entry for trace, throw the oldest one
    //
    ASSERT (mDebuggerPrivate.TraceEntry[EFI_DEBUGGER_TRACE_MAX].Type == Type);
    for (Index = 0; Index < EFI_DEBUGGER_TRACE_MAX; Index++) {
      CopyMem (
        &mDebuggerPrivate.TraceEntry[Index],
        &mDebuggerPrivate.TraceEntry[Index + 1],
        sizeof (mDebuggerPrivate.TraceEntry[Index])
        );
    }

    mDebuggerPrivate.TraceEntry[EFI_DEBUGGER_CALLSTACK_MAX - 1].DestAddress = DestEntry;
    mDebuggerPrivate.TraceEntryCount                                        = EFI_DEBUGGER_TRACE_MAX;
  }

  return;
}

/**

  It will record address for StepEntry, if STEPOVER or STEPOUT is enabled.

  @param  Entry    - Break Address
  @param  FramePtr - Break Frame pointer
  @param  Flag     - for STEPOVER or STEPOUT

**/
VOID
EbcDebuggerPushStepEntry (
  IN UINT64  Entry,
  IN UINT64  FramePtr,
  IN UINT32  Flag
  )
{
  //
  // Check StepOver
  //
  if ((Flag == EFI_DEBUG_FLAG_EBC_STEPOVER) &&
      ((mDebuggerPrivate.FeatureFlags & EFI_DEBUG_FLAG_EBC_STEPOVER) == EFI_DEBUG_FLAG_EBC_STEPOVER))
  {
    mDebuggerPrivate.StepContext.BreakAddress = Entry;
    mDebuggerPrivate.StepContext.FramePointer = FramePtr;
    mDebuggerPrivate.FeatureFlags            &= ~EFI_DEBUG_FLAG_EBC_B_STEPOVER;
  }

  //
  // Check StepOut
  //
  if ((Flag == EFI_DEBUG_FLAG_EBC_STEPOUT) &&
      ((mDebuggerPrivate.FeatureFlags & EFI_DEBUG_FLAG_EBC_STEPOUT) == EFI_DEBUG_FLAG_EBC_STEPOUT))
  {
    mDebuggerPrivate.StepContext.BreakAddress = Entry;
    mDebuggerPrivate.StepContext.FramePointer = FramePtr;
    mDebuggerPrivate.FeatureFlags            &= ~EFI_DEBUG_FLAG_EBC_B_STEPOUT;
  }
}

/**
  Notify the callback function when an event is triggered.

  @param  Event                    Indicates the event that invoke this function.
  @param  Context                  Indicates the calling context.

**/
VOID
EFIAPI
EbcDebuggerBreakEventFunc (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if ((mDebuggerPrivate.FeatureFlags & EFI_DEBUG_FLAG_EBC_BOK) != EFI_DEBUG_FLAG_EBC_BOK) {
    return;
  }

  Status = gBS->CheckEvent (gST->ConIn->WaitForKey);
  if (Status == EFI_SUCCESS) {
    mDebuggerPrivate.StatusFlags = EFI_DEBUG_FLAG_EBC_BOK;
  }
}

/**

  The hook in InitializeEbcDriver.
  It will init the EbcDebuggerPrivate data structure.

  @param Handle           - The EbcDebugProtocol handle.
  @param EbcDebugProtocol - The EbcDebugProtocol interface.

**/
VOID
EbcDebuggerHookInit (
  IN EFI_HANDLE                  Handle,
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol
  )
{
  EFI_STATUS                  Status;
  UINTN                       Index;
  EFI_DEBUGGER_SYMBOL_OBJECT  *Object;
  EFI_DEBUGGER_SYMBOL_ENTRY   *Entry;

  //
  // Register all exception handler
  //
  for (Index = EXCEPT_EBC_UNDEFINED; Index <= EXCEPT_EBC_STEP; Index++) {
    EbcDebugProtocol->RegisterExceptionCallback (
                        EbcDebugProtocol,
                        0,
                        NULL,
                        Index
                        );
    EbcDebugProtocol->RegisterExceptionCallback (
                        EbcDebugProtocol,
                        0,
                        EdbExceptionHandler,
                        Index
                        );
  }

  //
  // Init Symbol
  //
  Object = AllocateZeroPool (sizeof (EFI_DEBUGGER_SYMBOL_OBJECT) * EFI_DEBUGGER_SYMBOL_OBJECT_MAX);
  ASSERT (Object != NULL);
  mDebuggerPrivate.DebuggerSymbolContext.Object         = Object;
  mDebuggerPrivate.DebuggerSymbolContext.ObjectCount    = 0;
  mDebuggerPrivate.DebuggerSymbolContext.MaxObjectCount = EFI_DEBUGGER_SYMBOL_OBJECT_MAX;
  for (Index = 0; Index < EFI_DEBUGGER_SYMBOL_OBJECT_MAX; Index++) {
    Entry = AllocateZeroPool (sizeof (EFI_DEBUGGER_SYMBOL_ENTRY) * EFI_DEBUGGER_SYMBOL_ENTRY_MAX);
    ASSERT (Entry != NULL);
    Object[Index].Entry         = Entry;
    Object[Index].MaxEntryCount = EFI_DEBUGGER_SYMBOL_ENTRY_MAX;
    Object[Index].SourceBuffer  = AllocateZeroPool (sizeof (VOID *) * (EFI_DEBUGGER_SYMBOL_ENTRY_MAX + 1));
    ASSERT (Object[Index].SourceBuffer != NULL);
  }

  //
  // locate PciRootBridgeIo
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciRootBridgeIoProtocolGuid,
                  NULL,
                  (VOID **)&mDebuggerPrivate.PciRootBridgeIo
                  );

  //
  // locate DebugImageInfoTable
  //
  Status = EfiGetSystemConfigurationTable (
             &gEfiDebugImageInfoTableGuid,
             (VOID **)&mDebuggerPrivate.DebugImageInfoTableHeader
             );

  //
  // Register Debugger Configuration Protocol, for config in shell
  //
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDebuggerConfigurationProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mDebuggerPrivate.DebuggerConfiguration
                  );

  //
  //
  // Create break event
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  EbcDebuggerBreakEventFunc,
                  NULL,
                  &mDebuggerPrivate.BreakEvent
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->SetTimer (
                    mDebuggerPrivate.BreakEvent,
                    TimerPeriodic,
                    EFI_DEBUG_BREAK_TIMER_INTERVAL
                    );
  }

  return;
}

/**

  The hook in UnloadImage for EBC Interpreter.
  It clean up the environment.

**/
VOID
EbcDebuggerHookUnload (
  VOID
  )
{
  UINTN                       Index;
  UINTN                       SubIndex;
  EFI_DEBUGGER_SYMBOL_OBJECT  *Object;

  //
  // Close the break event
  //
  if (mDebuggerPrivate.BreakEvent != NULL) {
    gBS->CloseEvent (mDebuggerPrivate.BreakEvent);
  }

  //
  // Clean up the symbol
  //
  Object = mDebuggerPrivate.DebuggerSymbolContext.Object;
  for (Index = 0; Index < EFI_DEBUGGER_SYMBOL_OBJECT_MAX; Index++) {
    //
    // Clean up Entry
    //
    gBS->FreePool (Object[Index].Entry);
    Object[Index].Entry      = NULL;
    Object[Index].EntryCount = 0;
    //
    // Clean up source buffer
    //
    for (SubIndex = 0; Object[Index].SourceBuffer[SubIndex] != NULL; SubIndex++) {
      gBS->FreePool (Object[Index].SourceBuffer[SubIndex]);
      Object[Index].SourceBuffer[SubIndex] = NULL;
    }

    gBS->FreePool (Object[Index].SourceBuffer);
    Object[Index].SourceBuffer = NULL;
  }

  //
  // Clean up Object
  //
  gBS->FreePool (Object);
  mDebuggerPrivate.DebuggerSymbolContext.Object      = NULL;
  mDebuggerPrivate.DebuggerSymbolContext.ObjectCount = 0;

  //
  // Done
  //
  return;
}

/**

  The hook in EbcUnloadImage.
  Currently do nothing here.

  @param  Handle           - The EbcImage handle.

**/
VOID
EbcDebuggerHookEbcUnloadImage (
  IN EFI_HANDLE  Handle
  )
{
  return;
}

/**

  The hook in ExecuteEbcImageEntryPoint.
  It will record the call-stack entry. (-1 means EbcImageEntryPoint call)
  and trigger Exception if BOE enabled.


  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEbcImageEntryPoint (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushCallstackSource ((UINT64)(UINTN)-1, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushCallstackParameter ((UINT64)(UINTN)VmPtr->Gpr[0], EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushCallstackDest ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerCheckHookFlag (VmPtr, EFI_DEBUG_FLAG_EBC_BOE);
  return;
}

/**

  The hook in ExecuteEbcImageEntryPoint.
  It will record the call-stack entry. (-2 means EbcInterpret call)
  and trigger Exception if BOT enabled.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookEbcInterpret (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushCallstackSource ((UINT64)(UINTN)-2, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushCallstackParameter ((UINT64)(UINTN)VmPtr->Gpr[0], EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushCallstackDest ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerCheckHookFlag (VmPtr, EFI_DEBUG_FLAG_EBC_BOT);
  return;
}

/**

  The hook in EbcExecute, before ExecuteFunction.
  It will trigger Exception if GoTil, StepOver, or StepOut hit.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteStart (
  IN VM_CONTEXT  *VmPtr
  )
{
  EFI_TPL  CurrentTpl;

  //
  // Check Ip for GoTil
  //
  if (mDebuggerPrivate.GoTilContext.BreakAddress == (UINT64)(UINTN)VmPtr->Ip) {
    mDebuggerPrivate.StatusFlags               = EFI_DEBUG_FLAG_EBC_GT;
    mDebuggerPrivate.GoTilContext.BreakAddress = 0;
    EbcDebugSignalException (
      EXCEPT_EBC_BREAKPOINT,
      EXCEPTION_FLAG_NONE,
      VmPtr
      );
    mDebuggerPrivate.StatusFlags &= ~EFI_DEBUG_FLAG_EBC_B_GT;
    return;
  }

  //
  // Check ReturnAddress for StepOver
  //
  if ((mDebuggerPrivate.StepContext.BreakAddress == (UINT64)(UINTN)VmPtr->Ip) &&
      (mDebuggerPrivate.StepContext.FramePointer == (UINT64)(UINTN)VmPtr->FramePtr))
  {
    mDebuggerPrivate.StatusFlags              = EFI_DEBUG_FLAG_EBC_STEPOVER;
    mDebuggerPrivate.StepContext.BreakAddress = 0;
    mDebuggerPrivate.StepContext.FramePointer = 0;
    EbcDebugSignalException (
      EXCEPT_EBC_BREAKPOINT,
      EXCEPTION_FLAG_NONE,
      VmPtr
      );
    mDebuggerPrivate.StatusFlags &= ~EFI_DEBUG_FLAG_EBC_B_STEPOVER;
  }

  //
  // Check FramePtr for StepOut
  //
  if (mDebuggerPrivate.StepContext.BreakAddress == (UINT64)(UINTN)VmPtr->FramePtr) {
    mDebuggerPrivate.StatusFlags              = EFI_DEBUG_FLAG_EBC_STEPOUT;
    mDebuggerPrivate.StepContext.BreakAddress = 0;
    mDebuggerPrivate.StepContext.FramePointer = 0;
    EbcDebugSignalException (
      EXCEPT_EBC_BREAKPOINT,
      EXCEPTION_FLAG_NONE,
      VmPtr
      );
    mDebuggerPrivate.StatusFlags &= ~EFI_DEBUG_FLAG_EBC_B_STEPOUT;
  }

  //
  // Check Flags for BreakOnKey
  //
  if (mDebuggerPrivate.StatusFlags == EFI_DEBUG_FLAG_EBC_BOK) {
    //
    // Only break when the current TPL <= TPL_APPLICATION
    //
    CurrentTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);
    gBS->RestoreTPL (CurrentTpl);
    if (CurrentTpl <= TPL_APPLICATION) {
      EbcDebugSignalException (
        EXCEPT_EBC_BREAKPOINT,
        EXCEPTION_FLAG_NONE,
        VmPtr
        );
      mDebuggerPrivate.StatusFlags &= ~EFI_DEBUG_FLAG_EBC_B_BOK;
    }
  }

  return;
}

/**

  The hook in EbcExecute, after ExecuteFunction.
  It will record StepOut Entry if need.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookExecuteEnd (
  IN VM_CONTEXT  *VmPtr
  )
{
  UINTN  Address;

  //
  // Use FramePtr as checkpoint for StepOut
  //
  CopyMem (&Address, (VOID *)((UINTN)VmPtr->FramePtr), sizeof (Address));
  EbcDebuggerPushStepEntry (Address, (UINT64)(UINTN)VmPtr->FramePtr, EFI_DEBUG_FLAG_EBC_STEPOUT);

  return;
}

/**

  The hook in ExecuteCALL, before move IP.
  It will trigger Exception if BOC enabled,
  and record Callstack, and trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLStart (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerCheckHookFlag (VmPtr, EFI_DEBUG_FLAG_EBC_BOC);
  EbcDebuggerPushCallstackSource ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushCallstackParameter ((UINT64)(UINTN)VmPtr->Gpr[0], EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushTraceSourceEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);
  return;
}

/**

  The hook in ExecuteCALL, after move IP.
  It will record Callstack, trace information
  and record StepOver/StepOut Entry if need.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEnd (
  IN VM_CONTEXT  *VmPtr
  )
{
  UINT64  Address;
  UINTN   FramePtr;

  EbcDebuggerPushCallstackDest ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);
  EbcDebuggerPushTraceDestEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCall);

  //
  // Get Old FramePtr
  //
  CopyMem (&FramePtr, (VOID *)((UINTN)VmPtr->FramePtr), sizeof (FramePtr));

  //
  // Use ReturnAddress as checkpoint for StepOver
  //
  CopyMem (&Address, (VOID *)(UINTN)VmPtr->Gpr[0], sizeof (Address));
  EbcDebuggerPushStepEntry (Address, FramePtr, EFI_DEBUG_FLAG_EBC_STEPOVER);

  //
  // Use FramePtr as checkpoint for StepOut
  //
  Address = 0;
  CopyMem (&Address, (VOID *)(FramePtr), sizeof (UINTN));
  EbcDebuggerPushStepEntry (Address, FramePtr, EFI_DEBUG_FLAG_EBC_STEPOUT);

  return;
}

/**

  The hook in ExecuteCALL, before call EbcLLCALLEX.
  It will trigger Exception if BOCX enabled,
  and record Callstack information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXStart (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerCheckHookFlag (VmPtr, EFI_DEBUG_FLAG_EBC_BOCX);
  //  EbcDebuggerPushCallstackSource ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCallEx);
  //  EbcDebuggerPushCallstackParameter ((UINT64)(UINTN)VmPtr->R[0], EfiDebuggerBranchTypeEbcCallEx);
  EbcDebuggerPushTraceSourceEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCallEx);
  return;
}

/**

  The hook in ExecuteCALL, after call EbcLLCALLEX.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookCALLEXEnd (
  IN VM_CONTEXT  *VmPtr
  )
{
  //  EbcDebuggerPushCallstackDest ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCallEx);
  EbcDebuggerPushTraceDestEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcCallEx);
  return;
}

/**

  The hook in ExecuteRET, before move IP.
  It will trigger Exception if BOR enabled,
  and record Callstack, and trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETStart (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerCheckHookFlag (VmPtr, EFI_DEBUG_FLAG_EBC_BOR);
  EbcDebuggerPopCallstack ();
  EbcDebuggerPushTraceSourceEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcRet);
  return;
}

/**

  The hook in ExecuteRET, after move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookRETEnd (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushTraceDestEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcRet);
  return;
}

/**

  The hook in ExecuteJMP, before move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPStart (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushTraceSourceEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcJmp);
  return;
}

/**

  The hook in ExecuteJMP, after move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMPEnd (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushTraceDestEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcJmp);
  return;
}

/**

  The hook in ExecuteJMP8, before move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8Start (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushTraceSourceEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcJmp8);
  return;
}

/**

  The hook in ExecuteJMP8, after move IP.
  It will record trace information.

  @param  VmPtr - pointer to VM context.

**/
VOID
EbcDebuggerHookJMP8End (
  IN VM_CONTEXT  *VmPtr
  )
{
  EbcDebuggerPushTraceDestEntry ((UINT64)(UINTN)VmPtr->Ip, EfiDebuggerBranchTypeEbcJmp8);
  return;
}
