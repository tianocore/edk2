/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FwVol.c

Abstract:

  Pei Core Firmware File System service routines.

--*/

#include <PeiMain.h>

#define GETOCCUPIEDSIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))

STATIC
EFI_FFS_FILE_STATE
GetFileState(
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:

  Returns the highest bit set of the State field

Arguments:

  ErasePolarity   - Erase Polarity  as defined by EFI_FVB_ERASE_POLARITY
                    in the Attributes field.
  FfsHeader       - Pointer to FFS File Header.

Returns:
  Returns the highest bit in the State field

--*/
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

STATIC
UINT8
CalculateHeaderChecksum (
  IN EFI_FFS_FILE_HEADER  *FileHeader
  )
/*++

Routine Description:

  Calculates the checksum of the header of a file.

Arguments:

  FileHeader       - Pointer to FFS File Header.

Returns:
  Checksum of the header.
  
  The header is zero byte checksum.
    - Zero means the header is good.
    - Non-zero means the header is bad.
    
  
Bugbug: For PEI performance reason, we comments this code at this time.
--*/
{
  UINT8   *ptr;
  UINTN   Index;
  UINT8   Sum;
  
  Sum = 0;
  ptr = (UINT8 *)FileHeader;

  for (Index = 0; Index < sizeof(EFI_FFS_FILE_HEADER) - 3; Index += 4) {
    Sum = (UINT8)(Sum + ptr[Index]);
    Sum = (UINT8)(Sum + ptr[Index+1]);
    Sum = (UINT8)(Sum + ptr[Index+2]);
    Sum = (UINT8)(Sum + ptr[Index+3]);
  }

  for (; Index < sizeof(EFI_FFS_FILE_HEADER); Index++) {
    Sum = (UINT8)(Sum + ptr[Index]);
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

STATIC
EFI_STATUS
PeiFfsFindNextFileEx (
  IN     EFI_FV_FILETYPE             SearchType,
  IN     EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER         **FileHeader,
  IN     BOOLEAN                     Flag
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume as defined by SearchType. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.
    SearchType - Filter to find only files of this type.
      Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
    FwVolHeader - Pointer to the FV header of the volume to search.
      This parameter must point to a valid FFS volume.
    FileHeader  - Pointer to the current file from which to begin searching.
      This pointer will be updated upon return to reflect the file found.
    Flag        - Indicator for if this is for PEI Dispath search 
Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
{
  EFI_FFS_FILE_HEADER  *FfsFileHeader;
  UINT32               FileLength;
  UINT32               FileOccupiedSize;
  UINT32               FileOffset;
  UINT64               FvLength;
  UINT8                ErasePolarity;
  UINT8                FileState;
  

  FvLength = FwVolHeader->FvLength;
  if (FwVolHeader->Attributes & EFI_FVB_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }
  
  //
  // If FileHeader is not specified (NULL) start with the first file in the
  // firmware volume.  Otherwise, start from the FileHeader.
  //
  if (*FileHeader == NULL)  {
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
  } else {
    //
    // Length is 24 bits wide so mask upper 8 bits
    // FileLength is adjusted to FileOccupiedSize as it is 8 byte aligned.
    //
    FileLength = *(UINT32 *)(*FileHeader)->Size & 0x00FFFFFF;
    FileOccupiedSize = GETOCCUPIEDSIZE(FileLength, 8);
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)*FileHeader + FileOccupiedSize);
  }

  FileOffset = (UINT32) ((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);
  ASSERT (FileOffset <= 0xFFFFFFFF);
  
  while (FileOffset < (FvLength - sizeof(EFI_FFS_FILE_HEADER))) {
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
       if (CalculateHeaderChecksum (FfsFileHeader) == 0) {
        FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
        FileOccupiedSize = GETOCCUPIEDSIZE(FileLength, 8);
        if (Flag) {
          if ((FfsFileHeader->Type == EFI_FV_FILETYPE_PEIM) || 
              (FfsFileHeader->Type == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER)) { 
            
            *FileHeader = FfsFileHeader;
        
        
            return EFI_SUCCESS;
          }
        } else {        
          if ((SearchType == FfsFileHeader->Type) || 
              (SearchType == EFI_FV_FILETYPE_ALL)) { 
          
            *FileHeader = FfsFileHeader;
        
        
            return EFI_SUCCESS;
          }
        }

        FileOffset += FileOccupiedSize; 
        FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
      } else {
        ASSERT (FALSE);
        return EFI_NOT_FOUND;
      }
      break;
    
    case EFI_FILE_DELETED:
      FileLength = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
      FileOccupiedSize = GETOCCUPIEDSIZE(FileLength, 8);
      FileOffset += FileOccupiedSize;
      FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
      break;

    default:
      return EFI_NOT_FOUND;

    } 
  }

  return EFI_NOT_FOUND;  
}


EFI_STATUS
EFIAPI
PeiFfsFindSectionData (
  IN     EFI_PEI_SERVICES      **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     EFI_FFS_FILE_HEADER   *FfsFileHeader,
  IN OUT VOID                  **SectionData
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching section in the
    FFS volume.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.
    SearchType - Filter to find only sections of this type.
    FfsFileHeader  - Pointer to the current file to search.
    SectionData - Pointer to the Section matching SectionType in FfsFileHeader.
                - NULL if section not found

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
{
  UINT32                        FileSize;
  EFI_COMMON_SECTION_HEADER     *Section;
  UINT32                        SectionLength;
  UINT32                        ParsedLength;
  

  //
  // Size is 24 bits wide so mask upper 8 bits. 
  //    Does not include FfsFileHeader header size
  // FileSize is adjusted to FileOccupiedSize as it is 8 byte aligned.
  //
  Section = (EFI_COMMON_SECTION_HEADER *)(FfsFileHeader + 1);
  FileSize = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
  FileSize -= sizeof(EFI_FFS_FILE_HEADER);
  
  *SectionData = NULL;
  ParsedLength = 0;
  while (ParsedLength < FileSize) {
    if (Section->Type == SectionType) {
      *SectionData = (VOID *)(Section + 1);


      return EFI_SUCCESS;
    }
    //
    // Size is 24 bits wide so mask upper 8 bits. 
    // SectionLength is adjusted it is 4 byte aligned.
    // Go to the next section
    //
    SectionLength = *(UINT32 *)Section->Size & 0x00FFFFFF;
    SectionLength = GETOCCUPIEDSIZE (SectionLength, 4);
    ASSERT (SectionLength != 0);
    ParsedLength += SectionLength;
    Section = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
  }
  
  return EFI_NOT_FOUND;
  
}


EFI_STATUS
FindNextPeim (
  IN     EFI_PEI_SERVICES            **PeiServices,
  IN     EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER         **PeimFileHeader
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.

    FwVolHeader - Pointer to the FV header of the volume to search.
                     This parameter must point to a valid FFS volume.
                     
    PeimFileHeader  - Pointer to the current file from which to begin searching.
                  This pointer will be updated upon return to reflect the file found.

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
{
  return PeiFfsFindNextFileEx ( 
           0,
           FwVolHeader,
           PeimFileHeader,
           TRUE
           );
}

EFI_STATUS
EFIAPI
PeiFfsFindNextFile (
  IN     EFI_PEI_SERVICES            **PeiServices,
  IN     EFI_FV_FILETYPE             SearchType,
  IN     EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER         **FileHeader
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume as defined by SearchType. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.
    
    SearchType - Filter to find only files of this type.
      Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
      
    FwVolHeader - Pointer to the FV header of the volume to search.
      This parameter must point to a valid FFS volume.
      
    FileHeader  - Pointer to the current file from which to begin searching.
      This pointer will be updated upon return to reflect the file found.
  
Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
{
  return PeiFfsFindNextFileEx ( 
           SearchType,
           FwVolHeader,
           FileHeader,
           FALSE
           );
}

EFI_STATUS 
EFIAPI
PeiFvFindNextVolume (
  IN     EFI_PEI_SERVICES           **PeiServices,
  IN     UINTN                      Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER **FwVolHeader
  )
/*++

Routine Description:

  Return the BFV location

  BugBug -- Move this to the location of this code to where the
  other FV and FFS support code lives.
  Also, update to use FindFV for instances #'s >= 1.

Arguments:

  PeiServices - The PEI core services table.
  Instance    - Instance of FV to find
  FwVolHeader - Pointer to contain the data to return

Returns:
  Pointer to the Firmware Volume instance requested

  EFI_INVALID_PARAMETER     - FwVolHeader is NULL
  
  EFI_SUCCESS               - Firmware volume instance successfully found.

--*/
{
  PEI_CORE_INSTANCE       *PrivateData;
  EFI_STATUS              Status;
  EFI_PEI_FIND_FV_PPI     *FindFvPpi;
  UINT8                   LocalInstance;


  LocalInstance = (UINT8) Instance;

  Status = EFI_SUCCESS;
  PrivateData = PEI_CORE_INSTANCE_FROM_PS_THIS(PeiServices);

  if (FwVolHeader == NULL) {

    return EFI_INVALID_PARAMETER;
  }

  if (Instance == 0) {
    *FwVolHeader = PrivateData->DispatchData.BootFvAddress;


    return Status;
  } else {
    //
    // Locate all instances of FindFV
    // Alternately, could use FV HOBs, but the PPI is cleaner
    //
    Status = PeiServicesLocatePpi (
               &gEfiFindFvPpiGuid,
               0,
               NULL,
               (VOID **)&FindFvPpi
               );

    if (Status != EFI_SUCCESS) {
      Status = EFI_NOT_FOUND;
    } else {
      Status = FindFvPpi->FindFv (
                            FindFvPpi,
                            PeiServices,
                            &LocalInstance,
                            FwVolHeader
                            );  

    }
  }
  return Status;
}
