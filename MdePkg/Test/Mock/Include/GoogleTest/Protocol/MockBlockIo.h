/** @file MockBlockIo.h
  This file declares a mock of BlockIo protocol.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MOCK_BLOCK_IO_H_
#define MOCK_BLOCK_IO_H_

#include <Library/GoogleTestLib.h>
#include <Library/FunctionMockLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Protocol/BlockIo.h>
}
struct MockBlockIoProtocol {
  MOCK_INTERFACE_DECLARATION (MockBlockIoProtocol);

  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    Reset,
    (
     IN EFI_BLOCK_IO_PROTOCOL          *This,
     IN BOOLEAN                        ExtendedVerification
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    ReadBlocks,
    (
     IN EFI_BLOCK_IO_PROTOCOL * This,
     IN UINT32 MediaId,
     IN EFI_LBA Lba,
     IN UINTN BufferSize,
     OUT VOID *Buffer)
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    WriteBlocks,
    (
     IN EFI_BLOCK_IO_PROTOCOL          *This,
     IN UINT32                         MediaId,
     IN EFI_LBA                        Lba,
     IN UINTN                          BufferSize,
     IN VOID                           *Buffer
    )
    );
  MOCK_FUNCTION_DECLARATION (
    EFI_STATUS,
    FlushBlocks,
    (
     IN EFI_BLOCK_IO_PROTOCOL  *This)
    );
};

MOCK_INTERFACE_DEFINITION (MockBlockIoProtocol);
MOCK_FUNCTION_DEFINITION (MockBlockIoProtocol, Reset, 2, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBlockIoProtocol, ReadBlocks, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBlockIoProtocol, WriteBlocks, 5, EFIAPI);
MOCK_FUNCTION_DEFINITION (MockBlockIoProtocol, FlushBlocks, 1, EFIAPI);

#define MOCK_EFI_BLOCK_IO_PROTOCOL_INSTANCE(NAME, MEDIA)   \
  EFI_BLOCK_IO_PROTOCOL   NAME##_INSTANCE = {              \
    0,                                                     \
    MEDIA,                                                 \
    Reset,                                                 \
    ReadBlocks,                                            \
    WriteBlocks,                                           \
    FlushBlocks                                            \
  };                                                       \
  EFI_BLOCK_IO_PROTOCOL  *NAME = &NAME ## _INSTANCE;

#endif // MOCK_BLOCK_IO_H_
