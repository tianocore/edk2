/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FvLib.c

Abstract:

  These functions assist in parsing and manipulating a Firmware Volume.

--*/

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
EFI_STATUS
InitializeFvLib (
  IN VOID                         *Fv,
  IN UINT32                       FvLength
  )
/*++

Routine Description:

  This initializes the FV lib with a pointer to the FV and length.  It does not
  verify the FV in any way.

Arguments:

  Fv            Buffer containing the FV.
  FvLength      Length of the FV
    
Returns:
 
  EFI_SUCCESS             Function Completed successfully.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
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

EFI_STATUS
GetFvHeader (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FvHeader,
  OUT UINT32                      *FvLength
  )
/*++

Routine Description:

  This function returns a pointer to the current FV and the size.

Arguments:

  FvHeader      Pointer to the FV buffer.
  FvLength      Length of the FV
    
Returns:
 
  EFI_SUCCESS             Function Completed successfully.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_ABORTED             The library needs to be initialized.

--*/
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
  return EFI_SUCCESS;
}

EFI_STATUS
GetNextFile (
  IN EFI_FFS_FILE_HEADER          *CurrentFile,
  OUT EFI_FFS_FILE_HEADER         **NextFile
  )
/*++

Routine Description:

  This function returns the next file.  If the current file is NULL, it returns
  the first file in the FV.  If the function returns EFI_SUCCESS and the file 
  pointer is NULL, then there are no more files in the FV.

Arguments:

  CurrentFile   Pointer to the current file, must be within the current FV.
  NextFile      Pointer to the next file in the FV.
    
Returns:
 
  EFI_SUCCESS             Function completed successfully.
  EFI_INVALID_PARAMETER   A required parameter was NULL or is out of range.
  EFI_ABORTED             The library needs to be initialized.

--*/
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
      if ((UINTN) CurrentFile + GetLength (CurrentFile->Size) > (UINTN) mFvHeader + mFvLength) {
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
      ((UINTN) CurrentFile + GetLength (CurrentFile->Size) > (UINTN) mFvHeader + mFvLength)
     ) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get next file, compensate for 8 byte alignment if necessary.
  //
  *NextFile = (EFI_FFS_FILE_HEADER *) (((UINTN) CurrentFile + GetLength (CurrentFile->Size) + 0x07) & (-1 << 3));

  //
  // Verify file is in this FV.
  //
  if (((UINTN) *NextFile + sizeof (EFI_FFS_FILE_HEADER) >= (UINTN) mFvHeader + mFvLength) ||
      ((UINTN) *NextFile + GetLength ((*NextFile)->Size) > (UINTN) mFvHeader + mFvLength)
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

EFI_STATUS
GetFileByName (
  IN EFI_GUID                     *FileName,
  OUT EFI_FFS_FILE_HEADER         **File
  )
/*++

Routine Description:

  Find a file by name.  The function will return NULL if the file is not found.

Arguments:

  FileName    The GUID file name of the file to search for.
  File        Return pointer.  In the case of an error, contents are undefined.

Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_ABORTED             An error was encountered.
  EFI_INVALID_PARAMETER   One of the parameters was NULL.

--*/
{
  EFI_FFS_FILE_HEADER *CurrentFile;
  EFI_STATUS          Status;

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
    Error (NULL, 0, 0, "error parsing the FV", NULL);
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
      Error (NULL, 0, 0, "error parsing the FV", NULL);
      return EFI_ABORTED;
    }
  }
  //
  // File not found in this FV.
  //
  *File = NULL;
  return EFI_SUCCESS;
}

EFI_STATUS
GetFileByType (
  IN EFI_FV_FILETYPE              FileType,
  IN UINTN                        Instance,
  OUT EFI_FFS_FILE_HEADER         **File
  )
/*++

Routine Description:

  Find a file by type and instance.  An instance of 1 is the first instance.
  The function will return NULL if a matching file cannot be found.
  File type EFI_FV_FILETYPE_ALL means any file type is valid.

Arguments:

  FileType    Type of file to search for.
  Instance    Instace of the file type to return.
  File        Return pointer.  In the case of an error, contents are undefined.

Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_ABORTED             An error was encountered.
  EFI_INVALID_PARAMETER   One of the parameters was NULL.

--*/
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
    Error (NULL, 0, 0, "error parsing FV", NULL);
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
      Error (NULL, 0, 0, "error parsing the FV", NULL);
      return EFI_ABORTED;
    }
  }

  *File = NULL;
  return EFI_SUCCESS;
}

EFI_STATUS
SearchSectionByType (
  IN EFI_FILE_SECTION_POINTER  FirstSection,
  IN UINT8                     *SearchEnd,
  IN EFI_SECTION_TYPE          SectionType,
  IN OUT UINTN                 *StartIndex,
  IN UINTN                     Instance,
  OUT EFI_FILE_SECTION_POINTER *Section
  )
/*++

Routine Description:

  Helper function to search a sequence of sections from the section pointed
  by FirstSection to SearchEnd for the Instance-th section of type SectionType.
  The current counter is saved in StartIndex and when the section is found, it's
  saved in Section. GUID-defined sections, if special processing is not required,
  are searched recursively in a depth-first manner.

Arguments:

  FirstSection The first section to start searching from.
  SearchEnd    The end address to stop search.
  SectionType  The type of section to search.
  StartIndex   The current counter is saved.
  Instance     The requested n-th section number.
  Section      The found section returned.

Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_NOT_FOUND           The section is not found.
--*/
{
  EFI_FILE_SECTION_POINTER  CurrentSection;
  EFI_FILE_SECTION_POINTER  InnerSection;
  EFI_STATUS                Status;
  UINTN                     SectionSize;

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
    if (SectionType != EFI_SECTION_GUID_DEFINED &&
        CurrentSection.CommonHeader->Type == EFI_SECTION_GUID_DEFINED &&
        !(CurrentSection.GuidDefinedSection->Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED)) {
      InnerSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *)
        ((UINTN) CurrentSection.CommonHeader + CurrentSection.GuidDefinedSection->DataOffset);
      SectionSize = CurrentSection.CommonHeader->Size[0] +
        (CurrentSection.CommonHeader->Size[1] << 8) + 
        (CurrentSection.CommonHeader->Size[2] << 16);
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
    CurrentSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *) ((((UINTN) CurrentSection.CommonHeader) + GetLength (CurrentSection.CommonHeader->Size) + 0x03) & (-1 << 2));
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
GetSectionByType (
  IN EFI_FFS_FILE_HEADER          *File,
  IN EFI_SECTION_TYPE             SectionType,
  IN UINTN                        Instance,
  OUT EFI_FILE_SECTION_POINTER    *Section
  )
/*++

Routine Description:

  Find a section in a file by type and instance.  An instance of 1 is the first 
  instance.  The function will return NULL if a matching section cannot be found.
  GUID-defined sections, if special processing is not needed, are handled in a
  depth-first manner.

Arguments:

  File        The file to search.
  SectionType Type of file to search for.
  Instance    Instace of the section to return.
  Section     Return pointer.  In the case of an error, contents are undefined.

Returns:

  EFI_SUCCESS             The function completed successfully.
  EFI_ABORTED             An error was encountered.
  EFI_INVALID_PARAMETER   One of the parameters was NULL.
  EFI_NOT_FOUND           No found.
--*/
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
    Error (NULL, 0, 0, "invalid FFS file", NULL);
    return EFI_ABORTED;
  }
  //
  // Initialize the number of matching sections found.
  //
  SectionCount = 0;

  //
  // Get the first section
  //
  CurrentSection.CommonHeader = (EFI_COMMON_SECTION_HEADER *) ((UINTN) File + sizeof (EFI_FFS_FILE_HEADER));

  Status = SearchSectionByType (
             CurrentSection,
             (UINT8 *) ((UINTN) File + GetLength (File->Size)),
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
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER   *FvHeader
  )
/*++

Routine Description:

  Verify the current pointer points to a valid FV header.

Arguments:

  FvHeader     Pointer to an alleged FV file.

Returns:

  EFI_SUCCESS             The FV header is valid.
  EFI_VOLUME_CORRUPTED    The FV header is not valid.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_ABORTED             Operation aborted.

--*/
{
  UINT16  Checksum;

  //
  // Verify input parameters
  //
  if (FvHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (FvHeader->Signature != EFI_FVH_SIGNATURE) {
    Error (NULL, 0, 0, "invalid FV header signature", NULL);
    return EFI_VOLUME_CORRUPTED;
  }
  //
  // Verify header checksum
  //
  Checksum = CalculateSum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

  if (Checksum != 0) {
    Error (NULL, 0, 0, "invalid FV header checksum", NULL);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
VerifyFfsFile (
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:

  Verify the current pointer points to a FFS file header.

Arguments:

  FfsHeader     Pointer to an alleged FFS file.

Returns:

  EFI_SUCCESS           The Ffs header is valid.
  EFI_NOT_FOUND         This "file" is the beginning of free space.
  EFI_VOLUME_CORRUPTED  The Ffs header is not valid.
  EFI_ABORTED           The erase polarity is not known.

--*/
{
  BOOLEAN             ErasePolarity;
  EFI_STATUS          Status;
  EFI_FFS_FILE_HEADER BlankHeader;
  UINT8               Checksum;
  UINT32              FileLength;
  UINT32              OccupiedFileLength;
  UINT8               SavedChecksum;
  UINT8               SavedState;
  UINT8               FileGuidString[80];
  UINT32              TailSize;
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
  EFI_FFS_FILE_TAIL   *Tail;
#endif
  
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
  //
  // Check if we have free space
  //
  if (ErasePolarity) {
    memset (&BlankHeader, -1, sizeof (EFI_FFS_FILE_HEADER));
  } else {
    memset (&BlankHeader, 0, sizeof (EFI_FFS_FILE_HEADER));
  }

  if (memcmp (&BlankHeader, FfsHeader, sizeof (EFI_FFS_FILE_HEADER)) == 0) {
    return EFI_NOT_FOUND;
  }
  //
  // Convert the GUID to a string so we can at least report which file
  // if we find an error.
  //
  PrintGuidToBuffer (&FfsHeader->Name, FileGuidString, sizeof (FileGuidString), TRUE);
  if (FfsHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    TailSize = sizeof (EFI_FFS_FILE_TAIL);
  } else {
    TailSize = 0;
  }
  //
  // Verify file header checksum
  //
  SavedState = FfsHeader->State;
  FfsHeader->State = 0;
  SavedChecksum = FfsHeader->IntegrityCheck.Checksum.File;
  FfsHeader->IntegrityCheck.Checksum.File = 0;
  Checksum = CalculateSum8 ((UINT8 *) FfsHeader, sizeof (EFI_FFS_FILE_HEADER));
  FfsHeader->State = SavedState;
  FfsHeader->IntegrityCheck.Checksum.File = SavedChecksum;
  if (Checksum != 0) {
    Error (NULL, 0, 0, FileGuidString, "invalid FFS file header checksum");
    return EFI_ABORTED;
  }
  //
  // Verify file checksum
  //
  if (FfsHeader->Attributes & FFS_ATTRIB_CHECKSUM) {
    //
    // Verify file data checksum
    //
    FileLength          = GetLength (FfsHeader->Size);
    OccupiedFileLength  = (FileLength + 0x07) & (-1 << 3);
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
    Checksum            = CalculateSum8 ((UINT8 *) FfsHeader, FileLength - TailSize);
    Checksum            = (UINT8) (Checksum - FfsHeader->State);
#else
    Checksum            = CalculateSum8 ((UINT8 *) ((UINTN)FfsHeader + sizeof (EFI_FFS_FILE_HEADER)), FileLength - TailSize - sizeof (EFI_FFS_FILE_HEADER));
    Checksum            =  Checksum + (UINT8)FfsHeader->IntegrityCheck.Checksum.File;
#endif
    if (Checksum != 0) {
      Error (NULL, 0, 0, FileGuidString, "invalid FFS file checksum");
      return EFI_ABORTED;
    }
  } else {
    //
    // File does not have a checksum
    // Verify contents are 0x5A(Framework) and 0xAA(PI 1.0) as spec'd
    //
    if (FfsHeader->IntegrityCheck.Checksum.File != FFS_FIXED_CHECKSUM) {
      Error (NULL, 0, 0, FileGuidString, "invalid fixed file checksum");
      return EFI_ABORTED;
    }
  }
#if (PI_SPECIFICATION_VERSION < 0x00010000)  
  //
  // Check if the tail is present and verify it if it is.
  //
  if (FfsHeader->Attributes & FFS_ATTRIB_TAIL_PRESENT) {
    //
    // Verify tail is complement of integrity check field in the header.
    //
    Tail = (EFI_FFS_FILE_TAIL *) ((UINTN) FfsHeader + GetLength (FfsHeader->Size) - sizeof (EFI_FFS_FILE_TAIL));
    if (FfsHeader->IntegrityCheck.TailReference != (EFI_FFS_FILE_TAIL)~(*Tail)) {
      Error (NULL, 0, 0, FileGuidString, "invalid FFS file tail");
      return EFI_ABORTED;
    }
  }
#endif
  return EFI_SUCCESS;
}

UINT32
GetLength (
  UINT8     *ThreeByteLength
  )
/*++

Routine Description:

  Converts a three byte length value into a UINT32.

Arguments:

  ThreeByteLength   Pointer to the first of the 3 byte length.

Returns:

  UINT32      Size of the section

--*/
{
  UINT32  Length;

  if (ThreeByteLength == NULL) {
    return 0;
  }

  Length  = *((UINT32 *) ThreeByteLength);
  Length  = Length & 0x00FFFFFF;

  return Length;
}

EFI_STATUS
GetErasePolarity (
  OUT BOOLEAN   *ErasePolarity
  )
/*++

Routine Description:

  This function returns with the FV erase polarity.  If the erase polarity
  for a bit is 1, the function return TRUE.

Arguments:

  ErasePolarity   A pointer to the erase polarity.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              Operation aborted.
  
--*/
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

  if (mFvHeader->Attributes & EFI_FVB_ERASE_POLARITY) {
    *ErasePolarity = TRUE;
  } else {
    *ErasePolarity = FALSE;
  }

  return EFI_SUCCESS;
}

UINT8
GetFileState (
  IN BOOLEAN              ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
/*++

Routine Description:

  This function returns a the highest state bit in the FFS that is set.
  It in no way validate the FFS file.

Arguments:
  
  ErasePolarity The erase polarity for the file state bits.
  FfsHeader     Pointer to a FFS file.

Returns:

  UINT8   The hightest set state of the file.

--*/
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
