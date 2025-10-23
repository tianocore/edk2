/** @file MockSpiNorFlashProtocol.h
  This file declares a mock of SPI NOR Flash Protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_SPI_NOR_FLASH_PROTOCOL_H_
#define MOCK_SPI_NOR_FLASH_PROTOCOL_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/SpiNorFlash.h>
}

struct MockSpiNorFlashProtocol {
  MOCK_INTERFACE_DECLARATION (MockSpiNorFlashProtocol);
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetFlashid,
    (
     IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     OUT UINT8                             *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadData,
    (
     IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN  UINT32                            FlashAddress,
     IN  UINT32                            LengthInBytes,
     OUT UINT8                             *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    LfReadData,
    (
     IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN  UINT32                            FlashAddress,
     IN  UINT32                            LengthInBytes,
     OUT UINT8                             *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadStatus,
    (
     IN  CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN  UINT32                            LengthInBytes,
     OUT UINT8                             *FlashStatus
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    WriteStatus,
    (
     IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN UINT32                            LengthInBytes,
     IN UINT8                             *FlashStatus
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    WriteData,
    (
     IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN UINT32                            FlashAddress,
     IN UINT32                            LengthInBytes,
     IN UINT8                             *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Erase,
    (
     IN CONST EFI_SPI_NOR_FLASH_PROTOCOL  *This,
     IN UINT32                            FlashAddress,
     IN UINT32                            BlockCount
    )
    );
};

MOCK_INTERFACE_DEFINITION (MockSpiNorFlashProtocol);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, GetFlashid, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, ReadData, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, LfReadData, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, ReadStatus, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, WriteStatus, 3, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, WriteData, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockSpiNorFlashProtocol, Erase, 3, EFIAPI);

#define MOCK_EFI_SPI_NOR_FLASH_PROTOCOL_INSTANCE(NAME)  \
EFI_SPI_NOR_FLASH_PROTOCOL NAME##_INSTANCE = {          \
  NULL, \
  0, \
  {0, 0, 0}, \
  0, \
  GetFlashid, \
  ReadData, \
  LfReadData, \
  ReadStatus, \
  WriteStatus, \
  WriteData, \
  Erase \
}; \
EFI_SPI_NOR_FLASH_PROTOCOL   *NAME = &NAME##_INSTANCE;

#endif // MOCK_SPI_NOR_FLASH_PROTOCOL_H_
