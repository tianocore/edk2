/** @file
  Support routines for SMRAM profile.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiSmmCore.h"

#define IS_SMRAM_PROFILE_ENABLED ((PcdGet8 (PcdMemoryProfilePropertyMask) & BIT1) != 0)

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_CONTEXT        Context;
  LIST_ENTRY                    *DriverInfoList;
} MEMORY_PROFILE_CONTEXT_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_DRIVER_INFO    DriverInfo;
  LIST_ENTRY                    *AllocInfoList;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_DRIVER_INFO_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_ALLOC_INFO     AllocInfo;
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

BOOLEAN mSmramReadyToLock;
BOOLEAN mSmramProfileRecordingStatus = FALSE;

/**
  Dump SMRAM infromation.

**/
VOID
DumpSmramInfo (
  VOID
  );

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
  Retrieves the magic value from the PE/COFF header.

  @param Hdr    The buffer in which to return the PE32, PE32+, or TE header.

  @return EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC - Image is PE32
  @return EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC - Image is PE32+

**/
UINT16
InternalPeCoffGetPeHeaderMagicValue (
  IN  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr
  )
{
  //
  // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value
  //       in the PE/COFF Header.  If the MachineType is Itanium(IA64) and the
  //       Magic value in the OptionalHeader is  EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
  //       then override the returned value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
  //
  if (Hdr.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    return EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  }
  //
  // Return the magic value from the PC/COFF Optional Header
  //
  return Hdr.Pe32->OptionalHeader.Magic;
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
    Magic = InternalPeCoffGetPeHeaderMagicValue (Hdr);
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

  //
  // Use SmmInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  Status = SmmInternalAllocatePool (
             EfiRuntimeServicesData,
             sizeof (*DriverInfoData) + sizeof (LIST_ENTRY),
             (VOID **) &DriverInfoData
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ZeroMem (DriverInfoData, sizeof (*DriverInfoData));

  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfoData->Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverInfo->Header.Length = sizeof (MEMORY_PROFILE_DRIVER_INFO);
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

  if (IS_SMRAM_PROFILE_ENABLED) {

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

  if (IS_SMRAM_PROFILE_ENABLED) {

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

  ASSERT (ContextData != NULL);

  RegisterImageToDxe (
    &gEfiCallerIdGuid,
    gSmmCorePrivate->PiSmmCoreImageBase,
    gSmmCorePrivate->PiSmmCoreImageSize,
    EFI_FV_FILETYPE_SMM_CORE
    );

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

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return;
  }

  SmramProfileContext = GetSmramProfileContext ();
  if (SmramProfileContext != NULL) {
    return;
  }

  mSmramProfileRecordingStatus = TRUE;
  mSmramProfileContextPtr = &mSmramProfileContext;

  RegisterSmmCore (&mSmramProfileContext);

  DEBUG ((EFI_D_INFO, "SmramProfileInit SmramProfileContext - 0x%x\n", &mSmramProfileContext));
}

/**
  Register SMM image to SMRAM profile.

  @param DriverEntry    SMM image info.
  @param RegisterToDxe  Register image to DXE.

  @retval TRUE          Register success.
  @retval FALSE         Register fail.

**/
BOOLEAN
RegisterSmramProfileImage (
  IN EFI_SMM_DRIVER_ENTRY   *DriverEntry,
  IN BOOLEAN                RegisterToDxe
  )
{
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return FALSE;
  }

  if (RegisterToDxe) {
    RegisterImageToDxe (
      &DriverEntry->FileName,
      DriverEntry->ImageBuffer,
      EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage),
      EFI_FV_FILETYPE_SMM
      );
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
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
    return FALSE;
  }

  return TRUE;
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
  Search dummy image from SMRAM profile.

  @param ContextData    Memory profile context.

  @return Pointer to memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO_DATA *
FindDummyImage (
  IN MEMORY_PROFILE_CONTEXT_DATA    *ContextData
  )
{
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
    if (CompareGuid (&gZeroGuid, &DriverInfoData->DriverInfo.FileName)) {
      return DriverInfoData;
    }
  }

  return BuildDriverInfo (ContextData, &gZeroGuid, 0, 0, 0, 0, 0);
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

  //
  // Should never come here.
  //
  return FindDummyImage (ContextData);
}

/**
  Unregister image from SMRAM profile.

  @param DriverEntry        SMM image info.
  @param UnregisterFromDxe  Unregister image from DXE.

  @retval TRUE              Unregister success.
  @retval FALSE             Unregister fail.

**/
BOOLEAN
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

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return FALSE;
  }

  if (UnregisterFromDxe) {
    UnregisterImageFromDxe (
      &DriverEntry->FileName,
      DriverEntry->ImageBuffer,
      EFI_PAGES_TO_SIZE (DriverEntry->NumberOfPage)
      );
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
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
    return FALSE;
  }

  ContextData->Context.TotalImageSize -= DriverInfoData->DriverInfo.ImageSize;

  DriverInfoData->DriverInfo.ImageBase = 0;
  DriverInfoData->DriverInfo.ImageSize = 0;

  if (DriverInfoData->DriverInfo.PeakUsage == 0) {
    ContextData->Context.ImageCount --;
    RemoveEntryList (&DriverInfoData->Link);
    //
    // Use SmmInternalFreePool() that will not update profile for this FreePool action.
    //
    SmmInternalFreePool (DriverInfoData);
  }

  return TRUE;
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

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
SmmCoreUpdateProfileAllocate (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType,
  IN UINTN                  Size,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS                        Status;
  MEMORY_PROFILE_CONTEXT           *Context;
  MEMORY_PROFILE_DRIVER_INFO       *DriverInfo;
  MEMORY_PROFILE_ALLOC_INFO        *AllocInfo;
  MEMORY_PROFILE_CONTEXT_DATA       *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA   *DriverInfoData;
  MEMORY_PROFILE_ALLOC_INFO_DATA    *AllocInfoData;
  EFI_MEMORY_TYPE                   ProfileMemoryIndex;

  AllocInfoData = NULL;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);
  ASSERT (DriverInfoData != NULL);

  //
  // Use SmmInternalAllocatePool() that will not update profile for this AllocatePool action.
  //
  Status = SmmInternalAllocatePool (
             EfiRuntimeServicesData,
             sizeof (*AllocInfoData),
             (VOID **) &AllocInfoData
             );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  AllocInfo = &AllocInfoData->AllocInfo;
  AllocInfoData->Signature      = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Signature   = MEMORY_PROFILE_ALLOC_INFO_SIGNATURE;
  AllocInfo->Header.Length      = sizeof (MEMORY_PROFILE_ALLOC_INFO);
  AllocInfo->Header.Revision    = MEMORY_PROFILE_ALLOC_INFO_REVISION;
  AllocInfo->CallerAddress      = CallerAddress;
  AllocInfo->SequenceId         = ContextData->Context.SequenceCount;
  AllocInfo->Action             = Action;
  AllocInfo->MemoryType         = MemoryType;
  AllocInfo->Buffer             = (PHYSICAL_ADDRESS) (UINTN) Buffer;
  AllocInfo->Size               = Size;

  InsertTailList (DriverInfoData->AllocInfoList, &AllocInfoData->Link);

  ProfileMemoryIndex = GetProfileMemoryIndex (MemoryType);

  DriverInfo = &DriverInfoData->DriverInfo;
  DriverInfo->CurrentUsage += Size;
  if (DriverInfo->PeakUsage < DriverInfo->CurrentUsage) {
    DriverInfo->PeakUsage = DriverInfo->CurrentUsage;
  }
  DriverInfo->CurrentUsageByType[ProfileMemoryIndex] += Size;
  if (DriverInfo->PeakUsageByType[ProfileMemoryIndex] < DriverInfo->CurrentUsageByType[ProfileMemoryIndex]) {
    DriverInfo->PeakUsageByType[ProfileMemoryIndex] = DriverInfo->CurrentUsageByType[ProfileMemoryIndex];
  }
  DriverInfo->AllocRecordCount ++;

  Context = &ContextData->Context;
  Context->CurrentTotalUsage += Size;
  if (Context->PeakTotalUsage < Context->CurrentTotalUsage) {
    Context->PeakTotalUsage = Context->CurrentTotalUsage;
  }
  Context->CurrentTotalUsageByType[ProfileMemoryIndex] += Size;
  if (Context->PeakTotalUsageByType[ProfileMemoryIndex] < Context->CurrentTotalUsageByType[ProfileMemoryIndex]) {
    Context->PeakTotalUsageByType[ProfileMemoryIndex] = Context->CurrentTotalUsageByType[ProfileMemoryIndex];
  }
  Context->SequenceCount ++;

  SmramProfileUpdateFreePages (ContextData);
  return TRUE;
}

/**
  Get memory profile alloc info from memory profile

  @param DriverInfoData     Driver info
  @param Action             This Free action
  @param Size               Buffer size
  @param Buffer             Buffer address

  @return Pointer to memory profile alloc info.
**/
MEMORY_PROFILE_ALLOC_INFO_DATA *
GetMemoryProfileAllocInfoFromAddress (
  IN MEMORY_PROFILE_DRIVER_INFO_DATA    *DriverInfoData,
  IN MEMORY_PROFILE_ACTION              Action,
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
    if (AllocInfo->Action != Action) {
      continue;
    }
    switch (Action) {
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

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
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

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  DriverInfoData = GetMemoryProfileDriverInfoFromAddress (ContextData, CallerAddress);
  ASSERT (DriverInfoData != NULL);

  switch (Action) {
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
      switch (Action) {
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
      // No matched allocate operation is found for this free operation.
      // It is because the specified memory type allocate operation has been
      // filtered by CoreNeedRecordProfile(), but free operations have no
      // memory type information, they can not be filtered by CoreNeedRecordProfile().
      // Then, they will be filtered here.
      //
      return FALSE;
    }
  }

  Context = &ContextData->Context;
  DriverInfo = &DriverInfoData->DriverInfo;
  AllocInfo = &AllocInfoData->AllocInfo;

  ProfileMemoryIndex = GetProfileMemoryIndex (AllocInfo->MemoryType);

  Context->CurrentTotalUsage -= AllocInfo->Size;
  Context->CurrentTotalUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;

  DriverInfo->CurrentUsage -= AllocInfo->Size;
  DriverInfo->CurrentUsageByType[ProfileMemoryIndex] -= AllocInfo->Size;
  DriverInfo->AllocRecordCount --;

  RemoveEntryList (&AllocInfoData->Link);

  if (Action == MemoryProfileActionFreePages) {
    if (AllocInfo->Buffer != (PHYSICAL_ADDRESS) (UINTN) Buffer) {
      SmmCoreUpdateProfileAllocate (
        AllocInfo->CallerAddress,
        MemoryProfileActionAllocatePages,
        AllocInfo->MemoryType,
        (UINTN) ((PHYSICAL_ADDRESS) (UINTN) Buffer - AllocInfo->Buffer),
        (VOID *) (UINTN) AllocInfo->Buffer
        );
    }
    if (AllocInfo->Buffer + AllocInfo->Size != ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)) {
      SmmCoreUpdateProfileAllocate (
        AllocInfo->CallerAddress,
        MemoryProfileActionAllocatePages,
        AllocInfo->MemoryType,
        (UINTN) ((AllocInfo->Buffer + AllocInfo->Size) - ((PHYSICAL_ADDRESS) (UINTN) Buffer + Size)),
        (VOID *) ((UINTN) Buffer + Size)
        );
    }
  }

  //
  // Use SmmInternalFreePool() that will not update profile for this FreePool action.
  //
  SmmInternalFreePool (AllocInfoData);

  return TRUE;
}

/**
  Update SMRAM profile information.

  @param CallerAddress  Address of caller who call Allocate or Free.
  @param Action         This Allocate or Free action.
  @param MemoryType     Memory type.
  @param Size           Buffer size.
  @param Buffer         Buffer address.

  @retval TRUE          Profile udpate success.
  @retval FALSE         Profile update fail.

**/
BOOLEAN
SmmCoreUpdateProfile (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType, // Valid for AllocatePages/AllocatePool
  IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
  IN VOID                   *Buffer
  )
{
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;

  if (!IS_SMRAM_PROFILE_ENABLED) {
    return FALSE;
  }

  if (!mSmramProfileRecordingStatus) {
    return FALSE;
  }

  //
  // Free operations have no memory type information, so skip the check.
  //
  if ((Action == MemoryProfileActionAllocatePages) || (Action == MemoryProfileActionAllocatePool)) {
    //
    // Only record limited MemoryType.
    //
    if (!SmmCoreNeedRecordProfile (MemoryType)) {
      return FALSE;
    }
  }

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return FALSE;
  }

  switch (Action) {
    case MemoryProfileActionAllocatePages:
      SmmCoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer);
      break;
    case MemoryProfileActionFreePages:
      SmmCoreUpdateProfileFree (CallerAddress, Action, Size, Buffer);
      break;
    case MemoryProfileActionAllocatePool:
      SmmCoreUpdateProfileAllocate (CallerAddress, Action, MemoryType, Size, Buffer);
      break;
    case MemoryProfileActionFreePool:
      SmmCoreUpdateProfileFree (CallerAddress, Action, 0, Buffer);
      break;
    default:
      ASSERT (FALSE);
      break;
  }

  return TRUE;
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
  MEMORY_PROFILE_CONTEXT_DATA      *ContextData;
  MEMORY_PROFILE_DRIVER_INFO_DATA  *DriverInfoData;
  LIST_ENTRY                      *DriverInfoList;
  LIST_ENTRY                      *DriverLink;
  UINTN                           TotalSize;
  LIST_ENTRY                      *Node;
  LIST_ENTRY                      *FreePageList;
  LIST_ENTRY                      *FreePoolList;
  FREE_POOL_HEADER                *Pool;
  UINTN                           PoolListIndex;
  UINTN                           Index;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return 0;
  }

  TotalSize = sizeof (MEMORY_PROFILE_CONTEXT);
  TotalSize += sizeof (MEMORY_PROFILE_DRIVER_INFO) * (UINTN) ContextData->Context.ImageCount;

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
    TotalSize += sizeof (MEMORY_PROFILE_ALLOC_INFO) * (UINTN) DriverInfoData->DriverInfo.AllocRecordCount;
  }


  Index = 0;
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink;
       Node != FreePageList;
       Node = Node->BackLink) {
    Index++;
  }
  for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
    FreePoolList = &mSmmPoolLists[PoolListIndex];
    for (Node = FreePoolList->BackLink;
         Node != FreePoolList;
         Node = Node->BackLink) {
      Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
      if (Pool->Header.Available) {
        Index++;
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

**/
VOID
SmramProfileCopyData (
  IN VOID   *ProfileBuffer
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

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  Context = ProfileBuffer;
  CopyMem (Context, &ContextData->Context, sizeof (MEMORY_PROFILE_CONTEXT));
  DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) (Context + 1);

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
    CopyMem (DriverInfo, &DriverInfoData->DriverInfo, sizeof (MEMORY_PROFILE_DRIVER_INFO));
    AllocInfo = (MEMORY_PROFILE_ALLOC_INFO *) (DriverInfo + 1);

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
      CopyMem (AllocInfo, &AllocInfoData->AllocInfo, sizeof (MEMORY_PROFILE_ALLOC_INFO));
      AllocInfo += 1;
    }

    DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) ((UINTN) (DriverInfo + 1) + sizeof (MEMORY_PROFILE_ALLOC_INFO) * (UINTN) DriverInfo->AllocRecordCount);
  }


  FreeMemory = (MEMORY_PROFILE_FREE_MEMORY *) DriverInfo;
  CopyMem (FreeMemory, &mSmramFreeMemory, sizeof (MEMORY_PROFILE_FREE_MEMORY));
  MemoryProfileDescriptor = (MEMORY_PROFILE_DESCRIPTOR *) (FreeMemory + 1);
  Index = 0;
  FreePageList = &mSmmMemoryMap;
  for (Node = FreePageList->BackLink;
       Node != FreePageList;
       Node = Node->BackLink) {
    Pages = BASE_CR (Node, FREE_PAGE_LIST, Link);
    MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
    MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
    MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
    MemoryProfileDescriptor->Address = (PHYSICAL_ADDRESS) (UINTN) Pages;
    MemoryProfileDescriptor->Size = EFI_PAGES_TO_SIZE (Pages->NumberOfPages);
    MemoryProfileDescriptor++;
    Index++;
  }
  for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
    FreePoolList = &mSmmPoolLists[MAX_POOL_INDEX - PoolListIndex - 1];
    for (Node = FreePoolList->BackLink;
         Node != FreePoolList;
         Node = Node->BackLink) {
      Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
      if (Pool->Header.Available) {
        MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
        MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
        MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
        MemoryProfileDescriptor->Address = (PHYSICAL_ADDRESS) (UINTN) Pool;
        MemoryProfileDescriptor->Size = Pool->Header.Size;
        MemoryProfileDescriptor++;
        Index++;
      }
    }
  }
  FreeMemory->FreeMemoryEntryCount = Index;

  MemoryRange = (MEMORY_PROFILE_MEMORY_RANGE *) MemoryProfileDescriptor;
  MemoryRange->Header.Signature = MEMORY_PROFILE_MEMORY_RANGE_SIGNATURE;
  MemoryRange->Header.Length = sizeof (MEMORY_PROFILE_MEMORY_RANGE);
  MemoryRange->Header.Revision = MEMORY_PROFILE_MEMORY_RANGE_REVISION;
  MemoryRange->MemoryRangeCount = (UINT32) mFullSmramRangeCount;
  MemoryProfileDescriptor = (MEMORY_PROFILE_DESCRIPTOR *) (MemoryRange + 1);
  for (Index = 0; Index < mFullSmramRangeCount; Index++) {
    MemoryProfileDescriptor->Header.Signature = MEMORY_PROFILE_DESCRIPTOR_SIGNATURE;
    MemoryProfileDescriptor->Header.Length = sizeof (MEMORY_PROFILE_DESCRIPTOR);
    MemoryProfileDescriptor->Header.Revision = MEMORY_PROFILE_DESCRIPTOR_REVISION;
    MemoryProfileDescriptor->Address = mFullSmramRanges[Index].PhysicalStart;
    MemoryProfileDescriptor->Size = mFullSmramRanges[Index].PhysicalSize;
    MemoryProfileDescriptor++; 
  }
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
  BOOLEAN                       SmramProfileRecordingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;

  SmramProfileParameterGetInfo->ProfileSize = SmramProfileGetDataSize();
  SmramProfileParameterGetInfo->Header.ReturnStatus = 0;

  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
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
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA  SmramProfileGetData;
  MEMORY_PROFILE_CONTEXT_DATA               *ContextData;
  BOOLEAN                                   SmramProfileRecordingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;


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

  SmramProfileParameterGetData->ProfileSize = ProfileSize;
  SmramProfileCopyData ((VOID *) (UINTN) SmramProfileGetData.ProfileBuffer);
  SmramProfileParameterGetData->Header.ReturnStatus = 0;

Done:
  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
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
  BOOLEAN                           Ret;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  CopyMem (&DriverEntry.FileName, &SmramProfileParameterRegisterImage->FileName, sizeof(EFI_GUID));
  DriverEntry.ImageBuffer = SmramProfileParameterRegisterImage->ImageBuffer;
  DriverEntry.NumberOfPage = (UINTN) SmramProfileParameterRegisterImage->NumberOfPage;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  Ret = RegisterSmramProfileImage (&DriverEntry, FALSE);
  if (Ret) {
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
  BOOLEAN                           Ret;

  ZeroMem (&DriverEntry, sizeof (DriverEntry));
  CopyMem (&DriverEntry.FileName, &SmramProfileParameterUnregisterImage->FileName, sizeof (EFI_GUID));
  DriverEntry.ImageBuffer = SmramProfileParameterUnregisterImage->ImageBuffer;
  DriverEntry.NumberOfPage = (UINTN) SmramProfileParameterUnregisterImage->NumberOfPage;
  Status = InternalPeCoffGetEntryPoint ((VOID *) (UINTN) DriverEntry.ImageBuffer, &EntryPointInImage);
  ASSERT_EFI_ERROR (Status);
  DriverEntry.ImageEntryPoint = (PHYSICAL_ADDRESS) (UINTN) EntryPointInImage;

  Ret = UnregisterSmramProfileImage (&DriverEntry, FALSE);
  if (Ret) {
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
  BOOLEAN                       SmramProfileRecordingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;

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

  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
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
  BOOLEAN                       SmramProfileRecordingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;

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

  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
}

/**
  Dump SMRAM free pool list.

**/
VOID
DumpFreePoolList (
  VOID
  )
{
  LIST_ENTRY                     *FreePoolList;
  LIST_ENTRY                     *Node;
  FREE_POOL_HEADER               *Pool;
  UINTN                          Index;
  UINTN                          PoolListIndex;
  MEMORY_PROFILE_CONTEXT_DATA   *ContextData;
  BOOLEAN                       SmramProfileRecordingStatus;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;

  DEBUG ((EFI_D_INFO, "======= SmramProfile begin =======\n"));

  for (PoolListIndex = 0; PoolListIndex < MAX_POOL_INDEX; PoolListIndex++) {
    DEBUG ((EFI_D_INFO, "FreePoolList (%d):\n", PoolListIndex));
    FreePoolList = &mSmmPoolLists[PoolListIndex];
    for (Node = FreePoolList->BackLink, Index = 0;
         Node != FreePoolList;
         Node = Node->BackLink, Index++) {
      Pool = BASE_CR (Node, FREE_POOL_HEADER, Link);
      DEBUG ((EFI_D_INFO, "  Index - 0x%x\n", Index));
      DEBUG ((EFI_D_INFO, "    PhysicalStart - 0x%016lx\n", (PHYSICAL_ADDRESS) (UINTN) Pool));
      DEBUG ((EFI_D_INFO, "    Size          - 0x%08x\n", Pool->Header.Size));
      DEBUG ((EFI_D_INFO, "    Available     - 0x%02x\n", Pool->Header.Available));
    }
  }

  DEBUG ((EFI_D_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
}

GLOBAL_REMOVE_IF_UNREFERENCED CHAR16 *mActionString[] = {
  L"Unknown",
  L"AllocatePages",
  L"FreePages",
  L"AllocatePool",
  L"FreePool",
};

typedef struct {
  EFI_MEMORY_TYPE   MemoryType;
  CHAR16            *MemoryTypeStr;
} PROFILE_MEMORY_TYPE_STRING;

GLOBAL_REMOVE_IF_UNREFERENCED PROFILE_MEMORY_TYPE_STRING mMemoryTypeString[] = {
  {EfiRuntimeServicesCode, L"EfiRuntimeServicesCode"},
  {EfiRuntimeServicesData, L"EfiRuntimeServicesData"}
};

/**
  Memory type to string.

  @param[in] MemoryType Memory type.

  @return Pointer to string.

**/
CHAR16 *
ProfileMemoryTypeToStr (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  UINTN     Index;
  for (Index = 0; Index < sizeof (mMemoryTypeString) / sizeof (mMemoryTypeString[0]); Index++) {
    if (mMemoryTypeString[Index].MemoryType == MemoryType) {
      return mMemoryTypeString[Index].MemoryTypeStr;
    }
  }

  return L"UnexpectedMemoryType";
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
  BOOLEAN                           SmramProfileRecordingStatus;
  UINTN                             TypeIndex;

  ContextData = GetSmramProfileContext ();
  if (ContextData == NULL) {
    return ;
  }

  SmramProfileRecordingStatus = mSmramProfileRecordingStatus;
  mSmramProfileRecordingStatus = FALSE;

  Context = &ContextData->Context;
  DEBUG ((EFI_D_INFO, "======= SmramProfile begin =======\n"));
  DEBUG ((EFI_D_INFO, "MEMORY_PROFILE_CONTEXT\n"));

  DEBUG ((EFI_D_INFO, "  CurrentTotalUsage     - 0x%016lx\n", Context->CurrentTotalUsage));
  DEBUG ((EFI_D_INFO, "  PeakTotalUsage        - 0x%016lx\n", Context->PeakTotalUsage));
  for (TypeIndex = 0; TypeIndex < sizeof (Context->CurrentTotalUsageByType) / sizeof (Context->CurrentTotalUsageByType[0]); TypeIndex++) {
    if ((Context->CurrentTotalUsageByType[TypeIndex] != 0) ||
        (Context->PeakTotalUsageByType[TypeIndex] != 0)) {
      DEBUG ((EFI_D_INFO, "  CurrentTotalUsage[0x%02x]  - 0x%016lx (%s)\n", TypeIndex, Context->CurrentTotalUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
      DEBUG ((EFI_D_INFO, "  PeakTotalUsage[0x%02x]     - 0x%016lx (%s)\n", TypeIndex, Context->PeakTotalUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
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
        DEBUG ((EFI_D_INFO, "    CurrentUsage[0x%02x]     - 0x%016lx (%s)\n", TypeIndex, DriverInfo->CurrentUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
        DEBUG ((EFI_D_INFO, "    PeakUsage[0x%02x]        - 0x%016lx (%s)\n", TypeIndex, DriverInfo->PeakUsageByType[TypeIndex], ProfileMemoryTypeToStr (TypeIndex)));
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
      DEBUG ((EFI_D_INFO, "      Action         - 0x%08x (%s)\n", AllocInfo->Action, mActionString[(AllocInfo->Action < sizeof(mActionString)/sizeof(mActionString[0])) ? AllocInfo->Action : 0]));
      DEBUG ((EFI_D_INFO, "      MemoryType     - 0x%08x\n", AllocInfo->MemoryType));
      DEBUG ((EFI_D_INFO, "      Buffer         - 0x%016lx\n", AllocInfo->Buffer));
      DEBUG ((EFI_D_INFO, "      Size           - 0x%016lx\n", AllocInfo->Size));
    }
  }

  DEBUG ((EFI_D_INFO, "======= SmramProfile end =======\n"));

  mSmramProfileRecordingStatus = SmramProfileRecordingStatus;
}

/**
  Dump SMRAM infromation.

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

