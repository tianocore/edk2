/** @file
  SEC Core Debug Agent Library instance implementation.

  Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecPeiDebugAgentLib.h"

GLOBAL_REMOVE_IF_UNREFERENCED BOOLEAN  mSkipBreakpoint = FALSE;


GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_VECTOR_HANDOFF_INFO_PPI mVectorHandoffInfoPpi = {
  &mVectorHandoffInfoDebugAgent[0]
};

//
// Ppis to be installed
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR           mVectorHandoffInfoPpiList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiVectorHandoffInfoPpiGuid,
    &mVectorHandoffInfoPpi
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_NOTIFY_DESCRIPTOR mDebugAgentMemoryDiscoveredNotifyList[1] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    DebugAgentCallbackMemoryDiscoveredPpi
  }
};

/**
  Check if debug agent support multi-processor.

  @retval TRUE    Multi-processor is supported.
  @retval FALSE   Multi-processor is not supported.

**/
BOOLEAN
MultiProcessorDebugSupport (
  VOID
  )
{
  return FALSE;
}

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
  EFI_STATUS                 Status;
  DEBUG_PACKET_HEADER        DebugHeader;
  UINT8                      *Data8;

  *BreakSymbol = 0;
  //
  // If Debug Port buffer has data, read it till it was break symbol or Debug Port buffer empty.
  //
  Data8 = (UINT8 *) &DebugHeader;
  while (TRUE) {
    //
    // If start symbol is not received
    //
    if (!DebugPortPollBuffer (Handle)) {
      //
      // If no data in Debug Port, exit
      //
      break;
    }
    //
    // Try to read the start symbol
    //
    DebugAgentReadBuffer (Handle, Data8, 1, 0);
    if (*Data8 == DEBUG_STARTING_SYMBOL_ATTACH) {
      *BreakSymbol = *Data8;
      DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Debug Timer attach symbol received %x", *BreakSymbol);
      return EFI_SUCCESS;
    }
    if (*Data8 == DEBUG_STARTING_SYMBOL_NORMAL) {
      Status = ReadRemainingBreakPacket (Handle, &DebugHeader);
      if (Status == EFI_SUCCESS) {
        *BreakSymbol = DebugHeader.Command;
        DebugAgentMsgPrint (DEBUG_AGENT_INFO, "Debug Timer break symbol received %x", *BreakSymbol);
        return EFI_SUCCESS;
      }
      if (Status == EFI_TIMEOUT) {
        break;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Get the pointer to location saved Mailbox pointer from IDT entry.

**/
VOID *
GetLocationSavedMailboxPointerInIdtEntry (
  VOID
  )
{
  UINTN                     *MailboxLocation;

  MailboxLocation = (UINTN *) GetExceptionHandlerInIdtEntry (DEBUG_MAILBOX_VECTOR);
  //
  // *MailboxLocation is the pointer to Mailbox
  //
  VerifyMailboxChecksum ((DEBUG_AGENT_MAILBOX *) (*MailboxLocation));
  return MailboxLocation;
}

/**
  Set the pointer of Mailbox into IDT entry before memory is ready.

  @param[in]  MailboxLocation    Pointer to location saved Mailbox pointer.

**/
VOID
SetLocationSavedMailboxPointerInIdtEntry (
  IN VOID                  *MailboxLocation
  )
{
  SetExceptionHandlerInIdtEntry (DEBUG_MAILBOX_VECTOR, MailboxLocation);
}

/**
  Get the location of Mailbox pointer from the GUIDed HOB.

  @return Pointer to the location saved Mailbox pointer.

**/
UINT64 *
GetMailboxLocationFromHob (
  VOID
  )
{
  EFI_HOB_GUID_TYPE        *GuidHob;

  GuidHob = GetFirstGuidHob (&gEfiDebugAgentGuid);
  if (GuidHob == NULL) {
    return NULL;
  }
  return (UINT64 *) (GET_GUID_HOB_DATA(GuidHob));
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
  UINT64               DebugPortHandle;
  UINT64               *MailboxLocationInIdt;
  UINT64               *MailboxLocationInHob;
  DEBUG_AGENT_MAILBOX  *Mailbox;

  //
  // Get mailbox from IDT entry firstly
  //
  MailboxLocationInIdt = GetLocationSavedMailboxPointerInIdtEntry ();
  Mailbox = (DEBUG_AGENT_MAILBOX *)(UINTN)(*MailboxLocationInIdt);
  //
  // Cannot used GetDebugFlag() to get Debug Flag to avoid GetMailboxPointer() nested
  //
  if (Mailbox->DebugFlag.Bits.CheckMailboxInHob != 1 ||
      Mailbox->DebugFlag.Bits.InitArch != DEBUG_ARCH_SYMBOL) {
    //
    // If mailbox was setup in SEC or the current CPU arch is different from the init arch
    // Debug Agent initialized, return the mailbox from IDT entry directly.
    // Otherwise, we need to check the mailbox location saved in GUIDed HOB further.
    //
    return Mailbox;
  }

  MailboxLocationInHob = GetMailboxLocationFromHob ();
  //
  // Compare mailbox in IDT entry with mailbox in HOB,
  // need to fix mailbox location if HOB moved by PEI CORE
  //
  if (MailboxLocationInHob != MailboxLocationInIdt && MailboxLocationInHob != NULL) {
    Mailbox = (DEBUG_AGENT_MAILBOX *)(UINTN)(*MailboxLocationInHob);
    //
    // Fix up Debug Port handler and save new mailbox in IDT entry
    //
    Mailbox = (DEBUG_AGENT_MAILBOX *)((UINTN)Mailbox + ((UINTN)(MailboxLocationInHob) - (UINTN)MailboxLocationInIdt));
    DebugPortHandle = (UINTN)Mailbox->DebugPortHandle + ((UINTN)(MailboxLocationInHob) - (UINTN)MailboxLocationInIdt);
    UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, DebugPortHandle);
    *MailboxLocationInHob = (UINT64)(UINTN)Mailbox;
    SetLocationSavedMailboxPointerInIdtEntry (MailboxLocationInHob);
    //
    // Clean CheckMailboxInHob flag
    //
    Mailbox->DebugFlag.Bits.CheckMailboxInHob = 0;
    UpdateMailboxChecksum (Mailbox);
  }

  return Mailbox;
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

  DebugAgentMailbox = GetMailboxPointer ();

  return (DEBUG_PORT_HANDLE) (UINTN)(DebugAgentMailbox->DebugPortHandle);
}

/**
  Debug Agent provided notify callback function on Memory Discovered PPI.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @retval EFI_SUCCESS If the function completed successfully.

**/
EFI_STATUS
EFIAPI
DebugAgentCallbackMemoryDiscoveredPpi (
  IN EFI_PEI_SERVICES                     **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR            *NotifyDescriptor,
  IN VOID                                 *Ppi
  )
{
  EFI_STATUS                     Status;
  DEBUG_AGENT_MAILBOX            *Mailbox;
  BOOLEAN                        InterruptStatus;
  EFI_PHYSICAL_ADDRESS           Address;
  DEBUG_AGENT_MAILBOX            *NewMailbox;
  UINT64                         *MailboxLocationInHob;

  //
  // Save and disable original interrupt status
  //
  InterruptStatus = SaveAndDisableInterrupts ();

  //
  // Allocate ACPI NVS memory for new Mailbox and Debug Port Handle buffer
  //
  Status = PeiServicesAllocatePages (
             EfiACPIMemoryNVS,
             EFI_SIZE_TO_PAGES (sizeof(DEBUG_AGENT_MAILBOX) + PcdGet16(PcdDebugPortHandleBufferSize)),
             &Address
             );
  ASSERT_EFI_ERROR (Status);
  NewMailbox = (DEBUG_AGENT_MAILBOX *) (UINTN) Address;
  //
  // Copy Mailbox and Debug Port Handle buffer to new location in ACPI NVS memory, because original Mailbox
  // and Debug Port Handle buffer in the allocated pool that may be marked as free by DXE Core after DXE Core
  // reallocates the HOB.
  //
  Mailbox = GetMailboxPointer ();
  CopyMem (NewMailbox, Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
  CopyMem (NewMailbox + 1, (VOID *)(UINTN)Mailbox->DebugPortHandle, PcdGet16(PcdDebugPortHandleBufferSize));
  //
  // Update Mailbox Location pointer in GUIDed HOB and IDT entry with new one
  //
  MailboxLocationInHob = GetMailboxLocationFromHob ();
  ASSERT (MailboxLocationInHob != NULL);
  *MailboxLocationInHob = (UINT64)(UINTN)NewMailbox;
  SetLocationSavedMailboxPointerInIdtEntry (MailboxLocationInHob);
  //
  // Update Debug Port Handle in new Mailbox
  //
  UpdateMailboxContent (NewMailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, (UINT64)(UINTN)(NewMailbox + 1));
  //
  // Set physical memory ready flag
  //
  SetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY, 1);

  if (IsHostAttached ()) {
    //
    // Trigger one software interrupt to inform HOST
    //
    TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
  }

  //
  // Restore interrupt state.
  //
  SetInterruptState (InterruptStatus);

  return EFI_SUCCESS;
}

/**
  Initialize debug agent.

  This function is used to set up debug environment for SEC and PEI phase.

  If InitFlag is DEBUG_AGENT_INIT_PREMEM_SEC, it will override IDT table entries
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

  If the parameter Function is not NULL, Debug Agent Library instance will invoke it by
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
  DEBUG_AGENT_MAILBOX              *NewMailbox;
  DEBUG_AGENT_MAILBOX              MailboxInStack;
  DEBUG_AGENT_PHASE2_CONTEXT       Phase2Context;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  *DebugAgentContext;
  EFI_STATUS                       Status;
  IA32_DESCRIPTOR                  *Ia32Idtr;
  IA32_IDT_ENTRY                   *Ia32IdtEntry;
  UINT64                           DebugPortHandle;
  UINT64                           MailboxLocation;
  UINT64                           *MailboxLocationPointer;
  EFI_PHYSICAL_ADDRESS             Address;
  UINT32                           DebugTimerFrequency;
  BOOLEAN                          CpuInterruptState;

  //
  // Disable interrupts and save current interrupt state
  //
  CpuInterruptState = SaveAndDisableInterrupts();

  switch (InitFlag) {

  case DEBUG_AGENT_INIT_PREMEM_SEC:

    InitializeDebugIdt ();

    MailboxLocation = (UINT64)(UINTN)&MailboxInStack;
    Mailbox = &MailboxInStack;
    ZeroMem ((VOID *) Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
    //
    // Get and save debug port handle and set the length of memory block.
    //
    SetLocationSavedMailboxPointerInIdtEntry (&MailboxLocation);
    //
    // Force error message could be printed during the first shakehand between Target/HOST.
    //
    SetDebugFlag (DEBUG_AGENT_FLAG_PRINT_ERROR_LEVEL, DEBUG_AGENT_ERROR);
    //
    // Save init arch type when debug agent initialized
    //
    SetDebugFlag (DEBUG_AGENT_FLAG_INIT_ARCH, DEBUG_ARCH_SYMBOL);
    //
    // Initialize Debug Timer hardware and save its frequency
    //
    InitializeDebugTimer (&DebugTimerFrequency, TRUE);
    UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_TIMER_FREQUENCY, DebugTimerFrequency);

    Phase2Context.InitFlag = InitFlag;
    Phase2Context.Context  = Context;
    Phase2Context.Function = Function;
    DebugPortInitialize ((VOID *) &Phase2Context, InitializeDebugAgentPhase2);
    //
    // If reaches here, it means Debug Port initialization failed.
    //
    DEBUG ((EFI_D_ERROR, "Debug Agent: Debug port initialization failed.\n"));

    break;

  case DEBUG_AGENT_INIT_POSTMEM_SEC:
    Mailbox = GetMailboxPointer ();
    //
    // Memory has been ready
    //
    SetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY, 1);
    if (IsHostAttached ()) {
      //
      // Trigger one software interrupt to inform HOST
      //
      TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
    }
    //
    // Install Vector Handoff Info PPI to persist vectors used by Debug Agent
    //
    Status = PeiServicesInstallPpi (&mVectorHandoffInfoPpiList[0]);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Failed to install Vector Handoff Info PPI!\n"));
      CpuDeadLoop ();
    }
    //
    // Fix up Debug Port handle address and mailbox address
    //
    DebugAgentContext = (DEBUG_AGENT_CONTEXT_POSTMEM_SEC *) Context;
    if (DebugAgentContext != NULL) {
      DebugPortHandle = (UINT64)(UINT32)(Mailbox->DebugPortHandle + DebugAgentContext->StackMigrateOffset);
      UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, DebugPortHandle);
      Mailbox = (DEBUG_AGENT_MAILBOX *) ((UINTN) Mailbox + DebugAgentContext->StackMigrateOffset);
      MailboxLocation = (UINT64)(UINTN)Mailbox;
      //
      // Build mailbox location in HOB and fix-up its address
      //
      MailboxLocationPointer = BuildGuidDataHob (
                                 &gEfiDebugAgentGuid,
                                 &MailboxLocation,
                                 sizeof (UINT64)
                                 );
      MailboxLocationPointer = (UINT64 *) ((UINTN) MailboxLocationPointer + DebugAgentContext->HeapMigrateOffset);
    } else {
      //
      // DebugAgentContext is NULL. Then, Mailbox can directly be copied into memory.
      // Allocate ACPI NVS memory for new Mailbox and Debug Port Handle buffer
      //
      Status = PeiServicesAllocatePages (
                 EfiACPIMemoryNVS,
                 EFI_SIZE_TO_PAGES (sizeof(DEBUG_AGENT_MAILBOX) + PcdGet16(PcdDebugPortHandleBufferSize)),
                 &Address
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "DebugAgent: Failed to allocate pages!\n"));
        CpuDeadLoop ();
      }
      NewMailbox = (DEBUG_AGENT_MAILBOX *) (UINTN) Address;
      //
      // Copy Mailbox and Debug Port Handle buffer to new location in ACPI NVS memory, because original Mailbox
      // and Debug Port Handle buffer in the allocated pool that may be marked as free by DXE Core after DXE Core
      // reallocates the HOB.
      //
      CopyMem (NewMailbox, Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
      CopyMem (NewMailbox + 1, (VOID *)(UINTN)Mailbox->DebugPortHandle, PcdGet16(PcdDebugPortHandleBufferSize));
      UpdateMailboxContent (NewMailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, (UINT64)(UINTN)(NewMailbox + 1));
      MailboxLocation = (UINT64)(UINTN)NewMailbox;
      //
      // Build mailbox location in HOB
      //
      MailboxLocationPointer = BuildGuidDataHob (
                                 &gEfiDebugAgentGuid,
                                 &MailboxLocation,
                                 sizeof (UINT64)
                                 );
    }
    //
    // Update IDT entry to save the location saved mailbox pointer
    //
    SetLocationSavedMailboxPointerInIdtEntry (MailboxLocationPointer);
    break;

  case DEBUG_AGENT_INIT_PEI:
    if (Context == NULL) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Input parameter Context cannot be NULL!\n"));
      CpuDeadLoop ();
    }
    //
    // Check if Debug Agent has initialized before
    //
    if (IsDebugAgentInitialzed()) {
      DEBUG ((EFI_D_WARN, "Debug Agent: It has already initialized in SEC Core!\n"));
      break;
    }
    //
    // Install Vector Handoff Info PPI to persist vectors used by Debug Agent
    //
    Status = PeiServicesInstallPpi (&mVectorHandoffInfoPpiList[0]);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Failed to install Vector Handoff Info PPI!\n"));
      CpuDeadLoop ();
    }
    //
    // Set up IDT entries
    //
    InitializeDebugIdt ();
    //
    // Build mailbox in HOB and setup Mailbox Set In Pei flag
    //
    Mailbox = AllocateZeroPool (sizeof (DEBUG_AGENT_MAILBOX));
    if (Mailbox == NULL) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Failed to allocate memory!\n"));
      CpuDeadLoop ();
    } else {
      MailboxLocation = (UINT64)(UINTN)Mailbox;
      MailboxLocationPointer = BuildGuidDataHob (
                                 &gEfiDebugAgentGuid,
                                 &MailboxLocation,
                                 sizeof (UINT64)
                                 );
      //
      // Initialize Debug Timer hardware and save its frequency
      //
      InitializeDebugTimer (&DebugTimerFrequency, TRUE);
      UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_TIMER_FREQUENCY, DebugTimerFrequency);
      //
      // Update IDT entry to save the location pointer saved mailbox pointer
      //
      SetLocationSavedMailboxPointerInIdtEntry (MailboxLocationPointer);
    }
    //
    // Save init arch type when debug agent initialized
    //
    SetDebugFlag (DEBUG_AGENT_FLAG_INIT_ARCH, DEBUG_ARCH_SYMBOL);
    //
    // Register for a callback once memory has been initialized.
    // If memory has been ready, the callback function will be invoked immediately
    //
    Status = PeiServicesNotifyPpi (&mDebugAgentMemoryDiscoveredNotifyList[0]);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Failed to register memory discovered callback function!\n"));
      CpuDeadLoop ();
    }
    //
    // Set HOB check flag if memory has not been ready yet
    //
    if (GetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY) == 0) {
      SetDebugFlag (DEBUG_AGENT_FLAG_CHECK_MAILBOX_IN_HOB, 1);
    }

    Phase2Context.InitFlag = InitFlag;
    Phase2Context.Context  = Context;
    Phase2Context.Function = Function;
    DebugPortInitialize ((VOID *) &Phase2Context, InitializeDebugAgentPhase2);

    FindAndReportModuleImageInfo (4);

    break;

  case DEBUG_AGENT_INIT_THUNK_PEI_IA32TOX64:
    if (Context == NULL) {
      DEBUG ((EFI_D_ERROR, "DebugAgent: Input parameter Context cannot be NULL!\n"));
      CpuDeadLoop ();
    } else {
      Ia32Idtr =  (IA32_DESCRIPTOR *) Context;
      Ia32IdtEntry = (IA32_IDT_ENTRY *)(Ia32Idtr->Base);
      MailboxLocationPointer = (UINT64 *) ((UINTN) Ia32IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetLow +
                                          ((UINTN) Ia32IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetHigh << 16));
      Mailbox = (DEBUG_AGENT_MAILBOX *) (UINTN)(*MailboxLocationPointer);
      //
      // Mailbox should valid and setup before executing thunk code
      //
      VerifyMailboxChecksum (Mailbox);

      DebugPortHandle = (UINT64) (UINTN)DebugPortInitialize ((VOID *)(UINTN)Mailbox->DebugPortHandle, NULL);
      UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, DebugPortHandle);
      //
      // Set up IDT entries
      //
      InitializeDebugIdt ();
      //
      // Update IDT entry to save location pointer saved the mailbox pointer
      //
      SetLocationSavedMailboxPointerInIdtEntry (MailboxLocationPointer);

      FindAndReportModuleImageInfo (4);
    }
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

  if (InitFlag == DEBUG_AGENT_INIT_POSTMEM_SEC) {
    //
    // Restore CPU Interrupt state and keep debug timer interrupt state as is
    // in DEBUG_AGENT_INIT_POSTMEM_SEC case
    //
    SetInterruptState (CpuInterruptState);
  } else {
    //
    // Enable Debug Timer interrupt
    //
    SaveAndSetDebugTimerInterrupt (TRUE);
    //
    // Enable CPU interrupts so debug timer interrupts can be delivered
    //
    EnableInterrupts ();
  }
  //
  // If Function is not NULL, invoke it always whatever debug agent was initialized successfully or not.
  //
  if (Function != NULL) {
    Function (Context);
  }
  //
  // Set return status for DEBUG_AGENT_INIT_PEI
  //
  if (InitFlag == DEBUG_AGENT_INIT_PEI && Context != NULL) {
    *(EFI_STATUS *)Context = EFI_SUCCESS;
  }
}

/**
  Caller provided function to be invoked at the end of DebugPortInitialize().

  Refer to the description for DebugPortInitialize() for more details.

  @param[in] Context           The first input argument of DebugPortInitialize().
  @param[in] DebugPortHandle   Debug port handle created by Debug Communication Library.

**/
VOID
EFIAPI
InitializeDebugAgentPhase2 (
  IN VOID                  *Context,
  IN DEBUG_PORT_HANDLE     DebugPortHandle
  )
{
  DEBUG_AGENT_PHASE2_CONTEXT *Phase2Context;
  UINT64                     *MailboxLocation;
  DEBUG_AGENT_MAILBOX        *Mailbox;
  EFI_SEC_PEI_HAND_OFF       *SecCoreData;
  UINT16                     BufferSize;
  UINT64                     NewDebugPortHandle;

  Phase2Context = (DEBUG_AGENT_PHASE2_CONTEXT *) Context;
  MailboxLocation = GetLocationSavedMailboxPointerInIdtEntry ();
  Mailbox = (DEBUG_AGENT_MAILBOX *)(UINTN)(*MailboxLocation);
  BufferSize = PcdGet16(PcdDebugPortHandleBufferSize);
  if (Phase2Context->InitFlag == DEBUG_AGENT_INIT_PEI && BufferSize != 0) {
    NewDebugPortHandle = (UINT64)(UINTN)AllocateCopyPool (BufferSize, DebugPortHandle);
  } else {
    NewDebugPortHandle = (UINT64)(UINTN)DebugPortHandle;
  }
  UpdateMailboxContent (Mailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, NewDebugPortHandle);

  //
  // Trigger one software interrupt to inform HOST
  //
  TriggerSoftInterrupt (SYSTEM_RESET_SIGNATURE);

  if (Phase2Context->InitFlag == DEBUG_AGENT_INIT_PREMEM_SEC) {
    //
    // If Temporary RAM region is below 128 MB, then send message to
    // host to disable low memory filtering.
    //
    SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Phase2Context->Context;
    if ((UINTN)SecCoreData->TemporaryRamBase < BASE_128MB && IsHostAttached ()) {
      SetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY, 1);
      TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
    }
    //
    // Enable Debug Timer interrupt
    //
    SaveAndSetDebugTimerInterrupt (TRUE);
    //
    // Enable CPU interrupts so debug timer interrupts can be delivered
    //
    EnableInterrupts ();
    //
    // Call continuation function if it is not NULL.
    //
    Phase2Context->Function (Phase2Context->Context);
  }
}
