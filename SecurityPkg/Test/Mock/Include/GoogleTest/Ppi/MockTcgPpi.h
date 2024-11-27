/** @file MockTcgPpiLib.h
  Google Test mocks for TcgPpi

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_TCG_PPI_H__

#define MOCK_TCG_PPI_H__

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <PiPei.h>
  #include <Ppi/Tcg.h>
}

struct MockTcgPpiLib {
  MOCK_INTERFACE_DECLARATION (MockTcgPpiLib);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    HashLogExtendEvent,
    (IN      EDKII_TCG_PPI             *This,
     IN      UINT64                    Flags,
     IN      UINT8                     *HashData,
     IN      UINTN                     HashDataLen,
     IN      TCG_PCR_EVENT_HDR         *NewEventHdr,
     IN      UINT8                     *NewEventData)
    );
};

MOCK_INTERFACE_DEFINITION (MockTcgPpiLib);

MOCK_FUNCTION_DEFINITION (MockTcgPpiLib, HashLogExtendEvent, 6, EFIAPI);

#endif
