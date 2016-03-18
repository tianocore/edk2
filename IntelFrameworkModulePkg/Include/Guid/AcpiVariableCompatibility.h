/** @file
  Definitions for data structures used in S3 resume.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ACPI_VARIABLE_COMPATIBILITY_H_
#define _ACPI_VARIABLE_COMPATIBILITY_H_

#define EFI_ACPI_VARIABLE_COMPATIBILITY_GUID \
  { \
    0xc020489e, 0x6db2, 0x4ef2, {0x9a, 0xa5, 0xca, 0x6, 0xfc, 0x11, 0xd3, 0x6a } \
  }

#define ACPI_GLOBAL_VARIABLE  L"AcpiGlobalVariable"

extern EFI_GUID gEfiAcpiVariableCompatiblityGuid;

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

#endif
