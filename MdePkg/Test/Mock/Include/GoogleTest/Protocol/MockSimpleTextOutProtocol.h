/** @file
  This file declares a mock of EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL.

  QueryMode(), SetMode(), and OutputString() are mocked so far -- extend
  with additional MOCK_FUNCTION_DECLARATION/DEFINITION pairs (and
  instance fields) as other consumers need more of this protocol's
  surface. Reset/TestString/SetAttribute/ClearScreen/SetCursorPosition/
  EnableCursor are left NULL in the instance below since nothing under
  test calls them. Mode is a plain data pointer (not a method) --
  callers populate their own EFI_SIMPLE_TEXT_OUTPUT_MODE instance
  directly and assign it to gSimpleTextOutProtocol->Mode, no mocking
  needed for it.

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SimpleTextOut.h>
}

struct MockSimpleTextOutProtocol {
  MOCK_INTERFACE_DECLARATION (MockSimpleTextOutProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    TextQueryMode,
    (IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN  UINTN                           ModeNumber,
     OUT UINTN                           *Columns,
     OUT UINTN                           *Rows)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    TextSetMode,
    (IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN  UINTN                           ModeNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    TextOutputString,
    (IN  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN  CHAR16                          *String)
    );
};

extern "C" {
  extern EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *gSimpleTextOutProtocol;
}
