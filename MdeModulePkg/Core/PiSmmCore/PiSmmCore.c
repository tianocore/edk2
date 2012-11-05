/** @file
  SMM Core Main Entry Point

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PiSmmCore.h"

//
// Physical pointer to private structure shared between SMM IPL and the SMM Core
//
SMM_CORE_PRIVATE_DATA  *gSmmCorePrivate;

//
// SMM Core global variable for SMM System Table.  Only accessed as a physical structure in SMRAM.
//
EFI_SMM_SYSTEM_TABLE2  gSmmCoreSmst = {
  {
    SMM_SMST_SIGNATURE,
    EFI_SMM_SYSTEM_TABLE2_REVISION,
    sizeof (gSmmCoreSmst.Hdr)
  },
  NULL,                          // SmmFirmwareVendor
  0,                             // SmmFirmwareRevision
  SmmInstallConfigurationTable,
  {
    {
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5,       // SmmMemRead
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5        // SmmMemWrite
    },
    {
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5,       // SmmIoRead
      (EFI_SMM_CPU_IO2) SmmEfiNotAvailableYetArg5        // SmmIoWrite
    }
  },
  SmmAllocatePool,
  SmmFreePool,
  SmmAllocatePages,
  SmmFreePages,
  NULL,                          // SmmStartupThisAp
  0,                             // CurrentlyExecutingCpu
  0,                             // NumberOfCpus
  NULL,                          // CpuSaveStateSize
  NULL,                          // CpuSaveState
  0,                             // NumberOfTableEntries
  NULL,                          // SmmConfigurationTable
  SmmInstallProtocolInterface,
  SmmUninstallProtocolInterface,
  SmmHandleProtocol,
  SmmRegisterProtocolNotify,
  SmmLocateHandle,
  SmmLocateProtocol,
  SmiManage,
  SmiHandlerRegister,
  SmiHandlerUnRegister
};

//
// Flag to determine if the platform has performed a legacy boot.
// If this flag is TRUE, then the runtime code and runtime data associated with the 
// SMM IPL are converted to free memory, so the SMM COre must guarantee that is
// does not touch of the code/data associated with the SMM IPL if this flag is TRUE.
//
BOOLEAN  mInLegacyBoot = FALSE;

//
// Table of SMI Handlers that are registered by the SMM Core when it is initialized
//
SMM_CORE_SMI_HANDLERS  mSmmCoreSmiHandlers[] = {
  { SmmDriverDispatchHandler, &gEfiEventDxeDispatchGuid,          NULL, TRUE  },
  { SmmReadyToLockHandler,    &gEfiDxeSmmReadyToLockProtocolGuid, NULL, FALSE }, 
  { SmmLegacyBootHandler,     &gEfiEventLegacyBootGuid,           NULL, FALSE },
  { NULL,                     NULL,                               NULL, FALSE }
};

/**
  Place holder function until all the SMM System Table Service are available.

  Note: This function is only used by SMRAM invocation.  It is never used by DXE invocation.

  @param  Arg1                   Undefined
  @param  Arg2                   Undefined
  @param  Arg3                   Undefined
  @param  Arg4                   Undefined
  @param  Arg5                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
SmmEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  )
{
  //
  // This function should never be executed.  If it does, then the architectural protocols
  // have not been designed correctly.
  //
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Software SMI handler that is called when a Legacy Boot event is signalled.  The SMM
  Core uses this signal to know that a Legacy Boot has been performed and that 
  gSmmCorePrivate that is shared between the UEFI and SMM execution environments can
  not be accessed from SMM anymore since that structure is considered free memory by
  a legacy OS.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmLegacyBootHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  mInLegacyBoot = TRUE;
  return EFI_SUCCESS;
}

/**
  Software SMI handler that is called when the DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.  This function unregisters the 
  Software SMIs that are nor required after SMRAM is locked and installs the 
  SMM Ready To Lock Protocol so SMM Drivers are informed that SMRAM is about 
  to be locked.  It also verifies the the SMM CPU I/O 2 Protocol has been installed
  and NULLs gBS and gST because they can not longer be used after SMRAM is locked.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToLockHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_HANDLE  SmmHandle;
  VOID        *Interface;

  //
  // Unregister SMI Handlers that are no required after the SMM driver dispatch is stopped
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    if (mSmmCoreSmiHandlers[Index].UnRegister) {
      SmiHandlerUnRegister (mSmmCoreSmiHandlers[Index].DispatchHandle);
    }
  }

  //
  // Install SMM Ready to lock protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEfiSmmReadyToLockProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  //
  // Make sure SMM CPU I/O 2 Procol has been installed into the handle database
  //
  Status = SmmLocateProtocol (&gEfiSmmCpuIo2ProtocolGuid, NULL, &Interface);

  //
  // Print a message on a debug build if the SMM CPU I/O 2 Protocol is not installed
  //
  DEBUG_CODE_BEGIN ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "\nSMM: SmmCpuIo Arch Protocol not present!!\n"));
    }
  DEBUG_CODE_END ();

  //
  // Assert if the CPU I/O 2 Protocol is not installed
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Display any drivers that were not dispatched because dependency expression
  // evaluated to false if this is a debug build
  //
  DEBUG_CODE_BEGIN ();
    SmmDisplayDiscoveredNotDispatched ();
  DEBUG_CODE_END ();

  //
  // Not allowed to use gST or gBS after lock
  //
  gST = NULL;
  gBS = NULL;

  return Status;
}

/**
  The main entry point to SMM Foundation.

  Note: This function is only used by SMRAM invocation.  It is never used by DXE invocation.

  @param  SmmEntryContext           Processor information and functionality
                                    needed by SMM Foundation.

**/
VOID
EFIAPI
SmmEntryPoint (
  IN CONST EFI_SMM_ENTRY_CONTEXT  *SmmEntryContext
)
{
  EFI_STATUS                  Status;
  EFI_SMM_COMMUNICATE_HEADER  *CommunicateHeader;
  BOOLEAN                     InLegacyBoot;

  PERF_START (NULL, "SMM", NULL, 0) ;

  //
  // Update SMST using the context
  //
  CopyMem (&gSmmCoreSmst.SmmStartupThisAp, SmmEntryContext, sizeof (EFI_SMM_ENTRY_CONTEXT));

  //
  // Call platform hook before Smm Dispatch
  //
  PlatformHookBeforeSmmDispatch ();

  //
  // If a legacy boot has occured, then make sure gSmmCorePrivate is not accessed
  //
  InLegacyBoot = mInLegacyBoot;
  if (!InLegacyBoot) {
    //
    // Mark the InSmm flag as TRUE, it will be used by SmmBase2 protocol
    //
    gSmmCorePrivate->InSmm = TRUE;

    //
    // Check to see if this is a Synchronous SMI sent through the SMM Communication 
    // Protocol or an Asynchronous SMI
    //
    if (gSmmCorePrivate->CommunicationBuffer != NULL) {
      //
      // Synchronous SMI for SMM Core or request from Communicate protocol
      //
      CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)gSmmCorePrivate->CommunicationBuffer;
      gSmmCorePrivate->BufferSize -= OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
      Status = SmiManage (
                 &CommunicateHeader->HeaderGuid, 
                 NULL, 
                 CommunicateHeader->Data, 
                 &gSmmCorePrivate->BufferSize
                 );

      //
      // Update CommunicationBuffer, BufferSize and ReturnStatus
      // Communicate service finished, reset the pointer to CommBuffer to NULL
      //
      gSmmCorePrivate->BufferSize += OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
      gSmmCorePrivate->CommunicationBuffer = NULL;
      gSmmCorePrivate->ReturnStatus = (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
    }
  }

  //
  // Process Asynchronous SMI sources
  //
  SmiManage (NULL, NULL, NULL, NULL);
  
  //
  // Call platform hook after Smm Dispatch
  //
  PlatformHookAfterSmmDispatch ();

  //
  // If a legacy boot has occured, then make sure gSmmCorePrivate is not accessed
  //
  if (!InLegacyBoot) {
    //
    // Clear the InSmm flag as we are going to leave SMM
    //
    gSmmCorePrivate->InSmm = FALSE;
  }

  PERF_END (NULL, "SMM", NULL, 0) ;
}

/**
  The Entry Point for SMM Core

  Install DXE Protocols and reload SMM Core into SMRAM and register SMM Core 
  EntryPoint on the SMI vector.

  Note: This function is called for both DXE invocation and SMRAM invocation.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval Other          Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
SmmMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  //
  // Get SMM Core Private context passed in from SMM IPL in ImageHandle.
  //
  gSmmCorePrivate = (SMM_CORE_PRIVATE_DATA *)ImageHandle;

  //
  // Fill in SMRAM physical address for the SMM Services Table and the SMM Entry Point.
  //
  gSmmCorePrivate->Smst          = &gSmmCoreSmst;
  gSmmCorePrivate->SmmEntryPoint = SmmEntryPoint;
  
  //
  // Initialize memory service using free SMRAM
  //
  SmmInitializeMemoryServices (gSmmCorePrivate->SmramRangeCount, gSmmCorePrivate->SmramRanges);

  //
  // Register all SMI Handlers required by the SMM Core
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    Status = SmiHandlerRegister (
               mSmmCoreSmiHandlers[Index].Handler,
               mSmmCoreSmiHandlers[Index].HandlerType,
               &mSmmCoreSmiHandlers[Index].DispatchHandle
               );
    ASSERT_EFI_ERROR (Status);
  }
  
  return EFI_SUCCESS;
}
