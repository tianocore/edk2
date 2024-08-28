/** @file
  Provides an API that can be used to halt boot during a critical error and
  leave a message describing what went wrong.  This should be used for unrecoverable
  errors in boot that usually occur in early in boot.
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PANIC_LIB_H_
#define PANIC_LIB_H_

/**
  Prints a panic message containing a filename, line number, and description.
  This is always followed by a dead loop.
  Print a message of the form "PANIC <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  Immediately after that CpuDeadLoop() is called.
  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.
  @param[in]  FileName     The pointer to the name of the source file that generated the panic condition.
  @param[in]  LineNumber   The line number in the source file that generated the panic condition
  @param[in]  Description  The pointer to the description of the panic condition.
**/
VOID
EFIAPI
PanicReport (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );

//
// Source file line number.
// Default is use the to compiler provided __LINE__ macro value. The __LINE__
// mapping can be overriden by predefining PANIC_LINE_NUMBER

//
// Defining PANIC_LINE_NUMBER to a fixed value is useful when comparing builds
// across source code formatting changes that may add/remove lines in a source
// file.
//
#ifndef PANIC_LINE_NUMBER
#define PANIC_LINE_NUMBER  __LINE__
#endif

// File name being printed out.
// Default is the compiler provided __FILE__ macro value. You can overwrite this
// by defining __FILE_NAME__
#if defined (__clang__) && defined (__FILE_NAME__)
#define _PANIC(Message)  PanicReport (__FILE_NAME__, PANIC_LINE_NUMBER, Message)
#else
#define _PANIC(Message)  PanicReport (__FILE__, PANIC_LINE_NUMBER, Message)
#endif

/**
  Macro that calls PanicReport().
  @param  Message  A format string
**/
#define PANIC(Message)        \
    do {                            \
      _PANIC (Message);     \
      ANALYZER_UNREACHABLE ();  \
    } while (FALSE)

#endif // PANIC_LIB_H_
