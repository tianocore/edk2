/** @file
  TPM NVS MM guid, used for exchanging information, including SWI value and NVS region
  information, for patching TPM ACPI table.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TCG2_NVS_MM_H_
#define TCG2_NVS_MM_H_

#define MM_TPM_NVS_HOB_GUID \
  { 0xc96c76eb, 0xbc78, 0x429c, { 0x9f, 0x4b, 0xda, 0x51, 0x78, 0xc2, 0x84, 0x57 }}

extern EFI_GUID gTpmNvsMmGuid;

#pragma pack(1)
typedef struct {
  UINT8                  SoftwareSmi;
  UINT32                 Parameter;
  UINT32                 Response;
  UINT32                 Request;
  UINT32                 RequestParameter;
  UINT32                 LastRequest;
  UINT32                 ReturnCode;
} PHYSICAL_PRESENCE_NVS;

typedef struct {
  UINT8                  SoftwareSmi;
  UINT32                 Parameter;
  UINT32                 Request;
  UINT32                 ReturnCode;
} MEMORY_CLEAR_NVS;

typedef struct {
  PHYSICAL_PRESENCE_NVS  PhysicalPresence;
  MEMORY_CLEAR_NVS       MemoryClear;
  UINT32                 PPRequestUserConfirm;
  UINT32                 TpmIrqNum;
  BOOLEAN                IsShortFormPkgLength;
} TCG_NVS;

typedef struct {
  UINT8                  OpRegionOp;
  UINT32                 NameString;
  UINT8                  RegionSpace;
  UINT8                  DWordPrefix;
  UINT32                 RegionOffset;
  UINT8                  BytePrefix;
  UINT8                  RegionLen;
} AML_OP_REGION_32_8;

typedef struct {
  UINT64                Function;
  UINT64                ReturnStatus;
  EFI_PHYSICAL_ADDRESS  TargetAddress;
  UINT64                RegisteredPpSwiValue;
  UINT64                RegisteredMcSwiValue;
} TPM_NVS_MM_COMM_BUFFER;
#pragma pack()

typedef enum {
  TpmNvsMmExchangeInfo,
} TPM_NVS_MM_FUNCTION;

#endif  // TCG2_NVS_MM_H_
