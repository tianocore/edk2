/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  UtilsMsgs.c
  
Abstract:

  EFI tools utility functions to display warning, error, and informational
  messages.
  
--*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "Tiano.h"
#include "EfiUtilityMsgs.h"

#define MAX_LINE_LEN  200

//
// Declare module globals for keeping track of the the utility's
// name and other settings.
//
static STATUS mStatus             = STATUS_SUCCESS;
static INT8   mUtilityName[50]    = { 0 };
static INT8   *mSourceFileName    = NULL;
static UINT32 mSourceFileLineNum  = 0;
static UINT32 mErrorCount         = 0;
static UINT32 mWarningCount       = 0;
static UINT32 mDebugMsgMask       = 0;

static
void
PrintMessage (
  INT8    *Type,
  INT8    *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  INT8    *Text,
  INT8    *MsgFmt,
  va_list List
  );

void
Error (
  INT8    *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  INT8    *Text,
  INT8    *MsgFmt,
  ...
  )
/*++

Routine Description:
  Prints an error message.
  
Arguments:
  All arguments are optional, though the printed message may be useless if
  at least something valid is not specified.
  
  FileName - name of the file or application. If not specified, then the
             utilty name (as set by the utility calling SetUtilityName()
             earlier) is used. Otherwise "Unknown utility" is used.
  
  LineNumber - the line number of error, typically used by parsers. If the
               utility is not a parser, then 0 should be specified. Otherwise
               the FileName and LineNumber info can be used to cause
               MS Visual Studio to jump to the error.
               
  MessageCode - an application-specific error code that can be referenced in
              other documentation. 

  Text        - the text in question, typically used by parsers.
  
  MsgFmt - the format string for the error message. Can contain formatting
           controls for use with the varargs.
           
Returns:
  None.
  
Notes:
  We print the following (similar to the Warn() and Debug() 
  W
  Typical error/warning message format:

  bin\VfrCompile.cpp(330) : error C2660: 'AddVfrDataStructField' : function does not take 2 parameters

  BUGBUG -- these three utility functions are almost identical, and
  should be modified to share code.

  Visual Studio does not find error messages with:
  
     " error :"
     " error 1:"
     " error c1:"
     " error 1000:"
     " error c100:"

  It does find:
     " error c1000:"     
--*/
{
  va_list List;
  mErrorCount++;
  va_start (List, MsgFmt);
  PrintMessage ("error", FileName, LineNumber, MessageCode, Text, MsgFmt, List);
  va_end (List);
  //
  // Set status accordingly
  //
  if (mStatus < STATUS_ERROR) {
    mStatus = STATUS_ERROR;
  }
}

void
ParserError (
  UINT32  MessageCode,
  INT8    *Text,
  INT8    *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a parser error, using the source file name and line number
  set by a previous call to SetParserPosition().

Arguments:
  MessageCode   - application-specific error code
  Text          - text to print in the error message
  MsgFmt        - format string to print at the end of the error message
  ...

Returns:
  NA

--*/
{
  va_list List;
  mErrorCount++;
  va_start (List, MsgFmt);
  PrintMessage ("error", mSourceFileName, mSourceFileLineNum, MessageCode, Text, MsgFmt, List);
  va_end (List);
  //
  // Set status accordingly
  //
  if (mStatus < STATUS_ERROR) {
    mStatus = STATUS_ERROR;
  }
}

void
ParserWarning (
  UINT32  ErrorCode,
  INT8    *OffendingText,
  INT8    *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a parser warning, using the source file name and line number
  set by a previous call to SetParserPosition().

Arguments:
  ErrorCode     - application-specific error code
  OffendingText - text to print in the warning message
  MsgFmt        - format string to print at the end of the warning message
  ...

Returns:
  NA

--*/
{
  va_list List;
  mWarningCount++;
  va_start (List, MsgFmt);
  PrintMessage ("warning", mSourceFileName, mSourceFileLineNum, ErrorCode, OffendingText, MsgFmt, List);
  va_end (List);
  //
  // Set status accordingly
  //
  if (mStatus < STATUS_WARNING) {
    mStatus = STATUS_WARNING;
  }
}

void
Warning (
  INT8    *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  INT8    *Text,
  INT8    *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a warning message.

Arguments:
  FileName    - name of the file where the warning was detected, or the name
                of the application that detected the warning
  
  LineNumber  - the line number where the warning was detected (parsers).
                0 should be specified if the utility is not a parser.
               
  MessageCode - an application-specific warning code that can be referenced in
                other documentation. 

  Text        - the text in question (parsers)
  
  MsgFmt      - the format string for the warning message. Can contain formatting
                controls for use with varargs.
                
  ...
           
Returns:
  None.

--*/
{
  va_list List;
  mWarningCount++;
  va_start (List, MsgFmt);
  PrintMessage ("warning", FileName, LineNumber, MessageCode, Text, MsgFmt, List);
  va_end (List);
  //
  // Set status accordingly
  //
  if (mStatus < STATUS_WARNING) {
    mStatus = STATUS_WARNING;
  }
}

void
DebugMsg (
  INT8    *FileName,
  UINT32  LineNumber,
  UINT32  MsgMask,
  INT8    *Text,
  INT8    *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a warning message.

Arguments:
  FileName    - typically the name of the utility printing the debug message, but
                can be the name of a file being parsed.
  
  LineNumber  - the line number in FileName (parsers) 
               
  MsgMask     - an application-specific bitmask that, in combination with mDebugMsgMask,
                determines if the debug message gets printed.

  Text        - the text in question (parsers)
  
  MsgFmt      - the format string for the debug message. Can contain formatting
                controls for use with varargs.
          
  ... 
Returns:
  None.

--*/
{
  va_list List;
  //
  // If the debug mask is not applicable, then do nothing.
  //
  if ((MsgMask != 0) && ((mDebugMsgMask & MsgMask) == 0)) {
    return ;
  }

  va_start (List, MsgFmt);
  PrintMessage ("debug", FileName, LineNumber, 0, Text, MsgFmt, List);
  va_end (List);
}

static
void
PrintMessage (
  INT8    *Type,
  INT8    *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  INT8    *Text,
  INT8    *MsgFmt,
  va_list List
  )
/*++

Routine Description:
  Worker routine for all the utility printing services. Prints the message in
  a format that Visual Studio will find when scanning build outputs for
  errors or warnings.

Arguments:
  Type        - "warning" or "error" string to insert into the message to be
                printed. The first character of this string (converted to uppercase)
                is used to preceed the MessageCode value in the output string.

  FileName    - name of the file where the warning was detected, or the name
                of the application that detected the warning
  
  LineNumber  - the line number where the warning was detected (parsers).
                0 should be specified if the utility is not a parser.
               
  MessageCode - an application-specific warning code that can be referenced in
                other documentation. 

  Text        - part of the message to print
  
  MsgFmt      - the format string for the message. Can contain formatting
                controls for use with varargs.

  List        - Variable function parameter list.           
Returns:
  None.

Notes:
  If FileName == NULL then this utility will use the string passed into SetUtilityName(). 
  
  LineNumber is only used if the caller is a parser, in which case FileName refers to the
  file being parsed.

  Text and MsgFmt are both optional, though it would be of little use calling this function with
  them both NULL.

  Output will typically be of the form:
    <FileName>(<LineNumber>) : <Type> <Type[0]><MessageCode>: <Text> : <MsgFmt>

    Parser (LineNumber != 0)
      VfrCompile.cpp(330) : error E2660: AddVfrDataStructField : function does not take 2 parameters
    Generic utility (LineNumber == 0) 
      UtilityName : error E1234 : Text string : MsgFmt string and args

--*/
{
  INT8  Line[MAX_LINE_LEN];
  INT8  Line2[MAX_LINE_LEN];
  INT8  *Cptr;
  //
  // If given a filename, then add it (and the line number) to the string.
  // If there's no filename, then use the program name if provided.
  //
  if (FileName != NULL) {
    Cptr = FileName;
  } else if (mUtilityName[0] != 0) {
    Cptr = mUtilityName;
  } else {
    Cptr = "Unknown utility";
  }

  strcpy (Line, Cptr);
  if (LineNumber != 0) {
    sprintf (Line2, "(%d)", LineNumber);
    strcat (Line, Line2);
  }
  //
  // Have to print an error code or Visual Studio won't find the
  // message for you. It has to be decimal digits too.
  //
  sprintf (Line2, " : %s %c%04d", Type, toupper (Type[0]), MessageCode);
  strcat (Line, Line2);
  fprintf (stdout, "%s", Line);
  //
  // If offending text was provided, then print it
  //
  if (Text != NULL) {
    fprintf (stdout, ": %s ", Text);
  }
  //
  // Print formatted message if provided
  //
  if (MsgFmt != NULL) {
    vsprintf (Line2, MsgFmt, List);
    fprintf (stdout, ": %s", Line2);
  }

  fprintf (stdout, "\n");
}

void
ParserSetPosition (
  INT8    *SourceFileName,
  UINT32  LineNum
  )
/*++

Routine Description:
  Set the position in a file being parsed. This can be used to 
  print error messages deeper down in a parser.

Arguments:
  SourceFileName - name of the source file being parsed
  LineNum        - line number of the source file being parsed

Returns:
  NA

--*/
{
  mSourceFileName     = SourceFileName;
  mSourceFileLineNum  = LineNum;
}

void
SetUtilityName (
  INT8    *UtilityName
  )
/*++

Routine Description:
  All printed error/warning/debug messages follow the same format, and
  typically will print a filename or utility name followed by the error
  text. However if a filename is not passed to the print routines, then
  they'll print the utility name if you call this function early in your
  app to set the utility name.
  
Arguments:
  UtilityName  -  name of the utility, which will be printed with all
                  error/warning/debug messags.

Returns:
  NA
  
--*/
{
  //
  // Save the name of the utility in our local variable. Make sure its
  // length does not exceed our buffer.
  //
  if (UtilityName != NULL) {
    if (strlen (UtilityName) >= sizeof (mUtilityName)) {
      Error (UtilityName, 0, 0, "application error", "utility name length exceeds internal buffer size");
      strncpy (mUtilityName, UtilityName, sizeof (mUtilityName) - 1);
      mUtilityName[sizeof (mUtilityName) - 1] = 0;
      return ;
    } else {
      strcpy (mUtilityName, UtilityName);
    }
  } else {
    Error (NULL, 0, 0, "application error", "SetUtilityName() called with NULL utility name");
  }
}

STATUS
GetUtilityStatus (
  VOID
  )
/*++

Routine Description:
  When you call Error() or Warning(), this module keeps track of it and
  sets a local mStatus to STATUS_ERROR or STATUS_WARNING. When the utility
  exits, it can call this function to get the status and use it as a return
  value.
  
Arguments:
  None.

Returns:
  Worst-case status reported, as defined by which print function was called.
  
--*/
{
  return mStatus;
}
