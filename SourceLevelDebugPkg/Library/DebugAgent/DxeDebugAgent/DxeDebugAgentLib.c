/** @file
  Debug Agent library implementition for Dxe Core and Dxr modules.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeDebugAgentLib.h"

DEBUG_AGENT_MAILBOX          mMailbox;
DEBUG_AGENT_MAILBOX          *mMailboxPointer;
IA32_IDT_GATE_DESCRIPTOR     mIdtEntryTable[33];
BOOLEAN                      mDxeCoreFlag               = FALSE;
CONST BOOLEAN                MultiProcessorDebugSupport = TRUE;

/**
  Constructor allocates the NVS memory to store Mailbox and install configuration table
  in system table to store its pointer.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS   Allocate the global memory space to store guid and function tables.

**/
RETURN_STATUS
EFIAPI
DxeDebugAgentLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        Address;
  EFI_EVENT                   Event;
  VOID                        *EventRegistration;

  if (!mDxeCoreFlag) {
    return RETURN_SUCCESS;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InstallSerialIoNotification,
                  NULL,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //

  Status = gBS->RegisterProtocolNotify (
                  &gEfiPcdProtocolGuid,
                  Event,
                  &EventRegistration
                  );

  ASSERT_EFI_ERROR (Status);

  Address = 0;
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof (DEBUG_AGENT_MAILBOX)),
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  CopyMem (
    (UINT8 *) (UINTN) Address,
    (UINT8 *) (UINTN) mMailboxPointer,
    sizeof (DEBUG_AGENT_MAILBOX)
    );

  mMailboxPointer = (DEBUG_AGENT_MAILBOX *) (UINTN) Address;

  Status = gBS->InstallConfigurationTable (&gEfiDebugAgentGuid, (VOID *) mMailboxPointer);
  ASSERT_EFI_ERROR (Status);
  
  return Status;
}

/**
  Get the pointer to Mailbox from the GUIDed HOB.

  @param[in]  HobStart      The starting HOB pointer to search from.

  @return Pointer to Mailbox.

**/
DEBUG_AGENT_MAILBOX *
GetMailboxFromHob (
  IN VOID                  *HobStart
  )
{
  EFI_HOB_GUID_TYPE        *GuidHob;

  GuidHob = GetNextGuidHob (&gEfiDebugAgentGuid, HobStart);
  if (GuidHob == NULL) {
    return NULL;
  }

  return (DEBUG_AGENT_MAILBOX *) (GET_GUID_HOB_DATA(GuidHob));
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
  Initialize debug agent.

  This function is used to set up debug enviroment for DXE phase.

  If this function is called by DXE Core, Context must be the pointer
  to HOB list which will be used to get GUIDed HOB. It will enable
  interrupt to support break-in feature.
  If this function is called by DXE module, Context must be NULL. It
  will enable interrupt to support break-in feature.

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
  DEBUG_AGENT_MAILBOX          *Mailbox;
  IA32_DESCRIPTOR              Idtr;
  UINT16                       IdtEntryCount;
  BOOLEAN                      InterruptStatus;

  if (InitFlag != DEBUG_AGENT_INIT_DXE_CORE &&
      InitFlag != DEBUG_AGENT_INIT_S3 &&
      InitFlag != DEBUG_AGENT_INIT_DXE_AP) {
    return;
  }

  //
  // Save and disable original interrupt status
  //
  InterruptStatus = SaveAndDisableInterrupts ();

  if (InitFlag == DEBUG_AGENT_INIT_DXE_CORE) {
    //
    // Try to get Mailbox from GUIDed HOB.
    //
    mDxeCoreFlag = TRUE;
    Mailbox = GetMailboxFromHob (Context);
    
    //
    // Clear Break CPU index value
    //
    mDebugMpContext.BreakAtCpuIndex = (UINT32) -1;

  } else if (InitFlag == DEBUG_AGENT_INIT_DXE_AP) {

    EnableInterrupts ();

    return;

  } else {
    //
    // If it is in S3 path, needn't to install configuration table.
    //
    Mailbox = NULL;
  }

  if (Mailbox != NULL) {
    //
    // If Mailbox exists, copy it into one global variable.
    //
    CopyMem (&mMailbox, Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
  } else {
    //
    // If Mailbox not exists, used the local Mailbox.
    //
    ZeroMem (&mMailbox, sizeof (DEBUG_AGENT_MAILBOX));
  }

  mMailboxPointer = &mMailbox;

  //
  // Get original IDT address and size.
  //
  AsmReadIdtr ((IA32_DESCRIPTOR *) &Idtr);
  IdtEntryCount = (UINT16) ((Idtr.Limit + 1) / sizeof (IA32_IDT_GATE_DESCRIPTOR));
  if (IdtEntryCount < 33) {
    Idtr.Limit = (UINT16) (sizeof (IA32_IDT_GATE_DESCRIPTOR) * 33 - 1);
    Idtr.Base  = (UINTN) &mIdtEntryTable;
    ZeroMem (&mIdtEntryTable, Idtr.Limit + 1);
    AsmWriteIdtr ((IA32_DESCRIPTOR *) &Idtr);
  }

  //
  // Initialize the IDT table entries to support source level debug.
  //
  InitializeDebugIdt ();

  //
  // Initialize debug communication port
  //
  mMailboxPointer->DebugPortHandle = (UINT64) (UINTN)DebugPortInitialize ((VOID *)(UINTN)mMailbox.DebugPortHandle, NULL);

  InitializeSpinLock (&mDebugMpContext.MpContextSpinLock);
  InitializeSpinLock (&mDebugMpContext.DebugPortSpinLock);
 
  if (InitFlag == DEBUG_AGENT_INIT_DXE_CORE) {
    //
    // Initialize Debug Timer hardware and enable interrupt.
    //
    InitializeDebugTimer ();
    EnableInterrupts ();

    return;
  } else {
    //
    // Disable Debug Timer interrupt in S3 path.
    //
    SaveAndSetDebugTimerInterrupt (FALSE);

    //
    // Restore interrupt state.
    //
    SetInterruptState (InterruptStatus);
  }

}

