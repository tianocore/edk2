/** @file
  Entry point to the Standalone MM Foundation on RiscV platform.

Copyright (c) 2025, Rivos Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STANDALONEMMCORE_ENTRY_POINT_H__
#define __STANDALONEMMCORE_ENTRY_POINT_H__

#include <Uefi.h>
#include <Library/PeCoffLib.h>
#include <Library/FvLib.h>

typedef struct {
  UINT32    ProcessorId;
  UINT32    Package;
  UINT32    Core;
  UINT32    Flags;
} EFI_RISCV_SMM_CPU_INFO;

typedef struct {
  UINT64                    MmMemBase;
  UINT64                    MmMemLimit;
  UINT64                    MmImageBase;
  UINT64                    MmStackBase;
  UINT64                    MmHeapBase;
  UINT64                    MmNsCommBufBase;
  UINT64                    MmImageSize;
  UINT64                    MmPcpuStackSize;
  UINT64                    MmHeapSize;
  UINT64                    MmNsCommBufSize;
  UINT32                    NumMmMemRegions;
  UINT32                    NumCpus;
  UINT32                    MpxyChannelId;
  EFI_RISCV_SMM_CPU_INFO    CpuInfo;
} EFI_RISCV_SMM_PAYLOAD_INFO;

#define BOOT_INFO_STACK_BASE_OFFSET  24   // Used in assembly
#define CPU_INFO_FLAG_PRIMARY_CPU    0x00000001
STATIC_ASSERT (BOOT_INFO_STACK_BASE_OFFSET == OFFSET_OF (EFI_RISCV_SMM_PAYLOAD_INFO, MmStackBase));

// ReqFwd Services
typedef enum {
  REQFWD_ENABLE_NOTIFICATION      = 0x1,
  REQFWD_RETRIEVE_CURRENT_MESSAGE = 0x2,
  REQFWD_COMPLETE_CURRENT_MESSAGE = 0x3
} ReqFwdService;

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

// SMM return error codes
#define RISCV_SMM_RET_SUCCESS         0
#define RISCV_SMM_RET_NOT_SUPPORTED   -1
#define RISCV_SMM_RET_INVALID_PARAMS  -2
#define RISCV_SMM_RET_DENIED          -3
#define RISCV_SMM_RET_NO_MEMORY       -4

#pragma pack(1) // Ensure no padding between fields
// RPMI message header structure
typedef struct {
  UINT16    ServiceGroupId;      // SERVICEGROUP_ID (16 bits, bits 15:0)
  UINT8     ServiceId;           // SERVICE_ID (8 bits, bits 23:16)
  UINT8     Flags;               // FLAGS (8 bits, bits 31:24)
  UINT16    Token;               // TOKEN (16 bits, bits 31:16 of the second word): Not used in MM
  UINT16    DataLen;             // DATALEN (16 bits, bits 15:0 of the second word, length of data in bytes, 4-byte multiple)
} RPMI_MESSAGE_HEADER;

typedef struct {
  RPMI_MESSAGE_HEADER    hdr;
  UINT32                 StartIndex;
} REQFWD_RETRIEVE_CMD;

typedef struct {
  UINT32    Offset;
} MM_PAYLOAD;

typedef struct {
  UINT32    Status;
  UINT32    Remaining;
  UINT32    Returned;
} REQFWD_RETRIEVE_RESP;

typedef struct {
  RPMI_MESSAGE_HEADER     hdr;
  REQFWD_RETRIEVE_RESP    reqfwd_resp;
} RPMI_REQFWD_RETRIEVE_RESP;

typedef struct {
  RPMI_MESSAGE_HEADER        hdr;
  RISCV_SMM_MSG_COMM_RESP    mm_resp;
} RPMI_SMM_MSG_CMPL_CMD;

typedef struct {
  RPMI_MESSAGE_HEADER    hdr;
  UINT32                 Status;
} RPMI_SMM_MSG_CMPL_RESP;

typedef struct {
  RPMI_REQFWD_RETRIEVE_RESP    rpmi_resp;
  RISCV_SMM_MSG_COMM_ARGS      mm_data;
} RPMI_SMM_MSG_COMM_ARGS;

#pragma pack() // End of packed structure

VOID *
EFIAPI
CreateHobListFromBootInfo (
  IN       EFI_RISCV_SMM_PAYLOAD_INFO  *PayloadBootInfo
  );

/**
  The entry point of Standalone MM Foundation.

  @param  [in]  CpuId             The Id assigned to this running CPU
  @param  [in]  PayloadInfoAddress   The address of payload info

**/
VOID
EFIAPI
CModuleEntryPoint (
  IN UINT64  CpuId,
  IN VOID    *PayloadInfoAddress
  );

/**

  @param  HobStart  Pointer to the beginning of the HOB List passed in from the PEI Phase.

**/
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  );

#endif
