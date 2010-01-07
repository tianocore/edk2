/** @file
  SMM Base Helper SMM driver.

  This driver is the counterpart of the SMM Base On SMM Base2 Thunk driver. It
  provides helping services in SMM to the SMM Base On SMM Base2 Thunk driver.

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmmBaseHelper.h"

EFI_HANDLE                         mDispatchHandle;
EFI_SMM_CPU_PROTOCOL               *mSmmCpu;
EFI_GUID                           mEfiSmmCpuIoGuid = EFI_SMM_CPU_IO_GUID;
EFI_SMM_BASE_HELPER_READY_PROTOCOL *mSmmBaseHelperReady;
EFI_SMM_SYSTEM_TABLE               *mFrameworkSmst;

LIST_ENTRY mCallbackInfoListHead = INITIALIZE_LIST_HEAD_VARIABLE (mCallbackInfoListHead);

CPU_SAVE_STATE_CONVERSION mCpuSaveStateConvTable[] = {
  {EFI_SMM_SAVE_STATE_REGISTER_LDTBASE  , CPU_SAVE_STATE_GET_OFFSET(LDTBase)},
  {EFI_SMM_SAVE_STATE_REGISTER_ES       , CPU_SAVE_STATE_GET_OFFSET(ES)},
  {EFI_SMM_SAVE_STATE_REGISTER_CS       , CPU_SAVE_STATE_GET_OFFSET(CS)},
  {EFI_SMM_SAVE_STATE_REGISTER_SS       , CPU_SAVE_STATE_GET_OFFSET(SS)},
  {EFI_SMM_SAVE_STATE_REGISTER_DS       , CPU_SAVE_STATE_GET_OFFSET(DS)},
  {EFI_SMM_SAVE_STATE_REGISTER_FS       , CPU_SAVE_STATE_GET_OFFSET(FS)},
  {EFI_SMM_SAVE_STATE_REGISTER_GS       , CPU_SAVE_STATE_GET_OFFSET(GS)},
  {EFI_SMM_SAVE_STATE_REGISTER_TR_SEL   , CPU_SAVE_STATE_GET_OFFSET(TR)},
  {EFI_SMM_SAVE_STATE_REGISTER_DR7      , CPU_SAVE_STATE_GET_OFFSET(DR7)},
  {EFI_SMM_SAVE_STATE_REGISTER_DR6      , CPU_SAVE_STATE_GET_OFFSET(DR6)},
  {EFI_SMM_SAVE_STATE_REGISTER_RAX      , CPU_SAVE_STATE_GET_OFFSET(EAX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RBX      , CPU_SAVE_STATE_GET_OFFSET(EBX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RCX      , CPU_SAVE_STATE_GET_OFFSET(ECX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RDX      , CPU_SAVE_STATE_GET_OFFSET(EDX)},
  {EFI_SMM_SAVE_STATE_REGISTER_RSP      , CPU_SAVE_STATE_GET_OFFSET(ESP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RBP      , CPU_SAVE_STATE_GET_OFFSET(EBP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RSI      , CPU_SAVE_STATE_GET_OFFSET(ESI)},
  {EFI_SMM_SAVE_STATE_REGISTER_RDI      , CPU_SAVE_STATE_GET_OFFSET(EDI)},
  {EFI_SMM_SAVE_STATE_REGISTER_RIP      , CPU_SAVE_STATE_GET_OFFSET(EIP)},
  {EFI_SMM_SAVE_STATE_REGISTER_RFLAGS   , CPU_SAVE_STATE_GET_OFFSET(EFLAGS)},
  {EFI_SMM_SAVE_STATE_REGISTER_CR0      , CPU_SAVE_STATE_GET_OFFSET(CR0)},
  {EFI_SMM_SAVE_STATE_REGISTER_CR3      , CPU_SAVE_STATE_GET_OFFSET(CR3)}
};

/**
  Framework SMST SmmInstallConfigurationTable() Thunk.

  This thunk calls the PI SMM SmmInstallConfigurationTable() and then update the configuration
  table related fields in the Framework SMST because the PI SMM SmmInstallConfigurationTable()
  function may modify these fields.

  @param[in] SystemTable         A pointer to the SMM System Table.
  @param[in] Guid                A pointer to the GUID for the entry to add, update, or remove.
  @param[in] Table               A pointer to the buffer of the table to add.
  @param[in] TableSize           The size of the table to install.

  @retval EFI_SUCCESS            The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER  Guid is not valid.
  @retval EFI_NOT_FOUND          An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES   There is not enough memory available to complete the operation.
**/
EFI_STATUS
EFIAPI
SmmInstallConfigurationTable (
  IN EFI_SMM_SYSTEM_TABLE  *SystemTable,
  IN EFI_GUID              *Guid,
  IN VOID                  *Table,
  IN UINTN                 TableSize
  )
{
  EFI_STATUS  Status;
  
  Status = gSmst->SmmInstallConfigurationTable (gSmst, Guid, Table, TableSize);
  if (!EFI_ERROR (Status)) {
    mFrameworkSmst->NumberOfTableEntries = gSmst->NumberOfTableEntries;
    mFrameworkSmst->SmmConfigurationTable = gSmst->SmmConfigurationTable;
  }
  return Status;         
}

/**
  Construct a Framework SMST based on the PI SMM SMST.

  @return  Pointer to the constructed Framework SMST.
**/
EFI_SMM_SYSTEM_TABLE *
ConstructFrameworkSmst (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_SMM_SYSTEM_TABLE  *FrameworkSmst;

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    sizeof (EFI_SMM_SYSTEM_TABLE),
                    (VOID **)&FrameworkSmst
                    );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Copy same things from PI SMST to Framework SMST
  ///
  CopyMem (FrameworkSmst, gSmst, (UINTN)(&((EFI_SMM_SYSTEM_TABLE *)0)->SmmIo));
  CopyMem (
    &FrameworkSmst->SmmIo, 
    &gSmst->SmmIo,
    sizeof (EFI_SMM_SYSTEM_TABLE) - (UINTN)(&((EFI_SMM_SYSTEM_TABLE *)0)->SmmIo)
    );

  ///
  /// Update Framework SMST
  ///
  FrameworkSmst->Hdr.Revision = EFI_SMM_SYSTEM_TABLE_REVISION;
  CopyGuid (&FrameworkSmst->EfiSmmCpuIoGuid, &mEfiSmmCpuIoGuid);

  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesData,
                    gSmst->NumberOfCpus * sizeof (EFI_SMM_CPU_SAVE_STATE),
                    (VOID **)&FrameworkSmst->CpuSaveState
                    );
  ASSERT_EFI_ERROR (Status);
  ZeroMem (FrameworkSmst->CpuSaveState, gSmst->NumberOfCpus * sizeof (EFI_SMM_CPU_SAVE_STATE));

  ///
  /// Do not support floating point state now
  ///
  FrameworkSmst->CpuOptionalFloatingPointState = NULL;

  FrameworkSmst->SmmInstallConfigurationTable = SmmInstallConfigurationTable;

  return FrameworkSmst;
}

/**
  Load a given Framework SMM driver into SMRAM and invoke its entry point.

  @param[in]   FilePath              Location of the image to be installed as the handler.
  @param[in]   SourceBuffer          Optional source buffer in case the image file
                                     is in memory.
  @param[in]   SourceSize            Size of the source image file, if in memory.
  @param[out]  ImageHandle           The handle that the base driver uses to decode 
                                     the handler. Unique among SMM handlers only, 
                                     not unique across DXE/EFI.

  @retval      EFI_SUCCESS           The operation was successful.
  @retval      EFI_OUT_OF_RESOURCES  There were no additional SMRAM resources to load the handler
  @retval      EFI_UNSUPPORTED       Can not find its copy in normal memory.
  @retval      EFI_INVALID_PARAMETER The handlers was not the correct image type
**/
EFI_STATUS
LoadImage (
  IN      EFI_DEVICE_PATH_PROTOCOL  *FilePath,
  IN      VOID                      *SourceBuffer,
  IN      UINTN                     SourceSize,
  OUT     EFI_HANDLE                *ImageHandle
  )
{
  EFI_STATUS                    Status;
  UINTN                         PageCount;
  EFI_PHYSICAL_ADDRESS          Buffer;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;
  EFI_HANDLE                    PesudoImageHandle;
  UINTN                         NumHandles;
  UINTN                         Index;
  EFI_HANDLE                    *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedImage;
  EFI_DEVICE_PATH               *LoadedImageDevicePath;
  UINTN                         DevicePathSize;

  if (FilePath == NULL || ImageHandle == NULL) {    
    return EFI_INVALID_PARAMETER;
  }

  ///
  /// Assume Framework SMM driver has an image copy in memory before registering itself into SMRAM.
  /// Currently only supports load Framework SMM driver from existing image copy in memory.
  /// Load PE32 Image Protocol can be used to support loading Framework SMM driver directly from FV.
  ///
  if (SourceBuffer == NULL) {
    Status = gBS->LocateHandleBuffer (
                    ByProtocol,
                    &gEfiLoadedImageDevicePathProtocolGuid,
                    NULL,
                    &NumHandles,
                    &HandleBuffer
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    DevicePathSize = GetDevicePathSize (FilePath);

    for (Index = 0; Index < NumHandles; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiLoadedImageDevicePathProtocolGuid,
                      (VOID **)&LoadedImageDevicePath
                      );
      ASSERT_EFI_ERROR (Status);

      if (GetDevicePathSize (LoadedImageDevicePath) == DevicePathSize &&
          CompareMem (LoadedImageDevicePath, FilePath, DevicePathSize) == 0) {
          break;
      }     
    }

    if (Index < NumHandles) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
      ASSERT_EFI_ERROR (Status);
      
      SourceBuffer = LoadedImage->ImageBase;
      gBS->FreePool (HandleBuffer);
    } else {
      gBS->FreePool (HandleBuffer);
      return EFI_UNSUPPORTED;
    }
  }

  ImageContext.Handle = SourceBuffer;
  ImageContext.ImageRead = PeCoffLoaderImageReadFromMemory;

  ///
  /// Get information about the image being loaded
  ///
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ///
  /// Allocate buffer for loading image into SMRAM
  ///
  PageCount = (UINTN)EFI_SIZE_TO_PAGES (ImageContext.ImageSize + ImageContext.SectionAlignment);
  Status = gSmst->SmmAllocatePages (AllocateAnyPages, EfiRuntimeServicesCode, PageCount, &Buffer);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ImageContext.ImageAddress = (PHYSICAL_ADDRESS)Buffer;

  ///
  /// Align buffer on section boundry
  ///
  ImageContext.ImageAddress += ImageContext.SectionAlignment - 1;
  ImageContext.ImageAddress &= ~(ImageContext.SectionAlignment - 1);

  ///
  /// Load the image into SMRAM
  ///
  Status = PeCoffLoaderLoadImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  ///
  /// Relocate the image in our new buffer
  ///
  Status = PeCoffLoaderRelocateImage (&ImageContext);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  ///
  /// Flush the instruction cache so the image data are written before we execute it
  ///
  InvalidateInstructionCacheRange ((VOID *)(UINTN) ImageContext.ImageAddress, (UINTN) ImageContext.ImageSize);

  ///
  /// Update MP state in Framework SMST before transferring control to Framework SMM driver entry point
  /// in case it may invoke AP
  ///
  mFrameworkSmst->CurrentlyExecutingCpu = gSmst->CurrentlyExecutingCpu;

  ///
  /// For Framework SMM, ImageHandle does not have to be a UEFI image handle.  The only requirement is that the 
  /// ImageHandle is a unique value.  Use image base address as the unique value.
  ///
  PesudoImageHandle = (EFI_HANDLE)(UINTN)ImageContext.ImageAddress;

  Status = ((EFI_IMAGE_ENTRY_POINT)(UINTN)ImageContext.EntryPoint) (PesudoImageHandle, gST);
  if (!EFI_ERROR (Status)) {
    *ImageHandle = PesudoImageHandle;
    return EFI_SUCCESS;
  }

Error:
  gSmst->SmmFreePages (Buffer, PageCount);
  return Status;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.Register().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
Register (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  EFI_STATUS Status;

  if (FunctionData->Args.Register.LegacyIA32Binary) {
    Status = EFI_UNSUPPORTED;
  } else {
    Status = LoadImage (
               FunctionData->Args.Register.FilePath,
               FunctionData->Args.Register.SourceBuffer,
               FunctionData->Args.Register.SourceSize,
               FunctionData->Args.Register.ImageHandle
               );
  }
  FunctionData->Status = Status;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.UnRegister().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
UnRegister (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  ///
  /// Unregister not supported now
  ///
  FunctionData->Status = EFI_UNSUPPORTED;
}

/**
  Search for Framework SMI handler information according to specific PI SMM dispatch handle.

  @param[in] DispatchHandle  The unique handle assigned by SmiHandlerRegister().  

  @return  Pointer to CALLBACK_INFO. If NULL, no callback info record is found.
**/
CALLBACK_INFO *
GetCallbackInfo (
  IN EFI_HANDLE  DispatchHandle
  )
{
  LIST_ENTRY  *Node;

  Node = GetFirstNode (&mCallbackInfoListHead);
  while (!IsNull (&mCallbackInfoListHead, Node)) {
    if (((CALLBACK_INFO *)Node)->DispatchHandle == DispatchHandle) {
      return (CALLBACK_INFO *)Node;
    }
    Node = GetNextNode (&mCallbackInfoListHead, Node);
  }
  return NULL;
}

/**
  Callback thunk for Framework SMI handler.

  This thunk functions calls the Framework SMI handler and converts the return value
  defined from Framework SMI handlers to a correpsonding return value defined by PI SMM.

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
CallbackThunk (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  )
{
  EFI_STATUS        Status;
  CALLBACK_INFO     *CallbackInfo;
  UINTN             Index;
  UINTN             CpuIndex;
  EFI_SMM_CPU_STATE *State;
  EFI_SMI_CPU_SAVE_STATE *SaveState;

  ///
  /// Before transferring the control into the Framework SMI handler, update CPU Save States
  /// and MP states in the Framework SMST.
  ///

  for (CpuIndex = 0; CpuIndex < gSmst->NumberOfCpus; CpuIndex++) {
    State = (EFI_SMM_CPU_STATE *)gSmst->CpuSaveState[CpuIndex];
    SaveState = &mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState;

    if (State->x86.SMMRevId < EFI_SMM_MIN_REV_ID_x64) {
      SaveState->SMBASE = State->x86.SMBASE;
      SaveState->SMMRevId = State->x86.SMMRevId;
      SaveState->IORestart = State->x86.IORestart;
      SaveState->AutoHALTRestart = State->x86.AutoHALTRestart;
    } else {
      SaveState->SMBASE = State->x64.SMBASE;
      SaveState->SMMRevId = State->x64.SMMRevId;
      SaveState->IORestart = State->x64.IORestart;
      SaveState->AutoHALTRestart = State->x64.AutoHALTRestart;
    }

    for (Index = 0; Index < sizeof (mCpuSaveStateConvTable) / sizeof (CPU_SAVE_STATE_CONVERSION); Index++) {
      ///
      /// Try to use SMM CPU Protocol to access CPU save states if possible
      ///
      Status = mSmmCpu->ReadSaveState (
                          mSmmCpu,
                          (UINTN)sizeof (UINT32),
                          mCpuSaveStateConvTable[Index].Register,
                          CpuIndex,
                          ((UINT8 *)SaveState) + mCpuSaveStateConvTable[Index].Offset
                          );
      ASSERT_EFI_ERROR (Status);
    }
  }

  mFrameworkSmst->CurrentlyExecutingCpu = gSmst->CurrentlyExecutingCpu;

  ///
  /// Search for Framework SMI handler information
  ///
  CallbackInfo = GetCallbackInfo (DispatchHandle);
  ASSERT (CallbackInfo != NULL);

  ///
  /// Thunk into original Framwork SMI handler
  ///
  Status = (CallbackInfo->CallbackAddress) (
                            CallbackInfo->SmmImageHandle,
                            CommBuffer,
                            CommBufferSize
                            );
  ///
  /// Save CPU Save States in case any of them was modified
  ///
  for (CpuIndex = 0; CpuIndex < gSmst->NumberOfCpus; CpuIndex++) {
    for (Index = 0; Index < sizeof (mCpuSaveStateConvTable) / sizeof (CPU_SAVE_STATE_CONVERSION); Index++) {
      Status = mSmmCpu->WriteSaveState (
                          mSmmCpu,
                          (UINTN)sizeof (UINT32),
                          mCpuSaveStateConvTable[Index].Register,
                          CpuIndex,
                          ((UINT8 *)&mFrameworkSmst->CpuSaveState[CpuIndex].Ia32SaveState) + 
                          mCpuSaveStateConvTable[Index].Offset
                          );
    }
  }

  ///
  /// Conversion of returned status code
  ///
  switch (Status) {
    case EFI_HANDLER_SUCCESS:
      Status = EFI_WARN_INTERRUPT_SOURCE_QUIESCED;
      break;
    case EFI_HANDLER_CRITICAL_EXIT:
    case EFI_HANDLER_SOURCE_QUIESCED:
      Status = EFI_SUCCESS;
      break;
    case EFI_HANDLER_SOURCE_PENDING:
      Status = EFI_WARN_INTERRUPT_SOURCE_PENDING;
      break;
  }
  return Status;
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.RegisterCallback().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
RegisterCallback (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  EFI_STATUS     Status;
  CALLBACK_INFO  *Buffer;

  ///
  /// Note that MakeLast and FloatingPointSave options are not supported in PI SMM
  ///

  ///
  /// Allocate buffer for callback thunk information
  ///
  Status = gSmst->SmmAllocatePool (
                    EfiRuntimeServicesCode,
                    sizeof (CALLBACK_INFO),
                    (VOID **)&Buffer
                    );
  if (!EFI_ERROR (Status)) {
    ///
    /// Fill SmmImageHandle and CallbackAddress into the thunk
    ///
    Buffer->SmmImageHandle = FunctionData->Args.RegisterCallback.SmmImageHandle;
    Buffer->CallbackAddress = FunctionData->Args.RegisterCallback.CallbackAddress;

    ///
    /// Register the thunk code as a root SMI handler
    ///
    Status = gSmst->SmiHandlerRegister (
                      CallbackThunk,
                      NULL,
                      &Buffer->DispatchHandle
                      );
    if (!EFI_ERROR (Status)) {
      ///
      /// Save this callback info
      ///
      InsertTailList (&mCallbackInfoListHead, &Buffer->Link);
    } else {
      gSmst->SmmFreePool (Buffer);
    }
  }
  FunctionData->Status = Status;
}


/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.SmmAllocatePool().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
HelperAllocatePool (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  FunctionData->Status = gSmst->SmmAllocatePool (
                                  FunctionData->Args.AllocatePool.PoolType,
                                  FunctionData->Args.AllocatePool.Size,
                                  FunctionData->Args.AllocatePool.Buffer
                                  );
}

/** 
  Thunk service of EFI_SMM_BASE_PROTOCOL.SmmFreePool().

  @param[in, out] FunctionData  Pointer to SMMBASE_FUNCTION_DATA.
**/
VOID
HelperFreePool (
  IN OUT SMMBASE_FUNCTION_DATA *FunctionData
  )
{
  FunctionData->Status = gSmst->SmmFreePool (
                                  FunctionData->Args.FreePool.Buffer
                                  );
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for the SMM Base Thunk driver.

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
SmmHandlerEntry (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *RegisterContext,
  IN OUT VOID                     *CommBuffer,
  IN OUT UINTN                    *CommBufferSize
  )
{
  SMMBASE_FUNCTION_DATA *FunctionData;

  ASSERT (CommBuffer != NULL);
  ASSERT (*CommBufferSize == sizeof (SMMBASE_FUNCTION_DATA));

  FunctionData = (SMMBASE_FUNCTION_DATA *)CommBuffer;

  switch (FunctionData->Function) {
    case SMMBASE_REGISTER:
      Register (FunctionData);
      break;
    case SMMBASE_UNREGISTER:
      UnRegister (FunctionData);
      break;
    case SMMBASE_REGISTER_CALLBACK:
      RegisterCallback (FunctionData);
      break;
    case SMMBASE_ALLOCATE_POOL:
      HelperAllocatePool (FunctionData);
      break;
    case SMMBASE_FREE_POOL:
      HelperFreePool (FunctionData);
      break;
    default:
      ASSERT (FALSE);
      FunctionData->Status = EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
  Entry point function of the SMM Base Helper SMM driver.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.  
  @param[in] SystemTable  A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmmBaseHelperMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;

  ///
  /// Locate SMM CPU Protocol which is used later to retrieve/update CPU Save States
  ///
  Status = gSmst->SmmLocateProtocol (&gEfiSmmCpuProtocolGuid, NULL, (VOID **) &mSmmCpu);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Interface structure of SMM BASE Helper Ready Protocol is allocated from UEFI pool
  /// instead of SMM pool so that SMM Base Thunk driver can access it in Non-SMM mode.
  ///
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (EFI_SMM_BASE_HELPER_READY_PROTOCOL),
                  (VOID **)&mSmmBaseHelperReady
                  );
  ASSERT_EFI_ERROR (Status);

  ///
  /// Construct Framework SMST from PI SMST
  ///
  mFrameworkSmst = ConstructFrameworkSmst ();
  mSmmBaseHelperReady->FrameworkSmst = mFrameworkSmst;
  mSmmBaseHelperReady->ServiceEntry = SmmHandlerEntry;

  ///
  /// Register SMM Base Helper services for SMM Base Thunk driver
  ///
  Status = gSmst->SmiHandlerRegister (SmmHandlerEntry, &gEfiSmmBaseThunkCommunicationGuid, &mDispatchHandle);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Install EFI SMM Base Helper Protocol in the UEFI handle database
  ///
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiSmmBaseHelperReadyProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  mSmmBaseHelperReady
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

