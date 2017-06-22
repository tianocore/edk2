/** @file
  CPUID leaf definitions.

  Provides defines for CPUID leaf indexes.  Data structures are provided for
  registers returned by a CPUID leaf that contain one or more bit fields.
  If a register returned is a single 32-bit value, then a data structure is
  not provided for that register.

  Copyright (c) 2017, Advanced Micro Devices. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Specification Reference:
  AMD64 Architecture Programming Manaul volume 2, March 2017, Sections 15.34

**/

#ifndef __AMD_CPUID_H__
#define __AMD_CPUID_H__

/**

  Memory Encryption Information

  @param   EAX  CPUID_MEMORY_ENCRYPTION_INFO (0x8000001F)

  @retval  EAX  Returns the memory encryption feature support status.
  @retval  EBX  If memory encryption feature is present then return
                the page table bit number used to enable memory encryption support
                and reducing of physical address space in bits.
  @retval  ECX  Returns number of encrypted guest supported simultaneosuly.
  @retval  EDX  Returns minimum SEV enabled and SEV disbled ASID..

  <b>Example usage</b>
  @code
  UINT32 Eax;
  UINT32 Ebx;
  UINT32 Ecx;
  UINT32 Edx;

  AsmCpuid (CPUID_MEMORY_ENCRYPTION_INFO, &Eax, &Ebx, &Ecx, &Edx);
  @endcode
**/

#define CPUID_MEMORY_ENCRYPTION_INFO             0x8000001F

/**
  CPUID Memory Encryption support information EAX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] Secure Memory Encryption (Sme) Support
    ///
    UINT32  SmeBit:1;

    ///
    /// [Bit 1] Secure Encrypted Virtualization (Sev) Support
    ///
    UINT32  SevBit:1;

    ///
    /// [Bit 2] Page flush MSR support
    ///
    UINT32  PageFlushMsrBit:1;

    ///
    /// [Bit 3] Encrypted state support
    ///
    UINT32  SevEsBit:1;

    ///
    /// [Bit 4:31] Reserved
    ///
    UINT32  ReservedBits:28;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EAX;

/**
  CPUID Memory Encryption support information EBX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0:5] Page table bit number used to enable memory encryption
    ///
    UINT32  PtePosBits:6;

    ///
    /// [Bit 6:11] Reduction of system physical address space bits when memory encryption is enabled
    ///
    UINT32  ReducedPhysBits:5;

    ///
    /// [Bit 12:31] Reserved
    ///
    UINT32  ReservedBits:21;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EBX;

/**
  CPUID Memory Encryption support information ECX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0:31] Number of encrypted guest supported simultaneously
    ///
    UINT32  NumGuests;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_ECX;

/**
  CPUID Memory Encryption support information EDX for CPUID leaf
  #CPUID_MEMORY_ENCRYPTION_INFO.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0:31] Minimum SEV enabled, SEV-ES disabled ASID
    ///
    UINT32  MinAsid;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
} CPUID_MEMORY_ENCRYPTION_INFO_EDX;

#endif
