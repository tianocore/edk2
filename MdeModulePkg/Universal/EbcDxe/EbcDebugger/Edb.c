/** @file

Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include "Edb.h"

EFI_DEBUGGER_PRIVATE_DATA mDebuggerPrivate = {
  EFI_DEBUGGER_SIGNATURE,                    // Signature
  IsaEbc,                                    // Isa
  (EBC_DEBUGGER_MAJOR_VERSION << 16) |
    EBC_DEBUGGER_MINOR_VERSION,              // EfiDebuggerRevision
  (VM_MAJOR_VERSION << 16) |
    VM_MINOR_VERSION,                        // EbcVmRevision
  {
    EFI_DEBUGGER_CONFIGURATION_VERSION,
    &mDebuggerPrivate,
  },                                         // DebuggerConfiguration
  NULL,                                      // DebugImageInfoTableHeader
  NULL,                                      // Vol
  NULL,                                      // PciRootBridgeIo
  mDebuggerCommandSet,                       // DebuggerCommandSet
  {0},                                       // DebuggerSymbolContext
  0,                                         // DebuggerBreakpointCount
  {{0}},                                     // DebuggerBreakpointContext
  0,                                         // CallStackEntryCount
  {{0}},                                     // CallStackEntry
  0,                                         // TraceEntryCount
  {{0}},                                     // TraceEntry
  {0},                                       // StepContext
  {0},                                       // GoTilContext
  0,                                         // InstructionScope
  EFI_DEBUG_DEFAULT_INSTRUCTION_NUMBER,      // InstructionNumber
  EFI_DEBUG_FLAG_EBC_BOE | EFI_DEBUG_FLAG_EBC_BOT, // FeatureFlags
  0,                                               // StatusFlags
  FALSE,                                           // EnablePageBreak
  NULL                                             // BreakEvent
};

CHAR16 *mExceptionStr[] = {
  L"EXCEPT_EBC_UNDEFINED",
  L"EXCEPT_EBC_DIVIDE_ERROR",
  L"EXCEPT_EBC_DEBUG",
  L"EXCEPT_EBC_BREAKPOINT",
  L"EXCEPT_EBC_OVERFLOW",
  L"EXCEPT_EBC_INVALID_OPCODE",
  L"EXCEPT_EBC_STACK_FAULT",
  L"EXCEPT_EBC_ALIGNMENT_CHECK",
  L"EXCEPT_EBC_INSTRUCTION_ENCODING",
  L"EXCEPT_EBC_BAD_BREAK",
  L"EXCEPT_EBC_SINGLE_STEP",
};

/**

  Clear all the breakpoint.

  @param DebuggerPrivate    EBC Debugger private data structure
  @param NeedRemove         Whether need to remove all the breakpoint

**/
VOID
EdbClearAllBreakpoint (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     BOOLEAN                   NeedRemove
  )
{
  UINTN    Index;

  //
  // Patch all the breakpoint
  //
  for (Index = 0; (Index < DebuggerPrivate->DebuggerBreakpointCount) && (Index < EFI_DEBUGGER_BREAKPOINT_MAX); Index++) {
    if (DebuggerPrivate->DebuggerBreakpointContext[Index].State) {
      CopyMem (
        (VOID *)(UINTN)DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress,
        &DebuggerPrivate->DebuggerBreakpointContext[Index].OldInstruction,
        sizeof(UINT16)
        );
    }
  }

  //
  // Zero Breakpoint context, if need to remove all breakpoint
  //
  if (NeedRemove) {
    DebuggerPrivate->DebuggerBreakpointCount = 0;
    ZeroMem (DebuggerPrivate->DebuggerBreakpointContext, sizeof(DebuggerPrivate->DebuggerBreakpointContext));
  }

  //
  // Done
  //
  return ;
}

/**

  Set all the breakpoint.

  @param DebuggerPrivate    EBC Debugger private data structure

**/
VOID
EdbSetAllBreakpoint (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate
  )
{
  UINTN    Index;
  UINT16   Data16;

  //
  // Set all the breakpoint (BREAK(3) : 0x0300)
  //
  Data16 = 0x0300;
  for (Index = 0; (Index < DebuggerPrivate->DebuggerBreakpointCount) && (Index < EFI_DEBUGGER_BREAKPOINT_MAX); Index++) {
    if (DebuggerPrivate->DebuggerBreakpointContext[Index].State) {
      CopyMem (
        (VOID *)(UINTN)DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress,
        &Data16,
        sizeof(UINT16)
        );
    }
  }

  //
  // Check if current break is caused by breakpoint set.
  // If so, we need to patch memory back to let user see the real memory.
  //
  if (DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX].BreakpointAddress != 0) {
    CopyMem (
      (VOID *)(UINTN)DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX].BreakpointAddress,
      &DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX].OldInstruction,
      sizeof(UINT16)
      );
    DebuggerPrivate->StatusFlags &= ~EFI_DEBUG_FLAG_EBC_B_BP;
  }

  //
  // Done
  //
  return ;
}

/**

  Check all the breakpoint, if match, then set status flag, and record current breakpoint.
  Then clear all breakpoint to let user see a clean memory

  @param   DebuggerPrivate    EBC Debugger private data structure
  @param   SystemContext      EBC system context.

**/
VOID
EdbCheckBreakpoint (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_SYSTEM_CONTEXT        SystemContext
  )
{
  UINT64   Address;
  UINTN    Index;
  BOOLEAN  IsHitBreakpoint;

  //
  // Roll back IP for breakpoint instruction (BREAK(3) : 0x0300)
  //
  Address = SystemContext.SystemContextEbc->Ip - sizeof(UINT16);

  //
  // Check if the breakpoint is hit
  //
  IsHitBreakpoint = FALSE;
  for (Index = 0; (Index < DebuggerPrivate->DebuggerBreakpointCount) && (Index < EFI_DEBUGGER_BREAKPOINT_MAX); Index++) {
    if ((DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress == Address) &&
        (DebuggerPrivate->DebuggerBreakpointContext[Index].State)) {
      IsHitBreakpoint = TRUE;
      break;
    }
  }

  if (IsHitBreakpoint) {
    //
    // If hit, record current breakpoint
    //
    DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX] = DebuggerPrivate->DebuggerBreakpointContext[Index];
    DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX].State = TRUE;
    //
    // Update: IP and Instruction (NOTE: Since we not allow set breakpoint to BREAK 3, this update is safe)
    //
    SystemContext.SystemContextEbc->Ip = Address;
    //
    // Set Flags
    //
    DebuggerPrivate->StatusFlags |= EFI_DEBUG_FLAG_EBC_BP;
  } else {
    //
    // If not hit, check whether current IP is in breakpoint list,
    // because STEP will be triggered before execute the instruction.
    // We should not patch it in de-init.
    //
    Address = SystemContext.SystemContextEbc->Ip;

    //
    // Check if the breakpoint is hit
    //
    IsHitBreakpoint = FALSE;
    for (Index = 0; (Index < DebuggerPrivate->DebuggerBreakpointCount) && (Index < EFI_DEBUGGER_BREAKPOINT_MAX); Index++) {
      if ((DebuggerPrivate->DebuggerBreakpointContext[Index].BreakpointAddress == Address) &&
          (DebuggerPrivate->DebuggerBreakpointContext[Index].State)) {
        IsHitBreakpoint = TRUE;
        break;
      }
    }

    if (IsHitBreakpoint) {
      //
      // If hit, record current breakpoint
      //
      DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX] = DebuggerPrivate->DebuggerBreakpointContext[Index];
      DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX].State = TRUE;
      //
      // Do not set Breakpoint flag. We record the address here just let it not patch breakpoint address when de-init.
      //
    } else {
      //
      // Zero current breakpoint
      //
      ZeroMem (
        &DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX],
        sizeof(DebuggerPrivate->DebuggerBreakpointContext[EFI_DEBUGGER_BREAKPOINT_MAX])
        );
    }
  }

  //
  // Done
  //
  return ;
}

/**
  clear all the symbol.

  @param DebuggerPrivate    EBC Debugger private data structure

**/
VOID
EdbClearSymbol (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate
  )
{
  EFI_DEBUGGER_SYMBOL_CONTEXT *DebuggerSymbolContext;
  EFI_DEBUGGER_SYMBOL_OBJECT  *Object;
  UINTN                       ObjectIndex;
  UINTN                       Index;

  //
  // Go throuth each object
  //
  DebuggerSymbolContext = &DebuggerPrivate->DebuggerSymbolContext;
  for (ObjectIndex = 0; ObjectIndex < DebuggerSymbolContext->ObjectCount; ObjectIndex++) {
    Object = &DebuggerSymbolContext->Object[ObjectIndex];
    //
    // Go throuth each entry
    //
    for (Index = 0; Index < Object->EntryCount; Index++) {
      ZeroMem (&Object->Entry[Index], sizeof(Object->Entry[Index]));
    }
    ZeroMem (Object->Name, sizeof(Object->Name));
    Object->EntryCount = 0;
    Object->BaseAddress = 0;
    Object->StartEntrypointRVA = 0;
    Object->MainEntrypointRVA = 0;
    //
    // Free source buffer
    //
    for (Index = 0; Object->SourceBuffer[Index] != NULL; Index++) {
      gBS->FreePool (Object->SourceBuffer[Index]);
      Object->SourceBuffer[Index] = NULL;
    }
  }
  DebuggerSymbolContext->ObjectCount = 0;

  return ;
}

/**

  Initialize Debugger private data structure

  @param DebuggerPrivate   EBC Debugger private data structure
  @param ExceptionType     Exception type.
  @param SystemContext     EBC system context.
  @param Initialized       Whether the DebuggerPrivate data is initialized.

**/
EFI_STATUS
InitDebuggerPrivateData (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     BOOLEAN                   Initialized
  )
{
  //
  // clear STEP flag in any condition.
  //
  if (SystemContext.SystemContextEbc->Flags & ((UINT64) VMFLAGS_STEP)) {
    SystemContext.SystemContextEbc->Flags &= ~((UINT64) VMFLAGS_STEP);
  }

  if (!Initialized) {
    //
    // Initialize everything
    //
    DebuggerPrivate->InstructionNumber = EFI_DEBUG_DEFAULT_INSTRUCTION_NUMBER;

    DebuggerPrivate->DebuggerBreakpointCount = 0;
    ZeroMem (DebuggerPrivate->DebuggerBreakpointContext, sizeof(DebuggerPrivate->DebuggerBreakpointContext));

//    DebuggerPrivate->StatusFlags = 0;

    DebuggerPrivate->DebuggerSymbolContext.DisplaySymbol = TRUE;
    DebuggerPrivate->DebuggerSymbolContext.DisplayCodeOnly = FALSE;
    DebuggerPrivate->DebuggerSymbolContext.ObjectCount = 0;
  } else {
    //
    // Already initialized, just check Breakpoint here.
    //
    if (ExceptionType == EXCEPT_EBC_BREAKPOINT) {
      EdbCheckBreakpoint (DebuggerPrivate, SystemContext);
    }

    //
    // Clear all breakpoint
    //
    EdbClearAllBreakpoint (DebuggerPrivate, FALSE);
  }

  //
  // Set Scope to currentl IP. (Note: Check Breakpoint may change Ip)
  //
  DebuggerPrivate->InstructionScope = SystemContext.SystemContextEbc->Ip;

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  De-initialize Debugger private data structure.

  @param DebuggerPrivate   EBC Debugger private data structure
  @param ExceptionType     Exception type.
  @param SystemContext     EBC system context.
  @param Initialized       Whether the DebuggerPrivate data is initialized.

**/
EFI_STATUS
DeinitDebuggerPrivateData (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     BOOLEAN                   Initialized
  )
{
  if (!Initialized) {
    //
    // If it does not want initialized state, de-init everything
    //
    DebuggerPrivate->FeatureFlags = EFI_DEBUG_FLAG_EBC_BOE | EFI_DEBUG_FLAG_EBC_BOT;
    DebuggerPrivate->CallStackEntryCount = 0;
    DebuggerPrivate->TraceEntryCount = 0;
    ZeroMem (DebuggerPrivate->CallStackEntry, sizeof(DebuggerPrivate->CallStackEntry));
    ZeroMem (DebuggerPrivate->TraceEntry, sizeof(DebuggerPrivate->TraceEntry));

    //
    // Clear all breakpoint
    //
    EdbClearAllBreakpoint (DebuggerPrivate, TRUE);

    //
    // Clear symbol
    //
    EdbClearSymbol (DebuggerPrivate);
  } else {
    //
    // If it wants to keep initialized state, just set breakpoint.
    //
    EdbSetAllBreakpoint (DebuggerPrivate);
  }

  //
  // Clear Step context
  //
  ZeroMem (&mDebuggerPrivate.StepContext, sizeof(mDebuggerPrivate.StepContext));
  DebuggerPrivate->StatusFlags = 0;

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Print the reason of current break to EbcDebugger.

  @param DebuggerPrivate   EBC Debugger private data structure
  @param ExceptionType     Exception type.
  @param SystemContext     EBC system context.
  @param Initialized       Whether the DebuggerPrivate data is initialized.

**/
VOID
PrintExceptionReason (
  IN     EFI_DEBUGGER_PRIVATE_DATA *DebuggerPrivate,
  IN     EFI_EXCEPTION_TYPE        ExceptionType,
  IN     EFI_SYSTEM_CONTEXT        SystemContext,
  IN     BOOLEAN                   Initialized
  )
{
  //
  // Print break status
  //
  if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_GT) == EFI_DEBUG_FLAG_EBC_GT) {
    EDBPrint (L"Break on GoTil\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOC) == EFI_DEBUG_FLAG_EBC_BOC) {
    EDBPrint (L"Break on CALL\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOCX) == EFI_DEBUG_FLAG_EBC_BOCX) {
    EDBPrint (L"Break on CALLEX\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOR) == EFI_DEBUG_FLAG_EBC_BOR) {
    EDBPrint (L"Break on RET\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOE) == EFI_DEBUG_FLAG_EBC_BOE) {
    EDBPrint (L"Break on Entrypoint\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOT) == EFI_DEBUG_FLAG_EBC_BOT) {
    EDBPrint (L"Break on Thunk\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_STEPOVER) == EFI_DEBUG_FLAG_EBC_STEPOVER) {
    EDBPrint (L"Break on StepOver\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_STEPOUT) == EFI_DEBUG_FLAG_EBC_STEPOUT) {
    EDBPrint (L"Break on StepOut\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BP) == EFI_DEBUG_FLAG_EBC_BP) {
    EDBPrint (L"Break on Breakpoint\n");
  } else if ((DebuggerPrivate->StatusFlags & EFI_DEBUG_FLAG_EBC_BOK) == EFI_DEBUG_FLAG_EBC_BOK) {
    EDBPrint (L"Break on Key\n");
  } else {
    EDBPrint (L"Exception Type - %x", (UINTN)ExceptionType);
    if ((ExceptionType >= EXCEPT_EBC_UNDEFINED) && (ExceptionType <= EXCEPT_EBC_STEP)) {
      EDBPrint (L" (%s)\n", mExceptionStr[ExceptionType]);
    } else {
      EDBPrint (L"\n");
    }
  }

  return ;
}

/**

  The default Exception Callback for the VM interpreter.
  In this function, we report status code, and print debug information
  about EBC_CONTEXT, then dead loop.

  @param ExceptionType    Exception type.
  @param SystemContext    EBC system context.

**/
VOID
EFIAPI
EdbExceptionHandler (
  IN     EFI_EXCEPTION_TYPE   ExceptionType,
  IN OUT EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  CHAR16                  InputBuffer[EFI_DEBUG_INPUS_BUFFER_SIZE];
  CHAR16                  *CommandArg;
  EFI_DEBUGGER_COMMAND    DebuggerCommand;
  EFI_DEBUG_STATUS        DebugStatus;
  STATIC BOOLEAN          mInitialized;

  mInitialized = FALSE;

  DEBUG ((DEBUG_ERROR, "Hello EBC Debugger!\n"));

  if (!mInitialized) {
    //
    // Print version
    //
    EDBPrint (
      L"EBC Interpreter Version - %d.%d\n",
      (UINTN)VM_MAJOR_VERSION,
      (UINTN)VM_MINOR_VERSION
      );
    EDBPrint (
      L"EBC Debugger Version - %d.%d\n",
      (UINTN)EBC_DEBUGGER_MAJOR_VERSION,
      (UINTN)EBC_DEBUGGER_MINOR_VERSION
      );
  }
  //
  // Init Private Data
  //
  InitDebuggerPrivateData (&mDebuggerPrivate, ExceptionType, SystemContext, mInitialized);

  //
  // EDBPrint basic info
  //
  PrintExceptionReason (&mDebuggerPrivate, ExceptionType, SystemContext, mInitialized);

  EdbShowDisasm (&mDebuggerPrivate, SystemContext);
  // EFI_BREAKPOINT ();

  if (!mInitialized) {
    //
    // Interactive with user
    //
    EDBPrint (L"\nPlease enter command now, \'h\' for help.\n");
    EDBPrint (L"(Using <Command> -b <...> to enable page break.)\n");
  }
  mInitialized = TRUE;

  //
  // Dispatch each command
  //
  while (TRUE) {
    //
    // Get user input
    //
    Input (L"\n\r" EFI_DEBUG_PROMPT_STRING, InputBuffer, EFI_DEBUG_INPUS_BUFFER_SIZE);
    EDBPrint (L"\n");

    //
    // Get command
    //
    DebuggerCommand = MatchDebuggerCommand (InputBuffer, &CommandArg);
    if (DebuggerCommand == NULL) {
      EDBPrint (L"ERROR: Command not found!\n");
      continue;
    }

    //
    // Check PageBreak;
    //
    if (CommandArg != NULL) {
      if (StriCmp (CommandArg, L"-b") == 0) {
        CommandArg = StrGetNextTokenLine (L" ");
        mDebuggerPrivate.EnablePageBreak = TRUE;
      }
    }

    //
    // Dispatch command
    //
    DebugStatus = DebuggerCommand (CommandArg, &mDebuggerPrivate, ExceptionType, SystemContext);
    mDebuggerPrivate.EnablePageBreak = FALSE;

    //
    // Check command return status
    //
    if (DebugStatus == EFI_DEBUG_RETURN) {
      mInitialized = FALSE;
      break;
    } else if (DebugStatus == EFI_DEBUG_BREAK) {
      break;
    } else if (DebugStatus == EFI_DEBUG_CONTINUE) {
      continue;
    } else {
      ASSERT (FALSE);
    }
  }

  //
  // Deinit Private Data
  //
  DeinitDebuggerPrivateData (&mDebuggerPrivate, ExceptionType, SystemContext, mInitialized);

  DEBUG ((DEBUG_ERROR, "Goodbye EBC Debugger!\n"));

  return;
}
