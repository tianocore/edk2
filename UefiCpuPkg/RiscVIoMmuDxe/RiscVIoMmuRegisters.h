/** @file
  RISC-V IOMMU registers.

  TODO: Continue detailed read of register dump from CQCSR onwards.

  TODO: Capture attributes and description, if needed. Also, more N_* defines.
  - Debug support, if desired.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RISC_V_IO_MMU_REGISTERS_
#define _RISC_V_IO_MMU_REGISTERS_

#include <PiDxe.h>

#define R_RISCV_IOMMU_CAPABILITIES   0x00

#define V_RISCV_IOMMU_CAPABILITIES_VERSION_1_0   0x10
#define V_RISCV_IOMMU_CAPABILITIES_IGS_MSI       0x00
#define V_RISCV_IOMMU_CAPABILITIES_IGS_WSI       0x01
#define V_RISCV_IOMMU_CAPABILITIES_IGS_BOTH      0x02
#define V_RISCV_IOMMU_CAPABILITIES_IGS_RESERVED  0x03

typedef union {
  struct {
    UINT64  version   : 8;
    UINT64  Sv32      : 1;
    UINT64  Sv39      : 1;
    UINT64  Sv48      : 1;
    UINT64  Sv57      : 1;
    UINT64  Reserved0 : 3;
    UINT64  Svpbmt    : 1;
    UINT64  Sv32x4    : 1;
    UINT64  Sv39x4    : 1;
    UINT64  Sv48x4    : 1;
    UINT64  Sv57x4    : 1;
    UINT64  Reserved1 : 1;
    UINT64  AMO_MRIF  : 1;
    UINT64  MSI_FLAT  : 1;
    UINT64  MSI_MRIF  : 1;
    UINT64  AMO_HWAD  : 1;
    UINT64  ATS       : 1;
    UINT64  T2GPA     : 1;
    UINT64  END       : 1;
    UINT64  IGS       : 2;
    UINT64  HPM       : 1;
    UINT64  DBG       : 1;
    UINT64  PAS       : 6;
    UINT64  PD8       : 1;
    UINT64  PD17      : 1;
    UINT64  PD20      : 1;
    UINT64  Reserved2 : 8;
    UINT64  Custom    : 8;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_CAPABILITIES;

#define R_RISCV_IOMMU_FCTL           0x08

typedef union {
  struct {
    UINT32  BE       : 1;
    UINT32  WSI      : 1;
    UINT32  GXL      : 1;
    UINT32  Reserved : 12;
    UINT32  Custom   : 16;
  } Bits;
  UINT32  Uint32;
} RISCV_IOMMU_FCTL;

#define R_RISCV_IOMMU_CUSTOM_1       0x0c

#define R_RISCV_IOMMU_DDTP           0x10

// Values 5-13 are reserved, values 14-15 are custom
#define V_RISCV_IOMMU_DDTP_IOMMU_MODE_OFF   0
#define V_RISCV_IOMMU_DDTP_IOMMU_MODE_BARE  1
#define V_RISCV_IOMMU_DDTP_IOMMU_MODE_1LVL  2
#define V_RISCV_IOMMU_DDTP_IOMMU_MODE_2LVL  3
#define V_RISCV_IOMMU_DDTP_IOMMU_MODE_3LVL  4

#define N_RISCV_IOMMU_DDTP_BUSY  4

typedef union {
  struct {
    UINT64  iommu_mode : 4;
    UINT64  busy       : 1;
    UINT64  Reserved0  : 5;
    UINT64  PPN        : 44;
    UINT64  Reserved1  : 10;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_DDTP;

#define QUEUE_MAX_LOG_SIZE           16

#define R_RISCV_IOMMU_CQB            0x18

#define COMMAND_QUEUE_ENTRY_SIZE       16

#define R_RISCV_IOMMU_FQB            0x28

#define FAULT_QUEUE_ENTRY_SIZE         32

#define R_RISCV_IOMMU_PQB            0x38

#define PAGE_REQUEST_QUEUE_ENTRY_SIZE  16

typedef union {
  struct {
    UINT64  LOG2SZ_1  : 5;
    UINT64  Reserved0 : 5;
    UINT64  PPN       : 44;
    UINT64  Reserved1 : 10;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_QUEUE_BASE;

#define R_RISCV_IOMMU_CQH            0x20
#define R_RISCV_IOMMU_CQT            0x24

#define R_RISCV_IOMMU_FQH            0x30
#define R_RISCV_IOMMU_FQT            0x34

#define R_RISCV_IOMMU_PQH            0x40
#define R_RISCV_IOMMU_PQT            0x44

typedef union {
  struct {
    UINT32  index : 32;
  } Bits;
  UINT32  Uint32;
} RISCV_IOMMU_QUEUE_POINTER;

#define N_RISCV_IOMMU_QUEUE_CSR_QON   16

#define R_RISCV_IOMMU_CQCSR          0x48

typedef union {
  struct {
    UINT32  qen        : 1;
    UINT32  ie         : 1;
    UINT32  Reserved0  : 6;
    UINT32  qmf        : 1;
    UINT32  cmd_to     : 1;
    UINT32  cmd_ill    : 1;
    UINT32  fence_w_ip : 1;
    UINT32  Reserved1  : 4;
    UINT32  qon        : 1;
    UINT32  busy       : 1;
    UINT32  Reserved2  : 10;
    UINT32  Custom     : 4;
  } Bits;
  UINT32  Uint32;
} RISCV_IOMMU_SOFTWARE_REQUEST_QUEUE_CSR;

#define R_RISCV_IOMMU_FQCSR          0x4c

#define R_RISCV_IOMMU_PQCSR          0x50

typedef union {
  struct {
    UINT32  qen       : 1;
    UINT32  ie        : 1;
    UINT32  Reserved0 : 6;
    UINT32  qmf       : 1;
    UINT32  qof       : 1;
    UINT32  Reserved1 : 6;
    UINT32  qon       : 1;
    UINT32  busy      : 1;
    UINT32  Reserved2 : 10;
    UINT32  Custom    : 4;
  } Bits;
  UINT32  Uint32;
} RISCV_IOMMU_HARDWARE_REQUEST_QUEUE_CSR;

#define R_RISCV_IOMMU_IPSR           0x54

typedef union {
  struct {
    UINT32  cip       : 1;
    UINT32  fip       : 1;
    UINT32  pmip      : 1;
    UINT32  pip       : 1;
    UINT32  Reserved0 : 4;
    UINT32  Custom    : 8;
    UINT32  Reserved1 : 16;
  } Bits;
  UINT32  Uint32;
} RISCV_IOMMU_IPSR;

/* TODO: Performance monitoring registers */

#define R_RISCV_IOMMU_IOCNTOVF       0x58

#define R_RISCV_IOMMU_IOCNTINH       0x5c

#define R_RISCV_IOMMU_IOHPMCYCLES    0x60

#define R_RISCV_IOMMU_IOHPMCTR_1_31  0x68

#define R_RISCV_IOMMU_IOHPMEVT_1_31  0x160

#define R_RISCV_IOMMU_TR_REQ_IOVA    0x258

typedef union {
  struct {
    UINT64  Reserved : 12;
    UINT64  vpn      : 52;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_TR_REQ_IOVA;

#define R_RISCV_IOMMU_TR_REQ_CTL     0x260

typedef union {
  struct {
    UINT64  Go_Busy  : 1;
    UINT64  Priv     : 1;
    UINT64  Exe      : 1;
    UINT64  NW       : 1;
    UINT64  PID      : 20;
    UINT64  PV       : 1;
    UINT64  Reserved : 3;
    UINT64  Custom   : 4;
    UINT64  DID      : 24;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_TR_REQ_CTL;

#define R_RISCV_IOMMU_TR_RESPONSE    0x268

typedef union {
  struct {
    UINT64  fault     : 1;
    UINT64  Reserved0 : 6;
    UINT64  PBMT      : 2;
    UINT64  S         : 1;
    UINT64  PPN       : 44;
    UINT64  Reserved1 : 6;
    UINT64  Custom    : 4;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_TR_RESPONSE;

// TODO: IOMMU QoS ID

#define R_RISCV_IOMMU_QOSID          0x270

#define R_RISCV_IOMMU_RESERVED_1     0x274

#define R_RISCV_IOMMU_CUSTOM_2       0x2b0

#define R_RISCV_IOMMU_ICVEC          0x2f8

typedef union {
  struct {
    UINT64  civ      : 4;
    UINT64  fiv      : 4;
    UINT64  pmiv     : 4;
    UINT64  piv      : 4;
    UINT64  Reserved : 16;
    UINT64  Custom   : 32;
  } Bits;
  UINT64  Uint64;
} RISCV_IOMMU_ICVEC;

// TODO: MSI config table register

#define R_RISCV_IOMMU_MSI_CFG_TBL    0x300

#define R_RISCV_IOMMU_RESERVED_2     0x400

#endif
