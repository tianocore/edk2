/** @file  SMMStoreLib.h

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SMM_STORE_LIB_H__
#define __SMM_STORE_LIB_H__

#include <Base.h>
#include <Uefi/UefiBaseType.h>
#include <Guid/SMMSTOREInfoGuid.h>

#define SMMSTORE_COMBUF_SIZE 16

/**
  Read from SMMStore

  @param[in] Lba      The starting logical block index to read from.
  @param[in] Offset   Offset into the block at which to begin reading.
  @param[in] NumBytes On input, indicates the requested read size. On
                      output, indicates the actual number of bytes read
  @param[in] Buffer   Pointer to the buffer to read into.

**/
EFI_STATUS
SMMStoreRead (
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );


/**
  Write to SMMStore

  @param[in] Lba      The starting logical block index to write to.
  @param[in] Offset   Offset into the block at which to begin writing.
  @param[in] NumBytes On input, indicates the requested write size. On
                      output, indicates the actual number of bytes written
  @param[in] Buffer   Pointer to the data to write.

**/
EFI_STATUS
SMMStoreWrite (
  IN        EFI_LBA                              Lba,
  IN        UINTN                                Offset,
  IN        UINTN                                *NumBytes,
  IN        UINT8                                *Buffer
  );


/**
  Erase a block using the SMMStore

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SMMStoreEraseBlock (
  IN         EFI_LBA                              Lba
  );


/**
  Notify the SMMStore Library about a VirtualNotify

**/

VOID
EFIAPI
SMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  );

/**
  Initializes SMMStore support

  @param[in] Ptr                  A runtime buffer where arguments are stored
                                  for SMM communication
  @param[in] SmmStoreInfoHob      A runtime buffer with a copy of the
                                  SmmStore Info Hob

  @retval EFI_WRITE_PROTECTED   The SMMSTORE is not present.
  @retval EFI_SUCCESS           The SMMSTORE is supported.

**/
EFI_STATUS
SMMStoreInitialize (
    IN         VOID                      *Ptr,
    IN         SMMSTORE_INFO             *SmmStoreInfoHob
  );

#endif /* __SMM_STORE_LIB_H__ */
