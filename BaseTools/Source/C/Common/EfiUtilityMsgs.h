/** @file
Defines and prototypes for common EFI utility error and debug messages.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_UTILITY_MSGS_H_
#define _EFI_UTILITY_MSGS_H_

#include <Common/UefiBaseTypes.h>

//
// Log message print Level
//
#define VERBOSE_LOG_LEVEL    15
#define WARNING_LOG_LEVEL    15
#define INFO_LOG_LEVEL       20
#define KEY_LOG_LEVEL        40
#define ERROR_LOG_LEVLE      50

//
// Status codes returned by EFI utility programs and functions
//
#define STATUS_SUCCESS  0
#define STATUS_WARNING  1
#define STATUS_ERROR    2
#define VOID void

typedef int STATUS;

#define MAX_LINE_LEN               0x200
#define MAXIMUM_INPUT_FILE_NUM     10

#ifdef __cplusplus
extern "C" {
#endif
//
// When we call Error() or Warning(), the module keeps track of the worst
// case reported. GetUtilityStatus() will get the worst-case results, which
// can be used as the return value from the app.
//
STATUS
GetUtilityStatus (
  VOID
  );

//
// If someone prints an error message and didn't specify a source file name,
// then we print the utility name instead. However they must tell us the
// utility name early on via this function.
//
VOID
SetUtilityName (
  CHAR8 *ProgramName
  )
;

VOID
PrintMessage (
  CHAR8   *Type,
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  MessageCode,
  CHAR8   *Text,
  CHAR8   *MsgFmt,
  va_list List
  );

VOID
Error (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  ErrorCode,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
  ...
  )
;

VOID
Warning (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT32  WarningCode,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
  ...
  )
;

VOID
DebugMsg (
  CHAR8   *FileName,
  UINT32  LineNumber,
  UINT64  MsgLevel,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
  ...
  )
;

VOID
VerboseMsg (
  CHAR8   *MsgFmt,
  ...
  );

VOID
NormalMsg (
  CHAR8   *MsgFmt,
  ...
  );

VOID
KeyMsg (
  CHAR8   *MsgFmt,
  ...
  );

VOID
SetPrintLevel (
  UINT64  LogLevel
  );

VOID
ParserSetPosition (
  CHAR8   *SourceFileName,
  UINT32  LineNum
  )
;

VOID
ParserError (
  UINT32  ErrorCode,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
  ...
  )
;

VOID
ParserWarning (
  UINT32  ErrorCode,
  CHAR8   *OffendingText,
  CHAR8   *MsgFmt,
  ...
  )
;

VOID
SetPrintLimits (
  UINT32  NumErrors,
  UINT32  NumWarnings,
  UINT32  NumWarningsPlusErrors
  )
;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _EFI_UTILITY_MSGS_H_
