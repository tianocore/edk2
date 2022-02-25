/** @file  SmmStoreLib.h

  Copyright (c) 2022, 9elements GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMM_STORE_LIB_H_
#define SMM_STORE_LIB_H_

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Guid/SmmStoreInfoGuid.h>

#define SMMSTORE_COMBUF_SIZE  16

/**
  Get the SmmStore block size

  @param BlockSize    The pointer to store the block size in.

**/
EFI_STATUS
SmmStoreLibGetBlockSize (
  OUT UINTN  *BlockSize
  );

/**
  Get the SmmStore number of blocks

  @param NumBlocks    The pointer to store the number of blocks in.

**/
EFI_STATUS
SmmStoreLibGetNumBlocks (
  OUT UINTN  *NumBlocks
  );

/**
  Get the SmmStore MMIO address

  @param MmioAddress    The pointer to store the address in.

**/
EFI_STATUS
SmmStoreLibGetMmioAddress (
  OUT EFI_PHYSICAL_ADDRESS  *MmioAddress
  );

/**
  Read from SmmStore

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
SmmStoreLibRead (
  IN        EFI_LBA  Lba,
  IN        UINTN    Offset,
  IN        UINTN    *NumBytes,
  IN        UINT8    *Buffer
  );

/**
  Write to SmmStore

  @param[in] Lba      The starting logical block index to write to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes On input, indicates the requested write size. On
                      output, indicates the actual number of bytes written
  @param[in] Buffer   Pointer to the data to write.

**/
EFI_STATUS
SmmStoreLibWrite (
  IN        EFI_LBA  Lba,
  IN        UINTN    Offset,
  IN        UINTN    *NumBytes,
  IN        UINT8    *Buffer
  );

/**
  Erase a block using the SmmStore

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SmmStoreLibEraseBlock (
  IN         EFI_LBA  Lba
  );

/**
  Initializes SmmStore support

  @retval EFI_WRITE_PROTECTED   The SmmStore is not present.
  @retval EFI_UNSUPPORTED       The SmmStoreInfo HOB wasn't found.
  @retval EFI_SUCCESS           The SmmStore is supported.

**/
EFI_STATUS
SmmStoreLibInitialize (
  VOID
  );

/**
  Denitializes SmmStore support
**/
VOID
EFIAPI
SmmStoreLibDeinitialize (
  VOID
  );

#endif /* SMM_STORE_LIB_H_ */
