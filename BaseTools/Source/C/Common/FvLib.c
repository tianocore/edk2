/** @file
These functions assist in parsing and manipulating a Firmware Volume.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include files
//
#include "FvLib.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

//
// Module global variables
//
EFI_FIRMWARE_VOLUME_HEADER  *mFvHeader  = NULL;
UINT32                      mFvLength   = 0;

//
// External function implementations
//

/**
  This initializes the FV lib with a pointer to the FV and length.  It does not
  verify the FV in any way.

  @param Fv            Buffer containing the FV.
  @param FvLength      Length of the FV

  @retval EFI_SUCCESS             Function Completed successfully.
  @retval EFI_INVALID_PARAMETER   A required parameter was NULL.
**/
EFI_STATUS
InitializeFvLib (
  IN VOID                         *Fv,
  IN UINT32                       FvLength
  )
{
  //
  // Verify input arguments
  //
  if (Fv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) Fv;
  mFvLength = FvLength;

  return EFI_SUCCESS;
}

/**
  This function returns a pointer to the current FV and the size.

  @param FvHeader      Pointer to the FV buffer.
  @param FvLength      Length of the FV

  @retval EFI_SUCCESS             Function Completed successfully.
  @retval EFI_INVALID_PARAMETER   A required parameter was NULL.
  @retvalEFI_ABORTED             The library needs to be initialized.
**/
EFI_STATUS
GetFvHeader (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FvHeader,
  OUT UINT32                      *FvLength
  )
{
  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify input arguments
  //
  if (FvHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *FvHeader = mFvHeader;
  *FvLength = mFvLength;
  return EFI_SUCCESS;
}

/**
  This function returns the next file.  If the current file is NULL, it returns
  the first file in the FV.  If the function returns EFI_SUCCESS and the file
  pointer is NULL, then there are no more files in the FV.

  @param CurrentFile   Pointer to the current file, must be within the current FV.
  @param NextFile      Pointer to the next file in the FV.

  @retval EFI_SUCCESS             Function completed successfully.
  @retval EFI_INVALID_PARAMETER   A required parameter was NULL or is out of range.
  @retval EFI_ABORTED             The library needs to be initialized.
**/
EFI_STATUS
GetNextFile (
  IN EFI_FFS_FILE_HEADER          *CurrentFile,
  OUT EFI_FFS_FILE_HEADER         **NextFile
  )
{
  EFI_STATUS  Status;

  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify input arguments
  //
  if (NextFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Verify FV header
  //
  Status = VerifyFv (mFvHeader);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Get first file
  //
  if (CurrentFile == NULL) {
    CurrentFile = (EFI_FFS_FILE_HEADER *) ((UINTN) mFvHeader + mFvHeader->HeaderLength);

    //
    // Verify file is valid
    //
    Status = VerifyFfsFile (CurrentFile);
    if (EFI_ERROR (Status)) {
      //
      // no files in this FV
      //
      *NextFile = NULL;
      return EFI_SUCCESS;
    } else {
      //
      // Verify file is in this FV.
      //
      if ((UINTN) CurrentFile + GetFfsFileLength(CurrentFile) > (UINTN) mFvHeader + mFvLength) {
        *NextFile = NULL;
        return EFI_SUCCESS;
      }

      *NextFile = CurrentFile;
      return EFI_SUCCESS;
    }
  }
  //
  // Verify current file is in range
  //
  if (((UINTN) CurrentFile < (UINTN) mFvHeader + mFvHeader->HeaderLength) ||
      ((UINTN) CurrentFile + GetFfsFileLength(CurrentFile) > (UINTN) mFvHeader + mFvLength)
     ) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get next file, compensate for 8 byte alignment if necessary.
  //
  *NextFile = (EFI_FFS_FILE_HEADER *) ((((UINTN) CurrentFile - (UINTN) mFvHeader + GetFfsFileLength(CurrentFile) + 0x07) & (~(UINTN) 7)) + (UINT8 *) mFvHeader);

  //
  // Verify file is in this FV.
  //
  if (((UINTN) *NextFile + GetFfsHeaderLength(*NextFile) >= (UINTN) mFvHeader + mFvLength) ||
      ((UINTN) *NextFile + GetFfsFileLength (*NextFile) > (UINTN) mFvHeader + mFvLength)
     ) {
    *NextFile = NULL;
    return EFI_SUCCESS;
  }
  //
  // Verify file is valid
  //
  Status = VerifyFfsFile (*NextFile);
  if (EFI_ERROR (Status)) {
    //
    // no more files in this FV
    //
    *NextFile = NULL;
    return EFI_SUCCESS;
  }

  return EFI_SUCCESS;
}

/**
  Find a file by name.  The function will return NULL if the file is not found.

  @param FileName    The GUID file name of the file to search for.
  @param File        Return pointer.  In the case of an error, contents are undefined.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error was encountered.
  @retval EFI_INVALID_PARAMETER   One of the parameters was NULL.
**/
EFI_STATUS
GetFileByName (
  IN EFI_GUID                     *FileName,
  OUT EFI_FFS_FILE_HEADER         **File
  )
{
  EFI_FFS_FILE_HEADER *CurrentFile;
  EFI_STATUS          Status;
  CHAR8               FileGuidString[80];

  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify input parameters
  //
  if (FileName == NULL || File == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // File Guid String Name
  //
  PrintGuidToBuffer (FileName, (UINT8 *)FileGuidString, sizeof (FileGuidString), TRUE);
  //
  // Verify FV header
  //
  Status = VerifyFv (mFvHeader);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Get the first file
  //
  Status = GetNextFile (NULL, &CurrentFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0003, "error parsing FV image", "FFS file with Guid %s can't be found", FileGuidString);
    return EFI_ABORTED;
  }
  //
  // Loop as long as we have a valid file
  //
  while (CurrentFile) {
    if (!CompareGuid (&CurrentFile->Name, FileName)) {
      *File = CurrentFile;
      return EFI_SUCCESS;
    }

    Status = GetNextFile (CurrentFile, &CurrentFile);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "FFS file with Guid %s can't be found", FileGuidString);
      return EFI_ABORTED;
    }
  }
  //
  // File not found in this FV.
  //
  *File = NULL;
  return EFI_SUCCESS;
}

/**
  Find a file by type and instance.  An instance of 1 is the first instance.
  The function will return NULL if a matching file cannot be found.
  File type EFI_FV_FILETYPE_ALL means any file type is valid.

  @param FileType    Type of file to search for.
  @param Instance    Instance of the file type to return.
  @param File        Return pointer.  In the case of an error, contents are undefined.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error was encountered.
  @retval EFI_INVALID_PARAMETER   One of the parameters was NULL.
**/
EFI_STATUS
GetFileByType (
  IN EFI_FV_FILETYPE              FileType,
  IN UINTN                        Instance,
  OUT EFI_FFS_FILE_HEADER         **File
  )
{
  EFI_FFS_FILE_HEADER *CurrentFile;
  EFI_STATUS          Status;
  UINTN               FileCount;

  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify input parameters
  //
  if (File == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Verify FV header
  //
  Status = VerifyFv (mFvHeader);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Initialize the number of matching files found.
  //
  FileCount = 0;

  //
  // Get the first file
  //
  Status = GetNextFile (NULL, &CurrentFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0003, "error parsing FV image", "FFS file with FileType 0x%x can't be found", FileType);
    return EFI_ABORTED;
  }
  //
  // Loop as long as we have a valid file
  //
  while (CurrentFile) {
    if (FileType == EFI_FV_FILETYPE_ALL || CurrentFile->Type == FileType) {
      FileCount++;
    }

    if (FileCount == Instance) {
      *File = CurrentFile;
      return EFI_SUCCESS;
    }

    Status = GetNextFile (CurrentFile, &CurrentFile);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "error parsing FV image", "FFS file with FileType 0x%x can't be found", FileType);
      return EFI_ABORTED;
    }
  }

  *File = NULL;
  return EFI_SUCCESS;
}

/**
  Helper function to search a sequence of sections from the section pointed
  by FirstSection to SearchEnd for the Instance-th section of type SectionType.
  The current counter is saved in StartIndex and when the section is found, it's
  saved in Section. GUID-defined sections, if special processing is not required,
  are searched recursively in a depth-first manner.

  @param FirstSection The first section to start searching from.
  @param SearchEnd    The end address to stop search.
  @param SectionType  The type of section to search.
  @param StartIndex   The current counter is saved.
  @param Instance     The requested n-th section number.
  @param Section      The found section returned.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           The section is not found.
**/
EFI_STATUS
SearchSectionByType (
  IN EFI_FILE_SECTION_POINTER  FirstSection,
  IN UINT8                     *SearchEnd,
  IN EFI_SECTION_TYPE          SectionType,
  IN OUT UINTN                 *StartIndex,
  IN UINTN                     Instance,
  OUT EFI_FILE_SECTION_POINTER *Section
  )
{
  EFI_FILE_SECTION_POINTER  CurrentSection;
  EFI_FILE_SECTION_POINTER  InnerSection;
  EFI_STATUS                Status;
  UINTN                     SectionSize;
  UINT16                    GuidSecAttr;
  UINT16                    GuidDataOffset;

  GuidSecAttr = 0;
  GuidDataOffset = 0;
  CurrentSection = FirstSection;

  while ((UINTN) CurrentSection.CommonHeader < (UINTN) SearchEnd) {
    if (CurrentSection.CommonHeader->Type == SectionType) {
      (*StartIndex)++;
    }

    if (*StartIndex == Instance) {
      *Section = CurrentSection;
      return EFI_SUCCESS;
    }
    //
    // If the requesting section is not GUID-defined and
    // we find a GUID-defined section that doesn't need
    // special processing, go ahead to search the requesting
    // section inside the GUID-defined section.
    //
    if (CurrentSection.CommonHeader->Type == EFI_SECTION_GUID_DEFINED) {
      if (GetLength(CurrentSection.CommonHeader->Size) == 0xffffff) {
        GuidSecAttr = CurrentSection.GuidDefinedSection2->Attributes;
        GuidDataOffset = CurrentSection.GuidDefinedSection2->DataOffset;
      } else {
        GuidSecAttr = CurrentSection.GuidDefinedSection->Attributes;
        GuidDataOffset = CurrentSection.GuidDefinedSection->DataOffset;
      }
    }
    if (SectionType != EFI_SECTION_GUID_DEFINED &&
        CurrentSection.CommonHeader->Type == EFI_SECTION_GUID_DEFINED &&
        !(GuidSecAttr & EFI_GUIDED_SECTION_PROCESSING_REQUIRED)) {
      InnerSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *)
        ((UINTN) CurrentSection.CommonHeader + GuidDataOffset);
      SectionSize = GetSectionFileLength(CurrentSection.CommonHeader);
      Status = SearchSectionByType (
                 InnerSection,
                 (UINT8 *) ((UINTN) CurrentSection.CommonHeader + SectionSize),
                 SectionType,
                 StartIndex,
                 Instance,
                 Section
                 );
      if (!EFI_ERROR (Status)) {
        return EFI_SUCCESS;
      }
    }
    //
    // Find next section (including compensating for alignment issues.
    //
    CurrentSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *) ((((UINTN) CurrentSection.CommonHeader) + GetSectionFileLength(CurrentSection.CommonHeader) + 0x03) & (~(UINTN) 3));
  }

  return EFI_NOT_FOUND;
}

/**
  Find a section in a file by type and instance.  An instance of 1 is the first
  instance.  The function will return NULL if a matching section cannot be found.
  GUID-defined sections, if special processing is not needed, are handled in a
  depth-first manner.

  @param File        The file to search.
  @param SectionType Type of file to search for.
  @param Instance    Instance of the section to return.
  @param Section     Return pointer.  In the case of an error, contents are undefined.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error was encountered.
  @retval EFI_INVALID_PARAMETER   One of the parameters was NULL.
  @retval EFI_NOT_FOUND           No found.
**/
EFI_STATUS
GetSectionByType (
  IN EFI_FFS_FILE_HEADER          *File,
  IN EFI_SECTION_TYPE             SectionType,
  IN UINTN                        Instance,
  OUT EFI_FILE_SECTION_POINTER    *Section
  )
{
  EFI_FILE_SECTION_POINTER  CurrentSection;
  EFI_STATUS                Status;
  UINTN                     SectionCount;

  //
  // Verify input parameters
  //
  if (File == NULL || Instance == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Verify FFS header
  //
  Status = VerifyFfsFile (File);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0006, "invalid FFS file", NULL);
    return EFI_ABORTED;
  }
  //
  // Initialize the number of matching sections found.
  //
  SectionCount = 0;

  //
  // Get the first section
  //
  CurrentSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *) ((UINTN) File + GetFfsHeaderLength(File));

  //
  // Depth-first manner to find section file.
  //
  Status = SearchSectionByType (
             CurrentSection,
             (UINT8 *) ((UINTN) File + GetFfsFileLength (File)),
             SectionType,
             &SectionCount,
             Instance,
             Section
             );

  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  } else {
    //
    // Section not found
    //
    (*Section).Code16Section = NULL;
    return EFI_NOT_FOUND;
  }
}

//
// will not parse compressed sections
//

/**
  Verify the current pointer points to a valid FV header.

  @param FvHeader     Pointer to an alleged FV file.

  @retval EFI_SUCCESS             The FV header is valid.
  @retval EFI_VOLUME_CORRUPTED    The FV header is not valid.
  @retval EFI_INVALID_PARAMETER   A required parameter was NULL.
  @retval EFI_ABORTED             Operation aborted.
**/
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER   *FvHeader
  )
{
  UINT16  Checksum;

  //
  // Verify input parameters
  //
  if (FvHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
    Error (NULL, 0, 0006, "invalid FV header signature", NULL);
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Verify header checksum
  //
  Checksum = CalculateSum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

  if (Checksum != 0) {
    Error (NULL, 0, 0006, "invalid FV header checksum", NULL);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Verify the current pointer points to a FFS file header.

  @param FfsHeader     Pointer to an alleged FFS file.

  @retval EFI_SUCCESS           The Ffs header is valid.
  @retval EFI_NOT_FOUND         This "file" is the beginning of free space.
  @retval EFI_VOLUME_CORRUPTED  The Ffs header is not valid.
  @retval EFI_ABORTED           The erase polarity is not known.
**/
EFI_STATUS
VerifyFfsFile (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  BOOLEAN             ErasePolarity;
  EFI_STATUS          Status;
  EFI_FFS_FILE_HEADER2 BlankHeader;
  UINT8               Checksum;
  UINT32              FileLength;
  UINT8               SavedChecksum;
  UINT8               SavedState;
  UINT8               FileGuidString[80];
  UINT32              FfsHeaderSize;

  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify FV header
  //
  Status = VerifyFv (mFvHeader);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Get the erase polarity.
  //
  Status = GetErasePolarity (&ErasePolarity);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  FfsHeaderSize = GetFfsHeaderLength(FfsHeader);
  //
  // Check if we have free space
  //
  if (ErasePolarity) {
    memset (&BlankHeader, -1, FfsHeaderSize);
  } else {
    memset (&BlankHeader, 0, FfsHeaderSize);
  }

  if (memcmp (&BlankHeader, FfsHeader, FfsHeaderSize) == 0) {
    return EFI_NOT_FOUND;
  }
  //
  // Convert the GUID to a string so we can at least report which file
  // if we find an error.
  //
  PrintGuidToBuffer (&FfsHeader->Name, FileGuidString, sizeof (FileGuidString), TRUE);
  //
  // Verify file header checksum
  //
  SavedState = FfsHeader->State;
  FfsHeader->State = 0;
  SavedChecksum = FfsHeader->IntegrityCheck.Checksum.File;
  FfsHeader->IntegrityCheck.Checksum.File = 0;
  Checksum = CalculateSum8 ((UINT8 *) FfsHeader, FfsHeaderSize);
  FfsHeader->State = SavedState;
  FfsHeader->IntegrityCheck.Checksum.File = SavedChecksum;
  if (Checksum != 0) {
    Error (NULL, 0, 0006, "invalid FFS file header checksum", "Ffs file with Guid %s", FileGuidString);
    return EFI_ABORTED;
  }
  //
  // Verify file checksum
  //
  if (FfsHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
    //
    // Verify file data checksum
    //
    FileLength          = GetFfsFileLength (FfsHeader);
    Checksum            = CalculateSum8 ((UINT8 *) ((UINT8 *)FfsHeader + FfsHeaderSize), FileLength - FfsHeaderSize);
    Checksum            = Checksum + FfsHeader->IntegrityCheck.Checksum.File;
    if (Checksum != 0) {
      Error (NULL, 0, 0006, "invalid FFS file checksum", "Ffs file with Guid %s", FileGuidString);
      return EFI_ABORTED;
    }
  } else {
    //
    // File does not have a checksum
    // Verify contents are 0xAA as spec'd
    //
    if (FfsHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
      Error (NULL, 0, 0006, "invalid fixed FFS file header checksum", "Ffs file with Guid %s", FileGuidString);
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

UINT32
GetFfsHeaderLength(
   IN EFI_FFS_FILE_HEADER *FfsHeader
   )
{
  if (FfsHeader == NULL) {
    return 0;
  }
  if (FfsHeader->Attributes & FFS_ATTRIB_LARGE_FILE) {
    return sizeof(EFI_FFS_FILE_HEADER2);
  }
  return sizeof(EFI_FFS_FILE_HEADER);
}

UINT32
GetSectionHeaderLength(
   IN EFI_COMMON_SECTION_HEADER *SectionHeader
   )
{
  if (SectionHeader == NULL) {
    return 0;
  }
  if (GetLength(SectionHeader->Size) == 0xffffff) {
    return sizeof(EFI_COMMON_SECTION_HEADER2);
  }
  return sizeof(EFI_COMMON_SECTION_HEADER);
}

/**
  Get FFS file length including FFS header.

  @param FfsHeader   Pointer to EFI_FFS_FILE_HEADER.

  @return UINT32      Length of FFS file header.
**/
UINT32
GetFfsFileLength (
  EFI_FFS_FILE_HEADER *FfsHeader
  )
{
  if (FfsHeader == NULL) {
    return 0;
  }
  if (FfsHeader->Attributes & FFS_ATTRIB_LARGE_FILE) {
    return (UINT32) ((EFI_FFS_FILE_HEADER2 *)FfsHeader)->ExtendedSize;
  } else {
    return GetLength(FfsHeader->Size);
  }
}

UINT32
GetSectionFileLength (
  EFI_COMMON_SECTION_HEADER *SectionHeader
  )
{
  UINT32 Length;
  if (SectionHeader == NULL) {
    return 0;
  }
  Length = GetLength(SectionHeader->Size);
  if (Length == 0xffffff) {
    Length = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
  }
  return Length;
}

/**
  Converts a three byte length value into a UINT32.

  @param ThreeByteLength   Pointer to the first of the 3 byte length.

  @return UINT32      Size of the section
**/
UINT32
GetLength (
  UINT8     *ThreeByteLength
  )
{
  UINT32  Length;

  if (ThreeByteLength == NULL) {
    return 0;
  }

  Length  = *((UINT32 *) ThreeByteLength);
  Length  = Length & 0x00FFFFFF;

  return Length;
}

/**
  This function returns with the FV erase polarity.  If the erase polarity
  for a bit is 1, the function return TRUE.

  @param ErasePolarity   A pointer to the erase polarity.

  @retval EFI_SUCCESS              The function completed successfully.
  @retval EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  @retval EFI_ABORTED              Operation aborted.
**/
EFI_STATUS
GetErasePolarity (
  OUT BOOLEAN   *ErasePolarity
  )
{
  EFI_STATUS  Status;

  //
  // Verify library has been initialized.
  //
  if (mFvHeader == NULL || mFvLength == 0) {
    return EFI_ABORTED;
  }
  //
  // Verify FV header
  //
  Status = VerifyFv (mFvHeader);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }
  //
  // Verify input parameters.
  //
  if (ErasePolarity == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (mFvHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
    *ErasePolarity = TRUE;
  } else {
    *ErasePolarity = FALSE;
  }

  return EFI_SUCCESS;
}

/**
  This function returns a the highest state bit in the FFS that is set.
  It in no way validate the FFS file.

  @param ErasePolarity The erase polarity for the file state bits.
  @param FfsHeader     Pointer to a FFS file.

  @retval UINT8   The hightest set state of the file.
**/
UINT8
GetFileState (
  IN BOOLEAN              ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
{
  UINT8 FileState;
  UINT8 HighestBit;

  FileState = FfsHeader->State;

  if (ErasePolarity) {
    FileState = (UINT8)~FileState;
  }

  HighestBit = 0x80;
  while (HighestBit != 0 && (HighestBit & FileState) == 0) {
    HighestBit >>= 1;
  }

  return HighestBit;
}
