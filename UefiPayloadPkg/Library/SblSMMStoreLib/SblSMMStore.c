/** @file  SblSMMStore.c

  Copyright (c) 2020, 9elements Agency GmbH<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/SMMStoreLib.h>

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
  )
{
  return EFI_UNSUPPORTED;
}


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
  IN        EFI_LBA                             Lba,
  IN        UINTN                               Offset,
  IN        UINTN                               *NumBytes,
  IN        UINT8                               *Buffer
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Erase a SMMStore block

  @param Lba    The logical block index to erase.

**/
EFI_STATUS
SMMStoreEraseBlock (
  IN   EFI_LBA                              Lba
  )
{
  return EFI_UNSUPPORTED;
}

VOID
EFIAPI
SMMStoreVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  return;
}

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
  )
{
  return EFI_UNSUPPORTED;
}
