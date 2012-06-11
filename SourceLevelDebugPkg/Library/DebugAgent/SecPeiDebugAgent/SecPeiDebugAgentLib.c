/** @file
  SEC Core Debug Agent Library instance implementition.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecPeiDebugAgentLib.h"

CONST BOOLEAN                MultiProcessorDebugSupport = FALSE;

/**
  Read the Attach/Break-in symbols from the debug port.

  @param[in]  Handle         Pointer to Debug Port handle.
  @param[out] BreakSymbol    Returned break symbol.

  @retval EFI_SUCCESS        Read the symbol in BreakSymbol.
  @retval EFI_NOT_FOUND      No read the break symbol.

**/
EFI_STATUS
DebugReadBreakSymbol (
  IN  DEBUG_PORT_HANDLE      Handle,
  OUT UINT8                  *BreakSymbol
  )
{
  *BreakSymbol = 0;
  //
  // If Debug Port buffer has data, read it till it was break symbol or Debug Port buffer emty.
  //
  while (DebugPortPollBuffer (Handle)) {
    DebugPortReadBuffer (Handle, BreakSymbol, 1, 0);
    if (*BreakSymbol == DEBUG_STARTING_SYMBOL_ATTACH || *BreakSymbol == DEBUG_STARTING_SYMBOL_BREAK) {
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_FOUND;
}

/**
  Get pointer to Mailbox from IDT entry before memory is ready.

**/
VOID *
GetMailboxPointerInIdtEntry (
  VOID
  )
{
  IA32_IDT_GATE_DESCRIPTOR   *IdtEntry;
  IA32_DESCRIPTOR            IdtDescriptor;
  UINTN                      Mailbox;

  AsmReadIdtr (&IdtDescriptor);
  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *) IdtDescriptor.Base;

  Mailbox = IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetLow + (IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetHigh << 16);
  return (VOID *) Mailbox;
}

/**
  Set the pointer of Mailbox into IDT entry before memory is ready.

  @param[in]  Mailbox       The pointer of Mailbox.

**/
VOID
SetMailboxPointerInIdtEntry (
  IN VOID                    *Mailbox
  )
{
  IA32_IDT_GATE_DESCRIPTOR   *IdtEntry;
  IA32_DESCRIPTOR            IdtDescriptor;

  AsmReadIdtr (&IdtDescriptor);
  IdtEntry = (IA32_IDT_GATE_DESCRIPTOR *) IdtDescriptor.Base;

  IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetLow  = (UINT16)(UINTN)Mailbox;
  IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetHigh = (UINT16)((UINTN)Mailbox >> 16);
}

/**
  Get the pointer to Mailbox from IDT entry and build the Mailbox into GUIDed Hob
  after memory is ready.

  @return Pointer to Mailbox.

**/
DEBUG_AGENT_MAILBOX *
BuildMailboxHob (
  VOID
  )
{
  DEBUG_AGENT_MAILBOX       *Mailbox;

  Mailbox = (DEBUG_AGENT_MAILBOX *) GetMailboxPointerInIdtEntry ();

  return BuildGuidDataHob (
           &gEfiDebugAgentGuid,
           Mailbox,
           sizeof (DEBUG_AGENT_MAILBOX)
           );
}

/**
  Get Debug Agent Mailbox pointer.

  @return Mailbox pointer.

**/
DEBUG_AGENT_MAILBOX *
GetMailboxPointer (
  VOID
  )
{
  return (DEBUG_AGENT_MAILBOX *) GetMailboxPointerInIdtEntry ();
}

/**
  Get debug port handle.

  @return Debug port handle.

**/
DEBUG_PORT_HANDLE
GetDebugPortHandle (
  VOID
  )
{
  DEBUG_AGENT_MAILBOX    *DebugAgentMailbox;
  
  DebugAgentMailbox = (DEBUG_AGENT_MAILBOX *)GetMailboxPointerInIdtEntry ();

  return (DEBUG_PORT_HANDLE) (UINTN)(DebugAgentMailbox->DebugPortHandle);
}

/**
  Trigger one software interrupt to debug agent to handle it.

  @param Signature       Software interrupt signature.

**/
VOID
TriggerSoftInterrupt (
  UINT32                 Signature
  )
{
  UINTN                  Dr0;
  UINTN                  Dr1;

  //
  // Save Debug Register State
  //
  Dr0 = AsmReadDr0 ();
  Dr1 = AsmReadDr1 ();

  //
  // DR0 = Signature
  //
  AsmWriteDr0 (SOFT_INTERRUPT_SIGNATURE);
  AsmWriteDr1 (Signature);

  //
  // Do INT3 to communicate with HOST side
  //
  CpuBreakpoint ();

  //
  // Restore Debug Register State only when Host didn't change it inside exception handler.
  //   Dr registers can only be changed by setting the HW breakpoint.
  //
  AsmWriteDr0 (Dr0);
  AsmWriteDr1 (Dr1);

}

/**
  Initialize debug agent.

  This function is used to set up debug environment for SEC and PEI phase.

  If InitFlag is DEBUG_AGENT_INIT_PREMEM_SEC, it will overirde IDT table entries
  and initialize debug port. It will enable interrupt to support break-in feature.
  It will set up debug agent Mailbox in cache-as-ramfrom. It will be called before
  physical memory is ready.
  If InitFlag is DEBUG_AGENT_INIT_POSTMEM_SEC, debug agent will build one GUIDed
  HOB to copy debug agent Mailbox. It will be called after physical memory is ready.

  This function is used to set up debug environment to support source level debugging.
  If certain Debug Agent Library instance has to save some private data in the stack,
  this function must work on the mode that doesn't return to the caller, then
  the caller needs to wrap up all rest of logic after InitializeDebugAgent() into one
  function and pass it into InitializeDebugAgent(). InitializeDebugAgent() is
  responsible to invoke the passing-in function at the end of InitializeDebugAgent().

  If the parameter Function is not NULL, Debug Agent Libary instance will invoke it by
  passing in the Context to be its parameter.

  If Function() is NULL, Debug Agent Library instance will return after setup debug
  environment.

  @param[in] InitFlag     Init flag is used to decide the initialize process.
  @param[in] Context      Context needed according to InitFlag; it was optional.
  @param[in] Function     Continue function called by debug agent library; it was
                          optional.

**/
VOID
EFIAPI
InitializeDebugAgent (
  IN UINT32                InitFlag,
  IN VOID                  *Context, OPTIONAL
  IN DEBUG_AGENT_CONTINUE  Function  OPTIONAL
  )
{
  DEBUG_AGENT_MAILBOX              *Mailbox;
  DEBUG_AGENT_MAILBOX              MailboxInStack;
  DEBUG_AGENT_PHASE2_CONTEXT       Phase2Context;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  *DebugAgentContext;

  DisableInterrupts ();

  switch (InitFlag) {

  case DEBUG_AGENT_INIT_PREMEM_SEC:

    InitializeDebugIdt ();

    Mailbox = &MailboxInStack;
    ZeroMem ((VOID *) Mailbox, sizeof (DEBUG_AGENT_MAILBOX));

    //
    // Get and save debug port handle and set the length of memory block.
    //
    SetMailboxPointerInIdtEntry ((VOID *) Mailbox);

    InitializeDebugTimer ();

    Phase2Context.Context  = Context;
    Phase2Context.Function = Function;
    DebugPortInitialize ((VOID *) &Phase2Context, InitializeDebugAgentPhase2);

    //
    // If reaches here, it means Debug Port initialization failed.
    //
    DEBUG ((EFI_D_ERROR, "Debug Agent: Debug port initialization failed.\n"));

    break;

  case DEBUG_AGENT_INIT_POSTMEM_SEC:

    //
    // Memory has been ready
    //
    if (IsHostAttached()) {
      //
      // Trigger one software interrupt to inform HOST
      //
      TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
    }

    DebugAgentContext = (DEBUG_AGENT_CONTEXT_POSTMEM_SEC *) Context;

    Mailbox = (DEBUG_AGENT_MAILBOX *) GetMailboxPointerInIdtEntry ();
    Mailbox->DebugPortHandle = (UINT64)(UINT32)(Mailbox->DebugPortHandle + DebugAgentContext->StackMigrateOffset);
    Mailbox->DebugFlag.MemoryReady = 1;

    Mailbox = BuildMailboxHob ();
    Mailbox = (DEBUG_AGENT_MAILBOX *) ((UINTN) Mailbox + DebugAgentContext->HeapMigrateOffset);

    SetMailboxPointerInIdtEntry ((VOID *) Mailbox);

    EnableInterrupts ();

    break;

  default:

    //
    // Only DEBUG_AGENT_INIT_PREMEM_SEC and DEBUG_AGENT_INIT_POSTMEM_SEC are allowed for this 
    // Debug Agent library instance.
    //
    DEBUG ((EFI_D_ERROR, "Debug Agent: The InitFlag value is not allowed!\n"));
    CpuDeadLoop ();
    break;

  }

  //
  // If Function is not NULL, invoke it always whatever debug agent was initialized sucesssfully or not.
  //
  if (Function != NULL) {
    Function (Context);
  }
}

/**
  Caller provided function to be invoked at the end of DebugPortInitialize().

  Refer to the descrption for DebugPortInitialize() for more details.

  @param[in] Context           The first input argument of DebugPortInitialize().
  @param[in] DebugPortHandle   Debug port handle created by Debug Communication Libary.

**/
VOID
EFIAPI
InitializeDebugAgentPhase2 (
  IN VOID                  *Context,
  IN DEBUG_PORT_HANDLE     DebugPortHandle
  )
{
  DEBUG_AGENT_PHASE2_CONTEXT *Phase2Context;
  DEBUG_AGENT_MAILBOX        *Mailbox;
  EFI_SEC_PEI_HAND_OFF       *SecCoreData;

  Mailbox = GetMailboxPointerInIdtEntry ();
  Mailbox->DebugPortHandle = (UINT64) (UINTN)DebugPortHandle;

  //
  // Trigger one software interrupt to inform HOST
  //
  TriggerSoftInterrupt (SYSTEM_RESET_SIGNATURE);

  //
  // If Temporary RAM region is below 128 MB, then send message to 
  // host to disable low memory filtering.
  //
  Phase2Context = (DEBUG_AGENT_PHASE2_CONTEXT *) Context;
  SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Phase2Context->Context;
  if ((UINTN)SecCoreData->TemporaryRamBase < BASE_128MB && IsHostAttached ()) {
    TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
  }

  //
  // Enable CPU interrupts so debug timer interrupts can be delivered
  //
  EnableInterrupts ();

  //
  // Call continuation function if it is not NULL.
  //
  if (Phase2Context->Function != NULL) {
    Phase2Context->Function (Phase2Context->Context);
  }
}
