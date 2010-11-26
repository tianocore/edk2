/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ParseInf.c

Abstract:

  This contains some useful functions for parsing INF files.

--*/

#include "ParseInf.h"
#include <assert.h>
#include <string.h>
#include <ctype.h>

CHAR8 *
ReadLine (
  IN MEMORY_FILE    *InputFile,
  IN OUT CHAR8      *InputBuffer,
  IN UINTN          MaxLength
  )
/*++

Routine Description:

  This function reads a line, stripping any comments.
  The function reads a string from the input stream argument and stores it in 
  the input string. ReadLine reads characters from the current file position 
  to and including the first newline character, to the end of the stream, or 
  until the number of characters read is equal to MaxLength - 1, whichever 
  comes first.  The newline character, if read, is replaced with a \0. 

Arguments:

  InputFile     Memory file image.
  InputBuffer   Buffer to read into, must be _MAX_PATH size.
  MaxLength     The maximum size of the input buffer.

Returns:

  NULL if error or EOF
  InputBuffer otherwise

--*/
{
  CHAR8 *CharPtr;
  CHAR8 *EndOfLine;
  UINTN CharsToCopy;

  //
  // Verify input parameters are not null
  //
  assert (InputBuffer);
  assert (InputFile->FileImage);
  assert (InputFile->Eof);
  assert (InputFile->CurrentFilePointer);

  //
  // Check for end of file condition
  //
  if (InputFile->CurrentFilePointer >= InputFile->Eof) {
    return NULL;
  }
  //
  // Find the next newline char
  //
  EndOfLine = strchr (InputFile->CurrentFilePointer, '\n');

  //
  // Determine the number of characters to copy.
  //
  if (EndOfLine == 0) {
    //
    // If no newline found, copy to the end of the file.
    //
    CharsToCopy = InputFile->Eof - InputFile->CurrentFilePointer;
  } else if (EndOfLine >= InputFile->Eof) {
    //
    // If the newline found was beyond the end of file, copy to the eof.
    //
    CharsToCopy = InputFile->Eof - InputFile->CurrentFilePointer;
  } else {
    //
    // Newline found in the file.
    //
    CharsToCopy = EndOfLine - InputFile->CurrentFilePointer;
  }
  //
  // If the end of line is too big for the current buffer, set it to the max
  // size of the buffer (leaving room for the \0.
  //
  if (CharsToCopy > MaxLength - 1) {
    CharsToCopy = MaxLength - 1;
  }
  //
  // Copy the line.
  //
  memcpy (InputBuffer, InputFile->CurrentFilePointer, CharsToCopy);

  //
  // Add the null termination over the 0x0D
  //
  InputBuffer[CharsToCopy - 1] = '\0';

  //
  // Increment the current file pointer (include the 0x0A)
  //
  InputFile->CurrentFilePointer += CharsToCopy + 1;

  //
  // Strip any comments
  //
  CharPtr = strstr (InputBuffer, "//");
  if (CharPtr != 0) {
    CharPtr[0] = 0;
  }
  //
  // Return the string
  //
  return InputBuffer;
}

BOOLEAN
FindSection (
  IN MEMORY_FILE    *InputFile,
  IN CHAR8          *Section
  )
/*++

Routine Description:

  This function parses a file from the beginning to find a section.
  The section string may be anywhere within a line.

Arguments:

  InputFile     Memory file image.
  Section       Section to search for

Returns:

  FALSE if error or EOF
  TRUE if section found

--*/
{
  CHAR8 InputBuffer[_MAX_PATH];
  CHAR8 *CurrentToken;

  //
  // Verify input is not NULL
  //
  assert (InputFile->FileImage);
  assert (InputFile->Eof);
  assert (InputFile->CurrentFilePointer);
  assert (Section);

  //
  // Rewind to beginning of file
  //
  InputFile->CurrentFilePointer = InputFile->FileImage;

  //
  // Read lines until the section is found
  //
  while (InputFile->CurrentFilePointer < InputFile->Eof) {
    //
    // Read a line
    //
    ReadLine (InputFile, InputBuffer, _MAX_PATH);

    //
    // Check if the section is found
    //
    CurrentToken = strstr (InputBuffer, Section);
    if (CurrentToken != NULL) {
      return TRUE;
    }
  }

  return FALSE;
}

EFI_STATUS
FindToken (
  IN MEMORY_FILE    *InputFile,
  IN CHAR8          *Section,
  IN CHAR8          *Token,
  IN UINTN          Instance,
  OUT CHAR8         *Value
  )
/*++

Routine Description:

  Finds a token value given the section and token to search for.

Arguments:

  InputFile Memory file image.
  Section   The section to search for, a string within [].
  Token     The token to search for, e.g. EFI_PEIM_RECOVERY, followed by an = in the INF file.
  Instance  The instance of the token to search for.  Zero is the first instance.
  Value     The string that holds the value following the =.  Must be _MAX_PATH in size.

Returns:

  EFI_SUCCESS             Value found.
  EFI_ABORTED             Format error detected in INF file.
  EFI_INVALID_PARAMETER   Input argument was null.
  EFI_LOAD_ERROR          Error reading from the file.
  EFI_NOT_FOUND           Section/Token/Value not found.

--*/
{
  CHAR8   InputBuffer[_MAX_PATH];
  CHAR8   *CurrentToken;
  BOOLEAN ParseError;
  BOOLEAN ReadError;
  UINTN   Occurrance;

  //
  // Check input parameters
  //
  if (InputFile->FileImage == NULL ||
      InputFile->Eof == NULL ||
      InputFile->CurrentFilePointer == NULL ||
      Section == NULL ||
      strlen (Section) == 0 ||
      Token == NULL ||
      strlen (Token) == 0 ||
      Value == NULL
      ) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize error codes
  //
  ParseError  = FALSE;
  ReadError   = FALSE;

  //
  // Initialize our instance counter for the search token
  //
  Occurrance = 0;

  if (FindSection (InputFile, Section)) {
    //
    // Found the desired section, find and read the desired token
    //
    do {
      //
      // Read a line from the file
      //
      if (ReadLine (InputFile, InputBuffer, _MAX_PATH) == NULL) {
        //
        // Error reading from input file
        //
        ReadError = TRUE;
        break;
      }
      //
      // Get the first non-whitespace string
      //
      CurrentToken = strtok (InputBuffer, " \t\n");
      if (CurrentToken == NULL) {
        //
        // Whitespace line found (or comment) so continue
        //
        CurrentToken = InputBuffer;
        continue;
      }
      //
      // Make sure we have not reached the end of the current section
      //
      if (CurrentToken[0] == '[') {
        break;
      }
      //
      // Compare the current token with the desired token
      //
      if (strcmp (CurrentToken, Token) == 0) {
        //
        // Found it
        //
        //
        // Check if it is the correct instance
        //
        if (Instance == Occurrance) {
          //
          // Copy the contents following the =
          //
          CurrentToken = strtok (NULL, "= \t\n");
          if (CurrentToken == NULL) {
            //
            // Nothing found, parsing error
            //
            ParseError = TRUE;
          } else {
            //
            // Copy the current token to the output value
            //
            strcpy (Value, CurrentToken);
            return EFI_SUCCESS;
          }
        } else {
          //
          // Increment the occurrance found
          //
          Occurrance++;
        }
      }
    } while (
      !ParseError &&
      !ReadError &&
      InputFile->CurrentFilePointer < InputFile->Eof &&
      CurrentToken[0] != '[' &&
      Occurrance <= Instance
    );
  }
  //
  // Distinguish between read errors and INF file format errors.
  //
  if (ReadError) {
    return EFI_LOAD_ERROR;
  }

  if (ParseError) {
    return EFI_ABORTED;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
FindTokenInstanceInSection (
  IN MEMORY_FILE    *InputFile,
  IN CHAR8          *Section,
  IN UINTN          Instance,
  OUT CHAR8         *Token,
  OUT CHAR8         *Value
  )
/*++

Routine Description:

  Finds the Instance-th token in a section.

Arguments:

  InputFile Memory file image.
  Section   The section to search for, a string within [].
  Instance  Specify the Instance-th token to search for, starting from zero
  Token     The token name to return. Caller should allocate the buffer.
            Must be _MAX_PATH in size.
  Value     The token value to return. Caller should allocate the buffer.
            Must be _MAX_PATH in size.

Returns:

  EFI_SUCCESS             Token and Value found.
  EFI_ABORTED             Format error detected in INF file.
  EFI_INVALID_PARAMETER   Input argument was null.
  EFI_LOAD_ERROR          Error reading from the file.
  EFI_NOT_FOUND           Section/Token/Value not found.

--*/
{
  CHAR8   InputBuffer[_MAX_PATH];
  CHAR8   *CurrentToken;
  CHAR8   *CurrentValue;
  BOOLEAN ParseError;
  BOOLEAN ReadError;
  UINTN   InstanceIndex;

  //
  // Check input parameters
  //
  if (InputFile->FileImage == NULL ||
      InputFile->Eof == NULL ||
      InputFile->CurrentFilePointer == NULL ||
      Section == NULL ||
      strlen (Section) == 0 ||
      Value == NULL
      ) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize error codes
  //
  ParseError  = FALSE;
  ReadError   = FALSE;

  //
  // Initialize our instance counter for the search token
  //
  InstanceIndex = 0;

  if (FindSection (InputFile, Section)) {
    //
    // Found the desired section, find and read the desired token
    //
    do {
      //
      // Read a line from the file
      //
      if (ReadLine (InputFile, InputBuffer, _MAX_PATH) == NULL) {
        //
        // Error reading from input file
        //
        ReadError = TRUE;
        break;
      }
      //
      // Get the first non-whitespace string
      //
      CurrentToken = strtok (InputBuffer, " \t\n");
      if (CurrentToken == NULL) {
        //
        // Whitespace line found (or comment) so continue
        //
        CurrentToken = InputBuffer;
        continue;
      }
      //
      // Make sure we have not reached the end of the current section
      //
      if (CurrentToken[0] == '[') {
        break;
      }
      //
      // Check if it is the correct instance
      //
      if (Instance == InstanceIndex) {
        //
        // Copy the contents following the =
        //
        CurrentValue = strtok (NULL, "= \t\n");
        if (CurrentValue == NULL) {
          //
          // Nothing found, parsing error
          //
          ParseError = TRUE;
        } else {
          //
          // Copy the current token to the output value
          //
          strcpy (Token, CurrentToken);
          strcpy (Value, CurrentValue);
          return EFI_SUCCESS;
        }
      } else {
        //
        // Increment the occurrance found
        //
        InstanceIndex++;
      }
    } while (
      !ParseError &&
      !ReadError &&
      InputFile->CurrentFilePointer < InputFile->Eof &&
      CurrentToken[0] != '[' &&
      InstanceIndex <= Instance
    );
  }
  //
  // Distinguish between read errors and INF file format errors.
  //
  if (ReadError) {
    return EFI_LOAD_ERROR;
  }

  if (ParseError) {
    return EFI_ABORTED;
  }

  return EFI_NOT_FOUND;
}


EFI_STATUS
StringToGuid (
  IN CHAR8      *AsciiGuidBuffer,
  OUT EFI_GUID  *GuidBuffer
  )
/*++

Routine Description: 

  Converts a string to an EFI_GUID.  The string must be in the 
  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format.

Arguments:  

  AsciiGuidBuffer - pointer to ascii string
  GuidBuffer      - pointer to destination Guid

Returns:  

  EFI_ABORTED             Could not convert the string
  EFI_SUCCESS             The string was successfully converted
  EFI_INVALID_PARAMETER   Input parameter is invalid.

--*/
{
  INT32 Index;
  UINTN Data1;
  UINTN Data2;
  UINTN Data3;
  UINTN Data4[8];

  if (AsciiGuidBuffer == NULL || GuidBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Scan the guid string into the buffer
  //
  Index = sscanf (
            AsciiGuidBuffer,
            "%08x-%04x-%04x-%02x%02x-%02hx%02hx%02hx%02hx%02hx%02hx",
            &Data1,
            &Data2,
            &Data3,
            &Data4[0],
            &Data4[1],
            &Data4[2],
            &Data4[3],
            &Data4[4],
            &Data4[5],
            &Data4[6],
            &Data4[7]
            );

  //
  // Verify the correct number of items were scanned.
  //
  if (Index != 11) {
    printf ("ERROR: Malformed GUID \"%s\".\n\n", AsciiGuidBuffer);
    return EFI_ABORTED;
  }
  //
  // Copy the data into our GUID.
  //
  GuidBuffer->Data1     = (UINT32) Data1;
  GuidBuffer->Data2     = (UINT16) Data2;
  GuidBuffer->Data3     = (UINT16) Data3;
  GuidBuffer->Data4[0]  = (UINT8) Data4[0];
  GuidBuffer->Data4[1]  = (UINT8) Data4[1];
  GuidBuffer->Data4[2]  = (UINT8) Data4[2];
  GuidBuffer->Data4[3]  = (UINT8) Data4[3];
  GuidBuffer->Data4[4]  = (UINT8) Data4[4];
  GuidBuffer->Data4[5]  = (UINT8) Data4[5];
  GuidBuffer->Data4[6]  = (UINT8) Data4[6];
  GuidBuffer->Data4[7]  = (UINT8) Data4[7];

  return EFI_SUCCESS;
}

EFI_STATUS
AsciiStringToUint64 (
  IN CONST CHAR8  *AsciiString,
  IN BOOLEAN      IsHex,
  OUT UINT64      *ReturnValue
  )
/*++

Routine Description:

  Converts a null terminated ascii string that represents a number into a 
  UINT64 value.  A hex number may be preceeded by a 0x, but may not be 
  succeeded by an h.  A number without 0x or 0X is considered to be base 10 
  unless the IsHex input is true.

Arguments:

  AsciiString   The string to convert.
  IsHex         Force the string to be treated as a hex number.
  ReturnValue   The return value.

Returns:

  EFI_SUCCESS   Number successfully converted.
  EFI_ABORTED   Invalid character encountered.

--*/
{
  UINT8   Index;
  UINT64  HexNumber;
  CHAR8   CurrentChar;

  //
  // Initialize the result
  //
  HexNumber = 0;

  //
  // Add each character to the result
  //
  if (IsHex || (AsciiString[0] == '0' && (AsciiString[1] == 'x' || AsciiString[1] == 'X'))) {
    //
    // Verify string is a hex number
    //
    for (Index = 2; Index < strlen (AsciiString); Index++) {
      if (isxdigit (AsciiString[Index]) == 0) {
        return EFI_ABORTED;
      }
    }
    //
    // Convert the hex string.
    //
    for (Index = 2; AsciiString[Index] != '\0'; Index++) {
      CurrentChar = AsciiString[Index];
      HexNumber *= 16;
      if (CurrentChar >= '0' && CurrentChar <= '9') {
        HexNumber += CurrentChar - '0';
      } else if (CurrentChar >= 'a' && CurrentChar <= 'f') {
        HexNumber += CurrentChar - 'a' + 10;
      } else if (CurrentChar >= 'A' && CurrentChar <= 'F') {
        HexNumber += CurrentChar - 'A' + 10;
      } else {
        //
        // Unrecognized character
        //
        return EFI_ABORTED;
      }
    }

    *ReturnValue = HexNumber;
  } else {
    //
    // Verify string is a number
    //
    for (Index = 0; Index < strlen (AsciiString); Index++) {
      if (isdigit (AsciiString[Index]) == 0) {
        return EFI_ABORTED;
      }
    }

    *ReturnValue = atol (AsciiString);
  }

  return EFI_SUCCESS;
};

CHAR8 *
ReadLineInStream (
  IN FILE       *InputFile,
  IN OUT CHAR8  *InputBuffer
  )
/*++

Routine Description:

  This function reads a line, stripping any comments.
  // BUGBUG:  This is obsolete once genmake goes away...

Arguments:

  InputFile     Stream pointer.
  InputBuffer   Buffer to read into, must be _MAX_PATH size.

Returns:

  NULL if error or EOF
  InputBuffer otherwise

--*/
{
  CHAR8 *CharPtr;

  //
  // Verify input parameters are not null
  //
  assert (InputFile);
  assert (InputBuffer);

  //
  // Read a line
  //
  if (fgets (InputBuffer, _MAX_PATH, InputFile) == NULL) {
    return NULL;
  }
  //
  // Strip any comments
  //
  CharPtr = strstr (InputBuffer, "//");
  if (CharPtr != 0) {
    CharPtr[0] = 0;
  }

  CharPtr = strstr (InputBuffer, "#");
  if (CharPtr != 0) {
    CharPtr[0] = 0;
  }
  //
  // Return the string
  //
  return InputBuffer;
}

BOOLEAN
FindSectionInStream (
  IN FILE       *InputFile,
  IN CHAR8      *Section
  )
/*++

Routine Description:

  This function parses a stream file from the beginning to find a section.
  The section string may be anywhere within a line.
  // BUGBUG:  This is obsolete once genmake goes away...

Arguments:

  InputFile     Stream pointer.
  Section       Section to search for

Returns:

  FALSE if error or EOF
  TRUE if section found

--*/
{
  CHAR8 InputBuffer[_MAX_PATH];
  CHAR8 *CurrentToken;

  //
  // Verify input is not NULL
  //
  assert (InputFile);
  assert (Section);

  //
  // Rewind to beginning of file
  //
  if (fseek (InputFile, 0, SEEK_SET) != 0) {
    return FALSE;
  }
  //
  // Read lines until the section is found
  //
  while (feof (InputFile) == 0) {
    //
    // Read a line
    //
    ReadLineInStream (InputFile, InputBuffer);

    //
    // Check if the section is found
    //
    CurrentToken = strstr (InputBuffer, Section);
    if (CurrentToken != NULL) {
      return TRUE;
    }
  }

  return FALSE;
}
