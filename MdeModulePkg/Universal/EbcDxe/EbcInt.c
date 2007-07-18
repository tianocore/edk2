/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EbcInt.c

Abstract:

  Top level module for the EBC virtual machine implementation.
  Provides auxilliary support routines for the VM. That is, routines
  that are not particularly related to VM execution of EBC instructions.

--*/

#include "EbcInt.h"
#include "EbcExecute.h"

//
// We'll keep track of all thunks we create in a linked list. Each
// thunk is tied to an image handle, so we have a linked list of
// image handles, with each having a linked list of thunks allocated
// to that image handle.
//
typedef struct _EBC_THUNK_LIST {
  VOID                    *ThunkBuffer;
  struct _EBC_THUNK_LIST  *Next;
} EBC_THUNK_LIST;

typedef struct _EBC_IMAGE_LIST {
  struct _EBC_IMAGE_LIST  *Next;
  EFI_HANDLE              ImageHandle;
  EBC_THUNK_LIST          *ThunkList;
} EBC_IMAGE_LIST;

STATIC
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL     *This,
  IN EFI_HANDLE           ImageHandle
  );

STATIC
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL     *This,
  IN EFI_HANDLE           ImageHandle,
  IN VOID                 *EbcEntryPoint,
  OUT VOID                **Thunk
  );

STATIC
EFI_STATUS
EFIAPI
EbcGetVersion (
  IN EFI_EBC_PROTOCOL     *This,
  IN OUT UINT64           *Version
  );

STATIC
EFI_STATUS
EFIAPI
InitializeEbcCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This
  );

STATIC
VOID
EFIAPI
CommonEbcExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  );

STATIC
VOID
EFIAPI
EbcPeriodicNotifyFunction (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugPeriodic (
  IN VM_CONTEXT *VmPtr
  );

//
// These two functions and the  GUID are used to produce an EBC test protocol.
// This functionality is definitely not required for execution.
//
STATIC
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *Handle
  );

STATIC
EFI_STATUS
EbcVmTestUnsupported (
  VOID
  );

STATIC
EFI_STATUS
EFIAPI
EbcRegisterICacheFlush (
  IN EFI_EBC_PROTOCOL               *This,
  IN EBC_ICACHE_FLUSH               Flush
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  OUT UINTN                         *MaxProcessorIndex
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK          PeriodicCallback
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK         ExceptionCallback,
  IN EFI_EXCEPTION_TYPE             ExceptionType
  );

STATIC
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL     *This,
  IN UINTN                          ProcessorIndex,
  IN VOID                           *Start,
  IN UINT64                         Length
  );

//
// We have one linked list of image handles for the whole world. Since
// there should only be one interpreter, make them global. They must
// also be global since the execution of an EBC image does not provide
// a This pointer.
//
static EBC_IMAGE_LIST         *mEbcImageList = NULL;

//
// Callback function to flush the icache after thunk creation
//
static EBC_ICACHE_FLUSH       mEbcICacheFlush;

//
// These get set via calls by the debug agent
//
static EFI_PERIODIC_CALLBACK  mDebugPeriodicCallback                            = NULL;
static EFI_EXCEPTION_CALLBACK mDebugExceptionCallback[MAX_EBC_EXCEPTION + 1] = {NULL};
static EFI_GUID               mEfiEbcVmTestProtocolGuid = EFI_EBC_VM_TEST_PROTOCOL_GUID;

static VOID*      mStackBuffer[MAX_STACK_NUM];
static EFI_HANDLE mStackBufferIndex[MAX_STACK_NUM];
static UINTN      mStackNum = 0;

//
// Event for Periodic callback
//
static EFI_EVENT              mEbcPeriodicEvent;
VM_CONTEXT                    *mVmPtr = NULL;

EFI_STATUS
EFIAPI
InitializeEbcDriver (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Initializes the VM EFI interface.  Allocates memory for the VM interface
  and registers the VM protocol.

Arguments:

  ImageHandle - EFI image handle.
  SystemTable - Pointer to the EFI system table.

Returns:
  Standard EFI status code.

--*/
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

STATIC
EFI_STATUS
EFIAPI
EbcCreateThunk (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle,
  IN VOID               *EbcEntryPoint,
  OUT VOID              **Thunk
  )
/*++

Routine Description:

  This is the top-level routine plugged into the EBC protocol. Since thunks
  are very processor-specific, from here we dispatch directly to the very
  processor-specific routine EbcCreateThunks().

Arguments:

  This          - protocol instance pointer
  ImageHandle   - handle to the image. The EBC interpreter may use this to keep
                  track of any resource allocations performed in loading and
                  executing the image.
  EbcEntryPoint - the entry point for the image (as defined in the file header)
  Thunk         - pointer to thunk pointer where the address of the created
                  thunk is returned.

Returns:

  EFI_STATUS

--*/
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

STATIC
EFI_STATUS
EFIAPI
EbcDebugGetMaximumProcessorIndex (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  OUT UINTN                              *MaxProcessorIndex
  )
/*++

Routine Description:

  This EBC debugger protocol service is called by the debug agent

Arguments:

  This              - pointer to the caller's debug support protocol interface
  MaxProcessorIndex - pointer to a caller allocated UINTN in which the maximum
                      processor index is returned.

Returns:

  Standard EFI_STATUS

--*/
{
  *MaxProcessorIndex = 0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterPeriodicCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_PERIODIC_CALLBACK       PeriodicCallback
  )
/*++

Routine Description:

  This protocol service is called by the debug agent to register a function
  for us to call on a periodic basis.


Arguments:

  This              - pointer to the caller's debug support protocol interface
  PeriodicCallback  - pointer to the function to call periodically

Returns:

  Always EFI_SUCCESS

--*/
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

STATIC
EFI_STATUS
EFIAPI
EbcDebugRegisterExceptionCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN UINTN                       ProcessorIndex,
  IN EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN EFI_EXCEPTION_TYPE          ExceptionType
  )
/*++

Routine Description:

  This protocol service is called by the debug agent to register a function
  for us to call when we detect an exception.


Arguments:

  This              - pointer to the caller's debug support protocol interface
  ExceptionCallback - pointer to the function to the exception

Returns:

  Always EFI_SUCCESS

--*/
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

STATIC
EFI_STATUS
EFIAPI
EbcDebugInvalidateInstructionCache (
  IN EFI_DEBUG_SUPPORT_PROTOCOL          *This,
  IN UINTN                               ProcessorIndex,
  IN VOID                                *Start,
  IN UINT64                              Length
  )
/*++

Routine Description:

  This EBC debugger protocol service is called by the debug agent.  Required
  for DebugSupport compliance but is only stubbed out for EBC.

Arguments:

Returns:

  EFI_SUCCESS

--*/
{
  return EFI_SUCCESS;
}

EFI_STATUS
EbcDebugSignalException (
  IN EFI_EXCEPTION_TYPE                   ExceptionType,
  IN EXCEPTION_FLAGS                      ExceptionFlags,
  IN VM_CONTEXT                           *VmPtr
  )
/*++

Routine Description:

  The VM interpreter calls this function when an exception is detected.

Arguments:

  VmPtr - pointer to a VM context for passing info to the EFI debugger.

Returns:

  EFI_SUCCESS if it returns at all

--*/
{
  EFI_SYSTEM_CONTEXT_EBC  EbcContext;
  EFI_SYSTEM_CONTEXT      SystemContext;

  ASSERT ((ExceptionType >= 0) && (ExceptionType <= MAX_EBC_EXCEPTION));
  //
  // Save the exception in the context passed in
  //
  VmPtr->ExceptionFlags |= ExceptionFlags;
  VmPtr->LastException = ExceptionType;
  //
  // If it's a fatal exception, then flag it in the VM context in case an
  // attached debugger tries to return from it.
  //
  if (ExceptionFlags & EXCEPTION_FLAG_FATAL) {
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
    EbcContext.R0                   = VmPtr->R[0];
    EbcContext.R1                   = VmPtr->R[1];
    EbcContext.R2                   = VmPtr->R[2];
    EbcContext.R3                   = VmPtr->R[3];
    EbcContext.R4                   = VmPtr->R[4];
    EbcContext.R5                   = VmPtr->R[5];
    EbcContext.R6                   = VmPtr->R[6];
    EbcContext.R7                   = VmPtr->R[7];
    EbcContext.Ip                   = (UINT64)(UINTN)VmPtr->Ip;
    EbcContext.Flags                = VmPtr->Flags;
    EbcContext.ControlFlags         = 0;
    SystemContext.SystemContextEbc  = &EbcContext;

    mDebugExceptionCallback[ExceptionType] (ExceptionType, SystemContext);
    //
    // Restore the context structure and continue to execute
    //
    VmPtr->R[0]  = EbcContext.R0;
    VmPtr->R[1]  = EbcContext.R1;
    VmPtr->R[2]  = EbcContext.R2;
    VmPtr->R[3]  = EbcContext.R3;
    VmPtr->R[4]  = EbcContext.R4;
    VmPtr->R[5]  = EbcContext.R5;
    VmPtr->R[6]  = EbcContext.R6;
    VmPtr->R[7]  = EbcContext.R7;
    VmPtr->Ip    = (VMIP)(UINTN)EbcContext.Ip;
    VmPtr->Flags = EbcContext.Flags;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InitializeEbcCallback (
  IN EFI_DEBUG_SUPPORT_PROTOCOL  *This
  )
/*++

Routine Description:

  To install default Callback function for the VM interpreter.

Arguments:

  This - pointer to the instance of DebugSupport protocol

Returns:

  None

--*/
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

STATIC
VOID
CommonEbcExceptionHandler (
  IN EFI_EXCEPTION_TYPE   InterruptType,
  IN EFI_SYSTEM_CONTEXT   SystemContext
  )
/*++

Routine Description:

  The default Exception Callback for the VM interpreter.
  In this function, we report status code, and print debug information
  about EBC_CONTEXT, then dead loop.

Arguments:

  InterruptType - Interrupt type.
  SystemContext - EBC system context.

Returns:

  None

--*/
{
  //
  // We deadloop here to make it easy to debug this issue.
  //
  ASSERT (FALSE);

  return ;
}

STATIC
VOID
EFIAPI
EbcPeriodicNotifyFunction (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++

Routine Description:

  The periodic callback function for EBC VM interpreter, which is used
  to support the EFI debug support protocol.

Arguments:

  Event   - The Periodic Callback Event.
  Context - It should be the address of VM_CONTEXT pointer.

Returns:

  None.

--*/
{
  VM_CONTEXT *VmPtr;

  VmPtr = *(VM_CONTEXT **)Context;

  if (VmPtr != NULL) {
    EbcDebugPeriodic (VmPtr);
  }

  return ;
}

STATIC
EFI_STATUS
EbcDebugPeriodic (
  IN VM_CONTEXT *VmPtr
  )
/*++

Routine Description:

  The VM interpreter calls this function on a periodic basis to support
  the EFI debug support protocol.

Arguments:

  VmPtr - pointer to a VM context for passing info to the debugger.

Returns:

  Standard EFI status.

--*/
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
    EbcContext.R0                   = VmPtr->R[0];
    EbcContext.R1                   = VmPtr->R[1];
    EbcContext.R2                   = VmPtr->R[2];
    EbcContext.R3                   = VmPtr->R[3];
    EbcContext.R4                   = VmPtr->R[4];
    EbcContext.R5                   = VmPtr->R[5];
    EbcContext.R6                   = VmPtr->R[6];
    EbcContext.R7                   = VmPtr->R[7];
    EbcContext.Ip                   = (UINT64)(UINTN)VmPtr->Ip;
    EbcContext.Flags                = VmPtr->Flags;
    EbcContext.ControlFlags         = 0;
    SystemContext.SystemContextEbc  = &EbcContext;

    mDebugPeriodicCallback (SystemContext);

    //
    // Restore the context structure and continue to execute
    //
    VmPtr->R[0]  = EbcContext.R0;
    VmPtr->R[1]  = EbcContext.R1;
    VmPtr->R[2]  = EbcContext.R2;
    VmPtr->R[3]  = EbcContext.R3;
    VmPtr->R[4]  = EbcContext.R4;
    VmPtr->R[5]  = EbcContext.R5;
    VmPtr->R[6]  = EbcContext.R6;
    VmPtr->R[7]  = EbcContext.R7;
    VmPtr->Ip    = (VMIP)(UINTN)EbcContext.Ip;
    VmPtr->Flags = EbcContext.Flags;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
EbcUnloadImage (
  IN EFI_EBC_PROTOCOL   *This,
  IN EFI_HANDLE         ImageHandle
  )
/*++

Routine Description:

  This routine is called by the core when an image is being unloaded from
  memory. Basically we now have the opportunity to do any necessary cleanup.
  Typically this will include freeing any memory allocated for thunk-creation.

Arguments:

  This          - protocol instance pointer
  ImageHandle   - handle to the image being unloaded.

Returns:

  EFI_INVALID_PARAMETER  - the ImageHandle passed in was not found in
                           the internal list of EBC image handles.
  EFI_STATUS             - completed successfully

--*/
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

EFI_STATUS
EbcAddImageThunk (
  IN EFI_HANDLE      ImageHandle,
  IN VOID            *ThunkBuffer,
  IN UINT32          ThunkSize
  )
/*++

Routine Description:

  Add a thunk to our list of thunks for a given image handle.
  Also flush the instruction cache since we've written thunk code
  to memory that will be executed eventually.

Arguments:

  ImageHandle - the image handle to which the thunk is tied
  ThunkBuffer - the buffer we've created/allocated
  ThunkSize    - the size of the thunk memory allocated

Returns:

  EFI_OUT_OF_RESOURCES    - memory allocation failed
  EFI_SUCCESS             - successful completion

--*/
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

STATIC
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

STATIC
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

EFI_STATUS
GetEBCStack(
  EFI_HANDLE Handle,
  VOID       **StackBuffer,
  UINTN      *BufferIndex
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

EFI_STATUS
ReturnEBCStack(
  UINTN Index
  )
{
  mStackBufferIndex[Index] =NULL;
  return EFI_SUCCESS;
}

EFI_STATUS
ReturnEBCStackByHandle(
  EFI_HANDLE Handle
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
STATIC
EFI_STATUS
InitEbcVmTestProtocol (
  IN EFI_HANDLE     *IHandle
  )
/*++

Routine Description:

  Produce an EBC VM test protocol that can be used for regression tests.

Arguments:

  IHandle - handle on which to install the protocol.

Returns:

  EFI_OUT_OF_RESOURCES  - memory allocation failed
  EFI_SUCCESS           - successful completion

--*/
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
  Status  = gBS->InstallProtocolInterface (&Handle, &mEfiEbcVmTestProtocolGuid, EFI_NATIVE_INTERFACE, EbcVmTestProtocol);
  if (EFI_ERROR (Status)) {
    FreePool (EbcVmTestProtocol);
  }
  return Status;
}
STATIC
EFI_STATUS
EbcVmTestUnsupported ()
{
  return EFI_UNSUPPORTED;
}

