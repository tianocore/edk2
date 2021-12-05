/** @file

  Work Area structure definition

  Copyright (c) 2021, AMD Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __OVMF_WORK_AREA_H__
#define __OVMF_WORK_AREA_H__

//
// Guest type for the work area
//
typedef enum {
  GUEST_TYPE_NON_ENCRYPTED,
  GUEST_TYPE_AMD_SEV,
  GUEST_TYPE_INTEL_TDX,
} GUEST_TYPE;

//
// Confidential computing work area header definition. Any change
// to the structure need to be kept in sync with the
// PcdOvmfConfidentialComputingWorkAreaHeader.
//
// PcdOvmfConfidentialComputingWorkAreaHeader ==
//   sizeof (CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER)
// PcdOvmfConfidentialComputingWorkAreaHeader defined in:
//   OvmfPkg/OvmfPkg.dec
//   OvmfPkg/OvmfPkgDefines.fdf.inc
typedef struct _CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER {
  UINT8    GuestType;
  UINT8    Reserved1[3];
} CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER;

//
// Internal structure for holding SEV-ES information needed during SEC phase
// and valid only during SEC phase and early PEI during platform
// initialization.
//
// This structure is also used by assembler files:
//   OvmfPkg/ResetVector/ResetVector.nasmb
//   OvmfPkg/ResetVector/Ia32/PageTables64.asm
//   OvmfPkg/ResetVector/Ia32/Flat32ToFlat64.asm
// any changes must stay in sync with its usage.
//
typedef struct _SEC_SEV_ES_WORK_AREA {
  UINT8     SevEsEnabled;
  UINT8     Reserved1[7];

  UINT64    RandomData;

  UINT64    EncryptionMask;
} SEC_SEV_ES_WORK_AREA;

//
// The SEV work area definition.
//
typedef struct _SEV_WORK_AREA {
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER    Header;

  SEC_SEV_ES_WORK_AREA                       SevEsWorkArea;
} SEV_WORK_AREA;

typedef union {
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER    Header;
  SEV_WORK_AREA                              SevWorkArea;
} OVMF_WORK_AREA;

#endif
