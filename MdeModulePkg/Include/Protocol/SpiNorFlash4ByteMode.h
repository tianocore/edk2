/** @file SpiNorFlash4ByteMode.h
  Protocol definitions for SPI NOR Flash 4-Byte Address Mode transitions
  (Enter 4-Byte: EN4B opcode 0xB7, Exit 4-Byte: EX4B opcode 0xE9).

  DXE:  gEfiSpiNorFlash4ByteModeProtocolGuid
  SMM:  gEfiSpiNorFlash4ByteModeSmmProtocolGuid

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Protocol/SpiIo.h>
#include <IndustryStandard/SpiNorFlashJedecSfdp.h>

///
/// Forward declaration.
///
typedef struct _EFI_SPI_NOR_FLASH_4BYTE_MODE_PROTOCOL EFI_SPI_NOR_FLASH_4BYTE_MODE_PROTOCOL;

/**
  Check the AMD FCH SPI address mode and, if the hardware is not already in
  4-byte mode, issue the Enter 4-Byte Address (EN4B, opcode 0xB7) command to
  the flash chip via the supplied SPI I/O interface.

  @param[in]  This         Pointer to the protocol instance.
  @param[in]  SpiIo        The SPI I/O protocol for the target flash device.
  @param[out] AddressMode  Resolved address mode: SPI_ADDR_4BYTE_ONLY when
                           EN4B was sent successfully (or already active),
                           SPI_ADDR_3BYTE_ONLY otherwise.

  @retval EFI_SUCCESS      Address mode resolved.
  @retval other            SPI transaction to send EN4B failed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_ENTER_4BYTE_MODE)(
  IN  EFI_SPI_NOR_FLASH_4BYTE_MODE_PROTOCOL  *This,
  IN  EFI_SPI_IO_PROTOCOL                    *SpiIo,
  OUT UINT8                                  *AddressMode
  );

/**
  Check the AMD FCH SPI address mode and, if the hardware is currently in
  4-byte mode, issue the Exit 4-Byte Address (EX4B, opcode 0xE9) command to
  the flash chip via the supplied SPI I/O interface so it returns to 3-byte
  addressing.

  @param[in]  This         Pointer to the protocol instance.
  @param[in]  SpiIo        The SPI I/O protocol for the target flash device.
  @param[out] AddressMode  Resolved address mode: always SPI_ADDR_3BYTE_ONLY
                           on success.

  @retval EFI_SUCCESS      Address mode resolved (EX4B sent if required).
  @retval other            SPI transaction to send EX4B failed.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SPI_NOR_FLASH_EXIT_4BYTE_MODE)(
  IN  EFI_SPI_NOR_FLASH_4BYTE_MODE_PROTOCOL  *This,
  IN  EFI_SPI_IO_PROTOCOL                    *SpiIo,
  OUT UINT8                                  *AddressMode
  );

///
/// Protocol published by SpiNorFlash4ByteModeDxe / SpiNorFlash4ByteModeSmm.
/// Installing this protocol signals that 4-byte address mode transitions are
/// available for the current platform.
///
struct _EFI_SPI_NOR_FLASH_4BYTE_MODE_PROTOCOL {
  EFI_SPI_NOR_FLASH_ENTER_4BYTE_MODE    Enter4ByteMode; ///< Send EN4B and resolve address mode.
  EFI_SPI_NOR_FLASH_EXIT_4BYTE_MODE     Exit4ByteMode;  ///< Send EX4B and resolve address mode.
};

extern EFI_GUID  gEfiSpiNorFlash4ByteModeProtocolGuid;
extern EFI_GUID  gEfiSpiNorFlash4ByteModeSmmProtocolGuid;
