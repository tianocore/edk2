/** @file
  Internal interfaces specific to the QemuFwCfgLibMmio instances in OvmfPkg.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (C) 2017, Advanced Micro Devices. All rights reserved
  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef QEMU_FW_CFG_LIB_MMIO_INTERNAL_H_
#define QEMU_FW_CFG_LIB_MMIO_INTERNAL_H_

typedef struct {
  UINTN    FwCfgSelectorAddress;
  UINTN    FwCfgDataAddress;
  UINTN    FwCfgDmaAddress;
} QEMU_FW_CFG_RESOURCE;

/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into  (OPTIONAL if Size is 0)

**/
typedef
VOID(EFIAPI READ_BYTES_FUNCTION)(
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  );

/**
  Writes bytes from a buffer to firmware configuration

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to transfer data from (OPTIONAL if Size is 0)

**/
typedef
VOID(EFIAPI WRITE_BYTES_FUNCTION)(
  IN UINTN Size,
  IN VOID  *Buffer OPTIONAL
  );

/**
  Skips bytes in firmware configuration

  @param[in] Size  Size in bytes to skip

**/
typedef
VOID(EFIAPI SKIP_BYTES_FUNCTION)(
  IN UINTN Size
  );

/**
  Reads firmware configuration bytes into a buffer

  @param[in] Size    Size in bytes to read
  @param[in] Buffer  Buffer to store data into  (OPTIONAL if Size is 0)

**/
extern
VOID (EFIAPI *InternalQemuFwCfgReadBytes)(
  IN UINTN  Size,
  IN VOID   *Buffer  OPTIONAL
  );

/**
  Writes bytes from a buffer to firmware configuration

  @param[in] Size    Size in bytes to write
  @param[in] Buffer  Buffer to transfer data from (OPTIONAL if Size is 0)

**/
extern
VOID (EFIAPI *InternalQemuFwCfgWriteBytes)(
  IN UINTN  Size,
  IN VOID   *Buffer  OPTIONAL
  );

/**
  Skips bytes in firmware configuration

  @param[in] Size  Size in bytes to skip

**/
extern
VOID (EFIAPI *InternalQemuFwCfgSkipBytes)(
  IN UINTN  Size
  );

/**
  Build firmware configure resource HOB.

  @param[in]   FwCfgResource  A pointer to firmware configure resource.

  @retval  NULL
**/
VOID
QemuBuildFwCfgResourceHob (
  IN QEMU_FW_CFG_RESOURCE  *FwCfgResource
  );

/**
  Get firmware configure resource HOB.

  @param VOID

  @retval  FwCfgResource    The firmware configure resouce in HOB.
**/
QEMU_FW_CFG_RESOURCE *
QemuGetFwCfgResourceHob (
  VOID
  );

/**
  To get firmware configure selector address.

  @param VOID

  @retval  firmware configure selector address
**/
UINTN
EFIAPI
QemuGetFwCfgSelectorAddress (
  VOID
  );

/**
  To get firmware configure Data address.

  @param VOID

  @retval  firmware configure data address
**/
UINTN
EFIAPI
QemuGetFwCfgDataAddress (
  VOID
  );

/**
  To get firmware DMA address.

  @param VOID

  @retval  firmware DMA address
**/
UINTN
EFIAPI
QemuGetFwCfgDmaAddress (
  VOID
  );

/**
  Slow READ_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  );

/**
  Slow WRITE_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  );

/**
  Slow SKIP_BYTES_FUNCTION.
**/
VOID
EFIAPI
MmioSkipBytes (
  IN UINTN  Size
  );

/**
  Fast READ_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaReadBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  );

/**
  Fast WRITE_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaWriteBytes (
  IN UINTN  Size,
  IN VOID   *Buffer OPTIONAL
  );

/**
  Fast SKIP_BYTES_FUNCTION.
**/
VOID
EFIAPI
DmaSkipBytes (
  IN UINTN  Size
  );

/**
  Transfer an array of bytes, or skip a number of bytes, using the DMA
  interface.

  @param[in]     Size     Size in bytes to transfer or skip.

  @param[in,out] Buffer   Buffer to read data into or write data from. Ignored,
                          and may be NULL, if Size is zero, or Control is
                          FW_CFG_DMA_CTL_SKIP.

  @param[in]     Control  One of the following:
                          FW_CFG_DMA_CTL_WRITE - write to fw_cfg from Buffer.
                          FW_CFG_DMA_CTL_READ  - read from fw_cfg into Buffer.
                          FW_CFG_DMA_CTL_SKIP  - skip bytes in fw_cfg.
**/
VOID
DmaTransferBytes (
  IN     UINTN   Size,
  IN OUT VOID    *Buffer OPTIONAL,
  IN     UINT32  Control
  );

#endif
