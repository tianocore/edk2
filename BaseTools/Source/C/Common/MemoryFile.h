/** @file
Header file for helper functions useful for accessing files.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_MEMORY_FILE_H
#define _EFI_MEMORY_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <Common/UefiBaseTypes.h>

//
// Common data structures
//
typedef struct {
  CHAR8 *FileImage;
  CHAR8 *Eof;
  CHAR8 *CurrentFilePointer;
} MEMORY_FILE;


//
// Functions declarations
//

/**
  This opens a file, reads it into memory and returns a memory file
  object.

  @param InputFile          Memory file image.
  @param OutputMemoryFile   Handle to memory file

  @return EFI_STATUS
  OutputMemoryFile is valid if !EFI_ERROR
**/
EFI_STATUS
GetMemoryFile (
  IN CHAR8       *InputFileName,
  OUT EFI_HANDLE *OutputMemoryFile
  )
;

/**
  Frees all memory associated with the input memory file.

  @param InputMemoryFile   Handle to memory file

  @return EFI_STATUS
**/
EFI_STATUS
FreeMemoryFile (
  IN EFI_HANDLE InputMemoryFile
  )
;

/**
  This function reads a line from the memory file.  The newline characters
  are stripped and a null terminated string is returned.

  If the string pointer returned is non-NULL, then the caller must free the
  memory associated with this string.

  @param InputMemoryFile   Handle to memory file

  @retval NULL if error or EOF
  @retval NULL character termincated string otherwise (MUST BE FREED BY CALLER)
**/
CHAR8 *
ReadMemoryFileLine (
  IN EFI_HANDLE     InputMemoryFile
  )
;


#endif
