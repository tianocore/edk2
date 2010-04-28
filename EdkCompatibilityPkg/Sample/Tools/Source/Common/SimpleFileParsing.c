/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
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

#include "Tiano.h"
#include "EfiUtilityMsgs.h"
#include "SimpleFileParsing.h"

#define MAX_PATH  255
//
// just in case we get in an endless loop.
//
#define MAX_NEST_DEPTH  20
//
// number of wchars
//
#define MAX_STRING_IDENTIFIER_NAME  100

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
#define T_CHAR_STAR                 '*'

//
// We keep a linked list of these for the source files we process
//
typedef struct _SOURCE_FILE {
  FILE                *Fptr;
  T_CHAR              *FileBuffer;
  T_CHAR              *FileBufferPtr;
  unsigned int        FileSize;
  char                FileName[MAX_PATH];
  unsigned int        LineNum;
  BOOLEAN             EndOfFile;
  BOOLEAN             SkipToHash;
  struct _SOURCE_FILE *Previous;
  struct _SOURCE_FILE *Next;
  T_CHAR              ControlCharacter;
} SOURCE_FILE;

typedef struct {
  T_CHAR  *FileBufferPtr;
} FILE_POSITION;

//
// Keep all our module globals in this structure
//
static struct {
  SOURCE_FILE SourceFile;
  BOOLEAN     VerboseFile;
  BOOLEAN     VerboseToken;
} mGlobals;

static
unsigned int
t_strcmp (
  T_CHAR *Buffer,
  T_CHAR *Str
  );

static
unsigned int
t_strncmp (
  T_CHAR *Str1,
  T_CHAR *Str2,
  int    Len
  );

static
unsigned int
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
IsWhiteSpace (
  SOURCE_FILE *SourceFile
  );

static
unsigned int
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
  char          *FileName,
  char          *FoundFileName,
  unsigned int  FoundFileNameLen
  );

static
STATUS
ProcessFile (
  SOURCE_FILE *SourceFile
  );

static
STATUS
GetFilePosition (
  FILE_POSITION *Fpos
  );

static
STATUS
SetFilePosition (
  FILE_POSITION *Fpos
  );

STATUS
SFPInit (
  VOID
  )
/*++

Routine Description:

Arguments:
  None.

Returns:
  STATUS_SUCCESS always

--*/
{
  memset ((void *) &mGlobals, 0, sizeof (mGlobals));
  return STATUS_SUCCESS;
}

unsigned
int
SFPGetLineNumber (
  VOID
  )
/*++

Routine Description:
  Return the line number of the file we're parsing. Used
  for error reporting purposes.

Arguments:
  None.

Returns:
  The line number, or 0 if no file is being processed

--*/
{
  return mGlobals.SourceFile.LineNum;
}

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
  char      *FileName
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
  unsigned int  Len;
  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

  if ((Len = t_strcmp (mGlobals.SourceFile.FileBufferPtr, Str)) > 0) {
    mGlobals.SourceFile.FileBufferPtr += Len;
    if (mGlobals.VerboseToken) {
      printf ("Token: '%s'\n", Str);
    }

    return TRUE;
  }

  return FALSE;
}

BOOLEAN
SFPIsKeyword (
  T_CHAR *Str
  )
/*++

Routine Description:
  Check to see if the specified keyword is found at
  the current position in the input file.

Arguments:
  Str - keyword to look for

Returns:
  TRUE - the keyword is next
  FALSE - the keyword is not next

Notes:
  A keyword is defined as a "special" string that has a non-alphanumeric
  character following it.

--*/
{
  unsigned int  Len;
  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

  if ((Len = t_strcmp (mGlobals.SourceFile.FileBufferPtr, Str)) > 0) {
    if (isalnum (mGlobals.SourceFile.FileBufferPtr[Len])) {
      return FALSE;
    }

    mGlobals.SourceFile.FileBufferPtr += Len;
    if (mGlobals.VerboseToken) {
      printf ("Token: '%s'\n", Str);
    }

    return TRUE;
  }

  return FALSE;
}

BOOLEAN
SFPGetNextToken (
  T_CHAR        *Str,
  unsigned int  Len
  )
/*++

Routine Description:
  Get the next token from the input stream. 

Arguments:
  Str - pointer to a copy of the next token
  Len - size of buffer pointed to by Str

Returns:
  TRUE  - next token successfully returned
  FALSE - otherwise

Notes:
  Preceeding white space is ignored. 
  The parser's buffer pointer is advanced past the end of the
  token.

--*/
{
  unsigned int  Index;
  T_CHAR        TempChar;

  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }
  //
  // Have to have enough string for at least one char and a null-terminator
  //
  if (Len < 2) {
    return FALSE;
  }
  //
  // Look at the first character. If it's an identifier, then treat it
  // as such
  //
  TempChar = mGlobals.SourceFile.FileBufferPtr[0];
  if (((TempChar >= 'a') && (TempChar <= 'z')) || ((TempChar >= 'A') && (TempChar <= 'Z')) || (TempChar == '_')) {
    Str[0] = TempChar;
    mGlobals.SourceFile.FileBufferPtr++;
    Index = 1;
    while (!EndOfFile (&mGlobals.SourceFile) && (Index < Len)) {
      TempChar = mGlobals.SourceFile.FileBufferPtr[0];
      if (((TempChar >= 'a') && (TempChar <= 'z')) ||
          ((TempChar >= 'A') && (TempChar <= 'Z')) ||
          ((TempChar >= '0') && (TempChar <= '9')) ||
          (TempChar == '_')
          ) {
        Str[Index] = mGlobals.SourceFile.FileBufferPtr[0];
        mGlobals.SourceFile.FileBufferPtr++;
        Index++;
      } else {
        //
        // Invalid character for symbol name, so break out
        //
        break;
      }
    }
    //
    // Null terminate and return success
    //
    Str[Index] = 0;
    return TRUE;
  } else if ((TempChar == ')') || (TempChar == '(') || (TempChar == '*')) {
    Str[0] = mGlobals.SourceFile.FileBufferPtr[0];
    mGlobals.SourceFile.FileBufferPtr++;
    Str[1] = 0;
    return TRUE;
  } else {
    //
    // Everything else is white-space (or EOF) separated
    //
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
    //
    // See if we just ran out of file contents, but did find a token
    //
    if ((Index > 0) && EndOfFile (&mGlobals.SourceFile)) {
      Str[Index] = 0;
      return TRUE;
    }
  }

  return FALSE;
}

BOOLEAN
SFPGetGuidToken (
  T_CHAR *Str,
  UINT32 Len
  )
/*++

Routine Description:
  Parse a GUID from the input stream. Stop when you discover white space.

Arguments:
  Str - pointer to a copy of the next token
  Len - size of buffer pointed to by Str

Returns:
  TRUE  - GUID string returned successfully
  FALSE - otherwise

--*/
{
  UINT32  Index;
  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

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
  unsigned int  Len;
  T_CHAR        *SavePos;
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
  unsigned int *Value
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
  static unsigned int NestDepth = 0;
  char                FoundFileName[MAX_PATH];
  STATUS              Status;

  Status = STATUS_SUCCESS;
  NestDepth++;
  //
  // Print the file being processed. Indent so you can tell the include nesting
  // depth.
  //
  if (mGlobals.VerboseFile) {
    fprintf (stdout, "%*cProcessing file '%s'\n", NestDepth * 2, ' ', SourceFile->FileName);
    fprintf (stdout, "Parent source file = '%s'\n", ParentSourceFile);
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
  if ((SourceFile->Fptr = fopen (FoundFileName, "rb")) == NULL) {
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
/*++

Routine Description:

  Given a source file that's been opened, read the contents into an internal
  buffer and pre-process it to remove comments.
  
Arguments:

  SourceFile        - structure containing info on the file to process

Returns:

  Standard status.
  
--*/
{
  //
  // Get the file size, and then read the entire thing into memory.
  // Allocate extra space for a terminator character.
  //
  fseek (SourceFile->Fptr, 0, SEEK_END);
  SourceFile->FileSize = ftell (SourceFile->Fptr);
  if (mGlobals.VerboseFile) {
    printf ("FileSize = %d (0x%X)\n", SourceFile->FileSize, SourceFile->FileSize);
  }

  fseek (SourceFile->Fptr, 0, SEEK_SET);
  SourceFile->FileBuffer = (T_CHAR *) malloc (SourceFile->FileSize + sizeof (T_CHAR));
  if (SourceFile->FileBuffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failure", NULL);
    return STATUS_ERROR;
  }

  fread ((void *) SourceFile->FileBuffer, SourceFile->FileSize, 1, SourceFile->Fptr);
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
  we can print lines (as part of error messages) from the file to the screen.
  
Arguments:
  SourceFile - structure that we use to keep track of an input file.

Returns:
  Nothing.
  
--*/
{
  BOOLEAN InComment;
  BOOLEAN SlashSlashComment;
  int     LineNum;

  RewindFile (SourceFile);
  InComment         = FALSE;
  SlashSlashComment = FALSE;
  while (!EndOfFile (SourceFile)) {
    //
    // If a line-feed, then no longer in a comment if we're in a // comment
    //
    if (SourceFile->FileBufferPtr[0] == T_CHAR_LF) {
      SourceFile->FileBufferPtr++;
      SourceFile->LineNum++;
      if (InComment && SlashSlashComment) {
        InComment         = FALSE;
        SlashSlashComment = FALSE;
      }
    } else if (SourceFile->FileBufferPtr[0] == T_CHAR_CR) {
      //
      // Replace all carriage returns with a NULL so we can print stuff
      //
      SourceFile->FileBufferPtr[0] = 0;
      SourceFile->FileBufferPtr++;
      //
      // Check for */ comment end
      //
    } else if (InComment &&
             !SlashSlashComment &&
             (SourceFile->FileBufferPtr[0] == T_CHAR_STAR) &&
             (SourceFile->FileBufferPtr[1] == T_CHAR_SLASH)
            ) {
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
      InComment = FALSE;
    } else if (InComment) {
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
      //
      // Check for // comments
      //
    } else if ((SourceFile->FileBufferPtr[0] == T_CHAR_SLASH) && (SourceFile->FileBufferPtr[1] == T_CHAR_SLASH)) {
      InComment         = TRUE;
      SlashSlashComment = TRUE;
      //
      // Check for /* comment start
      //
    } else if ((SourceFile->FileBufferPtr[0] == T_CHAR_SLASH) && (SourceFile->FileBufferPtr[1] == T_CHAR_STAR)) {
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
      SourceFile->FileBufferPtr[0] = T_CHAR_SPACE;
      SourceFile->FileBufferPtr++;
      SlashSlashComment = FALSE;
      InComment         = TRUE;
    } else {
      SourceFile->FileBufferPtr++;
    }
  }
  //
  // Could check for end-of-file and still in a comment, but
  // should not be necessary. So just restore the file pointers.
  //
  RewindFile (SourceFile);
  //
  // Dump the reformatted file if verbose mode
  //
  if (mGlobals.VerboseFile) {
    LineNum = 1;
    printf ("%04d: ", LineNum);
    while (!EndOfFile (SourceFile)) {
      if (SourceFile->FileBufferPtr[0] == T_CHAR_LF) {
        printf ("'\n%04d: '", ++LineNum);
      } else {
        printf ("%c", SourceFile->FileBufferPtr[0]);
      }

      SourceFile->FileBufferPtr++;
    }

    printf ("'\n");
    printf ("FileSize = %d (0x%X)\n", SourceFile->FileSize, SourceFile->FileSize);
    RewindFile (SourceFile);
  }
}

BOOLEAN
SFPGetQuotedString (
  T_CHAR      *Str,
  int         Length
  )
/*++

Routine Description:
  Retrieve a quoted-string from the input file. 
  
Arguments:
  Str    - pointer to a copy of the quoted string parsed
  Length - size of buffer pointed to by Str

Returns:
  TRUE    - next token in input stream was a quoted string, and
            the string value was returned in Str
  FALSE   - otherwise
  
--*/
{
  SkipWhiteSpace (&mGlobals.SourceFile);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

  if (mGlobals.SourceFile.FileBufferPtr[0] == T_CHAR_DOUBLE_QUOTE) {
    mGlobals.SourceFile.FileBufferPtr++;
    while (Length > 0) {
      if (EndOfFile (&mGlobals.SourceFile)) {
        return FALSE;
      }
      //
      // Check for closing quote
      //
      if (mGlobals.SourceFile.FileBufferPtr[0] == T_CHAR_DOUBLE_QUOTE) {
        mGlobals.SourceFile.FileBufferPtr++;
        *Str = 0;
        return TRUE;
      }

      *Str = mGlobals.SourceFile.FileBufferPtr[0];
      Str++;
      Length--;
      mGlobals.SourceFile.FileBufferPtr++;
    }
  }
  //
  // First character was not a quote, or the input string length was
  // insufficient to contain the quoted string, so return failure code.
  //
  return FALSE;
}

BOOLEAN
SFPIsEOF (
  VOID
  )
/*++

Routine Description:
  Return TRUE of FALSE to indicate whether or not we've reached the end of the
  file we're parsing.
  
Arguments:
  NA

Returns:
  TRUE    - EOF reached
  FALSE   - otherwise
  
--*/
{
  SkipWhiteSpace (&mGlobals.SourceFile);
  return EndOfFile (&mGlobals.SourceFile);
}

#if 0
static
T_CHAR *
GetQuotedString (
  SOURCE_FILE *SourceFile,
  BOOLEAN     Optional
  )
{
  T_CHAR        *String;
  T_CHAR        *Start;
  T_CHAR        *Ptr;
  unsigned int  Len;
  BOOLEAN       PreviousBackslash;

  if (SourceFile->FileBufferPtr[0] != T_CHAR_DOUBLE_QUOTE) {
    if (Optional == FALSE) {
      Error (SourceFile->FileName, SourceFile->LineNum, 0, "expected quoted string", "%S", SourceFile->FileBufferPtr);
    }

    return NULL;
  }

  Len = 0;
  SourceFile->FileBufferPtr++;
  Start             = Ptr = SourceFile->FileBufferPtr;
  PreviousBackslash = FALSE;
  while (!EndOfFile (SourceFile)) {
    if ((SourceFile->FileBufferPtr[0] == T_CHAR_DOUBLE_QUOTE) && (PreviousBackslash == FALSE)) {
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
  char          IncludeFileName[MAX_PATH];
  char          *To;
  unsigned int  Len;
  BOOLEAN       ReportedError;
  SOURCE_FILE   IncludedSourceFile;

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
      *To = (T_CHAR) SourceFile->FileBufferPtr[0];
      To++;
    }

    SourceFile->FileBufferPtr++;
  }

  if (!ReportedError) {
    *To = 0;
    memset ((char *) &IncludedSourceFile, 0, sizeof (SOURCE_FILE));
    strcpy (IncludedSourceFile.FileName, IncludeFileName);
    ProcessIncludeFile (&IncludedSourceFile, SourceFile);
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

unsigned int
SkipWhiteSpace (
  SOURCE_FILE *SourceFile
  )
{
  unsigned int  Count;

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
unsigned int
t_strcmp (
  T_CHAR *Buffer,
  T_CHAR *Str
  )
/*++

Routine Description:
  Compare two strings for equality. The string pointed to by 'Buffer' may or may not be null-terminated,
  so only compare up to the length of Str.

Arguments:
  Buffer  - pointer to first (possibly not null-terminated) string
  Str     - pointer to null-terminated string to compare to Buffer

Returns:
  Number of bytes matched if exact match
  0 if Buffer does not start with Str

--*/
{
  unsigned int  Len;

  Len = 0;
  while (*Str && (*Str == *Buffer)) {
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
unsigned int
t_strlen (
  T_CHAR *Str
  )
{
  unsigned int  Len;
  Len = 0;
  while (*Str) {
    Len++;
    Str++;
  }

  return Len;
}

static
unsigned int
t_strncmp (
  T_CHAR *Str1,
  T_CHAR *Str2,
  int    Len
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

static
UINT32
GetHexChars (
  T_CHAR      *Buffer,
  UINT32      BufferLen
  )
{
  UINT32  Len;
  Len = 0;
  while (!EndOfFile (&mGlobals.SourceFile) && (BufferLen > 0)) {
    if (isxdigit (mGlobals.SourceFile.FileBufferPtr[0])) {
      *Buffer = mGlobals.SourceFile.FileBufferPtr[0];
      Buffer++;
      Len++;
      BufferLen--;
      mGlobals.SourceFile.FileBufferPtr++;
    } else {
      break;
    }
  }
  //
  // Null terminate if we can
  //
  if ((Len > 0) && (BufferLen > 0)) {
    *Buffer = 0;
  }

  return Len;
}

BOOLEAN
SFPGetGuid (
  int         GuidStyle,
  EFI_GUID    *Value
  )
/*++

Routine Description:
  Parse a GUID from the input stream. Stop when you discover white space.

Arguments:
  GuidStyle - Style of the following GUID token
  Value     - pointer to EFI_GUID struct for output

Returns:
  TRUE  - GUID string parsed successfully
  FALSE - otherwise

  GUID styles
    Style[0] 12345678-1234-5678-AAAA-BBBBCCCCDDDD

--*/
{
  UINT32        Value32;
  UINT32        Index;
  FILE_POSITION FPos;
  T_CHAR        TempString[20];
  T_CHAR        TempString2[3];
  T_CHAR        *From;
  T_CHAR        *To;
  UINT32        Len;
  BOOLEAN       Status;

  Status = FALSE;
  //
  // Skip white space, then start parsing
  //
  SkipWhiteSpace (&mGlobals.SourceFile);
  GetFilePosition (&FPos);
  if (EndOfFile (&mGlobals.SourceFile)) {
    return FALSE;
  }

  if (GuidStyle == PARSE_GUID_STYLE_5_FIELDS) {
    //
    // Style[0] 12345678-1234-5678-AAAA-BBBBCCCCDDDD
    //
    Len = GetHexChars (TempString, sizeof (TempString));
    if ((Len == 0) || (Len > 8)) {
      goto Done;
    }

    sscanf (TempString, "%x", &Value32);
    Value->Data1 = Value32;
    //
    // Next two UINT16 fields
    //
    if (mGlobals.SourceFile.FileBufferPtr[0] != '-') {
      goto Done;
    }

    mGlobals.SourceFile.FileBufferPtr++;
    Len = GetHexChars (TempString, sizeof (TempString));
    if ((Len == 0) || (Len > 4)) {
      goto Done;
    }

    sscanf (TempString, "%x", &Value32);
    Value->Data2 = (UINT16) Value32;

    if (mGlobals.SourceFile.FileBufferPtr[0] != '-') {
      goto Done;
    }

    mGlobals.SourceFile.FileBufferPtr++;
    Len = GetHexChars (TempString, sizeof (TempString));
    if ((Len == 0) || (Len > 4)) {
      goto Done;
    }

    sscanf (TempString, "%x", &Value32);
    Value->Data3 = (UINT16) Value32;
    //
    // Parse the "AAAA" as two bytes
    //
    if (mGlobals.SourceFile.FileBufferPtr[0] != '-') {
      goto Done;
    }

    mGlobals.SourceFile.FileBufferPtr++;
    Len = GetHexChars (TempString, sizeof (TempString));
    if ((Len == 0) || (Len > 4)) {
      goto Done;
    }

    sscanf (TempString, "%x", &Value32);
    Value->Data4[0] = (UINT8) (Value32 >> 8);
    Value->Data4[1] = (UINT8) Value32;
    if (mGlobals.SourceFile.FileBufferPtr[0] != '-') {
      goto Done;
    }

    mGlobals.SourceFile.FileBufferPtr++;
    //
    // Read the last 6 bytes of the GUID
    //
    //
    Len = GetHexChars (TempString, sizeof (TempString));
    if ((Len == 0) || (Len > 12)) {
      goto Done;
    }
    //
    // Insert leading 0's to make life easier
    //
    if (Len != 12) {
      From            = TempString + Len - 1;
      To              = TempString + 11;
      TempString[12]  = 0;
      while (From >= TempString) {
        *To = *From;
        To--;
        From--;
      }

      while (To >= TempString) {
        *To = '0';
        To--;
      }
    }
    //
    // Now parse each byte
    //
    TempString2[2] = 0;
    for (Index = 0; Index < 6; Index++) {
      //
      // Copy the two characters from the input string to something
      // we can parse.
      //
      TempString2[0]  = TempString[Index * 2];
      TempString2[1]  = TempString[Index * 2 + 1];
      sscanf (TempString2, "%x", &Value32);
      Value->Data4[Index + 2] = (UINT8) Value32;
    }

    Status = TRUE;
  } else {
    //
    // Unsupported GUID style
    //
    return FALSE;
  }

Done:
  if (Status == FALSE) {
    SetFilePosition (&FPos);
  }

  return Status;
}

static
STATUS
GetFilePosition (
  FILE_POSITION *Fpos
  )
{
  Fpos->FileBufferPtr = mGlobals.SourceFile.FileBufferPtr;
  return STATUS_SUCCESS;
}

static
STATUS
SetFilePosition (
  FILE_POSITION *Fpos
  )
{
  //
  // Should check range of pointer
  //
  mGlobals.SourceFile.FileBufferPtr = Fpos->FileBufferPtr;
  return STATUS_SUCCESS;
}
