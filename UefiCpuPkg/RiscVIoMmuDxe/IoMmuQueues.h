/** @file
  RISC-V IOMMU structures.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RISC_V_IO_MMU_QUEUES_
#define _RISC_V_IO_MMU_QUEUES_

#include <PiDxe.h>

#pragma pack (push, 1)

//
// Command queue
// - Values 5-63 are reserved, 64-127 are custom
//
#define OP_RISCV_IOMMU_IOTINVAL  1

#define FUNC_RISCV_IOMMU_IOTINVAL_VMA   0
#define FUNC_RISCV_IOMMU_IOTINVAL_GVMA  1

typedef struct {
  UINT64    opcode    : 7;
  UINT64    func3     : 3;
  UINT64    AV        : 1;
  UINT64    Reserved0 : 1;
  UINT64    PSCID     : 20;
  UINT64    PSCV      : 1;
  UINT64    GV        : 1;
  UINT64    NL        : 1;
  UINT64    Reserved1 : 9;
  UINT64    GSCID     : 16;
  UINT64    Reserved2 : 13;
  UINT64    S         : 1;
  UINT64    ADDR      : 52;
} RISCV_IOMMU_IOTINVAL;

#define OP_RISCV_IOMMU_IOFENCE  2

#define FUNC_RISCV_IOMMU_IOFENCE_C  0

typedef struct {
  UINT64    opcode    : 7;
  UINT64    func3     : 3;
  UINT64    AV        : 1;
  UINT64    WSI       : 1;
  UINT64    PR        : 1;
  UINT64    PW        : 1;
  UINT64    Reserved0 : 18;
  UINT64    DATA      : 32;
  UINT64    ADDR      : 62;
  UINT64    Reserved1 : 2;
} RISCV_IOMMU_IOFENCE;

#define OP_RISCV_IOMMU_IODIR  3

#define FUNC_RISCV_IOMMU_IODIR_INVAL_DDT  0
#define FUNC_RISCV_IOMMU_IODIR_INVAL_PDT  1

typedef struct {
  UINT64    opcode    : 7;
  UINT64    func3     : 3;
  UINT64    Reserved0 : 2;
  UINT64    PID       : 20;
  UINT64    Reserved1 : 1;
  UINT64    DV        : 1;
  UINT64    Reserved2 : 6;
  UINT64    DID       : 24;
  UINT64    Reserved3 : 64;
} RISCV_IOMMU_IODIR;

#define OP_RISCV_IOMMU_ATS  4

#define FUNC_RISCV_IOMMU_ATS_INVAL  0
#define FUNC_RISCV_IOMMU_ATS_PRGR   1

// NB: As PRI will not be enabled, the PRGR command is not required.

typedef struct {
  UINT64    opcode    : 7;
  UINT64    func3     : 3;
  UINT64    Reserved0 : 2;
  UINT64    PID       : 20;
  UINT64    PV        : 1;
  UINT64    DSV       : 1;
  UINT64    Reserved1 : 6;
  UINT64    RID       : 16;
  UINT64    DSEG      : 8;
  union {
    struct {
      UINT64    G       : 1;
      UINT64    Zeros   : 10;
      UINT64    S       : 1;
      UINT64    Address : 52;
    } Bits;
    UINT64    Uint64;
  } PAYLOAD;
} RISCV_IOMMU_ATS_INVAL;

//
// Fault queue
//
#define RISCV_IOMMU_FAULT_CAUSE_INSTR_ACCESS_FAULT     1
#define RISCV_IOMMU_FAULT_CAUSE_READ_ADDR_MISALIGNED   4
#define RISCV_IOMMU_FAULT_CAUSE_READ_ACCESS_FAULT      5
#define RISCV_IOMMU_FAULT_CAUSE_WRITE_ADDR_MISALIGNED  6
#define RISCV_IOMMU_FAULT_CAUSE_WRITE_ACCESS_FAULT     7
#define RISCV_IOMMU_FAULT_CAUSE_INSTR_PAGE_FAULT_1     12
#define RISCV_IOMMU_FAULT_CAUSE_READ_PAGE_FAULT_1      13
#define RISCV_IOMMU_FAULT_CAUSE_WRITE_PAGE_FAULT_1     15
#define RISCV_IOMMU_FAULT_CAUSE_INSTR_PAGE_FAULT_2     20
#define RISCV_IOMMU_FAULT_CAUSE_READ_PAGE_FAULT_2      21
#define RISCV_IOMMU_FAULT_CAUSE_WRITE_PAGE_FAULT_2     23
#define RISCV_IOMMU_FAULT_CAUSE_DMA_DISABLED           256
#define RISCV_IOMMU_FAULT_CAUSE_DDT_LOAD_FAULT         257
#define RISCV_IOMMU_FAULT_CAUSE_DDT_INVALID            258
#define RISCV_IOMMU_FAULT_CAUSE_DDT_MISCONFIGURED      259
#define RISCV_IOMMU_FAULT_CAUSE_TTYP_DISALLOWED        260
#define RISCV_IOMMU_FAULT_CAUSE_MSI_LOAD_FAULT         261
#define RISCV_IOMMU_FAULT_CAUSE_MSI_INVALID            262
#define RISCV_IOMMU_FAULT_CAUSE_MSI_MISCONFIGURED      263
#define RISCV_IOMMU_FAULT_CAUSE_MRIF_ACCESS_FAULT      264
#define RISCV_IOMMU_FAULT_CAUSE_PDT_LOAD_FAULT         265
#define RISCV_IOMMU_FAULT_CAUSE_PDT_INVALID            266
#define RISCV_IOMMU_FAULT_CAUSE_PDT_MISCONFIGURED      267
#define RISCV_IOMMU_FAULT_CAUSE_DDT_CORRUPTED          268
#define RISCV_IOMMU_FAULT_CAUSE_PDT_CORRUPTED          269
#define RISCV_IOMMU_FAULT_CAUSE_MSI_PT_CORRUPTED       270
#define RISCV_IOMMU_FAULT_CAUSE_MRIF_CORRUPTED         271
#define RISCV_IOMMU_FAULT_CAUSE_INTERNAL_PATH_ERROR    272
#define RISCV_IOMMU_FAULT_CAUSE_MSI_WRITE_FAULT        273
#define RISCV_IOMMU_FAULT_CAUSE_PT_CORRUPTED           274

#define RISCV_IOMMU_FAULT_TTYP_NONE                 0
#define RISCV_IOMMU_FAULT_TTYP_UNTRANS_INSTR_FETCH  1
#define RISCV_IOMMU_FAULT_TTYP_UNTRANS_READ         2
#define RISCV_IOMMU_FAULT_TTYP_UNTRANS_WRITE        3
#define RISCV_IOMMU_FAULT_TTYP_TRANS_INSTR_FETCH    5
#define RISCV_IOMMU_FAULT_TTYP_TRANS_READ           6
#define RISCV_IOMMU_FAULT_TTYP_TRANS_WRITE          7
#define RISCV_IOMMU_FAULT_TTYP_PCIE_ATS_TRANS_REQ   8
#define RISCV_IOMMU_FAULT_TTYP_PCIE_MSG_REQ         9

typedef struct {
  UINT64    CAUSE    : 12;
  UINT64    PID      : 20;
  UINT64    PV       : 1;
  UINT64    PRIV     : 1;
  UINT64    TTYP     : 6;
  UINT64    DID      : 24;
  UINT64    Custom   : 32;
  UINT64    Reserved : 32;
  UINT64    iotval   : 64;
  UINT64    iotval2  : 64;
} RISCV_IOMMU_FAULT_RECORD;

#pragma pack (pop)

#endif
