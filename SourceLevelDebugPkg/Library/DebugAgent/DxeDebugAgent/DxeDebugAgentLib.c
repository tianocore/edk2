/** @file
  Debug Agent library implementition for Dxe Core and Dxr modules.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeDebugAgentLib.h"

DEBUG_AGENT_MAILBOX          mMailbox;
DEBUG_AGENT_MAILBOX          *mMailboxPointer = NULL;
IA32_IDT_GATE_DESCRIPTOR     mIdtEntryTable[33];
BOOLEAN                      mDxeCoreFlag                = FALSE;
BOOLEAN                      mMultiProcessorDebugSupport = FALSE;
VOID                         *mSavedIdtTable             = NULL;
UINTN                        mSaveIdtTableSize           = 0;
BOOLEAN                      mDebugAgentInitialized      = FALSE;
BOOLEAN                      mSkipBreakpoint             = FALSE;

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
  return mMultiProcessorDebugSupport;
}

/**
  Internal constructor worker function.

  It will register one callback function on EFI PCD Protocol.
  It will allocate the NVS memory to store Mailbox and install configuration table
  in system table to store its pointer.

**/
VOID
InternalConstructorWorker (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        Address;
  BOOLEAN                     DebugTimerInterruptState;
  DEBUG_AGENT_MAILBOX         *Mailbox;
  DEBUG_AGENT_MAILBOX         *NewMailbox;

  //
  // Install EFI Serial IO protocol on debug port
  //
  InstallSerialIo ();

  Address = 0;
  Status = gBS->AllocatePages (
                  AllocateAnyPages,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (sizeof(DEBUG_AGENT_MAILBOX) + PcdGet16(PcdDebugPortHandleBufferSize)),
                  &Address
                  );
  ASSERT_EFI_ERROR (Status);

  DebugTimerInterruptState = SaveAndSetDebugTimerInterrupt (FALSE);

  NewMailbox = (DEBUG_AGENT_MAILBOX *) (UINTN) Address;
  //
  // Copy Mailbox and Debug Port Handle buffer to new location in ACPI NVS memory, because original Mailbox
  // and Debug Port Handle buffer may be free at runtime, SMM debug agent needs to access them
  //
  Mailbox = GetMailboxPointer ();
  CopyMem (NewMailbox, Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
  CopyMem (NewMailbox + 1, (VOID *)(UINTN)Mailbox->DebugPortHandle, PcdGet16(PcdDebugPortHandleBufferSize));
  //
  // Update Debug Port Handle in new Mailbox
  //
  UpdateMailboxContent (NewMailbox, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, (UINT64)(UINTN)(NewMailbox + 1));
  mMailboxPointer = NewMailbox;

  DebugTimerInterruptState = SaveAndSetDebugTimerInterrupt (DebugTimerInterruptState);

  Status = gBS->InstallConfigurationTable (&gEfiDebugAgentGuid, (VOID *) mMailboxPointer);
  ASSERT_EFI_ERROR (Status);
}

/**
  Debug Agent constructor function.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @retval  RETURN_SUCCESS  When this function completed.

**/
RETURN_STATUS
EFIAPI
DxeDebugAgentLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mDxeCoreFlag) {
    //
    // Invoke internal constructor function only when DXE core links this library instance
    //
    InternalConstructorWorker ();
  }

  return RETURN_SUCCESS;
}

/**
  Get the pointer to Mailbox from the configuration table.

  @return Pointer to Mailbox.

**/
DEBUG_AGENT_MAILBOX *
GetMailboxFromConfigurationTable (
  VOID
  )
{
  EFI_STATUS               Status;
  DEBUG_AGENT_MAILBOX      *Mailbox;
  
  Status = EfiGetSystemConfigurationTable (&gEfiDebugAgentGuid, (VOID **) &Mailbox);
  if (Status == EFI_SUCCESS && Mailbox != NULL) {
    VerifyMailboxChecksum (Mailbox);
    return Mailbox;
  } else {
    return NULL;
  }
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
  UINT64                   *MailboxLocation;
  DEBUG_AGENT_MAILBOX      *Mailbox;

  GuidHob = GetNextGuidHob (&gEfiDebugAgentGuid, HobStart);
  if (GuidHob == NULL) {
    return NULL;
  }
  MailboxLocation = (UINT64 *) (GET_GUID_HOB_DATA(GuidHob));
  Mailbox = (DEBUG_AGENT_MAILBOX *)(UINTN)(*MailboxLocation);
  VerifyMailboxChecksum (Mailbox);

  return Mailbox;
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
  AcquireMpSpinLock (&mDebugMpContext.MailboxSpinLock);
  VerifyMailboxChecksum (mMailboxPointer);
  ReleaseMpSpinLock (&mDebugMpContext.MailboxSpinLock);
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
  return (DEBUG_PORT_HANDLE) (UINTN)(GetMailboxPointer ()->DebugPortHandle);
}

/**
  Worker function to setup IDT table and initialize the IDT entries.

  @param[in] Mailbox        Pointer to Mailbox.

**/
VOID
SetupDebugAgentEnviroment (
  IN DEBUG_AGENT_MAILBOX       *Mailbox
  )
{
  IA32_DESCRIPTOR              Idtr;
  UINT16                       IdtEntryCount;
  UINT64                       DebugPortHandle;

  if (mMultiProcessorDebugSupport) {
    InitializeSpinLock (&mDebugMpContext.MpContextSpinLock);
    InitializeSpinLock (&mDebugMpContext.DebugPortSpinLock);
    InitializeSpinLock (&mDebugMpContext.MailboxSpinLock);
    //
    // Clear Break CPU index value
    //
    mDebugMpContext.BreakAtCpuIndex = (UINT32) -1;
  }

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

  if (Mailbox != NULL) {
    //
    // If Mailbox exists, copy it into one global variable,
    //
    CopyMem (&mMailbox, Mailbox, sizeof (DEBUG_AGENT_MAILBOX));
  } else {
    ZeroMem (&mMailbox, sizeof (DEBUG_AGENT_MAILBOX));
  }  

  mMailboxPointer = &mMailbox;
  //
  // Initialize debug communication port
  //
  DebugPortHandle = (UINT64) (UINTN)DebugPortInitialize ((VOID *)(UINTN)mMailboxPointer->DebugPortHandle, NULL);
  UpdateMailboxContent (mMailboxPointer, DEBUG_MAILBOX_DEBUG_PORT_HANDLE_INDEX, DebugPortHandle);

  if (Mailbox == NULL) {
    //
    // Trigger one software interrupt to inform HOST
    //
    TriggerSoftInterrupt (SYSTEM_RESET_SIGNATURE);
    SetDebugFlag (DEBUG_AGENT_FLAG_MEMORY_READY, 1);
    //
    // Memory has been ready
    //
    if (IsHostAttached ()) {
      //
      // Trigger one software interrupt to inform HOST
      //
      TriggerSoftInterrupt (MEMORY_READY_SIGNATURE);
    }
  }
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
  UINT64                       *MailboxLocation;
  DEBUG_AGENT_MAILBOX          *Mailbox;
  BOOLEAN                      InterruptStatus;
  VOID                         *HobList;
  IA32_DESCRIPTOR              IdtDescriptor;
  IA32_DESCRIPTOR              *Ia32Idtr;
  IA32_IDT_ENTRY               *Ia32IdtEntry;

  if (InitFlag == DEBUG_AGENT_INIT_DXE_AP) {
    //
    // Invoked by AP, enable interrupt to let AP could receive IPI from other processors
    //
    EnableInterrupts ();
    return ;
  }

  // 
  // Disable Debug Timer interrupt
  //
  SaveAndSetDebugTimerInterrupt (FALSE);
  //
  // Save and disable original interrupt status
  //
  InterruptStatus = SaveAndDisableInterrupts ();

  //
  // Try to get mailbox firstly
  //
  HobList         = NULL;
  Mailbox         = NULL;
  MailboxLocation = NULL;

  switch (InitFlag) {

  case DEBUG_AGENT_INIT_DXE_LOAD:
    //
    // Check if Debug Agent has been initialized before
    //
    if (IsDebugAgentInitialzed ()) {
      DEBUG ((EFI_D_INFO, "Debug Agent: The former agent will be overwritten by the new one!\n"));
    }

    mMultiProcessorDebugSupport = TRUE;
    //
    // Save original IDT table
    //
    AsmReadIdtr (&IdtDescriptor);
    mSaveIdtTableSize = IdtDescriptor.Limit + 1;
    mSavedIdtTable    = AllocateCopyPool (mSaveIdtTableSize, (VOID *) IdtDescriptor.Base);
    //
    // Initialize Debug Timer hardware
    //
    InitializeDebugTimer ();
    //
    // Check if Debug Agent initialized in DXE phase
    //
    Mailbox = GetMailboxFromConfigurationTable ();
    if (Mailbox == NULL) {
      //
      // Try to get mailbox from GUIDed HOB build in PEI 
      //
      HobList = GetHobList ();
      Mailbox = GetMailboxFromHob (HobList);
    }
    //
    // Set up IDT table and prepare for IDT entries
    //
    SetupDebugAgentEnviroment (Mailbox);
    //
    // For DEBUG_AGENT_INIT_S3, needn't to install configuration table and EFI Serial IO protocol
    // For DEBUG_AGENT_INIT_DXE_CORE, InternalConstructorWorker() will invoked in Constructor()
    //
    InternalConstructorWorker ();
    //
    // Enable interrupt to receive Debug Timer interrupt
    //
    EnableInterrupts ();

    mDebugAgentInitialized = TRUE;
    FindAndReportModuleImageInfo (SIZE_4KB);

    *(EFI_STATUS *)Context = EFI_SUCCESS;

    if (gST->ConOut != NULL) {
      Print (L"Debug Agent: Initialized successfully!\r\n");
      Print (L"If the Debug Port is serial port, please make sure this serial port isn't connected by ISA Serial driver\r\n");
      Print (L"You could do the following steps to disconnect the serial port:\r\n");
      Print (L"1: Shell> drivers\r\n");
      Print (L"   ...\r\n");
      Print (L"   V  VERSION  E G G #D #C DRIVER NAME                         IMAGE NAME\r\n");
      Print (L"   == ======== = = = == == =================================== ===================\r\n");
      Print (L"   8F 0000000A B - -  1 14 PCI Bus Driver                      PciBusDxe\r\n");
      Print (L"   91 00000010 ? - -  -  - ATA Bus Driver                      AtaBusDxe\r\n");
      Print (L"   ...\r\n");
      Print (L"   A7 0000000A B - -  1  1 ISA Serial Driver                   IsaSerialDxe\r\n");
      Print (L"   ...\r\n");
      Print (L"2: Shell> dh -d A7\r\n");
      Print (L"   A7: Image(IsaSerialDxe) ImageDevPath (..9FB3-11D4-9A3A-0090273FC14D))DriverBinding ComponentName ComponentName2\r\n");
      Print (L"        Driver Name    : ISA Serial Driver\r\n");
      Print (L"        Image Name     : FvFile(93B80003-9FB3-11D4-9A3A-0090273FC14D)\r\n");
      Print (L"        Driver Version : 0000000A\r\n");
      Print (L"        Driver Type    : BUS\r\n");
      Print (L"        Configuration  : NO\r\n");
      Print (L"        Diagnostics    : NO\r\n");
      Print (L"        Managing       :\r\n");
      Print (L"          Ctrl[EA] : PciRoot(0x0)/Pci(0x1F,0x0)/Serial(0x0)\r\n");
      Print (L"            Child[EB] : PciRoot(0x0)/Pci(0x1F,0x0)/Serial(0x0)/Uart(115200,8,N,1)\r\n");
      Print (L"3: Shell> disconnect EA\r\n");
      Print (L"4: Shell> load -nc DebugAgentDxe.efi\r\n\r\n");
    }
    break;

  case DEBUG_AGENT_INIT_DXE_UNLOAD:
    if (mDebugAgentInitialized) {
      if (IsHostAttached ()) {
        Print (L"Debug Agent: Host is still connected, please de-attach TARGET firstly!\r\n");
        *(EFI_STATUS *)Context = EFI_ACCESS_DENIED;
        // 
        // Enable Debug Timer interrupt again
        //
        SaveAndSetDebugTimerInterrupt (TRUE);
      } else {
        //
        // Restore original IDT table
        //
        AsmReadIdtr (&IdtDescriptor);
        IdtDescriptor.Limit = (UINT16) (mSaveIdtTableSize - 1);
        CopyMem ((VOID *) IdtDescriptor.Base, mSavedIdtTable, mSaveIdtTableSize);
        AsmWriteIdtr (&IdtDescriptor);
        FreePool (mSavedIdtTable);
        mDebugAgentInitialized = FALSE;
        *(EFI_STATUS *)Context = EFI_SUCCESS;
      }
    } else {
      Print (L"Debug Agent: It hasn't been initialized, cannot unload it!\r\n");
      *(EFI_STATUS *)Context = EFI_NOT_STARTED;
    }

    //
    // Restore interrupt state.
    //
    SetInterruptState (InterruptStatus);
    break;

  case DEBUG_AGENT_INIT_DXE_CORE:
    mDxeCoreFlag                = TRUE;
    mMultiProcessorDebugSupport = TRUE;
    //
    // Initialize Debug Timer hardware
    //
    InitializeDebugTimer ();
    //
    // Try to get mailbox from GUIDed HOB build in PEI 
    //
    HobList = Context;
    Mailbox = GetMailboxFromHob (HobList);
    //
    // Set up IDT table and prepare for IDT entries
    //
    SetupDebugAgentEnviroment (Mailbox);
    //
    // Enable interrupt to receive Debug Timer interrupt
    //
    EnableInterrupts ();

    break;

  case DEBUG_AGENT_INIT_S3:

    if (Context != NULL) {
      Ia32Idtr =  (IA32_DESCRIPTOR *) Context;
      Ia32IdtEntry = (IA32_IDT_ENTRY *)(Ia32Idtr->Base);
      MailboxLocation = (UINT64 *) (UINTN) (Ia32IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetLow + 
                                           (Ia32IdtEntry[DEBUG_MAILBOX_VECTOR].Bits.OffsetHigh << 16));
      Mailbox = (DEBUG_AGENT_MAILBOX *)(UINTN)(*MailboxLocation);
      VerifyMailboxChecksum (Mailbox);
    }
    //
    // Set up IDT table and prepare for IDT entries
    //
    SetupDebugAgentEnviroment (Mailbox);
    //
    // Disable interrupt
    //
    DisableInterrupts ();
    FindAndReportModuleImageInfo (SIZE_4KB);
    if (GetDebugFlag (DEBUG_AGENT_FLAG_BREAK_BOOT_SCRIPT) == 1) {
      //
      // If Boot Script entry break is set, code will be break at here.
      //
      CpuBreakpoint ();
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
}
