/** @file
  Declarations and macros for the console abstraction.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _LIBRARY_UEFI_CONSOLE_H
#define _LIBRARY_UEFI_CONSOLE_H
#include  <Protocol/SimpleTextOut.h>
#include  <Protocol/SimpleFileSystem.h>

/* The number of "special" character stream devices.
   These include:
    stdin, stdout, stderr
*/
#define NUM_SPECIAL   3

typedef struct {
  UINT32    Column;
  UINT32    Row;
} CursorXY;

typedef union {
  UINT64      Offset;
  CursorXY    XYpos;
} XYoffset;

/**
  In support of the "everything is a file" paradigm of the Standard C Library,
  the UEFI Console support is abstracted as an instance of EFI_FILE_PROTOCOL.
  The member functions of the protocol translate as:
    Open      Associates a stream with one of the pseudo-devices: stdin,
              stdout, or stderr; as defined by the UEFI System Table.
    Close     The stream is marked closed and released for use by a
              subsequent Open().
    Delete    Returns RETURN_UNSUPPORTED.  Does Nothing.
    Read      Blocks reading BufferSize characters using the
              EFI_SIMPLE_TEXT_INPUT_PROTOCOL::ReadKeyStroke() function.
    Write     Sends BufferSize characters to the console for display.  The
              output is examined for new line characters, '\n', which are then
              translated into a Return + New Line, '\r' '\n', sequence.
    GetPosition   Returns the number of characters input or output to this
                  stream.  Return, '\r', characters inserted due to line ending
                  translation are not counted.
    SetPosition   Only valid for Stdout or Stderr.  Offset is interpreted as a
                  CursorXY structure and is used to position the console cursor
                  using the EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL::SetCursorPosition()
                  call.
    GetInfo   Populates an EFI_FILE_INFO Buffer with no useful information.
    SetInfo   Returns RETURN_UNSUPPORTED.  Does Nothing.
    Flush     Returns RETURN_SUCCESS.  Does Nothing.

**/
typedef struct {
  UINT32                      Cookie;
  UINT32                      ConOutMode;   // Only valid for stdout & stderr
  UINT64                      NumRead;
  UINT64                      NumWritten;
  XYoffset                    MaxConXY;     // Only valid for stdout & stderr
  EFI_HANDLE                  Dev;          // Could be either Input or Output
  EFI_FILE_PROTOCOL           Abstraction;
} ConInstance;

EFI_STATUS
EFIAPI
ConOpen(
  IN  EFI_FILE_PROTOCOL        *This,
  OUT EFI_FILE_PROTOCOL       **NewHandle,
  IN  CHAR16                   *FileName,
  IN  UINT64                    OpenMode,
  IN  UINT64                    Attributes
);

#endif  /* _LIBRARY_UEFI_CONSOLE_H */
