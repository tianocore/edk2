/** @file
Common basic Library Functions

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef __GNUC__
#include <unistd.h>
#else
#include <direct.h>
#endif
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

#define SAFE_STRING_CONSTRAINT_CHECK(Expression, Status)  \
  do { \
    ASSERT (Expression); \
    if (!(Expression)) { \
      return Status; \
    } \
  } while (FALSE)

VOID
PeiZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
/*++

Routine Description:

  Set Buffer to zero for Size bytes.

Arguments:

  Buffer  - Memory to set.

  Size    - Number of bytes to set

Returns:

  None

--*/
{
  INT8  *Ptr;

  Ptr = Buffer;
  while (Size--) {
    *(Ptr++) = 0;
  }
}

VOID
PeiCopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
/*++

Routine Description:

  Copy Length bytes from Source to Destination.

Arguments:

  Destination - Target of copy

  Source      - Place to copy from

  Length      - Number of bytes to copy

Returns:

  None

--*/
{
  CHAR8 *Destination8;
  CHAR8 *Source8;

  Destination8  = Destination;
  Source8       = Source;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }
}

VOID
ZeroMem (
  IN VOID   *Buffer,
  IN UINTN  Size
  )
{
  PeiZeroMem (Buffer, Size);
}

VOID
CopyMem (
  IN VOID   *Destination,
  IN VOID   *Source,
  IN UINTN  Length
  )
{
  PeiCopyMem (Destination, Source, Length);
}

INTN
CompareGuid (
  IN EFI_GUID     *Guid1,
  IN EFI_GUID     *Guid2
  )
/*++

Routine Description:

  Compares to GUIDs

Arguments:

  Guid1 - guid to compare
  Guid2 - guid to compare

Returns:
  =  0  if Guid1 == Guid2
  != 0  if Guid1 != Guid2

--*/
{
  INT32 *g1;
  INT32 *g2;
  INT32 r;

  //
  // Compare 32 bits at a time
  //
  g1  = (INT32 *) Guid1;
  g2  = (INT32 *) Guid2;

  r   = g1[0] - g2[0];
  r |= g1[1] - g2[1];
  r |= g1[2] - g2[2];
  r |= g1[3] - g2[3];

  return r;
}


EFI_STATUS
GetFileImage (
  IN CHAR8    *InputFileName,
  OUT CHAR8   **InputFileImage,
  OUT UINT32  *BytesRead
  )
/*++

Routine Description:

  This function opens a file and reads it into a memory buffer.  The function
  will allocate the memory buffer and returns the size of the buffer.

Arguments:

  InputFileName     The name of the file to read.
  InputFileImage    A pointer to the memory buffer.
  BytesRead         The size of the memory buffer.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     No resource to complete operations.

--*/
{
  FILE    *InputFile;
  UINT32  FileSize;

  //
  // Verify input parameters.
  //
  if (InputFileName == NULL || strlen (InputFileName) == 0 || InputFileImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Open the file and copy contents into a memory buffer.
  //
  //
  // Open the file
  //
  InputFile = fopen (LongFilePath (InputFileName), "rb");
  if (InputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening the input file", InputFileName);
    return EFI_ABORTED;
  }
  //
  // Go to the end so that we can determine the file size
  //
  if (fseek (InputFile, 0, SEEK_END)) {
    Error (NULL, 0, 0004, "Error reading the input file", InputFileName);
    fclose (InputFile);
    return EFI_ABORTED;
  }
  //
  // Get the file size
  //
  FileSize = ftell (InputFile);
  if (FileSize == -1) {
    Error (NULL, 0, 0003, "Error parsing the input file", InputFileName);
    fclose (InputFile);
    return EFI_ABORTED;
  }
  //
  // Allocate a buffer
  //
  *InputFileImage = malloc (FileSize);
  if (*InputFileImage == NULL) {
    fclose (InputFile);
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Reset to the beginning of the file
  //
  if (fseek (InputFile, 0, SEEK_SET)) {
    Error (NULL, 0, 0004, "Error reading the input file", InputFileName);
    fclose (InputFile);
    free (*InputFileImage);
    *InputFileImage = NULL;
    return EFI_ABORTED;
  }
  //
  // Read all of the file contents.
  //
  *BytesRead = fread (*InputFileImage, sizeof (UINT8), FileSize, InputFile);
  if (*BytesRead != sizeof (UINT8) * FileSize) {
    Error (NULL, 0, 0004, "Error reading the input file", InputFileName);
    fclose (InputFile);
    free (*InputFileImage);
    *InputFileImage = NULL;
    return EFI_ABORTED;
  }
  //
  // Close the file
  //
  fclose (InputFile);

  return EFI_SUCCESS;
}

EFI_STATUS
PutFileImage (
  IN CHAR8    *OutputFileName,
  IN CHAR8    *OutputFileImage,
  IN UINT32   BytesToWrite
  )
/*++

Routine Description:

  This function opens a file and writes OutputFileImage into the file.

Arguments:

  OutputFileName     The name of the file to write.
  OutputFileImage    A pointer to the memory buffer.
  BytesToWrite       The size of the memory buffer.

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     No resource to complete operations.

--*/
{
  FILE    *OutputFile;
  UINT32  BytesWrote;

  //
  // Verify input parameters.
  //
  if (OutputFileName == NULL || strlen (OutputFileName) == 0 || OutputFileImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Open the file and copy contents into a memory buffer.
  //
  //
  // Open the file
  //
  OutputFile = fopen (LongFilePath (OutputFileName), "wb");
  if (OutputFile == NULL) {
    Error (NULL, 0, 0001, "Error opening the output file", OutputFileName);
    return EFI_ABORTED;
  }

  //
  // Write all of the file contents.
  //
  BytesWrote = fwrite (OutputFileImage, sizeof (UINT8), BytesToWrite, OutputFile);
  if (BytesWrote != sizeof (UINT8) * BytesToWrite) {
    Error (NULL, 0, 0002, "Error writing the output file", OutputFileName);
    fclose (OutputFile);
    return EFI_ABORTED;
  }
  //
  // Close the file
  //
  fclose (OutputFile);

  return EFI_SUCCESS;
}

UINT8
CalculateChecksum8 (
  IN UINT8        *Buffer,
  IN UINTN        Size
  )
/*++

Routine Description:

  This function calculates the value needed for a valid UINT8 checksum

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 8 bit checksum value needed.

--*/
{
  return (UINT8) (0x100 - CalculateSum8 (Buffer, Size));
}

UINT8
CalculateSum8 (
  IN UINT8  *Buffer,
  IN UINTN  Size
  )
/*++

Routine Description::

  This function calculates the UINT8 sum for the requested region.

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 8 bit checksum value needed.

--*/
{
  UINTN Index;
  UINT8 Sum;

  Sum = 0;

  //
  // Perform the byte sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT8) (Sum + Buffer[Index]);
  }

  return Sum;
}

UINT16
CalculateChecksum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
/*++

Routine Description::

  This function calculates the value needed for a valid UINT16 checksum

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 16 bit checksum value needed.

--*/
{
  return (UINT16) (0x10000 - CalculateSum16 (Buffer, Size));
}

UINT16
CalculateSum16 (
  IN UINT16       *Buffer,
  IN UINTN        Size
  )
/*++

Routine Description:

  This function calculates the UINT16 sum for the requested region.

Arguments:

  Buffer      Pointer to buffer containing byte data of component.
  Size        Size of the buffer

Returns:

  The 16 bit checksum

--*/
{
  UINTN   Index;
  UINT16  Sum;

  Sum = 0;

  //
  // Perform the word sum for buffer
  //
  for (Index = 0; Index < Size; Index++) {
    Sum = (UINT16) (Sum + Buffer[Index]);
  }

  return (UINT16) Sum;
}

EFI_STATUS
PrintGuid (
  IN EFI_GUID *Guid
  )
/*++

Routine Description:

  This function prints a GUID to STDOUT.

Arguments:

  Guid    Pointer to a GUID to print.

Returns:

  EFI_SUCCESS             The GUID was printed.
  EFI_INVALID_PARAMETER   The input was NULL.

--*/
{
  if (Guid == NULL) {
    Error (NULL, 0, 2000, "Invalid parameter", "PrintGuidToBuffer() called with a NULL value");
    return EFI_INVALID_PARAMETER;
  }

  printf (
    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
    (unsigned) Guid->Data1,
    Guid->Data2,
    Guid->Data3,
    Guid->Data4[0],
    Guid->Data4[1],
    Guid->Data4[2],
    Guid->Data4[3],
    Guid->Data4[4],
    Guid->Data4[5],
    Guid->Data4[6],
    Guid->Data4[7]
    );
  return EFI_SUCCESS;
}

EFI_STATUS
PrintGuidToBuffer (
  IN EFI_GUID     *Guid,
  IN OUT UINT8    *Buffer,
  IN UINT32       BufferLen,
  IN BOOLEAN      Uppercase
  )
/*++

Routine Description:

  This function prints a GUID to a buffer

Arguments:

  Guid      - Pointer to a GUID to print.
  Buffer    - Pointer to a user-provided buffer to print to
  BufferLen - Size of the Buffer
  Uppercase - If use upper case.

Returns:

  EFI_SUCCESS             The GUID was printed.
  EFI_INVALID_PARAMETER   The input was NULL.
  EFI_BUFFER_TOO_SMALL    The input buffer was not big enough

--*/
{
  if (Guid == NULL) {
    Error (NULL, 0, 2000, "Invalid parameter", "PrintGuidToBuffer() called with a NULL value");
    return EFI_INVALID_PARAMETER;
  }

  if (BufferLen < PRINTED_GUID_BUFFER_SIZE) {
    Error (NULL, 0, 2000, "Invalid parameter", "PrintGuidToBuffer() called with invalid buffer size");
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Uppercase) {
    sprintf (
      (CHAR8 *)Buffer,
      "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      (unsigned) Guid->Data1,
      Guid->Data2,
      Guid->Data3,
      Guid->Data4[0],
      Guid->Data4[1],
      Guid->Data4[2],
      Guid->Data4[3],
      Guid->Data4[4],
      Guid->Data4[5],
      Guid->Data4[6],
      Guid->Data4[7]
      );
  } else {
    sprintf (
      (CHAR8 *)Buffer,
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      (unsigned) Guid->Data1,
      Guid->Data2,
      Guid->Data3,
      Guid->Data4[0],
      Guid->Data4[1],
      Guid->Data4[2],
      Guid->Data4[3],
      Guid->Data4[4],
      Guid->Data4[5],
      Guid->Data4[6],
      Guid->Data4[7]
      );
  }

  return EFI_SUCCESS;
}

#ifdef __GNUC__

size_t _filelength(int fd)
{
  struct stat stat_buf;
  fstat(fd, &stat_buf);
  return stat_buf.st_size;
}

#ifndef __CYGWIN__
char *strlwr(char *s)
{
  char *p = s;
  for(;*s;s++) {
    *s = tolower(*s);
  }
  return p;
}
#endif
#endif

#define WINDOWS_EXTENSION_PATH "\\\\?\\"
#define WINDOWS_UNC_EXTENSION_PATH "\\\\?\\UNC"

//
// Global data to store full file path. It is not required to be free.
//
CHAR8 mCommonLibFullPath[MAX_LONG_FILE_PATH];

CHAR8 *
LongFilePath (
 IN CHAR8 *FileName
 )
/*++

Routine Description:
  Convert FileName to the long file path, which can support larger than 260 length.

Arguments:
  FileName         - FileName.

Returns:
  LongFilePath      A pointer to the converted long file path.

--*/
{
#ifdef __GNUC__
  //
  // __GNUC__ may not be good way to differentiate unix and windows. Need more investigation here.
  // unix has no limitation on file path. Just return FileName.
  //
  return FileName;
#else
  CHAR8 *RootPath;
  CHAR8 *PathPointer;
  CHAR8 *NextPointer;

  PathPointer = (CHAR8 *) FileName;

  if (FileName != NULL) {
    //
    // Add the extension string first to support long file path.
    //
    mCommonLibFullPath[0] = 0;
    strcpy (mCommonLibFullPath, WINDOWS_EXTENSION_PATH);

    if (strlen (FileName) > 1 && FileName[0] == '\\' && FileName[1] == '\\') {
      //
      // network path like \\server\share to \\?\UNC\server\share
      //
      strcpy (mCommonLibFullPath, WINDOWS_UNC_EXTENSION_PATH);
      FileName ++;
    } else if (strlen (FileName) < 3 || FileName[1] != ':' || (FileName[2] != '\\' && FileName[2] != '/')) {
      //
      // Relative file path. Convert it to absolute path.
      //
      RootPath = getcwd (NULL, 0);
      if (RootPath != NULL) {
        if (strlen (mCommonLibFullPath) + strlen (RootPath) > MAX_LONG_FILE_PATH - 1) {
          Error (NULL, 0, 2000, "Invalid parameter", "RootPath is too long!");
          free (RootPath);
          return NULL;
        }
        strncat (mCommonLibFullPath, RootPath, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
        if (FileName[0] != '\\' && FileName[0] != '/') {
          if (strlen (mCommonLibFullPath) + 1 > MAX_LONG_FILE_PATH - 1) {
            Error (NULL, 0, 2000, "Invalid parameter", "RootPath is too long!");
            free (RootPath);
            return NULL;
          }
          //
          // Attach directory separator
          //
          strncat (mCommonLibFullPath, "\\", MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
        }
        free (RootPath);
      }
    }

    //
    // Construct the full file path
    //
    if (strlen (mCommonLibFullPath) + strlen (FileName) > MAX_LONG_FILE_PATH - 1) {
      Error (NULL, 0, 2000, "Invalid parameter", "FileName %s is too long!", FileName);
      return NULL;
    }
    strncat (mCommonLibFullPath, FileName, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);

    //
    // Convert directory separator '/' to '\\'
    //
    PathPointer = (CHAR8 *) mCommonLibFullPath;
    do {
      if (*PathPointer == '/') {
        *PathPointer = '\\';
      }
    } while (*PathPointer ++ != '\0');

    //
    // Convert ":\\\\" to ":\\", because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    if ((PathPointer = strstr (mCommonLibFullPath, ":\\\\")) != NULL) {
      *(PathPointer + 2) = '\0';
      strncat (mCommonLibFullPath, PathPointer + 3, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
    }

    //
    // Convert ".\\" to "", because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    while ((PathPointer = strstr (mCommonLibFullPath, ".\\")) != NULL) {
      *PathPointer = '\0';
      strncat (mCommonLibFullPath, PathPointer + 2, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
    }

    //
    // Convert "\\.\\" to "\\", because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    while ((PathPointer = strstr (mCommonLibFullPath, "\\.\\")) != NULL) {
      *PathPointer = '\0';
      strncat (mCommonLibFullPath, PathPointer + 2, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
    }

    //
    // Convert "\\..\\" to last directory, because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    while ((PathPointer = strstr (mCommonLibFullPath, "\\..\\")) != NULL) {
      NextPointer = PathPointer + 3;
      do {
        PathPointer --;
      } while (PathPointer > mCommonLibFullPath && *PathPointer != ':' && *PathPointer != '\\');

      if (*PathPointer == '\\') {
        //
        // Skip one directory
        //
        *PathPointer = '\0';
        strncat (mCommonLibFullPath, NextPointer, MAX_LONG_FILE_PATH - strlen (mCommonLibFullPath) - 1);
      } else {
        //
        // No directory is found. Just break.
        //
        break;
      }
    }

    PathPointer = mCommonLibFullPath;
  }

  return PathPointer;
#endif
}

CHAR16
InternalCharToUpper (
        CHAR16                    Char
  )
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

UINTN
StrnLenS (
   CONST CHAR16              *String,
   UINTN                     MaxSize
  )
{
  UINTN     Length;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // If String is a null pointer or MaxSize is 0, then the StrnLenS function returns zero.
  //
  if ((String == NULL) || (MaxSize == 0)) {
    return 0;
  }

  Length = 0;
  while (String[Length] != 0) {
    if (Length >= MaxSize - 1) {
      return MaxSize;
    }
    Length++;
  }
  return Length;
}


VOID *
InternalAllocatePool (
   UINTN   AllocationSize
  )
{
  VOID * Memory;

  Memory = malloc(AllocationSize);
  ASSERT(Memory != NULL);
  return Memory;
}


VOID *
InternalReallocatePool (
   UINTN            OldSize,
   UINTN            NewSize,
   VOID             *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = AllocateZeroPool (NewSize);
  if (NewBuffer != NULL && OldBuffer != NULL) {
    memcpy (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
    free(OldBuffer);
  }
  return NewBuffer;
}

VOID *
ReallocatePool (
   UINTN  OldSize,
   UINTN  NewSize,
   VOID   *OldBuffer  OPTIONAL
  )
{
  return InternalReallocatePool (OldSize, NewSize, OldBuffer);
}

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  String  A pointer to a Null-terminated Unicode string.

  @return The length of String.

**/
UINTN
StrLen (
  CONST CHAR16              *String
  )
{
  UINTN   Length;

  ASSERT (String != NULL);
  ASSERT (((UINTN) String & BIT0) == 0);

  for (Length = 0; *String != L'\0'; String++, Length++) {
    //
    // If PcdMaximumUnicodeStringLength is not zero,
    // length should not more than PcdMaximumUnicodeStringLength
    //
  }
  return Length;
}

BOOLEAN
InternalSafeStringIsOverlap (
  IN VOID    *Base1,
  IN UINTN   Size1,
  IN VOID    *Base2,
  IN UINTN   Size2
  )
{
  if ((((UINTN)Base1 >= (UINTN)Base2) && ((UINTN)Base1 < (UINTN)Base2 + Size2)) ||
      (((UINTN)Base2 >= (UINTN)Base1) && ((UINTN)Base2 < (UINTN)Base1 + Size1))) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
InternalSafeStringNoStrOverlap (
  IN CHAR16  *Str1,
  IN UINTN   Size1,
  IN CHAR16  *Str2,
  IN UINTN   Size2
  )
{
  return !InternalSafeStringIsOverlap (Str1, Size1 * sizeof(CHAR16), Str2, Size2 * sizeof(CHAR16));
}

/**
  Convert a Null-terminated Unicode decimal string to a value of type UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Unicode string specified by String as a decimal number. The format of the
  input Unicode string String is:

                  [spaces] [decimal digits].

  The valid decimal digit character is in the range [0-9]. The function will
  ignore the pad space, which includes spaces or tab characters, before
  [decimal digits]. The running zero in the beginning of [decimal digits] will
  be ignored. Then, the function stops at the first character that is a not a
  valid decimal character or a Null-terminator, whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid decimal digits in the above format, then 0 is stored
  at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  decimal digits right after the optional pad spaces, the value of String is
  stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
StrDecimalToUint64S (
    CONST CHAR16             *String,
         CHAR16             **EndPointer,  OPTIONAL
         UINT64             *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  *Data = 0;

  while (InternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
    if (*Data > ((MAX_UINT64 - (*String - L'0'))/10)) {
      *Data = MAX_UINT64;
      if (EndPointer != NULL) {
        *EndPointer = (CHAR16 *) String;
      }
      return RETURN_UNSUPPORTED;
    }

    *Data = (*Data) * 10 + (*String - L'0');
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type
  UINT64.

  This function outputs a value of type UINT64 by interpreting the contents of
  the Unicode string specified by String as a hexadecimal number. The format of
  the input Unicode string String is:

                  [spaces][zeros][x][hexadecimal digits].

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F].
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix.
  If "x" appears in the input string, it must be prefixed with at least one 0.
  The function will ignore the pad space, which includes spaces or tab
  characters, before [zeros], [x] or [hexadecimal digit]. The running zero
  before [x] or [hexadecimal digit] will be ignored. Then, the decoding starts
  after [x] or the first valid hexadecimal digit. Then, the function stops at
  the first character that is a not a valid hexadecimal character or NULL,
  whichever one comes first.

  If String is NULL, then ASSERT().
  If Data is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  If String has no valid hexadecimal digits in the above format, then 0 is
  stored at the location pointed to by Data.
  If the number represented by String exceeds the range defined by UINT64, then
  MAX_UINT64 is stored at the location pointed to by Data.

  If EndPointer is not NULL, a pointer to the character that stopped the scan
  is stored at the location pointed to by EndPointer. If String has no valid
  hexadecimal digits right after the optional pad spaces, the value of String
  is stored at the location pointed to by EndPointer.

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  EndPointer               Pointer to character that stops scan.
  @param  Data                     Pointer to the converted value.

  @retval RETURN_SUCCESS           Value is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If PcdMaximumUnicodeStringLength is not
                                   zero, and String contains more than
                                   PcdMaximumUnicodeStringLength Unicode
                                   characters, not including the
                                   Null-terminator.
  @retval RETURN_UNSUPPORTED       If the number represented by String exceeds
                                   the range defined by UINT64.

**/
RETURN_STATUS
StrHexToUint64S (
    CONST CHAR16             *String,
         CHAR16             **EndPointer,  OPTIONAL
         UINT64             *Data
  )
{
  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. Neither String nor Data shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Data != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. The length of String shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((StrnLenS (String, RSIZE_MAX + 1) <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalCharToUpper (*String) == L'X') {
    if (*(String - 1) != L'0') {
      *Data = 0;
      return RETURN_SUCCESS;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  *Data = 0;

  while (InternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according to the range
    // defined by UINT64, then MAX_UINT64 is stored in *Data and
    // RETURN_UNSUPPORTED is returned.
    //
    if (*Data > ((MAX_UINT64 - InternalHexCharToUintn (*String))>>4)) {
      *Data = MAX_UINT64;
      if (EndPointer != NULL) {
        *EndPointer = (CHAR16 *) String;
      }
      return RETURN_UNSUPPORTED;
    }

    *Data =  ((*Data) << 4) + InternalHexCharToUintn (*String);
    String++;
  }

  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) String;
  }
  return RETURN_SUCCESS;
}

UINT64
StrDecimalToUint64 (
  CONST CHAR16              *String
  )
{
  UINT64     Result;

  StrDecimalToUint64S (String, (CHAR16 **) NULL, &Result);
  return Result;
}


UINT64
StrHexToUint64 (
  CONST CHAR16             *String
  )
{
  UINT64    Result;

  StrHexToUint64S (String, (CHAR16 **) NULL, &Result);
  return Result;
}

UINTN
StrSize (
  CONST CHAR16              *String
  )
{
  return (StrLen (String) + 1) * sizeof (*String);
}


UINT64
ReadUnaligned64 (
   CONST UINT64              *Buffer
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer;
}

UINT64
WriteUnaligned64 (
   UINT64                    *Buffer,
   UINT64                    Value
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer = Value;
}


EFI_GUID *
CopyGuid (
   EFI_GUID         *DestinationGuid,
   CONST EFI_GUID  *SourceGuid
  )
{
  WriteUnaligned64 (
    (UINT64*)DestinationGuid,
    ReadUnaligned64 ((CONST UINT64*)SourceGuid)
    );
  WriteUnaligned64 (
    (UINT64*)DestinationGuid + 1,
    ReadUnaligned64 ((CONST UINT64*)SourceGuid + 1)
    );
  return DestinationGuid;
}

UINT16
SwapBytes16 (
  UINT16                    Value
  )
{
  return (UINT16) ((Value<< 8) | (Value>> 8));
}


UINT32
SwapBytes32 (
  UINT32                    Value
  )
{
  UINT32  LowerBytes;
  UINT32  HigherBytes;

  LowerBytes  = (UINT32) SwapBytes16 ((UINT16) Value);
  HigherBytes = (UINT32) SwapBytes16 ((UINT16) (Value >> 16));
  return (LowerBytes << 16 | HigherBytes);
}

BOOLEAN
InternalIsDecimalDigitCharacter (
  CHAR16                    Char
  )
{
  return (BOOLEAN) (Char >= L'0' && Char <= L'9');
}

VOID *
InternalAllocateCopyPool (
   UINTN            AllocationSize,
   CONST VOID       *Buffer
  )
{
  VOID  *Memory;

  ASSERT (Buffer != NULL);

  Memory = malloc (AllocationSize);
  if (Memory != NULL) {
     Memory = memcpy (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

BOOLEAN
InternalIsHexaDecimalDigitCharacter (
  CHAR16                    Char
  )
{

  return (BOOLEAN) (InternalIsDecimalDigitCharacter (Char) ||
    (Char >= L'A' && Char <= L'F') ||
    (Char >= L'a' && Char <= L'f'));
}

UINTN
InternalHexCharToUintn (
        CHAR16                    Char
  )
{
  if (InternalIsDecimalDigitCharacter (Char)) {
    return Char - L'0';
  }

  return (10 + InternalCharToUpper (Char) - L'A');
}


/**
  Convert a Null-terminated Unicode hexadecimal string to a byte array.

  This function outputs a byte array by interpreting the contents of
  the Unicode string specified by String in hexadecimal format. The format of
  the input Unicode string String is:

                  [XX]*

  X is a hexadecimal digit character in the range [0-9], [a-f] and [A-F].
  The function decodes every two hexadecimal digit characters as one byte. The
  decoding stops after Length of characters and outputs Buffer containing
  (Length / 2) bytes.

  If String is not aligned in a 16-bit boundary, then ASSERT().

  If String is NULL, then ASSERT().

  If Buffer is NULL, then ASSERT().

  If Length is not multiple of 2, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().

  If MaxBufferSize is less than (Length / 2), then ASSERT().

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  Length                   The number of Unicode characters to decode.
  @param  Buffer                   Pointer to the converted bytes array.
  @param  MaxBufferSize            The maximum size of Buffer.

  @retval RETURN_SUCCESS           Buffer is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
                                   If Length is not multiple of 2.
                                   If PcdMaximumUnicodeStringLength is not zero,
                                    and Length is greater than
                                    PcdMaximumUnicodeStringLength.
  @retval RETURN_UNSUPPORTED       If Length of characters from String contain
                                    a character that is not valid hexadecimal
                                    digit characters, or a Null-terminator.
  @retval RETURN_BUFFER_TOO_SMALL  If MaxBufferSize is less than (Length / 2).
**/
RETURN_STATUS
StrHexToBytes (
   CONST CHAR16       *String,
   UINTN              Length,
   UINT8              *Buffer,
   UINTN              MaxBufferSize
  )
{
  UINTN                  Index;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. None of String or Buffer shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Buffer != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. Length shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((Length <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. Length shall not be odd.
  //
  SAFE_STRING_CONSTRAINT_CHECK (((Length & BIT0) == 0), RETURN_INVALID_PARAMETER);

  //
  // 4. MaxBufferSize shall equal to or greater than Length / 2.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((MaxBufferSize >= Length / 2), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. String shall not contains invalid hexadecimal digits.
  //
  for (Index = 0; Index < Length; Index++) {
    if (!InternalIsHexaDecimalDigitCharacter (String[Index])) {
      break;
    }
  }
  if (Index != Length) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Convert the hex string to bytes.
  //
  for(Index = 0; Index < Length; Index++) {

    //
    // For even characters, write the upper nibble for each buffer byte,
    // and for even characters, the lower nibble.
    //
    if ((Index & BIT0) == 0) {
      Buffer[Index / 2]  = (UINT8) InternalHexCharToUintn (String[Index]) << 4;
    } else {
      Buffer[Index / 2] |= (UINT8) InternalHexCharToUintn (String[Index]);
    }
  }
  return RETURN_SUCCESS;
}

/**
  Convert a Null-terminated Unicode GUID string to a value of type
  EFI_GUID.

  This function outputs a GUID value by interpreting the contents of
  the Unicode string specified by String. The format of the input
  Unicode string String consists of 36 characters, as follows:

                  aabbccdd-eeff-gghh-iijj-kkllmmnnoopp

  The pairs aa - pp are two characters in the range [0-9], [a-f] and
  [A-F], with each pair representing a single byte hexadecimal value.

  The mapping between String and the EFI_GUID structure is as follows:
                  aa          Data1[24:31]
                  bb          Data1[16:23]
                  cc          Data1[8:15]
                  dd          Data1[0:7]
                  ee          Data2[8:15]
                  ff          Data2[0:7]
                  gg          Data3[8:15]
                  hh          Data3[0:7]
                  ii          Data4[0:7]
                  jj          Data4[8:15]
                  kk          Data4[16:23]
                  ll          Data4[24:31]
                  mm          Data4[32:39]
                  nn          Data4[40:47]
                  oo          Data4[48:55]
                  pp          Data4[56:63]

  If String is NULL, then ASSERT().
  If Guid is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().

  @param  String                   Pointer to a Null-terminated Unicode string.
  @param  Guid                     Pointer to the converted GUID.

  @retval RETURN_SUCCESS           Guid is translated from String.
  @retval RETURN_INVALID_PARAMETER If String is NULL.
                                   If Data is NULL.
  @retval RETURN_UNSUPPORTED       If String is not as the above format.

**/
RETURN_STATUS
StrToGuid (
   CONST CHAR16       *String,
   EFI_GUID           *Guid
  )
{
  RETURN_STATUS          Status;
  EFI_GUID               LocalGuid;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. None of String or Guid shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Guid != NULL), RETURN_INVALID_PARAMETER);

  //
  // Get aabbccdd in big-endian.
  //
  Status = StrHexToBytes (String, 2 * sizeof (LocalGuid.Data1), (UINT8 *) &LocalGuid.Data1, sizeof (LocalGuid.Data1));
  if (RETURN_ERROR (Status) || String[2 * sizeof (LocalGuid.Data1)] != L'-') {
    return RETURN_UNSUPPORTED;
  }
  //
  // Convert big-endian to little-endian.
  //
  LocalGuid.Data1 = SwapBytes32 (LocalGuid.Data1);
  String += 2 * sizeof (LocalGuid.Data1) + 1;

  //
  // Get eeff in big-endian.
  //
  Status = StrHexToBytes (String, 2 * sizeof (LocalGuid.Data2), (UINT8 *) &LocalGuid.Data2, sizeof (LocalGuid.Data2));
  if (RETURN_ERROR (Status) || String[2 * sizeof (LocalGuid.Data2)] != L'-') {
    return RETURN_UNSUPPORTED;
  }
  //
  // Convert big-endian to little-endian.
  //
  LocalGuid.Data2 = SwapBytes16 (LocalGuid.Data2);
  String += 2 * sizeof (LocalGuid.Data2) + 1;

  //
  // Get gghh in big-endian.
  //
  Status = StrHexToBytes (String, 2 * sizeof (LocalGuid.Data3), (UINT8 *) &LocalGuid.Data3, sizeof (LocalGuid.Data3));
  if (RETURN_ERROR (Status) || String[2 * sizeof (LocalGuid.Data3)] != L'-') {
    return RETURN_UNSUPPORTED;
  }
  //
  // Convert big-endian to little-endian.
  //
  LocalGuid.Data3 = SwapBytes16 (LocalGuid.Data3);
  String += 2 * sizeof (LocalGuid.Data3) + 1;

  //
  // Get iijj.
  //
  Status = StrHexToBytes (String, 2 * 2, &LocalGuid.Data4[0], 2);
  if (RETURN_ERROR (Status) || String[2 * 2] != L'-') {
    return RETURN_UNSUPPORTED;
  }
  String += 2 * 2 + 1;

  //
  // Get kkllmmnnoopp.
  //
  Status = StrHexToBytes (String, 2 * 6, &LocalGuid.Data4[2], 6);
  if (RETURN_ERROR (Status)) {
    return RETURN_UNSUPPORTED;
  }

  CopyGuid (Guid, &LocalGuid);
  return RETURN_SUCCESS;
}

/**
  Compares up to a specified length the contents of two Null-terminated Unicode strings,
  and returns the difference between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. At most, Length Unicode
  characters will be compared. If Length is 0, then 0 is returned. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched Unicode character in SecondString
  subtracted from the first mismatched Unicode character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and FirstString is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If Length > 0 and SecondString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.
  @param  Length        The maximum number of Unicode characters to compare.

  @retval 0      FirstString is identical to SecondString.
  @return others FirstString is not identical to SecondString.

**/
INTN
StrnCmp (
        CONST CHAR16              *FirstString,
        CONST CHAR16              *SecondString,
        UINTN                     Length
  )
{
  if (Length == 0) {
    return 0;
  }

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  while ((*FirstString != L'\0') &&
         (*SecondString != L'\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }

  return *FirstString - *SecondString;
}

VOID *
AllocateCopyPool (
   UINTN       AllocationSize,
   CONST VOID  *Buffer
  )
{
  return InternalAllocateCopyPool (AllocationSize, Buffer);
}

INTN
StrCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }
  return *FirstString - *SecondString;
}

UINT64
SwapBytes64 (
  UINT64                    Value
  )
{
  return InternalMathSwapBytes64 (Value);
}

UINT64
InternalMathSwapBytes64 (
  UINT64                    Operand
  )
{
  UINT64  LowerBytes;
  UINT64  HigherBytes;

  LowerBytes  = (UINT64) SwapBytes32 ((UINT32) Operand);
  HigherBytes = (UINT64) SwapBytes32 ((UINT32) (Operand >> 32));

  return (LowerBytes << 32 | HigherBytes);
}

RETURN_STATUS
StrToIpv4Address (
  CONST CHAR16       *String,
  CHAR16             **EndPointer,
  EFI_IPv4_ADDRESS       *Address,
  UINT8              *PrefixLength
  )
{
  RETURN_STATUS          Status;
  UINTN                  AddressIndex;
  UINT64                 Uint64;
  EFI_IPv4_ADDRESS       LocalAddress;
  UINT8                  LocalPrefixLength;
  CHAR16                 *Pointer;

  LocalPrefixLength = MAX_UINT8;
  LocalAddress.Addr[0] = 0;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. None of String or Guid shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Address != NULL), RETURN_INVALID_PARAMETER);

  for (Pointer = (CHAR16 *) String, AddressIndex = 0; AddressIndex < ARRAY_SIZE (Address->Addr) + 1;) {
    if (!InternalIsDecimalDigitCharacter (*Pointer)) {
      //
      // D or P contains invalid characters.
      //
      break;
    }

    //
    // Get D or P.
    //
    Status = StrDecimalToUint64S ((CONST CHAR16 *) Pointer, &Pointer, &Uint64);
    if (RETURN_ERROR (Status)) {
      return RETURN_UNSUPPORTED;
    }
    if (AddressIndex == ARRAY_SIZE (Address->Addr)) {
      //
      // It's P.
      //
      if (Uint64 > 32) {
        return RETURN_UNSUPPORTED;
      }
      LocalPrefixLength = (UINT8) Uint64;
    } else {
      //
      // It's D.
      //
      if (Uint64 > MAX_UINT8) {
        return RETURN_UNSUPPORTED;
      }
      LocalAddress.Addr[AddressIndex] = (UINT8) Uint64;
      AddressIndex++;
    }

    //
    // Check the '.' or '/', depending on the AddressIndex.
    //
    if (AddressIndex == ARRAY_SIZE (Address->Addr)) {
      if (*Pointer == L'/') {
        //
        // '/P' is in the String.
        // Skip "/" and get P in next loop.
        //
        Pointer++;
      } else {
        //
        // '/P' is not in the String.
        //
        break;
      }
    } else if (AddressIndex < ARRAY_SIZE (Address->Addr)) {
      if (*Pointer == L'.') {
        //
        // D should be followed by '.'
        //
        Pointer++;
      } else {
        return RETURN_UNSUPPORTED;
      }
    }
  }

  if (AddressIndex < ARRAY_SIZE (Address->Addr)) {
    return RETURN_UNSUPPORTED;
  }

  memcpy (Address, &LocalAddress, sizeof (*Address));
  if (PrefixLength != NULL) {
    *PrefixLength = LocalPrefixLength;
  }
  if (EndPointer != NULL) {
    *EndPointer = Pointer;
  }

  return RETURN_SUCCESS;
}

RETURN_STATUS
StrToIpv6Address (
  CONST CHAR16       *String,
  CHAR16             **EndPointer,
  EFI_IPv6_ADDRESS   *Address,
  UINT8              *PrefixLength
  )
{
  RETURN_STATUS          Status;
  UINTN                  AddressIndex;
  UINT64                 Uint64;
  EFI_IPv6_ADDRESS       LocalAddress;
  UINT8                  LocalPrefixLength;
  CONST CHAR16           *Pointer;
  CHAR16                 *End;
  UINTN                  CompressStart;
  BOOLEAN                ExpectPrefix;

  LocalPrefixLength = MAX_UINT8;
  CompressStart     = ARRAY_SIZE (Address->Addr);
  ExpectPrefix      = FALSE;

  ASSERT (((UINTN) String & BIT0) == 0);

  //
  // 1. None of String or Guid shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((String != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Address != NULL), RETURN_INVALID_PARAMETER);

  for (Pointer = String, AddressIndex = 0; AddressIndex < ARRAY_SIZE (Address->Addr) + 1;) {
    if (!InternalIsHexaDecimalDigitCharacter (*Pointer)) {
      if (*Pointer != L':') {
        //
        // ":" or "/" should be followed by digit characters.
        //
        return RETURN_UNSUPPORTED;
      }

      //
      // Meet second ":" after previous ":" or "/"
      // or meet first ":" in the beginning of String.
      //
      if (ExpectPrefix) {
        //
        // ":" shall not be after "/"
        //
        return RETURN_UNSUPPORTED;
      }

      if (CompressStart != ARRAY_SIZE (Address->Addr) || AddressIndex == ARRAY_SIZE (Address->Addr)) {
        //
        // "::" can only appear once.
        // "::" can only appear when address is not full length.
        //
        return RETURN_UNSUPPORTED;
      } else {
        //
        // Remember the start of zero compressing.
        //
        CompressStart = AddressIndex;
        Pointer++;

        if (CompressStart == 0) {
          if (*Pointer != L':') {
            //
            // Single ":" shall not be in the beginning of String.
            //
            return RETURN_UNSUPPORTED;
          }
          Pointer++;
        }
      }
    }

    if (!InternalIsHexaDecimalDigitCharacter (*Pointer)) {
      if (*Pointer == L'/') {
        //
        // Might be optional "/P" after "::".
        //
        if (CompressStart != AddressIndex) {
          return RETURN_UNSUPPORTED;
        }
      } else {
        break;
      }
    } else {
      if (!ExpectPrefix) {
        //
        // Get X.
        //
        Status = StrHexToUint64S (Pointer, &End, &Uint64);
        if (RETURN_ERROR (Status) || End - Pointer > 4) {
          //
          // Number of hexadecimal digit characters is no more than 4.
          //
          return RETURN_UNSUPPORTED;
        }
        Pointer = End;
        //
        // Uint64 won't exceed MAX_UINT16 if number of hexadecimal digit characters is no more than 4.
        //
        ASSERT (AddressIndex + 1 < ARRAY_SIZE (Address->Addr));
        LocalAddress.Addr[AddressIndex] = (UINT8) ((UINT16) Uint64 >> 8);
        LocalAddress.Addr[AddressIndex + 1] = (UINT8) Uint64;
        AddressIndex += 2;
      } else {
        //
        // Get P, then exit the loop.
        //
        Status = StrDecimalToUint64S (Pointer, &End, &Uint64);
        if (RETURN_ERROR (Status) || End == Pointer || Uint64 > 128) {
          //
          // Prefix length should not exceed 128.
          //
          return RETURN_UNSUPPORTED;
        }
        LocalPrefixLength = (UINT8) Uint64;
        Pointer = End;
        break;
      }
    }

    //
    // Skip ':' or "/"
    //
    if (*Pointer == L'/') {
      ExpectPrefix = TRUE;
    } else if (*Pointer == L':') {
      if (AddressIndex == ARRAY_SIZE (Address->Addr)) {
        //
        // Meet additional ":" after all 8 16-bit address
        //
        break;
      }
    } else {
      //
      // Meet other character that is not "/" or ":" after all 8 16-bit address
      //
      break;
    }
    Pointer++;
  }

  if ((AddressIndex == ARRAY_SIZE (Address->Addr) && CompressStart != ARRAY_SIZE (Address->Addr)) ||
    (AddressIndex != ARRAY_SIZE (Address->Addr) && CompressStart == ARRAY_SIZE (Address->Addr))
      ) {
    //
    // Full length of address shall not have compressing zeros.
    // Non-full length of address shall have compressing zeros.
    //
    return RETURN_UNSUPPORTED;
  }
  memcpy (&Address->Addr[0], &LocalAddress.Addr[0], CompressStart);
  if (AddressIndex > CompressStart) {
    memset (&Address->Addr[CompressStart], 0,  ARRAY_SIZE (Address->Addr) - AddressIndex);
    memcpy (
      &Address->Addr[CompressStart + ARRAY_SIZE (Address->Addr) - AddressIndex],
      &LocalAddress.Addr[CompressStart],
      AddressIndex - CompressStart
      );
  }

  if (PrefixLength != NULL) {
    *PrefixLength = LocalPrefixLength;
  }
  if (EndPointer != NULL) {
    *EndPointer = (CHAR16 *) Pointer;
  }

  return RETURN_SUCCESS;
}


RETURN_STATUS
UnicodeStrToAsciiStrS (
  CONST CHAR16              *Source,
  CHAR8                     *Destination,
  UINTN                     DestMax
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than ASCII_RSIZE_MAX or RSIZE_MAX.
  //
  if (ASCII_RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= ASCII_RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS (Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (!InternalSafeStringIsOverlap (Destination, DestMax, (VOID *)Source, (SourceLen + 1) * sizeof(CHAR16)), RETURN_ACCESS_DENIED);

  //
  // convert string
  //
  while (*Source != '\0') {
    //
    // If any Unicode characters in Source contain
    // non-zero value in the upper 8 bits, then ASSERT().
    //
    ASSERT (*Source < 0x100);
    *(Destination++) = (CHAR8) *(Source++);
  }
  *Destination = '\0';

  return RETURN_SUCCESS;
}

RETURN_STATUS
StrCpyS (
  CHAR16       *Destination,
  UINTN        DestMax,
  CONST CHAR16 *Source
  )
{
  UINTN            SourceLen;

  ASSERT (((UINTN) Destination & BIT0) == 0);
  ASSERT (((UINTN) Source & BIT0) == 0);

  //
  // 1. Neither Destination nor Source shall be a null pointer.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((Destination != NULL), RETURN_INVALID_PARAMETER);
  SAFE_STRING_CONSTRAINT_CHECK ((Source != NULL), RETURN_INVALID_PARAMETER);

  //
  // 2. DestMax shall not be greater than RSIZE_MAX.
  //
  if (RSIZE_MAX != 0) {
    SAFE_STRING_CONSTRAINT_CHECK ((DestMax <= RSIZE_MAX), RETURN_INVALID_PARAMETER);
  }

  //
  // 3. DestMax shall not equal zero.
  //
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax != 0), RETURN_INVALID_PARAMETER);

  //
  // 4. DestMax shall be greater than StrnLenS(Source, DestMax).
  //
  SourceLen = StrnLenS (Source, DestMax);
  SAFE_STRING_CONSTRAINT_CHECK ((DestMax > SourceLen), RETURN_BUFFER_TOO_SMALL);

  //
  // 5. Copying shall not take place between objects that overlap.
  //
  SAFE_STRING_CONSTRAINT_CHECK (InternalSafeStringNoStrOverlap (Destination, DestMax, (CHAR16 *)Source, SourceLen + 1), RETURN_ACCESS_DENIED);

  //
  // The StrCpyS function copies the string pointed to by Source (including the terminating
  // null character) into the array pointed to by Destination.
  //
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;

  return RETURN_SUCCESS;
}

VOID *
AllocateZeroPool (
  UINTN  AllocationSize
  )
{
  VOID * Memory;
  Memory = malloc(AllocationSize);
  ASSERT (Memory != NULL);
  if (Memory == NULL) {
    fprintf(stderr, "Not memory for malloc\n");
  }
  memset(Memory, 0, AllocationSize);
  return Memory;
}

VOID *
AllocatePool (
  UINTN  AllocationSize
  )
{
  return InternalAllocatePool (AllocationSize);
}

UINT16
WriteUnaligned16 (
  UINT16                    *Buffer,
  UINT16                    Value
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer = Value;
}

UINT16
ReadUnaligned16 (
  CONST UINT16              *Buffer
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer;
}
/**
  Return whether the integer string is a hex string.

  @param Str             The integer string

  @retval TRUE   Hex string
  @retval FALSE  Decimal string

**/
BOOLEAN
IsHexStr (
   CHAR16   *Str
  )
{
  //
  // skip preceding white space
  //
  while ((*Str != 0) && *Str == L' ') {
    Str ++;
  }
  //
  // skip preceding zeros
  //
  while ((*Str != 0) && *Str == L'0') {
    Str ++;
  }

  return (BOOLEAN) (*Str == L'x' || *Str == L'X');
}

/**

  Convert integer string to uint.

  @param Str             The integer string. If leading with "0x" or "0X", it's hexadecimal.

  @return A UINTN value represented by Str

**/
UINTN
Strtoi (
   CHAR16  *Str
  )
{
  if (IsHexStr (Str)) {
    return (UINTN)StrHexToUint64 (Str);
  } else {
    return (UINTN)StrDecimalToUint64 (Str);
  }
}

/**

  Convert integer string to 64 bit data.

  @param Str             The integer string. If leading with "0x" or "0X", it's hexadecimal.
  @param Data            A pointer to the UINT64 value represented by Str

**/
VOID
Strtoi64 (
    CHAR16  *Str,
   UINT64  *Data
  )
{
  if (IsHexStr (Str)) {
    *Data = StrHexToUint64 (Str);
  } else {
    *Data = StrDecimalToUint64 (Str);
  }
}

/**
  Converts a Unicode string to ASCII string.

  @param Str             The equivalent Unicode string
  @param AsciiStr        On input, it points to destination ASCII string buffer; on output, it points
                         to the next ASCII string next to it

**/
VOID
StrToAscii (
       CHAR16 *Str,
    CHAR8  **AsciiStr
  )
{
  CHAR8 *Dest;

  Dest = *AsciiStr;
  while (!IS_NULL (*Str)) {
    *(Dest++) = (CHAR8) *(Str++);
  }
  *Dest = 0;

  //
  // Return the string next to it
  //
  *AsciiStr = Dest + 1;
}

/**
  Gets current sub-string from a string list, before return
  the list header is moved to next sub-string. The sub-string is separated
  by the specified character. For example, the separator is ',', the string
  list is "2,0,3", it returns "2", the remain list move to "0,3"

  @param  List        A string list separated by the specified separator
  @param  Separator   The separator character

  @return A pointer to the current sub-string

**/
CHAR16 *
SplitStr (
    CHAR16 **List,
       CHAR16 Separator
  )
{
  CHAR16  *Str;
  CHAR16  *ReturnStr;

  Str = *List;
  ReturnStr = Str;

  if (IS_NULL (*Str)) {
    return ReturnStr;
  }

  //
  // Find first occurrence of the separator
  //
  while (!IS_NULL (*Str)) {
    if (*Str == Separator) {
      break;
    }
    Str++;
  }

  if (*Str == Separator) {
    //
    // Find a sub-string, terminate it
    //
    *Str = L'\0';
    Str++;
  }

  //
  // Move to next sub-string
  //
  *List = Str;
  return ReturnStr;
}

