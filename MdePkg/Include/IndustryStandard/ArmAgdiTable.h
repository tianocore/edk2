/** @file
  Arm Global Diagnostic Dump and Reset Interface Table as defined in
  'ACPI for the Arm Components 1.2 EAC1' Platform Design Document

  Copyright (c) 2025 Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
    - ACPI for the Arm Components 1.2 EAC1 Platform Design Document,
      dated July 2025.
      (https://developer.arm.com/documentation/den0093/1-2eac1/)
**/

#ifndef ARM_AGDI_TABLE_H_
#define ARM_AGDI_TABLE_H_

#include <IndustryStandard/Acpi.h>

#define EFI_ACPI_ARM_AGDI_TABLE_SIGNATURE  SIGNATURE_32('A', 'G', 'D', 'I')

#define EFI_ACPI_ARM_AGDI_TABLE_REVISION  0

#pragma pack(1)

/// Signaling Mode Values
typedef enum {
  ArmAgdiSdeiSignalingMode             = 0x0,
  ArmAgdiInterruptSignalingMode        = 0x1,
  ArmAgdiSdeiAndInterruptSignalingMode = 0x2,
  ArmAgdiSignalingModeInval            = 0x3,
} ARM_AGDI_SIGNALING_MODE;

/// Arm AGDI Table definition
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER    Header;

  /// Signaling Mode bits [1:0]
  /// 0x0 - SDEI based signaling mode
  /// 0x1 - Interrupt based signaling mode
  /// 0x2 - Both SDEI and Interuppt based signaling mode
  /// Rest of the bits [7:2] are reserved
  UINT8                          Flags;

  /// Reserved - Must be zero
  UINT8                          Reserved[3];

  /// SDEI event number
  UINT32                         SdeiEventNum;

  /// GSIV of the interrupt
  UINT32                         Gsiv;
} EFI_ACPI_ARM_AGDI_TABLE;

#pragma pack()

#endif /* ARM_AGDI_TABLE_H_ */
