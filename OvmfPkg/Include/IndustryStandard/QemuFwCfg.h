/** @file
  Macro and type definitions corresponding to the QEMU fw_cfg interface.

  Refer to "docs/specs/fw_cfg.txt" in the QEMU source directory.

  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013 - 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __FW_CFG_H__
#define __FW_CFG_H__

#include <Base.h>

//
// The size, in bytes, of names of firmware configuration files, including at
// least one terminating NUL byte.
//
#define QEMU_FW_CFG_FNAME_SIZE  56

//
// If the following bit is set in the UINT32 fw_cfg revision / feature bitmap
// -- read from key 0x0001 with the basic IO Port or MMIO method --, then the
// DMA interface is available.
//
#define FW_CFG_F_DMA  BIT1

//
// Macros for the FW_CFG_DMA_ACCESS.Control bitmap (in native encoding).
//
#define FW_CFG_DMA_CTL_ERROR   BIT0
#define FW_CFG_DMA_CTL_READ    BIT1
#define FW_CFG_DMA_CTL_SKIP    BIT2
#define FW_CFG_DMA_CTL_SELECT  BIT3
#define FW_CFG_DMA_CTL_WRITE   BIT4

//
// The fw_cfg registers can be found at these IO Ports
//
#define FW_CFG_IO_SELECTOR     0x510
#define FW_CFG_IO_DATA         0x511
#define FW_CFG_IO_DMA_ADDRESS  0x514

//
// Numerically defined keys.
//
typedef enum {
  QemuFwCfgItemSignature          = 0x0000,
  QemuFwCfgItemInterfaceVersion   = 0x0001,
  QemuFwCfgItemSystemUuid         = 0x0002,
  QemuFwCfgItemRamSize            = 0x0003,
  QemuFwCfgItemGraphicsEnabled    = 0x0004,
  QemuFwCfgItemSmpCpuCount        = 0x0005,
  QemuFwCfgItemMachineId          = 0x0006,
  QemuFwCfgItemKernelAddress      = 0x0007,
  QemuFwCfgItemKernelSize         = 0x0008,
  QemuFwCfgItemKernelCommandLine  = 0x0009,
  QemuFwCfgItemInitrdAddress      = 0x000a,
  QemuFwCfgItemInitrdSize         = 0x000b,
  QemuFwCfgItemBootDevice         = 0x000c,
  QemuFwCfgItemNumaData           = 0x000d,
  QemuFwCfgItemBootMenu           = 0x000e,
  QemuFwCfgItemMaximumCpuCount    = 0x000f,
  QemuFwCfgItemKernelEntry        = 0x0010,
  QemuFwCfgItemKernelData         = 0x0011,
  QemuFwCfgItemInitrdData         = 0x0012,
  QemuFwCfgItemCommandLineAddress = 0x0013,
  QemuFwCfgItemCommandLineSize    = 0x0014,
  QemuFwCfgItemCommandLineData    = 0x0015,
  QemuFwCfgItemKernelSetupAddress = 0x0016,
  QemuFwCfgItemKernelSetupSize    = 0x0017,
  QemuFwCfgItemKernelSetupData    = 0x0018,
  QemuFwCfgItemFileDir            = 0x0019,

  QemuFwCfgItemX86AcpiTables   = 0x8000,
  QemuFwCfgItemX86SmbiosTables = 0x8001,
  QemuFwCfgItemX86Irq0Override = 0x8002,
  QemuFwCfgItemX86E820Table    = 0x8003,
  QemuFwCfgItemX86HpetData     = 0x8004,
} FIRMWARE_CONFIG_ITEM;

//
// Communication structure for the DMA access method. All fields are encoded in
// big endian.
//
#pragma pack (1)
typedef struct {
  UINT32    Control;
  UINT32    Length;
  UINT64    Address;
} FW_CFG_DMA_ACCESS;
#pragma pack ()

#endif
