/*++ @file
  A simple FV stack so the SEC can extract the SEC Core from an
  FV.

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#define GET_OCCUPIED_SIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))

EFI_FFS_FILE_STATE
GetFileState (
  IN UINT8                ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )

/*++

Routine Description:
  Returns the highest bit set of the State field

Arguments:
  ErasePolarity   - Erase Polarity  as defined by EFI_FVB2_ERASE_POLARITY
                    in the Attributes field.
  FfsHeader       - Pointer to FFS File Header.

Returns:
  Returns the highest bit in the State field

**/
{
  EFI_FFS_FILE_STATE  FileState;
  EFI_FFS_FILE_STATE  HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity != 0) {
    FileState = (EFI_FFS_FILE_STATE) ~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && (HighestBit & FileState) == 0) {
    HighestBit >>= 1;
  }

  return HighestBit;
}

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

**/
{
  UINT8  *ptr;
  UINTN  Index;
  UINT8  Sum;

  Sum = 0;
  ptr = (UINT8 *)FileHeader;

  for (Index = 0; Index < sizeof (EFI_FFS_FILE_HEADER) - 3; Index += 4) {
    Sum = (UINT8)(Sum + ptr[Index]);
    Sum = (UINT8)(Sum + ptr[Index + 1]);
    Sum = (UINT8)(Sum + ptr[Index + 2]);
    Sum = (UINT8)(Sum + ptr[Index + 3]);
  }

  for ( ; Index < sizeof (EFI_FFS_FILE_HEADER); Index++) {
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

EFI_STATUS
SecFfsFindNextFile (
  IN EFI_FV_FILETYPE          SearchType,
  IN EFI_PEI_FV_HANDLE        FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )

/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume as defined by SearchType. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    SearchType - Filter to find only files of this type.
                 Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
    FwVolHeader - Pointer to the FV header of the volume to search.
                  This parameter must point to a valid FFS volume.
    FileHeader  - Pointer to the current file from which to begin searching.
                  This pointer will be updated upon return to reflect the file
                  found.

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

**/
{
  EFI_FFS_FILE_HEADER         *FfsFileHeader;
  UINT32                      FileLength;
  UINT32                      FileOccupiedSize;
  UINT32                      FileOffset;
  UINT64                      FvLength;
  UINT8                       ErasePolarity;
  UINT8                       FileState;
  EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader;
  EFI_FFS_FILE_HEADER         **FileHeader;

  //
  // Convert the handle of FV to FV header for memory-mapped firmware volume
  //
  FwVolHeader = (EFI_FIRMWARE_VOLUME_HEADER *)FvHandle;
  FileHeader  = (EFI_FFS_FILE_HEADER **)FileHandle;

  FvLength = FwVolHeader->FvLength;
  if (FwVolHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
    ErasePolarity = 1;
  } else {
    ErasePolarity = 0;
  }

  //
  // If FileHeader is not specified (NULL) start with the first file in the
  // firmware volume.  Otherwise, start from the FileHeader.
  //
  if (*FileHeader == NULL) {
    FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FwVolHeader + FwVolHeader->HeaderLength);
  } else {
    //
    // Length is 24 bits wide so mask upper 8 bits
    // FileLength is adjusted to FileOccupiedSize as it is 8 byte aligned.
    //
    FileLength       = *(UINT32 *)(*FileHeader)->Size & 0x00FFFFFF;
    FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
    FfsFileHeader    = (EFI_FFS_FILE_HEADER *)((UINT8 *)*FileHeader + FileOccupiedSize);
  }

  FileOffset = (UINT32)((UINT8 *)FfsFileHeader - (UINT8 *)FwVolHeader);

  while (FileOffset < (FvLength - sizeof (EFI_FFS_FILE_HEADER))) {
    //
    // Get FileState which is the highest bit of the State
    //
    FileState = GetFileState (ErasePolarity, FfsFileHeader);

    switch (FileState) {
      case EFI_FILE_HEADER_INVALID:
        FileOffset   += sizeof (EFI_FFS_FILE_HEADER);
        FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + sizeof (EFI_FFS_FILE_HEADER));
        break;

      case EFI_FILE_DATA_VALID:
      case EFI_FILE_MARKED_FOR_UPDATE:
        if (CalculateHeaderChecksum (FfsFileHeader) == 0) {
          FileLength       = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
          FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);

          if ((SearchType == FfsFileHeader->Type) || (SearchType == EFI_FV_FILETYPE_ALL)) {
            *FileHeader = FfsFileHeader;

            return EFI_SUCCESS;
          }

          FileOffset   += FileOccupiedSize;
          FfsFileHeader = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        } else {
          return EFI_NOT_FOUND;
        }

        break;

      case EFI_FILE_DELETED:
        FileLength       = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
        FileOccupiedSize = GET_OCCUPIED_SIZE (FileLength, 8);
        FileOffset      += FileOccupiedSize;
        FfsFileHeader    = (EFI_FFS_FILE_HEADER *)((UINT8 *)FfsFileHeader + FileOccupiedSize);
        break;

      default:
        return EFI_NOT_FOUND;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
SecFfsFindSectionData (
  IN EFI_SECTION_TYPE     SectionType,
  IN EFI_FFS_FILE_HEADER  *FfsFileHeader,
  IN OUT VOID             **SectionData
  )

/*++

Routine Description:
    Given the input file pointer, search for the next matching section in the
    FFS volume.

Arguments:
    SearchType    - Filter to find only sections of this type.
    FfsFileHeader - Pointer to the current file to search.
    SectionData   - Pointer to the Section matching SectionType in FfsFileHeader.
                     NULL if section not found

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

**/
{
  UINT32                     FileSize;
  EFI_COMMON_SECTION_HEADER  *Section;
  UINT32                     SectionLength;
  UINT32                     ParsedLength;

  //
  // Size is 24 bits wide so mask upper 8 bits.
  //    Does not include FfsFileHeader header size
  // FileSize is adjusted to FileOccupiedSize as it is 8 byte aligned.
  //
  Section   = (EFI_COMMON_SECTION_HEADER *)(FfsFileHeader + 1);
  FileSize  = *(UINT32 *)(FfsFileHeader->Size) & 0x00FFFFFF;
  FileSize -= sizeof (EFI_FFS_FILE_HEADER);

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
    SectionLength = GET_OCCUPIED_SIZE (SectionLength, 4);

    ParsedLength += SectionLength;
    Section       = (EFI_COMMON_SECTION_HEADER *)((UINT8 *)Section + SectionLength);
  }

  return EFI_NOT_FOUND;
}
