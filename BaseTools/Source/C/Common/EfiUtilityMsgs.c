/** @file
EFI tools utility functions to display warning, error, and informational messages

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

--*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "EfiUtilityMsgs.h"

//
// Declare module globals for keeping track of the the utility's
// name and other settings.
//
STATIC STATUS mStatus                 = STATUS_SUCCESS;
STATIC CHAR8  mUtilityName[50]        = { 0 };
STATIC UINT64 mPrintLogLevel          = INFO_LOG_LEVEL;
STATIC CHAR8  *mSourceFileName        = NULL;
STATIC UINT32 mSourceFileLineNum      = 0;
STATIC UINT32 mErrorCount             = 0;
STATIC UINT32 mWarningCount           = 0;
STATIC UINT32 mMaxErrors              = 0;
STATIC UINT32 mMaxWarnings            = 0;
STATIC UINT32 mMaxWarningsPlusErrors  = 0;
STATIC INT8   mPrintLimitsSet         = 0;

STATIC
VOID
PrintLimitExceeded (
  VOID
  );

VOID
Error (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
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
  //
  // If limits have been set, then check that we have not exceeded them
  //
  if (mPrintLimitsSet) {
    //
    // See if we've exceeded our total count
    //
    if (mMaxWarningsPlusErrors != 0) {
      if (mErrorCount + mWarningCount > mMaxWarningsPlusErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
    //
    // See if we've exceeded our error count
    //
    if (mMaxErrors != 0) {
      if (mErrorCount > mMaxErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
  }

  mErrorCount++;
  va_start (List, MsgFmt);
  PrintMessage ("ERROR", FileName, LineNumber, MessageCode, Text, MsgFmt, List);
  va_end (List);
}

VOID
ParserError (
  UINT32  MessageCode,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
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

Returns:
  NA

--*/
{
  va_list List;
  //
  // If limits have been set, then check them
  //
  if (mPrintLimitsSet) {
    //
    // See if we've exceeded our total count
    //
    if (mMaxWarningsPlusErrors != 0) {
      if (mErrorCount + mWarningCount > mMaxWarningsPlusErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
    //
    // See if we've exceeded our error count
    //
    if (mMaxErrors != 0) {
      if (mErrorCount > mMaxErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
  }

  mErrorCount++;
  va_start (List, MsgFmt);
  PrintMessage ("ERROR", mSourceFileName, mSourceFileLineNum, MessageCode, Text, MsgFmt, List);
  va_end (List);
}

VOID
ParserWarning (
  UINT32  ErrorCode,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
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

Returns:
  NA

--*/
{
  va_list List;
  //
  // If limits have been set, then check them
  //
  if (mPrintLimitsSet) {
    //
    // See if we've exceeded our total count
    //
    if (mMaxWarningsPlusErrors != 0) {
      if (mErrorCount + mWarningCount > mMaxWarningsPlusErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
    //
    // See if we've exceeded our warning count
    //
    if (mMaxWarnings != 0) {
      if (mWarningCount > mMaxWarnings) {
        PrintLimitExceeded ();
        return ;
      }
    }
  }

  mWarningCount++;
  va_start (List, MsgFmt);
  PrintMessage ("WARNING", mSourceFileName, mSourceFileLineNum, ErrorCode, OffendingText, MsgFmt, List);
  va_end (List);
  //
  // Don't set warning status accordingly
  //
  //  if (mStatus < STATUS_WARNING) {
  //    mStatus = STATUS_WARNING;
  //  }
}

VOID
Warning (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
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

Returns:
  None.

--*/
{
  va_list List;

  //
  // Current Print Level not output warning information.
  //
  if (WARNING_LOG_LEVEL < mPrintLogLevel) {
    return;
  }
  //
  // If limits have been set, then check them
  //
  if (mPrintLimitsSet) {
    //
    // See if we've exceeded our total count
    //
    if (mMaxWarningsPlusErrors != 0) {
      if (mErrorCount + mWarningCount > mMaxWarningsPlusErrors) {
        PrintLimitExceeded ();
        return ;
      }
    }
    //
    // See if we've exceeded our warning count
    //
    if (mMaxWarnings != 0) {
      if (mWarningCount > mMaxWarnings) {
        PrintLimitExceeded ();
        return ;
      }
    }
  }

  mWarningCount++;
  va_start (List, MsgFmt);
  PrintMessage ("WARNING", FileName, LineNumber, MessageCode, Text, MsgFmt, List);
  va_end (List);
}

VOID
DebugMsg (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT64  MsgLevel,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a Debug message.

Arguments:
  FileName    - typically the name of the utility printing the debug message, but
                can be the name of a file being parsed.

  LineNumber  - the line number in FileName (parsers)

  MsgLevel    - Debug message print level (0~9)

  Text        - the text in question (parsers)

  MsgFmt      - the format string for the debug message. Can contain formatting
                controls for use with varargs.

Returns:
  None.

--*/
{
  va_list List;
  //
  // If the debug level is less than current print level, then do nothing.
  //
  if (MsgLevel < mPrintLogLevel) {
    return ;
  }

  va_start (List, MsgFmt);
  PrintMessage ("DEBUG", FileName, LineNumber, 0, Text, MsgFmt, List);
  va_end (List);
}

VOID
PrintMessage (
  CHAR8   *Type,
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
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
  List        - the variable list.

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
  CHAR8       Line[MAX_LINE_LEN];
  CHAR8       Line2[MAX_LINE_LEN];
  CHAR8       *Cptr;
  struct tm   *NewTime;
  time_t      CurrentTime;

  //
  // init local variable
  //
  Line[0] = '\0';
  Line2[0] = '\0';

  //
  // If given a filename, then add it (and the line number) to the string.
  // If there's no filename, then use the program name if provided.
  //
  if (FileName != NULL) {
    Cptr = FileName;
  } else {
    Cptr = NULL;
  }

  if (strcmp (Type, "DEBUG") == 0) {
    //
    // Debug Message requires current time.
    //
    time (&CurrentTime);
    NewTime = localtime (&CurrentTime);
    fprintf (stdout, "%04d-%02d-%02d %02d:%02d:%02d",
                     NewTime->tm_year + 1900,
                     NewTime->tm_mon + 1,
                     NewTime->tm_mday,
                     NewTime->tm_hour,
                     NewTime->tm_min,
                     NewTime->tm_sec
                     );
    if (Cptr != NULL) {
      sprintf (Line, ": %s", Cptr);
      if (LineNumber != 0) {
        sprintf (Line2, "(%u)", (unsigned) LineNumber);
        strcat (Line, Line2);
      }
    }
  } else {
    //
    // Error and Warning Information.
    //
    if (Cptr != NULL) {
      if (mUtilityName[0] != '\0') {
        fprintf (stdout, "%s...\n", mUtilityName);
      }
      sprintf (Line, "%s", Cptr);
      if (LineNumber != 0) {
        sprintf (Line2, "(%u)", (unsigned) LineNumber);
        strcat (Line, Line2);
      }
    } else {
      if (mUtilityName[0] != '\0') {
        sprintf (Line, "%s", mUtilityName);
      }
    }

    if (strcmp (Type, "ERROR") == 0) {
      //
      // Set status accordingly for ERROR information.
      //
      if (mStatus < STATUS_ERROR) {
        mStatus = STATUS_ERROR;
      }
    }
  }

  //
  // Have to print an error code or Visual Studio won't find the
  // message for you. It has to be decimal digits too.
  //
  if (MessageCode != 0) {
    sprintf (Line2, ": %s %04u", Type, (unsigned) MessageCode);
  } else {
    sprintf (Line2, ": %s", Type);
  }
  strcat (Line, Line2);
  fprintf (stdout, "%s", Line);
  //
  // If offending text was provided, then print it
  //
  if (Text != NULL) {
    fprintf (stdout, ": %s", Text);
  }
  fprintf (stdout, "\n");

  //
  // Print formatted message if provided
  //
  if (MsgFmt != NULL) {
    vsprintf (Line2, MsgFmt, List);
    fprintf (stdout, "  %s\n", Line2);
  }

}

STATIC
VOID
PrintSimpleMessage (
  CHAR8   *MsgFmt,
  va_list List
  )
/*++
Routine Description:
  Print message into stdout.

Arguments:
  MsgFmt      - the format string for the message. Can contain formatting
                controls for use with varargs.
  List        - the variable list.

Returns:
  None.
--*/
{
  CHAR8       Line[MAX_LINE_LEN];
  //
  // Print formatted message if provided
  //
  if (MsgFmt != NULL) {
    vsprintf (Line, MsgFmt, List);
    fprintf (stdout, "%s\n", Line);
  }
}

VOID
ParserSetPosition (
  CHAR8   *SourceFileName,
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

VOID
SetUtilityName (
  CHAR8   *UtilityName
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

VOID
SetPrintLevel (
  UINT64  LogLevel
  )
/*++

Routine Description:
  Set the printing message Level. This is used by the PrintMsg() function
  to determine when/if a message should be printed.

Arguments:
  LogLevel  - 0~50 to specify the different level message.

Returns:
  NA

--*/
{
  mPrintLogLevel = LogLevel;
}

VOID
VerboseMsg (
  CHAR8   *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a verbose level message.

Arguments:
  MsgFmt      - the format string for the message. Can contain formatting
                controls for use with varargs.
  List        - the variable list.

Returns:
  NA

--*/
{
  va_list List;
  //
  // If the debug level is less than current print level, then do nothing.
  //
  if (VERBOSE_LOG_LEVEL < mPrintLogLevel) {
    return ;
  }

  va_start (List, MsgFmt);
  PrintSimpleMessage (MsgFmt, List);
  va_end (List);
}

VOID
NormalMsg (
  CHAR8   *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a default level message.

Arguments:
  MsgFmt      - the format string for the message. Can contain formatting
                controls for use with varargs.
  List        - the variable list.

Returns:
  NA

--*/
{
  va_list List;
  //
  // If the debug level is less than current print level, then do nothing.
  //
  if (INFO_LOG_LEVEL < mPrintLogLevel) {
    return ;
  }

  va_start (List, MsgFmt);
  PrintSimpleMessage (MsgFmt, List);
  va_end (List);
}

VOID
KeyMsg (
  CHAR8   *MsgFmt,
  ...
  )
/*++

Routine Description:
  Print a key level message.

Arguments:
  MsgFmt      - the format string for the message. Can contain formatting
                controls for use with varargs.
  List        - the variable list.

Returns:
  NA

--*/
{
  va_list List;
  //
  // If the debug level is less than current print level, then do nothing.
  //
  if (KEY_LOG_LEVEL < mPrintLogLevel) {
    return ;
  }

  va_start (List, MsgFmt);
  PrintSimpleMessage (MsgFmt, List);
  va_end (List);
}

VOID
SetPrintLimits (
  UINT32  MaxErrors,
  UINT32  MaxWarnings,
  UINT32  MaxWarningsPlusErrors
  )
/*++

Routine Description:
  Set the limits of how many errors, warnings, and errors+warnings
  we will print.

Arguments:
  MaxErrors       - maximum number of error messages to print
  MaxWarnings     - maximum number of warning messages to print
  MaxWarningsPlusErrors
                  - maximum number of errors+warnings to print

Returns:
  NA

--*/
{
  mMaxErrors              = MaxErrors;
  mMaxWarnings            = MaxWarnings;
  mMaxWarningsPlusErrors  = MaxWarningsPlusErrors;
  mPrintLimitsSet         = 1;
}

STATIC
VOID
PrintLimitExceeded (
  VOID
  )
{
  STATIC INT8 mPrintLimitExceeded = 0;
  //
  // If we've already printed the message, do nothing. Otherwise
  // temporarily increase our print limits so we can pass one
  // more message through.
  //
  if (mPrintLimitExceeded == 0) {
    mPrintLimitExceeded++;
    mMaxErrors++;
    mMaxWarnings++;
    mMaxWarningsPlusErrors++;
    Error (NULL, 0, 0, "error/warning print limit exceeded", NULL);
    mMaxErrors--;
    mMaxWarnings--;
    mMaxWarningsPlusErrors--;
  }
}

#if 0
VOID
TestUtilityMessages (
  VOID
  )
{
  CHAR8 *ArgStr = "ArgString";
  int   ArgInt;

  ArgInt  = 0x12345678;
  //
  // Test without setting utility name
  //
  fprintf (stdout, "* Testing without setting utility name\n");
  fprintf (stdout, "** Test debug message not printed\n");
  DebugMsg (NULL, 0, 0x00000001, NULL, NULL);
  fprintf (stdout, "** Test warning with two strings and two args\n");
  Warning (NULL, 0, 1234, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
  fprintf (stdout, "** Test error with two strings and two args\n");
  Warning (NULL, 0, 1234, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
  fprintf (stdout, "** Test parser warning with nothing\n");
  ParserWarning (0, NULL, NULL);
  fprintf (stdout, "** Test parser error with nothing\n");
  ParserError (0, NULL, NULL);
  //
  // Test with utility name set now
  //
  fprintf (stdout, "** Testingin with utility name set\n");
  SetUtilityName ("MyUtilityName");
  //
  // Test debug prints
  //
  SetDebugMsgMask (2);
  fprintf (stdout, "** Test debug message with one string\n");
  DebugMsg (NULL, 0, 0x00000002, "Text1", NULL);
  fprintf (stdout, "** Test debug message with one string\n");
  DebugMsg (NULL, 0, 0x00000002, NULL, "Text2");
  fprintf (stdout, "** Test debug message with two strings\n");
  DebugMsg (NULL, 0, 0x00000002, "Text1", "Text2");
  fprintf (stdout, "** Test debug message with two strings and two args\n");
  DebugMsg (NULL, 0, 0x00000002, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
  //
  // Test warning prints
  //
  fprintf (stdout, "** Test warning with no strings\n");
  Warning (NULL, 0, 1234, NULL, NULL);
  fprintf (stdout, "** Test warning with one string\n");
  Warning (NULL, 0, 1234, "Text1", NULL);
  fprintf (stdout, "** Test warning with one string\n");
  Warning (NULL, 0, 1234, NULL, "Text2");
  fprintf (stdout, "** Test warning with two strings and two args\n");
  Warning (NULL, 0, 1234, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
  //
  // Test error prints
  //
  fprintf (stdout, "** Test error with no strings\n");
  Error (NULL, 0, 1234, NULL, NULL);
  fprintf (stdout, "** Test error with one string\n");
  Error (NULL, 0, 1234, "Text1", NULL);
  fprintf (stdout, "** Test error with one string\n");
  Error (NULL, 0, 1234, NULL, "Text2");
  fprintf (stdout, "** Test error with two strings and two args\n");
  Error (NULL, 0, 1234, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
  //
  // Test parser prints
  //
  fprintf (stdout, "** Test parser errors\n");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserError (1234, NULL, NULL);
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserError (1234, "Text1", NULL);
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserError (1234, NULL, "Text2");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserError (1234, "Text1", "Text2");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserError (1234, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);

  fprintf (stdout, "** Test parser warnings\n");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserWarning (4321, NULL, NULL);
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserWarning (4321, "Text1", NULL);
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserWarning (4321, NULL, "Text2");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserWarning (4321, "Text1", "Text2");
  ParserSetPosition (__FILE__, __LINE__ + 1);
  ParserWarning (4321, "Text1", "Text2 %s 0x%X", ArgStr, ArgInt);
}
#endif
