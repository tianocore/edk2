/** @file

  Copyright (c) 2016-2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef MM_COMMUNICATE_H_
#define MM_COMMUNICATE_H_

#define MM_MAJOR_VER_MASK   0xEFFF0000
#define MM_MINOR_VER_MASK   0x0000FFFF
#define MM_MAJOR_VER_SHIFT  16

#define MM_MAJOR_VER(x)  (((x) & MM_MAJOR_VER_MASK) >> MM_MAJOR_VER_SHIFT)
#define MM_MINOR_VER(x)  ((x) & MM_MINOR_VER_MASK)

#define MM_CALLER_MAJOR_VER  0x1UL
#define MM_CALLER_MINOR_VER  0x0

#define MPXY_SHMEM_SIZE  4096

// SMM Message ID
#define RISCV_MSG_ID_SMM_ENABLE_NOTIFICATION  0x1
#define RISCV_MSG_ID_SMM_GET_ATTRIBUTES       0x2
#define RISCV_MSG_ID_SMM_COMMUNICATE          0x3

// SMM return error codes
#define RISCV_SMM_RET_SUCCESS         0
#define RISCV_SMM_RET_NOT_SUPPORTED   -1
#define RISCV_SMM_RET_INVALID_PARAMS  -2
#define RISCV_SMM_RET_DENIED          -3
#define RISCV_SMM_RET_NO_MEMORY       -4

typedef struct {
  UINT32    IpDataOffset;
  UINT32    IpDataSize;
  UINT32    OpDataOffset;
  UINT32    OpDataSize;
} RISCV_SMM_MSG_COMM_ARGS;

typedef struct {
  UINT32    Status;
  UINT32    RetDataSize;
} RISCV_SMM_MSG_COMM_RESP;

typedef PACKED struct {
  INT32     Status;
  UINT32    mmVersion;
  UINT32    mmShmemAddrLow;
  UINT32    mmShmemaddrHigh;
  UINT32    mmShmemSize;
} MM_GET_ATTRIBUTES;

typedef struct {
  EFI_PHYSICAL_ADDRESS    PhysicalBase;
  EFI_VIRTUAL_ADDRESS     VirtualBase;
  UINT64                  Length;
} RISCV_SMM_MEM_REGION_DESCRIPTOR;

#endif /* MM_COMMUNICATE_H_ */
