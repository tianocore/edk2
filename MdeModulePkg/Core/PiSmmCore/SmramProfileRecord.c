/** @file
  Support routines for SMRAM profile.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCore.h"

#define IS_SMRAM_PROFILE_ENABLED ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT1) != 0)
#define IS_UEFI_MEMORY_PROFILE_ENABLED ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT0) != 0)

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_CONTEXT        Context;
  LIST_ENTRY                    *DriverInfoList;
} MEMORY_PROFILE_CONTEXT_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_DRIVER_INFO    DriverInfo;
  LIST_ENTRY                    *AllocInfoList;
  CHAR8                         *PdbString;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_DRIVER_INFO_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_ALLOC_INFO     AllocInfo;
  CHAR8                         *ActionString;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_ALLOC_INFO_DATA;

//
// When free memory less than 4 pages, dump it.
//
#define SMRAM_INFO_DUMP_PAGE_THRESHOLD  4

GLOBAL_REMOVE_IF_UNREFERENCED MEMORY_PROFILE_FREE_MEMORY mSmramFreeMemory = {
  {
    MEMORY_PROFILE_FREE_MEMORY_SIGNATURE,
    sizeof (MEMORY_PROFILE_FREE_MEMORY),
    MEMORY_PROFILE_FREE_MEMORY_REVISION
  },
  0,
  0
};

GLOBAL_REMOVE_IF_UNREFERENCED LIST_ENTRY  mImageQueue = INITIALIZE_LIST_HEAD_VARIABLE (mImageQueue);
GLOBAL_REMOVE_IF_UNREFERENCED MEMORY_PROFILE_CONTEXT_DATA mSmramProfileContext = {
  MEMORY_PROFILE_CONTEXT_SIGNATURE,
  {
    {
      MEMORY_PROFILE_CONTEXT_SIGNATURE,
      sizeof (MEMORY_PROFILE_CONTEXT),
      MEMORY_PROFILE_CONTEXT_REVISION
    },
    0,
    0,
    {0},
    {0},
    0,
    0,
    0
  },
  &mImageQueue,
};
GLOBAL_REMOVE_IF_UNREFERENCED MEMORY_PROFILE_CONTEXT_DATA *mSmramProfileContextPtr = NULL;

GLOBAL_REMOVE_IF_UNREFERENCED BOOLEAN mSmramReadyToLock;
GLOBAL_REMOVE_IF_UNREFERENCED BOOLEAN mSmramProfileGettingStatus = FALSE;
GLOBAL_REMOVE_IF_UNREFERENCED BOOLEAN mSmramProfileRecordingEnable = MEMORY_PROFILE_RECORDING_DISABLE;
GLOBAL_REMOVE_IF_UNREFERENCED EFI_DEVICE_PATH_PROTOCOL *mSmramProfileDriverPath;
GLOBAL_REMOVE_IF_UNREFERENCED UINTN                    mSmramProfileDriverPathSize;

/**
  Dump SMRAM information.

**/
VOID
DumpSmramInfo (
  VOID
  );

/**
  Get memory profile data.

  @param[in]      This              The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in, out] ProfileSize       On entry, points to the size in bytes of the ProfileBuffer.
                                    On return, points to the size of the data returned in ProfileBuffer.
  @param[out]     ProfileBuffer     Profile buffer.

  @return EFI_SUCCESS               Get the memory profile data successfully.
  @return EFI_UNSUPPORTED           Memory profile is unsupported.
  @return EFI_BUFFER_TO_SMALL       The ProfileSize is too small for the resulting data.
                                    ProfileSize is updated with the size required.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolGetData (
  IN     EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN OUT UINT64                             *ProfileSize,
     OUT VOID                               *ProfileBuffer
  );

/**
  Register image to memory profile.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.
  @param[in] FileType           File type of the image.

  @return EFI_SUCCESS           Register successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCE   No enough resource for this register.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolRegisterImage (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize,
  IN EFI_FV_FILETYPE                    FileType
  );

/**
  Unregister image from memory profile.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.

  @return EFI_SUCCESS           Unregister successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolUnregisterImage (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize
  );

/**
  Get memory profile recording state.

  @param[in]  This              The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[out] RecordingState    Recording state.

  @return EFI_SUCCESS           Memory profile recording state is returned.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.
  @return EFI_INVALID_PARAMETER RecordingState is NULL.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolGetRecordingState (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  OUT BOOLEAN                           *RecordingState
  );

/**
  Set memory profile recording state.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] RecordingState     Recording state.

  @return EFI_SUCCESS           Set memory profile recording state successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolSetRecordingState (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN BOOLEAN                            RecordingState
  );

/**
  Record memory profile of multilevel caller.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] CallerAddress      Address of caller.
  @param[in] Action             Memory profile action.
  @param[in] MemoryType         Memory type.
                                EfiMaxMemoryType means the MemoryType is unknown.
  @param[in] Buffer             Buffer address.
  @param[in] Size               Buffer size.
  @param[in] ActionString       String for memory profile action.
                                Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolRecord (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN PHYSICAL_ADDRESS                   CallerAddress,
  IN MEMORY_PROFILE_ACTION              Action,
  IN EFI_MEMORY_TYPE                    MemoryType,
  IN VOID                               *Buffer,
  IN UINTN                              Size,
  IN CHAR8                              *ActionString OPTIONAL
  );

GLOBAL_REMOVE_IF_UNREFERENCED EDKII_SMM_MEMORY_PROFILE_PROTOCOL mSmmProfileProtocol = {
  SmramProfileProtocolGetData,
  SmramProfileProtocolRegisterImage,
  SmramProfileProtocolUnregisterImage,
  SmramProfileProtocolGetRecordingState,
  SmramProfileProtocolSetRecordingState,
  SmramProfileProtocolRecord,
};

/**
  Return SMRAM profile context.

  @return SMRAM profile context.

**/
MEMORY_PROFILE_CONTEXT_DATA *
GetSmramProfileContext (
  VOID
  )
{
  return mSmramProfileContextPtr;
}

/**
  Retrieves and returns the Subsystem of a PE/COFF image that has been loaded into system memory.
  If Pe32Data is NULL, then ASSERT().

  @param Pe32Data   The pointer to the PE/COFF image that is loaded in system memory.

  @return The Subsystem of the PE/COFF image.

**/
UINT16
InternalPeCoffGetSubsystem (
  IN VOID  *Pe32Data
  )
{
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT16                               Magic;

  ASSERT (Pe32Data != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) ((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) Pe32Data;
  }

  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    return Hdr.Te->Subsystem;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE)  {
    Magic = Hdr.Pe32->OptionalHeader.Magic;
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      return Hdr.Pe32->OptionalHeader.Subsystem;
    } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      return Hdr.Pe32Plus->OptionalHeader.Subsystem;
    }
  }

  return 0x0000;
}

/**
  Retrieves and returns a pointer to the entry point to a PE/COFF image that has been loaded
  into system memory with the PE/COFF Loader Library functions.

  Retrieves the entry point to the PE/COFF image specified by Pe32Data and returns this entry
  point in EntryPoint.  If the entry point could not be retrieved from the PE/COFF image, then
  return RETURN_INVALID_PARAMETER.  Otherwise return RETURN_SUCCESS.
  If Pe32Data is NULL, then ASSERT().
  If EntryPoint is NULL, then ASSERT().

  @param  Pe32Data                  The pointer to the PE/COFF image that is loaded in system memory.
  @param  EntryPoint                The pointer to entry point to the PE/COFF image to return.

  @retval RETURN_SUCCESS            EntryPoint was returned.
  @retval RETURN_INVALID_PARAMETER  The entry point could not be found in the PE/COFF image.

**/
RETURN_STATUS
InternalPeCoffGetEntryPoint (
  IN  VOID  *Pe32Data,
  OUT VOID  **EntryPoint
  )
{
  EFI_IMAGE_DOS_HEADER                  *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;

  ASSERT (Pe32Data   != NULL);
  ASSERT (EntryPoint != NULL);

  DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present, so read the PE header after the DOS image header.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) ((UINTN) Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
  } else {
    //
    // DOS image header is not present, so PE header is at the image base.
    //
    Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) Pe32Data;
  }

  //
  // Calculate the entry point relative to the start of the image.
  // AddressOfEntryPoint is common for PE32 & PE32+
  //
  if (Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    *EntryPoint = (VOID *) ((UINTN) Pe32Data + (UINTN) (Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) + sizeof (EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize);
    return RETURN_SUCCESS;
  } else if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
    *EntryPoint = (VOID *) ((UINTN) Pe32Data + (UINTN) (Hdr.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff));
    return RETURN_SUCCESS;
  }

  return RETURN_UNSUPPORTED;
}

/**
  Build driver info.

  @param ContextData    Memory profile context.
  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.
  @param EntryPoint     Entry point of the image.
  @param ImageSubsystem Image subsystem of the image.
  @param FileType       File type of the image.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
BuildDriverInfo (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize,
  IN PHYSICAL_ADDRESS               EntryPoint,
  IN UINT16                         ImageSubsystem,
  IN EFI_FV_FILETYPE                FileType
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  VOID                              *EntryPointInImage;
  CHAR8                             *PdbString;
  UINTN                             PdbSize;
  UINTN                             PdbOccupiedSize;

  PdbSize = 0;
  PdbOccupiedSize = 0;
  PdbString = NULL;
  if (ImageBase != 0) {
    PdbString = PeCoffLoaderGetPdbPointer ((VOID*) (UINTN) ImageBase);
    if (PdbString != NULL) {
      PdbSize = AsciiStrSize (PdbString);
      PdbOccupiedSize = GET_OCCUPIED_SIZE (PdbSize, sizeof (UINT64));
    }
  }

  //
  // Use SmmInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  Status = SmmInternalAllocatePool (
             EfiRuntimeServicesData,
             sizeof (*DriverInfoData) + sizeof (LIST_ENTRY) + PdbSize,
             (VOID **) &DriverInfoData
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  ASSERT (DriverInfoData != NULL);

  ZeroMem (DriverInfoData, sizeof (*DriverInfoData));

  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfoData->Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Length = (UINT16) (sizeof (MEMORY_PROFILE_DRIVER_INFO) + PdbOccupiedSize);
  DriverInfo->Header.Revision = MEMORY_PROFILE_DRIVER_INFO_REVISION;
  if (FileName != NULL) {
    CopyMem (&DriverInfo->FileName, FileName, sizeof (EFI_GUID));
  }
  DriverInfo->ImageBase = ImageBase;
  DriverInfo->ImageSize = ImageSize;
  DriverInfo->EntryPoint = EntryPoint;
  DriverInfo->ImageSubsystem = ImageSubsystem;
  if ((EntryPoint != 0) && ((EntryPoint < ImageBase) || (EntryPoint >= (ImageBase + ImageSize)))) {
    //
    // If the EntryPoint is not in the range of image buffer, it should come from emulation environment.
    // So patch ImageBuffer here to align the EntryPoint.
    //
    Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageBase, &EntryPointInImage);
    ASSERT_EFI_ERROR (Status);
    DriverInfo->ImageBase = ImageBase + EntryPoint - (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;
  }
  DriverInfo->FileType = FileType;
  DriverInfoData->AllocInfoList = (LIST_ENTRY *) (DriverInfoData + 1);
  InitializeListHead (DriverInfoData->AllocInfoList);
  DriverInfo->CurrentUsage = 0;
  DriverInfo->PeakUsage = 0;
  DriverInfo->AllocRecordCount = 0;
  if (PdbSize != 0) {
    DriverInfo->PdbStringOffset = (UINT16) sizeof (MEMORY_PROFILE_DRIVER_INFO);
    DriverInfoData->PdbString = (CHAR8 *) (DriverInfoData->AllocInfoList + 1);
    CopyMem (DriverInfoData->PdbString, PdbString, PdbSize);
  } else {
    DriverInfo->PdbStringOffset = 0;
    DriverInfoData->PdbString = NULL;
  }

  InsertTailList (ContextData->DriverInfoList, &DriverInfoData->Link);
  ContextData->Context.ImageCount ++;
  ContextData->Context.TotalImageSize += DriverInfo->ImageSize;

  return DriverInfoData;
}

/**
  Register image to DXE.

  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.
  @param FileType       File type of the image.

**/
VOID
RegisterImageToDxe (
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize,
  IN EFI_FV_FILETYPE                FileType
  )
{
  EFI_STATUS                        Status;
  EDKII_MEMORY_PROFILE_PROTOCOL     *ProfileProtocol;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;
  UINT8                             TempBuffer[sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof (EFI_DEVICE_PATH_PROTOCOL)];

  if (IS_UEFI_MEMORY_PROFILE_ENABLED) {

    FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)TempBuffer;
    Status = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID **) &ProfileProtocol);
    if (!EFI_ERROR (Status)) {
      EfiInitializeFwVolDevicepathNode (FilePath, FileName);
      SetDevicePathEndNode (FilePath + 1);

      Status = ProfileProtocol->RegisterImage (
                                  ProfileProtocol,
                                  (EFI_DEVICE_PATH_PROTOCOL *) FilePath,
                                  ImageBase,
                                  ImageSize,
                                  FileType
                                  );
    }
  }
}

/**
  Unregister image from DXE.

  @param FileName       File name of the image.
  @param ImageBase      Image base address.
  @param ImageSize      Image size.

**/
VOID
UnregisterImageFromDxe (
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               ImageBase,
  IN UINT64                         ImageSize
  )
{
  EFI_STATUS                        Status;
  EDKII_MEMORY_PROFILE_PROTOCOL     *ProfileProtocol;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;
  UINT8                             TempBuffer[sizeof (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof (EFI_DEVICE_PATH_PROTOCOL)];

  if (IS_UEFI_MEMORY_PROFILE_ENABLED) {

    FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *)TempBuffer;
    Status = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID *) &ProfileProtocol);
    if (!EFI_ERROR (Status)) {
      EfiInitializeFwVolDevicepathNode (FilePath, FileName);
      SetDevicePathEndNode (FilePath + 1);

      Status = ProfileProtocol->UnregisterImage (
                                  ProfileProtocol,
                                  (EFI_DEVICE_PATH_PROTOCOL *) FilePath,
                                  ImageBase,
                                  ImageSize
                                  );
    }
  }
}

/**
  Return if record for this driver is needed..

  @param DriverFilePath     Driver file path.

  @retval TRUE              Record for this driver is needed.
  @retval FALSE             Record for this driver is not needed.

**/
BOOLEAN
NeedRecordThisDriver (
  IN EFI_DEVICE_PATH_PROTOCOL       *DriverFilePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePathInstance;
  UINTN                       DevicePathSize;
  UINTN                       FilePathSize;

  if (!IsDevicePathValid (mSmramProfileDriverPath, mSmramProfileDriverPathSize)) {
    //
    // Invalid Device Path means record all.
    //
    return TRUE;
  }

  //
  // Record FilePath without end node.
  //
  FilePathSize = GetDevicePathSize (DriverFilePath) - sizeof(EFI_DEVICE_PATH_PROTOCOL);

  DevicePathInstance = mSmramProfileDriverPath;
  do {
    //
    // Find End node (it might be END_ENTIRE or END_INSTANCE)
    //
    TmpDevicePath = DevicePathInstance;
    while (!IsDevicePathEndType (TmpDevicePath)) {
      TmpDevicePath = NextDevicePathNode (TmpDevicePath);
    }

    //
    // Do not compare END node
    //
    DevicePathSize = (UINTN)TmpDevicePath - (UINTN)DevicePathInstance;
    if ((FilePathSize == DevicePathSize) &&
        (CompareMem (DriverFilePath, DevicePathInstance, DevicePathSize) == 0)) {
      return TRUE;
    }

    //
    // Get next instance
    //
    DevicePathInstance = (EFI_DEVICE_PATH_PROTOCOL *)((UINTN)DevicePathInstance + DevicePathSize + DevicePathNodeLength(TmpDevicePath));
  } while (DevicePathSubType (TmpDevicePath) != END_ENTIRE_DEVICE_PATH_SUBTYPE);

  return FALSE;
}

/**
  Register SMM Core to SMRAM profile.

  @param ContextData    SMRAM profile context.

  @retval TRUE          Register success.
  @retval FALSE         Register fail.

**/
BOOLEAN
RegisterSmmCore (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData
  )
{
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  PHYSICAL_ADDRESS                  ImageBase;
  UINT8                             TempBuffer[sizeof(MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof(EFI_DEVICE_PATH_PROTOCOL)];
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;

  FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) TempBuffer;
  EfiInitializeFwVolDevicepathNode (FilePath, &gEfiCallerIdGuid);
  SetDevicePathEndNode (FilePath + 1);

  if (!NeedRecordThisDriver ((EFI_DEVICE_PATH_PROTOCOL *) FilePath)) {
    return FALSE;
  }

  ImageBase = gSmmCorePrivate->PiSmmCoreImageBase;
  DriverInfoData = BuildDriverInfo (
                     ContextData,
                     &gEfiCallerIdGuid,
                     ImageBase,
                     gSmmCorePrivate->PiSmmCoreImageSize,
                     gSmmCorePrivate->PiSmmCoreEntryPoint,
                     InternalPeCoffGetSubsystem ((VOID *) (UINTN) ImageBase),
                     EFI_FV_FILETYPE_SMM_CORE
                     );
  if (DriverInfoData == NULL) {
    return FALSE;
  }

  return TRUE;
}

/**
  Initialize SMRAM profile.

**/
VOID
SmramProfileInit (
  VOID
  )
{
  MEMORY_PROFILE_CONTEXT_DATA *SmramProfileContext;

  RegisterImageToDxe (
    &gEfiCallerIdGuid,
    gSmmCorePrivate->PiSmmCoreImageBase,
    gSmmCorePrivate->PiSmmCoreImageSize,
    EFI_FV_FILETYPE_SMM_CORE
    );

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return;
  }

  SmramProfileContext = GetSmramProfileContext ();
  if (SmramProfileContext != NULL) {
    return;
  }

  mSmramProfileGettingStatus = FALSE;
  if ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT7) != 0) {
    mSmramProfileRecordingEnable = MEMORY_PROFILE_RECORDING_DISABLE;
  } else {
    mSmramProfileRecordingEnable = MEMORY_PROFILE_RECORDING_ENABLE;
  }
  mSmramProfileDriverPathSize = PcdGetSize (PcdMemoryProfileDriverPath);
  mSmramProfileDriverPath = AllocateCopyPool (mSmramProfileDriverPathSize, PcdGetPtr (PcdMemoryProfileDriverPath));
  mSmramProfileContextPtr = &mSmramProfileContext;

  RegisterSmmCore (&mSmramProfileContext);

  DEBUG ((EFI_D_INFO, "SmramProfileInit SmramProfileContext - 0x%x\n", &mSmramProfileContext));
}

/**
  Install SMRAM profile protocol.

**/
VOID
SmramProfileInstallProtocol (
  VOID
  )
{
  EFI_HANDLE    Handle;
  EFI_STATUS    Status;

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return;
  }

  Handle = NULL;
  Status = SmmInstallProtocolInterface (
             &Handle,
             &gEdkiiSmmMemoryProfileGuid,
             EFI_NATIVE_INTERFACE,
             &mSmmProfileProtocol
             );
  ASSERT_EFI_ERROR (Status);
}

/**
  Get the GUID file name from the file path.

  @param FilePath  File path.

  @return The GUID file name from the file path.

**/
EFI_GUID *
GetFileNameFromFilePath (
  IN EFI_DEVICE_PATH_PROTOCOL   *FilePath
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *ThisFilePath;
  EFI_GUID                              *FileName;

  FileName = NULL;
  if (FilePath != NULL) {
    ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) FilePath;
    while (!IsDevicePathEnd (ThisFilePath)) {
      FileName = EfiGetNameGuidFromFwVolDevicePathNode (ThisFilePath);
      if (FileName != NULL) {
        break;
      }
      ThisFilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) NextDevicePathNode (ThisFilePath);
    }
  }

  return FileName;
}

/**
  Register SMM image to SMRAM profile.

  @param DriverEntry    SMM image info.
  @param RegisterToDxe  Register image to DXE.

  @return EFI_SUCCESS           Register successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCES  No enough resource for this register.

**/
EFI_STATUS
RegisterSmramProfileImage (
  IN EFI_SMM_DRIVER_ENTRY   *DriverEntry,
  IN BOOLEAN                RegisterToDxe
  )
{
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  UINT8                             TempBuffer[sizeof(MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof(EFI_DEVICE_PATH_PROTOCOL)];
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;

  if (RegisterToDxe) {
    RegisterImageToDxe (
      &DriverEntry->FileName,
      DriverEntry->ImageBuffer,
      EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage),
      EFI_FV_FILETYPE_SMM
      );
  }

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return EFI_UNSUPPORTED;
  }

  FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) TempBuffer;
  EfiInitializeFwVolDevicepathNode (FilePath, &DriverEntry->FileName);
  SetDevicePathEndNode (FilePath + 1);

  if (!NeedRecordThisDriver ((EFI_DEVICE_PATH_PROTOCOL *) FilePath)) {
    return EFI_UNSUPPORTED;
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  DriverInfoData = BuildDriverInfo (
                     ContextData,
                     &DriverEntry->FileName,
                     DriverEntry->ImageBuffer,
                     EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage),
                     DriverEntry->ImageEntryPoint,
                     InternalPeCoffGetSubsystem ((VOID *) (UINTN) DriverEntry->ImageBuffer),
                     EFI_FV_FILETYPE_SMM
                     );
  if (DriverInfoData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Search image from memory profile.

  @param ContextData    Memory profile context.
  @param FileName       Image file name.
  @param Address        Image Address.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
GetMemoryProfileDriverInfoByFileNameAndAddress (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN EFI_GUID                       *FileName,
  IN PHYSICAL_ADDRESS               Address
  )
{
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *DriverInfoList;

  DriverInfoList = ContextData->DriverInfoList;

  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    DriverInfo = &DriverInfoData->DriverInfo;
    if ((CompareGuid (&DriverInfo->FileName, FileName)) &&
        (Address >= DriverInfo->ImageBase) &&
        (Address < (DriverInfo->ImageBase + DriverInfo->ImageSize))) {
      return DriverInfoData;
    }
  }

  return NULL;
}

/**
  Search image from memory profile.
  It will return image, if (Address >= ImageBuffer) AND (Address < ImageBuffer + ImageSize)

  @param ContextData    Memory profile context.
  @param Address        Image or Function address.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
GetMemoryProfileDriverInfoFromAddress (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData,
  IN PHYSICAL_ADDRESS               Address
  )
{
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *DriverInfoList;

  DriverInfoList = ContextData->DriverInfoList;

  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    DriverInfo = &DriverInfoData->DriverInfo;
    if ((Address >= DriverInfo->ImageBase) &&
        (Address < (DriverInfo->ImageBase + DriverInfo->ImageSize))) {
      return DriverInfoData;
    }
  }

  return NULL;
}

/**
  Unregister image from SMRAM profile.

  @param DriverEntry        SMM image info.
  @param UnregisterFromDxe  Unregister image from DXE.

  @return EFI_SUCCESS           Unregister successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
UnregisterSmramProfileImage (
  IN EFI_SMM_DRIVER_ENTRY  *DriverEntry,
  IN BOOLEAN               UnregisterFromDxe
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  EFI_GUID                          *FileName;
  PHYSICAL_ADDRESS                  ImageAddress;
  VOID                              *EntryPointInImage;
  UINT8                             TempBuffer[sizeof(MEDIA_FW_VOL_FILEPATH_DEVICE_PATH) + sizeof(EFI_DEVICE_PATH_PROTOCOL)];
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FilePath;

  if (UnregisterFromDxe) {
    UnregisterImageFromDxe (
      &DriverEntry->FileName,
      DriverEntry->ImageBuffer,
      EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage)
      );
  }

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return EFI_UNSUPPORTED;
  }

  FilePath = (MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) TempBuffer;
  EfiInitializeFwVolDevicepathNode (FilePath, &DriverEntry->FileName);
  SetDevicePathEndNode (FilePath + 1);

  if (!NeedRecordThisDriver ((EFI_DEVICE_PATH_PROTOCOL *) FilePath)) {
    return EFI_UNSUPPORTED;
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  DriverInfoData = NULL;
  FileName = &DriverEntry->FileName;
  ImageAddress = DriverEntry->ImageBuffer;
  if ((DriverEntry->ImageEntryPoint < ImageAddress) || (DriverEntry->ImageEntryPoint >= (ImageAddress + EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage)))) {
    //
    // If the EntryPoint is not in the range of image buffer, it should come from emulation environment.
    // So patch ImageAddress here to align the EntryPoint.
    //
    Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) ImageAddress, &EntryPointInImage);
    ASSERT_EFI_ERROR (Status);
    ImageAddress = ImageAddress + (UINTN) DriverEntry->ImageEntryPoint - (UINTN) EntryPointInImage;
  }
  if (FileName != NULL) {
    DriverInfoData = GetMemoryProfileDriverInfoByFileNameAndAddress (ContextData, FileName, ImageAddress);
  }
  if (DriverInfoData == NULL) {
    DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, ImageAddress);
  }
  if (DriverInfoData == NULL) {
    return EFI_NOT_FOUND;
  }

  ContextData->Context.TotalImageSize -= DriverInfoData->DriverInfo.ImageSize;

  // Keep the ImageBase for RVA calculation in Application.
  //DriverInfoData->DriverInfo.ImageBase = 0;
  DriverInfoData->DriverInfo.ImageSize = 0;

  if (DriverInfoData->DriverInfo.PeakUsage == 0) {
    ContextData->Context.ImageCount --;
    RemoveEntryList (&DriverInfoData->Link);
    //
    // Use SmmInternalFreePool() that will not update profile for this FreePool action.
    //
    SmmInternalFreePool (DriverInfoData);
  }

  return EFI_SUCCESS;
}

/**
  Return if this memory type needs to be recorded into memory profile.
  Only need to record EfiRuntimeServicesCode and EfiRuntimeServicesData for SMRAM profile.

  @param MemoryType     Memory type.

  @retval TRUE          This memory type need to be recorded.
  @retval FALSE         This memory type need not to be recorded.

**/
BOOLEAN
SmmCoreNeedRecordProfile (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  UINT64 TestBit;

  if (MemoryType != EfiRuntimeServicesCode &&
      MemoryType != EfiRuntimeServicesData) {
    return FALSE;
  }

  TestBit = LShiftU64 (1, MemoryType);

  if ((PcdGet64 (PcdMemoryProfileMemoryType) & TestBit) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Convert EFI memory type to profile memory index. The rule is:
  If BIOS memory type (0 ~ EfiMaxMemoryType - 1), ProfileMemoryIndex = MemoryType.
  As SMRAM profile is only to record EfiRuntimeServicesCode and EfiRuntimeServicesData,
  so return input memory type directly.

  @param MemoryType     Memory type.

  @return EFI memory type as profile memory index.

**/
EFI_MEMORY_TYPE
GetProfileMemoryIndex (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  return MemoryType;
}

/**
  Update SMRAM profile FreeMemoryPages information

  @param ContextData    Memory profile context.

**/
VOID
SmramProfileUpdateFreePages (
  IN MEMORY_PROFILE_CONTEXT_DATA  *ContextData
  )
{
  LIST_ENTRY                      *Node;
  FREE_PAGE_LIST                  *Pages;
  LIST_ENTRY                      *FreePageList;
  UINTN                           NumberOfPages;

  NumberOfPages = 0;
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink;
       Node != FreePageList;
       Node = Node->BackLink) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    NumberOfPages += Pages->NumberOfPages;
  }

  mSmramFreeMemory.TotalFreeMemoryPages = NumberOfPages;

  if (NumberOfPages <= SMRAM_INFO_DUMP_PAGE_THRESHOLD) {
    DumpSmramInfo ();
  }
}

/**
  Update SMRAM profile Allocate information.

  @param CallerAddress  Address of caller who call Allocate.
  @param Action         This Allocate action.
  @param MemoryType     Memory type.
  @param Size           Buffer size.
  @param Buffer         Buffer address.
  @param ActionString   String for memory profile action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.

**/
EFI_STATUS
SmmCoreUpdateProfileAllocate (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  Size,
  IN VOID                   *Buffer,
  IN CHAR8                  *ActionString OPTIONAL
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_CONTEXT            *Context;
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO         *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  EFI_MEMORY_TYPE                   ProfileMemoryIndex;
  MEMORY_PROFILE_ACTION             BasicAction;
  UINTN                             ActionStringSize;
  UINTN                             ActionStringOccupiedSize;

  BasicAction = Action & MEMORY_PROFILE_ACTION_BASIC_MASK;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);
  if (DriverInfoData == NULL) {
    return EFI_UNSUPPORTED;
  }

  ActionStringSize = 0;
  ActionStringOccupiedSize = 0;
  if (ActionString != NULL) {
    ActionStringSize = AsciiStrSize (ActionString);
    ActionStringOccupiedSize = GET_OCCUPIED_SIZE (ActionStringSize, sizeof (UINT64));
  }

  //
  // Use SmmInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  AllocInfoData = NULL;
  Status = SmmInternalAllocatePool (
             EfiRuntimeServicesData,
             sizeof (*AllocInfoData) + ActionStringSize,
             (VOID **) &AllocInfoData
             );
  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }
  ASSERT (AllocInfoData != NULL);

  //
  // Only update SequenceCount if and only if it is basic action.
  //
  if (Action == BasicAction) {
    ContextData->Context.SequenceCount ++;
  }

  AllocInfo = &AllocInfoData->AllocInfo;
  AllocInfoData->Signature      = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Signature   = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Length      = (UINT16) (sizeof (MEMORY_PROFILE_ALLOC_INFO) + ActionStringOccupiedSize);
  AllocInfo->Header.Revision    = MEMORY_PROFILE_ALLOC_INFO_REVISION;
  AllocInfo->CallerAddress      = CallerAddress;
  AllocInfo->SequenceId         = ContextData->Context.SequenceCount;
  AllocInfo->Action             = Action;
  AllocInfo->MemoryType         = MemoryType;
  AllocInfo->Buffer             = (PHYSICAL_ADDRESS) (UINTN) Buffer;
  AllocInfo->Size               = Size;
  if (ActionString != NULL) {
    AllocInfo->ActionStringOffset = (UINT16) sizeof (MEMORY_PROFILE_ALLOC_INFO);
    AllocInfoData->ActionString = (CHAR8 *) (AllocInfoData + 1);
    CopyMem (AllocInfoData->ActionString, ActionString, ActionStringSize);
  } else {
    AllocInfo->ActionStringOffset = 0;
    AllocInfoData->ActionString = NULL;
  }

  InsertTailList (DriverInfoData->AllocInfoList, &AllocInfoData->Link);

  Context = &ContextData->Context;
  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfo->AllocRecordCount ++;

  //
  // Update summary if and only if it is basic action.
  //
  if (Action == BasicAction) {
    ProfileMemoryIndex = GetProfileMemoryIndex (MemoryType);

    DriverInfo->CurrentUsage += Size;
    if (DriverInfo->PeakUsage < DriverInfo->CurrentUsage) {
      DriverInfo->PeakUsage = DriverInfo->CurrentUsage;
    }
    DriverInfo->CurrentUsageByType[ProfileMemoryIndex] += Size;
    if (DriverInfo->PeakUsageByType[ProfileMemoryIndex] < DriverInfo->CurrentUsageByType[ProfileMemoryIndex]) {
      DriverInfo->PeakUsageByType[ProfileMemoryIndex] = DriverInfo->CurrentUsageByType[ProfileMemoryIndex];
    }

    Context->CurrentTotalUsage += Size;
    if (Context->PeakTotalUsage < Context->CurrentTotalUsage) {
      Context->PeakTotalUsage = Context->CurrentTotalUsage;
    }
    Context->CurrentTotalUsageByType[ProfileMemoryIndex] += Size;
    if (Context->PeakTotalUsageByType[ProfileMemoryIndex] < Context->CurrentTotalUsageByType[ProfileMemoryIndex]) {
      Context->PeakTotalUsageByType[ProfileMemoryIndex] = Context->CurrentTotalUsageByType[ProfileMemoryIndex];
    }

    SmramProfileUpdateFreePages (ContextData);
  }

  return EFI_SUCCESS;
}

/**
  Get memory profile alloc info from memory profile

  @param DriverInfoData     Driver info
  @param BasicAction        This Free basic action
  @param Size               Buffer size
  @param Buffer             Buffer address

  @return Pointer to memory profile alloc info.
**/
MEMORY_PROFILE_ALLOC_INFO_DATA *
GetMemoryProfileAllocInfoFromAddress (
  IN MEMORY_PROFILE_DRIVER_INFO_DATA    *DriverInfoData,
  IN MEMORY_PROFILE_ACTION              BasicAction,
  IN UINTN                              Size,
  IN VOID                               *Buffer
  )
{
  LIST_ENTRY                        *AllocInfoList;
  LIST_ENTRY                        *AllocLink;
  MEMORY_PROFILE_ALLOC_INFO         *AllocInfo;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;

  AllocInfoList = DriverInfoData->AllocInfoList;

  for (AllocLink = AllocInfoList->ForwardLink;
       AllocLink != AllocInfoList;
       AllocLink = AllocLink->ForwardLink) {
    AllocInfoData = CR (
                      AllocLink,
                      MEMORY_PROFILE_ALLOC_INFO_DATA,
                      Link,
                      MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                      );
    AllocInfo = &AllocInfoData->AllocInfo;
    if ((AllocInfo->Action & MEMORY_PROFILE_ACTION_BASIC_MASK) != BasicAction) {
      continue;
    }
    switch (BasicAction) {
      case MemoryProfileActionAllocatePages:
        if ((AllocInfo->Buffer <= (PHYSICAL_ADDRESS) (UINTN) Buffer) &&
            ((AllocInfo->Buffer + AllocInfo->Size) >= ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size))) {
          return AllocInfoData;
        }
        break;
      case MemoryProfileActionAllocatePool:
        if (AllocInfo->Buffer == (PHYSICAL_ADDRESS) (UINTN) Buffer) {
          return AllocInfoData;
        }
        break;
      default:
        ASSERT (FALSE);
        break;
    }
  }

  return NULL;
}

/**
  Update SMRAM profile Free information.

  @param CallerAddress  Address of caller who call Free.
  @param Action         This Free action.
  @param Size           Buffer size.
  @param Buffer         Buffer address.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
SmmCoreUpdateProfileFree (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  MEMORY_PROFILE_CONTEXT           *Context;
  MEMORY_PROFILE_DRIVER_INFO       *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO        *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA      *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *DriverInfoData;
  LIST_ENTRY                       *DriverLink;
  LIST_ENTRY                       *DriverInfoList;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *ThisDriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA   *AllocInfoData;
  EFI_MEMORY_TYPE                  ProfileMemoryIndex;
  MEMORY_PROFILE_ACTION            BasicAction;
  BOOLEAN                          Found;

  BasicAction = Action & MEMORY_PROFILE_ACTION_BASIC_MASK;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);

  //
  // Do not return if DriverInfoData == NULL here,
  // because driver A might free memory allocated by driver B.
  //

  //
  // Need use do-while loop to find all possible record,
  // because one address might be recorded multiple times.
  //
  Found = FALSE;
  AllocInfoData = NULL;
  do {
    if (DriverInfoData != NULL) {
      switch (BasicAction) {
        case MemoryProfileActionFreePages:
          AllocInfoData = GetMemoryProfileAllocInfoFromAddress (DriverInfoData, MemoryProfileActionAllocatePages, Size, Buffer);
          break;
        case MemoryProfileActionFreePool:
          AllocInfoData = GetMemoryProfileAllocInfoFromAddress (DriverInfoData, MemoryProfileActionAllocatePool, 0, Buffer);
          break;
        default:
          ASSERT (FALSE);
          AllocInfoData = NULL;
          break;
      }
    }
    if (AllocInfoData == NULL) {
      //
      // Legal case, because driver A might free memory allocated by driver B, by some protocol.
      //
      DriverInfoList = ContextData->DriverInfoList;

      for (DriverLink = DriverInfoList->ForwardLink;
           DriverLink != DriverInfoList;
           DriverLink = DriverLink->ForwardLink) {
        ThisDriverInfoData = CR (
                               DriverLink,
                               MEMORY_PROFILE_DRIVER_INFO_DATA,
                               Link,
                               MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                               );
        switch (BasicAction) {
          case MemoryProfileActionFreePages:
            AllocInfoData = GetMemoryProfileAllocInfoFromAddress (ThisDriverInfoData, MemoryProfileActionAllocatePages, Size, Buffer);
            break;
          case MemoryProfileActionFreePool:
            AllocInfoData = GetMemoryProfileAllocInfoFromAddress (ThisDriverInfoData, MemoryProfileActionAllocatePool, 0, Buffer);
            break;
          default:
            ASSERT (FALSE);
            AllocInfoData = NULL;
            break;
        }
        if (AllocInfoData != NULL) {
          DriverInfoData = ThisDriverInfoData;
          break;
        }
      }

      if (AllocInfoData == NULL) {
        //
        // If (!Found), no matched allocate info is found for this free action.
        // It is because the specified memory type allocate actions have been filtered by
        // CoreNeedRecordProfile(), but free actions have no memory type information,
        // they can not be filtered by CoreNeedRecordProfile(). Then, they will be
        // filtered here.
        //
        // If (Found), it is normal exit path.
        return (Found ? EFI_SUCCESS : EFI_NOT_FOUND);
      }
    }

    ASSERT (DriverInfoData != NULL);
    ASSERT (AllocInfoData != NULL);

    Found = TRUE;

    Context = &ContextData->Context;
    DriverInfo = &DriverInfoData->DriverInfo;
    AllocInfo = &AllocInfoData->AllocInfo;

    DriverInfo->AllocRecordCount --;
    //
    // Update summary if and only if it is basic action.
    //
    if (AllocInfo->Action == (AllocInfo->Action & MEMORY_PROFILE_ACTION_BASIC_MASK)) {
      ProfileMemoryIndex = GetProfileMemoryIndex (AllocInfo->MemoryType);

      Context->CurrentTotalUsage -= AllocInfo->Size;
      Context->CurrentTotalUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;

      DriverInfo->CurrentUsage -= AllocInfo->Size;
      DriverInfo->CurrentUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;
    }

    RemoveEntryList (&AllocInfoData->Link);

    if (BasicAction == MemoryProfileActionFreePages) {
      if (AllocInfo->Buffer != (PHYSICAL_ADDRESS) (UINTN) Buffer) {
        SmmCoreUpdateProfileAllocate (
          AllocInfo->CallerAddress,
          AllocInfo->Action,
          AllocInfo->MemoryType,
          (UINTN) ((PHYSICAL_ADDRESS) (UINTN) Buffer - AllocInfo->Buffer),
          (VOID *) (UINTN) AllocInfo->Buffer,
          AllocInfoData->ActionString
          );
      }
      if (AllocInfo->Buffer + AllocInfo->Size != ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)) {
        SmmCoreUpdateProfileAllocate (
          AllocInfo->CallerAddress,
          AllocInfo->Action,
          AllocInfo->MemoryType,
          (UINTN) ((AllocInfo->Buffer + AllocInfo->Size) - ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)),
          (VOID *) ((UINTN) Buffer + Size),
          AllocInfoData->ActionString
          );
      }
    }

    //
    // Use SmmInternalFreePool() that will not update profile for this FreePool action.
    //
    SmmInternalFreePool (AllocInfoData);
  } while (TRUE);
}

/**
  Update SMRAM profile information.

  @param CallerAddress  Address of caller who call Allocate or Free.
  @param Action         This Allocate or Free action.
  @param MemoryType     Memory type.
                        EfiMaxMemoryType means the MemoryType is unknown.
  @param Size           Buffer size.
  @param Buffer         Buffer address.
  @param ActionString   String for memory profile action.
                        Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
EFIAPI
SmmCoreUpdateProfile (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType, // Valid for AllocatePages/AllocatePool
  IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
  IN VOID                   *Buffer,
  IN CHAR8                  *ActionString OPTIONAL
  )
{
  EFI_STATUS                    Status;
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  MEMORY_PROFILE_ACTION         BasicAction;

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return EFI_UNSUPPORTED;
  }

  if (mSmramProfileGettingStatus) {
    return EFI_ACCESS_DENIED;
  }

  if (!mSmramProfileRecordingEnable) {
    return EFI_ABORTED;
  }

  //
  // Get the basic action to know how to process the record
  //
  BasicAction = Action & MEMORY_PROFILE_ACTION_BASIC_MASK;

  //
  // Free operations have no memory type information, so skip the check.
  //
  if ((BasicAction == MemoryProfileActionAllocatePages) || (BasicAction == MemoryProfileActionAllocatePool)) {
    //
    // Only record limited MemoryType.
    //
    if (!SmmCoreNeedRecordProfile (MemoryType)) {
      return EFI_UNSUPPORTED;
    }
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  switch (BasicAction) {
    case MemoryProfileActionAllocatePages:
      Status = SmmCoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer, ActionString);
      break;
    case MemoryProfileActionFreePages:
      Status = SmmCoreUpdateProfileFree (CallerAddress, Action, Size, Buffer);
      break;
    case MemoryProfileActionAllocatePool:
      Status = SmmCoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer, ActionString);
      break;
    case MemoryProfileActionFreePool:
      Status = SmmCoreUpdateProfileFree (CallerAddress, Action, 0, Buffer);
      break;
    default:
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}

/**
  SMRAM profile ready to lock callback function.

**/
VOID
SmramProfileReadyToLock (
  VOID
  )
{
  if (!IS_SMRAM_PROFILE_ENABLED) {
    return;
  }

  DEBUG ((EFI_D_INFO, "SmramProfileReadyToLock\n"));
  mSmramReadyToLock = TRUE;
}

////////////////////

/**
  Get SMRAM profile data size.

  @return SMRAM profile data size.

**/
UINTN
SmramProfileGetDataSize (
  VOID
  )
{
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  LIST_ENTRY                        *DriverInfoList;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *AllocInfoList;
  LIST_ENTRY                        *AllocLink;
  UINTN                             TotalSize;
  LIST_ENTRY                        *Node;
  LIST_ENTRY                        *FreePageList;
  LIST_ENTRY                        *FreePoolList;
  FREE_POOL_HEADER                  *Pool;
  UINTN                             PoolListIndex;
  UINTN                             Index;
  UINTN                             SmmPoolTypeIndex;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return 0;
  }

  TotalSize = sizeof (MEMORY_PROFILE_CONTEXT);

  DriverInfoList = ContextData->DriverInfoList;
  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                   DriverLink,
                   MEMORY_PROFILE_DRIVER_INFO_DATA,
                   Link,
                   MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                   );
    TotalSize += DriverInfoData->DriverInfo.Header.Length;

    AllocInfoList = DriverInfoData->AllocInfoList;
    for (AllocLink = AllocInfoList->ForwardLink;
         AllocLink != AllocInfoList;
         AllocLink = AllocLink->ForwardLink) {
      AllocInfoData = CR (
                        AllocLink,
                        MEMORY_PROFILE_ALLOC_INFO_DATA,
                        Link,
                        MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                        );
      TotalSize += AllocInfoData->AllocInfo.Header.Length;
    }
  }


  Index = 0;
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink;
       Node != FreePageList;
       Node = Node->BackLink) {
    Index++;
  }
  for (SmmPoolTypeIndex = 0; SmmPoolTypeIndex < SmmPoolTypeMax; SmmPoolTypeIndex++) {
    for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
      FreePoolList = &mSmmPoolLists[SmmPoolTypeIndex][PoolListIndex];
      for (Node = FreePoolList->BackLink;
           Node != FreePoolList;
           Node = Node->BackLink) {
        Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
        if (Pool->Header.Available) {
          Index++;
        }
      }
    }
  }

  TotalSize += (sizeof (MEMORY_PROFILE_FREE_MEMORY) + Index * sizeof (MEMORY_PROFILE_DESCRIPTOR));
  TotalSize += (sizeof (MEMORY_PROFILE_MEMORY_RANGE) + mFullSmramRangeCount * sizeof (MEMORY_PROFILE_DESCRIPTOR));

  return TotalSize;
}

/**
  Copy SMRAM profile data.

  @param ProfileBuffer  The buffer to hold SMRAM profile data.
  @param ProfileSize    On input, profile buffer size.
                        On output, actual profile data size copied.
  @param ProfileOffset  On input, profile buffer offset to copy.
                        On output, next time profile buffer offset to copy.

**/
VOID
SmramProfileCopyData (
  OUT VOID      *ProfileBuffer,
  IN OUT UINT64 *ProfileSize,
  IN OUT UINT64 *ProfileOffset
  )
{
  MEMORY_PROFILE_CONTEXT           *Context;
  MEMORY_PROFILE_DRIVER_INFO       *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO        *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA      *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA   *AllocInfoData;
  LIST_ENTRY                      *DriverInfoList;
  LIST_ENTRY                      *DriverLink;
  LIST_ENTRY                      *AllocInfoList;
  LIST_ENTRY                      *AllocLink;
  LIST_ENTRY                      *Node;
  FREE_PAGE_LIST                  *Pages;
  LIST_ENTRY                      *FreePageList;
  LIST_ENTRY                      *FreePoolList;
  FREE_POOL_HEADER                *Pool;
  UINTN                           PoolListIndex;
  UINT32                          Index;
  MEMORY_PROFILE_FREE_MEMORY      *FreeMemory;
  MEMORY_PROFILE_MEMORY_RANGE     *MemoryRange;
  MEMORY_PROFILE_DESCRIPTOR       *MemoryProfileDescriptor;
  UINT64                          Offset;
  UINT64                          RemainingSize;
  UINTN                           PdbSize;
  UINTN                           ActionStringSize;
  UINTN                           SmmPoolTypeIndex;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  RemainingSize = *ProfileSize;
  Offset = 0;

  if (*ProfileOffset < sizeof (MEMORY_PROFILE_CONTEXT)) {
    if (RemainingSize >= sizeof (MEMORY_PROFILE_CONTEXT)) {
      Context = ProfileBuffer;
      CopyMem (Context, &ContextData->Context, sizeof (MEMORY_PROFILE_CONTEXT));
      RemainingSize -= sizeof (MEMORY_PROFILE_CONTEXT);
      ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_CONTEXT);
    } else {
      goto Done;
    }
  }
  Offset += sizeof (MEMORY_PROFILE_CONTEXT);

  DriverInfoList = ContextData->DriverInfoList;
  for (DriverLink = DriverInfoList->ForwardLink;
       DriverLink != DriverInfoList;
       DriverLink = DriverLink->ForwardLink) {
    DriverInfoData = CR (
                       DriverLink,
                       MEMORY_PROFILE_DRIVER_INFO_DATA,
                       Link,
                       MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                       );
    if (*ProfileOffset < (Offset + DriverInfoData->DriverInfo.Header.Length)) {
      if (RemainingSize >= DriverInfoData->DriverInfo.Header.Length) {
        DriverInfo = ProfileBuffer;
        CopyMem (DriverInfo, &DriverInfoData->DriverInfo, sizeof (MEMORY_PROFILE_DRIVER_INFO));
        if (DriverInfo->PdbStringOffset != 0) {
          PdbSize = AsciiStrSize (DriverInfoData->PdbString);
          CopyMem ((VOID *) ((UINTN) DriverInfo + DriverInfo->PdbStringOffset), DriverInfoData->PdbString, PdbSize);
        }
        RemainingSize -= DriverInfo->Header.Length;
        ProfileBuffer = (UINT8 *) ProfileBuffer + DriverInfo->Header.Length;
      } else {
        goto Done;
      }
    }
    Offset += DriverInfoData->DriverInfo.Header.Length;

    AllocInfoList = DriverInfoData->AllocInfoList;
    for (AllocLink = AllocInfoList->ForwardLink;
         AllocLink != AllocInfoList;
         AllocLink = AllocLink->ForwardLink) {
      AllocInfoData = CR (
                        AllocLink,
                        MEMORY_PROFILE_ALLOC_INFO_DATA,
                        Link,
                        MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                        );
      if (*ProfileOffset < (Offset + AllocInfoData->AllocInfo.Header.Length)) {
        if (RemainingSize >= AllocInfoData->AllocInfo.Header.Length) {
          AllocInfo = ProfileBuffer;
          CopyMem (AllocInfo, &AllocInfoData->AllocInfo, sizeof (MEMORY_PROFILE_ALLOC_INFO));
          if (AllocInfo->ActionStringOffset) {
            ActionStringSize = AsciiStrSize (AllocInfoData->ActionString);
            CopyMem ((VOID *) ((UINTN) AllocInfo + AllocInfo->ActionStringOffset), AllocInfoData->ActionString, ActionStringSize);
          }
          RemainingSize -= AllocInfo->Header.Length;
          ProfileBuffer = (UINT8 *) ProfileBuffer + AllocInfo->Header.Length;
        } else {
          goto Done;
        }
      }
      Offset += AllocInfoData->AllocInfo.Header.Length;
    }
  }


  if (*ProfileOffset < (Offset + sizeof (MEMORY_PROFILE_FREE_MEMORY))) {
    if (RemainingSize >= sizeof (MEMORY_PROFILE_FREE_MEMORY)) {
      FreeMemory = ProfileBuffer;
      CopyMem (FreeMemory, &mSmramFreeMemory, sizeof (MEMORY_PROFILE_FREE_MEMORY));
      Index = 0;
      FreePageList = &mSmmMemoryMap;
      for (Node = FreePageList->BackLink;
           Node != FreePageList;
           Node = Node->BackLink) {
        Index++;
      }
      for (SmmPoolTypeIndex = 0; SmmPoolTypeIndex < SmmPoolTypeMax; SmmPoolTypeIndex++) {
        for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
          FreePoolList = &mSmmPoolLists[SmmPoolTypeIndex][MAX_POOL_INDEX - PoolListIndex - 1];
          for (Node = FreePoolList->BackLink;
               Node != FreePoolList;
               Node = Node->BackLink) {
            Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
            if (Pool->Header.Available) {
              Index++;
            }
          }
        }
      }
      FreeMemory->FreeMemoryEntryCount = Index;

      RemainingSize -= sizeof (MEMORY_PROFILE_FREE_MEMORY);
      ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_FREE_MEMORY);
    } else {
      goto Done;
    }
  }
  Offset += sizeof (MEMORY_PROFILE_FREE_MEMORY);
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink;
       Node != FreePageList;
       Node = Node->BackLink) {
    if (*ProfileOffset < (Offset + sizeof (MEMORY_PROFILE_DESCRIPTOR))) {
      if (RemainingSize >= sizeof (MEMORY_PROFILE_DESCRIPTOR)) {
        Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
        MemoryProfileDescriptor = ProfileBuffer;
        MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
        MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
        MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
        MemoryProfileDescriptor->Address = (PHYSICAL_ADDRESS) (UINTN) Pages;
        MemoryProfileDescriptor->Size = EFI_PAGES_TO_SIZE (Pages->NumberOfPages);

        RemainingSize -= sizeof (MEMORY_PROFILE_DESCRIPTOR);
        ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_DESCRIPTOR);
      } else {
        goto Done;
      }
    }
    Offset += sizeof (MEMORY_PROFILE_DESCRIPTOR);
  }
  for (SmmPoolTypeIndex = 0; SmmPoolTypeIndex < SmmPoolTypeMax; SmmPoolTypeIndex++) {
    for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
      FreePoolList = &mSmmPoolLists[SmmPoolTypeIndex][MAX_POOL_INDEX - PoolListIndex - 1];
      for (Node = FreePoolList->BackLink;
           Node != FreePoolList;
           Node = Node->BackLink) {
        Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
        if (Pool->Header.Available) {
          if (*ProfileOffset < (Offset + sizeof (MEMORY_PROFILE_DESCRIPTOR))) {
            if (RemainingSize >= sizeof (MEMORY_PROFILE_DESCRIPTOR)) {
              MemoryProfileDescriptor = ProfileBuffer;
              MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
              MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
              MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
              MemoryProfileDescriptor->Address = (PHYSICAL_ADDRESS) (UINTN) Pool;
              MemoryProfileDescriptor->Size = Pool->Header.Size;

              RemainingSize -= sizeof (MEMORY_PROFILE_DESCRIPTOR);
              ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_DESCRIPTOR);
            } else {
              goto Done;
            }
          }
          Offset += sizeof (MEMORY_PROFILE_DESCRIPTOR);
        }
      }
    }
  }

  if (*ProfileOffset < (Offset + sizeof (MEMORY_PROFILE_MEMORY_RANGE))) {
    if (RemainingSize >= sizeof (MEMORY_PROFILE_MEMORY_RANGE)) {
      MemoryRange = ProfileBuffer;
      MemoryRange->Header.Signature = MEMORY_PROFILE_MEMORY_RANGE_SIGNATURE;
      MemoryRange->Header.Length = sizeof (MEMORY_PROFILE_MEMORY_RANGE);
      MemoryRange->Header.Revision = MEMORY_PROFILE_MEMORY_RANGE_REVISION;
      MemoryRange->MemoryRangeCount = (UINT32) mFullSmramRangeCount;

      RemainingSize -= sizeof (MEMORY_PROFILE_MEMORY_RANGE);
      ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_MEMORY_RANGE);
    } else {
      goto Done;
    }
  }
  Offset += sizeof (MEMORY_PROFILE_MEMORY_RANGE);
  for (Index = 0; Index < mFullSmramRangeCount; Index++) {
    if (*ProfileOffset < (Offset + sizeof (MEMORY_PROFILE_DESCRIPTOR))) {
      if (RemainingSize >= sizeof (MEMORY_PROFILE_DESCRIPTOR)) {
        MemoryProfileDescriptor = ProfileBuffer;
        MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
        MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
        MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
        MemoryProfileDescriptor->Address = mFullSmramRanges[Index].PhysicalStart;
        MemoryProfileDescriptor->Size = mFullSmramRanges[Index].PhysicalSize;

        RemainingSize -= sizeof (MEMORY_PROFILE_DESCRIPTOR);
        ProfileBuffer = (UINT8 *) ProfileBuffer + sizeof (MEMORY_PROFILE_DESCRIPTOR);
      } else {
        goto Done;
      }
    }
    Offset += sizeof (MEMORY_PROFILE_DESCRIPTOR);
  }

Done:
  //
  // On output, actual profile data size copied.
  //
  *ProfileSize -= RemainingSize;
  //
  // On output, next time profile buffer offset to copy.
  //
  *ProfileOffset = Offset;
}

/**
  Get memory profile data.

  @param[in]      This              The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in, out] ProfileSize       On entry, points to the size in bytes of the ProfileBuffer.
                                    On return, points to the size of the data returned in ProfileBuffer.
  @param[out]     ProfileBuffer     Profile buffer.

  @return EFI_SUCCESS               Get the memory profile data successfully.
  @return EFI_UNSUPPORTED           Memory profile is unsupported.
  @return EFI_BUFFER_TO_SMALL       The ProfileSize is too small for the resulting data.
                                    ProfileSize is updated with the size required.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolGetData (
  IN     EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN OUT UINT64                             *ProfileSize,
     OUT VOID                               *ProfileBuffer
  )
{
  UINT64                                Size;
  UINT64                                Offset;
  MEMORY_PROFILE_CONTEXT_DATA           *ContextData;
  BOOLEAN                               SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  Size = SmramProfileGetDataSize ();

  if (*ProfileSize < Size) {
    *ProfileSize = Size;
    mSmramProfileGettingStatus = SmramProfileGettingStatus;
    return EFI_BUFFER_TOO_SMALL;
  }

  Offset = 0;
  SmramProfileCopyData (ProfileBuffer, &Size, &Offset);
  *ProfileSize = Size;

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
  return EFI_SUCCESS;
}

/**
  Register image to memory profile.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.
  @param[in] FileType           File type of the image.

  @return EFI_SUCCESS           Register successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCES  No enough resource for this register.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolRegisterImage (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize,
  IN EFI_FV_FILETYPE                    FileType
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_DRIVER_ENTRY              DriverEntry;
  VOID                              *EntryPointInImage;
  EFI_GUID                          *Name;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  Name = GetFileNameFromFilePath (FilePath);
  if (Name != NULL) {
    CopyMem (&DriverEntry.FileName, Name, sizeof (EFI_GUID));
  }
  DriverEntry.ImageBuffer = ImageBase;
  DriverEntry.NumberOfPage = EFI_SIZE_TO_PAGES ((UINTN) ImageSize);
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  return RegisterSmramProfileImage (&DriverEntry, FALSE);
}

/**
  Unregister image from memory profile.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] FilePath           File path of the image.
  @param[in] ImageBase          Image base address.
  @param[in] ImageSize          Image size.

  @return EFI_SUCCESS           Unregister successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolUnregisterImage (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL           *FilePath,
  IN PHYSICAL_ADDRESS                   ImageBase,
  IN UINT64                             ImageSize
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_DRIVER_ENTRY              DriverEntry;
  VOID                              *EntryPointInImage;
  EFI_GUID                          *Name;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  Name = GetFileNameFromFilePath (FilePath);
  if (Name != NULL) {
    CopyMem (&DriverEntry.FileName, Name, sizeof (EFI_GUID));
  }
  DriverEntry.ImageBuffer = ImageBase;
  DriverEntry.NumberOfPage = EFI_SIZE_TO_PAGES ((UINTN) ImageSize);
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  return UnregisterSmramProfileImage (&DriverEntry, FALSE);
}

/**
  Get memory profile recording state.

  @param[in]  This              The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[out] RecordingState    Recording state.

  @return EFI_SUCCESS           Memory profile recording state is returned.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.
  @return EFI_INVALID_PARAMETER RecordingState is NULL.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolGetRecordingState (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  OUT BOOLEAN                           *RecordingState
  )
{
  MEMORY_PROFILE_CONTEXT_DATA           *ContextData;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (RecordingState == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *RecordingState = mSmramProfileRecordingEnable;
  return EFI_SUCCESS;
}

/**
  Set memory profile recording state.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] RecordingState     Recording state.

  @return EFI_SUCCESS           Set memory profile recording state successfully.
  @return EFI_UNSUPPORTED       Memory profile is unsupported.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolSetRecordingState (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN BOOLEAN                            RecordingState
  )
{
  MEMORY_PROFILE_CONTEXT_DATA           *ContextData;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return EFI_UNSUPPORTED;
  }

  mSmramProfileRecordingEnable = RecordingState;
  return EFI_SUCCESS;
}

/**
  Record memory profile of multilevel caller.

  @param[in] This               The EDKII_SMM_MEMORY_PROFILE_PROTOCOL instance.
  @param[in] CallerAddress      Address of caller.
  @param[in] Action             Memory profile action.
  @param[in] MemoryType         Memory type.
                                EfiMaxMemoryType means the MemoryType is unknown.
  @param[in] Buffer             Buffer address.
  @param[in] Size               Buffer size.
  @param[in] ActionString       String for memory profile action.
                                Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
EFIAPI
SmramProfileProtocolRecord (
  IN EDKII_SMM_MEMORY_PROFILE_PROTOCOL  *This,
  IN PHYSICAL_ADDRESS                   CallerAddress,
  IN MEMORY_PROFILE_ACTION              Action,
  IN EFI_MEMORY_TYPE                    MemoryType,
  IN VOID                               *Buffer,
  IN UINTN                              Size,
  IN CHAR8                              *ActionString OPTIONAL
  )
{
  return SmmCoreUpdateProfile (CallerAddress, Action, MemoryType, Size, Buffer, ActionString);
}

/**
  SMRAM profile handler to get profile info.

  @param SmramProfileParameterGetInfo The parameter of SMM profile get size.

**/
VOID
SmramProfileHandlerGetInfo (
  IN SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO   *SmramProfileParameterGetInfo
  )
{
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  BOOLEAN                       SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  SmramProfileParameterGetInfo->ProfileSize = SmramProfileGetDataSize();
  SmramProfileParameterGetInfo->Header.ReturnStatus = 0;

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  SMRAM profile handler to get profile data.

  @param SmramProfileParameterGetData The parameter of SMM profile get data.

**/
VOID
SmramProfileHandlerGetData (
  IN SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA   *SmramProfileParameterGetData
  )
{
  UINT64                                    ProfileSize;
  UINT64                                    ProfileOffset;
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA  SmramProfileGetData;
  MEMORY_PROFILE_CONTEXT_DATA               *ContextData;
  BOOLEAN                                   SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;


  CopyMem (&SmramProfileGetData, SmramProfileParameterGetData, sizeof (SmramProfileGetData));

  ProfileSize = SmramProfileGetDataSize();

  //
  // Sanity check
  //
  if (!SmmIsBufferOutsideSmmValid ((UINTN) SmramProfileGetData.ProfileBuffer, (UINTN) ProfileSize)) {
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetData: SMM ProfileBuffer in SMRAM or overflow!\n"));
    SmramProfileParameterGetData->ProfileSize = ProfileSize;
    SmramProfileParameterGetData->Header.ReturnStatus = (UINT64) (INT64) (INTN) EFI_ACCESS_DENIED;
    goto Done;
  }

  if (SmramProfileGetData.ProfileSize < ProfileSize) {
    SmramProfileParameterGetData->ProfileSize = ProfileSize;
    SmramProfileParameterGetData->Header.ReturnStatus = (UINT64) (INT64) (INTN) EFI_BUFFER_TOO_SMALL;
    goto Done;
  }

  ProfileOffset = 0;
  SmramProfileCopyData ((VOID *) (UINTN) SmramProfileGetData.ProfileBuffer, &ProfileSize, &ProfileOffset);
  SmramProfileParameterGetData->ProfileSize = ProfileSize;
  SmramProfileParameterGetData->Header.ReturnStatus = 0;

Done:
  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  SMRAM profile handler to get profile data by offset.

  @param SmramProfileParameterGetDataByOffset   The parameter of SMM profile get data by offset.

**/
VOID
SmramProfileHandlerGetDataByOffset (
  IN SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET     *SmramProfileParameterGetDataByOffset
  )
{
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET    SmramProfileGetDataByOffset;
  MEMORY_PROFILE_CONTEXT_DATA                           *ContextData;
  BOOLEAN                                               SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;


  CopyMem (&SmramProfileGetDataByOffset, SmramProfileParameterGetDataByOffset, sizeof (SmramProfileGetDataByOffset));

  //
  // Sanity check
  //
  if (!SmmIsBufferOutsideSmmValid ((UINTN) SmramProfileGetDataByOffset.ProfileBuffer, (UINTN) SmramProfileGetDataByOffset.ProfileSize)) {
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetDataByOffset: SMM ProfileBuffer in SMRAM or overflow!\n"));
    SmramProfileParameterGetDataByOffset->Header.ReturnStatus = (UINT64) (INT64) (INTN) EFI_ACCESS_DENIED;
    goto Done;
  }

  SmramProfileCopyData ((VOID *) (UINTN) SmramProfileGetDataByOffset.ProfileBuffer, &SmramProfileGetDataByOffset.ProfileSize, &SmramProfileGetDataByOffset.ProfileOffset);
  CopyMem (SmramProfileParameterGetDataByOffset, &SmramProfileGetDataByOffset, sizeof (SmramProfileGetDataByOffset));
  SmramProfileParameterGetDataByOffset->Header.ReturnStatus = 0;

Done:
  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  SMRAM profile handler to register SMM image.

  @param SmramProfileParameterRegisterImage The parameter of SMM profile register image.

**/
VOID
SmramProfileHandlerRegisterImage (
  IN SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE *SmramProfileParameterRegisterImage
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_DRIVER_ENTRY              DriverEntry;
  VOID                              *EntryPointInImage;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  CopyMem (&DriverEntry.FileName, &SmramProfileParameterRegisterImage->FileName, sizeof(EFI_GUID));
  DriverEntry.ImageBuffer = SmramProfileParameterRegisterImage->ImageBuffer;
  DriverEntry.NumberOfPage = (UINTN) SmramProfileParameterRegisterImage->NumberOfPage;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  Status = RegisterSmramProfileImage (&DriverEntry, FALSE);
  if (!EFI_ERROR (Status)) {
    SmramProfileParameterRegisterImage->Header.ReturnStatus = 0;
  }
}

/**
  SMRAM profile handler to unregister SMM image.

  @param SmramProfileParameterUnregisterImage The parameter of SMM profile unregister image.

**/
VOID
SmramProfileHandlerUnregisterImage (
  IN SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE *SmramProfileParameterUnregisterImage
  )
{
  EFI_STATUS                        Status;
  EFI_SMM_DRIVER_ENTRY              DriverEntry;
  VOID                              *EntryPointInImage;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  CopyMem (&DriverEntry.FileName, &SmramProfileParameterUnregisterImage->FileName, sizeof (EFI_GUID));
  DriverEntry.ImageBuffer = SmramProfileParameterUnregisterImage->ImageBuffer;
  DriverEntry.NumberOfPage = (UINTN) SmramProfileParameterUnregisterImage->NumberOfPage;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  Status = UnregisterSmramProfileImage (&DriverEntry, FALSE);
  if (!EFI_ERROR (Status)) {
    SmramProfileParameterUnregisterImage->Header.ReturnStatus = 0;
  }
}

/**
  Dispatch function for a Software SMI handler.

  Caution: This function may receive untrusted input.
  Communicate buffer and buffer size are external input, so this function will do basic validation.

  @param DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param Context         Points to an optional handler context which was specified when the
                         handler was registered.
  @param CommBuffer      A pointer to a collection of data in memory that will
                         be conveyed from a non-SMM environment into an SMM environment.
  @param CommBufferSize  The size of the CommBuffer.

  @retval EFI_SUCCESS Command is handled successfully.

**/
EFI_STATUS
EFIAPI
SmramProfileHandler (
  IN EFI_HANDLE  DispatchHandle,
  IN CONST VOID  *Context         OPTIONAL,
  IN OUT VOID    *CommBuffer      OPTIONAL,
  IN OUT UINTN   *CommBufferSize  OPTIONAL
  )
{
  SMRAM_PROFILE_PARAMETER_HEADER           *SmramProfileParameterHeader;
  UINTN                                    TempCommBufferSize;
  SMRAM_PROFILE_PARAMETER_RECORDING_STATE  *ParameterRecordingState;

  DEBUG ((EFI_D_ERROR, "SmramProfileHandler Enter\n"));

  //
  // If input is invalid, stop processing this SMI
  //
  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;

  if (TempCommBufferSize < sizeof (SMRAM_PROFILE_PARAMETER_HEADER)) {
    DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
    return EFI_SUCCESS;
  }

  if (mSmramReadyToLock && !SmmIsBufferOutsideSmmValid ((UINTN)CommBuffer, TempCommBufferSize)) {
    DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer in SMRAM or overflow!\n"));
    return EFI_SUCCESS;
  }

  SmramProfileParameterHeader = (SMRAM_PROFILE_PARAMETER_HEADER *) ((UINTN) CommBuffer);

  SmramProfileParameterHeader->ReturnStatus = (UINT64)-1;

  if (GetSmramProfileContext () == NULL) {
    SmramProfileParameterHeader->ReturnStatus = (UINT64) (INT64) (INTN) EFI_UNSUPPORTED;
    return EFI_SUCCESS;
  }

  switch (SmramProfileParameterHeader->Command) {
  case SMRAM_PROFILE_COMMAND_GET_PROFILE_INFO:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetInfo\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    SmramProfileHandlerGetInfo ((SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO *) (UINTN) CommBuffer);
    break;
  case SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetData\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    SmramProfileHandlerGetData ((SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA *) (UINTN) CommBuffer);
    break;
  case SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA_BY_OFFSET:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetDataByOffset\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    SmramProfileHandlerGetDataByOffset ((SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET *) (UINTN) CommBuffer);
    break;
  case SMRAM_PROFILE_COMMAND_REGISTER_IMAGE:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerRegisterImage\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    if (mSmramReadyToLock) {
      return EFI_SUCCESS;
    }
    SmramProfileHandlerRegisterImage ((SMRAM_PROFILE_PARAMETER_REGISTER_IMAGE *) (UINTN) CommBuffer);
    break;
  case SMRAM_PROFILE_COMMAND_UNREGISTER_IMAGE:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerUnregisterImage\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    if (mSmramReadyToLock) {
      return EFI_SUCCESS;
    }
    SmramProfileHandlerUnregisterImage ((SMRAM_PROFILE_PARAMETER_UNREGISTER_IMAGE *) (UINTN) CommBuffer);
    break;
  case SMRAM_PROFILE_COMMAND_GET_RECORDING_STATE:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerGetRecordingState\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    ParameterRecordingState = (SMRAM_PROFILE_PARAMETER_RECORDING_STATE *) (UINTN) CommBuffer;
    ParameterRecordingState->RecordingState = mSmramProfileRecordingEnable;
    ParameterRecordingState->Header.ReturnStatus = 0;
    break;
  case SMRAM_PROFILE_COMMAND_SET_RECORDING_STATE:
    DEBUG ((EFI_D_ERROR, "SmramProfileHandlerSetRecordingState\n"));
    if (TempCommBufferSize != sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE)) {
      DEBUG ((EFI_D_ERROR, "SmramProfileHandler: SMM communication buffer size invalid!\n"));
      return EFI_SUCCESS;
    }
    ParameterRecordingState = (SMRAM_PROFILE_PARAMETER_RECORDING_STATE *) (UINTN) CommBuffer;
    mSmramProfileRecordingEnable = ParameterRecordingState->RecordingState;
    ParameterRecordingState->Header.ReturnStatus = 0;
    break;

  default:
    break;
  }

  DEBUG ((EFI_D_ERROR, "SmramProfileHandler Exit\n"));

  return EFI_SUCCESS;
}

/**
  Register SMRAM profile handler.

**/
VOID
RegisterSmramProfileHandler (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_HANDLE    DispatchHandle;

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return;
  }

  Status = SmiHandlerRegister (
             SmramProfileHandler,
             &gEdkiiMemoryProfileGuid,
             &DispatchHandle
             );
  ASSERT_EFI_ERROR (Status);
}

////////////////////

/**
  Dump SMRAM range.

**/
VOID
DumpSmramRange (
  VOID
  )
{
  UINTN                         Index;
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  BOOLEAN                       SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  DEBUG ((EFI_D_INFO, "FullSmramRange address - 0x%08x\n", mFullSmramRanges));

  DEBUG ((EFI_D_INFO, "======= SmramProfile begin =======\n"));

  DEBUG ((EFI_D_INFO, "FullSmramRange:\n"));
  for (Index = 0; Index < mFullSmramRangeCount; Index++) {
    DEBUG ((EFI_D_INFO, "  FullSmramRange (0x%x)\n", Index));
    DEBUG ((EFI_D_INFO, "    PhysicalStart      - 0x%016lx\n", mFullSmramRanges[Index].PhysicalStart));
    DEBUG ((EFI_D_INFO, "    CpuStart           - 0x%016lx\n", mFullSmramRanges[Index].CpuStart));
    DEBUG ((EFI_D_INFO, "    PhysicalSize       - 0x%016lx\n", mFullSmramRanges[Index].PhysicalSize));
    DEBUG ((EFI_D_INFO, "    RegionState        - 0x%016lx\n", mFullSmramRanges[Index].RegionState));
  }

  DEBUG ((EFI_D_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  Dump SMRAM free page list.

**/
VOID
DumpFreePagesList (
  VOID
  )
{
  LIST_ENTRY                    *FreePageList;
  LIST_ENTRY                    *Node;
  FREE_PAGE_LIST                *Pages;
  UINTN                         Index;
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  BOOLEAN                       SmramProfileGettingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  DEBUG ((EFI_D_INFO, "======= SmramProfile begin =======\n"));

  DEBUG ((EFI_D_INFO, "FreePagesList:\n"));
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink, Index = 0;
       Node != FreePageList;
       Node = Node->BackLink, Index++) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    DEBUG ((EFI_D_INFO, "  Index - 0x%x\n", Index));
    DEBUG ((EFI_D_INFO, "    PhysicalStart - 0x%016lx\n", (PHYSICAL_ADDRESS) (UINTN) Pages));
    DEBUG ((EFI_D_INFO, "    NumberOfPages - 0x%08x\n", Pages->NumberOfPages));
  }

  DEBUG ((EFI_D_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  Dump SMRAM free pool list.

**/
VOID
DumpFreePoolList (
  VOID
  )
{
  LIST_ENTRY                    *FreePoolList;
  LIST_ENTRY                    *Node;
  FREE_POOL_HEADER              *Pool;
  UINTN                         Index;
  UINTN                         PoolListIndex;
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  BOOLEAN                       SmramProfileGettingStatus;
  UINTN                         SmmPoolTypeIndex;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  DEBUG ((DEBUG_INFO, "======= SmramProfile begin =======\n"));

  for (SmmPoolTypeIndex = 0; SmmPoolTypeIndex < SmmPoolTypeMax; SmmPoolTypeIndex++) {
    for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
      DEBUG ((DEBUG_INFO, "FreePoolList(%d)(%d):\n", SmmPoolTypeIndex, PoolListIndex));
      FreePoolList = &mSmmPoolLists[SmmPoolTypeIndex][PoolListIndex];
      for (Node = FreePoolList->BackLink, Index = 0;
           Node != FreePoolList;
           Node = Node->BackLink, Index++) {
        Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
        DEBUG ((DEBUG_INFO, "  Index - 0x%x\n", Index));
        DEBUG ((DEBUG_INFO, "    PhysicalStart - 0x%016lx\n", (PHYSICAL_ADDRESS) (UINTN) Pool));
        DEBUG ((DEBUG_INFO, "    Size          - 0x%08x\n", Pool->Header.Size));
        DEBUG ((DEBUG_INFO, "    Available     - 0x%02x\n", Pool->Header.Available));
      }
    }
  }

  DEBUG ((DEBUG_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

GLOBAL_REMOVE_IF_UNREFERENCED CHAR8 *mSmmActionString[] = {
  "SmmUnknown",
  "gSmst->SmmAllocatePages",
  "gSmst->SmmFreePages",
  "gSmst->SmmAllocatePool",
  "gSmst->SmmFreePool",
};

typedef struct {
  MEMORY_PROFILE_ACTION  Action;
  CHAR8                 *String;
} ACTION_STRING;

GLOBAL_REMOVE_IF_UNREFERENCED ACTION_STRING mExtActionString[] = {
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_PAGES,                    "Lib:AllocatePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_PAGES,            "Lib:AllocateRuntimePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_PAGES,           "Lib:AllocateReservedPages"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_PAGES,                        "Lib:FreePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_PAGES,            "Lib:AllocateAlignedPages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RUNTIME_PAGES,    "Lib:AllocateAlignedRuntimePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RESERVED_PAGES,   "Lib:AllocateAlignedReservedPages"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_ALIGNED_PAGES,                "Lib:FreeAlignedPages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_POOL,                     "Lib:AllocatePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_POOL,             "Lib:AllocateRuntimePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_POOL,            "Lib:AllocateReservedPool"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_POOL,                         "Lib:FreePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ZERO_POOL,                "Lib:AllocateZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_ZERO_POOL,        "Lib:AllocateRuntimeZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_ZERO_POOL,       "Lib:AllocateReservedZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_COPY_POOL,                "Lib:AllocateCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_COPY_POOL,        "Lib:AllocateRuntimeCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_COPY_POOL,       "Lib:AllocateReservedCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_POOL,                   "Lib:ReallocatePool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RUNTIME_POOL,           "Lib:ReallocateRuntimePool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RESERVED_POOL,          "Lib:ReallocateReservedPool"},
};

typedef struct {
  EFI_MEMORY_TYPE   MemoryType;
  CHAR8             *MemoryTypeStr;
} PROFILE_MEMORY_TYPE_STRING;

GLOBAL_REMOVE_IF_UNREFERENCED PROFILE_MEMORY_TYPE_STRING mMemoryTypeString[] = {
  {EfiRuntimeServicesCode, "EfiRuntimeServicesCode"},
  {EfiRuntimeServicesData, "EfiRuntimeServicesData"}
};

/**
  Memory type to string.

  @param[in] MemoryType Memory type.

  @return Pointer to string.

**/
CHAR8 *
ProfileMemoryTypeToStr (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  UINTN     Index;
  for (Index = 0; Index < ARRAY_SIZE (mMemoryTypeString); Index++) {
    if (mMemoryTypeString[Index].MemoryType == MemoryType) {
      return mMemoryTypeString[Index].MemoryTypeStr;
    }
  }

  return "UnexpectedMemoryType";
}

/**
  Action to string.

  @param[in] Action                     Profile action.

  @return Pointer to string.

**/
CHAR8 *
ProfileActionToStr (
  IN MEMORY_PROFILE_ACTION  Action
  )
{
  UINTN     Index;
  UINTN     ActionStringCount;
  CHAR8     **ActionString;

  ActionString = mSmmActionString;
  ActionStringCount = ARRAY_SIZE (mSmmActionString);

  if ((UINTN) (UINT32) Action < ActionStringCount) {
    return ActionString[Action];
  }
  for (Index = 0; Index < ARRAY_SIZE (mExtActionString); Index++) {
    if (mExtActionString[Index].Action == Action) {
      return mExtActionString[Index].String;
    }
  }

  return ActionString[0];
}

/**
  Dump SMRAM profile.

**/
VOID
DumpSmramProfile (
  VOID
  )
{
  MEMORY_PROFILE_CONTEXT            *Context;
  MEMORY_PROFILE_DRIVER_INFO        *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO         *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  LIST_ENTRY                        *SmramDriverInfoList;
  UINTN                             DriverIndex;
  LIST_ENTRY                        *DriverLink;
  LIST_ENTRY                        *AllocInfoList;
  UINTN                             AllocIndex;
  LIST_ENTRY                        *AllocLink;
  BOOLEAN                           SmramProfileGettingStatus;
  UINTN                             TypeIndex;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileGettingStatus = mSmramProfileGettingStatus;
  mSmramProfileGettingStatus = TRUE;

  Context = &ContextData->Context;
  DEBUG ((EFI_D_INFO, "======= SmramProfile begin =======\n"));
  DEBUG ((EFI_D_INFO, "MEMORY_PROFILE_CONTEXT\n"));

  DEBUG ((EFI_D_INFO, "  CurrentTotalUsage     - 0x%016lx\n", Context->CurrentTotalUsage));
  DEBUG ((EFI_D_INFO, "  PeakTotalUsage        - 0x%016lx\n", Context->PeakTotalUsage));
  for (TypeIndex = 0; TypeIndex < sizeof (Context->CurrentTotalUsageByType) / sizeof (Context->CurrentTotalUsageByType[0]); TypeIndex++) {
    if ((Context->CurrentTotalUsageByType[TypeIndex] != 0) ||
        (Context->PeakTotalUsageByType[TypeIndex] != 0)) {
      DEBUG ((EFI_D_INFO, "  CurrentTotalUsage[0x%02x]  - 0x%016lx (%a)\n", TypeIndex, Context->CurrentTotalUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
      DEBUG ((EFI_D_INFO, "  PeakTotalUsage[0x%02x]     - 0x%016lx (%a)\n", TypeIndex, Context->PeakTotalUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
    }
  }
  DEBUG ((EFI_D_INFO, "  TotalImageSize        - 0x%016lx\n", Context->TotalImageSize));
  DEBUG ((EFI_D_INFO, "  ImageCount            - 0x%08x\n", Context->ImageCount));
  DEBUG ((EFI_D_INFO, "  SequenceCount         - 0x%08x\n", Context->SequenceCount));

  SmramDriverInfoList = ContextData->DriverInfoList;
  for (DriverLink = SmramDriverInfoList->ForwardLink, DriverIndex = 0;
       DriverLink != SmramDriverInfoList;
       DriverLink = DriverLink->ForwardLink, DriverIndex++) {
    DriverInfoData = CR (
                   DriverLink,
                   MEMORY_PROFILE_DRIVER_INFO_DATA,
                   Link,
                   MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                   );
    DriverInfo = &DriverInfoData->DriverInfo;
    DEBUG ((EFI_D_INFO, "  MEMORY_PROFILE_DRIVER_INFO (0x%x)\n", DriverIndex));
    DEBUG ((EFI_D_INFO, "    FileName            - %g\n", &DriverInfo->FileName));
    DEBUG ((EFI_D_INFO, "    ImageBase           - 0x%016lx\n", DriverInfo->ImageBase));
    DEBUG ((EFI_D_INFO, "    ImageSize           - 0x%016lx\n", DriverInfo->ImageSize));
    DEBUG ((EFI_D_INFO, "    EntryPoint          - 0x%016lx\n", DriverInfo->EntryPoint));
    DEBUG ((EFI_D_INFO, "    ImageSubsystem      - 0x%04x\n", DriverInfo->ImageSubsystem));
    DEBUG ((EFI_D_INFO, "    FileType            - 0x%02x\n", DriverInfo->FileType));
    DEBUG ((EFI_D_INFO, "    CurrentUsage        - 0x%016lx\n", DriverInfo->CurrentUsage));
    DEBUG ((EFI_D_INFO, "    PeakUsage           - 0x%016lx\n", DriverInfo->PeakUsage));
    for (TypeIndex = 0; TypeIndex < sizeof (DriverInfo->CurrentUsageByType) / sizeof (DriverInfo->CurrentUsageByType[0]); TypeIndex++) {
      if ((DriverInfo->CurrentUsageByType[TypeIndex] != 0) ||
          (DriverInfo->PeakUsageByType[TypeIndex] != 0)) {
        DEBUG ((EFI_D_INFO, "    CurrentUsage[0x%02x]     - 0x%016lx (%a)\n", TypeIndex, DriverInfo->CurrentUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
        DEBUG ((EFI_D_INFO, "    PeakUsage[0x%02x]        - 0x%016lx (%a)\n", TypeIndex, DriverInfo->PeakUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
      }
    }
    DEBUG ((EFI_D_INFO, "    AllocRecordCount    - 0x%08x\n", DriverInfo->AllocRecordCount));

    AllocInfoList = DriverInfoData->AllocInfoList;
    for (AllocLink = AllocInfoList->ForwardLink, AllocIndex = 0;
         AllocLink != AllocInfoList;
         AllocLink = AllocLink->ForwardLink, AllocIndex++) {
      AllocInfoData = CR (
                     AllocLink,
                     MEMORY_PROFILE_ALLOC_INFO_DATA,
                     Link,
                     MEMORY_PROFILE_ALLOC_INFO_SIGNATURE
                     );
      AllocInfo = &AllocInfoData->AllocInfo;
      DEBUG ((EFI_D_INFO, "    MEMORY_PROFILE_ALLOC_INFO (0x%x)\n", AllocIndex));
      DEBUG ((EFI_D_INFO, "      CallerAddress  - 0x%016lx (Offset: 0x%08x)\n", AllocInfo->CallerAddress, AllocInfo->CallerAddress - DriverInfo->ImageBase));
      DEBUG ((EFI_D_INFO, "      SequenceId     - 0x%08x\n", AllocInfo->SequenceId));
      if ((AllocInfo->Action & MEMORY_PROFILE_ACTION_USER_DEFINED_MASK) != 0) {
        if (AllocInfoData->ActionString != NULL) {
          DEBUG ((EFI_D_INFO, "      Action         - 0x%08x (%a)\n", AllocInfo->Action, AllocInfoData->ActionString));
        } else {
          DEBUG ((EFI_D_INFO, "      Action         - 0x%08x (UserDefined-0x%08x)\n", AllocInfo->Action, AllocInfo->Action));
        }
      } else {
        DEBUG ((EFI_D_INFO, "      Action         - 0x%08x (%a)\n", AllocInfo->Action, ProfileActionToStr (AllocInfo->Action)));
      }
      DEBUG ((EFI_D_INFO, "      MemoryType     - 0x%08x (%a)\n", AllocInfo->MemoryType, ProfileMemoryTypeToStr (AllocInfo->MemoryType)));
      DEBUG ((EFI_D_INFO, "      Buffer         - 0x%016lx\n", AllocInfo->Buffer));
      DEBUG ((EFI_D_INFO, "      Size           - 0x%016lx\n", AllocInfo->Size));
    }
  }

  DEBUG ((EFI_D_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileGettingStatus = SmramProfileGettingStatus;
}

/**
  Dump SMRAM information.

**/
VOID
DumpSmramInfo (
  VOID
  )
{
  DEBUG_CODE (
    if (IS_SMRAM_PROFILE_ENABLED) {
      DumpSmramProfile ();
      DumpFreePagesList ();
      DumpFreePoolList ();
      DumpSmramRange ();
    }
  );
}

