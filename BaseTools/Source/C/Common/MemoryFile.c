/** @file
This contains some useful functions for accessing files.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "CommonLib.h"
#include "MemoryFile.h"


//
// Local (static) function prototypes
//
STATIC
VOID
CheckMemoryFileState (
  IN EFI_HANDLE InputMemoryFile
  );

//
// Function implementations
//

EFI_STATUS
GetMemoryFile (
  IN CHAR8       *InputFileName,
  OUT EFI_HANDLE *OutputMemoryFile
  )
/*++

Routine Description:

  This opens a file, reads it into memory and returns a memory file
  object.

Arguments:

  InputFile          Memory file image.
  OutputMemoryFile   Handle to memory file

Returns:

  EFI_STATUS
  OutputMemoryFile is valid if !EFI_ERROR

--*/
{
  EFI_STATUS  Status;
  CHAR8       *InputFileImage;
  UINT32      BytesRead;
  MEMORY_FILE *NewMemoryFile;

  Status = GetFileImage (InputFileName, &InputFileImage, &BytesRead);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  NewMemoryFile = malloc (sizeof (*NewMemoryFile));
  if (NewMemoryFile == NULL) {
    free (InputFileImage);
    return EFI_OUT_OF_RESOURCES;
  }

  NewMemoryFile->FileImage           = InputFileImage;
  NewMemoryFile->CurrentFilePointer  = InputFileImage;
  NewMemoryFile->Eof                 = InputFileImage + BytesRead;

  *OutputMemoryFile = (EFI_HANDLE)NewMemoryFile;

  CheckMemoryFileState (*OutputMemoryFile);

  return EFI_SUCCESS;
}


EFI_STATUS
FreeMemoryFile (
  IN EFI_HANDLE InputMemoryFile
  )
/*++

Routine Description:

  Frees all memory associated with the input memory file.

Arguments:

  InputMemoryFile   Handle to memory file

Returns:

  EFI_STATUS

--*/
{
  MEMORY_FILE *MemoryFile;

  CheckMemoryFileState (InputMemoryFile);

  MemoryFile = (MEMORY_FILE*)InputMemoryFile;

  free (MemoryFile->FileImage);

  //
  // Invalidate state of MEMORY_FILE structure to catch invalid usage.
  //
  memset (MemoryFile, 0xcc, sizeof (*MemoryFile));
  MemoryFile->Eof -= 1;

  free (MemoryFile);

  return EFI_SUCCESS;
}


CHAR8 *
ReadMemoryFileLine (
  IN EFI_HANDLE     InputMemoryFile
  )
/*++

Routine Description:

  This function reads a line from the memory file.  The newline characters
  are stripped and a null terminated string is returned.

  If the string pointer returned is non-NULL, then the caller must free the
  memory associated with this string.

Arguments:

  InputMemoryFile   Handle to memory file

Returns:

  NULL if error or EOF
  NULL character termincated string otherwise (MUST BE FREED BY CALLER)

--*/
{
  CHAR8       *EndOfLine;
  UINTN       CharsToCopy;
  MEMORY_FILE *InputFile;
  UINTN       BytesToEof;
  CHAR8       *OutputString;

  //
  // Verify input parameters are not null
  //
  CheckMemoryFileState (InputMemoryFile);

  InputFile = (MEMORY_FILE*)InputMemoryFile;

  //
  // Check for end of file condition
  //
  if (InputFile->CurrentFilePointer >= InputFile->Eof) {
    return NULL;
  }

  //
  // Determine the number of bytes remaining until the EOF
  //
  BytesToEof = InputFile->Eof - InputFile->CurrentFilePointer;

  //
  // Find the next newline char
  //
  EndOfLine = memchr (InputFile->CurrentFilePointer, '\n', BytesToEof);

  //
  // Determine the number of characters to copy.
  //
  if (EndOfLine == 0) {
    //
    // If no newline found, copy to the end of the file.
    //
    CharsToCopy = InputFile->Eof - InputFile->CurrentFilePointer;
  } else {
    //
    // Newline found in the file.
    //
    CharsToCopy = EndOfLine - InputFile->CurrentFilePointer;
  }

  OutputString = malloc (CharsToCopy + 1);
  if (OutputString == NULL) {
    return NULL;
  }

  //
  // Copy the line.
  //
  memcpy (OutputString, InputFile->CurrentFilePointer, CharsToCopy);

  //
  // Add the null termination over the 0x0D
  //
  if (OutputString[CharsToCopy - 1] == '\r') {

    OutputString[CharsToCopy - 1] = '\0';

  } else {

    OutputString[CharsToCopy] = '\0';

  }

  //
  // Increment the current file pointer (include the 0x0A)
  //
  InputFile->CurrentFilePointer += CharsToCopy + 1;
  if (InputFile->CurrentFilePointer > InputFile->Eof) {
    InputFile->CurrentFilePointer = InputFile->Eof;
  }
  CheckMemoryFileState (InputMemoryFile);

  //
  // Return the string
  //
  return OutputString;
}


STATIC
VOID
CheckMemoryFileState (
  IN EFI_HANDLE InputMemoryFile
  )
{
  MEMORY_FILE *MemoryFile;

  assert (InputMemoryFile != NULL);

  MemoryFile = (MEMORY_FILE*)InputMemoryFile;

  assert (MemoryFile->FileImage != NULL);
  assert (MemoryFile->CurrentFilePointer != NULL);
  assert (MemoryFile->Eof != NULL);
  assert (MemoryFile->Eof >= MemoryFile->FileImage);
  assert (MemoryFile->CurrentFilePointer >= MemoryFile->FileImage);
  assert (MemoryFile->CurrentFilePointer <= MemoryFile->Eof);
}


