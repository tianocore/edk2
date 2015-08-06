/** @file
  Definitions for loading microcode on processors.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_MICROCODE_H_
#define _CPU_MICROCODE_H_

#define EFI_MSR_IA32_PLATFORM_ID         0x17
#define EFI_MSR_IA32_BIOS_UPDT_TRIG      0x79
#define EFI_MSR_IA32_BIOS_SIGN_ID        0x8b

#define MAX_MICROCODE_DESCRIPTOR_LENGTH  100

typedef struct {
  VOID     *MicrocodeData;
  UINTN    MicrocodeSize;
  UINT32   ProcessorId;
  BOOLEAN  Load;
} MICROCODE_INFO;

//
// Definition for IA32 microcode format
//
typedef struct {
  UINT32  HeaderVersion;
  UINT32  UpdateRevision;
  UINT32  Date;
  UINT32  ProcessorId;
  UINT32  Checksum;
  UINT32  LoaderRevision;
  UINT32  ProcessorFlags;
  UINT32  DataSize;
  UINT32  TotalSize;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_HEADER;

typedef struct {
  UINT32  ExtendedSignatureCount;
  UINT32  ExtendedTableChecksum;
  UINT8   Reserved[12];
} EFI_CPU_MICROCODE_EXTENDED_TABLE_HEADER;

typedef struct {
  UINT32  ProcessorSignature;
  UINT32  ProcessorFlag;
  UINT32  ProcessorChecksum;
} EFI_CPU_MICROCODE_EXTENDED_TABLE;

/**
  Detect whether specified processor can find matching microcode patch and load it.

**/
VOID
MicrocodeDetect (
  VOID
  );

#endif
