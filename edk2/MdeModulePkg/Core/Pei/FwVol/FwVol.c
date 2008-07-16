/** @file
  Pei Core Firmware File System service routines.
  
Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVol.c

Abstract:

  

**/

#include <PeiMain.h>

STATIC EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnFvInfoList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiFirmwareVolumeInfoPpiGuid,
  FirmwareVolmeInfoPpiNotifyCallback 
};


#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))

/**
  Returns the highest bit set of the State field


  @param ErasePolarity   Erase Polarity  as defined by EFI_FVB2_ERASE_POLARITY
                         in the Attributes field.
  @param FfsHeader       Pointer to FFS File Header.

  @return Returns the highest bit in the State field

**/
EFI_FFS_FILE_STATE
GetFileState(
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_STATE  HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE)~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && (HighestBit & FileState) == 0) {
    HighestBit >>= 1;
  }

  return HighestBit;
} 

/**
  Calculates the checksum of the header of a file.

  @param FileHeader      Pointer to FFS File Header.

  @return Checksum of the header.
          The header is zero byte checksum.
          Zero means the header is good.
          Non-zero means the header is bad.
**/
UINT8
CalculateHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FileHeader
  )
{
  UINT8   *Ptr;
  UINTN   Index;
  UINT8   Sum;
  
  Sum = 0;
  Ptr = (UINT8 *)FileHeader;

  for (Index = 0; Index < sizeof(EFI_FFS_FILE_HEADER) - 3; Index += 4) {
    Sum = (UINT8)(Sum + Ptr[Index]);
    Sum = (UINT8)(Sum + Ptr[Index+1]);
    Sum = (UINT8)(Sum + Ptr[Index+2]);
    Sum = (UINT8)(Sum + Ptr[Index+3]);
  }

  for (; Index < sizeof(EFI_FFS_FILE_HEADER); Index++) {
    Sum = (UINT8)(Sum + Ptr[Index]);
  }
  
  //
  // State field (since this indicates the different state of file). 
  //
  Sum = (UINT8)(Sum - FileHeader->State);
  //
  // Checksum field of the file is not part of the header checksum.
  //
  Sum = (UINT8)(Sum - FileHeader->IntegrityCheck.Checksum.File);

  return Sum;
}

/**
  Find FV handler according some FileHandle in that FV.

  @param FileHandle      Handle of file image
  @param VolumeHandle    Handle of FV

  @return EDES_TODO: Add description for return value

**/
BOOLEAN
EFIAPI
PeiFileHandleToVolume (
  IN   EFI_PEI_FILE_HANDLE     FileHandle,
  OUT  EFI_PEI_FV_HANDLE       *VolumeHandle
  )
{
  UINTN                       Index;
  PEI_CORE_INSTANCE           *PrivateData;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer ());
  for (Index = 0; Index < PrivateData->FvCount; Index++) {
    FwVolHeader = PrivateData->Fv[Index].FvHeader;
    if (((UINT64) (UINTN) FileHandle > (UINT64) (UINTN) FwVolHeader ) &&   \
        ((UINT64) (UINTN) FileHandle <= ((UINT64) (UINTN) FwVolHeader + FwVolHeader->FvLength - 1))) {
      *VolumeHandle = (EFI_PEI_FV_HANDLE)FwVolHeader;
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.


  @param FvHandle        Pointer to the FV header of the volume to search
  @param FileName        File name
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FileHandle      This parameter must point to a valid FFS volume.
  @param AprioriFile     Pointer to AprioriFile image in this FV if has

  @return EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to search given file

**/
EFI_STATUS
PeiFindFileEx (
  IN  CONST EFI_PEI_FV_HANDLE        FvHandle,
  IN  CONST EFI_GUID                 *FileName,   OPTIONAL
  IN        EFI_FV_FILETYPE          SearchType,
  IN OUT    EFI_PEI_FILE_HANDLE      *FileHandle,
  IN OUT    EFI_PEI_FV_HANDLE        *AprioriFile  OPTIONAL
  )
{
  EFI_FIRMWARE_VOLUME_HEADER           *FwVolHeader;
  EFI_FFS_FILE_HEADER                   **FileHeader;
  EFI_FFS_FILE_HEADER                   *FfsFileHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER        *FwVolExHeaderInfo;
  UINT32                                FileLength;
  UINT32                                FileOccupiedSize;
  UINT32                                FileOffset;
  UINT64                                FvLength;
  UINT8                                 ErasePolarity;
  UINT8                                 FileState;

  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FvHandle;
  FileHeader  = (EFI_FFS_FILE_HEADER **)FileHandle;

  FvLength = FwVolHeader->FvLength;
  if (FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }
  
  //
  // If FileHeader is not specified (NULL) or FileName is not NULL,
  // start with the first file in the firmware volume.  Otherwise,
  // start from the FileHeader.
  //
  if ((*FileHeader == NULL) || (FileName != NULL)) {
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
    if (FwVolHeader->ExtHeaderOffset != 0) {
      FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER *)(((UINT8 *)FwVolHeader) + FwVolHeader->ExtHeaderOffset);
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)(((UINT8 *)FwVolExHeaderInfo) + FwVolExHeaderInfo->ExtHeaderSize);
    }
  } else {
    //
    // Length is 24 bits wide so mask upper 8 bits
    // FileLength is adjusted to FileOccupiedSize as it is 8 byte aligned.
    //
    FileLength = *(UINT32 *)(*FileHeader)->Size & 0x00FFFFFF;
    FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)*FileHeader + FileOccupiedSize);
  }
  
  FileOffset = (UINT32) ((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);
  ASSERT (FileOffset <= 0xFFFFFFFF);

  while (FileOffset < (FvLength - sizeof (EFI_FFS_FILE_HEADER))) {
    //
    // Get FileState which is the highest bit of the State 
    //
    FileState = GetFileState (ErasePolarity, FfsFileHeader);
    switch (FileState) {

    case EFI_FILE_HEADER_INVALID:
      FileOffset += sizeof(EFI_FFS_FILE_HEADER);
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + sizeof(EFI_FFS_FILE_HEADER));
      break;
        
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      if (CalculateHeaderChecksum (FfsFileHeader) != 0) {
        ASSERT (FALSE);
        *FileHeader = NULL;
        return EFI_NOT_FOUND;
      }

      FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
      FileOccupiedSize = GET_OCCUPIED_SIZE(FileLength, 8);

      if (FileName != NULL) {
        if (CompareGuid (&FfsFileHeader->Name, (EFI_GUID*)FileName)) {
          *FileHeader = FfsFileHeader;
          return EFI_SUCCESS;
        }
      } else if (SearchType == PEI_CORE_INTERNAL_FFS_FILE_DISPATCH_TYPE) {
        if ((FfsFileHeader->Type == EFI_FV_FILETYPE_PEIM) || 
            (FfsFileHeader->Type == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER) ||
            (FfsFileHeader->Type == EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE)) { 
          
          *FileHeader = FfsFileHeader;
          return EFI_SUCCESS;
        } else if (AprioriFile != NULL) {
          if (FfsFileHeader->Type == EFI_FV_FILETYPE_FREEFORM) {
            if (CompareGuid (&FfsFileHeader->Name, &gPeiAprioriFileNameGuid)) {
              *AprioriFile = FfsFileHeader;
            }           
          } 
        }
      } else if (((SearchType == FfsFileHeader->Type) || (SearchType == EFI_FV_FILETYPE_ALL)) && 
                 (FfsFileHeader->Type != EFI_FV_FILETYPE_FFS_PAD)) { 
        *FileHeader = FfsFileHeader;
        return EFI_SUCCESS;
      }

      FileOffset += FileOccupiedSize; 
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
      break;
    
    case EFI_FILE_DELETED:
      FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
      FileOccupiedSize = GET_OCCUPIED_SIZE(FileLength, 8);
      FileOffset += FileOccupiedSize;
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
      break;

    default:
      *FileHeader = NULL;
      return EFI_NOT_FOUND;
    } 
  }
  
  *FileHeader = NULL;
  return EFI_NOT_FOUND;  
}

/**

  Initialize PeiCore Fv List.


  @param PrivateData     - Pointer to PEI_CORE_INSTANCE.
  @param SecCoreData     - Pointer to EFI_SEC_PEI_HAND_OFF.

  @return NONE

**/
VOID 
PeiInitializeFv (
  IN  PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  )
{
  EFI_STATUS  Status;
  //
  // The BFV must be the first entry. The Core FV support is stateless 
  // The AllFV list has a single entry per FV in PEI. 
  // The Fv list only includes FV that PEIMs will be dispatched from and
  // its File System Format is PI 1.0 definition.
  //
  PrivateData->FvCount = 1;
  PrivateData->Fv[0].FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;

  PrivateData->AllFvCount = 1;
  PrivateData->AllFv[0] = (EFI_PEI_FV_HANDLE)PrivateData->Fv[0].FvHeader;


  //
  // Post a call-back for the FvInfoPPI services to expose
  // additional Fvs to PeiCore.
  //
  Status = PeiServicesNotifyPpi (&mNotifyOnFvInfoList);
  ASSERT_EFI_ERROR (Status);

}

/**
  Process Firmware Volum Information once FvInfoPPI install.


  @param PeiServices     - General purpose services available to every PEIM.
  @param NotifyDescriptor EDES_TODO: Add parameter description
  @param Ppi             EDES_TODO: Add parameter description

  @retval EFI_SUCCESS if the interface could be successfully installed

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  UINT8                                 FvCount;
  EFI_PEI_FIRMWARE_VOLUME_INFO_PPI      *Fv;
  PEI_CORE_INSTANCE                     *PrivateData;
  EFI_PEI_FILE_HANDLE                   FileHandle;
  VOID                                  *DepexData;
  UINT32                                AuthenticationStatus;
  EFI_STATUS                            Status;
  
  FileHandle   = NULL;
  DepexData    = NULL;
  Status       = EFI_SUCCESS;
  PrivateData  = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  if (PrivateData->FvCount >= FixedPcdGet32 (PcdPeiCoreMaxFvSupported)) {
    ASSERT (FALSE);
  }

  Fv = (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI *)Ppi;

  if (CompareGuid (&Fv->FvFormat, &gEfiFirmwareFileSystem2Guid)) {
    for (FvCount = 0; FvCount < PrivateData->FvCount; FvCount ++) {
      if ((UINTN)PrivateData->Fv[FvCount].FvHeader == (UINTN)Fv->FvInfo) {
        return EFI_SUCCESS;
      }
    }
    PrivateData->Fv[PrivateData->FvCount++].FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Fv->FvInfo;

    //
    // Only add FileSystem2 Fv to the All list
    //
    PrivateData->AllFv[PrivateData->AllFvCount++] = (EFI_PEI_FV_HANDLE)Fv->FvInfo;
    
    DEBUG ((EFI_D_INFO, "The %dth FvImage start address is 0x%10p and size is 0x%08x\n", PrivateData->AllFvCount, (VOID *) Fv->FvInfo, Fv->FvInfoSize));
    //
    // Preprocess all FV type files in this new FileSystem2 Fv image
    //
    do {
      Status = PeiFindFileEx (
                 (EFI_PEI_FV_HANDLE)Fv->FvInfo, 
                 NULL, 
                 EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, 
                 &FileHandle, 
                 NULL
                 );
      if (!EFI_ERROR (Status)) {
        Status = PeiFfsFindSectionData (
                    (CONST EFI_PEI_SERVICES **) PeiServices,
                    EFI_SECTION_PEI_DEPEX,
                    FileHandle, 
                    (VOID **)&DepexData
                    );
        if (!EFI_ERROR (Status)) {
          if (!PeimDispatchReadiness (PeiServices, DepexData)) {
            //
            // Dependency is not satisfied.
            //
            continue;
          }
        }
        //
        // Process FvFile to install FvInfo ppi and build FvHob
        // 
        ProcessFvFile (PeiServices, FileHandle, &AuthenticationStatus);
      }
    } while (FileHandle != NULL);
  }

  return EFI_SUCCESS;
}

/**

  Go through the file to search SectionType section,
  when meeting an encapsuled section.


  @param PeiServices     - General purpose services available to every PEIM.
                         SearchType   - Filter to find only section of this type.
  @param SectionType     EDES_TODO: Add parameter description
  @param Section         - From where to search.
  @param SectionSize     - The file size to search.
  @param OutputBuffer    - Pointer to the section to search.

  @return EFI_STATUS

**/
EFI_STATUS
PeiFfsProcessSection (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_SECTION_TYPE           SectionType,
  IN EFI_COMMON_SECTION_HEADER  *Section,
  IN UINTN                      SectionSize,
  OUT VOID                      **OutputBuffer
  )
{
  EFI_STATUS                              Status;
  UINT32                                  SectionLength;
  UINT32                                  ParsedLength;
  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI   *GuidSectionPpi;
  EFI_PEI_DECOMPRESS_PPI                  *DecompressPpi;
  VOID                                    *PpiOutput;
  UINTN                                   PpiOutputSize;
  UINTN                                   Index;
  UINT32                                  Authentication;
  PEI_CORE_INSTANCE                       *PrivateData;

  PrivateData   = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  *OutputBuffer = NULL;
  ParsedLength  = 0;
  Index         = 0;
  Status        = EFI_NOT_FOUND;
  PpiOutput     = NULL;
  PpiOutputSize = 0;
  while (ParsedLength < SectionSize) {
    if (Section->Type == SectionType) {
      *OutputBuffer = (VOID *)(Section + 1);
      return EFI_SUCCESS;
    } else if ((Section->Type == EFI_SECTION_GUID_DEFINED) || (Section->Type == EFI_SECTION_COMPRESSION)) {
      //
      // Check the encapsulated section is extracted into the cache data.
      //
      for (Index = 0; Index < PrivateData->CacheSection.AllSectionCount; Index ++) {
        if (Section == PrivateData->CacheSection.Section[Index]) {
          PpiOutput     = PrivateData->CacheSection.SectionData[Index];
          PpiOutputSize = PrivateData->CacheSection.SectionSize[Index];
          //
          // Search section directly from the cache data.
          //
          return PeiFfsProcessSection (
                  PeiServices,
                  SectionType, 
                  PpiOutput, 
                  PpiOutputSize, 
                  OutputBuffer 
                  );
        }
      }
      
      Status = EFI_NOT_FOUND;
      if (Section->Type == EFI_SECTION_GUID_DEFINED) {
        Status = PeiServicesLocatePpi (
                   &((EFI_GUID_DEFINED_SECTION *)Section)->SectionDefinitionGuid, 
                   0, 
                   NULL, 
                   (VOID **) &GuidSectionPpi
                   );
        if (!EFI_ERROR (Status)) {
          Status = GuidSectionPpi->ExtractSection (
                                    GuidSectionPpi,
                                    Section,
                                    &PpiOutput,
                                    &PpiOutputSize,
                                    &Authentication
                                    );
        }
      } else if (Section->Type == EFI_SECTION_COMPRESSION) {
        Status = PeiServicesLocatePpi (&gEfiPeiDecompressPpiGuid, 0, NULL, (VOID **) &DecompressPpi);
        if (!EFI_ERROR (Status)) {
          Status = DecompressPpi->Decompress (
                                    DecompressPpi,
                                    (CONST EFI_COMPRESSION_SECTION*) Section,
                                    &PpiOutput,
                                    &PpiOutputSize
                                    );
        }
      }
      
      if (!EFI_ERROR (Status)) {
        //
        // Update cache section data.
        //
        if (PrivateData->CacheSection.AllSectionCount < CACHE_SETION_MAX_NUMBER) {
          PrivateData->CacheSection.AllSectionCount ++;
        }
        PrivateData->CacheSection.Section [PrivateData->CacheSection.SectionIndex]     = Section;
        PrivateData->CacheSection.SectionData [PrivateData->CacheSection.SectionIndex] = PpiOutput;
        PrivateData->CacheSection.SectionSize [PrivateData->CacheSection.SectionIndex] = PpiOutputSize;
        PrivateData->CacheSection.SectionIndex = (PrivateData->CacheSection.SectionIndex + 1)%CACHE_SETION_MAX_NUMBER;
        
        return PeiFfsProcessSection (
                PeiServices,
                SectionType, 
                PpiOutput, 
                PpiOutputSize, 
                OutputBuffer 
                );
      }
    }

    //
    // Size is 24 bits wide so mask upper 8 bits. 
    // SectionLength is adjusted it is 4 byte aligned.
    // Go to the next section
    //
    SectionLength = *(UINT32 *)Section->Size & 0x00FFFFFF;
    SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
    ASSERT (SectionLength != 0);
    ParsedLength += SectionLength;
    Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
  }
  
  return EFI_NOT_FOUND;
}


/**
  Given the input file pointer, search for the next matching section in the
  FFS volume.


  @param PeiServices     Pointer to the PEI Core Services Table.
  @param SectionType     Filter to find only sections of this type.
  @param FileHandle      Pointer to the current file to search.
  @param SectionData     Pointer to the Section matching SectionType in FfsFileHeader.
                         NULL if section not found

  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to find section data in given file

**/
EFI_STATUS
EFIAPI
PeiFfsFindSectionData (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     EFI_PEI_FILE_HANDLE   FileHandle,
  IN OUT VOID                  **SectionData
  )
{
  EFI_FFS_FILE_HEADER                     *FfsFileHeader;
  UINT32                                  FileSize;
  EFI_COMMON_SECTION_HEADER               *Section;

  FfsFileHeader = (EFI_FFS_FILE_HEADER *)(FileHandle);

  //
  // Size is 24 bits wide so mask upper 8 bits. 
  // Does not include FfsFileHeader header size
  // FileSize is adjusted to FileOccupiedSize as it is 8 byte aligned.
  //
  Section = (EFI_COMMON_SECTION_HEADER *)(FfsFileHeader + 1);
  FileSize = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
  FileSize -= sizeof (EFI_FFS_FILE_HEADER);

  return PeiFfsProcessSection (
          PeiServices,
          SectionType, 
          Section, 
          FileSize, 
          SectionData
          );
}

/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.


  @param PeiServices     Pointer to the PEI Core Services Table.
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param VolumeHandle    Pointer to the FV header of the volume to search.
  @param FileHandle      Pointer to the current file from which to begin searching.
                         This pointer will be updated upon return to reflect the file found.
  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to find next file in given volume

**/
EFI_STATUS
EFIAPI
PeiFfsFindNextFile (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT8                       SearchType,
  IN EFI_PEI_FV_HANDLE           VolumeHandle,
  IN OUT EFI_PEI_FILE_HANDLE     *FileHandle
  )
{
  return PeiFindFileEx (VolumeHandle, NULL, SearchType, FileHandle, NULL);
}


/**
  search the firmware volumes by index

  @param PeiServices     The PEI core services table.
  @param Instance        Instance of FV to find
  @param VolumeHandle    Pointer to found Volume.

  @retval EFI_INVALID_PARAMETER  FwVolHeader is NULL
  @retval EFI_SUCCESS            Firmware volume instance successfully found.

**/
EFI_STATUS 
EFIAPI
PeiFvFindNextVolume (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN     UINTN                      Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  )
{
  PEI_CORE_INSTANCE   *Private;

  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  if (VolumeHandle == NULL) {
   return EFI_INVALID_PARAMETER;
  } 

  if (Instance >= Private->AllFvCount) {
   VolumeHandle = NULL;
   return EFI_NOT_FOUND;
  }

  *VolumeHandle = Private->AllFv[Instance];
  return EFI_SUCCESS;
}


/**

  Given the input VolumeHandle, search for the next matching name file.


  @param FileName        - File name to search.
  @param VolumeHandle    - The current FV to search.
  @param FileHandle      - Pointer to the file matching name in VolumeHandle.
                         - NULL if file not found

  @return EFI_STATUS

**/
EFI_STATUS
EFIAPI 
PeiFfsFindFileByName (
  IN  CONST EFI_GUID        *FileName,
  IN  EFI_PEI_FV_HANDLE     VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  )
{
  EFI_STATUS  Status;
  if ((VolumeHandle == NULL) || (FileName == NULL) || (FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  Status = PeiFindFileEx (VolumeHandle, FileName, 0, FileHandle, NULL);
  if (Status == EFI_NOT_FOUND) {
    *FileHandle = NULL;
  }
  return Status;
}

/**

  Returns information about a specific file.


  @param FileHandle      - The handle to file.
  @param FileInfo        - Pointer to the file information.

  @retval EFI_INVALID_PARAMETER Invalid FileHandle or FileInfo.
  @retval EFI_SUCCESS           Success to collect file info.

**/
EFI_STATUS
EFIAPI 
PeiFfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
  )
{
  UINT8                       FileState;
  UINT8                       ErasePolarity;
  EFI_FFS_FILE_HEADER         *FileHeader;
  EFI_PEI_FV_HANDLE           VolumeHandle;

  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VolumeHandle = 0;
  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  if (!PeiFileHandleToVolume(FileHandle, &VolumeHandle)) {
    return EFI_INVALID_PARAMETER;
  }

  if (((EFI_FIRMWARE_VOLUME_HEADER*)VolumeHandle)->Attributes & EFI_FVB2_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }

  //
  // Get FileState which is the highest bit of the State 
  //
  FileState = GetFileState (ErasePolarity, (EFI_FFS_FILE_HEADER*)FileHandle);

  switch (FileState) {
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      break;  
    default:
      return EFI_INVALID_PARAMETER;
    }

  FileHeader = (EFI_FFS_FILE_HEADER *)FileHandle;
  CopyMem (&FileInfo->FileName, &FileHeader->Name, sizeof(EFI_GUID));
  FileInfo->FileType = FileHeader->Type;
  FileInfo->FileAttributes = FileHeader->Attributes;
  FileInfo->BufferSize = ((*(UINT32 *)FileHeader->Size) & 0x00FFFFFF) -  sizeof (EFI_FFS_FILE_HEADER);
  FileInfo->Buffer = (FileHeader + 1);
  return EFI_SUCCESS;
}


/**

  Collect information of given Fv Volume.

  @param VolumeHandle    - The handle to Fv Volume.
  @param VolumeInfo      - The pointer to volume information.

  @retval EFI_INVALID_PARAMETER VolumeInfo is NULL
  @retval EFI_SUCCESS           Success to collect fv info.
**/
EFI_STATUS
EFIAPI 
PeiFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
{
  EFI_FIRMWARE_VOLUME_HEADER             FwVolHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER         *FwVolExHeaderInfo;

  if (VolumeInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // VolumeHandle may not align at 8 byte, 
  // but FvLength is UINT64 type, which requires FvHeader align at least 8 byte. 
  // So, Copy FvHeader into the local FvHeader structure.
  //
  CopyMem (&FwVolHeader, VolumeHandle, sizeof (EFI_FIRMWARE_VOLUME_HEADER));
  //
  // Check Fv Image Signature
  //
  if (FwVolHeader.Signature != EFI_FVH_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }
  VolumeInfo->FvAttributes = FwVolHeader.Attributes;
  VolumeInfo->FvStart = (VOID *) VolumeHandle;
  VolumeInfo->FvSize = FwVolHeader.FvLength;
  CopyMem (&VolumeInfo->FvFormat, &FwVolHeader.FileSystemGuid, sizeof(EFI_GUID));

  if (FwVolHeader.ExtHeaderOffset != 0) {
    FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER*)(((UINT8 *)VolumeHandle) + FwVolHeader.ExtHeaderOffset);
    CopyMem (&VolumeInfo->FvName, &FwVolExHeaderInfo->FvName, sizeof(EFI_GUID));
  }
  return EFI_SUCCESS;
}

