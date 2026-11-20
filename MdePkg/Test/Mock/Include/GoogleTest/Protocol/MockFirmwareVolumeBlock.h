/** @file MockFirmwareVolumeBlock.h
  This file declares a mock of Firmware Volume Block Protocol

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Pi/PiFirmwareVolume.h>
  #include <Protocol/FirmwareVolumeBlock.h>
}

//
// Declarations to handle usage of the FirmwareVolumeBlockProtocol
//
struct MockEfiFirmwareVolumeBlockProtocol {
  MOCK_INTERFACE_DECLARATION (MockEfiFirmwareVolumeBlockProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     OUT       EFI_FVB_ATTRIBUTES_2* Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    SetAttributes,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     IN OUT    EFI_FVB_ATTRIBUTES_2* Attributes)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetPhysicalAddress,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     OUT       EFI_PHYSICAL_ADDRESS* Address)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    GetBlockSize,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     IN        EFI_LBA                             Lba,
     OUT       UINTN* BlockSize,
     OUT       UINTN* NumberOfBlocks)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Read,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     IN        EFI_LBA                             Lba,
     IN        UINTN                               Offset,
     IN OUT    UINTN* NumBytes,
     IN OUT    UINT8* Buffer)
    );

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Write,
    (
     IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL* This,
     IN        EFI_LBA                             Lba,
     IN        UINTN                               Offset,
     IN OUT    UINTN* NumBytes,
     IN        UINT8* Buffer)
    );
};

MOCK_INTERFACE_DEFINITION (MockEfiFirmwareVolumeBlockProtocol);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, GetAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, SetAttributes, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, GetPhysicalAddress, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, GetBlockSize, 4, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, Read, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockEfiFirmwareVolumeBlockProtocol, Write, 5, EFIAPI);

#define MOCK_EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_INSTANCE(NAME)   \
 EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL NAME##_INSTANCE = {         \
   GetAttributes,                                                \
   SetAttributes,                                                \
   GetPhysicalAddress,                                           \
   GetBlockSize,                                                 \
   Read,                                                         \
   Write };                                                      \
 EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL  *NAME = &NAME##_INSTANCE;
