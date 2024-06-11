/** @file MockDebugPort.h
    This file declares a mock of DebugPort Protocol.

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_DEBUG_PORT_PROTOCOL_H_
#define MOCK_DEBUG_PORT_PROTOCOL_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/DebugPort.h>
}
struct MockDebugPort {
  MOCK_INTERFACE_DECLARATION (MockDebugPort);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Reset,
    (IN EFI_DEBUGPORT_PROTOCOL  *This)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Write,
    (IN EFI_DEBUGPORT_PROTOCOL  *This,
     IN UINT32                  Timeout,
     IN OUT UINTN               *BufferSize,
     IN VOID                    *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Read,
    (IN EFI_DEBUGPORT_PROTOCOL  *This,
     IN UINT32                  Timeout,
     IN OUT UINTN               *BufferSize,
     IN VOID                    *Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Poll,
    (IN EFI_DEBUGPORT_PROTOCOL  *This)
    );
};

MOCK_INTERFACE_DEFINITION (MockDebugPort);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Reset, 1, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Write, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Read, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockDebugPort, Poll, 1, EFIAPI);

EFI_DEBUGPORT_PROTOCOL  DEBUG_PORT_PROTOCOL_INSTANCE = {
  Reset, // EFI_DEBUGPORT_RESET
  Write, // EFI_DEBUGPORT_WRITE
  Read,  // EFI_DEBUGPORT_READ
  Poll   // EFI_DEBUGPORT_POLLF
};

extern "C" {
  EFI_DEBUGPORT_PROTOCOL  *gDebugPortProtocol = &DEBUG_PORT_PROTOCOL_INSTANCE;
}

#endif // MOCK_DEBUG_PORT_PROTOCOL_H_
