/** @file
  Pei Core Firmware File System service routines.
  
Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "FwVol.h"

EFI_PEI_NOTIFY_DESCRIPTOR mNotifyOnFvInfoList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK,
    &gEfiPeiFirmwareVolumeInfoPpiGuid,
    FirmwareVolmeInfoPpiNotifyCallback 
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiFirmwareVolumeInfo2PpiGuid,
    FirmwareVolmeInfoPpiNotifyCallback 
  }
};

PEI_FW_VOL_INSTANCE mPeiFfs2FwVol = {
  PEI_FW_VOL_SIGNATURE,
  FALSE,
  {
    PeiFfsFvPpiProcessVolume,
    PeiFfsFvPpiFindFileByType,
    PeiFfsFvPpiFindFileByName,
    PeiFfsFvPpiGetFileInfo,
    PeiFfsFvPpiGetVolumeInfo,
    PeiFfsFvPpiFindSectionByType,
    PeiFfsFvPpiGetFileInfo2,
    PeiFfsFvPpiFindSectionByType2,
    EFI_PEI_FIRMWARE_VOLUME_PPI_SIGNATURE,
    EFI_PEI_FIRMWARE_VOLUME_PPI_REVISION
  }
};

PEI_FW_VOL_INSTANCE mPeiFfs3FwVol = {
  PEI_FW_VOL_SIGNATURE,
  TRUE,
  {
    PeiFfsFvPpiProcessVolume,
    PeiFfsFvPpiFindFileByType,
    PeiFfsFvPpiFindFileByName,
    PeiFfsFvPpiGetFileInfo,
    PeiFfsFvPpiGetVolumeInfo,
    PeiFfsFvPpiFindSectionByType,
    PeiFfsFvPpiGetFileInfo2,
    PeiFfsFvPpiFindSectionByType2,
    EFI_PEI_FIRMWARE_VOLUME_PPI_SIGNATURE,
    EFI_PEI_FIRMWARE_VOLUME_PPI_REVISION
  }
};
            
EFI_PEI_PPI_DESCRIPTOR  mPeiFfs2FvPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiFirmwareFileSystem2Guid,
  &mPeiFfs2FwVol.Fv
};

EFI_PEI_PPI_DESCRIPTOR  mPeiFfs3FvPpiList = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiFirmwareFileSystem3Guid,
  &mPeiFfs3FwVol.Fv
};

/**
Required Alignment             Alignment Value in FFS         Alignment Value in
(bytes)                        Attributes Field               Firmware Volume Interfaces
1                                    0                                     0
16                                   1                                     4
128                                  2                                     7
512                                  3                                     9
1 KB                                 4                                     10
4 KB                                 5                                     12
32 KB                                6                                     15
64 KB                                7                                     16
**/
UINT8 mFvAttributes[] = {0, 4, 7, 9, 10, 12, 15, 16};

/**
  Convert the FFS File Attributes to FV File Attributes

  @param  FfsAttributes              The attributes of UINT8 type.

  @return The attributes of EFI_FV_FILE_ATTRIBUTES

**/
EFI_FV_FILE_ATTRIBUTES
FfsAttributes2FvFileAttributes (
  IN EFI_FFS_FILE_ATTRIBUTES FfsAttributes
  )
{
  UINT8                     DataAlignment;
  EFI_FV_FILE_ATTRIBUTES    FileAttribute;

  DataAlignment = (UINT8) ((FfsAttributes & FFS_ATTRIB_DATA_ALIGNMENT) >> 3);
  ASSERT (DataAlignment < 8);

  FileAttribute = (EFI_FV_FILE_ATTRIBUTES) mFvAttributes[DataAlignment];

  if ((FfsAttributes & FFS_ATTRIB_FIXED) == FFS_ATTRIB_FIXED) {
    FileAttribute |= EFI_FV_FILE_ATTRIB_FIXED;
  }

  return FileAttribute;
}

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
  EFI_FFS_FILE_HEADER2 TestFileHeader;

  if (IS_FFS_FILE2 (FileHeader)) {
    CopyMem (&TestFileHeader, FileHeader, sizeof (EFI_FFS_FILE_HEADER2));
    //
    // Ingore State and File field in FFS header.
    //
    TestFileHeader.State = 0;
    TestFileHeader.IntegrityCheck.Checksum.File = 0;

    return CalculateSum8 ((CONST UINT8 *) &TestFileHeader, sizeof (EFI_FFS_FILE_HEADER2));
  } else {
    CopyMem (&TestFileHeader, FileHeader, sizeof (EFI_FFS_FILE_HEADER));
    //
    // Ingore State and File field in FFS header.
    //
    TestFileHeader.State = 0;
    TestFileHeader.IntegrityCheck.Checksum.File = 0;

    return CalculateSum8 ((CONST UINT8 *) &TestFileHeader, sizeof (EFI_FFS_FILE_HEADER));
  }
}

/**
  Find FV handler according to FileHandle in that FV.

  @param FileHandle      Handle of file image
  
  @return Pointer to instance of PEI_CORE_FV_HANDLE.
**/
PEI_CORE_FV_HANDLE*
FileHandleToVolume (
  IN   EFI_PEI_FILE_HANDLE          FileHandle
  )
{
  UINTN                       Index;
  PEI_CORE_INSTANCE           *PrivateData;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  UINTN                       BestIndex;

  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer ());
  BestIndex   = PrivateData->FvCount;
  
  //
  // Find the best matched FV image that includes this FileHandle.
  // FV may include the child FV, and they are in the same continuous space. 
  // If FileHandle is from the child FV, the updated logic can find its matched FV.
  //
  for (Index = 0; Index < PrivateData->FvCount; Index++) {
    FwVolHeader = PrivateData->Fv[Index].FvHeader;
    if (((UINT64) (UINTN) FileHandle > (UINT64) (UINTN) FwVolHeader ) &&   \
        ((UINT64) (UINTN) FileHandle <= ((UINT64) (UINTN) FwVolHeader + FwVolHeader->FvLength - 1))) {
      if (BestIndex == PrivateData->FvCount) {
        BestIndex = Index;
      } else {
        if ((UINT64) (UINTN) PrivateData->Fv[BestIndex].FvHeader < (UINT64) (UINTN) FwVolHeader) {
          BestIndex = Index;
        }
      }
    }
  }

  if (BestIndex < PrivateData->FvCount) {
    return &PrivateData->Fv[BestIndex];
  }

  return NULL;
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
FindFileEx (
  IN  CONST EFI_PEI_FV_HANDLE        FvHandle,
  IN  CONST EFI_GUID                 *FileName,   OPTIONAL
  IN        EFI_FV_FILETYPE          SearchType,
  IN OUT    EFI_PEI_FILE_HANDLE      *FileHandle,
  IN OUT    EFI_PEI_FILE_HANDLE      *AprioriFile  OPTIONAL
  )
{
  EFI_FIRMWARE_VOLUME_HEADER            *FwVolHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER        *FwVolExtHeader;
  EFI_FFS_FILE_HEADER                   **FileHeader;
  EFI_FFS_FILE_HEADER                   *FfsFileHeader;
  UINT32                                FileLength;
  UINT32                                FileOccupiedSize;
  UINT32                                FileOffset;
  UINT64                                FvLength;
  UINT8                                 ErasePolarity;
  UINT8                                 FileState;
  UINT8                                 DataCheckSum;
  BOOLEAN                               IsFfs3Fv;
  
  //
  // Convert the handle of FV to FV header for memory-mapped firmware volume
  //
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *) FvHandle;
  FileHeader  = (EFI_FFS_FILE_HEADER **)FileHandle;

  IsFfs3Fv = CompareGuid (&FwVolHeader->FileSystemGuid, &gEfiFirmwareFileSystem3Guid);

  FvLength = FwVolHeader->FvLength;
  if ((FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) != 0) {
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
    if (FwVolHeader->ExtHeaderOffset != 0) {
      //
      // Searching for files starts on an 8 byte aligned boundary after the end of the Extended Header if it exists.
      //
      FwVolExtHeader = (EFI_FIRMWARE_VOLUME_EXT_HEADER *) ((UINT8 *) FwVolHeader + FwVolHeader->ExtHeaderOffset);
      FfsFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FwVolExtHeader + FwVolExtHeader->ExtHeaderSize);
      FfsFileHeader = (EFI_FFS_FILE_HEADER *) ALIGN_POINTER (FfsFileHeader, 8);
    } else {
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *) FwVolHeader + FwVolHeader->HeaderLength);
    }
  } else {
    if (IS_FFS_FILE2 (*FileHeader)) {
      if (!IsFfs3Fv) {
        DEBUG ((EFI_D_ERROR, "It is a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &(*FileHeader)->Name));
      }
      FileLength = FFS_FILE2_SIZE (*FileHeader);
      ASSERT (FileLength > 0x00FFFFFF);
    } else {
      FileLength = FFS_FILE_SIZE (*FileHeader);
    }
    //
    // FileLength is adjusted to FileOccupiedSize as it is 8 byte aligned.
    //
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

    case EFI_FILE_HEADER_CONSTRUCTION:
    case EFI_FILE_HEADER_INVALID:
      if (IS_FFS_FILE2 (FfsFileHeader)) {
        if (!IsFfs3Fv) {
          DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FfsFileHeader->Name));
        }
        FileOffset    += sizeof (EFI_FFS_FILE_HEADER2);
        FfsFileHeader =  (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
      } else {
        FileOffset    += sizeof (EFI_FFS_FILE_HEADER);
        FfsFileHeader =  (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
      }
      break;
        
    case EFI_FILE_DATA_VALID:
    case EFI_FILE_MARKED_FOR_UPDATE:
      if (CalculateHeaderChecksum (FfsFileHeader) != 0) {
        ASSERT (FALSE);
        *FileHeader = NULL;
        return EFI_NOT_FOUND;
      }

      if (IS_FFS_FILE2 (FfsFileHeader)) {
        FileLength = FFS_FILE2_SIZE (FfsFileHeader);
        ASSERT (FileLength > 0x00FFFFFF);
        FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
        if (!IsFfs3Fv) {
          DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FfsFileHeader->Name));
          FileOffset += FileOccupiedSize;
          FfsFileHeader = (EFI_FFS_FILE_HEADER *) ((UINT8 *) FfsFileHeader + FileOccupiedSize);
          break;
        }
      } else {
        FileLength = FFS_FILE_SIZE (FfsFileHeader);
        FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
      }

      DataCheckSum = FFS_FIXED_CHECKSUM;
      if ((FfsFileHeader->Attributes & FFS_ATTRIB_CHECKSUM) == FFS_ATTRIB_CHECKSUM) {
        if (IS_FFS_FILE2 (FfsFileHeader)) {
          DataCheckSum = CalculateCheckSum8 ((CONST UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2), FileLength - sizeof(EFI_FFS_FILE_HEADER2));
        } else {
          DataCheckSum = CalculateCheckSum8 ((CONST UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER), FileLength - sizeof(EFI_FFS_FILE_HEADER));
        }
      }
      if (FfsFileHeader->IntegrityCheck.Checksum.File != DataCheckSum) {
        ASSERT (FALSE);
        *FileHeader = NULL;
        return EFI_NOT_FOUND;
      }

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
      if (IS_FFS_FILE2 (FfsFileHeader)) {
        if (!IsFfs3Fv) {
          DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FfsFileHeader->Name));
        }
        FileLength = FFS_FILE2_SIZE (FfsFileHeader);
        ASSERT (FileLength > 0x00FFFFFF);
      } else {
        FileLength = FFS_FILE_SIZE (FfsFileHeader);
      }
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
  EFI_STATUS                    Status;
  EFI_PEI_FIRMWARE_VOLUME_PPI   *FvPpi;
  EFI_PEI_FV_HANDLE             FvHandle;
  EFI_FIRMWARE_VOLUME_HEADER    *BfvHeader;
  
  //
  // Install FV_PPI for FFS2 file system.
  //
  PeiServicesInstallPpi (&mPeiFfs2FvPpiList);

  //
  // Install FV_PPI for FFS3 file system.
  //
  PeiServicesInstallPpi (&mPeiFfs3FvPpiList);

  BfvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  
  //
  // The FV_PPI in BFV's format should be installed.
  //
  Status = PeiServicesLocatePpi (
             &BfvHeader->FileSystemGuid,
             0,
             NULL,
             (VOID**)&FvPpi
             );
  ASSERT_EFI_ERROR (Status);
    
  //
  // Get handle of BFV
  //
  FvPpi->ProcessVolume (
           FvPpi, 
           SecCoreData->BootFirmwareVolumeBase,
           (UINTN)BfvHeader->FvLength,
           &FvHandle
           );

  //
  // Update internal PEI_CORE_FV array.
  //
  PrivateData->Fv[PrivateData->FvCount].FvHeader = BfvHeader;
  PrivateData->Fv[PrivateData->FvCount].FvPpi    = FvPpi;
  PrivateData->Fv[PrivateData->FvCount].FvHandle = FvHandle;
  PrivateData->Fv[PrivateData->FvCount].AuthenticationStatus = 0;
  DEBUG ((
    EFI_D_INFO, 
    "The %dth FV start address is 0x%11p, size is 0x%08x, handle is 0x%p\n", 
    (UINT32) PrivateData->FvCount, 
    (VOID *) BfvHeader, 
    BfvHeader->FvLength,
    FvHandle
    ));    
  PrivateData->FvCount ++;
                            
  //
  // Post a call-back for the FvInfoPPI and FvInfo2PPI services to expose
  // additional Fvs to PeiCore.
  //
  Status = PeiServicesNotifyPpi (mNotifyOnFvInfoList);
  ASSERT_EFI_ERROR (Status);

}
  
/**
  Process Firmware Volum Information once FvInfoPPI or FvInfo2PPI install.
  The FV Info will be registered into PeiCore private data structure.
  And search the inside FV image, if found, the new FV INFO(2) PPI will be installed.

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
  EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI     FvInfo2Ppi;
  EFI_PEI_FIRMWARE_VOLUME_PPI           *FvPpi;
  PEI_CORE_INSTANCE                     *PrivateData;
  EFI_STATUS                            Status;
  EFI_PEI_FV_HANDLE                     FvHandle;
  UINTN                                 FvIndex;
  EFI_PEI_FILE_HANDLE                   FileHandle;
  VOID                                  *DepexData;
  BOOLEAN                               IsFvInfo2;
  
  Status       = EFI_SUCCESS;
  PrivateData  = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);

  if (CompareGuid (NotifyDescriptor->Guid, &gEfiPeiFirmwareVolumeInfo2PpiGuid)) {
    //
    // It is FvInfo2PPI.
    //
    CopyMem (&FvInfo2Ppi, Ppi, sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI));
    IsFvInfo2 = TRUE;
  } else {
    //
    // It is FvInfoPPI.
    //
    CopyMem (&FvInfo2Ppi, Ppi, sizeof (EFI_PEI_FIRMWARE_VOLUME_INFO_PPI));
    FvInfo2Ppi.AuthenticationStatus = 0;
    IsFvInfo2 = FALSE;
  }

  //
  // Locate the corresponding FV_PPI according to founded FV's format guid
  //
  Status = PeiServicesLocatePpi (
             &FvInfo2Ppi.FvFormat,
             0,
             NULL,
             (VOID**)&FvPpi
             );
  if (!EFI_ERROR (Status)) {
    //
    // Process new found FV and get FV handle.
    //
    Status = FvPpi->ProcessVolume (FvPpi, FvInfo2Ppi.FvInfo, FvInfo2Ppi.FvInfoSize, &FvHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Fail to process new found FV, FV may be corrupted!\n"));
      return Status;
    }

    //
    // Check whether the FV has already been processed.
    //
    for (FvIndex = 0; FvIndex < PrivateData->FvCount; FvIndex ++) {
      if (PrivateData->Fv[FvIndex].FvHandle == FvHandle) {
        if (IsFvInfo2 && (FvInfo2Ppi.AuthenticationStatus != PrivateData->Fv[FvIndex].AuthenticationStatus)) {
          PrivateData->Fv[FvIndex].AuthenticationStatus = FvInfo2Ppi.AuthenticationStatus;
          DEBUG ((EFI_D_INFO, "Update AuthenticationStatus of the %dth FV to 0x%x!\n", FvIndex, FvInfo2Ppi.AuthenticationStatus));          
        }
        DEBUG ((EFI_D_INFO, "The Fv %p has already been processed!\n", FvInfo2Ppi.FvInfo));
        return EFI_SUCCESS;
      }
    }

    if (PrivateData->FvCount >= PcdGet32 (PcdPeiCoreMaxFvSupported)) {
      DEBUG ((EFI_D_ERROR, "The number of Fv Images (%d) exceed the max supported FVs (%d) in Pei", PrivateData->FvCount + 1, PcdGet32 (PcdPeiCoreMaxFvSupported)));
      DEBUG ((EFI_D_ERROR, "PcdPeiCoreMaxFvSupported value need be reconfigurated in DSC"));
      ASSERT (FALSE);
    }

    //
    // Update internal PEI_CORE_FV array.
    //
    PrivateData->Fv[PrivateData->FvCount].FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*) FvInfo2Ppi.FvInfo;
    PrivateData->Fv[PrivateData->FvCount].FvPpi    = FvPpi;
    PrivateData->Fv[PrivateData->FvCount].FvHandle = FvHandle;
    PrivateData->Fv[PrivateData->FvCount].AuthenticationStatus = FvInfo2Ppi.AuthenticationStatus;
    DEBUG ((
      EFI_D_INFO, 
      "The %dth FV start address is 0x%11p, size is 0x%08x, handle is 0x%p\n", 
      (UINT32) PrivateData->FvCount, 
      (VOID *) FvInfo2Ppi.FvInfo, 
      FvInfo2Ppi.FvInfoSize,
      FvHandle
      ));    
    PrivateData->FvCount ++;

    //
    // Scan and process the new discoveried FV for EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 
    //
    FileHandle = NULL;
    do {
      Status = FvPpi->FindFileByType (
                        FvPpi,
                        EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
                        FvHandle,
                        &FileHandle
                       );
      if (!EFI_ERROR (Status)) {
        Status = FvPpi->FindSectionByType (
                          FvPpi,
                          EFI_SECTION_PEI_DEPEX,
                          FileHandle,
                          (VOID**)&DepexData
                          );
        if (!EFI_ERROR (Status)) {
          if (!PeimDispatchReadiness (PeiServices, DepexData)) {
            //
            // Dependency is not satisfied.
            //
            continue;
          }
        }
        
        DEBUG ((EFI_D_INFO, "Found firmware volume Image File %p in FV[%d] %p\n", FileHandle, PrivateData->FvCount - 1, FvHandle));
        ProcessFvFile (PrivateData, &PrivateData->Fv[PrivateData->FvCount - 1], FileHandle);
      }
    } while (FileHandle != NULL);
  } else {
    DEBUG ((EFI_D_ERROR, "Fail to process FV %p because no corresponding EFI_FIRMWARE_VOLUME_PPI is found!\n", FvInfo2Ppi.FvInfo));
    
    AddUnknownFormatFvInfo (PrivateData, &FvInfo2Ppi);
  }
  
  return EFI_SUCCESS;
}

/**
  Verify the Guided Section GUID by checking if there is the Guided Section GUID HOB recorded the GUID itself.

  @param GuidedSectionGuid          The Guided Section GUID.
  @param GuidedSectionExtraction    A pointer to the pointer to the supported Guided Section Extraction Ppi
                                    for the Guided Section.

  @return TRUE      The GuidedSectionGuid could be identified, and the pointer to
                    the Guided Section Extraction Ppi will be returned to *GuidedSectionExtraction.
  @return FALSE     The GuidedSectionGuid could not be identified, or 
                    the Guided Section Extraction Ppi has not been installed yet.

**/
BOOLEAN
VerifyGuidedSectionGuid (
  IN  EFI_GUID                                  *GuidedSectionGuid,
  OUT EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI     **GuidedSectionExtraction
  )
{
  EFI_PEI_HOB_POINTERS  Hob;
  EFI_GUID              *GuidRecorded;
  VOID                  *Interface;
  EFI_STATUS            Status;

  //
  // Check if there is the Guided Section GUID HOB recorded the GUID itself.
  //
  Hob.Raw = GetFirstGuidHob (GuidedSectionGuid);
  if (Hob.Raw != NULL) {
    GuidRecorded = (EFI_GUID *) GET_GUID_HOB_DATA (Hob);
    if (CompareGuid (GuidRecorded, GuidedSectionGuid)) {
      //
      // Found the recorded GuidedSectionGuid.
      //
      Status = PeiServicesLocatePpi (GuidedSectionGuid, 0, NULL, (VOID **) &Interface);
      if (!EFI_ERROR (Status) && Interface != NULL) {
        //
        // Found the supported Guided Section Extraction Ppi for the Guided Section.
        //
        *GuidedSectionExtraction = (EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *) Interface;
        return TRUE;
      }
      return FALSE;
    }
  }

  return FALSE;
}

/**
  Go through the file to search SectionType section. 
  Search within encapsulation sections (compression and GUIDed) recursively, 
  until the match section is found.
  
  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SectionType       Filter to find only section of this type.
  @param SectionInstance   Pointer to the filter to find the specific instance of section.
  @param Section           From where to search.
  @param SectionSize       The file size to search.
  @param OutputBuffer      A pointer to the discovered section, if successful.
                           NULL if section not found
  @param AuthenticationStatus Updated upon return to point to the authentication status for this section.
  @param IsFfs3Fv          Indicates the FV format.

  @return EFI_NOT_FOUND    The match section is not found.
  @return EFI_SUCCESS      The match section is found.

**/
EFI_STATUS
ProcessSection (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_SECTION_TYPE           SectionType,
  IN OUT UINTN                  *SectionInstance,
  IN EFI_COMMON_SECTION_HEADER  *Section,
  IN UINTN                      SectionSize,
  OUT VOID                      **OutputBuffer,
  OUT UINT32                    *AuthenticationStatus,
  IN BOOLEAN                    IsFfs3Fv
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
  EFI_GUID                                *SectionDefinitionGuid;
  BOOLEAN                                 SectionCached;
  VOID                                    *TempOutputBuffer;
  UINT32                                  TempAuthenticationStatus;
  UINT16                                  GuidedSectionAttributes;

  PrivateData   = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  *OutputBuffer = NULL;
  ParsedLength  = 0;
  Index         = 0;
  Status        = EFI_NOT_FOUND;
  PpiOutput     = NULL;
  PpiOutputSize = 0;
  while (ParsedLength < SectionSize) {

    if (IS_SECTION2 (Section)) {
      ASSERT (SECTION2_SIZE (Section) > 0x00FFFFFF);
      if (!IsFfs3Fv) {
        DEBUG ((EFI_D_ERROR, "Found a FFS3 formatted section in a non-FFS3 formatted FV.\n"));
        SectionLength = SECTION2_SIZE (Section);
        //
        // SectionLength is adjusted it is 4 byte aligned.
        // Go to the next section
        //
        SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
        ASSERT (SectionLength != 0);
        ParsedLength += SectionLength;
        Section = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) Section + SectionLength);
        continue;
      }
    }

    if (Section->Type == SectionType) {
      //
      // The type matches, so check the instance count to see if it's the one we want.
      //
      (*SectionInstance)--;
      if (*SectionInstance == 0) {
        //
        // Got it!
        //
        if (IS_SECTION2 (Section)) {
          *OutputBuffer = (VOID *)((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER2));
        } else {
          *OutputBuffer = (VOID *)((UINT8 *) Section + sizeof (EFI_COMMON_SECTION_HEADER));
        }
        return EFI_SUCCESS;
      } else {
        if (IS_SECTION2 (Section)) {
          SectionLength = SECTION2_SIZE (Section);
        } else {
          SectionLength = SECTION_SIZE (Section);
        }
        //
        // SectionLength is adjusted it is 4 byte aligned.
        // Go to the next section
        //
        SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
        ASSERT (SectionLength != 0);
        ParsedLength += SectionLength;
        Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
        continue;
      }
    } else if ((Section->Type == EFI_SECTION_GUID_DEFINED) || (Section->Type == EFI_SECTION_COMPRESSION)) {
      //
      // Check the encapsulated section is extracted into the cache data.
      //
      SectionCached = FALSE;
      for (Index = 0; Index < PrivateData->CacheSection.AllSectionCount; Index ++) {
        if (Section == PrivateData->CacheSection.Section[Index]) {
          SectionCached = TRUE;
          PpiOutput     = PrivateData->CacheSection.SectionData[Index];
          PpiOutputSize = PrivateData->CacheSection.SectionSize[Index];
          Authentication = PrivateData->CacheSection.AuthenticationStatus[Index];
          //
          // Search section directly from the cache data.
          //
          TempAuthenticationStatus = 0;
          Status = ProcessSection (
                     PeiServices,
                     SectionType,
                     SectionInstance,
                     PpiOutput, 
                     PpiOutputSize, 
                     &TempOutputBuffer,
                     &TempAuthenticationStatus,
                     IsFfs3Fv
                   );
          if (!EFI_ERROR (Status)) {
            *OutputBuffer = TempOutputBuffer;
            *AuthenticationStatus = TempAuthenticationStatus | Authentication;
            return EFI_SUCCESS;
          }
        }
      }

      //
      // If SectionCached is TRUE, the section data has been cached and scanned.
      //
      if (!SectionCached) {
        Status = EFI_NOT_FOUND;
        Authentication = 0;
        if (Section->Type == EFI_SECTION_GUID_DEFINED) {
          if (IS_SECTION2 (Section)) {
            SectionDefinitionGuid   = &((EFI_GUID_DEFINED_SECTION2 *)Section)->SectionDefinitionGuid;
            GuidedSectionAttributes = ((EFI_GUID_DEFINED_SECTION2 *)Section)->Attributes;
          } else {
            SectionDefinitionGuid   = &((EFI_GUID_DEFINED_SECTION *)Section)->SectionDefinitionGuid;
            GuidedSectionAttributes = ((EFI_GUID_DEFINED_SECTION *)Section)->Attributes;
          }
          if (VerifyGuidedSectionGuid (SectionDefinitionGuid, &GuidSectionPpi)) {
            Status = GuidSectionPpi->ExtractSection (
                                       GuidSectionPpi,
                                       Section,
                                       &PpiOutput,
                                       &PpiOutputSize,
                                       &Authentication
                                       );
          } else if ((GuidedSectionAttributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
            //
            // Figure out the proper authentication status for GUIDED section without processing required
            //
            Status = EFI_SUCCESS;
            if ((GuidedSectionAttributes & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) == EFI_GUIDED_SECTION_AUTH_STATUS_VALID) {
              Authentication |= EFI_AUTH_STATUS_IMAGE_SIGNED | EFI_AUTH_STATUS_NOT_TESTED;
            }
            if (IS_SECTION2 (Section)) {
              PpiOutputSize = SECTION2_SIZE (Section) - ((EFI_GUID_DEFINED_SECTION2 *) Section)->DataOffset;
              PpiOutput     = (UINT8 *) Section + ((EFI_GUID_DEFINED_SECTION2 *) Section)->DataOffset;
            } else {
              PpiOutputSize = SECTION_SIZE (Section) - ((EFI_GUID_DEFINED_SECTION *) Section)->DataOffset;
              PpiOutput     = (UINT8 *) Section + ((EFI_GUID_DEFINED_SECTION *) Section)->DataOffset;
            }
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
          PrivateData->CacheSection.AuthenticationStatus [PrivateData->CacheSection.SectionIndex] = Authentication;
          PrivateData->CacheSection.SectionIndex = (PrivateData->CacheSection.SectionIndex + 1)%CACHE_SETION_MAX_NUMBER;

          TempAuthenticationStatus = 0;
          Status = ProcessSection (
                     PeiServices,
                     SectionType,
                     SectionInstance,
                     PpiOutput, 
                     PpiOutputSize, 
                     &TempOutputBuffer,
                     &TempAuthenticationStatus,
                     IsFfs3Fv
                   );
          if (!EFI_ERROR (Status)) {
            *OutputBuffer = TempOutputBuffer;
            *AuthenticationStatus = TempAuthenticationStatus | Authentication;
            return EFI_SUCCESS;
          }
        }
      }
    }

    if (IS_SECTION2 (Section)) {
      SectionLength = SECTION2_SIZE (Section);
    } else {
      SectionLength = SECTION_SIZE (Section);
    }
    //
    // SectionLength is adjusted it is 4 byte aligned.
    // Go to the next section
    //
    SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);
    ASSERT (SectionLength != 0);
    ParsedLength += SectionLength;
    Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
  }
  
  return EFI_NOT_FOUND;
}


/**
  Searches for the next matching section within the specified file.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param SectionType     Filter to find only sections of this type.
  @param FileHandle      Pointer to the current file to search.
  @param SectionData     A pointer to the discovered section, if successful.
                         NULL if section not found

  @retval EFI_NOT_FOUND  The section was not found.
  @retval EFI_SUCCESS    The section was found.

**/
EFI_STATUS
EFIAPI
PeiFfsFindSectionData (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     EFI_PEI_FILE_HANDLE   FileHandle,
  OUT VOID                     **SectionData
  )
{
  PEI_CORE_FV_HANDLE           *CoreFvHandle;
  
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if ((CoreFvHandle == NULL) || (CoreFvHandle->FvPpi == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  return CoreFvHandle->FvPpi->FindSectionByType (CoreFvHandle->FvPpi, SectionType, FileHandle, SectionData);
}

/**
  Searches for the next matching section within the specified file.

  @param  PeiServices           An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  SectionType           The value of the section type to find.
  @param  SectionInstance       Section instance to find.
  @param  FileHandle            Handle of the firmware file to search.
  @param  SectionData           A pointer to the discovered section, if successful.
  @param  AuthenticationStatus  A pointer to the authentication status for this section.

  @retval EFI_SUCCESS      The section was found.
  @retval EFI_NOT_FOUND    The section was not found.

**/
EFI_STATUS
EFIAPI
PeiFfsFindSectionData3 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     UINTN                 SectionInstance,
  IN     EFI_PEI_FILE_HANDLE   FileHandle,
  OUT VOID                     **SectionData,
  OUT UINT32                   *AuthenticationStatus
  )
{
  PEI_CORE_FV_HANDLE           *CoreFvHandle;
  
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if ((CoreFvHandle == NULL) || (CoreFvHandle->FvPpi == NULL)) {
    return EFI_NOT_FOUND;
  }

  if ((CoreFvHandle->FvPpi->Signature == EFI_PEI_FIRMWARE_VOLUME_PPI_SIGNATURE) &&
      (CoreFvHandle->FvPpi->Revision == EFI_PEI_FIRMWARE_VOLUME_PPI_REVISION)) {
    return CoreFvHandle->FvPpi->FindSectionByType2 (CoreFvHandle->FvPpi, SectionType, SectionInstance, FileHandle, SectionData, AuthenticationStatus);
  }
  //
  // The old FvPpi doesn't support to find section by section instance
  // and return authentication status, so return EFI_UNSUPPORTED.
  //
  return EFI_UNSUPPORTED;
}

/**
  Searches for the next matching file in the firmware volume.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FvHandle        Handle of firmware volume in which to search.
  @param FileHandle      On entry, points to the current handle from which to begin searching or NULL to start
                         at the beginning of the firmware volume. On exit, points the file handle of the next file
                         in the volume or NULL if there are no more files.

  @retval EFI_NOT_FOUND  The file was not found.
  @retval EFI_NOT_FOUND  The header checksum was not zero.
  @retval EFI_SUCCESS    The file was found.

**/
EFI_STATUS
EFIAPI
PeiFfsFindNextFile (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT8                       SearchType,
  IN EFI_PEI_FV_HANDLE           FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE     *FileHandle
  )
{
  PEI_CORE_FV_HANDLE      *CoreFvHandle;
  
  CoreFvHandle = FvHandleToCoreHandle (FvHandle);
  
  //
  // To make backward compatiblity, if can not find corresponding the handle of FV
  // then treat FV as build-in FFS2/FFS3 format and memory mapped FV that FV handle is pointed
  // to the address of first byte of FV.
  //
  if ((CoreFvHandle == NULL) && FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
    return FindFileEx (FvHandle, NULL, SearchType, FileHandle, NULL);
  } 
  
  if ((CoreFvHandle == NULL) || CoreFvHandle->FvPpi == NULL) {
    return EFI_NOT_FOUND;
  }
  
  return CoreFvHandle->FvPpi->FindFileByType (CoreFvHandle->FvPpi, SearchType, FvHandle, FileHandle);
}


/**
  Search the firmware volumes by index

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param Instance        This instance of the firmware volume to find. The value 0 is the Boot Firmware
                         Volume (BFV).
  @param VolumeHandle    On exit, points to the next volume handle or NULL if it does not exist.

  @retval EFI_INVALID_PARAMETER  VolumeHandle is NULL
  @retval EFI_NOT_FOUND          The volume was not found.
  @retval EFI_SUCCESS            The volume was found.

**/
EFI_STATUS 
EFIAPI
PeiFfsFindNextVolume (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN     UINTN                      Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  )
{
  PEI_CORE_INSTANCE  *Private;
  PEI_CORE_FV_HANDLE *CoreFvHandle;
  
  if (VolumeHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  Private = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  
  CoreFvHandle = FindNextCoreFvHandle (Private, Instance);
  if (CoreFvHandle == NULL) {
    *VolumeHandle = NULL;
    return EFI_NOT_FOUND;
  }
  
  *VolumeHandle = CoreFvHandle->FvHandle;
  
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
  PEI_CORE_FV_HANDLE            *CoreFvHandle;
  
  if ((VolumeHandle == NULL) || (FileName == NULL) || (FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  CoreFvHandle = FvHandleToCoreHandle (VolumeHandle);
  if ((CoreFvHandle == NULL) || (CoreFvHandle->FvPpi == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  return CoreFvHandle->FvPpi->FindFileByName (CoreFvHandle->FvPpi, FileName, &VolumeHandle, FileHandle);
}

/**
  Returns information about a specific file.

  @param FileHandle       Handle of the file.
  @param FileInfo         Upon exit, points to the file's information.

  @retval EFI_INVALID_PARAMETER If FileInfo is NULL.
  @retval EFI_INVALID_PARAMETER If FileHandle does not represent a valid file.
  @retval EFI_SUCCESS           File information returned.

**/
EFI_STATUS
EFIAPI 
PeiFfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
  )
{
  PEI_CORE_FV_HANDLE          *CoreFvHandle;
  
  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if ((CoreFvHandle == NULL) || (CoreFvHandle->FvPpi == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return CoreFvHandle->FvPpi->GetFileInfo (CoreFvHandle->FvPpi, FileHandle, FileInfo);
}

/**
  Returns information about a specific file.

  @param FileHandle       Handle of the file.
  @param FileInfo         Upon exit, points to the file's information.

  @retval EFI_INVALID_PARAMETER If FileInfo is NULL.
  @retval EFI_INVALID_PARAMETER If FileHandle does not represent a valid file.
  @retval EFI_SUCCESS           File information returned.

**/
EFI_STATUS
EFIAPI 
PeiFfsGetFileInfo2 (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO2   *FileInfo
  )
{
  PEI_CORE_FV_HANDLE          *CoreFvHandle;
  
  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if ((CoreFvHandle == NULL) || (CoreFvHandle->FvPpi == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((CoreFvHandle->FvPpi->Signature == EFI_PEI_FIRMWARE_VOLUME_PPI_SIGNATURE) &&
      (CoreFvHandle->FvPpi->Revision == EFI_PEI_FIRMWARE_VOLUME_PPI_REVISION)) {
    return CoreFvHandle->FvPpi->GetFileInfo2 (CoreFvHandle->FvPpi, FileHandle, FileInfo);
  }
  //
  // The old FvPpi doesn't support to return file info with authentication status,
  // so return EFI_UNSUPPORTED.
  //
  return EFI_UNSUPPORTED;
}

/**
  Returns information about the specified volume.

  This function returns information about a specific firmware
  volume, including its name, type, attributes, starting address
  and size.

  @param VolumeHandle   Handle of the volume.
  @param VolumeInfo     Upon exit, points to the volume's information.

  @retval EFI_SUCCESS             Volume information returned.
  @retval EFI_INVALID_PARAMETER   If VolumeHandle does not represent a valid volume.
  @retval EFI_INVALID_PARAMETER   If VolumeHandle is NULL.
  @retval EFI_SUCCESS             Information successfully returned.
  @retval EFI_INVALID_PARAMETER   The volume designated by the VolumeHandle is not available.

**/
EFI_STATUS
EFIAPI 
PeiFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
{
  PEI_CORE_FV_HANDLE                     *CoreHandle;
  
  if ((VolumeInfo == NULL) || (VolumeHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  CoreHandle = FvHandleToCoreHandle (VolumeHandle);
  
  if ((CoreHandle == NULL) || (CoreHandle->FvPpi == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  return CoreHandle->FvPpi->GetVolumeInfo (CoreHandle->FvPpi, VolumeHandle, VolumeInfo);
}

/**
  Get Fv image from the FV type file, then install FV INFO(2) ppi, Build FV hob.

  @param PrivateData          PeiCore's private data structure
  @param ParentFvCoreHandle   Pointer of EFI_CORE_FV_HANDLE to parent Fv image that contain this Fv image.
  @param ParentFvFileHandle   File handle of a Fv type file that contain this Fv image.

  @retval EFI_NOT_FOUND         FV image can't be found.
  @retval EFI_SUCCESS           Successfully to process it.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate page when aligning FV image
  @retval EFI_SECURITY_VIOLATION Image is illegal
  @retval Others                Can not find EFI_SECTION_FIRMWARE_VOLUME_IMAGE section
  
**/
EFI_STATUS
ProcessFvFile (
  IN  PEI_CORE_INSTANCE           *PrivateData,
  IN  PEI_CORE_FV_HANDLE          *ParentFvCoreHandle,
  IN  EFI_PEI_FILE_HANDLE         ParentFvFileHandle
  )
{
  EFI_STATUS                    Status;
  EFI_FV_INFO                   ParentFvImageInfo;
  UINT32                        FvAlignment;
  VOID                          *NewFvBuffer;
  EFI_PEI_HOB_POINTERS          HobPtr;
  EFI_PEI_FIRMWARE_VOLUME_PPI   *ParentFvPpi;
  EFI_PEI_FV_HANDLE             ParentFvHandle;
  EFI_FIRMWARE_VOLUME_HEADER    *FvHeader;
  EFI_FV_FILE_INFO              FileInfo;
  UINT64                        FvLength;
  UINT32                        AuthenticationStatus;
  
  //
  // Check if this EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE file has already
  // been extracted.
  //
  HobPtr.Raw = GetHobList ();
  while ((HobPtr.Raw = GetNextHob (EFI_HOB_TYPE_FV2, HobPtr.Raw)) != NULL) {
    if (CompareGuid (&(((EFI_FFS_FILE_HEADER *)ParentFvFileHandle)->Name), &HobPtr.FirmwareVolume2->FileName)) {
      //
      // this FILE has been dispatched, it will not be dispatched again.
      //
      DEBUG ((EFI_D_INFO, "FV file %p has been dispatched!\r\n", ParentFvFileHandle));
      return EFI_SUCCESS;
    }
    HobPtr.Raw = GET_NEXT_HOB (HobPtr);
  }

  ParentFvHandle = ParentFvCoreHandle->FvHandle;
  ParentFvPpi    = ParentFvCoreHandle->FvPpi;
  
  //
  // Find FvImage in FvFile
  //
  AuthenticationStatus = 0;
  if ((ParentFvPpi->Signature == EFI_PEI_FIRMWARE_VOLUME_PPI_SIGNATURE) &&
      (ParentFvPpi->Revision == EFI_PEI_FIRMWARE_VOLUME_PPI_REVISION)) {
    Status = ParentFvPpi->FindSectionByType2 (
                            ParentFvPpi,
                            EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
                            0,
                            ParentFvFileHandle,
                            (VOID **)&FvHeader,
                            &AuthenticationStatus
                            );
  } else {
    Status = ParentFvPpi->FindSectionByType (
                            ParentFvPpi,
                            EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
                            ParentFvFileHandle,
                            (VOID **)&FvHeader
                            );
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = VerifyPeim (PrivateData, ParentFvHandle, ParentFvFileHandle, AuthenticationStatus);
  if (Status == EFI_SECURITY_VIOLATION) {
    return Status;
  }

  //
  // If EFI_FVB2_WEAK_ALIGNMENT is set in the volume header then the first byte of the volume
  // can be aligned on any power-of-two boundary. A weakly aligned volume can not be moved from
  // its initial linked location and maintain its alignment.
  //
  if ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_WEAK_ALIGNMENT) != EFI_FVB2_WEAK_ALIGNMENT) {
    //
    // FvAlignment must be greater than or equal to 8 bytes of the minimum FFS alignment value.
    //
    FvAlignment = 1 << ((ReadUnaligned32 (&FvHeader->Attributes) & EFI_FVB2_ALIGNMENT) >> 16);
    if (FvAlignment < 8) {
      FvAlignment = 8;
    }

    //
    // Check FvImage
    //
    if ((UINTN) FvHeader % FvAlignment != 0) {
      FvLength    = ReadUnaligned64 (&FvHeader->FvLength);
      NewFvBuffer = AllocateAlignedPages (EFI_SIZE_TO_PAGES ((UINT32) FvLength), FvAlignment);
      if (NewFvBuffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      CopyMem (NewFvBuffer, FvHeader, (UINTN) FvLength);
      FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*) NewFvBuffer;
    }
  }

  Status = ParentFvPpi->GetVolumeInfo (ParentFvPpi, ParentFvHandle, &ParentFvImageInfo);
  ASSERT_EFI_ERROR (Status);
  
  Status = ParentFvPpi->GetFileInfo (ParentFvPpi, ParentFvFileHandle, &FileInfo);
  ASSERT_EFI_ERROR (Status);
  
  //
  // Install FvInfo(2) Ppi
  //
  PeiServicesInstallFvInfoPpi (
    &FvHeader->FileSystemGuid,
    (VOID**) FvHeader,
    (UINT32) FvHeader->FvLength,
    &ParentFvImageInfo.FvName,
    &FileInfo.FileName
    );

  PeiServicesInstallFvInfo2Ppi (
    &FvHeader->FileSystemGuid,
    (VOID**) FvHeader,
    (UINT32) FvHeader->FvLength,
    &ParentFvImageInfo.FvName,
    &FileInfo.FileName,
    AuthenticationStatus
    );

  //
  // Inform the extracted FvImage to Fv HOB consumer phase, i.e. DXE phase
  //
  BuildFvHob (
    (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader,
    FvHeader->FvLength
    );

  //
  // Makes the encapsulated volume show up in DXE phase to skip processing of
  // encapsulated file again.
  //
  BuildFv2Hob (
    (EFI_PHYSICAL_ADDRESS) (UINTN) FvHeader,
    FvHeader->FvLength,
    &ParentFvImageInfo.FvName,
    &FileInfo.FileName
    );

  return EFI_SUCCESS;
}

/**
  Process a firmware volume and create a volume handle.

  Create a volume handle from the information in the buffer. For
  memory-mapped firmware volumes, Buffer and BufferSize refer to
  the start of the firmware volume and the firmware volume size.
  For non memory-mapped firmware volumes, this points to a
  buffer which contains the necessary information for creating
  the firmware volume handle. Normally, these values are derived
  from the EFI_FIRMWARE_VOLUME_INFO_PPI.
  
  
  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param Buffer                 Points to the start of the buffer.
  @param BufferSize             Size of the buffer.
  @param FvHandle               Points to the returned firmware volume
                                handle. The firmware volume handle must
                                be unique within the system. 

  @retval EFI_SUCCESS           Firmware volume handle created.
  @retval EFI_VOLUME_CORRUPTED  Volume was corrupt.

**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiProcessVolume (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN  VOID                               *Buffer,
  IN  UINTN                              BufferSize,
  OUT EFI_PEI_FV_HANDLE                  *FvHandle
  )
{
  EFI_STATUS          Status;
  
  ASSERT (FvHandle != NULL);
  
  if (Buffer == NULL) {
    return EFI_VOLUME_CORRUPTED;
  }
  
  //
  // The build-in EFI_PEI_FIRMWARE_VOLUME_PPI for FFS2/FFS3 support memory-mapped
  // FV image and the handle is pointed to Fv image's buffer.
  //
  *FvHandle = (EFI_PEI_FV_HANDLE) Buffer;
  
  //
  // Do verify for given FV buffer.
  //
  Status = VerifyFv ((EFI_FIRMWARE_VOLUME_HEADER*) Buffer);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "Fail to verify FV which address is 0x%11p", Buffer));
    return EFI_VOLUME_CORRUPTED;
  }

  return EFI_SUCCESS;
}  

/**
  Finds the next file of the specified type.

  This service enables PEI modules to discover additional firmware files. 
  The FileHandle must be unique within the system.

  @param This           Points to this instance of the
                        EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType     A filter to find only files of this type. Type
                        EFI_FV_FILETYPE_ALL causes no filtering to be
                        done.
  @param FvHandle       Handle of firmware volume in which to
                        search.
  @param FileHandle     Points to the current handle from which to
                        begin searching or NULL to start at the
                        beginning of the firmware volume. Updated
                        upon return to reflect the file found.

  @retval EFI_SUCCESS   The file was found.
  @retval EFI_NOT_FOUND The file was not found. FileHandle contains NULL.

**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindFileByType (
  IN CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN        EFI_FV_FILETYPE             SearchType,
  IN        EFI_PEI_FV_HANDLE           FvHandle,
  IN OUT    EFI_PEI_FILE_HANDLE         *FileHandle
  )
{ 
  return FindFileEx (FvHandle, NULL, SearchType, FileHandle, NULL);
}

/**
  Find a file within a volume by its name. 
  
  This service searches for files with a specific name, within
  either the specified firmware volume or all firmware volumes.

  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileName               A pointer to the name of the file to find
                                within the firmware volume.
  @param FvHandle               Upon entry, the pointer to the firmware
                                volume to search or NULL if all firmware
                                volumes should be searched. Upon exit, the
                                actual firmware volume in which the file was
                                found.
  @param FileHandle             Upon exit, points to the found file's
                                handle or NULL if it could not be found.

  @retval EFI_SUCCESS           File was found.
  @retval EFI_NOT_FOUND         File was not found.
  @retval EFI_INVALID_PARAMETER FvHandle or FileHandle or
                                FileName was NULL.


**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindFileByName (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI *This,
  IN  CONST  EFI_GUID                    *FileName,
  IN  EFI_PEI_FV_HANDLE                  *FvHandle,
  OUT EFI_PEI_FILE_HANDLE                *FileHandle  
  )
{
  EFI_STATUS        Status;
  PEI_CORE_INSTANCE *PrivateData;
  UINTN             Index;
  
  if ((FvHandle == NULL) || (FileName == NULL) || (FileHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (*FvHandle != NULL) {
    Status = FindFileEx (*FvHandle, FileName, 0, FileHandle, NULL);
    if (Status == EFI_NOT_FOUND) {
      *FileHandle = NULL;
    }
  } else {   
    //
    // If *FvHandle = NULL, so search all FV for given filename
    //
    Status = EFI_NOT_FOUND;
    
    PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer());
    for (Index = 0; Index < PrivateData->FvCount; Index ++) {
      //
      // Only search the FV which is associated with a EFI_PEI_FIRMWARE_VOLUME_PPI instance.
      //
      if (PrivateData->Fv[Index].FvPpi != NULL) {
        Status = FindFileEx (PrivateData->Fv[Index].FvHandle, FileName, 0, FileHandle, NULL);
        if (!EFI_ERROR (Status)) {
          *FvHandle = PrivateData->Fv[Index].FvHandle;
          break;
        }
      }
    }
  }
  
  return Status;  
}  

/**
  Returns information about a specific file.

  This function returns information about a specific
  file, including its file name, type, attributes, starting
  address and size. 
   
  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileHandle               Handle of the file.
  @param FileInfo                 Upon exit, points to the file's
                                  information.

  @retval EFI_SUCCESS             File information returned.
  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.
  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.
  
**/ 
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetFileInfo (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI   *This, 
  IN        EFI_PEI_FILE_HANDLE           FileHandle, 
  OUT       EFI_FV_FILE_INFO              *FileInfo
  )
{
  UINT8                       FileState;
  UINT8                       ErasePolarity;
  EFI_FFS_FILE_HEADER         *FileHeader;
  PEI_CORE_FV_HANDLE          *CoreFvHandle;
  PEI_FW_VOL_INSTANCE         *FwVolInstance;

  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if (CoreFvHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FwVolInstance = PEI_FW_VOL_INSTANCE_FROM_FV_THIS (This);

  if ((CoreFvHandle->FvHeader->Attributes & EFI_FVB2_ERASE_POLARITY) != 0) {
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
  if (IS_FFS_FILE2 (FileHeader)) {
    ASSERT (FFS_FILE2_SIZE (FileHeader) > 0x00FFFFFF);
    if (!FwVolInstance->IsFfs3Fv) {
      DEBUG ((EFI_D_ERROR, "It is a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FileHeader->Name));
      return EFI_INVALID_PARAMETER;
    }
    FileInfo->BufferSize = FFS_FILE2_SIZE (FileHeader) - sizeof (EFI_FFS_FILE_HEADER2);
    FileInfo->Buffer = (UINT8 *) FileHeader + sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    FileInfo->BufferSize = FFS_FILE_SIZE (FileHeader) - sizeof (EFI_FFS_FILE_HEADER);
    FileInfo->Buffer = (UINT8 *) FileHeader + sizeof (EFI_FFS_FILE_HEADER);
  }
  CopyMem (&FileInfo->FileName, &FileHeader->Name, sizeof(EFI_GUID));
  FileInfo->FileType = FileHeader->Type;
  FileInfo->FileAttributes = FfsAttributes2FvFileAttributes (FileHeader->Attributes);
  if ((CoreFvHandle->FvHeader->Attributes & EFI_FVB2_MEMORY_MAPPED) == EFI_FVB2_MEMORY_MAPPED) {
    FileInfo->FileAttributes |= EFI_FV_FILE_ATTRIB_MEMORY_MAPPED;
  }
  return EFI_SUCCESS;
}

/**
  Returns information about a specific file.

  This function returns information about a specific
  file, including its file name, type, attributes, starting
  address, size and authentication status.

  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FileHandle               Handle of the file.
  @param FileInfo                 Upon exit, points to the file's
                                  information.

  @retval EFI_SUCCESS             File information returned.
  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.
  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetFileInfo2 (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI   *This, 
  IN        EFI_PEI_FILE_HANDLE           FileHandle, 
  OUT       EFI_FV_FILE_INFO2             *FileInfo
  )
{
  EFI_STATUS                  Status;
  PEI_CORE_FV_HANDLE          *CoreFvHandle;

  if ((FileHandle == NULL) || (FileInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if (CoreFvHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiFfsFvPpiGetFileInfo (This, FileHandle, (EFI_FV_FILE_INFO *) FileInfo);
  if (!EFI_ERROR (Status)) {
    FileInfo->AuthenticationStatus = CoreFvHandle->AuthenticationStatus;
  }

  return Status;
}

/**
  This function returns information about the firmware volume.
  
  @param This                     Points to this instance of the
                                  EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param FvHandle                 Handle to the firmware handle.
  @param VolumeInfo               Points to the returned firmware volume
                                  information.

  @retval EFI_SUCCESS             Information returned successfully.
  @retval EFI_INVALID_PARAMETER   FvHandle does not indicate a valid
                                  firmware volume or VolumeInfo is NULL.

**/   
EFI_STATUS
EFIAPI
PeiFfsFvPpiGetVolumeInfo (
  IN  CONST  EFI_PEI_FIRMWARE_VOLUME_PPI   *This, 
  IN  EFI_PEI_FV_HANDLE                    FvHandle, 
  OUT EFI_FV_INFO                          *VolumeInfo
  )
{
  EFI_FIRMWARE_VOLUME_HEADER             FwVolHeader;
  EFI_FIRMWARE_VOLUME_EXT_HEADER         *FwVolExHeaderInfo;

  if ((VolumeInfo == NULL) || (FvHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // VolumeHandle may not align at 8 byte, 
  // but FvLength is UINT64 type, which requires FvHeader align at least 8 byte. 
  // So, Copy FvHeader into the local FvHeader structure.
  //
  CopyMem (&FwVolHeader, FvHandle, sizeof (EFI_FIRMWARE_VOLUME_HEADER));

  //
  // Check Fv Image Signature
  //
  if (FwVolHeader.Signature != EFI_FVH_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (VolumeInfo, sizeof (EFI_FV_INFO));
  VolumeInfo->FvAttributes  = FwVolHeader.Attributes;
  VolumeInfo->FvStart       = (VOID *) FvHandle;
  VolumeInfo->FvSize        = FwVolHeader.FvLength;
  CopyMem (&VolumeInfo->FvFormat, &FwVolHeader.FileSystemGuid, sizeof(EFI_GUID));

  if (FwVolHeader.ExtHeaderOffset != 0) {
    FwVolExHeaderInfo = (EFI_FIRMWARE_VOLUME_EXT_HEADER*)(((UINT8 *)FvHandle) + FwVolHeader.ExtHeaderOffset);
    CopyMem (&VolumeInfo->FvName, &FwVolExHeaderInfo->FvName, sizeof(EFI_GUID));
  }
  
  return EFI_SUCCESS;  
}    

/**
  Find the next matching section in the firmware file.
  
  This service enables PEI modules to discover sections
  of a given type within a valid file.
  
  @param This             Points to this instance of the
                          EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType       A filter to find only sections of this
                          type.
  @param FileHandle       Handle of firmware file in which to
                          search.
  @param SectionData      Updated upon return to point to the
                          section found.
  
  @retval EFI_SUCCESS     Section was found.
  @retval EFI_NOT_FOUND   Section of the specified type was not
                          found. SectionData contains NULL.
**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindSectionByType (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI    *This,
  IN        EFI_SECTION_TYPE               SearchType,
  IN        EFI_PEI_FILE_HANDLE            FileHandle,
  OUT VOID                                 **SectionData
  )
{
  UINT32 AuthenticationStatus;
  return PeiFfsFvPpiFindSectionByType2 (This, SearchType, 0, FileHandle, SectionData, &AuthenticationStatus);
}

/**
  Find the next matching section in the firmware file.

  This service enables PEI modules to discover sections
  of a given instance and type within a valid file.

  @param This                   Points to this instance of the
                                EFI_PEI_FIRMWARE_VOLUME_PPI.
  @param SearchType             A filter to find only sections of this
                                type.
  @param SearchInstance         A filter to find the specific instance
                                of sections.
  @param FileHandle             Handle of firmware file in which to
                                search.
  @param SectionData            Updated upon return to point to the
                                section found.
  @param AuthenticationStatus   Updated upon return to point to the
                                authentication status for this section.

  @retval EFI_SUCCESS     Section was found.
  @retval EFI_NOT_FOUND   Section of the specified type was not
                          found. SectionData contains NULL.
**/
EFI_STATUS
EFIAPI
PeiFfsFvPpiFindSectionByType2 (
  IN  CONST EFI_PEI_FIRMWARE_VOLUME_PPI    *This,
  IN        EFI_SECTION_TYPE               SearchType,
  IN        UINTN                          SearchInstance,
  IN        EFI_PEI_FILE_HANDLE            FileHandle,
  OUT VOID                                 **SectionData,
  OUT UINT32                               *AuthenticationStatus
  )
{
  EFI_STATUS                              Status;
  EFI_FFS_FILE_HEADER                     *FfsFileHeader;
  UINT32                                  FileSize;
  EFI_COMMON_SECTION_HEADER               *Section;
  PEI_FW_VOL_INSTANCE                     *FwVolInstance;
  PEI_CORE_FV_HANDLE                      *CoreFvHandle;
  UINTN                                   Instance;
  UINT32                                  ExtractedAuthenticationStatus;

  if (SectionData == NULL) {
    return EFI_NOT_FOUND;
  }

  FwVolInstance = PEI_FW_VOL_INSTANCE_FROM_FV_THIS (This);

  //
  // Retrieve the FirmwareVolume which the file resides in.
  //
  CoreFvHandle = FileHandleToVolume (FileHandle);
  if (CoreFvHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  FfsFileHeader = (EFI_FFS_FILE_HEADER *)(FileHandle);

  if (IS_FFS_FILE2 (FfsFileHeader)) {
    ASSERT (FFS_FILE2_SIZE (FfsFileHeader) > 0x00FFFFFF);
    if (!FwVolInstance->IsFfs3Fv) {
      DEBUG ((EFI_D_ERROR, "It is a FFS3 formatted file: %g in a non-FFS3 formatted FV.\n", &FfsFileHeader->Name));
      return EFI_NOT_FOUND;
    }
    Section = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER2));
    FileSize = FFS_FILE2_SIZE (FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER2);
  } else {
    Section = (EFI_COMMON_SECTION_HEADER *) ((UINT8 *) FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
    FileSize = FFS_FILE_SIZE (FfsFileHeader) - sizeof (EFI_FFS_FILE_HEADER);
  }

  Instance = SearchInstance + 1;
  ExtractedAuthenticationStatus = 0;
  Status = ProcessSection (
             GetPeiServicesTablePointer (),
             SearchType,
             &Instance,
             Section, 
             FileSize, 
             SectionData,
             &ExtractedAuthenticationStatus,
             FwVolInstance->IsFfs3Fv
             );
  if (!EFI_ERROR (Status)) {
    //
    // Inherit the authentication status.
    //
    *AuthenticationStatus = ExtractedAuthenticationStatus | CoreFvHandle->AuthenticationStatus;
  }
  return Status;
}

/**
  Convert the handle of FV to pointer of corresponding PEI_CORE_FV_HANDLE.
  
  @param FvHandle   The handle of a FV.
  
  @retval NULL if can not find.
  @return Pointer of corresponding PEI_CORE_FV_HANDLE. 
**/
PEI_CORE_FV_HANDLE *
FvHandleToCoreHandle (
  IN EFI_PEI_FV_HANDLE  FvHandle
  )
{
  UINTN             Index;
  PEI_CORE_INSTANCE *PrivateData;
  
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS (GetPeiServicesTablePointer());  
  for (Index = 0; Index < PrivateData->FvCount; Index ++) {
    if (FvHandle == PrivateData->Fv[Index].FvHandle) {
      return &PrivateData->Fv[Index];
    }
  }
  
  return NULL;
}  

/**
  Get instance of PEI_CORE_FV_HANDLE for next volume according to given index.
  
  This routine also will install FvInfo ppi for FV hob in PI ways.
  
  @param Private    Pointer of PEI_CORE_INSTANCE
  @param Instance   The index of FV want to be searched.
  
  @return Instance of PEI_CORE_FV_HANDLE.
**/
PEI_CORE_FV_HANDLE *
FindNextCoreFvHandle (
  IN PEI_CORE_INSTANCE  *Private,
  IN UINTN              Instance
  )
{
  UINTN                    Index;
  BOOLEAN                  Match;
  EFI_HOB_FIRMWARE_VOLUME  *FvHob;
  
  //
  // Handle Framework FvHob and Install FvInfo Ppi for it.
  //
  if (FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
    //
    // Loop to search the wanted FirmwareVolume which supports FFS
    //
    FvHob = (EFI_HOB_FIRMWARE_VOLUME *)GetFirstHob (EFI_HOB_TYPE_FV);
    while (FvHob != NULL) {
      //
      // Search whether FvHob has been installed into PeiCore's FV database.
      // If found, no need install new FvInfoPpi for it.
      //
      for (Index = 0, Match = FALSE; Index < Private->FvCount; Index++) {
        if ((EFI_PEI_FV_HANDLE)(UINTN)FvHob->BaseAddress == Private->Fv[Index].FvHeader) {
          Match = TRUE;
          break;
        }
      }
      
      //
      // Search whether FvHob has been cached into PeiCore's Unknown FV database.
      // If found, no need install new FvInfoPpi for it.
      //
      if (!Match) {
        for (Index = 0; Index < Private->UnknownFvInfoCount; Index ++) {
          if ((UINTN)FvHob->BaseAddress == (UINTN)Private->UnknownFvInfo[Index].FvInfo) {
            Match = TRUE;
            break;
          }
        }
      }

      //
      // If the Fv in FvHob has not been installed into PeiCore's FV database and has
      // not been cached into PeiCore's Unknown FV database, install a new FvInfoPpi
      // for it then PeiCore will dispatch it in callback of FvInfoPpi.
      //
      if (!Match) {
        PeiServicesInstallFvInfoPpi (
          &(((EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)FvHob->BaseAddress)->FileSystemGuid),
          (VOID *)(UINTN)FvHob->BaseAddress,
          (UINT32)FvHob->Length,
          NULL,
          NULL
          );
      }
      
      FvHob = (EFI_HOB_FIRMWARE_VOLUME *)GetNextHob (EFI_HOB_TYPE_FV, (VOID *)((UINTN)FvHob + FvHob->Header.HobLength)); 
    }
  }

  ASSERT (Private->FvCount <= PcdGet32 (PcdPeiCoreMaxFvSupported));
  if (Instance >= Private->FvCount) {
    return NULL;
  }

  return &Private->Fv[Instance];
}  

/**
  After PeiCore image is shadowed into permanent memory, all build-in FvPpi should
  be re-installed with the instance in permanent memory and all cached FvPpi pointers in 
  PrivateData->Fv[] array should be fixed up to be pointed to the one in permenant
  memory.
  
  @param PrivateData   Pointer to PEI_CORE_INSTANCE.
**/  
VOID
PeiReinitializeFv (
  IN  PEI_CORE_INSTANCE           *PrivateData
  )
{
  VOID                    *OldFfsFvPpi;
  EFI_PEI_PPI_DESCRIPTOR  *OldDescriptor;
  UINTN                   Index;
  EFI_STATUS              Status;

  //
  // Locate old build-in Ffs2 EFI_PEI_FIRMWARE_VOLUME_PPI which
  // in flash.
  //
  Status = PeiServicesLocatePpi (
            &gEfiFirmwareFileSystem2Guid,
            0,
            &OldDescriptor,
            &OldFfsFvPpi
            );
  ASSERT_EFI_ERROR (Status);

  //
  // Re-install the EFI_PEI_FIRMWARE_VOLUME_PPI for build-in Ffs2
  // which is shadowed from flash to permanent memory within PeiCore image.
  //
  Status = PeiServicesReInstallPpi (OldDescriptor, &mPeiFfs2FvPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Fixup all FvPpi pointers for the implementation in flash to permanent memory.
  //
  for (Index = 0; Index < PcdGet32 (PcdPeiCoreMaxFvSupported); Index ++) {
    if (PrivateData->Fv[Index].FvPpi == OldFfsFvPpi) {
      PrivateData->Fv[Index].FvPpi = &mPeiFfs2FwVol.Fv;
    }
  }

  //
  // Locate old build-in Ffs3 EFI_PEI_FIRMWARE_VOLUME_PPI which
  // in flash.
  //
  Status = PeiServicesLocatePpi (
             &gEfiFirmwareFileSystem3Guid,
             0,
             &OldDescriptor,
             &OldFfsFvPpi
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Re-install the EFI_PEI_FIRMWARE_VOLUME_PPI for build-in Ffs3
  // which is shadowed from flash to permanent memory within PeiCore image.
  //
  Status = PeiServicesReInstallPpi (OldDescriptor, &mPeiFfs3FvPpiList);
  ASSERT_EFI_ERROR (Status);

  //
  // Fixup all FvPpi pointers for the implementation in flash to permanent memory.
  //
  for (Index = 0; Index < PcdGet32 (PcdPeiCoreMaxFvSupported); Index ++) {
    if (PrivateData->Fv[Index].FvPpi == OldFfsFvPpi) {
      PrivateData->Fv[Index].FvPpi = &mPeiFfs3FwVol.Fv;
    }
  }
}

/**
  Report the information for a new discoveried FV in unknown third-party format.
  
  If the EFI_PEI_FIRMWARE_VOLUME_PPI has not been installed for third-party FV format, but
  the FV in this format has been discoveried, then this FV's information will be cached into
  PEI_CORE_INSTANCE's UnknownFvInfo array.
  Also a notification would be installed for unknown third-party FV format guid, if EFI_PEI_FIRMWARE_VOLUME_PPI
  is installed later by platform's PEIM, the original unknown third-party FV will be processed by
  using new installed EFI_PEI_FIRMWARE_VOLUME_PPI.
  
  @param PrivateData  Point to instance of PEI_CORE_INSTANCE
  @param FvInfo2Ppi   Point to FvInfo2 PPI.
  
  @retval EFI_OUT_OF_RESOURCES  The FV info array in PEI_CORE_INSTANCE has no more spaces.
  @retval EFI_SUCCESS           Success to add the information for unknown FV.
**/
EFI_STATUS
AddUnknownFormatFvInfo (
  IN PEI_CORE_INSTANCE                  *PrivateData,
  IN EFI_PEI_FIRMWARE_VOLUME_INFO2_PPI  *FvInfo2Ppi
  )
{
  PEI_CORE_UNKNOW_FORMAT_FV_INFO    *NewUnknownFv;
  
  if (PrivateData->UnknownFvInfoCount + 1 >= PcdGet32 (PcdPeiCoreMaxFvSupported)) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  NewUnknownFv = &PrivateData->UnknownFvInfo[PrivateData->UnknownFvInfoCount];
  PrivateData->UnknownFvInfoCount ++;
  
  CopyGuid (&NewUnknownFv->FvFormat, &FvInfo2Ppi->FvFormat);
  NewUnknownFv->FvInfo     = FvInfo2Ppi->FvInfo;
  NewUnknownFv->FvInfoSize = FvInfo2Ppi->FvInfoSize;
  NewUnknownFv->AuthenticationStatus = FvInfo2Ppi->AuthenticationStatus;
  NewUnknownFv->NotifyDescriptor.Flags  = (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
  NewUnknownFv->NotifyDescriptor.Guid   = &NewUnknownFv->FvFormat;
  NewUnknownFv->NotifyDescriptor.Notify = ThirdPartyFvPpiNotifyCallback;
  
  PeiServicesNotifyPpi (&NewUnknownFv->NotifyDescriptor);
  return EFI_SUCCESS;
}

/**
  Find the FV information according to third-party FV format guid.
  
  This routine also will remove the FV information found by given FV format guid from
  PrivateData->UnknownFvInfo[].
  
  @param PrivateData      Point to instance of PEI_CORE_INSTANCE
  @param Format           Point to given FV format guid
  @param FvInfo           On return, the pointer of FV information buffer
  @param FvInfoSize       On return, the size of FV information buffer.
  @param AuthenticationStatus On return, the authentication status of FV information buffer.
  
  @retval EFI_NOT_FOUND  The FV is not found for new installed EFI_PEI_FIRMWARE_VOLUME_PPI
  @retval EFI_SUCCESS    Success to find a FV which could be processed by new installed EFI_PEI_FIRMWARE_VOLUME_PPI.
**/
EFI_STATUS
FindUnknownFormatFvInfo (
  IN  PEI_CORE_INSTANCE *PrivateData,
  IN  EFI_GUID          *Format,
  OUT VOID              **FvInfo,
  OUT UINT32            *FvInfoSize,
  OUT UINT32            *AuthenticationStatus
  )
{
  UINTN Index;
  UINTN Index2;

  Index = 0;
  for (; Index < PrivateData->UnknownFvInfoCount; Index ++) {
    if (CompareGuid (Format, &PrivateData->UnknownFvInfo[Index].FvFormat)) {
      break;
    }
  }
  
  if (Index == PrivateData->UnknownFvInfoCount) {
    return EFI_NOT_FOUND;
  }
  
  *FvInfo     = PrivateData->UnknownFvInfo[Index].FvInfo;
  *FvInfoSize = PrivateData->UnknownFvInfo[Index].FvInfoSize;
  *AuthenticationStatus = PrivateData->UnknownFvInfo[Index].AuthenticationStatus;
  
  //
  // Remove an entry from UnknownFvInfo array.
  //
  Index2 = Index + 1;
  for (;Index2 < PrivateData->UnknownFvInfoCount; Index2 ++, Index ++) {
    CopyMem (&PrivateData->UnknownFvInfo[Index], &PrivateData->UnknownFvInfo[Index2], sizeof (PEI_CORE_UNKNOW_FORMAT_FV_INFO));
  }
  PrivateData->UnknownFvInfoCount --;
  return EFI_SUCCESS;
}  

/**
  Notification callback function for EFI_PEI_FIRMWARE_VOLUME_PPI.
  
  When a EFI_PEI_FIRMWARE_VOLUME_PPI is installed to support new FV format, this 
  routine is called to process all discoveried FVs in this format.
  
  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.
  
  @retval EFI_SUCCESS  The notification callback is processed correctly.
**/
EFI_STATUS
EFIAPI
ThirdPartyFvPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
{
  PEI_CORE_INSTANCE            *PrivateData;
  EFI_PEI_FIRMWARE_VOLUME_PPI  *FvPpi;
  VOID                         *FvInfo;
  UINT32                       FvInfoSize;
  UINT32                       AuthenticationStatus;
  EFI_STATUS                   Status;
  EFI_PEI_FV_HANDLE            FvHandle;
  BOOLEAN                      IsProcessed;
  UINTN                        FvIndex;
  EFI_PEI_FILE_HANDLE          FileHandle;
  VOID                         *DepexData;  
  
  PrivateData  = PEI_CORE_INSTANCE_FROM_PS_THIS (PeiServices);
  FvPpi = (EFI_PEI_FIRMWARE_VOLUME_PPI*) Ppi;
  
  do {
    Status = FindUnknownFormatFvInfo (PrivateData, NotifyDescriptor->Guid, &FvInfo, &FvInfoSize, &AuthenticationStatus);
    if (EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    }
    
    //
    // Process new found FV and get FV handle.
    //
    Status = FvPpi->ProcessVolume (FvPpi, FvInfo, FvInfoSize, &FvHandle);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Fail to process the FV 0x%p, FV may be corrupted!\n", FvInfo));
      continue;
    }

    //
    // Check whether the FV has already been processed.
    //
    IsProcessed = FALSE;
    for (FvIndex = 0; FvIndex < PrivateData->FvCount; FvIndex ++) {
      if (PrivateData->Fv[FvIndex].FvHandle == FvHandle) {
        DEBUG ((EFI_D_INFO, "The Fv %p has already been processed!\n", FvInfo));
        IsProcessed = TRUE;
        break;
      }
    }
  
    if (IsProcessed) {
      continue;
    }
    
    if (PrivateData->FvCount >= PcdGet32 (PcdPeiCoreMaxFvSupported)) {
      DEBUG ((EFI_D_ERROR, "The number of Fv Images (%d) exceed the max supported FVs (%d) in Pei", PrivateData->FvCount + 1, PcdGet32 (PcdPeiCoreMaxFvSupported)));
      DEBUG ((EFI_D_ERROR, "PcdPeiCoreMaxFvSupported value need be reconfigurated in DSC"));
      ASSERT (FALSE);
    }
        
    //
    // Update internal PEI_CORE_FV array.
    //
    PrivateData->Fv[PrivateData->FvCount].FvHeader = (EFI_FIRMWARE_VOLUME_HEADER*) FvInfo;
    PrivateData->Fv[PrivateData->FvCount].FvPpi    = FvPpi;
    PrivateData->Fv[PrivateData->FvCount].FvHandle = FvHandle;
    PrivateData->Fv[PrivateData->FvCount].AuthenticationStatus = AuthenticationStatus;
    DEBUG ((
      EFI_D_INFO, 
      "The %dth FV start address is 0x%11p, size is 0x%08x, handle is 0x%p\n", 
      (UINT32) PrivateData->FvCount, 
      (VOID *) FvInfo, 
      FvInfoSize,
      FvHandle
      ));    
    PrivateData->FvCount ++;

    //
    // Scan and process the new discoveried FV for EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE 
    //
    FileHandle = NULL;
    do {
      Status = FvPpi->FindFileByType (
                        FvPpi,
                        EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
                        FvHandle,
                        &FileHandle
                       );
      if (!EFI_ERROR (Status)) {
        Status = FvPpi->FindSectionByType (
                          FvPpi,
                          EFI_SECTION_PEI_DEPEX,
                          FileHandle,
                          (VOID**)&DepexData
                          );
        if (!EFI_ERROR (Status)) {
          if (!PeimDispatchReadiness (PeiServices, DepexData)) {
            //
            // Dependency is not satisfied.
            //
            continue;
          }
        }
        
        DEBUG ((EFI_D_INFO, "Found firmware volume Image File %p in FV[%d] %p\n", FileHandle, PrivateData->FvCount - 1, FvHandle));
        ProcessFvFile (PrivateData, &PrivateData->Fv[PrivateData->FvCount - 1], FileHandle);
      }
    } while (FileHandle != NULL);
  } while (TRUE);
}
