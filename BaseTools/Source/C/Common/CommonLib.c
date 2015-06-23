/** @file
Common basic Library Functions

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
        strcat (mCommonLibFullPath, RootPath);
        if (FileName[0] != '\\' && FileName[0] != '/') {
          //
          // Attach directory separator
          //
          strcat (mCommonLibFullPath, "\\");
        }
        free (RootPath);
      }
    }

    //
    // Construct the full file path
    //
    strcat (mCommonLibFullPath, FileName);
    
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
      strcat (mCommonLibFullPath, PathPointer + 3);
    }
    
    //
    // Convert ".\\" to "", because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    while ((PathPointer = strstr (mCommonLibFullPath, ".\\")) != NULL) {
      *PathPointer = '\0';
      strcat (mCommonLibFullPath, PathPointer + 2);
    }
        
    //
    // Convert "\\.\\" to "\\", because it doesn't work with WINDOWS_EXTENSION_PATH.
    //
    while ((PathPointer = strstr (mCommonLibFullPath, "\\.\\")) != NULL) {
      *PathPointer = '\0';
      strcat (mCommonLibFullPath, PathPointer + 2);
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
        strcat (mCommonLibFullPath, NextPointer);
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
