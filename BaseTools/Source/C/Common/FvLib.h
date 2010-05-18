/** @file

Copyright (c) 2004 - 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FvLib.h

Abstract:

  These functions assist in parsing and manipulating a Firmware Volume.

**/

#ifndef _EFI_FV_LIB_H
#define _EFI_FV_LIB_H

//
// Include files
//
#include <string.h>

#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>
#include <Common/PiFirmwareVolume.h>

EFI_STATUS
InitializeFvLib (
  IN VOID                         *Fv,
  IN UINT32                       FvLength
  )
;

EFI_STATUS
GetFvHeader (
  OUT EFI_FIRMWARE_VOLUME_HEADER  **FvHeader,
  OUT UINT32                      *FvLength
  )
;

EFI_STATUS
GetNextFile (
  IN EFI_FFS_FILE_HEADER          *CurrentFile,
  OUT EFI_FFS_FILE_HEADER         **NextFile
  )
;

EFI_STATUS
GetFileByName (
  IN EFI_GUID                     *FileName,
  OUT EFI_FFS_FILE_HEADER         **File
  )
;

EFI_STATUS
GetFileByType (
  IN EFI_FV_FILETYPE              FileType,
  IN UINTN                        Instance,
  OUT EFI_FFS_FILE_HEADER         **File
  )
;

EFI_STATUS
GetSectionByType (
  IN EFI_FFS_FILE_HEADER          *File,
  IN EFI_SECTION_TYPE             SectionType,
  IN UINTN                        Instance,
  OUT EFI_FILE_SECTION_POINTER    *Section
  )
;
//
// will not parse compressed sections
//
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER   *FvHeader
  )
;

EFI_STATUS
VerifyFfsFile (
  IN EFI_FFS_FILE_HEADER          *FfsHeader
  )
;

/*++

Routine Description:

  Verify the current pointer points to a FFS file header.

Arguments:

  FfsHeader     Pointer to an alleged FFS file.

Returns:

  EFI_SUCCESS           The Ffs header is valid.
  EFI_NOT_FOUND         This "file" is the beginning of free space.
  EFI_VOLUME_CORRUPTED  The Ffs header is not valid.

--*/
UINT32
GetLength (
  UINT8                           *ThreeByteLength
  )
;

/*++

Routine Description:

  Converts a three byte length value into a UINT32.

Arguments:

  ThreeByteLength   Pointer to the first of the 3 byte length.

Returns:

  UINT32      Size of the section

--*/
EFI_STATUS
GetErasePolarity (
  OUT BOOLEAN   *ErasePolarity
  )
;

/*++

Routine Description:

  This function returns with the FV erase polarity.  If the erase polarity
  for a bit is 1, the function return TRUE.

Arguments:

  ErasePolarity   A pointer to the erase polarity.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.

--*/
UINT8
GetFileState (
  IN BOOLEAN              ErasePolarity,
  IN EFI_FFS_FILE_HEADER  *FfsHeader
  )
;

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
#endif
