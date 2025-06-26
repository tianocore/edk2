/** @file MockSimpleTextIn.h
    Google Test mocks for SimpleTextIn Protocol

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SIMPLE_TEXT_IN_PROTOCOL_H_
#define MOCK_SIMPLE_TEXT_IN_PROTOCOL_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Protocol/SimpleTextIn.h>
}
struct MockEfiSimpleTextInputProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiSimpleTextInputProtocol);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    MockEfiSimpleTextInProtocolReset,
    (IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
     IN BOOLEAN                         ExtendedVerification)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadKeyStroke,
    (IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *This,
     OUT EFI_INPUT_KEY                  *Key)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiSimpleTextInputProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextInputProtocol, MockEfiSimpleTextInProtocolReset, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiSimpleTextInputProtocol, ReadKeyStroke, 2, EFIAPI);

#define MOCK_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_INSTANCE(NAME)  \
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL NAME##_INSTANCE = {        \
    (EFI_INPUT_RESET)     MockEfiSimpleTextInProtocolReset, \
    (EFI_INPUT_READ_KEY)  ReadKeyStroke,                    \
    NULL };                                                 \
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif // MOCK_SIMPLE_TEXT_IN_PROTOCOL_H_
