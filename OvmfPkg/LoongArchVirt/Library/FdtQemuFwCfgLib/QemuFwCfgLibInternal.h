/** @file
  fw_cfg library implementation.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FwCfg   - firmWare  Configure
**/

#ifndef QEMU_FW_CFG_LIB_INTERNAL_H_
#define QEMU_FW_CFG_LIB_INTERNAL_H_

#define FW_CONFIG_SELECTOR_ADDRESS_HOB_GUID \
  { \
    0x3cc47b04, 0x0d3e, 0xaa64, { 0x06, 0xa6, 0x4b, 0xdc, 0x9a, 0x2c, 0x61, 0x19 } \
  }

#define FW_CONFIG_DATA_ADDRESS_HOB_GUID \
  { \
    0xef854788, 0x10f3, 0x8e7a, { 0x3e, 0xd0, 0x4d, 0x16, 0xc1, 0x79, 0x55, 0x2f } \
  }

/**
  Returns a boolean indicating if the firmware configuration interface is
  available for library-internal purposes.

  This function never changes fw_cfg state.

  @retval    TRUE   The interface is available internally.
  @retval    FALSE  The interface is not available internally.
**/
BOOLEAN
InternalQemuFwCfgIsAvailable (
  VOID
  );

/**
  Returns a boolean indicating whether QEMU provides the DMA-like access method
  for fw_cfg.

  @retval    TRUE   The DMA-like access method is available.
  @retval    FALSE  The DMA-like access method is unavailable.
**/
BOOLEAN
InternalQemuFwCfgDmaIsAvailable (
  VOID
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
InternalQemuFwCfgDmaBytes (
  IN     UINT32  Size,
  IN OUT VOID    *Buffer OPTIONAL,
  IN     UINT32  Control
  );

#endif // QEMU_FW_CFG_LIB_INTERNAL_H_
