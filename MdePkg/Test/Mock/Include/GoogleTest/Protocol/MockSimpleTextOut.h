/** @file MockSimpleTextOut.h
  Google Test mocks for SimpleTextOut Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SIMPLE_TEXT_OUT_PROTOCOL_H_
#define MOCK_SIMPLE_TEXT_OUT_PROTOCOL_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SimpleTextOut.h>
}
struct MockEfiSimpleTextOutProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSimpleTextOutProtocol);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockEfiSimpleTextOutProtocolReset,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN BOOLEAN                         ExtendedVerification)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    OutputString,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN CHAR16                          *String)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    TestString,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN CHAR16                          *String)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    QueryMode,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN UINTN                           ModeNumber,
     OUT UINTN                          *Columns,
     OUT UINTN                          *Rows)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,

    SetMode,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN UINTN                           ModeNumber)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetAttribute,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN UINTN                           Attribute)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ClearScreen,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetCursorPosition,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN UINTN                           Column,
     IN UINTN                           Row)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    EnableCursor,
    (IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *This,
     IN BOOLEAN                         Visible)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSimpleTextOutProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, MockEfiSimpleTextOutProtocolReset, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, OutputString, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, TestString, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, QueryMode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, SetMode, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, SetAttribute, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, ClearScreen, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, SetCursorPosition, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextOutProtocol, EnableCursor, 2, EFIAPI);

#define MOCK_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_INSTANCE(NAME) \
  EFI_SIMPLE_TEXT_OUTPUT_MODE NAME##_MODE_INSTANCE = {  \
    1,                                                  \
    0,                                                  \
    0,                                                  \
    0,                                                  \
    0,                                                  \
    FALSE};                                             \
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL NAME##_INSTANCE = {   \
    (EFI_TEXT_RESET)                MockEfiSimpleTextOutProtocolReset,  \
    (EFI_TEXT_STRING)               OutputString,                       \
    (EFI_TEXT_TEST_STRING)          TestString,                         \
    (EFI_TEXT_QUERY_MODE)           QueryMode,                          \
    (EFI_TEXT_SET_MODE)             SetMode,                            \
    (EFI_TEXT_SET_ATTRIBUTE)        SetAttribute,                       \
    (EFI_TEXT_CLEAR_SCREEN)         ClearScreen,                        \
    (EFI_TEXT_SET_CURSOR_POSITION)  SetCursorPosition,                  \
    (EFI_TEXT_ENABLE_CURSOR)        EnableCursor,                       \
    &NAME##_MODE_INSTANCE};                                             \
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif
