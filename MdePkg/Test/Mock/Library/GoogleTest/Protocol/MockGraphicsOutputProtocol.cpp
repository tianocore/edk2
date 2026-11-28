/** @file
  Google Test mock for EFI_GRAPHICS_OUTPUT_PROTOCOL

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Protocol/MockGraphicsOutputProtocol.h>

MOCK_INTERFACE_DEFINITION (MockGraphicsOutputProtocol);
MOCK_FUNCTION_DEFINITION (MockGraphicsOutputProtocol, GopQueryMode, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockGraphicsOutputProtocol, GopSetMode, 2, EFIAPI);

EFI_GRAPHICS_OUTPUT_PROTOCOL  GRAPHICS_OUTPUT_PROTOCOL_INSTANCE = {
  GopQueryMode,  // QueryMode
  GopSetMode,    // SetMode
  NULL,          // Blt
  NULL           // Mode
};

extern "C" {
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGraphicsOutputProtocol = &GRAPHICS_OUTPUT_PROTOCOL_INSTANCE;
}
