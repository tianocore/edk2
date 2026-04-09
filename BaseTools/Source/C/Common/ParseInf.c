/** @file
This contains some useful functions for parsing INF files.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include "CommonLib.h"

/**
  This function reads a line, stripping any comments.
  The function reads a string from the input stream argument and stores it in
  the input string. ReadLine reads characters from the current file position
  to and including the first newline character, to the end of the stream, or
  until the number of characters read is equal to MaxLength - 1, whichever
  comes first.  The newline character, if read, is replaced with a \0.

  @param InputFile     Memory file image.
  @param InputBuffer   Buffer to read into, must be MaxLength size.
  @param MaxLength     The maximum size of the input buffer.

  @retval NULL if error or EOF
  @retval InputBuffer otherwise
**/
CHAR8 *
ReadLine (
  IN MEMORY_FILE    *InputFile,
  IN OUT CHAR8      *InputBuffer,
  IN UINTN          MaxLength
  )

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
  if (InputBuffer[CharsToCopy - 1] == '\r') {

    InputBuffer[CharsToCopy - 1] = '\0';

  } else {

    InputBuffer[CharsToCopy] = '\0';

  }

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

/**
  This function parses a file from the beginning to find a section.
  The section string may be anywhere within a line.

  @param InputFile     Memory file image.
  @param Section       Section to search for

  @retval FALSE if error or EOF
  @retval TRUE if section found
**/
BOOLEAN
FindSection (
  IN MEMORY_FILE    *InputFile,
  IN CHAR8          *Section
  )
{
  CHAR8 InputBuffer[MAX_LONG_FILE_PATH];
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
    ReadLine (InputFile, InputBuffer, MAX_LONG_FILE_PATH);

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

/**
  Finds a token value given the section and token to search for.

  @param InputFile Memory file image.
  @param Section   The section to search for, a string within [].
  @param Token     The token to search for, e.g. EFI_PEIM_RECOVERY, followed by an = in the INF file.
  @param Instance  The instance of the token to search for.  Zero is the first instance.
  @param Value     The string that holds the value following the =.  Must be MAX_LONG_FILE_PATH in size.

  @retval EFI_SUCCESS             Value found.
  @retval EFI_ABORTED             Format error detected in INF file.
  @retval EFI_INVALID_PARAMETER   Input argument was null.
  @retval EFI_LOAD_ERROR          Error reading from the file.
  @retval EFI_NOT_FOUND           Section/Token/Value not found.
**/
EFI_STATUS
FindToken (
  IN MEMORY_FILE    *InputFile,
  IN CHAR8          *Section,
  IN CHAR8          *Token,
  IN UINTN          Instance,
  OUT CHAR8         *Value
  )
{
  CHAR8   InputBuffer[MAX_LONG_FILE_PATH];
  CHAR8   *CurrentToken;
  CHAR8   *Delimiter;
  BOOLEAN ParseError;
  BOOLEAN ReadError;
  UINTN   Occurrence;

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
  Occurrence = 0;

  if (FindSection (InputFile, Section)) {
    //
    // Found the desired section, find and read the desired token
    //
    do {
      //
      // Read a line from the file
      //
      if (ReadLine (InputFile, InputBuffer, MAX_LONG_FILE_PATH) == NULL) {
        //
        // Error reading from input file
        //
        ReadError = TRUE;
        break;
      }
      //
      // Get the first non-whitespace string
      //
      Delimiter = strchr (InputBuffer, '=');
      if (Delimiter != NULL) {
        *Delimiter = 0;
      }

      CurrentToken = strtok (InputBuffer, " \t\n");
      if (CurrentToken == NULL || Delimiter == NULL) {
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
        if (Instance == Occurrence) {
          //
          // Copy the contents following the =
          //
          CurrentToken = Delimiter + 1;
          if (*CurrentToken == 0) {
            //
            // Nothing found, parsing error
            //
            ParseError = TRUE;
          } else {
            //
            // Strip leading white space
            //
            while (*CurrentToken == ' ' || *CurrentToken == '\t') {
              CurrentToken++;
            }
            //
            // Copy the current token to the output value
            //
            strcpy (Value, CurrentToken);
            //
            // Strip trailing white space
            //
            while (strlen(Value) > 0 && (*(Value + strlen(Value) - 1) == ' ' || *(Value + strlen(Value) - 1) == '\t')) {
              *(Value + strlen(Value) - 1) = 0;
            }
            return EFI_SUCCESS;
          }
        } else {
          //
          // Increment the occurrence found
          //
          Occurrence++;
        }
      }
    } while (
      !ParseError &&
      !ReadError &&
      InputFile->CurrentFilePointer < InputFile->Eof &&
      CurrentToken[0] != '[' &&
      Occurrence <= Instance
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

/**
  Converts a string to an EFI_GUID.  The string must be in the
  xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx format.

  @param AsciiGuidBuffer pointer to ascii string
  @param GuidBuffer      pointer to destination Guid

  @retval EFI_ABORTED             Could not convert the string
  @retval EFI_SUCCESS             The string was successfully converted
  @retval EFI_INVALID_PARAMETER   Input parameter is invalid.
**/
EFI_STATUS
StringToGuid (
  IN CHAR8      *AsciiGuidBuffer,
  OUT EFI_GUID  *GuidBuffer
  )
{
  INT32 Index;
  int   Data1;
  int   Data2;
  int   Data3;
  int   Data4[8];

  if (AsciiGuidBuffer == NULL || GuidBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check Guid Format strictly xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
  //
  for (Index = 0; AsciiGuidBuffer[Index] != '\0' && Index < 37; Index ++) {
    if (Index == 8 || Index == 13 || Index == 18 || Index == 23) {
      if (AsciiGuidBuffer[Index] != '-') {
        break;
      }
    } else {
      if (((AsciiGuidBuffer[Index] >= '0') && (AsciiGuidBuffer[Index] <= '9')) ||
         ((AsciiGuidBuffer[Index] >= 'a') && (AsciiGuidBuffer[Index] <= 'f')) ||
         ((AsciiGuidBuffer[Index] >= 'A') && (AsciiGuidBuffer[Index] <= 'F'))) {
        continue;
      } else {
        break;
      }
    }
  }

  if (Index < 36 || AsciiGuidBuffer[36] != '\0') {
    Error (NULL, 0, 1003, "Invalid option value", "Incorrect GUID \"%s\"\n  Correct Format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\"", AsciiGuidBuffer);
    return EFI_ABORTED;
  }

  //
  // Scan the guid string into the buffer
  //
  Index = sscanf (
            AsciiGuidBuffer,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
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
    Error (NULL, 0, 1003, "Invalid option value", "Incorrect GUID \"%s\"\n  Correct Format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\"", AsciiGuidBuffer);
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

/**
  Converts a null terminated ascii string that represents a number into a
  UINT64 value.  A hex number may be preceded by a 0x, but may not be
  succeeded by an h.  A number without 0x or 0X is considered to be base 10
  unless the IsHex input is true.

  @param AsciiString   The string to convert.
  @param IsHex         Force the string to be treated as a hex number.
  @param ReturnValue   The return value.

  @retval EFI_SUCCESS   Number successfully converted.
  @retval EFI_ABORTED   Invalid character encountered.
**/
EFI_STATUS
AsciiStringToUint64 (
  IN CONST CHAR8  *AsciiString,
  IN BOOLEAN      IsHex,
  OUT UINT64      *ReturnValue
  )
{
  UINT8   Index;
  UINT64  Value;
  CHAR8   CurrentChar;

  //
  // Initialize the result
  //
  Value = 0;
  Index = 0;

  //
  // Check input parameter
  //
  if (AsciiString == NULL || ReturnValue == NULL || strlen(AsciiString) > 0xFF) {
    return EFI_INVALID_PARAMETER;
  }
  while (AsciiString[Index] == ' ') {
    Index ++;
  }

  //
  // Add each character to the result
  //

  //
  // Skip first two chars only if the string starts with '0x' or '0X'
  //
  if (AsciiString[Index] == '0' && (AsciiString[Index + 1] == 'x' || AsciiString[Index + 1] == 'X')) {
    IsHex = TRUE;
    Index += 2;
  }
  if (IsHex) {
    //
    // Convert the hex string.
    //
    for (; AsciiString[Index] != '\0'; Index++) {
      CurrentChar = AsciiString[Index];
      if (CurrentChar == ' ') {
        break;
      }
      //
      // Verify Hex string
      //
      if (isxdigit ((int)CurrentChar) == 0) {
        return EFI_ABORTED;
      }
      //
      // Add hex value
      //
      Value *= 16;
      if (CurrentChar >= '0' && CurrentChar <= '9') {
        Value += CurrentChar - '0';
      } else if (CurrentChar >= 'a' && CurrentChar <= 'f') {
        Value += CurrentChar - 'a' + 10;
      } else if (CurrentChar >= 'A' && CurrentChar <= 'F') {
        Value += CurrentChar - 'A' + 10;
      }
    }

    *ReturnValue = Value;
  } else {
    //
    // Convert dec string is a number
    //
    for (; Index < strlen (AsciiString); Index++) {
      CurrentChar = AsciiString[Index];
      if (CurrentChar == ' ') {
        break;
      }
      //
      // Verify Dec string
      //
      if (isdigit ((int)CurrentChar) == 0) {
        return EFI_ABORTED;
      }
      //
      // Add dec value
      //
      Value = Value * 10;
      Value += CurrentChar - '0';
    }

    *ReturnValue = Value;
  }

  return EFI_SUCCESS;
}

/**
  This function reads a line, stripping any comments.
  // BUGBUG:  This is obsolete once genmake goes away...

  @param InputFile     Stream pointer.
  @param InputBuffer   Buffer to read into, must be MAX_LONG_FILE_PATH size.

  @retval NULL if error or EOF
  @retval InputBuffer otherwise
**/
CHAR8 *
ReadLineInStream (
  IN FILE       *InputFile,
  IN OUT CHAR8  *InputBuffer
  )
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
  if (fgets (InputBuffer, MAX_LONG_FILE_PATH, InputFile) == NULL) {
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

/**
  This function parses a stream file from the beginning to find a section.
  The section string may be anywhere within a line.
  // BUGBUG:  This is obsolete once genmake goes away...

  @param InputFile     Stream pointer.
  @param Section       Section to search for

  @retval FALSE if error or EOF
  @retval TRUE if section found
**/
BOOLEAN
FindSectionInStream (
  IN FILE       *InputFile,
  IN CHAR8      *Section
  )
{
  CHAR8 InputBuffer[MAX_LONG_FILE_PATH];
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
