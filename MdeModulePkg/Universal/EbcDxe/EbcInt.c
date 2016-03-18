/** @file
  Top level module for the EBC virtual machine implementation.
  Provides auxiliary support routines for the VM. That is, routines
  that are not particularly related to VM execution of EBC instructions.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EbcInt.h"
#include "EbcExecute.h"

//
// We'll keep track of all thunks we create in a linked list. Each
// thunk is tied to an image handle, so we have a linked list of
// image handles, with each having a linked list of thunks allocated
// to that image handle.
//
typedef struct _EBC_THUNK_LIST EBC_THUNK_LIST;
struct _EBC_THUNK_LIST {
  VOID            *ThunkBuffer;
  EBC_THUNK_LIST  *Next;
};

typedef struct _EBC_IMAGE_LIST EBC_IMAGE_LIST;
struct _EBC_IMAGE_LIST {
  EBC_IMAGE_LIST  *Next;
  EFI_HANDLE      ImageHandle;
  EBC_THUNK_LIST  *ThunkList;
};

/**
  This routine is called by the core when an image is being unloaded from
  memory. Basically we now have the opportunity to do any necessary cleanup.
  Typically this will include freeing any memory allocated for thunk-creation.

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle           Handle of image for which the thunk is being
                                created.

  @retval EFI_INVALID_PARAMETER The ImageHandle passed in was not found in the
                                internal list of EBC image handles.
  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle
  );

/**
  This is the top-level routine plugged into the EBC protocol. Since thunks
  are very processor-specific, from here we dispatch directly to the very
  processor-specific routine EbcCreateThunks().

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle           Handle of image for which the thunk is being
                                created. The EBC interpreter may use this to
                                keep track of any resource allocations
                                performed in loading and executing the image.
  @param  EbcEntryPoint         Address of the actual EBC entry point or
                                protocol service the thunk should call.
  @param  Thunk                 Returned pointer to a thunk created.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Image entry point is not 2-byte aligned.
  @retval EFI_OUT_OF_RESOURCES  Memory could not be allocated for the thunk.

**/
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle,
  IN VOID               *EbcEntryPoint,
  OUT VOID              **Thunk
  );

/**
  Called to get the version of the interpreter.

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  Version               Pointer to where to store the returned version
                                of the interpreter.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Version pointer is NULL.

**/
EFI_STATUS
EFIAPI
EbcGetVersion (
  IN EFI_EBC_PROTOCOL   *This,
  IN OUT UINT64         *Version
  );

/**
  To install default Callback function for the VM interpreter.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval Others                Some error occurs when creating periodic event.

**/
EFI_STATUS
EFIAPI
InitializeEbcCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This
  );

/**
  The default Exception Callback for the VM interpreter.
  In this function, we report status code, and print debug information
  about EBC_CONTEXT, then dead loop.

  @param  InterruptType          Interrupt type.
  @param  SystemContext          EBC system context.

**/
VOID
EFIAPI
CommonEbcExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

/**
  The periodic callback function for EBC VM interpreter, which is used
  to support the EFI debug support protocol.

  @param  Event                  The Periodic Callback Event.
  @param  Context                It should be the address of VM_CONTEXT pointer.

**/
VOID
EFIAPI
EbcPeriodicNotifyFunction (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

/**
  The VM interpreter calls this function on a periodic basis to support
  the EFI debug support protocol.

  @param  VmPtr                  Pointer to a VM context for passing info to the
                                 debugger.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugPeriodic (
  IN VM_CONTEXT *VmPtr
  );

//
// These two functions and the  GUID are used to produce an EBC test protocol.
// This functionality is definitely not required for execution.
//
/**
  Produces an EBC VM test protocol that can be used for regression tests.

  @param  IHandle                Handle on which to install the protocol.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *IHandle
  );

/**
  Returns the EFI_UNSUPPORTED Status.

  @return EFI_UNSUPPORTED  This function always return EFI_UNSUPPORTED status.

**/
EFI_STATUS
EFIAPI
EbcVmTestUnsupported (
  VOID
  );

/**
  Registers a callback function that the EBC interpreter calls to flush the
  processor instruction cache following creation of thunks.

  @param  This        A pointer to the EFI_EBC_PROTOCOL instance.
  @param  Flush       Pointer to a function of type EBC_ICACH_FLUSH.

  @retval EFI_SUCCESS The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcRegisterICacheFlush (
  IN EFI_EBC_PROTOCOL   *This,
  IN EBC_ICACHE_FLUSH   Flush
  );

/**
  This EBC debugger protocol service is called by the debug agent

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  MaxProcessorIndex     Pointer to a caller-allocated UINTN in which the
                                maximum supported processor index is returned.

  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  OUT UINTN                              *MaxProcessorIndex
  );

/**
  This protocol service is called by the debug agent to register a function
  for us to call on a periodic basis.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  PeriodicCallback      A pointer to a function of type
                                PERIODIC_CALLBACK that is the main periodic
                                entry point of the debug agent. It receives as a
                                parameter a pointer to the full context of the
                                interrupted execution thread.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a
                                callback function was previously registered.
  @retval EFI_INVALID_PARAMETER Null PeriodicCallback parameter when no
                                callback function was previously registered.

**/
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK       PeriodicCallback
  );

/**
  This protocol service is called by the debug agent to register a function
  for us to call when we detect an exception.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  ExceptionCallback     A pointer to a function of type
                                EXCEPTION_CALLBACK that is called when the
                                processor exception specified by ExceptionType
                                occurs. Passing NULL unregisters any previously
                                registered function associated with
                                ExceptionType.
  @param  ExceptionType         Specifies which processor exception to hook.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL ExceptionCallback parameter when a
                                callback function was previously registered.
  @retval EFI_INVALID_PARAMETER ExceptionType parameter is negative or exceeds
                                MAX_EBC_EXCEPTION.
  @retval EFI_INVALID_PARAMETER Null ExceptionCallback parameter when no
                                callback function was previously registered.

**/
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN EFI_EXCEPTION_TYPE          ExceptionType
  );

/**
  This EBC debugger protocol service is called by the debug agent.  Required
  for DebugSupport compliance but is only stubbed out for EBC.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  Start                 StartSpecifies the physical base of the memory
                                range to be invalidated.
  @param  Length                Specifies the minimum number of bytes in the
                                processor's instruction cache to invalidate.

  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  IN UINTN                               ProcessorIndex,
  IN VOID                                *Start,
  IN UINT64                              Length
  );

//
// We have one linked list of image handles for the whole world. Since
// there should only be one interpreter, make them global. They must
// also be global since the execution of an EBC image does not provide
// a This pointer.
//
EBC_IMAGE_LIST         *mEbcImageList = NULL;

//
// Callback function to flush the icache after thunk creation
//
EBC_ICACHE_FLUSH       mEbcICacheFlush;

//
// These get set via calls by the debug agent
//
EFI_PERIODIC_CALLBACK  mDebugPeriodicCallback = NULL;
EFI_EXCEPTION_CALLBACK mDebugExceptionCallback[MAX_EBC_EXCEPTION + 1] = {NULL};

VOID                   *mStackBuffer[MAX_STACK_NUM];
EFI_HANDLE             mStackBufferIndex[MAX_STACK_NUM];
UINTN                  mStackNum = 0;

//
// Event for Periodic callback
//
EFI_EVENT              mEbcPeriodicEvent;
VM_CONTEXT             *mVmPtr = NULL;


/**
  Initializes the VM EFI interface.  Allocates memory for the VM interface
  and registers the VM protocol.

  @param  ImageHandle            EFI image handle.
  @param  SystemTable            Pointer to the EFI system table.

  @return Standard EFI status code.

**/
EFI_STATUS
EFIAPI
InitializeEbcDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_EBC_PROTOCOL            *EbcProtocol;
  EFI_EBC_PROTOCOL            *OldEbcProtocol;
  EFI_STATUS                  Status;
  EFI_DEBUG_SUPPORT_PROTOCOL  *EbcDebugProtocol;
  EFI_HANDLE                  *HandleBuffer;
  UINTN                       NumHandles;
  UINTN                       Index;
  BOOLEAN                     Installed;

  EbcProtocol      = NULL;
  EbcDebugProtocol = NULL;

  //
  // Allocate memory for our protocol. Then fill in the blanks.
  //
  EbcProtocol = AllocatePool (sizeof (EFI_EBC_PROTOCOL));

  if (EbcProtocol == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EbcProtocol->CreateThunk          = EbcCreateThunk;
  EbcProtocol->UnloadImage          = EbcUnloadImage;
  EbcProtocol->RegisterICacheFlush  = EbcRegisterICacheFlush;
  EbcProtocol->GetVersion           = EbcGetVersion;
  mEbcICacheFlush                   = NULL;

  //
  // Find any already-installed EBC protocols and uninstall them
  //
  Installed     = FALSE;
  HandleBuffer  = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiEbcProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (Status == EFI_SUCCESS) {
    //
    // Loop through the handles
    //
    for (Index = 0; Index < NumHandles; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiEbcProtocolGuid,
                      (VOID **) &OldEbcProtocol
                      );
      if (Status == EFI_SUCCESS) {
        if (gBS->ReinstallProtocolInterface (
                  HandleBuffer[Index],
                  &gEfiEbcProtocolGuid,
                  OldEbcProtocol,
                  EbcProtocol
                  ) == EFI_SUCCESS) {
          Installed = TRUE;
        }
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }
  //
  // Add the protocol so someone can locate us if we haven't already.
  //
  if (!Installed) {
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEfiEbcProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    EbcProtocol
                    );
    if (EFI_ERROR (Status)) {
      FreePool (EbcProtocol);
      return Status;
    }
  }

  Status = InitEBCStack();
  if (EFI_ERROR(Status)) {
    goto ErrorExit;
  }

  //
  // Allocate memory for our debug protocol. Then fill in the blanks.
  //
  EbcDebugProtocol = AllocatePool (sizeof (EFI_DEBUG_SUPPORT_PROTOCOL));

  if (EbcDebugProtocol == NULL) {
    goto ErrorExit;
  }

  EbcDebugProtocol->Isa                         = IsaEbc;
  EbcDebugProtocol->GetMaximumProcessorIndex    = EbcDebugGetMaximumProcessorIndex;
  EbcDebugProtocol->RegisterPeriodicCallback    = EbcDebugRegisterPeriodicCallback;
  EbcDebugProtocol->RegisterExceptionCallback   = EbcDebugRegisterExceptionCallback;
  EbcDebugProtocol->InvalidateInstructionCache  = EbcDebugInvalidateInstructionCache;

  //
  // Add the protocol so the debug agent can find us
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiDebugSupportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  EbcDebugProtocol
                  );
  //
  // This is recoverable, so free the memory and continue.
  //
  if (EFI_ERROR (Status)) {
    FreePool (EbcDebugProtocol);
    goto ErrorExit;
  }
  //
  // Install EbcDebugSupport Protocol Successfully
  // Now we need to initialize the Ebc default Callback
  //
  Status = InitializeEbcCallback (EbcDebugProtocol);

  //
  // Produce a VM test interface protocol. Not required for execution.
  //
  DEBUG_CODE_BEGIN ();
    InitEbcVmTestProtocol (&ImageHandle);
  DEBUG_CODE_END ();

  return EFI_SUCCESS;

ErrorExit:
  FreeEBCStack();
  HandleBuffer  = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiEbcProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandleBuffer
                  );
  if (Status == EFI_SUCCESS) {
    //
    // Loop through the handles
    //
    for (Index = 0; Index < NumHandles; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiEbcProtocolGuid,
                      (VOID **) &OldEbcProtocol
                      );
      if (Status == EFI_SUCCESS) {
        gBS->UninstallProtocolInterface (
               HandleBuffer[Index],
               &gEfiEbcProtocolGuid,
               OldEbcProtocol
               );
      }
    }
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
    HandleBuffer = NULL;
  }

  FreePool (EbcProtocol);

  return Status;
}


/**
  This is the top-level routine plugged into the EBC protocol. Since thunks
  are very processor-specific, from here we dispatch directly to the very
  processor-specific routine EbcCreateThunks().

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle           Handle of image for which the thunk is being
                                created. The EBC interpreter may use this to
                                keep track of any resource allocations
                                performed in loading and executing the image.
  @param  EbcEntryPoint         Address of the actual EBC entry point or
                                protocol service the thunk should call.
  @param  Thunk                 Returned pointer to a thunk created.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Image entry point is not 2-byte aligned.
  @retval EFI_OUT_OF_RESOURCES  Memory could not be allocated for the thunk.

**/
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle,
  IN VOID               *EbcEntryPoint,
  OUT VOID              **Thunk
  )
{
  EFI_STATUS  Status;

  Status = EbcCreateThunks (
            ImageHandle,
            EbcEntryPoint,
            Thunk,
            FLAG_THUNK_ENTRY_POINT
            );
  return Status;
}


/**
  This EBC debugger protocol service is called by the debug agent

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  MaxProcessorIndex     Pointer to a caller-allocated UINTN in which the
                                maximum supported processor index is returned.

  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  OUT UINTN                              *MaxProcessorIndex
  )
{
  *MaxProcessorIndex = 0;
  return EFI_SUCCESS;
}


/**
  This protocol service is called by the debug agent to register a function
  for us to call on a periodic basis.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  PeriodicCallback      A pointer to a function of type
                                PERIODIC_CALLBACK that is the main periodic
                                entry point of the debug agent. It receives as a
                                parameter a pointer to the full context of the
                                interrupted execution thread.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL PeriodicCallback parameter when a
                                callback function was previously registered.
  @retval EFI_INVALID_PARAMETER Null PeriodicCallback parameter when no
                                callback function was previously registered.

**/
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK       PeriodicCallback
  )
{
  if ((mDebugPeriodicCallback == NULL) && (PeriodicCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugPeriodicCallback != NULL) && (PeriodicCallback != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  mDebugPeriodicCallback = PeriodicCallback;
  return EFI_SUCCESS;
}


/**
  This protocol service is called by the debug agent to register a function
  for us to call when we detect an exception.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  ExceptionCallback     A pointer to a function of type
                                EXCEPTION_CALLBACK that is called when the
                                processor exception specified by ExceptionType
                                occurs. Passing NULL unregisters any previously
                                registered function associated with
                                ExceptionType.
  @param  ExceptionType         Specifies which processor exception to hook.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_ALREADY_STARTED   Non-NULL ExceptionCallback parameter when a
                                callback function was previously registered.
  @retval EFI_INVALID_PARAMETER ExceptionType parameter is negative or exceeds
                                MAX_EBC_EXCEPTION.
  @retval EFI_INVALID_PARAMETER Null ExceptionCallback parameter when no
                                callback function was previously registered.

**/
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN EFI_EXCEPTION_TYPE          ExceptionType
  )
{
  if ((ExceptionType < 0) || (ExceptionType > MAX_EBC_EXCEPTION)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugExceptionCallback[ExceptionType] == NULL) && (ExceptionCallback == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  if ((mDebugExceptionCallback[ExceptionType] != NULL) && (ExceptionCallback != NULL)) {
    return EFI_ALREADY_STARTED;
  }
  mDebugExceptionCallback[ExceptionType] = ExceptionCallback;
  return EFI_SUCCESS;
}


/**
  This EBC debugger protocol service is called by the debug agent.  Required
  for DebugSupport compliance but is only stubbed out for EBC.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.
  @param  ProcessorIndex        Specifies which processor the callback function
                                applies to.
  @param  Start                 StartSpecifies the physical base of the memory
                                range to be invalidated.
  @param  Length                Specifies the minimum number of bytes in the
                                processor's instruction cache to invalidate.

  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  IN UINTN                               ProcessorIndex,
  IN VOID                                *Start,
  IN UINT64                              Length
  )
{
  return EFI_SUCCESS;
}


/**
  The VM interpreter calls this function when an exception is detected.

  @param  ExceptionType          Specifies the processor exception detected.
  @param  ExceptionFlags         Specifies the exception context.
  @param  VmPtr                  Pointer to a VM context for passing info to the
                                 EFI debugger.

  @retval EFI_SUCCESS            This function completed successfully.

**/
EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE                   ExceptionType,
  IN EXCEPTION_FLAGS                      ExceptionFlags,
  IN VM_CONTEXT                           *VmPtr
  )
{
  EFI_SYSTEM_CONTEXT_EBC  EbcContext;
  EFI_SYSTEM_CONTEXT      SystemContext;

  ASSERT ((ExceptionType >= 0) && (ExceptionType <= MAX_EBC_EXCEPTION));
  //
  // Save the exception in the context passed in
  //
  VmPtr->ExceptionFlags |= ExceptionFlags;
  VmPtr->LastException = (UINTN) ExceptionType;
  //
  // If it's a fatal exception, then flag it in the VM context in case an
  // attached debugger tries to return from it.
  //
  if ((ExceptionFlags & EXCEPTION_FLAG_FATAL) != 0) {
    VmPtr->StopFlags |= STOPFLAG_APP_DONE;
  }

  //
  // If someone's registered for exception callbacks, then call them.
  //
  // EBC driver will register default exception callback to report the
  // status code via the status code API
  //
  if (mDebugExceptionCallback[ExceptionType] != NULL) {

    //
    // Initialize the context structure
    //
    EbcContext.R0                   = (UINT64) VmPtr->Gpr[0];
    EbcContext.R1                   = (UINT64) VmPtr->Gpr[1];
    EbcContext.R2                   = (UINT64) VmPtr->Gpr[2];
    EbcContext.R3                   = (UINT64) VmPtr->Gpr[3];
    EbcContext.R4                   = (UINT64) VmPtr->Gpr[4];
    EbcContext.R5                   = (UINT64) VmPtr->Gpr[5];
    EbcContext.R6                   = (UINT64) VmPtr->Gpr[6];
    EbcContext.R7                   = (UINT64) VmPtr->Gpr[7];
    EbcContext.Ip                   = (UINT64)(UINTN)VmPtr->Ip;
    EbcContext.Flags                = VmPtr->Flags;
    EbcContext.ControlFlags         = 0;
    SystemContext.SystemContextEbc  = &EbcContext;

    mDebugExceptionCallback[ExceptionType] (ExceptionType, SystemContext);
    //
    // Restore the context structure and continue to execute
    //
    VmPtr->Gpr[0]  = EbcContext.R0;
    VmPtr->Gpr[1]  = EbcContext.R1;
    VmPtr->Gpr[2]  = EbcContext.R2;
    VmPtr->Gpr[3]  = EbcContext.R3;
    VmPtr->Gpr[4]  = EbcContext.R4;
    VmPtr->Gpr[5]  = EbcContext.R5;
    VmPtr->Gpr[6]  = EbcContext.R6;
    VmPtr->Gpr[7]  = EbcContext.R7;
    VmPtr->Ip    = (VMIP)(UINTN)EbcContext.Ip;
    VmPtr->Flags = EbcContext.Flags;
  }

  return EFI_SUCCESS;
}


/**
  To install default Callback function for the VM interpreter.

  @param  This                  A pointer to the EFI_DEBUG_SUPPORT_PROTOCOL
                                instance.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval Others                Some error occurs when creating periodic event.

**/
EFI_STATUS
EFIAPI
InitializeEbcCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This
  )
{
  INTN       Index;
  EFI_STATUS Status;

  //
  // For ExceptionCallback
  //
  for (Index = 0; Index <= MAX_EBC_EXCEPTION; Index++) {
    EbcDebugRegisterExceptionCallback (
      This,
      0,
      CommonEbcExceptionHandler,
      Index
      );
  }

  //
  // For PeriodicCallback
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  EbcPeriodicNotifyFunction,
                  &mVmPtr,
                  &mEbcPeriodicEvent
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  mEbcPeriodicEvent,
                  TimerPeriodic,
                  EBC_VM_PERIODIC_CALLBACK_RATE
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  The default Exception Callback for the VM interpreter.
  In this function, we report status code, and print debug information
  about EBC_CONTEXT, then dead loop.

  @param  InterruptType          Interrupt type.
  @param  SystemContext          EBC system context.

**/
VOID
EFIAPI
CommonEbcExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
{
  //
  // We print debug information to let user know what happen.
  //
  DEBUG ((
    EFI_D_ERROR,
    "EBC Interrupter Version - 0x%016lx\n",
    (UINT64) (((VM_MAJOR_VERSION & 0xFFFF) << 16) | ((VM_MINOR_VERSION & 0xFFFF)))
    ));
  DEBUG ((
    EFI_D_ERROR,
    "Exception Type - 0x%016lx\n",
    (UINT64)(UINTN)InterruptType
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  R0 - 0x%016lx, R1 - 0x%016lx\n",
    SystemContext.SystemContextEbc->R0,
    SystemContext.SystemContextEbc->R1
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  R2 - 0x%016lx, R3 - 0x%016lx\n",
    SystemContext.SystemContextEbc->R2,
    SystemContext.SystemContextEbc->R3
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  R4 - 0x%016lx, R5 - 0x%016lx\n",
    SystemContext.SystemContextEbc->R4,
    SystemContext.SystemContextEbc->R5
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  R6 - 0x%016lx, R7 - 0x%016lx\n",
    SystemContext.SystemContextEbc->R6,
    SystemContext.SystemContextEbc->R7
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  Flags - 0x%016lx\n",
    SystemContext.SystemContextEbc->Flags
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  ControlFlags - 0x%016lx\n",
    SystemContext.SystemContextEbc->ControlFlags
    ));
  DEBUG ((
    EFI_D_ERROR,
    "  Ip - 0x%016lx\n\n",
    SystemContext.SystemContextEbc->Ip
    ));

  //
  // We deadloop here to make it easy to debug this issue.
  //
  CpuDeadLoop ();

  return ;
}


/**
  The periodic callback function for EBC VM interpreter, which is used
  to support the EFI debug support protocol.

  @param  Event                  The Periodic Callback Event.
  @param  Context                It should be the address of VM_CONTEXT pointer.

**/
VOID
EFIAPI
EbcPeriodicNotifyFunction (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
{
  VM_CONTEXT *VmPtr;

  VmPtr = *(VM_CONTEXT **)Context;

  if (VmPtr != NULL) {
    EbcDebugPeriodic (VmPtr);
  }

  return ;
}


/**
  The VM interpreter calls this function on a periodic basis to support
  the EFI debug support protocol.

  @param  VmPtr                  Pointer to a VM context for passing info to the
                                 debugger.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcDebugPeriodic (
  IN VM_CONTEXT *VmPtr
  )
{
  EFI_SYSTEM_CONTEXT_EBC   EbcContext;
  EFI_SYSTEM_CONTEXT       SystemContext;

  //
  // If someone's registered for periodic callbacks, then call them.
  //
  if (mDebugPeriodicCallback != NULL) {

    //
    // Initialize the context structure
    //
    EbcContext.R0                   = (UINT64) VmPtr->Gpr[0];
    EbcContext.R1                   = (UINT64) VmPtr->Gpr[1];
    EbcContext.R2                   = (UINT64) VmPtr->Gpr[2];
    EbcContext.R3                   = (UINT64) VmPtr->Gpr[3];
    EbcContext.R4                   = (UINT64) VmPtr->Gpr[4];
    EbcContext.R5                   = (UINT64) VmPtr->Gpr[5];
    EbcContext.R6                   = (UINT64) VmPtr->Gpr[6];
    EbcContext.R7                   = (UINT64) VmPtr->Gpr[7];
    EbcContext.Ip                   = (UINT64)(UINTN)VmPtr->Ip;
    EbcContext.Flags                = VmPtr->Flags;
    EbcContext.ControlFlags         = 0;
    SystemContext.SystemContextEbc  = &EbcContext;

    mDebugPeriodicCallback (SystemContext);

    //
    // Restore the context structure and continue to execute
    //
    VmPtr->Gpr[0]  = EbcContext.R0;
    VmPtr->Gpr[1]  = EbcContext.R1;
    VmPtr->Gpr[2]  = EbcContext.R2;
    VmPtr->Gpr[3]  = EbcContext.R3;
    VmPtr->Gpr[4]  = EbcContext.R4;
    VmPtr->Gpr[5]  = EbcContext.R5;
    VmPtr->Gpr[6]  = EbcContext.R6;
    VmPtr->Gpr[7]  = EbcContext.R7;
    VmPtr->Ip    = (VMIP)(UINTN)EbcContext.Ip;
    VmPtr->Flags = EbcContext.Flags;
  }

  return EFI_SUCCESS;
}


/**
  This routine is called by the core when an image is being unloaded from
  memory. Basically we now have the opportunity to do any necessary cleanup.
  Typically this will include freeing any memory allocated for thunk-creation.

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  ImageHandle           Handle of image for which the thunk is being
                                created.

  @retval EFI_INVALID_PARAMETER The ImageHandle passed in was not found in the
                                internal list of EBC image handles.
  @retval EFI_SUCCESS           The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle
  )
{
  EBC_THUNK_LIST  *ThunkList;
  EBC_THUNK_LIST  *NextThunkList;
  EBC_IMAGE_LIST  *ImageList;
  EBC_IMAGE_LIST  *PrevImageList;
  //
  // First go through our list of known image handles and see if we've already
  // created an image list element for this image handle.
  //
  ReturnEBCStackByHandle(ImageHandle);
  PrevImageList = NULL;
  for (ImageList = mEbcImageList; ImageList != NULL; ImageList = ImageList->Next) {
    if (ImageList->ImageHandle == ImageHandle) {
      break;
    }
    //
    // Save the previous so we can connect the lists when we remove this one
    //
    PrevImageList = ImageList;
  }

  if (ImageList == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Free up all the thunk buffers and thunks list elements for this image
  // handle.
  //
  ThunkList = ImageList->ThunkList;
  while (ThunkList != NULL) {
    NextThunkList = ThunkList->Next;
    FreePool (ThunkList->ThunkBuffer);
    FreePool (ThunkList);
    ThunkList = NextThunkList;
  }
  //
  // Now remove this image list element from the chain
  //
  if (PrevImageList == NULL) {
    //
    // Remove from head
    //
    mEbcImageList = ImageList->Next;
  } else {
    PrevImageList->Next = ImageList->Next;
  }
  //
  // Now free up the image list element
  //
  FreePool (ImageList);
  return EFI_SUCCESS;
}


/**
  Add a thunk to our list of thunks for a given image handle.
  Also flush the instruction cache since we've written thunk code
  to memory that will be executed eventually.

  @param  ImageHandle            The image handle to which the thunk is tied.
  @param  ThunkBuffer            The buffer that has been created/allocated.
  @param  ThunkSize              The size of the thunk memory allocated.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EbcAddImageThunk (
  IN EFI_HANDLE      ImageHandle,
  IN VOID            *ThunkBuffer,
  IN UINT32          ThunkSize
  )
{
  EBC_THUNK_LIST  *ThunkList;
  EBC_IMAGE_LIST  *ImageList;
  EFI_STATUS      Status;

  //
  // It so far so good, then flush the instruction cache
  //
  if (mEbcICacheFlush != NULL) {
    Status = mEbcICacheFlush ((EFI_PHYSICAL_ADDRESS) (UINTN) ThunkBuffer, ThunkSize);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Go through our list of known image handles and see if we've already
  // created a image list element for this image handle.
  //
  for (ImageList = mEbcImageList; ImageList != NULL; ImageList = ImageList->Next) {
    if (ImageList->ImageHandle == ImageHandle) {
      break;
    }
  }

  if (ImageList == NULL) {
    //
    // Allocate a new one
    //
    ImageList = AllocatePool (sizeof (EBC_IMAGE_LIST));

    if (ImageList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ImageList->ThunkList    = NULL;
    ImageList->ImageHandle  = ImageHandle;
    ImageList->Next         = mEbcImageList;
    mEbcImageList           = ImageList;
  }
  //
  // Ok, now create a new thunk element to add to the list
  //
  ThunkList = AllocatePool (sizeof (EBC_THUNK_LIST));

  if (ThunkList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Add it to the head of the list
  //
  ThunkList->Next         = ImageList->ThunkList;
  ThunkList->ThunkBuffer  = ThunkBuffer;
  ImageList->ThunkList    = ThunkList;
  return EFI_SUCCESS;
}

/**
  Registers a callback function that the EBC interpreter calls to flush the
  processor instruction cache following creation of thunks.

  @param  This        A pointer to the EFI_EBC_PROTOCOL instance.
  @param  Flush       Pointer to a function of type EBC_ICACH_FLUSH.

  @retval EFI_SUCCESS The function completed successfully.

**/
EFI_STATUS
EFIAPI
EbcRegisterICacheFlush (
  IN EFI_EBC_PROTOCOL   *This,
  IN EBC_ICACHE_FLUSH   Flush
  )
{
  mEbcICacheFlush = Flush;
  return EFI_SUCCESS;
}

/**
  Called to get the version of the interpreter.

  @param  This                  A pointer to the EFI_EBC_PROTOCOL instance.
  @param  Version               Pointer to where to store the returned version
                                of the interpreter.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER Version pointer is NULL.

**/
EFI_STATUS
EFIAPI
EbcGetVersion (
  IN EFI_EBC_PROTOCOL   *This,
  IN OUT UINT64         *Version
  )
{
  if (Version == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Version = GetVmVersion ();
  return EFI_SUCCESS;
}

/**
  Returns the stack index and buffer assosicated with the Handle parameter.

  @param  Handle                The EFI handle as the index to the EBC stack.
  @param  StackBuffer           A pointer to hold the returned stack buffer.
  @param  BufferIndex           A pointer to hold the returned stack index.

  @retval EFI_OUT_OF_RESOURCES  The Handle parameter does not correspond to any
                                existing EBC stack.
  @retval EFI_SUCCESS           The stack index and buffer were found and
                                returned to the caller.

**/
EFI_STATUS
GetEBCStack(
  IN  EFI_HANDLE Handle,
  OUT VOID       **StackBuffer,
  OUT UINTN      *BufferIndex
  )
{
  UINTN   Index;
  EFI_TPL OldTpl;
  OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);
  for (Index = 0; Index < mStackNum; Index ++) {
    if (mStackBufferIndex[Index] == NULL) {
      mStackBufferIndex[Index] = Handle;
      break;
    }
  }
  gBS->RestoreTPL(OldTpl);
  if (Index == mStackNum) {
    return EFI_OUT_OF_RESOURCES;
  }
  *BufferIndex = Index;
  *StackBuffer = mStackBuffer[Index];
  return EFI_SUCCESS;
}

/**
  Returns from the EBC stack by stack Index.

  @param  Index        Specifies which EBC stack to return from.

  @retval EFI_SUCCESS  The function completed successfully.

**/
EFI_STATUS
ReturnEBCStack(
  IN UINTN Index
  )
{
  mStackBufferIndex[Index] = NULL;
  return EFI_SUCCESS;
}

/**
  Returns from the EBC stack associated with the Handle parameter.

  @param  Handle      Specifies the EFI handle to find the EBC stack with.

  @retval EFI_SUCCESS The function completed successfully.

**/
EFI_STATUS
ReturnEBCStackByHandle(
  IN EFI_HANDLE Handle
  )
{
  UINTN Index;
  for (Index = 0; Index < mStackNum; Index ++) {
    if (mStackBufferIndex[Index] == Handle) {
      break;
    }
  }
  if (Index == mStackNum) {
    return EFI_NOT_FOUND;
  }
  mStackBufferIndex[Index] = NULL;
  return EFI_SUCCESS;
}

/**
  Allocates memory to hold all the EBC stacks.

  @retval EFI_SUCCESS          The EBC stacks were allocated successfully.
  @retval EFI_OUT_OF_RESOURCES Not enough memory available for EBC stacks.

**/
EFI_STATUS
InitEBCStack (
  VOID
  )
{
  for (mStackNum = 0; mStackNum < MAX_STACK_NUM; mStackNum ++) {
    mStackBuffer[mStackNum] = AllocatePool(STACK_POOL_SIZE);
    mStackBufferIndex[mStackNum] = NULL;
    if (mStackBuffer[mStackNum] == NULL) {
      break;
    }
  }
  if (mStackNum == 0) {
    return EFI_OUT_OF_RESOURCES;
  }
  return EFI_SUCCESS;
}


/**
  Free all EBC stacks allocated before.

  @retval EFI_SUCCESS   All the EBC stacks were freed.

**/
EFI_STATUS
FreeEBCStack(
  VOID
  )
{
  UINTN Index;
  for (Index = 0; Index < mStackNum; Index ++) {
    FreePool(mStackBuffer[Index]);
  }
  return EFI_SUCCESS;
}

/**
  Produces an EBC VM test protocol that can be used for regression tests.

  @param  IHandle                Handle on which to install the protocol.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *IHandle
  )
{
  EFI_HANDLE Handle;
  EFI_STATUS Status;
  EFI_EBC_VM_TEST_PROTOCOL *EbcVmTestProtocol;

  //
  // Allocate memory for the protocol, then fill in the fields
  //
  EbcVmTestProtocol = AllocatePool (sizeof (EFI_EBC_VM_TEST_PROTOCOL));
  if (EbcVmTestProtocol == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  EbcVmTestProtocol->Execute      = (EBC_VM_TEST_EXECUTE) EbcExecuteInstructions;

  DEBUG_CODE_BEGIN ();
    EbcVmTestProtocol->Assemble     = (EBC_VM_TEST_ASM) EbcVmTestUnsupported;
    EbcVmTestProtocol->Disassemble  = (EBC_VM_TEST_DASM) EbcVmTestUnsupported;
  DEBUG_CODE_END ();

  //
  // Publish the protocol
  //
  Handle  = NULL;
  Status  = gBS->InstallProtocolInterface (&Handle, &gEfiEbcVmTestProtocolGuid, EFI_NATIVE_INTERFACE, EbcVmTestProtocol);
  if (EFI_ERROR (Status)) {
    FreePool (EbcVmTestProtocol);
  }
  return Status;
}


/**
  Returns the EFI_UNSUPPORTED Status.

  @return EFI_UNSUPPORTED  This function always return EFI_UNSUPPORTED status.

**/
EFI_STATUS
EFIAPI
EbcVmTestUnsupported (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}

