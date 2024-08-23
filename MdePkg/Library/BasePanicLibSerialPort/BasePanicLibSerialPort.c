/** @file
  Base Serial Port Panic Library.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>
#include <Library/PanicLib.h>

//
// Define the maximum panic message length that this library supports
//
#define MAX_PANIC_MESSAGE_LENGTH  0x100

/**
  Prints a panic message containing a filename, line number, and description.
  This is always followed by a dead loop.

  Print a message of the form "PANIC <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  Immediately after that CpuDeadLoop() is called.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.
  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     The pointer to the name of the source file that generated the panic condition.
  @param  LineNumber   The line number in the source file that generated the panic condition
  @param  Description  The pointer to the description of the panic condition.

**/
VOID
EFIAPI
PanicReport (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  )
{
  CHAR8  Buffer[MAX_PANIC_MESSAGE_LENGTH];

  if (FileName == NULL) {
    FileName = "(NULL) Filename";
  }

  if (Description == NULL) {
    Description = "(NULL) Description";
  }

  //
  // Generate the PANIC message in ASCII format
  //
  AsciiSPrint (Buffer, sizeof (Buffer), "PANIC [%a] %a(%d): %a\n", gEfiCallerBaseName, FileName, LineNumber, Description);

  // Initialize Serial Port to write the panic message
  SerialPortInitialize ();

  //
  // Send the print string to the Logging device device
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));

  // Deadlooping because system is in an unrecoverable state.
  CpuDeadLoop ();
}
