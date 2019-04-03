/** @file

Processor power management initialization code.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#ifndef _PPM_H
#define _PPM_H

//
// Bit definitions of PPMFlags
//
#define PPM_GV3         (1 << 0)  // Geyserville 3
#define PPM_TURBO       (1 << 1)  // Turbo Mode
#define PPM_SUPER_LFM   (1 << 2)  // N/2 Ratio
#define PPM_C1          (1 << 4)  // C1 Capable, Enabled
#define PPM_C2          (1 << 5)  // C2 Capable, Enabled
#define PPM_C3          (1 << 6)  // C3 Capable, Enabled
#define PPM_C4          (1 << 7)  // C4 Capable, Enabled
#define PPM_C5          (1 << 8)  // C5/Deep C4 Capable, Enabled
#define PPM_C6          (1 << 9)  // C6 Capable, Enabled
#define PPM_C1E         (1 << 10) // C1E Enabled
#define PPM_C2E         (1 << 11) // C2E Enabled
#define PPM_C3E         (1 << 12) // C3E Enabled
#define PPM_C4E         (1 << 13) // C4E Enabled
#define PPM_HARD_C4E    (1 << 14) // Hard C4E Capable, Enabled
#define PPM_TM1         (1 << 16) // Thermal Monitor 1
#define PPM_TM2         (1 << 17) // Thermal Monitor 2
#define PPM_PHOT        (1 << 19) // Bi-directional ProcHot
#define PPM_MWAIT_EXT   (1 << 21) // MWAIT extensions supported
#define PPM_CMP         (1 << 24) // CMP supported, Enabled
#define PPM_TSTATE      (1 << 28) // CPU T states supported

#define PPM_C_STATES    (PPM_C1 + PPM_C2 + PPM_C3 + PPM_C4 + PPM_C5 + PPM_C6)
#define PPM_CE_STATES   (PPM_C1E + PPM_C2E + PPM_C3E + PPM_C4E + PPM_HARD_C4E)


#define MAX_P_STATES_NUM    12

#define AML_NAME_OP         0x08
#define AML_SCOPE_OP        0x10
#define AML_PACKAGE_OP      0x12
#define AML_METHOD_OP       0x14

#define S3_CPU_REGISTER_TABLE_GUID \
  { \
    0xc4ef988d, 0xe5e, 0x4403, { 0xbe, 0xeb, 0xf1, 0xbb, 0x6, 0x79, 0x6e, 0xdf } \
  }

#pragma pack(1)
typedef struct {
  UINT8   StartByte;
  UINT32  NameStr;
  UINT8   OpCode;
  UINT16  Size;                // Hardcode to 16bit width because the table we use is fixed size
  UINT8   NumEntries;
} EFI_ACPI_NAME_COMMAND;

typedef struct {
  UINT8   PackageOp;
  UINT8   PkgLeadByte;
  UINT8   NumEntries;
  UINT8   DwordPrefix0;
  UINT32  CoreFreq;
  UINT8   DwordPrefix1;
  UINT32  Power;
  UINT8   DwordPrefix2;
  UINT32  TransLatency;
  UINT8   DwordPrefix3;
  UINT32  BMLatency;
  UINT8   DwordPrefix4;
  UINT32  Control;
  UINT8   DwordPrefix5;
  UINT32  Status;
} EFI_PSS_PACKAGE;
#pragma pack()

typedef struct {
  UINT32  Index;
  UINT64  Value;
} S3_CPU_REGISTER;

//
// Function prototypes
//

/**
  This function is the entry of processor power management initialization code.
  It initializes the processor's power management features based on the user
  configurations and hardware capablities.
**/
VOID
PpmInit (
  VOID
  );

/**
  This function is to determine the Processor Power Management Flags
  based on the hardware capability.
**/
VOID
PpmDetectCapability (
  VOID
  );

/**
  This function is to determine the user configuration mask
**/
VOID
PpmGetUserConfigurationMask (
  VOID
  );

/**
  This function is to patch and publish power management related acpi tables.
**/
VOID
PpmPatchAndPublishAcpiTables (
  VOID
  );

/**
  This function is to patch PLvl2Lat and PLvl3Lat to enable C2, C3 support in OS.
**/
VOID
PpmPatchFadtTable (
  VOID
  );

/**
  This function is to load all the power management acpi tables and patch IST table.
**/
VOID
PpmLoadAndPatchPMTables (
  VOID
  );

/**
  This function is to save cpu registers for s3 resume.
**/
VOID
PpmS3SaveRegisters (
  VOID
  );
#endif
