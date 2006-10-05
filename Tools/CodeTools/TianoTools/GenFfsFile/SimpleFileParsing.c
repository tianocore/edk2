/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleFileParsing.c  

Abstract:

  Generic but simple file parsing routines.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>

#include "EfiUtilityMsgs.h"
#include "SimpleFileParsing.h"

#define MAX_PATH                    255
#define MAX_NEST_DEPTH              20  // just in case we get in an endless loop.
#define MAX_STRING_IDENTIFIER_NAME  100 // number of wchars
#define MAX_LINE_LEN                400

#define T_CHAR_SPACE                ' '
#define T_CHAR_NULL                 0
#define T_CHAR_CR                   '\r'
#define T_CHAR_TAB                  '\t'
#define T_CHAR_LF                   '\n'
#define T_CHAR_SLASH                '/'
#define T_CHAR_BACKSLASH            '\\'
#define T_CHAR_DOUBLE_QUOTE         '"'
#define T_CHAR_LC_X                 'x'
#define T_CHAR_0                    '0'

//
// We keep a linked list of these for the source files we process
//
typedef struct _SOURCE_FILE {
  FILE                *Fptr;
  T_CHAR              *FileBuffer;
  T_CHAR              *FileBufferPtr;
  UINT32              FileSize;
  INT8                FileName[MAX_PATH];
  UINT32              LineNum;
  BOOLEAN             EndOfFile;
  BOOLEAN             SkipToHash;
  struct _SOURCE_FILE *Previous;
  struct _SOURCE_FILE *Next;
  T_CHAR              ControlCharacter;
} SOURCE_FILE;

//
// Here's all our module globals.
//
static struct {
  SOURCE_FILE SourceFile;
  BOOLEAN     Verbose;
} mGlobals;

static
UINT32
t_strcmp (
  T_CHAR *Buffer,
  T_CHAR *Str
  );

static
UINT32
t_strncmp (
  T_CHAR *Str1,
  T_CHAR *Str2,
  UINT32 Len
  );

static
UINT32
t_strlen (
  T_CHAR *Str
  );

static
void
RewindFile (
  SOURCE_FILE *SourceFile
  );

static
BOOLEAN
SkipTo (
  SOURCE_FILE *SourceFile,
  T_CHAR      TChar,
  BOOLEAN     StopAfterNewline
  );

static
BOOLEAN
IsWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
UINT32
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
BOOLEAN
EndOfFile (
  SOURCE_FILE *SourceFile
  );

static
void
PreprocessFile (
  SOURCE_FILE *SourceFile
  );

//
// static
// T_CHAR *
// GetQuotedString (
//  SOURCE_FILE *SourceFile,
//  BOOLEAN     Optional
//  );
//
static
T_CHAR  *
t_strcpy (
  T_CHAR *Dest,
  T_CHAR *Src
  );

static
STATUS
ProcessIncludeFile (
  SOURCE_FILE *SourceFile,
  SOURCE_FILE *ParentSourceFile
  );

static
STATUS
ParseFile (
  SOURCE_FILE *SourceFile
  );

static
FILE    *
FindFile (
  IN INT8     *FileName,
  OUT INT8    *FoundFileName,
  IN UINT32   FoundFileNameLen
  );

static
STATUS
ProcessFile (
  SOURCE_FILE *SourceFile
  );

STATUS
SFPInit (
  VOID
  )
{
  memset ((void *) &mGlobals, 0, sizeof (mGlobals));
  return STATUS_SUCCESS;
}

UINT32
SFPGetLineNumber (
  VOID
  )
{
  return mGlobals.SourceFile.LineNum;
}

/*++

Routine Description:
  Return the line number of the file we're parsing. Used
  for error reporting purposes.

Arguments:
  None.

Returns:
  The line number, or 0 if no file is being processed

--*/
T_CHAR *
SFPGetFileName (
  VOID
  )
/*++

Routine Description:
  Return the name of the file we're parsing. Used
  for error reporting purposes.

Arguments:
  None.

Returns:
  A pointer to the file name. Null if no file is being
  processed.

--*/
{
  if (mGlobals.SourceFile.FileName[0]) {
    return mGlobals.SourceFile.FileName;
  }

  return NULL;
}

STATUS
SFPOpenFile (
  IN INT8   *FileName
  )
/*++

Routine Description:
  Open a file for parsing.

Arguments:
  FileName  - name of the file to parse

Returns:
  

--*/
{
  STATUS  Status;
  t_strcpy (mGlobals.SourceFile.FileName, FileName);
  Status = ProcessIncludeFile (&mGlobals.SourceFile, NULL);
  return Status;
}

BOOLEAN
SFPIsToken (
  T_CHAR *Str
  )
/*++

Routine Description:
  Check to see if the specified token is found at
  the current position in the input file.

Arguments:
  Str - the token to look for

Returns:
  TRUE - the token is next
  FALSE - the token is not next

Notes:
  We do a simple string comparison on this function. It is
  the responsibility of the caller to ensure that the token
  is not a subset of some other token.

  The file pointer is advanced past the token in the input file.

--*/
{
  UINT32  Len;
  SkipWhiteSpace (&mGlobals.SourceFile);

  if ((Len = t_strcmp (mGlobals.SourceFile.FileBufferPtr, Str)) > 0) {
    mGlobals.SourceFile.FileBufferPtr += Len;
    return TRUE;
  }

  return FALSE;

}

BOOLEAN
SFPGetNextToken (
  T_CHAR *Str,
  UINT32 Len
  )
{
  UINT32  Index;
  SkipWhiteSpace (&mGlobals.SourceFile);
  Index = 0;
  while (!EndOfFile (&mGlobals.SourceFile) && (Index < Len)) {
    if (IsWhiteSpace (&mGlobals.SourceFile)) {
      if (Index > 0) {
        Str[Index] = 0;
        return TRUE;
      }

      return FALSE;
    } else {
      Str[Index] = mGlobals.SourceFile.FileBufferPtr[0];
      mGlobals.SourceFile.FileBufferPtr++;
      Index++;
    }
  }

  return FALSE;
}

BOOLEAN
SFPSkipToToken (
  T_CHAR *Str
  )
{
  UINT32  Len;
  T_CHAR  *SavePos;
  Len     = t_strlen (Str);
  SavePos = mGlobals.SourceFile.FileBufferPtr;
  SkipWhiteSpace (&mGlobals.SourceFile);
  while (!EndOfFile (&mGlobals.SourceFile)) {
    if (t_strncmp (Str, mGlobals.SourceFile.FileBufferPtr, Len) == 0) {
      mGlobals.SourceFile.FileBufferPtr += Len;
      return TRUE;
    }

    mGlobals.SourceFile.FileBufferPtr++;
    SkipWhiteSpace (&mGlobals.SourceFile);
  }

  mGlobals.SourceFile.FileBufferPtr = SavePos;
  return FALSE;
}

BOOLEAN
SFPGetNumber (
  UINT32   *Value
  )
/*++

Routine Description:
  Check the token at the current file position for a numeric value.
  May be either decimal or hex.

Arguments:
  Value  - pointer where to store the value

Returns:
  FALSE    - current token is not a number
  TRUE     - current token is a number

--*/
{
  //
  //  UINT32 Len;
  //
  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

  if (isdigit (mGlobals.SourceFile.FileBufferPtr[0])) {
    //
    // Check for hex value
    //
    if ((mGlobals.SourceFile.FileBufferPtr[0] == T_CHAR_0) && (mGlobals.SourceFile.FileBufferPtr[1] == T_CHAR_LC_X)) {
      if (!isxdigit (mGlobals.SourceFile.FileBufferPtr[2])) {
        return FALSE;
      }

      mGlobals.SourceFile.FileBufferPtr += 2;
      sscanf (mGlobals.SourceFile.FileBufferPtr, "%x", Value);
      while (isxdigit (mGlobals.SourceFile.FileBufferPtr[0])) {
        mGlobals.SourceFile.FileBufferPtr++;
      }

      return TRUE;
    } else {
      *Value = atoi (mGlobals.SourceFile.FileBufferPtr);
      while (isdigit (mGlobals.SourceFile.FileBufferPtr[0])) {
        mGlobals.SourceFile.FileBufferPtr++;
      }

      return TRUE;
    }
  } else {
    return FALSE;
  }
}

STATUS
SFPCloseFile (
  VOID
  )
/*++

Routine Description:
  Close the file being parsed.

Arguments:
  None.

Returns:
  STATUS_SUCCESS - the file was closed 
  STATUS_ERROR   - no file is currently open

--*/
{
  if (mGlobals.SourceFile.FileBuffer != NULL) {
    free (mGlobals.SourceFile.FileBuffer);
    memset (&mGlobals.SourceFile, 0, sizeof (mGlobals.SourceFile));
    return STATUS_SUCCESS;
  }

  return STATUS_ERROR;
}

static
STATUS
ProcessIncludeFile (
  SOURCE_FILE *SourceFile,
  SOURCE_FILE *ParentSourceFile
  )
/*++

Routine Description:

  Given a source file, open the file and parse it
  
Arguments:

  SourceFile        - name of file to parse
  ParentSourceFile  - for error reporting purposes, the file that #included SourceFile.

Returns:

  Standard status.
  
--*/
{
  static UINT32 NestDepth = 0;
  INT8          FoundFileName[MAX_PATH];
  STATUS        Status;

  Status = STATUS_SUCCESS;
  NestDepth++;
  //
  // Print the file being processed. Indent so you can tell the include nesting
  // depth.
  //
  if (mGlobals.Verbose) {
    fprintf (stdout, "%*cProcessing file '%s'\n", NestDepth * 2, ' ', SourceFile->FileName);
  }

  //
  // Make sure we didn't exceed our maximum nesting depth
  //
  if (NestDepth > MAX_NEST_DEPTH) {
    Error (NULL, 0, 0, SourceFile->FileName, "max nesting depth (%d) exceeded", NestDepth);
    Status = STATUS_ERROR;
    goto Finish;
  }
  //
  // Try to open the file locally, and if that fails try along our include paths.
  //
  strcpy (FoundFileName, SourceFile->FileName);
  if ((SourceFile->Fptr = fopen (FoundFileName, "r")) == NULL) {
    //
    // Try to find it among the paths if it has a parent (that is, it is included
    // by someone else).
    //
    Error (NULL, 0, 0, SourceFile->FileName, "file not found");
    return STATUS_ERROR;
  }
  //
  // Process the file found
  //
  ProcessFile (SourceFile);
Finish:
  //
  // Close open files and return status
  //
  if (SourceFile->Fptr != NULL) {
    fclose (SourceFile->Fptr);
    SourceFile->Fptr = NULL;
  }

  return Status;
}

static
STATUS
ProcessFile (
  SOURCE_FILE *SourceFile
  )
{
  //
  // Get the file size, and then read the entire thing into memory.
  // Allocate space for a terminator character.
  //
  fseek (SourceFile->Fptr, 0, SEEK_END);
  SourceFile->FileSize = ftell (SourceFile->Fptr);
  fseek (SourceFile->Fptr, 0, SEEK_SET);
  SourceFile->FileBuffer = (T_CHAR *) malloc (SourceFile->FileSize + sizeof (T_CHAR));
  if (SourceFile->FileBuffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  fread ((VOID *) SourceFile->FileBuffer, SourceFile->FileSize, 1, SourceFile->Fptr);
  SourceFile->FileBuffer[(SourceFile->FileSize / sizeof (T_CHAR))] = T_CHAR_NULL;
  //
  // Pre-process the file to replace comments with spaces
  //
  PreprocessFile (SourceFile);
  SourceFile->LineNum = 1;
  return STATUS_SUCCESS;
}

static
void
PreprocessFile (
  SOURCE_FILE *SourceFile
  )
/*++

Routine Description:
  Preprocess a file to replace all carriage returns with NULLs so
  we can print lines from the file to the screen.
  
Arguments:
  SourceFile - structure that we use to keep track of an input file.

Returns:
  Nothing.
  
--*/
{
  BOOLEAN InComment;

  RewindFile (SourceFile);
  InComment = FALSE;
  while (!EndOfFile (SourceFile)) {
    //
    // If a line-feed, then no longer in a comment
    //
    if (SourceFile->FileBufferPtr[0] == T_CHAR_LF) {
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      InComment = 0;
    } else if (SourceFile->FileBufferPtr[0] == T_CHAR_CR) {
      //
      // Replace all carriage returns with a NULL so we can print stuff
      //
      SourceFile->FileBufferPtr[0] = 0;
      SourceFile->FileBufferPtr++;
    } else if (InComment) {
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
    } else if ((SourceFile->FileBufferPtr[0] == T_CHAR_SLASH) && (SourceFile->FileBufferPtr[1] == T_CHAR_SLASH)) {
      SourceFile->FileBufferPtr += 2;
      InComment = TRUE;
    } else {
      SourceFile->FileBufferPtr++;
    }
  }
  //
  // Could check for end-of-file and still in a comment, but
  // should not be necessary. So just restore the file pointers.
  //
  RewindFile (SourceFile);
}

#if 0
static
T_CHAR *
GetQuotedString (
  SOURCE_FILE *SourceFile,
  BOOLEAN     Optional
  )
{
  T_CHAR  *String;
  T_CHAR  *Start;
  T_CHAR  *Ptr;
  UINT32  Len;
  BOOLEAN PreviousBackslash;

  if (SourceFile->FileBufferPtr[0] != T_CHAR_DOUBLE_QUOTE) {
    if (!Optional) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted string", "%S", SourceFile->FileBufferPtr);
    }

    return NULL;
  }

  Len = 0;
  SourceFile->FileBufferPtr++;
  Start             = Ptr = SourceFile->FileBufferPtr;
  PreviousBackslash = FALSE;
  while (!EndOfFile (SourceFile)) {
    if ((SourceFile->FileBufferPtr[0] == T_CHAR_DOUBLE_QUOTE) && (!PreviousBackslash)) {
      break;
    } else if (SourceFile->FileBufferPtr[0] == T_CHAR_CR) {
      Warning (SourceFile->FileName, SourceFile->LineNum, 0, "carriage return found in quoted string", "%S", Start);
      PreviousBackslash = FALSE;
    } else if (SourceFile->FileBufferPtr[0] == T_CHAR_BACKSLASH) {
      PreviousBackslash = TRUE;
    } else {
      PreviousBackslash = FALSE;
    }

    SourceFile->FileBufferPtr++;
    Len++;
  }

  if (SourceFile->FileBufferPtr[0] != T_CHAR_DOUBLE_QUOTE) {
    Warning (SourceFile->FileName, SourceFile->LineNum, 0, "missing closing quote on string", "%S", Start);
  } else {
    SourceFile->FileBufferPtr++;
  }
  //
  // Now allocate memory for the string and save it off
  //
  String = (T_CHAR *) malloc ((Len + 1) * sizeof (T_CHAR));
  if (String == NULL) {
    Error (NULL, 0, 0, "memory allocation failed", NULL);
    return NULL;
  }
  //
  // Copy the string from the file buffer to the local copy.
  // We do no reformatting of it whatsoever at this point.
  //
  Ptr = String;
  while (Len > 0) {
    *Ptr = *Start;
    Start++;
    Ptr++;
    Len--;
  }

  *Ptr = 0;
  return String;
}
#endif
static
BOOLEAN
EndOfFile (
  SOURCE_FILE *SourceFile
  )
{
  //
  // The file buffer pointer will typically get updated before the End-of-file flag in the
  // source file structure, so check it first.
  //
  if (SourceFile->FileBufferPtr >= SourceFile->FileBuffer + SourceFile->FileSize / sizeof (T_CHAR)) {
    SourceFile->EndOfFile = TRUE;
    return TRUE;
  }

  if (SourceFile->EndOfFile) {
    return TRUE;
  }

  return FALSE;
}

#if 0
static
void
ProcessTokenInclude (
  SOURCE_FILE *SourceFile
  )
{
  INT8        IncludeFileName[MAX_PATH];
  INT8        *To;
  UINT32      Len;
  BOOLEAN     ReportedError;
  SOURCE_FILE IncludedSourceFile;

  ReportedError = FALSE;
  if (SkipWhiteSpace (SourceFile) == 0) {
    Warning (SourceFile->FileName, SourceFile->LineNum, 0, "expected whitespace following #include keyword", NULL);
  }
  //
  // Should be quoted file name
  //
  if (SourceFile->FileBufferPtr[0] != T_CHAR_DOUBLE_QUOTE) {
    Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted include file name", NULL);
    goto FailDone;
  }

  SourceFile->FileBufferPtr++;
  //
  // Copy the filename as ascii to our local string
  //
  To  = IncludeFileName;
  Len = 0;
  while (!EndOfFile (SourceFile)) {
    if ((SourceFile->FileBufferPtr[0] == T_CHAR_CR) || (SourceFile->FileBufferPtr[0] == T_CHAR_LF)) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "end-of-line found in quoted include file name", NULL);
      goto FailDone;
    }

    if (SourceFile->FileBufferPtr[0] == T_CHAR_DOUBLE_QUOTE) {
      SourceFile->FileBufferPtr++;
      break;
    }
    //
    // If too long, then report the error once and process until the closing quote
    //
    Len++;
    if (!ReportedError && (Len >= sizeof (IncludeFileName))) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "length of include file name exceeds limit", NULL);
      ReportedError = TRUE;
    }

    if (!ReportedError) {
      //
      // *To = UNICODE_TO_ASCII(SourceFile->FileBufferPtr[0]);
      //
      *To = (T_CHAR) SourceFile->FileBufferPtr[0];
      To++;
    }

    SourceFile->FileBufferPtr++;
  }

  if (!ReportedError) {
    *To = 0;
    memset ((char *) &IncludedSourceFile, 0, sizeof (SOURCE_FILE));
    strcpy (IncludedSourceFile.FileName, IncludeFileName);
    //
    // IncludedSourceFile.ControlCharacter = DEFAULT_CONTROL_CHARACTER;
    //
    ProcessIncludeFile (&IncludedSourceFile, SourceFile);
    //
    // printf ("including file '%s'\n", IncludeFileName);
    //
  }

  return ;
FailDone:
  //
  // Error recovery -- skip to next #
  //
  SourceFile->SkipToHash = TRUE;
}
#endif
static
BOOLEAN
IsWhiteSpace (
  SOURCE_FILE *SourceFile
  )
{
  switch (*SourceFile->FileBufferPtr) {
  case T_CHAR_NULL:
  case T_CHAR_CR:
  case T_CHAR_SPACE:
  case T_CHAR_TAB:
  case T_CHAR_LF:
    return TRUE;

  default:
    return FALSE;
  }
}

UINT32
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  )
{
  UINT32  Count;

  Count = 0;
  while (!EndOfFile (SourceFile)) {
    Count++;
    switch (*SourceFile->FileBufferPtr) {
    case T_CHAR_NULL:
    case T_CHAR_CR:
    case T_CHAR_SPACE:
    case T_CHAR_TAB:
      SourceFile->FileBufferPtr++;
      break;

    case T_CHAR_LF:
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      if (mGlobals.Verbose) {
        printf ("%d: %S\n", SourceFile->LineNum, SourceFile->FileBufferPtr);
      }
      break;

    default:
      return Count - 1;
    }
  }
  //
  // Some tokens require trailing whitespace. If we're at the end of the
  // file, then we count that as well.
  //
  if ((Count == 0) && (EndOfFile (SourceFile))) {
    Count++;
  }

  return Count;
}

static
UINT32
t_strcmp (
  T_CHAR *Buffer,
  T_CHAR *Str
  )
{
  UINT32  Len;

  Len = 0;
  while (*Str == *Buffer) {
    Buffer++;
    Str++;
    Len++;
  }

  if (*Str) {
    return 0;
  }

  return Len;
}

static
UINT32
t_strlen (
  T_CHAR *Str
  )
{
  UINT32  Len;
  Len = 0;
  while (*Str) {
    Len++;
    Str++;
  }

  return Len;
}

static
UINT32
t_strncmp (
  T_CHAR *Str1,
  T_CHAR *Str2,
  UINT32 Len
  )
{
  while (Len > 0) {
    if (*Str1 != *Str2) {
      return Len;
    }

    Len--;
    Str1++;
    Str2++;
  }

  return 0;
}

static
T_CHAR *
t_strcpy (
  T_CHAR *Dest,
  T_CHAR *Src
  )
{
  T_CHAR  *SaveDest;
  SaveDest = Dest;
  while (*Src) {
    *Dest = *Src;
    Dest++;
    Src++;
  }

  *Dest = 0;
  return SaveDest;
}

#if 0
static
BOOLEAN
IsValidIdentifierChar (
  INT8      Char,
  BOOLEAN   FirstChar
  )
{
  //
  // If it's the first character of an identifier, then
  // it must be one of [A-Za-z_].
  //
  if (FirstChar) {
    if (isalpha (Char) || (Char == '_')) {
      return TRUE;
    }
  } else {
    //
    // If it's not the first character, then it can
    // be one of [A-Za-z_0-9]
    //
    if (isalnum (Char) || (Char == '_')) {
      return TRUE;
    }
  }

  return FALSE;
}
#endif
static
void
RewindFile (
  SOURCE_FILE *SourceFile
  )
{
  SourceFile->LineNum       = 1;
  SourceFile->FileBufferPtr = SourceFile->FileBuffer;
  SourceFile->EndOfFile     = 0;
}

#if 0
static
BOOLEAN
SkipTo (
  SOURCE_FILE  *SourceFile,
  T_CHAR       TChar,
  BOOLEAN      StopAfterNewline
  )
{
  while (!EndOfFile (SourceFile)) {
    //
    // Check for the character of interest
    //
    if (SourceFile->FileBufferPtr[0] == TChar) {
      return TRUE;
    } else {
      if (SourceFile->FileBufferPtr[0] == T_CHAR_LF) {
        SourceFile->LineNum++;
        if (StopAfterNewline) {
          SourceFile->FileBufferPtr++;
          if (SourceFile->FileBufferPtr[0] == 0) {
            SourceFile->FileBufferPtr++;
          }

          return FALSE;
        }
      }

      SourceFile->FileBufferPtr++;
    }
  }

  return FALSE;
}
#endif
