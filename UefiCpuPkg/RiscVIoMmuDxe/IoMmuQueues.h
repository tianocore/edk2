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
  UINT64  opcode    : 7;
  UINT64  func3     : 3;
  UINT64  AV        : 1;
  UINT64  Reserved0 : 1;
  UINT64  PSCID     : 20;
  UINT64  PSCV      : 1;
  UINT64  GV        : 1;
  UINT64  NL        : 1;
  UINT64  Reserved1 : 9;
  UINT64  GSCID     : 16;
  UINT64  Reserved2 : 13;
  UINT64  S         : 1;
  UINT64  ADDR      : 52;
} RISCV_IOMMU_IOTINVAL;

#define OP_RISCV_IOMMU_IOFENCE   2

#define FUNC_RISCV_IOMMU_IOFENCE_C  0

typedef struct {
  UINT64  opcode    : 7;
  UINT64  func3     : 3;
  UINT64  AV        : 1;
  UINT64  WSI       : 1;
  UINT64  PR        : 1;
  UINT64  PW        : 1;
  UINT64  Reserved0 : 18;
  UINT64  DATA      : 32;
  UINT64  ADDR      : 62;
  UINT64  Reserved1 : 2;
} RISCV_IOMMU_IOFENCE;

#define OP_RISCV_IOMMU_IODIR     3

#define FUNC_RISCV_IOMMU_IODIR_INVAL_DDT  0
#define FUNC_RISCV_IOMMU_IODIR_INVAL_PDT  1

typedef struct {
  UINT64  opcode    : 7;
  UINT64  func3     : 3;
  UINT64  Reserved0 : 2;
  UINT64  PID       : 20;
  UINT64  Reserved1 : 1;
  UINT64  DV        : 1;
  UINT64  Reserved2 : 6;
  UINT64  DID       : 24;
  UINT64  Reserved3 : 64;
} RISCV_IOMMU_IODIR;

#define OP_RISCV_IOMMU_ATS       4

#define FUNC_RISCV_IOMMU_ATS_INVAL  0
#define FUNC_RISCV_IOMMU_ATS_PRGR   1

// TODO: ATS command struct

typedef struct {
  UINT64  opcode : 7;
  UINT64  func3  : 3;
} RISCV_IOMMU_ATS;

//
// Fault queue
// - TODO: Create definitions
//
typedef struct {
  UINT64  CAUSE    : 12;
  UINT64  PID      : 20;
  UINT64  PV       : 1;
  UINT64  PRIV     : 1;
  UINT64  TTYP     : 6;
  UINT64  DID      : 24;
  UINT64  Custom   : 32;
  UINT64  Reserved : 32;
  UINT64  iotval   : 64;
  UINT64  iotval2  : 64;
} RISCV_IOMMU_FAULT_RECORD;

#pragma pack (pop)

#endif
