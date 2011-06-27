/** @file
Provides the data format and Name and GUID definition for the EFI Variable ACPI_VARIABLE_SET and the Framework version's correponding 
EFI Variable.
This variable is the persistent data that will be saved to NV EFI Variable. It is the central repository
for configuration data saved by the CPU Driver, Platform SMM Handler. It is read by 
Platform PEIM and PEIM on S3 boot path to restore the system configuration.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACPI_VARIABLE_H_
#define _ACPI_VARIABLE_H_

#define EFI_ACPI_VARIABLE_GUID \
  { \
    0xaf9ffd67, 0xec10, 0x488a, {0x9d, 0xfc, 0x6c, 0xbf, 0x5e, 0xe2, 0x2c, 0x2e } \
  }

#define ACPI_GLOBAL_VARIABLE  L"AcpiGlobalVariable"

#define EFI_ACPI_VARIABLE_COMPATIBILITY_GUID \
  { \
    0xc020489e, 0x6db2, 0x4ef2, {0x9a, 0xa5, 0xca, 0x6, 0xfc, 0x11, 0xd3, 0x6a } \
  }
//
// The following structure boosts performance by combining all ACPI related variables into one.
//
#pragma pack(1)
typedef struct {
  UINT16  Limit;
  UINTN   Base;
} PSEUDO_DESCRIPTOR;
#pragma pack()

typedef struct {
  BOOLEAN               APState;
  BOOLEAN               S3BootPath;
  EFI_PHYSICAL_ADDRESS  WakeUpBuffer;
  EFI_PHYSICAL_ADDRESS  GdtrProfile;
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  EFI_PHYSICAL_ADDRESS  CpuPrivateData;
  EFI_PHYSICAL_ADDRESS  StackAddress;
  EFI_PHYSICAL_ADDRESS  MicrocodePointerBuffer;
  EFI_PHYSICAL_ADDRESS  SmramBase;
  EFI_PHYSICAL_ADDRESS  SmmStartImageBase;
  UINT32                SmmStartImageSize;
  UINT32                NumberOfCpus;
} ACPI_CPU_DATA_COMPATIBILITY;

typedef struct {
  //
  // Acpi Related variables
  //
  EFI_PHYSICAL_ADDRESS  AcpiReservedMemoryBase;
  UINT32                AcpiReservedMemorySize;
  EFI_PHYSICAL_ADDRESS  S3ReservedLowMemoryBase;
  EFI_PHYSICAL_ADDRESS  AcpiBootScriptTable;
  EFI_PHYSICAL_ADDRESS  RuntimeScriptTableBase;
  EFI_PHYSICAL_ADDRESS  AcpiFacsTable;
  UINT64                SystemMemoryLength;
  ACPI_CPU_DATA_COMPATIBILITY         AcpiCpuData;
  //
  // VGA OPROM to support Video Re-POST for Linux S3
  //
  EFI_PHYSICAL_ADDRESS  VideoOpromAddress;
  UINT32                VideoOpromSize;

  //
  // S3 Debug extension
  //
  EFI_PHYSICAL_ADDRESS  S3DebugBufferAddress; 
  EFI_PHYSICAL_ADDRESS  S3ResumeNvsEntryPoint;    
} ACPI_VARIABLE_SET_COMPATIBILITY;

typedef struct {
  BOOLEAN               APState;
  EFI_PHYSICAL_ADDRESS  StartupVector;
  EFI_PHYSICAL_ADDRESS  GdtrProfile;
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  EFI_PHYSICAL_ADDRESS  StackAddress;
  UINT32                StackSize;
  UINT32                NumberOfCpus;
  EFI_PHYSICAL_ADDRESS  MtrrTable;
  EFI_PHYSICAL_ADDRESS  RegisterTable;
  EFI_PHYSICAL_ADDRESS  Microcode;
} ACPI_CPU_DATA;

typedef struct {
  //
  // Acpi Related variables
  //
  EFI_PHYSICAL_ADDRESS  AcpiReservedMemoryBase;
  UINT32                AcpiReservedMemorySize;
  EFI_PHYSICAL_ADDRESS  S3ReservedLowMemoryBase;
  EFI_PHYSICAL_ADDRESS  AcpiFacsTable;
  UINT64                SystemMemoryLength;
  ACPI_CPU_DATA         AcpiCpuData;

  //
  // VGA OPROM to support Video Re-POST for Linux S3
  //
  EFI_PHYSICAL_ADDRESS  VideoOpromAddress;
  UINT32                VideoOpromSize;  
  EFI_PHYSICAL_ADDRESS  S3PostVideoStatusRestoreEntryPoint;

  EFI_PHYSICAL_ADDRESS  S3NvsPageTableAddress;
  EFI_PHYSICAL_ADDRESS  S3DebugBufferAddress;  
} ACPI_VARIABLE_SET;

#define SMM_S3_RESUME_SMM_32 SIGNATURE_64 ('S','M','M','S','3','_','3','2')
#define SMM_S3_RESUME_SMM_64 SIGNATURE_64 ('S','M','M','S','3','_','6','4')

typedef struct {
  UINT64                Signature;
  EFI_PHYSICAL_ADDRESS  SmmS3ResumeEntryPoint;
  EFI_PHYSICAL_ADDRESS  SmmS3StackBase;
  UINT64                SmmS3StackSize;
  UINT64                SmmS3Cr0;
  UINT64                SmmS3Cr3;
  UINT64                SmmS3Cr4;
  UINT16                ReturnCs;
  EFI_PHYSICAL_ADDRESS  ReturnEntryPoint;
  EFI_PHYSICAL_ADDRESS  ReturnContext1;
  EFI_PHYSICAL_ADDRESS  ReturnContext2;
  EFI_PHYSICAL_ADDRESS  ReturnStackPointer;
  EFI_PHYSICAL_ADDRESS  Smst;
} SMM_S3_RESUME_STATE;


typedef struct {
  EFI_PHYSICAL_ADDRESS  AcpiFacsTable;
  EFI_PHYSICAL_ADDRESS  IdtrProfile;
  EFI_PHYSICAL_ADDRESS  S3NvsPageTableAddress;
  EFI_PHYSICAL_ADDRESS  BootScriptStackBase;
  UINT64                BootScriptStackSize;
  EFI_PHYSICAL_ADDRESS  S3DebugBufferAddress;  
} ACPI_S3_CONTEXT;

typedef struct {
  UINT16                ReturnCs;
  EFI_PHYSICAL_ADDRESS  ReturnEntryPoint;
  EFI_PHYSICAL_ADDRESS  ReturnStackPointer;
  EFI_PHYSICAL_ADDRESS  AsmTransferControl;
  PSEUDO_DESCRIPTOR     Idtr;
} PEI_S3_RESUME_STATE;

#define EFI_ACPI_S3_CONTEXT_GUID \
  { \
    0xef98d3a, 0x3e33, 0x497a, {0xa4, 0x1, 0x77, 0xbe, 0x3e, 0xb7, 0x4f, 0x38} \
  }

extern EFI_GUID gEfiAcpiS3ContextGuid;

extern EFI_GUID gEfiAcpiVariableGuid;
extern EFI_GUID gEfiAcpiVariableCompatiblityGuid;

#endif
