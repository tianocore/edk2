/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CommonLib.c

Abstract:

  Common Library Functions
 
--*/

#include "TianoCommon.h"
#include "PeiHob.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "CommonLib.h"

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
  InputFile = fopen (InputFileName, "rb");
  if (InputFile == NULL) {
    printf ("ERROR: Could not open input file \"%s\".\n", InputFileName);
    return EFI_ABORTED;
  }
  //
  // Go to the end so that we can determine the file size
  //
  if (fseek (InputFile, 0, SEEK_END)) {
    printf ("ERROR: System error reading input file \"%s\".\n", InputFileName);
    fclose (InputFile);
    return EFI_ABORTED;
  }
  //
  // Get the file size
  //
  FileSize = ftell (InputFile);
  if (FileSize == -1) {
    printf ("ERROR: System error parsing input file \"%s\".\n", InputFileName);
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
    printf ("ERROR: System error reading input file \"%s\".\n", InputFileName);
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
    printf ("ERROR: Reading file \"%s\"%i.\n", InputFileName);
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
  IN UINT32 Size
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
    printf ("ERROR: PrintGuid called with a NULL value.\n");
    return EFI_INVALID_PARAMETER;
  }

  printf (
    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
    Guid->Data1,
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
    printf ("ERROR: PrintGuidToBuffer() called with a NULL value\n");
    return EFI_INVALID_PARAMETER;
  }

  if (BufferLen < PRINTED_GUID_BUFFER_SIZE) {
    printf ("ERORR: PrintGuidToBuffer() called with invalid buffer size\n");
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Uppercase) {
    sprintf (
      Buffer,
      "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
      Guid->Data1,
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
      Buffer,
      "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      Guid->Data1,
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
