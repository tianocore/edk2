/** @file
  MSR Definitions.

  Provides defines for Machine Specific Registers(MSR) indexes. Data structures
  are provided for MSRs that contain one or more bit fields.  If the MSR value
  returned is a single 32-bit or 64-bit value, then a data structure is not
  provided for that MSR.

  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Specification Reference:
  AMD64 Architecture Programming Manaul volume 2, March 2017, Sections 15.34

**/

#ifndef __FAM17_MSR_H__
#define __FAM17_MSR_H__

/**
  Secure Encrypted Virtualization (SEV) status register

**/
#define MSR_SEV_STATUS                     0xc0010131

/**
  MSR information returned for #MSR_SEV_STATUS
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Secure Encrypted Virtualization (Sev) is enabled
    ///
    UINT32  SevBit:1;

    ///
    /// [Bit 1] Secure Encrypted Virtualization Encrypted State (SevEs) is enabled
    ///
    UINT32  SevEsBit:1;

    UINT32  Reserved:30;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_SEV_STATUS_REGISTER;

#endif
