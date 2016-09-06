/** @file
  MSR Definitions for Intel(R) Xeon(R) Processor E7 Family.

  Provides defines for Machine Specific Registers(MSR) indexes. Data structures
  are provided for MSRs that contain one or more bit fields.  If the MSR value
  returned is a single 32-bit or 64-bit value, then a data structure is not
  provided for that MSR.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Specification Reference:
  Intel(R) 64 and IA-32 Architectures Software Developer's Manual, Volume 3,
  December 2015, Chapter 35 Model-Specific-Registers (MSR), Section 35-7.

**/

#ifndef __XEON_E7_MSR_H__
#define __XEON_E7_MSR_H__

#include <Register/ArchitecturalMsr.h>

/**
  Package. Reserved Attempt to read/write will cause #UD.

  @param  ECX  MSR_XEON_E7_TURBO_RATIO_LIMIT (0x000001AD)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_TURBO_RATIO_LIMIT);
  AsmWriteMsr64 (MSR_XEON_E7_TURBO_RATIO_LIMIT, Msr);
  @endcode
  @note MSR_XEON_E7_TURBO_RATIO_LIMIT is defined as MSR_TURBO_RATIO_LIMIT in SDM.
**/
#define MSR_XEON_E7_TURBO_RATIO_LIMIT            0x000001AD


/**
  Package. Uncore C-box 8 perfmon local box control MSR.

  @param  ECX  MSR_XEON_E7_C8_PMON_BOX_CTRL (0x00000F40)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C8_PMON_BOX_CTRL);
  AsmWriteMsr64 (MSR_XEON_E7_C8_PMON_BOX_CTRL, Msr);
  @endcode
  @note MSR_XEON_E7_C8_PMON_BOX_CTRL is defined as MSR_C8_PMON_BOX_CTRL in SDM.
**/
#define MSR_XEON_E7_C8_PMON_BOX_CTRL             0x00000F40


/**
  Package. Uncore C-box 8 perfmon local box status MSR.

  @param  ECX  MSR_XEON_E7_C8_PMON_BOX_STATUS (0x00000F41)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C8_PMON_BOX_STATUS);
  AsmWriteMsr64 (MSR_XEON_E7_C8_PMON_BOX_STATUS, Msr);
  @endcode
  @note MSR_XEON_E7_C8_PMON_BOX_STATUS is defined as MSR_C8_PMON_BOX_STATUS in SDM.
**/
#define MSR_XEON_E7_C8_PMON_BOX_STATUS           0x00000F41


/**
  Package. Uncore C-box 8 perfmon local box overflow control MSR.

  @param  ECX  MSR_XEON_E7_C8_PMON_BOX_OVF_CTRL (0x00000F42)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C8_PMON_BOX_OVF_CTRL);
  AsmWriteMsr64 (MSR_XEON_E7_C8_PMON_BOX_OVF_CTRL, Msr);
  @endcode
  @note MSR_XEON_E7_C8_PMON_BOX_OVF_CTRL is defined as MSR_C8_PMON_BOX_OVF_CTRL in SDM.
**/
#define MSR_XEON_E7_C8_PMON_BOX_OVF_CTRL         0x00000F42


/**
  Package. Uncore C-box 8 perfmon event select MSR.

  @param  ECX  MSR_XEON_E7_C8_PMON_EVNT_SELn
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C8_PMON_EVNT_SEL0);
  AsmWriteMsr64 (MSR_XEON_E7_C8_PMON_EVNT_SEL0, Msr);
  @endcode
  @note MSR_XEON_E7_C8_PMON_EVNT_SEL0 is defined as MSR_C8_PMON_EVNT_SEL0 in SDM.
        MSR_XEON_E7_C8_PMON_EVNT_SEL1 is defined as MSR_C8_PMON_EVNT_SEL1 in SDM.
        MSR_XEON_E7_C8_PMON_EVNT_SEL2 is defined as MSR_C8_PMON_EVNT_SEL2 in SDM.
        MSR_XEON_E7_C8_PMON_EVNT_SEL3 is defined as MSR_C8_PMON_EVNT_SEL3 in SDM.
        MSR_XEON_E7_C8_PMON_EVNT_SEL4 is defined as MSR_C8_PMON_EVNT_SEL4 in SDM.
        MSR_XEON_E7_C8_PMON_EVNT_SEL5 is defined as MSR_C8_PMON_EVNT_SEL5 in SDM.
  @{
**/
#define MSR_XEON_E7_C8_PMON_EVNT_SEL0            0x00000F50
#define MSR_XEON_E7_C8_PMON_EVNT_SEL1            0x00000F52
#define MSR_XEON_E7_C8_PMON_EVNT_SEL2            0x00000F54
#define MSR_XEON_E7_C8_PMON_EVNT_SEL3            0x00000F56
#define MSR_XEON_E7_C8_PMON_EVNT_SEL4            0x00000F58
#define MSR_XEON_E7_C8_PMON_EVNT_SEL5            0x00000F5A
/// @}


/**
  Package. Uncore C-box 8 perfmon counter MSR.

  @param  ECX  MSR_XEON_E7_C8_PMON_CTRn
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C8_PMON_CTR0);
  AsmWriteMsr64 (MSR_XEON_E7_C8_PMON_CTR0, Msr);
  @endcode
  @note MSR_XEON_E7_C8_PMON_CTR0 is defined as MSR_C8_PMON_CTR0 in SDM.
        MSR_XEON_E7_C8_PMON_CTR1 is defined as MSR_C8_PMON_CTR1 in SDM.
        MSR_XEON_E7_C8_PMON_CTR2 is defined as MSR_C8_PMON_CTR2 in SDM.
        MSR_XEON_E7_C8_PMON_CTR3 is defined as MSR_C8_PMON_CTR3 in SDM.
        MSR_XEON_E7_C8_PMON_CTR4 is defined as MSR_C8_PMON_CTR4 in SDM.
        MSR_XEON_E7_C8_PMON_CTR5 is defined as MSR_C8_PMON_CTR5 in SDM.
  @{
**/
#define MSR_XEON_E7_C8_PMON_CTR0                 0x00000F51
#define MSR_XEON_E7_C8_PMON_CTR1                 0x00000F53
#define MSR_XEON_E7_C8_PMON_CTR2                 0x00000F55
#define MSR_XEON_E7_C8_PMON_CTR3                 0x00000F57
#define MSR_XEON_E7_C8_PMON_CTR4                 0x00000F59
#define MSR_XEON_E7_C8_PMON_CTR5                 0x00000F5B
/// @}


/**
  Package. Uncore C-box 9 perfmon local box control MSR.

  @param  ECX  MSR_XEON_E7_C9_PMON_BOX_CTRL (0x00000FC0)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C9_PMON_BOX_CTRL);
  AsmWriteMsr64 (MSR_XEON_E7_C9_PMON_BOX_CTRL, Msr);
  @endcode
  @note MSR_XEON_E7_C9_PMON_BOX_CTRL is defined as MSR_C9_PMON_BOX_CTRL in SDM.
**/
#define MSR_XEON_E7_C9_PMON_BOX_CTRL             0x00000FC0


/**
  Package. Uncore C-box 9 perfmon local box status MSR.

  @param  ECX  MSR_XEON_E7_C9_PMON_BOX_STATUS (0x00000FC1)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C9_PMON_BOX_STATUS);
  AsmWriteMsr64 (MSR_XEON_E7_C9_PMON_BOX_STATUS, Msr);
  @endcode
  @note MSR_XEON_E7_C9_PMON_BOX_STATUS is defined as MSR_C9_PMON_BOX_STATUS in SDM.
**/
#define MSR_XEON_E7_C9_PMON_BOX_STATUS           0x00000FC1


/**
  Package. Uncore C-box 9 perfmon local box overflow control MSR.

  @param  ECX  MSR_XEON_E7_C9_PMON_BOX_OVF_CTRL (0x00000FC2)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C9_PMON_BOX_OVF_CTRL);
  AsmWriteMsr64 (MSR_XEON_E7_C9_PMON_BOX_OVF_CTRL, Msr);
  @endcode
  @note MSR_XEON_E7_C9_PMON_BOX_OVF_CTRL is defined as MSR_C9_PMON_BOX_OVF_CTRL in SDM.
**/
#define MSR_XEON_E7_C9_PMON_BOX_OVF_CTRL         0x00000FC2


/**
  Package. Uncore C-box 9 perfmon event select MSR.

  @param  ECX  MSR_XEON_E7_C9_PMON_EVNT_SELn
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C9_PMON_EVNT_SEL0);
  AsmWriteMsr64 (MSR_XEON_E7_C9_PMON_EVNT_SEL0, Msr);
  @endcode
  @note MSR_XEON_E7_C9_PMON_EVNT_SEL0 is defined as MSR_C9_PMON_EVNT_SEL0 in SDM.
        MSR_XEON_E7_C9_PMON_EVNT_SEL1 is defined as MSR_C9_PMON_EVNT_SEL1 in SDM.
        MSR_XEON_E7_C9_PMON_EVNT_SEL2 is defined as MSR_C9_PMON_EVNT_SEL2 in SDM.
        MSR_XEON_E7_C9_PMON_EVNT_SEL3 is defined as MSR_C9_PMON_EVNT_SEL3 in SDM.
        MSR_XEON_E7_C9_PMON_EVNT_SEL4 is defined as MSR_C9_PMON_EVNT_SEL4 in SDM.
        MSR_XEON_E7_C9_PMON_EVNT_SEL5 is defined as MSR_C9_PMON_EVNT_SEL5 in SDM.
  @{
**/
#define MSR_XEON_E7_C9_PMON_EVNT_SEL0            0x00000FD0
#define MSR_XEON_E7_C9_PMON_EVNT_SEL1            0x00000FD2
#define MSR_XEON_E7_C9_PMON_EVNT_SEL2            0x00000FD4
#define MSR_XEON_E7_C9_PMON_EVNT_SEL3            0x00000FD6
#define MSR_XEON_E7_C9_PMON_EVNT_SEL4            0x00000FD8
#define MSR_XEON_E7_C9_PMON_EVNT_SEL5            0x00000FDA
/// @}


/**
  Package. Uncore C-box 9 perfmon counter MSR.

  @param  ECX  MSR_XEON_E7_C9_PMON_CTRn
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_E7_C9_PMON_CTR0);
  AsmWriteMsr64 (MSR_XEON_E7_C9_PMON_CTR0, Msr);
  @endcode
  @note MSR_XEON_E7_C9_PMON_CTR0 is defined as MSR_C9_PMON_CTR0 in SDM.
        MSR_XEON_E7_C9_PMON_CTR1 is defined as MSR_C9_PMON_CTR1 in SDM.
        MSR_XEON_E7_C9_PMON_CTR2 is defined as MSR_C9_PMON_CTR2 in SDM.
        MSR_XEON_E7_C9_PMON_CTR3 is defined as MSR_C9_PMON_CTR3 in SDM.
        MSR_XEON_E7_C9_PMON_CTR4 is defined as MSR_C9_PMON_CTR4 in SDM.
        MSR_XEON_E7_C9_PMON_CTR5 is defined as MSR_C9_PMON_CTR5 in SDM.
  @{
**/
#define MSR_XEON_E7_C9_PMON_CTR0                 0x00000FD1
#define MSR_XEON_E7_C9_PMON_CTR1                 0x00000FD3
#define MSR_XEON_E7_C9_PMON_CTR2                 0x00000FD5
#define MSR_XEON_E7_C9_PMON_CTR3                 0x00000FD7
#define MSR_XEON_E7_C9_PMON_CTR4                 0x00000FD9
#define MSR_XEON_E7_C9_PMON_CTR5                 0x00000FDB
/// @}

#endif
