/** @file
  Debug Agent library implementition.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmmDebugAgentLib.h"

DEBUG_AGENT_MAILBOX         *mMailboxPointer = NULL;
DEBUG_AGENT_MAILBOX         mLocalMailbox;
UINTN                       mSavedDebugRegisters[6];
CONST BOOLEAN               MultiProcessorDebugSupport = FALSE;

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
  //
  // Smm instance has no debug timer to poll break symbol.
  //
  return EFI_NOT_FOUND;
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
  return mMailboxPointer;
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
  return (DEBUG_PORT_HANDLE) (UINTN)(mMailboxPointer->DebugPortHandle);
}

/**
  Store debug register when SMI exit.

**/
VOID
SaveDebugRegister (
  VOID
  )
{
  mSavedDebugRegisters[0] = AsmReadDr0 ();
  mSavedDebugRegisters[1] = AsmReadDr1 ();
  mSavedDebugRegisters[2] = AsmReadDr2 ();
  mSavedDebugRegisters[3] = AsmReadDr3 ();
  mSavedDebugRegisters[4] = AsmReadDr6 ();
  mSavedDebugRegisters[5] = AsmReadDr7 ();
}

/**
  Restore debug register when SMI exit.

**/
VOID
RestoreDebugRegister (
  VOID
  )
{
  AsmWriteDr7 (0);
  AsmWriteDr0 (mSavedDebugRegisters[0]);
  AsmWriteDr1 (mSavedDebugRegisters[1]);
  AsmWriteDr2 (mSavedDebugRegisters[2]);
  AsmWriteDr3 (mSavedDebugRegisters[3]);
  AsmWriteDr6 (mSavedDebugRegisters[4]);
  AsmWriteDr7 (mSavedDebugRegisters[5]);
}

/**
  Initialize debug agent.

  This function is used to set up debug enviroment for source level debug
  in SMM code.

  If InitFlag is DEBUG_AGENT_INIT_SMM, it will overirde IDT table entries
  and initialize debug port. It will get debug agent Mailbox from GUIDed HOB,
  it it exists, debug agent wiil copied it into the local Mailbox in SMM space.
  it will overirde IDT table entries and initialize debug port. Context will be
  NULL.
  If InitFlag is DEBUG_AGENT_INIT_ENTER_SMI, debug agent will save Debug
  Registers and get local Mailbox in SMM space. Context will be NULL.
  If InitFlag is DEBUG_AGENT_INIT_EXIT_SMI, debug agent will restore Debug
  Registers. Context will be NULL.

  @param[in] InitFlag     Init flag is used to decide initialize process.
  @param[in] Context      Context needed according to InitFlag.
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
  EFI_STATUS               Status;
  UINT64                   DebugPortHandle;

  switch (InitFlag) {
  case DEBUG_AGENT_INIT_SMM:
    Status = EfiGetSystemConfigurationTable (&gEfiDebugAgentGuid, (VOID **) &mMailboxPointer);
    if (EFI_ERROR (Status) || mMailboxPointer == NULL) {
      ZeroMem (&mLocalMailbox, sizeof (DEBUG_AGENT_MAILBOX));
      mMailboxPointer = &mLocalMailbox;
    }

    break;

  case DEBUG_AGENT_INIT_ENTER_SMI:
    SaveDebugRegister ();
    InitializeDebugIdt ();

    if (mMailboxPointer != NULL) {
      //
      // Initialize debug communication port
      //
      DebugPortHandle = (UINT64) (UINTN)DebugPortInitialize ((DEBUG_PORT_HANDLE) (UINTN)mMailboxPointer->DebugPortHandle, NULL);
      mMailboxPointer->DebugPortHandle = DebugPortHandle;

      if (mMailboxPointer->DebugFlag.BreakOnNextSmi == 1) {
        //
        // If SMM entry break is set, SMM code will be break at here.
        //
        CpuBreakpoint ();
      }
    }
    break;

  case DEBUG_AGENT_INIT_EXIT_SMI:
    RestoreDebugRegister ();
    break;
  }
}

