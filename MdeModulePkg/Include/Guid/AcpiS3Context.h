/** @file
  Definitions for data structures used in S3 resume.

Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ACPI_S3_DATA_H_
#define _ACPI_S3_DATA_H_

#include <Library/BaseLib.h>

#define SMM_S3_RESUME_SMM_32  SIGNATURE_64 ('S','M','M','S','3','_','3','2')
#define SMM_S3_RESUME_SMM_64  SIGNATURE_64 ('S','M','M','S','3','_','6','4')

#pragma pack(1)

typedef struct {
  UINT64                  Signature;
  EFI_PHYSICAL_ADDRESS    SmmS3ResumeEntryPoint;
  EFI_PHYSICAL_ADDRESS    SmmS3StackBase;
  UINT64                  SmmS3StackSize;
  UINT64                  SmmS3Cr0;
  UINT64                  SmmS3Cr3;
  UINT64                  SmmS3Cr4;
  UINT16                  ReturnCs;
  EFI_PHYSICAL_ADDRESS    ReturnEntryPoint;
  EFI_PHYSICAL_ADDRESS    ReturnContext1;
  EFI_PHYSICAL_ADDRESS    ReturnContext2;
  EFI_PHYSICAL_ADDRESS    ReturnStackPointer;
  EFI_PHYSICAL_ADDRESS    Smst;
} SMM_S3_RESUME_STATE;

typedef struct {
  EFI_PHYSICAL_ADDRESS    AcpiFacsTable;
  EFI_PHYSICAL_ADDRESS    IdtrProfile;
  EFI_PHYSICAL_ADDRESS    S3NvsPageTableAddress;
  EFI_PHYSICAL_ADDRESS    BootScriptStackBase;
  UINT64                  BootScriptStackSize;
  EFI_PHYSICAL_ADDRESS    S3DebugBufferAddress;
} ACPI_S3_CONTEXT;

typedef struct {
  UINT16                  ReturnCs;
  UINT64                  ReturnStatus;
  EFI_PHYSICAL_ADDRESS    ReturnEntryPoint;
  EFI_PHYSICAL_ADDRESS    ReturnStackPointer;
  EFI_PHYSICAL_ADDRESS    AsmTransferControl;
  IA32_DESCRIPTOR         Idtr;
} PEI_S3_RESUME_STATE;

#pragma pack()

#define EFI_ACPI_S3_CONTEXT_GUID \
  { \
    0xef98d3a, 0x3e33, 0x497a, {0xa4, 0x1, 0x77, 0xbe, 0x3e, 0xb7, 0x4f, 0x38} \
  }

extern EFI_GUID  gEfiAcpiS3ContextGuid;

extern EFI_GUID  gEfiAcpiVariableGuid;

#endif
