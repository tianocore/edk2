/** @file
  SMM Base Protocol on SMM Base2 Protocol Thunk driver.

  This driver co-operates with SMM Base Helper SMM driver to provide SMM Base Protocol
  based on SMM Base2 Protocol.

  This thunk driver is expected to be loaded before PI SMM IPL driver so that
  SMM BASE Protocol can be published immediately after SMM Base2 Protocol is installed to
  make SMM Base Protocol.InSmm() as early as possible.

  Copyright (c) 2009 - 2010, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include  "SmmBaseOnSmmBase2Thunk.h"

EFI_SMM_BASE_PROTOCOL gSmmBase = {
  SmmBaseRegister,
  SmmBaseUnregister,
  SmmBaseCommunicate,
  SmmBaseRegisterCallback,
  SmmBaseInSmm,
  SmmBaseSmmAllocatePool,
  SmmBaseSmmFreePool,
  SmmBaseGetSmstLocation
};

SMMBASETHUNK_COMMUNICATION_DATA gCommunicationData = {
  EFI_SMM_BASE_THUNK_COMMUNICATION_GUID,
  sizeof (SMMBASE_FUNCTION_DATA)
};

EFI_HANDLE                         mImageHandle;
EFI_SMM_BASE2_PROTOCOL             *mSmmBase2 = NULL;
EFI_SMM_COMMUNICATION_PROTOCOL     *mSmmCommunication = NULL;
EFI_SMM_BASE_HELPER_READY_PROTOCOL *mSmmBaseHelperReady = NULL;

/**
  Determine if in SMM mode.

  @retval TRUE   In SMM mode.
  @retval FALSE  Not in SMM mode.
**/
BOOLEAN
IsInSmm (
  VOID
  )
{
  EFI_STATUS Status;
  BOOLEAN    InSmm;
  
  Status = mSmmBase2->InSmm (mSmmBase2, &InSmm);
  ASSERT_EFI_ERROR (Status);
  return InSmm;
}

/**
  Invoke services provided by SMM Base Helper SMM driver.
**/
VOID
SmmBaseHelperService (
  VOID
  )
{
  UINTN DataSize;

  gCommunicationData.FunctionData.Status = EFI_UNSUPPORTED;

  if (IsInSmm()) {
    ///
    /// If in SMM mode, directly call services in SMM Base Helper.
    ///
    if (mSmmBaseHelperReady == NULL) {
      ASSERT (FALSE);
      return;
    }

    DataSize = (UINTN)(sizeof (SMMBASE_FUNCTION_DATA));
    mSmmBaseHelperReady->ServiceEntry (
                           NULL,
                           NULL,
                           &gCommunicationData.FunctionData,
                           &DataSize
                           );
  } else {
    ///
    /// If in non-SMM mode, call services in SMM Base Helper via SMM Communication Protocol.
    ///
    if (mSmmCommunication == NULL) {
      ASSERT (FALSE);
      return;
    }

    DataSize = (UINTN)(sizeof (gCommunicationData));
    mSmmCommunication->Communicate (
                         mSmmCommunication,
                         &gCommunicationData,
                         &DataSize
                         );
  }
}

/**
  Register a given driver into SMRAM. This is the equivalent of performing
  the LoadImage/StartImage into System Management Mode.

  @param[in]   This                  Protocol instance pointer.
  @param[in]   FilePath              Location of the image to be installed as the handler.
  @param[in]   SourceBuffer          Optional source buffer in case the image file
                                     is in memory.
  @param[in]   SourceSize            Size of the source image file, if in memory.
  @param[out]  ImageHandle           The handle that the base driver uses to decode 
                                     the handler. Unique among SMM handlers only, 
                                     not unique across DXE/EFI.
  @param[in]   LegacyIA32Binary      An optional parameter specifying that the associated 
                                     file is a real-mode IA-32 binary.

  @retval      EFI_SUCCESS           The operation was successful.
  @retval      EFI_OUT_OF_RESOURCES  There were no additional SMRAM resources to load the handler
  @retval      EFI_UNSUPPORTED       This platform does not support 16-bit handlers.
  @retval      EFI_UNSUPPORTED       Platform is in runtime.
  @retval      EFI_INVALID_PARAMETER The handlers was not the correct image type
**/
EFI_STATUS
EFIAPI
SmmBaseRegister (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      VOID                      *SourceBuffer,
  IN      UINTN                     SourceSize,
  OUT     EFI_HANDLE                *ImageHandle,
  IN      BOOLEAN                   LegacyIA32Binary
  )
{
  if (LegacyIA32Binary) {
    return EFI_UNSUPPORTED;
  }

  gCommunicationData.FunctionData.Function = SMMBASE_REGISTER;
  gCommunicationData.FunctionData.Args.Register.FilePath = FilePath;
  gCommunicationData.FunctionData.Args.Register.SourceBuffer = SourceBuffer;
  gCommunicationData.FunctionData.Args.Register.SourceSize = SourceSize;
  gCommunicationData.FunctionData.Args.Register.ImageHandle = ImageHandle;
  gCommunicationData.FunctionData.Args.Register.LegacyIA32Binary = LegacyIA32Binary;

  SmmBaseHelperService ();
  return gCommunicationData.FunctionData.Status;
}

/**
  Removes a handler from execution within SMRAM.  This is the equivalent of performing
  the UnloadImage in System Management Mode.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  ImageHandle           The handler to be removed.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_INVALID_PARAMETER The handler did not exist
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseUnregister (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_HANDLE                ImageHandle
  )
{
  gCommunicationData.FunctionData.Function = SMMBASE_UNREGISTER;
  gCommunicationData.FunctionData.Args.UnRegister.ImageHandle = ImageHandle;

  SmmBaseHelperService ();
  return gCommunicationData.FunctionData.Status;
}

/**
  The SMM Inter-module Communicate Service Communicate() function
  provides a service to send/receive messages from a registered
  EFI service.  The BASE protocol driver is responsible for doing
  any of the copies such that the data lives in boot-service-accessible RAM.

  @param[in]      This                  Protocol instance pointer.
  @param[in]      ImageHandle           The handle of the registered driver.
  @param[in,out]  CommunicationBuffer   Pointer to the buffer to convey into SMRAM.
  @param[in,out]  BufferSize            The size of the data buffer being passed in.
                                        On exit, the size of data being returned.
                                        Zero if the handler does not wish to reply with any data.

  @retval         EFI_SUCCESS           The message was successfully posted
  @retval         EFI_INVALID_PARAMETER The buffer was NULL
**/
EFI_STATUS
EFIAPI
SmmBaseCommunicate (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_HANDLE                ImageHandle,
  IN OUT  VOID                      *CommunicationBuffer,
  IN OUT  UINTN                     *BufferSize
  )
{
  if (mSmmCommunication == NULL) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  return mSmmCommunication->Communicate (
                              mSmmCommunication,
                              CommunicationBuffer,
                              BufferSize
                              );
}

/**
  Register a callback to execute within SMM.
  This allows receipt of messages created with EFI_SMM_BASE_PROTOCOL.Communicate().

  @param[in]  This                  Protocol instance pointer.
  @param[in]  SmmImageHandle        Handle of the callback service.
  @param[in]  CallbackAddress       Address of the callback service.
  @param[in]  MakeLast              If present, will stipulate that the handler is posted to 
                                    be executed last in the dispatch table.
  @param[in]  FloatingPointSave     An optional parameter that informs the
                                    EFI_SMM_ACCESS_PROTOCOL Driver core if it needs to save
                                    the floating point register state. If any handler
                                    require this, the state will be saved for all handlers.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_OUT_OF_RESOURCES  Not enough space in the dispatch queue
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
  @retval     EFI_UNSUPPORTED       The caller is not in SMM.
**/
EFI_STATUS
EFIAPI
SmmBaseRegisterCallback (
  IN      EFI_SMM_BASE_PROTOCOL         *This,
  IN      EFI_HANDLE                    SmmImageHandle,
  IN      EFI_SMM_CALLBACK_ENTRY_POINT  CallbackAddress,
  IN      BOOLEAN                       MakeLast,
  IN      BOOLEAN                       FloatingPointSave
  )
{
  if (!IsInSmm()) {
    return EFI_UNSUPPORTED;
  }

  gCommunicationData.FunctionData.Function = SMMBASE_REGISTER_CALLBACK;
  gCommunicationData.FunctionData.Args.RegisterCallback.SmmImageHandle = SmmImageHandle;
  gCommunicationData.FunctionData.Args.RegisterCallback.CallbackAddress = CallbackAddress;
  gCommunicationData.FunctionData.Args.RegisterCallback.MakeLast = MakeLast;
  gCommunicationData.FunctionData.Args.RegisterCallback.FloatingPointSave = FloatingPointSave;

  SmmBaseHelperService();
  return gCommunicationData.FunctionData.Status;
}

/**
  This routine tells caller if execution context is SMM or not.

  @param[in]   This                   Protocol instance pointer.
  @param[out]  InSmm                  Whether the caller is inside SMM for IA-32
                                      or servicing a PMI for the Itanium processor
                                      family.

  @retval      EFI_SUCCESS            The operation was successful
  @retval      EFI_INVALID_PARAMETER  InSmm was NULL.
**/
EFI_STATUS
EFIAPI
SmmBaseInSmm (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  OUT     BOOLEAN                   *InSmm
  )
{
  return mSmmBase2->InSmm (mSmmBase2, InSmm);
}

/**
  The SmmAllocatePool() function allocates a memory region of Size bytes from memory of
  type PoolType and returns the address of the allocated memory in the location referenced
  by Buffer.  This function allocates pages from EFI SMRAM Memory as needed to grow the
  requested pool type.  All allocations are eight-byte aligned.

  @param[in]   This                  Protocol instance pointer.
  @param[in]   PoolType              The type of pool to allocate.
                                     The only supported type is EfiRuntimeServicesData;
                                     the interface will internally map this runtime request to 
                                     SMRAM for IA-32 and leave as this type for the Itanium 
                                     processor family. Other types can be ignored.
  @param[in]   Size                  The number of bytes to allocate from the pool.
  @param[out]  Buffer                A pointer to a pointer to the allocated buffer if the call
                                     succeeds; undefined otherwise.

  @retval      EFI_SUCCESS           The requested number of bytes was allocated.
  @retval      EFI_OUT_OF_RESOURCES  The pool requested could not be allocated.
  @retval      EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseSmmAllocatePool (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      EFI_MEMORY_TYPE           PoolType,
  IN      UINTN                     Size,
  OUT     VOID                      **Buffer
  )
{
  gCommunicationData.FunctionData.Function = SMMBASE_ALLOCATE_POOL;
  gCommunicationData.FunctionData.Args.AllocatePool.PoolType = PoolType;
  gCommunicationData.FunctionData.Args.AllocatePool.Size = Size;
  gCommunicationData.FunctionData.Args.AllocatePool.Buffer = Buffer;

  SmmBaseHelperService ();
  return gCommunicationData.FunctionData.Status;
}

/**
  The SmmFreePool() function returns the memory specified by Buffer to the system.
  On return, the memory's type is EFI SMRAM Memory.  The Buffer that is freed must
  have been allocated by SmmAllocatePool().

  @param[in]  This                  Protocol instance pointer.
  @param[in]  Buffer                Pointer to the buffer allocation.

  @retval     EFI_SUCCESS           The memory was returned to the system.
  @retval     EFI_INVALID_PARAMETER Buffer was invalid.
  @retval     EFI_UNSUPPORTED       Platform is in runtime.
**/
EFI_STATUS
EFIAPI
SmmBaseSmmFreePool (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  IN      VOID                      *Buffer
  )
{
  gCommunicationData.FunctionData.Function = SMMBASE_FREE_POOL;
  gCommunicationData.FunctionData.Args.FreePool.Buffer = Buffer;

  SmmBaseHelperService ();
  return gCommunicationData.FunctionData.Status;
}

/**
  The GetSmstLocation() function returns the location of the System Management
  Service Table.  The use of the API is such that a driver can discover the
  location of the SMST in its entry point and then cache it in some driver
  global variable so that the SMST can be invoked in subsequent callbacks.

  @param[in]  This                  Protocol instance pointer.
  @param[in]  Smst                  Pointer to the SMST.

  @retval     EFI_SUCCESS           The operation was successful
  @retval     EFI_INVALID_PARAMETER Smst was invalid.
  @retval     EFI_UNSUPPORTED       Not in SMM.
**/
EFI_STATUS
EFIAPI
SmmBaseGetSmstLocation (
  IN      EFI_SMM_BASE_PROTOCOL     *This,
  OUT     EFI_SMM_SYSTEM_TABLE      **Smst
  )
{
  if (mSmmBaseHelperReady == NULL) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  if (!IsInSmm ()) {
    return EFI_UNSUPPORTED;
  }

  if (Smst == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Smst = mSmmBaseHelperReady->FrameworkSmst;
  return EFI_SUCCESS;
}

/**
  SMM Base Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
SmmBaseProtocolNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS Status;

  ///
  /// Assume only one instance of SMM Base2 Protocol in the system
  /// Locate SMM Base2 Protocol
  ///
  Status = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, (VOID **) &mSmmBase2);
  if (!EFI_ERROR (Status)) {
    ///
    /// Publish Framework SMM BASE Protocol immediately after SMM Base2 Protocol is installed to
    /// make SMM Base Protocol.InSmm() available as early as possible.
    ///
    Status = gBS->InstallProtocolInterface (
                    &mImageHandle,
                    &gEfiSmmBaseProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gSmmBase
                    );
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  SMM Communication Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
SmmCommunicationProtocolNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ///
  /// Assume only one instance of SMM Communication Protocol in the system
  /// Locate SMM Communication Protocol
  ///
  gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &mSmmCommunication);
}

/**
  SMM Base Helper Ready Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
SmmBaseHelperReadyProtocolNotification (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ///
  /// Assume only one instance of SMM Base Helper Ready Protocol in the system
  /// Locate SMM Base Helper Ready Protocol
  ///
  gBS->LocateProtocol (&gEfiSmmBaseHelperReadyProtocolGuid, NULL, (VOID **) &mSmmBaseHelperReady);
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked
  @param  Context      Pointer to the notification function's context
**/
VOID
EFIAPI
SmmBaseAddressChangeEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (mSmmCommunication != NULL) {
    EfiConvertPointer (0x0, (VOID **) &mSmmCommunication);
  }
}

/**
  Entry Point for SMM Base Protocol on SMM Base2 Protocol Thunk driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The entry point is executed successfully.
**/
EFI_STATUS
EFIAPI
SmmBaseThunkMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID       *Registration;
  EFI_EVENT  VirtualAddressChangeEvent;
  EFI_STATUS Status;

  mImageHandle = ImageHandle;

  ///
  /// Install notifications for required protocols
  ///
  /// Note we use protocol notifications here so as that this thunk driver can be
  /// loaded before PI SMM IPL driver. Framework SMM BASE Protocol will be published 
  /// immediately after SMM Base2 Protocol is installed to make SMM Base Protocol.InSmm()
  /// available as early as possible because some Framework code's behavior depends on
  /// correct detection of SMM mode via SMM Base Protocol.InSmm().
  ///
  /// Also SMM Base Helper driver is expected to be dispatched
  /// in the earliest round of SMM driver dispatch just after SMM IPL driver loads SMM Foundation.
  /// So the full functionality of SMM Base Protocol is ready immediately after SMM IPL driver is
  /// loaded. Since that point Framework SMM driver can be succesufully supported.
  ///
  EfiCreateProtocolNotifyEvent (
    &gEfiSmmBase2ProtocolGuid,
    TPL_CALLBACK,
    SmmBaseProtocolNotification,
    NULL,
    &Registration
    );

  EfiCreateProtocolNotifyEvent (
    &gEfiSmmCommunicationProtocolGuid,
    TPL_CALLBACK,
    SmmCommunicationProtocolNotification,
    NULL,
    &Registration
    );

  EfiCreateProtocolNotifyEvent (
    &gEfiSmmBaseHelperReadyProtocolGuid,
    TPL_CALLBACK,
    SmmBaseHelperReadyProtocolNotification,
    NULL,
    &Registration
    );

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SmmBaseAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &VirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

