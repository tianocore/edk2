/** @file
Library functions for Setting QNC internal network port

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __QNC_ACCESS_LIB_H__
#define __QNC_ACCESS_LIB_H__

#include <IntelQNCRegs.h>

#define MESSAGE_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define ALT_MESSAGE_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_ALT_OPCODE_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define ALT_MESSAGE_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_ALT_OPCODE_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_IO_READ_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_IO_READ << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_IO_WRITE_DW(Port, Reg)  \
        (UINT32)((QUARK_OPCODE_IO_WRITE << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)

#define MESSAGE_SHADOW_DW(Port, Reg)  \
        (UINT32)((QUARK_DRAM_BASE_ADDR_READY << QNC_MCR_OP_OFFSET) | ((Port << QNC_MCR_PORT_OFFSET) & 0xFF0000) | ((Reg << QNC_MCR_REG_OFFSET) & 0xFF00) | 0xF0)


/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCPortRead(
  UINT8 Port,
  UINT32 RegAddress
  );

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCPortWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  );

/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCAltPortRead(
  UINT8 Port,
  UINT32 RegAddress
  );

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCAltPortWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  );

/**
  Read required data from QNC internal message network
**/
UINT32
EFIAPI
QNCPortIORead(
  UINT8 Port,
  UINT32 RegAddress
  );

/**
  Write prepared data into QNC internal message network.

**/
VOID
EFIAPI
QNCPortIOWrite (
  UINT8 Port,
  UINT32 RegAddress,
  UINT32 WriteValue
  );

/**
  This is for the special consideration for QNC MMIO write, as required by FWG,
  a reading must be performed after MMIO writing to ensure the expected write
  is processed and data is flushed into chipset

**/
RETURN_STATUS
EFIAPI
QNCMmIoWrite (
  UINT32             MmIoAddress,
  QNC_MEM_IO_WIDTH    Width,
  UINT32             DataNumber,
  VOID               *pData
  );

UINT32
EFIAPI
QncHsmmcRead (
  VOID
  );

VOID
EFIAPI
QncHsmmcWrite (
  UINT32 WriteValue
  );

VOID
EFIAPI
QncImrWrite (
  UINT32 ImrBaseOffset,
  UINT32 ImrLow,
  UINT32 ImrHigh,
  UINT32 ImrReadMask,
  UINT32 ImrWriteMask
  );

VOID
EFIAPI
QncIClkAndThenOr (
  UINT32 RegAddress,
  UINT32 AndValue,
  UINT32 OrValue
  );

VOID
EFIAPI
QncIClkOr (
  UINT32 RegAddress,
  UINT32 OrValue
  );

UINTN
EFIAPI
QncGetPciExpressBaseAddress (
  VOID
  );

#endif
