/** @file MockMmCommunication2.h
  Declare mock MM Communication Protocol 2.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_MM_COMMUNICATION2_H_
#define MOCK_MM_COMMUNICATION2_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>
extern "C" {
  #include <Uefi.h>
  #include <Pi/PiMultiPhase.h>
  #include <Protocol/MmCommunication2.h>
}

struct MockMmCommunication2Protocol {
  MOCK_INTERFACE_DECLARATION (MockMmCommunication2Protocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Communicate,
    (
     IN CONST EFI_MM_COMMUNICATION2_PROTOCOL   *This,
     IN OUT VOID                               *CommBufferPhysical,
     IN OUT VOID                               *CommBufferVirtual,
     IN OUT UINTN                              *CommSize OPTIONAL
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockMmCommunication2Protocol);
MOCK_FUNCTION_DEFINITION (MockMmCommunication2Protocol, Communicate, 4, EFIAPI);

#define MOCK_MM_COMMUNICATION2_PROTOCOL_INSTANCE(NAME)  \
  EFI_MM_COMMUNICATION2_PROTOCOL  NAME##_INSTANCE = {   \
    Communicate                                         \
  };                                                    \
  EFI_MM_COMMUNICATION2_PROTOCOL  *NAME = &NAME##_INSTANCE;

#endif
