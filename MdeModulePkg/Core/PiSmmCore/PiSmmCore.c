/** @file
  SMM Core Main Entry Point

  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
// SMM IPL are converted to free memory, so the SMM Core must guarantee that is
// does not touch of the code/data associated with the SMM IPL if this flag is TRUE.
//
BOOLEAN  mInLegacyBoot = FALSE;

//
// Flag to determine if it is during S3 resume.
// It will be set in S3 entry callback and cleared at EndOfS3Resume.
//
BOOLEAN  mDuringS3Resume = FALSE;

//
// Flag to determine if platform enabled S3.
// Get the value from PcdAcpiS3Enable.
//
BOOLEAN  mAcpiS3Enable = FALSE;

//
// Table of SMI Handlers that are registered by the SMM Core when it is initialized
//
SMM_CORE_SMI_HANDLERS  mSmmCoreSmiHandlers[] = {
  { SmmDriverDispatchHandler,   &gEfiEventDxeDispatchGuid,           NULL, TRUE  },
  { SmmReadyToLockHandler,      &gEfiDxeSmmReadyToLockProtocolGuid,  NULL, TRUE },
  { SmmLegacyBootHandler,       &gEfiEventLegacyBootGuid,            NULL, FALSE },
  { SmmExitBootServicesHandler, &gEfiEventExitBootServicesGuid,      NULL, FALSE },
  { SmmReadyToBootHandler,      &gEfiEventReadyToBootGuid,           NULL, FALSE },
  { SmmEndOfDxeHandler,         &gEfiEndOfDxeEventGroupGuid,         NULL, TRUE },
  { NULL,                       NULL,                                NULL, FALSE }
};

//
// Table of SMI Handlers that are registered by the SMM Core when it is initialized
//
SMM_CORE_SMI_HANDLERS  mSmmCoreS3SmiHandlers[] = {
  { SmmS3SmmInitDoneHandler,    &gEdkiiS3SmmInitDoneGuid,            NULL, FALSE },
  { SmmEndOfS3ResumeHandler,    &gEdkiiEndOfS3ResumeGuid,            NULL, FALSE },
  { NULL,                       NULL,                                NULL, FALSE }
};

UINTN                           mFullSmramRangeCount;
EFI_SMRAM_DESCRIPTOR            *mFullSmramRanges;

EFI_SMM_DRIVER_ENTRY            *mSmmCoreDriverEntry;

EFI_LOADED_IMAGE_PROTOCOL       *mSmmCoreLoadedImage;

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
  a legacy OS. Then the SMM Core also install SMM Legacy Boot protocol to notify SMM
  driver that system enter legacy boot.

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
  EFI_STATUS    Status;
  EFI_HANDLE    SmmHandle;
  UINTN         Index;

  //
  // Install SMM Legacy Boot protocol.
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEdkiiSmmLegacyBootProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  mInLegacyBoot = TRUE;

  SmiHandlerUnRegister (DispatchHandle);

  //
  // It is legacy boot, unregister ExitBootService SMI handler.
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    if (CompareGuid (mSmmCoreSmiHandlers[Index].HandlerType, &gEfiEventExitBootServicesGuid)) {
      SmiHandlerUnRegister (mSmmCoreSmiHandlers[Index].DispatchHandle);
      break;
    }
  }

  return Status;
}

/**
  Software SMI handler that is called when an Exit Boot Services event is signalled.
  Then the SMM Core also install SMM Exit Boot Services protocol to notify SMM driver
  that system enter exit boot services.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmExitBootServicesHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    SmmHandle;
  UINTN         Index;

  //
  // Install SMM Exit Boot Services protocol.
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEdkiiSmmExitBootServicesProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  SmiHandlerUnRegister (DispatchHandle);

  //
  // It is UEFI boot, unregister LegacyBoot SMI handler.
  //
  for (Index = 0; mSmmCoreSmiHandlers[Index].HandlerType != NULL; Index++) {
    if (CompareGuid (mSmmCoreSmiHandlers[Index].HandlerType, &gEfiEventLegacyBootGuid)) {
      SmiHandlerUnRegister (mSmmCoreSmiHandlers[Index].DispatchHandle);
      break;
    }
  }

  return Status;
}

/**
  Main entry point for an SMM handler dispatch or communicate-based callback.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
SmmS3EntryCallBack (
  IN           EFI_HANDLE           DispatchHandle,
  IN     CONST VOID                 *Context         OPTIONAL,
  IN OUT       VOID                 *CommBuffer      OPTIONAL,
  IN OUT       UINTN                *CommBufferSize  OPTIONAL
  )
{
  mDuringS3Resume = TRUE;
  return EFI_SUCCESS;
}

/**
  Software SMI handler that is called when an Ready To Boot event is signalled.
  Then the SMM Core also install SMM Ready To Boot protocol to notify SMM driver
  that system enter ready to boot.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToBootHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        SmmHandle;

  //
  // Install SMM Ready To Boot protocol.
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEdkiiSmmReadyToBootProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  SmiHandlerUnRegister (DispatchHandle);

  return Status;
}

/**
  Software SMI handler that is called when the DxeSmmReadyToLock protocol is added
  or if gEfiEventReadyToBootGuid is signalled.  This function unregisters the
  Software SMIs that are nor required after SMRAM is locked and installs the
  SMM Ready To Lock Protocol so SMM Drivers are informed that SMRAM is about
  to be locked.  It also verifies the SMM CPU I/O 2 Protocol has been installed
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

  SmramProfileReadyToLock ();

  return Status;
}

/**
  Software SMI handler that is called when the EndOfDxe event is signalled.
  This function installs the SMM EndOfDxe Protocol so SMM Drivers are informed that
  platform code will invoke 3rd part code.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfDxeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        SmmHandle;
  EFI_SMM_SX_DISPATCH2_PROTOCOL     *SxDispatch;
  EFI_SMM_SX_REGISTER_CONTEXT       EntryRegisterContext;
  EFI_HANDLE                        S3EntryHandle;

  DEBUG ((EFI_D_INFO, "SmmEndOfDxeHandler\n"));

  //
  // Install SMM EndOfDxe protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEfiSmmEndOfDxeProtocolGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );

  if (mAcpiS3Enable) {
    //
    // Locate SmmSxDispatch2 protocol.
    //
    Status = SmmLocateProtocol (
               &gEfiSmmSxDispatch2ProtocolGuid,
               NULL,
               (VOID **)&SxDispatch
               );
    if (!EFI_ERROR (Status) && (SxDispatch != NULL)) {
      //
      // Register a S3 entry callback function to
      // determine if it will be during S3 resume.
      //
      EntryRegisterContext.Type  = SxS3;
      EntryRegisterContext.Phase = SxEntry;
      Status = SxDispatch->Register (
                             SxDispatch,
                             SmmS3EntryCallBack,
                             &EntryRegisterContext,
                             &S3EntryHandle
                             );
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}

/**
  Software SMI handler that is called when the S3SmmInitDone signal is triggered.
  This function installs the SMM S3SmmInitDone Protocol so SMM Drivers are informed that
  S3 SMM initialization has been done.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmS3SmmInitDoneHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  SmmHandle;

  DEBUG ((DEBUG_INFO, "SmmS3SmmInitDoneHandler\n"));

  if (!mDuringS3Resume) {
    DEBUG ((DEBUG_ERROR, "It is not during S3 resume\n"));
    return EFI_SUCCESS;
  }

  //
  // Install SMM S3SmmInitDone protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEdkiiS3SmmInitDoneGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Uninstall the protocol here because the comsumer just hook the
  // installation event.
  //
  Status = SmmUninstallProtocolInterface (
           SmmHandle,
           &gEdkiiS3SmmInitDoneGuid,
           NULL
           );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Software SMI handler that is called when the EndOfS3Resume signal is triggered.
  This function installs the SMM EndOfS3Resume Protocol so SMM Drivers are informed that
  S3 resume has finished.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfS3ResumeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  SmmHandle;

  DEBUG ((DEBUG_INFO, "SmmEndOfS3ResumeHandler\n"));

  if (!mDuringS3Resume) {
    DEBUG ((DEBUG_ERROR, "It is not during S3 resume\n"));
    return EFI_SUCCESS;
  }

  //
  // Install SMM EndOfS3Resume protocol
  //
  SmmHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &SmmHandle,
             &gEdkiiEndOfS3ResumeGuid,
             EFI_NATIVE_INTERFACE,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Uninstall the protocol here because the comsumer just hook the
  // installation event.
  //
  Status = SmmUninstallProtocolInterface (
           SmmHandle,
           &gEdkiiEndOfS3ResumeGuid,
           NULL
           );
  ASSERT_EFI_ERROR (Status);

  mDuringS3Resume = FALSE;
  return Status;
}

/**
  Determine if two buffers overlap in memory.

  @param[in] Buff1  Pointer to first buffer
  @param[in] Size1  Size of Buff1
  @param[in] Buff2  Pointer to second buffer
  @param[in] Size2  Size of Buff2

  @retval TRUE      Buffers overlap in memory.
  @retval FALSE     Buffer doesn't overlap.

**/
BOOLEAN
InternalIsBufferOverlapped (
  IN UINT8      *Buff1,
  IN UINTN      Size1,
  IN UINT8      *Buff2,
  IN UINTN      Size2
  )
{
  //
  // If buff1's end is less than the start of buff2, then it's ok.
  // Also, if buff1's start is beyond buff2's end, then it's ok.
  //
  if (((Buff1 + Size1) <= Buff2) || (Buff1 >= (Buff2 + Size2))) {
    return FALSE;
  }

  return TRUE;
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
  BOOLEAN                     IsOverlapped;
  VOID                        *CommunicationBuffer;
  UINTN                       BufferSize;

  //
  // Update SMST with contents of the SmmEntryContext structure
  //
  gSmmCoreSmst.SmmStartupThisAp      = SmmEntryContext->SmmStartupThisAp;
  gSmmCoreSmst.CurrentlyExecutingCpu = SmmEntryContext->CurrentlyExecutingCpu;
  gSmmCoreSmst.NumberOfCpus          = SmmEntryContext->NumberOfCpus;
  gSmmCoreSmst.CpuSaveStateSize      = SmmEntryContext->CpuSaveStateSize;
  gSmmCoreSmst.CpuSaveState          = SmmEntryContext->CpuSaveState;

  //
  // Call platform hook before Smm Dispatch
  //
  PlatformHookBeforeSmmDispatch ();

  //
  // Call memory management hook function
  //
  SmmEntryPointMemoryManagementHook ();

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
    CommunicationBuffer = gSmmCorePrivate->CommunicationBuffer;
    BufferSize          = gSmmCorePrivate->BufferSize;
    if (CommunicationBuffer != NULL) {
      //
      // Synchronous SMI for SMM Core or request from Communicate protocol
      //
      IsOverlapped = InternalIsBufferOverlapped (
                       (UINT8 *) CommunicationBuffer,
                       BufferSize,
                       (UINT8 *) gSmmCorePrivate,
                       sizeof (*gSmmCorePrivate)
                       );
      if (!SmmIsBufferOutsideSmmValid ((UINTN)CommunicationBuffer, BufferSize) || IsOverlapped) {
        //
        // If CommunicationBuffer is not in valid address scope,
        // or there is overlap between gSmmCorePrivate and CommunicationBuffer,
        // return EFI_INVALID_PARAMETER
        //
        gSmmCorePrivate->CommunicationBuffer = NULL;
        gSmmCorePrivate->ReturnStatus = EFI_ACCESS_DENIED;
      } else {
        CommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *)CommunicationBuffer;
        BufferSize -= OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
        Status = SmiManage (
                   &CommunicateHeader->HeaderGuid,
                   NULL,
                   CommunicateHeader->Data,
                   &BufferSize
                   );
        //
        // Update CommunicationBuffer, BufferSize and ReturnStatus
        // Communicate service finished, reset the pointer to CommBuffer to NULL
        //
        gSmmCorePrivate->BufferSize = BufferSize + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data);
        gSmmCorePrivate->CommunicationBuffer = NULL;
        gSmmCorePrivate->ReturnStatus = (Status == EFI_SUCCESS) ? EFI_SUCCESS : EFI_NOT_FOUND;
      }
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
}

/**
  Install LoadedImage protocol for SMM Core.
**/
VOID
SmmCoreInstallLoadedImage (
  VOID
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 Handle;

  //
  // Allocate a Loaded Image Protocol in EfiBootServicesData
  //
  Status = gBS->AllocatePool (EfiBootServicesData, sizeof(EFI_LOADED_IMAGE_PROTOCOL), (VOID **)&mSmmCoreLoadedImage);
  ASSERT_EFI_ERROR (Status);

  ZeroMem (mSmmCoreLoadedImage, sizeof (EFI_LOADED_IMAGE_PROTOCOL));
  //
  // Fill in the remaining fields of the Loaded Image Protocol instance.
  // Note: ImageBase is an SMRAM address that can not be accessed outside of SMRAM if SMRAM window is closed.
  //
  mSmmCoreLoadedImage->Revision      = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  mSmmCoreLoadedImage->ParentHandle  = gSmmCorePrivate->SmmIplImageHandle;
  mSmmCoreLoadedImage->SystemTable   = gST;

  mSmmCoreLoadedImage->ImageBase     = (VOID *)(UINTN)gSmmCorePrivate->PiSmmCoreImageBase;
  mSmmCoreLoadedImage->ImageSize     = gSmmCorePrivate->PiSmmCoreImageSize;
  mSmmCoreLoadedImage->ImageCodeType = EfiRuntimeServicesCode;
  mSmmCoreLoadedImage->ImageDataType = EfiRuntimeServicesData;

  //
  // Create a new image handle in the UEFI handle database for the SMM Driver
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiLoadedImageProtocolGuid, mSmmCoreLoadedImage,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Allocate a Loaded Image Protocol in SMM
  //
  Status = SmmAllocatePool (EfiRuntimeServicesData, sizeof(EFI_SMM_DRIVER_ENTRY), (VOID **)&mSmmCoreDriverEntry);
  ASSERT_EFI_ERROR(Status);

  ZeroMem (mSmmCoreDriverEntry, sizeof(EFI_SMM_DRIVER_ENTRY));
  //
  // Fill in the remaining fields of the Loaded Image Protocol instance.
  //
  mSmmCoreDriverEntry->Signature = EFI_SMM_DRIVER_ENTRY_SIGNATURE;
  mSmmCoreDriverEntry->SmmLoadedImage.Revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  mSmmCoreDriverEntry->SmmLoadedImage.ParentHandle = gSmmCorePrivate->SmmIplImageHandle;
  mSmmCoreDriverEntry->SmmLoadedImage.SystemTable = gST;

  mSmmCoreDriverEntry->SmmLoadedImage.ImageBase = (VOID *)(UINTN)gSmmCorePrivate->PiSmmCoreImageBase;
  mSmmCoreDriverEntry->SmmLoadedImage.ImageSize = gSmmCorePrivate->PiSmmCoreImageSize;
  mSmmCoreDriverEntry->SmmLoadedImage.ImageCodeType = EfiRuntimeServicesCode;
  mSmmCoreDriverEntry->SmmLoadedImage.ImageDataType = EfiRuntimeServicesData;

  mSmmCoreDriverEntry->ImageEntryPoint = gSmmCorePrivate->PiSmmCoreEntryPoint;
  mSmmCoreDriverEntry->ImageBuffer     = gSmmCorePrivate->PiSmmCoreImageBase;
  mSmmCoreDriverEntry->NumberOfPage    = EFI_SIZE_TO_PAGES((UINTN)gSmmCorePrivate->PiSmmCoreImageSize);

  //
  // Create a new image handle in the SMM handle database for the SMM Driver
  //
  mSmmCoreDriverEntry->SmmImageHandle = NULL;
  Status = SmmInstallProtocolInterface (
             &mSmmCoreDriverEntry->SmmImageHandle,
             &gEfiLoadedImageProtocolGuid,
             EFI_NATIVE_INTERFACE,
             &mSmmCoreDriverEntry->SmmLoadedImage
             );
  ASSERT_EFI_ERROR(Status);

  return ;
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
  // No need to initialize memory service.
  // It is done in constructor of PiSmmCoreMemoryAllocationLib(),
  // so that the library linked with PiSmmCore can use AllocatePool() in constuctor.
  //

  SmramProfileInit ();

  //
  // Copy FullSmramRanges to SMRAM
  //
  mFullSmramRangeCount = gSmmCorePrivate->SmramRangeCount;
  mFullSmramRanges = AllocatePool (mFullSmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR));
  ASSERT (mFullSmramRanges != NULL);
  CopyMem (mFullSmramRanges, gSmmCorePrivate->SmramRanges, mFullSmramRangeCount * sizeof (EFI_SMRAM_DESCRIPTOR));

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

  mAcpiS3Enable = PcdGetBool (PcdAcpiS3Enable);
  if (mAcpiS3Enable) {
    //
    // Register all S3 related SMI Handlers required by the SMM Core
    //
    for (Index = 0; mSmmCoreS3SmiHandlers[Index].HandlerType != NULL; Index++) {
      Status = SmiHandlerRegister (
                 mSmmCoreS3SmiHandlers[Index].Handler,
                 mSmmCoreS3SmiHandlers[Index].HandlerType,
                 &mSmmCoreS3SmiHandlers[Index].DispatchHandle
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }

  RegisterSmramProfileHandler ();
  SmramProfileInstallProtocol ();

  SmmCoreInstallLoadedImage ();

  SmmCoreInitializeMemoryAttributesTable ();

  SmmCoreInitializeSmiHandlerProfile ();

  return EFI_SUCCESS;
}
