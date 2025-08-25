/** @file

  Work Area structure definition

  Copyright (c) 2021 - 2024, AMD Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __OVMF_WORK_AREA_H__
#define __OVMF_WORK_AREA_H__

#include <ConfidentialComputingGuestAttr.h>
#include <IndustryStandard/Tpm20.h>
#include <Library/BaseLib.h>

//
// Confidential computing work area header definition. Any change
// to the structure need to be kept in sync with the
// PcdOvmfConfidentialComputingWorkAreaHeader.
//
// PcdOvmfConfidentialComputingWorkAreaHeader ==
//   sizeof (CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER)
// PcdOvmfConfidentialComputingWorkAreaHeader defined in:
//   OvmfPkg/OvmfPkg.dec
//   OvmfPkg/Include/Fdf/OvmfPkgDefines.fdf.inc
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
  //
  // Hold the SevStatus MSR value read by OvmfPkg/ResetVector/Ia32/AmdSev.c
  //
  UINT64    SevStatusMsrValue;

  UINT64    RandomData;

  UINT64    EncryptionMask;

  // Flags:
  // - ReceivedVc: Indicator that the VC handler was called. It is used during
  //   the SevFeature detection in OvmfPkg/ResetVector/Ia32/AmdSev.asm
  // - CoherencySfwNo: Indicator that the SEV-SNP cache line evication
  //   mitigation is not needed.
  //
  UINT8     ReceivedVc     : 1;
  UINT8     CoherencySfwNo : 1;
  UINT8     Reserved1      : 6;

  UINT8     Reserved2[7];

  // Used by SEC to generate Page State Change requests. This should be
  // sized less than an equal to the GHCB shared buffer area to allow a
  // single call to the hypervisor.
  //
  UINT8     WorkBuffer[1024];
} SEC_SEV_ES_WORK_AREA;

//
// The SEV work area definition.
//
typedef struct _SEV_WORK_AREA {
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER    Header;

  SEC_SEV_ES_WORK_AREA                       SevEsWorkArea;
} SEV_WORK_AREA;

//
// Start of TDX Specific WorkArea definition
//

#define TDX_MEASUREMENT_TDHOB_BITMASK   0x1
#define TDX_MEASUREMENT_CFVIMG_BITMASK  0x2

typedef struct _TDX_MEASUREMENTS_DATA {
  UINT32    MeasurementsBitmap;
  UINT8     TdHobHashValue[SHA384_DIGEST_SIZE];
  UINT8     CfvImgHashValue[SHA384_DIGEST_SIZE];
} TDX_MEASUREMENTS_DATA;

#define MAILBOX_GDT_SIZE  (sizeof(IA32_SEGMENT_DESCRIPTOR) * 5)
typedef struct _MAILBOX_GDT {
  IA32_DESCRIPTOR    Gdtr;
  UINT8              Data[MAILBOX_GDT_SIZE];
} MAILBOX_GDT;
//
// The TDX work area definition
//
typedef struct _SEC_TDX_WORK_AREA {
  UINT32                   PageTableReady;
  UINT32                   Gpaw;
  UINT64                   HobList;
  TDX_MEASUREMENTS_DATA    TdxMeasurementsData;
} SEC_TDX_WORK_AREA;

typedef struct _TDX_WORK_AREA {
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER    Header;
  SEC_TDX_WORK_AREA                          SecTdxWorkArea;
  MAILBOX_GDT                                MailboxGdt;
} TDX_WORK_AREA;

//
// End of TDX Specific WorkArea definition
//

typedef union {
  CONFIDENTIAL_COMPUTING_WORK_AREA_HEADER    Header;
  SEV_WORK_AREA                              SevWorkArea;
  TDX_WORK_AREA                              TdxWorkArea;
} OVMF_WORK_AREA;

#endif
