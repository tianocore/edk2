/** @file
  Google Test mock for EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockSimpleTextOutProtocol.h>

MOCK_INTERFACE_DEFINITION (MockSimpleTextOutProtocol);
MOCK_FUNCTION_DEFINITION (MockSimpleTextOutProtocol, TextQueryMode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSimpleTextOutProtocol, TextSetMode, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSimpleTextOutProtocol, TextOutputString, 2, EFIAPI);

EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  SIMPLE_TEXT_OUTPUT_PROTOCOL_INSTANCE = {
  NULL,             // Reset
  TextOutputString, // OutputString
  NULL,             // TestString
  TextQueryMode,    // QueryMode
  TextSetMode,      // SetMode
  NULL,             // SetAttribute
  NULL,             // ClearScreen
  NULL,             // SetCursorPosition
  NULL,             // EnableCursor
  NULL              // Mode
};

extern "C" {
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *gSimpleTextOutProtocol = &SIMPLE_TEXT_OUTPUT_PROTOCOL_INSTANCE;
}
