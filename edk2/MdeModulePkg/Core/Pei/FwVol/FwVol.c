/** @file
  Pei Core Firmware File System service routines.
  
Copyright (c) 2006 - 2008, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include <PeiMain.h>

STATIC EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnFvInfoList = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiFirmwareVolumeInfoPpiGuid,
  FirmwareVolmeInfoPpiNotifyCallback 
};


#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  ((ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1)))

/**
  Returns the file state set by the highest zero bit in the State field

  @param ErasePolarity   Erase Polarity  as defined by EFI_FVB2_ERASE_POLARITY
                         in the Attributes field.
  @param FfsHeader       Pointer to FFS File Header.

  @retval EFI_FFS_FILE_STATE File state is set by the highest none zero bit 
                             in the header State field.
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
  
  //
  // Get file state set by its highest none zero bit.
  //
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
          Zero means the header is good.
          Non-zero means the header is bad.
**/
UINT8
CalculateHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FileHeader
  )
{
  EFI_FFS_FILE_HEADER TestFileHeader;
  
  CopyMem (&TestFileHeader, FileHeader, sizeof (EFI_FFS_FILE_HEADER));
  //
  // Ingore State and File field in FFS header.
  //
  TestFileHeader.State = 0;
  TestFileHeader.IntegrityCheck.Checksum.File = 0;

  return CalculateSum8 ((CONST UINT8 *) &TestFileHeader, sizeof (EFI_FFS_FILE_HEADER));
}

/**
  Find FV handler according some FileHandle in that FV.

  @param FileHandle      Handle of file image
  @param VolumeHandle    Handle of the found FV, if not found, NULL will be set.

  @retval TRUE           The corresponding FV handler is found.
  @retval FALSE          The corresponding FV handler is not found.

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
  *VolumeHandle = NULL;
  return FALSE;
}

/**
  Given the input file pointer, search for the first matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.
  If SearchType is EFI_FV_FILETYPE_ALL, the first FFS file will return without check its file type.
  If SearchType is PEI_CORE_INTERNAL_FFS_FILE_DISPATCH_TYPE, 
  the first PEIM, or COMBINED PEIM or FV file type FFS file will return.  

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
      FileOffset    += sizeof(EFI_FFS_FILE_HEADER);
      FfsFileHeader =  (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + sizeof(EFI_FFS_FILE_HEADER));
      break;
        
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      if (CalculateHeaderChecksum (FfsFileHeader) != 0) {
        ASSERT (FALSE);
        *FileHeader = NULL;
        return EFI_NOT_FOUND;
      }

      FileLength       = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
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

      FileOffset    += FileOccupiedSize; 
      FfsFileHeader =  (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
      break;
    
    case EFI_FILE_DELETED:
      FileLength       =  *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
      FileOccupiedSize =  GET_OCCUPIED_SIZE(FileLength, 8);
      FileOffset       += FileOccupiedSize;
      FfsFileHeader    =  (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
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
  The FV Info will be registered into PeiCore private data structure.
  And search the inside FV image, if found, the new FV INFO PPI will be installed.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS    The FV Info is registered into PeiCore private data structure.
  @return if not EFI_SUCESS, fail to verify FV.

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
    
    Status = VerifyFv ((EFI_FIRMWARE_VOLUME_HEADER*)Fv->FvInfo);
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_ERROR, "Fail to verify FV which address is 0x%11p", (VOID *) Fv->FvInfo));
      return Status;
    }
    
    PrivateData->Fv[PrivateData->FvCount++].FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*)Fv->FvInfo;

    //
    // Only add FileSystem2 Fv to the All list
    //
    PrivateData->AllFv[PrivateData->AllFvCount++] = (EFI_PEI_FV_HANDLE)Fv->FvInfo;
    
    DEBUG ((EFI_D_INFO, "The %dth FvImage start address is 0x%11p and size is 0x%08x\n", PrivateData->AllFvCount, (VOID *) Fv->FvInfo, Fv->FvInfoSize));
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
  Go through the file to search SectionType section. 
  Search within encapsulation sections (compression and GUIDed) recursively, 
  until the match section is found.
  
  @param PeiServices     - An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SectionType     - Filter to find only section of this type.
  @param Section         - From where to search.
  @param SectionSize     - The file size to search.
  @param OutputBuffer    - A pointer to the discovered section, if successful.
                           NULL if section not found

  @return EFI_NOT_FOUND    The match section is not found.
  @return EFI_SUCCESS      The match section is found.

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
  Given the input file pointer, search for the first matching section in the
  FFS volume.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param SectionType     Filter to find only sections of this type.
  @param FileHandle      Pointer to the current file to search.
  @param SectionData     A pointer to the discovered section, if successful.
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


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
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
  Search the firmware volumes by index

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param Instance        Instance of FV to find
  @param VolumeHandle    Pointer to found Volume.

  @retval EFI_INVALID_PARAMETER  FwVolHeader is NULL
  @retval EFI_SUCCESS            Firmware volume instance successfully found.

**/
EFI_STATUS 
EFIAPI
PeiFvFindNextVolume (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN     UINTN                      Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  )
{
  PEI_CORE_INSTANCE        *Private;
  UINTN                    Index;
  BOOLEAN                  Match;
  EFI_HOB_FIRMWARE_VOLUME  *FvHob;

  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  if (VolumeHandle == NULL) {
   return EFI_INVALID_PARAMETER;
  }
  
  //
  // Handle Framework FvHob and Install FvInfo Ppi for it.
  //
  if (FeaturePcdGet (PcdFrameworkFvHobCompatibilitySupport)) {
    //
    // Loop to search the wanted FirmwareVolume which supports FFS
    //
    FvHob = (EFI_HOB_FIRMWARE_VOLUME *)GetFirstHob (EFI_HOB_TYPE_FV);
    while (FvHob != NULL) {
      for (Index = 0, Match = FALSE; Index < Private->AllFvCount; Index++) {
        if ((EFI_PEI_FV_HANDLE)(UINTN)FvHob->BaseAddress == Private->AllFv[Index]) {
          Match = TRUE;
          break;
        }
      }
      //
      // If Not Found, Install FvInfo Ppi for it.
      //
      if (!Match) {
        PiLibInstallFvInfoPpi (
          NULL,
          (VOID *)(UINTN)FvHob->BaseAddress,
          (UINT32)FvHob->Length,
          NULL,
          NULL
          );
      }
      FvHob = (EFI_HOB_FIRMWARE_VOLUME *)GetNextHob (EFI_HOB_TYPE_FV, (VOID *)((UINTN)FvHob + FvHob->Header.HobLength)); 
    }
  }

  if (Instance >= Private->AllFvCount) {
   VolumeHandle = NULL;
   return EFI_NOT_FOUND;
  }

  *VolumeHandle = Private->AllFv[Instance];
  return EFI_SUCCESS;
}


/**
  Find a file within a volume by its name.

  @param FileName        A pointer to the name of the file to find within the firmware volume.
  @param VolumeHandle    The firmware volume to search
  @param FileHandle      Upon exit, points to the found file's handle 
                         or NULL if it could not be found.

  @retval EFI_SUCCESS            File was found.
  @retval EFI_NOT_FOUND          File was not found.
  @retval EFI_INVALID_PARAMETER  VolumeHandle or FileHandle or FileName was NULL.

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

/**
  Get Fv image from the FV type file, then install FV INFO ppi, Build FV hob.

  @param PeiServices          An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param FvFileHandle         File handle of a Fv type file.
  @param AuthenticationState  Pointer to attestation authentication state of image.


  @retval EFI_NOT_FOUND         FV image can't be found.
  @retval EFI_SUCCESS           Successfully to process it.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate page when aligning FV image
  @retval Others                Can not find EFI_SECTION_FIRMWARE_VOLUME_IMAGE section
  
**/
EFI_STATUS
ProcessFvFile (
  IN  EFI_PEI_SERVICES      **PeiServices,
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle,
  OUT UINT32                *AuthenticationState
  )
{
  EFI_STATUS            Status;
  EFI_PEI_FV_HANDLE     FvImageHandle;
  EFI_FV_INFO           FvImageInfo;
  UINT32                FvAlignment;
  VOID                  *FvBuffer;
  EFI_PEI_HOB_POINTERS  HobPtr;

  FvBuffer             = NULL;
  *AuthenticationState = 0;

  //
  // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has already
  // been extracted.
  //
  HobPtr.Raw = GetHobList ();
  while ((HobPtr.Raw = GetNextHob (EFI_HOB_TYPE_FV2, HobPtr.Raw)) != NULL) {
    if (CompareGuid (&(((EFI_FFS_FILE_HEADER *)FvFileHandle)->Name), &HobPtr.FirmwareVolume2->FileName)) {
      //
      // this FILE has been dispatched, it will not be dispatched again.
      //
      return EFI_SUCCESS;
    }
    HobPtr.Raw = GET_NEXT_HOB (HobPtr);
  }

  //
  // Find FvImage in FvFile
  //
  Status = PeiFfsFindSectionData (
             (CONST EFI_PEI_SERVICES **) PeiServices,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             FvFileHandle,
             (VOID **)&FvImageHandle
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Collect FvImage Info.
  //
  Status = PeiFfsGetVolumeInfo (FvImageHandle, &FvImageInfo);
  ASSERT_EFI_ERROR (Status);
  
  //
  // FvAlignment must be more than 8 bytes required by FvHeader structure.
  //
  FvAlignment = 1 << ((FvImageInfo.FvAttributes & EFI_FVB2_ALIGNMENT) >> 16);
  if (FvAlignment < 8) {
    FvAlignment = 8;
  }
  
  //
  // Check FvImage
  //
  if ((UINTN) FvImageInfo.FvStart % FvAlignment != 0) {
    FvBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINT32) FvImageInfo.FvSize), FvAlignment);
    if (FvBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (FvBuffer, FvImageInfo.FvStart, (UINTN) FvImageInfo.FvSize);
    //
    // Update FvImageInfo after reload FvImage to new aligned memory
    //
    PeiFfsGetVolumeInfo ((EFI_PEI_FV_HANDLE) FvBuffer, &FvImageInfo);
  }

  //
  // Install FvPpi and Build FvHob
  //
  PiLibInstallFvInfoPpi (
    NULL,
    FvImageInfo.FvStart,
    (UINT32) FvImageInfo.FvSize,
    &(FvImageInfo.FvName),
    &(((EFI_FFS_FILE_HEADER*)FvFileHandle)->Name)
    );

  //
  // Inform the extracted FvImage to Fv HOB consumer phase, i.e. DXE phase
  //
  BuildFvHob (
    (EFI_PHYSICAL_ADDRESS) (UINTN) FvImageInfo.FvStart,
    FvImageInfo.FvSize
  );

  //
  // Makes the encapsulated volume show up in DXE phase to skip processing of
  // encapsulated file again.
  //
  BuildFv2Hob (
    (EFI_PHYSICAL_ADDRESS) (UINTN) FvImageInfo.FvStart,
    FvImageInfo.FvSize,
    &FvImageInfo.FvName,
    &(((EFI_FFS_FILE_HEADER *)FvFileHandle)->Name)
    );

  return EFI_SUCCESS;
}


