/** @file
  MSR Definitions for Intel(R) Xeon(R) Processor D product Family.

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
  December 2015, Chapter 35 Model-Specific-Registers (MSR), Section 35-13.

**/

#ifndef __XEON_D_MSR_H__
#define __XEON_D_MSR_H__

#include <Register/ArchitecturalMsr.h>

/**
  Package. Protected Processor Inventory Number Enable Control (R/W).

  @param  ECX  MSR_XEON_D_PPIN_CTL (0x0000004E)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PPIN_CTL_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PPIN_CTL_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PPIN_CTL_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PPIN_CTL);
  AsmWriteMsr64 (MSR_XEON_D_PPIN_CTL, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PPIN_CTL is defined as MSR_PPIN_CTL in SDM.
**/
#define MSR_XEON_D_PPIN_CTL                      0x0000004E

/**
  MSR information returned for MSR index #MSR_XEON_D_PPIN_CTL
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] LockOut (R/WO) See Table 35-21.
    ///
    UINT32  LockOut:1;
    ///
    /// [Bit 1] Enable_PPIN (R/W) See Table 35-21.
    ///
    UINT32  Enable_PPIN:1;
    UINT32  Reserved1:30;
    UINT32  Reserved2:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PPIN_CTL_REGISTER;


/**
  Package. Protected Processor Inventory Number (R/O). Protected Processor
  Inventory Number (R/O) See Table 35-21.

  @param  ECX  MSR_XEON_D_PPIN (0x0000004F)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_PPIN);
  @endcode
  @note MSR_XEON_D_PPIN is defined as MSR_PPIN in SDM.
**/
#define MSR_XEON_D_PPIN                          0x0000004F


/**
  Package. See http://biosbits.org.

  @param  ECX  MSR_XEON_D_PLATFORM_INFO (0x000000CE)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PLATFORM_INFO_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PLATFORM_INFO_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PLATFORM_INFO_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PLATFORM_INFO);
  AsmWriteMsr64 (MSR_XEON_D_PLATFORM_INFO, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PLATFORM_INFO is defined as MSR_PLATFORM_INFO in SDM.
**/
#define MSR_XEON_D_PLATFORM_INFO                 0x000000CE

/**
  MSR information returned for MSR index #MSR_XEON_D_PLATFORM_INFO
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32  Reserved1:8;
    ///
    /// [Bits 15:8] Package. Maximum Non-Turbo Ratio (R/O) See Table 35-21.
    ///
    UINT32  MaximumNonTurboRatio:8;
    UINT32  Reserved2:7;
    ///
    /// [Bit 23] Package. PPIN_CAP (R/O) See Table 35-21.
    ///
    UINT32  PPIN_CAP:1;
    UINT32  Reserved3:4;
    ///
    /// [Bit 28] Package. Programmable Ratio Limit for Turbo Mode (R/O) See
    /// Table 35-21.
    ///
    UINT32  RatioLimit:1;
    ///
    /// [Bit 29] Package. Programmable TDP Limit for Turbo Mode (R/O) See
    /// Table 35-21.
    ///
    UINT32  TDPLimit:1;
    ///
    /// [Bit 30] Package. Programmable TJ OFFSET (R/O) See Table 35-21.
    ///
    UINT32  TJOFFSET:1;
    UINT32  Reserved4:1;
    UINT32  Reserved5:8;
    ///
    /// [Bits 47:40] Package. Maximum Efficiency Ratio (R/O) See Table 35-21.
    ///
    UINT32  MaximumEfficiencyRatio:8;
    UINT32  Reserved6:16;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PLATFORM_INFO_REGISTER;


/**
  Core. C-State Configuration Control (R/W) Note: C-state values are processor
  specific C-state code names, unrelated to MWAIT extension C-state parameters
  or ACPI C-states. `See http://biosbits.org. <http://biosbits.org>`__.

  @param  ECX  MSR_XEON_D_PKG_CST_CONFIG_CONTROL (0x000000E2)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_CST_CONFIG_CONTROL_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_CST_CONFIG_CONTROL_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PKG_CST_CONFIG_CONTROL_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PKG_CST_CONFIG_CONTROL);
  AsmWriteMsr64 (MSR_XEON_D_PKG_CST_CONFIG_CONTROL, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PKG_CST_CONFIG_CONTROL is defined as MSR_PKG_CST_CONFIG_CONTROL in SDM.
**/
#define MSR_XEON_D_PKG_CST_CONFIG_CONTROL        0x000000E2

/**
  MSR information returned for MSR index #MSR_XEON_D_PKG_CST_CONFIG_CONTROL
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 2:0] Package C-State Limit (R/W) Specifies the lowest
    /// processor-specific C-state code name (consuming the least power) for
    /// the package. The default is set as factory-configured package C-state
    /// limit. The following C-state code name encodings are supported: 000b:
    /// C0/C1 (no package C-state support) 001b: C2 010b: C6 (non-retention)
    /// 011b: C6 (retention) 111b: No Package C state limits. All C states
    /// supported by the processor are available.
    ///
    UINT32  Limit:3;
    UINT32  Reserved1:7;
    ///
    /// [Bit 10] I/O MWAIT Redirection Enable (R/W).
    ///
    UINT32  IO_MWAIT:1;
    UINT32  Reserved2:4;
    ///
    /// [Bit 15] CFG Lock (R/WO).
    ///
    UINT32  CFGLock:1;
    ///
    /// [Bit 16] Automatic C-State Conversion Enable (R/W) If 1, the processor
    /// will convert HALT or MWAT(C1) to MWAIT(C6).
    ///
    UINT32  CStateConversion:1;
    UINT32  Reserved3:8;
    ///
    /// [Bit 25] C3 State Auto Demotion Enable (R/W).
    ///
    UINT32  C3AutoDemotion:1;
    ///
    /// [Bit 26] C1 State Auto Demotion Enable (R/W).
    ///
    UINT32  C1AutoDemotion:1;
    ///
    /// [Bit 27] Enable C3 Undemotion (R/W).
    ///
    UINT32  C3Undemotion:1;
    ///
    /// [Bit 28] Enable C1 Undemotion (R/W).
    ///
    UINT32  C1Undemotion:1;
    ///
    /// [Bit 29] Package C State Demotion Enable (R/W).
    ///
    UINT32  CStateDemotion:1;
    ///
    /// [Bit 30] Package C State UnDemotion Enable (R/W).
    ///
    UINT32  CStateUndemotion:1;
    UINT32  Reserved4:1;
    UINT32  Reserved5:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PKG_CST_CONFIG_CONTROL_REGISTER;


/**
  Thread. Global Machine Check Capability (R/O).

  @param  ECX  MSR_XEON_D_IA32_MCG_CAP (0x00000179)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_MCG_CAP_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_MCG_CAP_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_IA32_MCG_CAP_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_IA32_MCG_CAP);
  @endcode
  @note MSR_XEON_D_IA32_MCG_CAP is defined as IA32_MCG_CAP in SDM.
**/
#define MSR_XEON_D_IA32_MCG_CAP                  0x00000179

/**
  MSR information returned for MSR index #MSR_XEON_D_IA32_MCG_CAP
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Count.
    ///
    UINT32  Count:8;
    ///
    /// [Bit 8] MCG_CTL_P.
    ///
    UINT32  MCG_CTL_P:1;
    ///
    /// [Bit 9] MCG_EXT_P.
    ///
    UINT32  MCG_EXT_P:1;
    ///
    /// [Bit 10] MCP_CMCI_P.
    ///
    UINT32  MCP_CMCI_P:1;
    ///
    /// [Bit 11] MCG_TES_P.
    ///
    UINT32  MCG_TES_P:1;
    UINT32  Reserved1:4;
    ///
    /// [Bits 23:16] MCG_EXT_CNT.
    ///
    UINT32  MCG_EXT_CNT:8;
    ///
    /// [Bit 24] MCG_SER_P.
    ///
    UINT32  MCG_SER_P:1;
    ///
    /// [Bit 25] MCG_EM_P.
    ///
    UINT32  MCG_EM_P:1;
    ///
    /// [Bit 26] MCG_ELOG_P.
    ///
    UINT32  MCG_ELOG_P:1;
    UINT32  Reserved2:5;
    UINT32  Reserved3:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_IA32_MCG_CAP_REGISTER;


/**
  THREAD. Enhanced SMM Capabilities (SMM-RO) Reports SMM capability
  Enhancement. Accessible only while in SMM.

  @param  ECX  MSR_XEON_D_SMM_MCA_CAP (0x0000017D)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_SMM_MCA_CAP_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_SMM_MCA_CAP_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_SMM_MCA_CAP_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_SMM_MCA_CAP);
  AsmWriteMsr64 (MSR_XEON_D_SMM_MCA_CAP, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_SMM_MCA_CAP is defined as MSR_SMM_MCA_CAP in SDM.
**/
#define MSR_XEON_D_SMM_MCA_CAP                   0x0000017D

/**
  MSR information returned for MSR index #MSR_XEON_D_SMM_MCA_CAP
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32  Reserved1:32;
    UINT32  Reserved2:26;
    ///
    /// [Bit 58] SMM_Code_Access_Chk (SMM-RO) If set to 1 indicates that the
    /// SMM code access restriction is supported and a host-space interface
    /// available to SMM handler.
    ///
    UINT32  SMM_Code_Access_Chk:1;
    ///
    /// [Bit 59] Long_Flow_Indication (SMM-RO) If set to 1 indicates that the
    /// SMM long flow indicator is supported and a host-space interface
    /// available to SMM handler.
    ///
    UINT32  Long_Flow_Indication:1;
    UINT32  Reserved3:4;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_SMM_MCA_CAP_REGISTER;


/**
  Package.

  @param  ECX  MSR_XEON_D_TEMPERATURE_TARGET (0x000001A2)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_TEMPERATURE_TARGET_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_TEMPERATURE_TARGET_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_TEMPERATURE_TARGET_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_TEMPERATURE_TARGET);
  AsmWriteMsr64 (MSR_XEON_D_TEMPERATURE_TARGET, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_TEMPERATURE_TARGET is defined as MSR_TEMPERATURE_TARGET in SDM.
**/
#define MSR_XEON_D_TEMPERATURE_TARGET            0x000001A2

/**
  MSR information returned for MSR index #MSR_XEON_D_TEMPERATURE_TARGET
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32  Reserved1:16;
    ///
    /// [Bits 23:16] Temperature Target (RO) See Table 35-21.
    ///
    UINT32  TemperatureTarget:8;
    ///
    /// [Bits 27:24] TCC Activation Offset (R/W) See Table 35-21.
    ///
    UINT32  TCCActivationOffset:4;
    UINT32  Reserved2:4;
    UINT32  Reserved3:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_TEMPERATURE_TARGET_REGISTER;


/**
  Package. Maximum Ratio Limit of Turbo Mode RO if MSR_PLATFORM_INFO.[28] = 0,
  RW if MSR_PLATFORM_INFO.[28] = 1.

  @param  ECX  MSR_XEON_D_TURBO_RATIO_LIMIT (0x000001AD)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_TURBO_RATIO_LIMIT_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_TURBO_RATIO_LIMIT);
  @endcode
  @note MSR_XEON_D_TURBO_RATIO_LIMIT is defined as MSR_TURBO_RATIO_LIMIT in SDM.
**/
#define MSR_XEON_D_TURBO_RATIO_LIMIT             0x000001AD

/**
  MSR information returned for MSR index #MSR_XEON_D_TURBO_RATIO_LIMIT
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Package. Maximum Ratio Limit for 1C.
    ///
    UINT32  Maximum1C:8;
    ///
    /// [Bits 15:8] Package. Maximum Ratio Limit for 2C.
    ///
    UINT32  Maximum2C:8;
    ///
    /// [Bits 23:16] Package. Maximum Ratio Limit for 3C.
    ///
    UINT32  Maximum3C:8;
    ///
    /// [Bits 31:24] Package. Maximum Ratio Limit for 4C.
    ///
    UINT32  Maximum4C:8;
    ///
    /// [Bits 39:32] Package. Maximum Ratio Limit for 5C.
    ///
    UINT32  Maximum5C:8;
    ///
    /// [Bits 47:40] Package. Maximum Ratio Limit for 6C.
    ///
    UINT32  Maximum6C:8;
    ///
    /// [Bits 55:48] Package. Maximum Ratio Limit for 7C.
    ///
    UINT32  Maximum7C:8;
    ///
    /// [Bits 63:56] Package. Maximum Ratio Limit for 8C.
    ///
    UINT32  Maximum8C:8;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_TURBO_RATIO_LIMIT_REGISTER;


/**
  Package. Maximum Ratio Limit of Turbo Mode RO if MSR_PLATFORM_INFO.[28] = 0,
  RW if MSR_PLATFORM_INFO.[28] = 1.

  @param  ECX  MSR_XEON_D_TURBO_RATIO_LIMIT1 (0x000001AE)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT1_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT1_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_TURBO_RATIO_LIMIT1_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_TURBO_RATIO_LIMIT1);
  @endcode
  @note MSR_XEON_D_TURBO_RATIO_LIMIT1 is defined as MSR_TURBO_RATIO_LIMIT1 in SDM.
**/
#define MSR_XEON_D_TURBO_RATIO_LIMIT1            0x000001AE

/**
  MSR information returned for MSR index #MSR_XEON_D_TURBO_RATIO_LIMIT1
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] Package. Maximum Ratio Limit for 9C.
    ///
    UINT32  Maximum9C:8;
    ///
    /// [Bits 15:8] Package. Maximum Ratio Limit for 10C.
    ///
    UINT32  Maximum10C:8;
    ///
    /// [Bits 23:16] Package. Maximum Ratio Limit for 11C.
    ///
    UINT32  Maximum11C:8;
    ///
    /// [Bits 31:24] Package. Maximum Ratio Limit for 12C.
    ///
    UINT32  Maximum12C:8;
    ///
    /// [Bits 39:32] Package. Maximum Ratio Limit for 13C.
    ///
    UINT32  Maximum13C:8;
    ///
    /// [Bits 47:40] Package. Maximum Ratio Limit for 14C.
    ///
    UINT32  Maximum14C:8;
    ///
    /// [Bits 55:48] Package. Maximum Ratio Limit for 15C.
    ///
    UINT32  Maximum15C:8;
    ///
    /// [Bits 63:56] Package. Maximum Ratio Limit for 16C.
    ///
    UINT32  Maximum16C:8;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_TURBO_RATIO_LIMIT1_REGISTER;


/**
  Package. Unit Multipliers used in RAPL Interfaces (R/O).

  @param  ECX  MSR_XEON_D_RAPL_POWER_UNIT (0x00000606)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_RAPL_POWER_UNIT_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_RAPL_POWER_UNIT_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_RAPL_POWER_UNIT_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_RAPL_POWER_UNIT);
  @endcode
  @note MSR_XEON_D_RAPL_POWER_UNIT is defined as MSR_RAPL_POWER_UNIT in SDM.
**/
#define MSR_XEON_D_RAPL_POWER_UNIT               0x00000606

/**
  MSR information returned for MSR index #MSR_XEON_D_RAPL_POWER_UNIT
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 3:0] Package. Power Units See Section 14.9.1, "RAPL Interfaces.".
    ///
    UINT32  PowerUnits:4;
    UINT32  Reserved1:4;
    ///
    /// [Bits 12:8] Package. Energy Status Units Energy related information
    /// (in Joules) is based on the multiplier, 1/2^ESU; where ESU is an
    /// unsigned integer represented by bits 12:8. Default value is 0EH (or 61
    /// micro-joules).
    ///
    UINT32  EnergyStatusUnits:5;
    UINT32  Reserved2:3;
    ///
    /// [Bits 19:16] Package. Time Units See Section 14.9.1, "RAPL
    /// Interfaces.".
    ///
    UINT32  TimeUnits:4;
    UINT32  Reserved3:12;
    UINT32  Reserved4:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_RAPL_POWER_UNIT_REGISTER;


/**
  Package. DRAM RAPL Power Limit Control (R/W)  See Section 14.9.5, "DRAM RAPL
  Domain.".

  @param  ECX  MSR_XEON_D_DRAM_POWER_LIMIT (0x00000618)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_DRAM_POWER_LIMIT);
  AsmWriteMsr64 (MSR_XEON_D_DRAM_POWER_LIMIT, Msr);
  @endcode
  @note MSR_XEON_D_DRAM_POWER_LIMIT is defined as MSR_DRAM_POWER_LIMIT in SDM.
**/
#define MSR_XEON_D_DRAM_POWER_LIMIT              0x00000618


/**
  Package. DRAM Energy Status (R/O)  See Section 14.9.5, "DRAM RAPL Domain.".

  @param  ECX  MSR_XEON_D_DRAM_ENERGY_STATUS (0x00000619)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_DRAM_ENERGY_STATUS);
  @endcode
  @note MSR_XEON_D_DRAM_ENERGY_STATUS is defined as MSR_DRAM_ENERGY_STATUS in SDM.
**/
#define MSR_XEON_D_DRAM_ENERGY_STATUS            0x00000619


/**
  Package. DRAM Performance Throttling Status (R/O) See Section 14.9.5, "DRAM
  RAPL Domain.".

  @param  ECX  MSR_XEON_D_DRAM_PERF_STATUS (0x0000061B)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_DRAM_PERF_STATUS);
  @endcode
  @note MSR_XEON_D_DRAM_PERF_STATUS is defined as MSR_DRAM_PERF_STATUS in SDM.
**/
#define MSR_XEON_D_DRAM_PERF_STATUS              0x0000061B


/**
  Package. DRAM RAPL Parameters (R/W) See Section 14.9.5, "DRAM RAPL Domain.".

  @param  ECX  MSR_XEON_D_DRAM_POWER_INFO (0x0000061C)
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_DRAM_POWER_INFO);
  AsmWriteMsr64 (MSR_XEON_D_DRAM_POWER_INFO, Msr);
  @endcode
  @note MSR_XEON_D_DRAM_POWER_INFO is defined as MSR_DRAM_POWER_INFO in SDM.
**/
#define MSR_XEON_D_DRAM_POWER_INFO               0x0000061C


/**
  Package. Indicator of Frequency Clipping in Processor Cores (R/W) (frequency
  refers to processor core frequency).

  @param  ECX  MSR_XEON_D_CORE_PERF_LIMIT_REASONS (0x00000690)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_CORE_PERF_LIMIT_REASONS_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_CORE_PERF_LIMIT_REASONS_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_CORE_PERF_LIMIT_REASONS_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_CORE_PERF_LIMIT_REASONS);
  AsmWriteMsr64 (MSR_XEON_D_CORE_PERF_LIMIT_REASONS, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_CORE_PERF_LIMIT_REASONS is defined as MSR_CORE_PERF_LIMIT_REASONS in SDM.
**/
#define MSR_XEON_D_CORE_PERF_LIMIT_REASONS       0x00000690

/**
  MSR information returned for MSR index #MSR_XEON_D_CORE_PERF_LIMIT_REASONS
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] PROCHOT Status (R0) When set, processor core frequency is
    /// reduced below the operating system request due to assertion of
    /// external PROCHOT.
    ///
    UINT32  PROCHOT_Status:1;
    ///
    /// [Bit 1] Thermal Status (R0) When set, frequency is reduced below the
    /// operating system request due to a thermal event.
    ///
    UINT32  ThermalStatus:1;
    ///
    /// [Bit 2] Power Budget Management Status (R0) When set, frequency is
    /// reduced below the operating system request due to PBM limit.
    ///
    UINT32  PowerBudgetManagementStatus:1;
    ///
    /// [Bit 3] Platform Configuration Services Status (R0) When set,
    /// frequency is reduced below the operating system request due to PCS
    /// limit.
    ///
    UINT32  PlatformConfigurationServicesStatus:1;
    UINT32  Reserved1:1;
    ///
    /// [Bit 5] Autonomous Utilization-Based Frequency Control Status (R0)
    /// When set, frequency is reduced below the operating system request
    /// because the processor has detected that utilization is low.
    ///
    UINT32  AutonomousUtilizationBasedFrequencyControlStatus:1;
    ///
    /// [Bit 6] VR Therm Alert Status (R0) When set, frequency is reduced
    /// below the operating system request due to a thermal alert from the
    /// Voltage Regulator.
    ///
    UINT32  VRThermAlertStatus:1;
    UINT32  Reserved2:1;
    ///
    /// [Bit 8] Electrical Design Point Status (R0) When set, frequency is
    /// reduced below the operating system request due to electrical design
    /// point constraints (e.g. maximum electrical current consumption).
    ///
    UINT32  ElectricalDesignPointStatus:1;
    UINT32  Reserved3:1;
    ///
    /// [Bit 10] Multi-Core Turbo Status (R0) When set, frequency is reduced
    /// below the operating system request due to Multi-Core Turbo limits.
    ///
    UINT32  MultiCoreTurboStatus:1;
    UINT32  Reserved4:2;
    ///
    /// [Bit 13] Core Frequency P1 Status (R0) When set, frequency is reduced
    /// below max non-turbo P1.
    ///
    UINT32  FrequencyP1Status:1;
    ///
    /// [Bit 14] Core Max n-core Turbo Frequency Limiting Status (R0) When
    /// set, frequency is reduced below max n-core turbo frequency.
    ///
    UINT32  TurboFrequencyLimitingStatus:1;
    ///
    /// [Bit 15] Core Frequency Limiting Status (R0) When set, frequency is
    /// reduced below the operating system request.
    ///
    UINT32  FrequencyLimitingStatus:1;
    ///
    /// [Bit 16] PROCHOT Log  When set, indicates that the PROCHOT Status bit
    /// has asserted since the log bit was last cleared. This log bit will
    /// remain set until cleared by software writing 0.
    ///
    UINT32  PROCHOT_Log:1;
    ///
    /// [Bit 17] Thermal Log  When set, indicates that the Thermal Status bit
    /// has asserted since the log bit was last cleared. This log bit will
    /// remain set until cleared by software writing 0.
    ///
    UINT32  ThermalLog:1;
    ///
    /// [Bit 18] Power Budget Management Log  When set, indicates that the PBM
    /// Status bit has asserted since the log bit was last cleared. This log
    /// bit will remain set until cleared by software writing 0.
    ///
    UINT32  PowerBudgetManagementLog:1;
    ///
    /// [Bit 19] Platform Configuration Services Log  When set, indicates that
    /// the PCS Status bit has asserted since the log bit was last cleared.
    /// This log bit will remain set until cleared by software writing 0.
    ///
    UINT32  PlatformConfigurationServicesLog:1;
    UINT32  Reserved5:1;
    ///
    /// [Bit 21] Autonomous Utilization-Based Frequency Control Log  When set,
    /// indicates that the AUBFC Status bit has asserted since the log bit was
    /// last cleared. This log bit will remain set until cleared by software
    /// writing 0.
    ///
    UINT32  AutonomousUtilizationBasedFrequencyControlLog:1;
    ///
    /// [Bit 22] VR Therm Alert Log  When set, indicates that the VR Therm
    /// Alert Status bit has asserted since the log bit was last cleared. This
    /// log bit will remain set until cleared by software writing 0.
    ///
    UINT32  VRThermAlertLog:1;
    UINT32  Reserved6:1;
    ///
    /// [Bit 24] Electrical Design Point Log  When set, indicates that the EDP
    /// Status bit has asserted since the log bit was last cleared. This log
    /// bit will remain set until cleared by software writing 0.
    ///
    UINT32  ElectricalDesignPointLog:1;
    UINT32  Reserved7:1;
    ///
    /// [Bit 26] Multi-Core Turbo Log  When set, indicates that the Multi-Core
    /// Turbo Status bit has asserted since the log bit was last cleared. This
    /// log bit will remain set until cleared by software writing 0.
    ///
    UINT32  MultiCoreTurboLog:1;
    UINT32  Reserved8:2;
    ///
    /// [Bit 29] Core Frequency P1 Log When set, indicates that the Core
    /// Frequency P1 Status bit has asserted since the log bit was last
    /// cleared. This log bit will remain set until cleared by software
    /// writing 0.
    ///
    UINT32  CoreFrequencyP1Log:1;
    ///
    /// [Bit 30] Core Max n-core Turbo Frequency Limiting Log When set,
    /// indicates that the Core Max n-core Turbo Frequency Limiting Status bit
    /// has asserted since the log bit was last cleared. This log bit will
    /// remain set until cleared by software writing 0.
    ///
    UINT32  TurboFrequencyLimitingLog:1;
    ///
    /// [Bit 31] Core Frequency Limiting Log When set, indicates that the Core
    /// Frequency Limiting Status bit has asserted since the log bit was last
    /// cleared. This log bit will remain set until cleared by software
    /// writing 0.
    ///
    UINT32  CoreFrequencyLimitingLog:1;
    UINT32  Reserved9:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_CORE_PERF_LIMIT_REASONS_REGISTER;


/**
  THREAD. Monitoring Event Select Register (R/W) if CPUID.(EAX=07H,
  ECX=0):EBX.PQM[bit 12] = 1.

  @param  ECX  MSR_XEON_D_IA32_QM_EVTSEL (0x00000C8D)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_QM_EVTSEL_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_QM_EVTSEL_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_IA32_QM_EVTSEL_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_IA32_QM_EVTSEL);
  AsmWriteMsr64 (MSR_XEON_D_IA32_QM_EVTSEL, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_IA32_QM_EVTSEL is defined as IA32_QM_EVTSEL in SDM.
**/
#define MSR_XEON_D_IA32_QM_EVTSEL                0x00000C8D

/**
  MSR information returned for MSR index #MSR_XEON_D_IA32_QM_EVTSEL
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 7:0] EventID (RW) Event encoding: 0x00: no monitoring 0x01: L3
    /// occupancy monitoring 0x02: Total memory bandwidth monitoring 0x03:
    /// Local memory bandwidth monitoring All other encoding reserved.
    ///
    UINT32  EventID:8;
    UINT32  Reserved1:24;
    ///
    /// [Bits 41:32] RMID (RW).
    ///
    UINT32  RMID:10;
    UINT32  Reserved2:22;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_IA32_QM_EVTSEL_REGISTER;


/**
  THREAD. Resource Association Register (R/W).

  @param  ECX  MSR_XEON_D_IA32_PQR_ASSOC (0x00000C8F)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_PQR_ASSOC_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_PQR_ASSOC_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_IA32_PQR_ASSOC_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_IA32_PQR_ASSOC);
  AsmWriteMsr64 (MSR_XEON_D_IA32_PQR_ASSOC, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_IA32_PQR_ASSOC is defined as IA32_PQR_ASSOC in SDM.
**/
#define MSR_XEON_D_IA32_PQR_ASSOC                0x00000C8F

/**
  MSR information returned for MSR index #MSR_XEON_D_IA32_PQR_ASSOC
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 9:0] RMID.
    ///
    UINT32  RMID:10;
    UINT32  Reserved1:22;
    ///
    /// [Bits 51:32] COS (R/W).
    ///
    UINT32  COS:20;
    UINT32  Reserved2:12;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_IA32_PQR_ASSOC_REGISTER;


/**
  Package. L3 Class Of Service Mask - COS n (R/W) if CPUID.(EAX=10H,
  ECX=1):EDX.COS_MAX[15:0] >= n.

  @param  ECX  MSR_XEON_D_IA32_L3_QOS_MASK_n
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_L3_QOS_MASK_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_L3_QOS_MASK_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_IA32_L3_QOS_MASK_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_IA32_L3_QOS_MASK_0);
  AsmWriteMsr64 (MSR_XEON_D_IA32_L3_QOS_MASK_0, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_IA32_L3_QOS_MASK_0  is defined as IA32_L3_QOS_MASK_0  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_1  is defined as IA32_L3_QOS_MASK_1  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_2  is defined as IA32_L3_QOS_MASK_2  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_3  is defined as IA32_L3_QOS_MASK_3  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_4  is defined as IA32_L3_QOS_MASK_4  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_5  is defined as IA32_L3_QOS_MASK_5  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_6  is defined as IA32_L3_QOS_MASK_6  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_7  is defined as IA32_L3_QOS_MASK_7  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_8  is defined as IA32_L3_QOS_MASK_8  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_9  is defined as IA32_L3_QOS_MASK_9  in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_10 is defined as IA32_L3_QOS_MASK_10 in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_11 is defined as IA32_L3_QOS_MASK_11 in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_12 is defined as IA32_L3_QOS_MASK_12 in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_13 is defined as IA32_L3_QOS_MASK_13 in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_14 is defined as IA32_L3_QOS_MASK_14 in SDM.
        MSR_XEON_D_IA32_L3_QOS_MASK_15 is defined as IA32_L3_QOS_MASK_15 in SDM.
  @{
**/
#define MSR_XEON_D_IA32_L3_QOS_MASK_0            0x00000C90
#define MSR_XEON_D_IA32_L3_QOS_MASK_1            0x00000C91
#define MSR_XEON_D_IA32_L3_QOS_MASK_2            0x00000C92
#define MSR_XEON_D_IA32_L3_QOS_MASK_3            0x00000C93
#define MSR_XEON_D_IA32_L3_QOS_MASK_4            0x00000C94
#define MSR_XEON_D_IA32_L3_QOS_MASK_5            0x00000C95
#define MSR_XEON_D_IA32_L3_QOS_MASK_6            0x00000C96
#define MSR_XEON_D_IA32_L3_QOS_MASK_7            0x00000C97
#define MSR_XEON_D_IA32_L3_QOS_MASK_8            0x00000C98
#define MSR_XEON_D_IA32_L3_QOS_MASK_9            0x00000C99
#define MSR_XEON_D_IA32_L3_QOS_MASK_10           0x00000C9A
#define MSR_XEON_D_IA32_L3_QOS_MASK_11           0x00000C9B
#define MSR_XEON_D_IA32_L3_QOS_MASK_12           0x00000C9C
#define MSR_XEON_D_IA32_L3_QOS_MASK_13           0x00000C9D
#define MSR_XEON_D_IA32_L3_QOS_MASK_14           0x00000C9E
#define MSR_XEON_D_IA32_L3_QOS_MASK_15           0x00000C9F
/// @}

/**
  MSR information returned for MSR indexes #MSR_XEON_D_IA32_L3_QOS_MASK_0
  to #MSR_XEON_D_IA32_L3_QOS_MASK_15.
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 19:0] CBM: Bit vector of available L3 ways for COS 0 enforcement.
    ///
    UINT32  CBM:20;
    UINT32  Reserved2:12;
    UINT32  Reserved3:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_IA32_L3_QOS_MASK_REGISTER;


/**
  Package. Config Ratio Limit of Turbo Mode RO if MSR_PLATFORM_INFO.[28] = 0,
  RW if MSR_PLATFORM_INFO.[28] = 1.

  @param  ECX  MSR_XEON_D_TURBO_RATIO_LIMIT3 (0x000001AC)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT3_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_TURBO_RATIO_LIMIT3_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_TURBO_RATIO_LIMIT3_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_TURBO_RATIO_LIMIT3);
  @endcode
  @note MSR_XEON_D_TURBO_RATIO_LIMIT3 is defined as MSR_TURBO_RATIO_LIMIT3 in SDM.
**/
#define MSR_XEON_D_TURBO_RATIO_LIMIT3            0x000001AC

/**
  MSR information returned for MSR index #MSR_XEON_D_TURBO_RATIO_LIMIT3
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    UINT32  Reserved1:32;
    UINT32  Reserved2:31;
    ///
    /// [Bit 63] Package. Semaphore for Turbo Ratio Limit Configuration If 1,
    /// the processor uses override configuration specified in
    /// MSR_TURBO_RATIO_LIMIT, MSR_TURBO_RATIO_LIMIT1. If 0, the processor
    /// uses factory-set configuration (Default).
    ///
    UINT32  TurboRatioLimitConfigurationSemaphore:1;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_TURBO_RATIO_LIMIT3_REGISTER;


/**
  Package. See Section 15.3.2.1, "IA32_MCi_CTL MSRs." through Section
  15.3.2.4, "IA32_MCi_MISC MSRs.".

  * Bank MC5 reports MC error from the Intel QPI 0 module.
  * Bank MC6 reports MC error from the integrated I/O module.
  * Bank MC7 reports MC error from the home agent HA 0.
  * Bank MC8 reports MC error from the home agent HA 1.
  * Banks MC9 through MC16 report MC error from each channel of the integrated
    memory controllers.
  * Bank MC17 reports MC error from the following pair of CBo/L3 Slices
    (if the pair is present): CBo0, CBo3, CBo6, CBo9, CBo12, CBo15.
  * Bank MC18 reports MC error from the following pair of CBo/L3 Slices
    (if the pair is present): CBo1, CBo4, CBo7, CBo10, CBo13, CBo16.
  * Bank MC19 reports MC error from the following pair of CBo/L3 Slices
    (if the pair is present): CBo2, CBo5, CBo8, CBo11, CBo14, CBo17.
  * Bank MC20 reports MC error from the Intel QPI 1 module.
  * Bank MC21 reports MC error from the Intel QPI 2 module.

  @param  ECX  MSR_XEON_D_MCi_CTL
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_MC5_CTL);
  AsmWriteMsr64 (MSR_XEON_D_MC5_CTL, Msr);
  @endcode
  @note MSR_XEON_D_MC5_CTL  is defined as MSR_MC5_CTL  in SDM.
        MSR_XEON_D_MC6_CTL  is defined as MSR_MC6_CTL  in SDM.
        MSR_XEON_D_MC7_CTL  is defined as MSR_MC7_CTL  in SDM.
        MSR_XEON_D_MC8_CTL  is defined as MSR_MC8_CTL  in SDM.
        MSR_XEON_D_MC9_CTL  is defined as MSR_MC9_CTL  in SDM.
        MSR_XEON_D_MC10_CTL is defined as MSR_MC10_CTL in SDM.
        MSR_XEON_D_MC11_CTL is defined as MSR_MC11_CTL in SDM.
        MSR_XEON_D_MC12_CTL is defined as MSR_MC12_CTL in SDM.
        MSR_XEON_D_MC13_CTL is defined as MSR_MC13_CTL in SDM.
        MSR_XEON_D_MC14_CTL is defined as MSR_MC14_CTL in SDM.
        MSR_XEON_D_MC15_CTL is defined as MSR_MC15_CTL in SDM.
        MSR_XEON_D_MC16_CTL is defined as MSR_MC16_CTL in SDM.
        MSR_XEON_D_MC17_CTL is defined as MSR_MC17_CTL in SDM.
        MSR_XEON_D_MC18_CTL is defined as MSR_MC18_CTL in SDM.
        MSR_XEON_D_MC19_CTL is defined as MSR_MC19_CTL in SDM.
        MSR_XEON_D_MC20_CTL is defined as MSR_MC20_CTL in SDM.
        MSR_XEON_D_MC21_CTL is defined as MSR_MC21_CTL in SDM.
  @{
**/
#define MSR_XEON_D_MC5_CTL                       0x00000414
#define MSR_XEON_D_MC6_CTL                       0x00000418
#define MSR_XEON_D_MC7_CTL                       0x0000041C
#define MSR_XEON_D_MC8_CTL                       0x00000420
#define MSR_XEON_D_MC9_CTL                       0x00000424
#define MSR_XEON_D_MC10_CTL                      0x00000428
#define MSR_XEON_D_MC11_CTL                      0x0000042C
#define MSR_XEON_D_MC12_CTL                      0x00000430
#define MSR_XEON_D_MC13_CTL                      0x00000434
#define MSR_XEON_D_MC14_CTL                      0x00000438
#define MSR_XEON_D_MC15_CTL                      0x0000043C
#define MSR_XEON_D_MC16_CTL                      0x00000440
#define MSR_XEON_D_MC17_CTL                      0x00000444
#define MSR_XEON_D_MC18_CTL                      0x00000448
#define MSR_XEON_D_MC19_CTL                      0x0000044C
#define MSR_XEON_D_MC20_CTL                      0x00000450
#define MSR_XEON_D_MC21_CTL                      0x00000454
/// @}

/**
  Package. See Section 15.3.2.1, "IA32_MCi_CTL MSRs." through Section
  15.3.2.4, "IA32_MCi_MISC MSRs.".

  @param  ECX  MSR_XEON_D_MCi_STATUS
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_MC6_STATUS);
  AsmWriteMsr64 (MSR_XEON_D_MC6_STATUS, Msr);
  @endcode
  @note MSR_XEON_D_MC5_STATUS  is defined as MSR_MC5_STATUS  in SDM.
        MSR_XEON_D_MC6_STATUS  is defined as MSR_MC6_STATUS  in SDM.
        MSR_XEON_D_MC7_STATUS  is defined as MSR_MC7_STATUS  in SDM.
        MSR_XEON_D_MC8_STATUS  is defined as MSR_MC8_STATUS  in SDM.
        MSR_XEON_D_MC9_STATUS  is defined as MSR_MC9_STATUS  in SDM.
        MSR_XEON_D_MC10_STATUS is defined as MSR_MC10_STATUS in SDM.
        MSR_XEON_D_MC11_STATUS is defined as MSR_MC11_STATUS in SDM.
        MSR_XEON_D_MC12_STATUS is defined as MSR_MC12_STATUS in SDM.
        MSR_XEON_D_MC13_STATUS is defined as MSR_MC13_STATUS in SDM.
        MSR_XEON_D_MC14_STATUS is defined as MSR_MC14_STATUS in SDM.
        MSR_XEON_D_MC15_STATUS is defined as MSR_MC15_STATUS in SDM.
        MSR_XEON_D_MC16_STATUS is defined as MSR_MC16_STATUS in SDM.
        MSR_XEON_D_MC17_STATUS is defined as MSR_MC17_STATUS in SDM.
        MSR_XEON_D_MC18_STATUS is defined as MSR_MC18_STATUS in SDM.
        MSR_XEON_D_MC19_STATUS is defined as MSR_MC19_STATUS in SDM.
        MSR_XEON_D_MC20_STATUS is defined as MSR_MC20_STATUS in SDM.
        MSR_XEON_D_MC21_STATUS is defined as MSR_MC21_STATUS in SDM.
  @{
**/
#define MSR_XEON_D_MC5_STATUS                    0x00000415
#define MSR_XEON_D_MC6_STATUS                    0x00000419
#define MSR_XEON_D_MC7_STATUS                    0x0000041D
#define MSR_XEON_D_MC8_STATUS                    0x00000421
#define MSR_XEON_D_MC9_STATUS                    0x00000425
#define MSR_XEON_D_MC10_STATUS                   0x00000429
#define MSR_XEON_D_MC11_STATUS                   0x0000042D
#define MSR_XEON_D_MC12_STATUS                   0x00000431
#define MSR_XEON_D_MC13_STATUS                   0x00000435
#define MSR_XEON_D_MC14_STATUS                   0x00000439
#define MSR_XEON_D_MC15_STATUS                   0x0000043D
#define MSR_XEON_D_MC16_STATUS                   0x00000441
#define MSR_XEON_D_MC17_STATUS                   0x00000445
#define MSR_XEON_D_MC18_STATUS                   0x00000449
#define MSR_XEON_D_MC19_STATUS                   0x0000044D
#define MSR_XEON_D_MC20_STATUS                   0x00000451
#define MSR_XEON_D_MC21_STATUS                   0x00000455
/// @}

/**
  Package. See Section 15.3.2.1, "IA32_MCi_CTL MSRs." through Section
  15.3.2.4, "IA32_MCi_MISC MSRs.".

  @param  ECX  MSR_XEON_D_MCi_ADDR
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_MC6_ADDR);
  AsmWriteMsr64 (MSR_XEON_D_MC6_ADDR, Msr);
  @endcode
  @note MSR_XEON_D_MC5_ADDR  is defined as MSR_MC5_ADDR  in SDM.
        MSR_XEON_D_MC6_ADDR  is defined as MSR_MC6_ADDR  in SDM.
        MSR_XEON_D_MC7_ADDR  is defined as MSR_MC7_ADDR  in SDM.
        MSR_XEON_D_MC8_ADDR  is defined as MSR_MC8_ADDR  in SDM.
        MSR_XEON_D_MC9_ADDR  is defined as MSR_MC9_ADDR  in SDM.
        MSR_XEON_D_MC10_ADDR is defined as MSR_MC10_ADDR in SDM.
        MSR_XEON_D_MC11_ADDR is defined as MSR_MC11_ADDR in SDM.
        MSR_XEON_D_MC12_ADDR is defined as MSR_MC12_ADDR in SDM.
        MSR_XEON_D_MC13_ADDR is defined as MSR_MC13_ADDR in SDM.
        MSR_XEON_D_MC14_ADDR is defined as MSR_MC14_ADDR in SDM.
        MSR_XEON_D_MC15_ADDR is defined as MSR_MC15_ADDR in SDM.
        MSR_XEON_D_MC16_ADDR is defined as MSR_MC16_ADDR in SDM.
        MSR_XEON_D_MC17_ADDR is defined as MSR_MC17_ADDR in SDM.
        MSR_XEON_D_MC18_ADDR is defined as MSR_MC18_ADDR in SDM.
        MSR_XEON_D_MC19_ADDR is defined as MSR_MC19_ADDR in SDM.
        MSR_XEON_D_MC20_ADDR is defined as MSR_MC20_ADDR in SDM.
        MSR_XEON_D_MC21_ADDR is defined as MSR_MC21_ADDR in SDM.
  @{
**/
#define MSR_XEON_D_MC5_ADDR                      0x00000416
#define MSR_XEON_D_MC6_ADDR                      0x0000041A
#define MSR_XEON_D_MC7_ADDR                      0x0000041E
#define MSR_XEON_D_MC8_ADDR                      0x00000422
#define MSR_XEON_D_MC9_ADDR                      0x00000426
#define MSR_XEON_D_MC10_ADDR                     0x0000042A
#define MSR_XEON_D_MC11_ADDR                     0x0000042E
#define MSR_XEON_D_MC12_ADDR                     0x00000432
#define MSR_XEON_D_MC13_ADDR                     0x00000436
#define MSR_XEON_D_MC14_ADDR                     0x0000043A
#define MSR_XEON_D_MC15_ADDR                     0x0000043E
#define MSR_XEON_D_MC16_ADDR                     0x00000442
#define MSR_XEON_D_MC17_ADDR                     0x00000446
#define MSR_XEON_D_MC18_ADDR                     0x0000044A
#define MSR_XEON_D_MC19_ADDR                     0x0000044E
#define MSR_XEON_D_MC20_ADDR                     0x00000452
#define MSR_XEON_D_MC21_ADDR                     0x00000456
/// @}


/**
  Package. See Section 15.3.2.1, "IA32_MCi_CTL MSRs." through Section
  15.3.2.4, "IA32_MCi_MISC MSRs.".

  @param  ECX  MSR_XEON_D_MCi_MISC
  @param  EAX  Lower 32-bits of MSR value.
  @param  EDX  Upper 32-bits of MSR value.

  <b>Example usage</b>
  @code
  UINT64  Msr;

  Msr = AsmReadMsr64 (MSR_XEON_D_MC6_MISC);
  AsmWriteMsr64 (MSR_XEON_D_MC6_MISC, Msr);
  @endcode
  @note MSR_XEON_D_MC5_MISC  is defined as MSR_MC5_MISC  in SDM.
        MSR_XEON_D_MC6_MISC  is defined as MSR_MC6_MISC  in SDM.
        MSR_XEON_D_MC7_MISC  is defined as MSR_MC7_MISC  in SDM.
        MSR_XEON_D_MC8_MISC  is defined as MSR_MC8_MISC  in SDM.
        MSR_XEON_D_MC9_MISC  is defined as MSR_MC9_MISC  in SDM.
        MSR_XEON_D_MC10_MISC is defined as MSR_MC10_MISC in SDM.
        MSR_XEON_D_MC11_MISC is defined as MSR_MC11_MISC in SDM.
        MSR_XEON_D_MC12_MISC is defined as MSR_MC12_MISC in SDM.
        MSR_XEON_D_MC13_MISC is defined as MSR_MC13_MISC in SDM.
        MSR_XEON_D_MC14_MISC is defined as MSR_MC14_MISC in SDM.
        MSR_XEON_D_MC15_MISC is defined as MSR_MC15_MISC in SDM.
        MSR_XEON_D_MC16_MISC is defined as MSR_MC16_MISC in SDM.
        MSR_XEON_D_MC17_MISC is defined as MSR_MC17_MISC in SDM.
        MSR_XEON_D_MC18_MISC is defined as MSR_MC18_MISC in SDM.
        MSR_XEON_D_MC19_MISC is defined as MSR_MC19_MISC in SDM.
        MSR_XEON_D_MC20_MISC is defined as MSR_MC20_MISC in SDM.
        MSR_XEON_D_MC21_MISC is defined as MSR_MC21_MISC in SDM.
  @{
**/
#define MSR_XEON_D_MC5_MISC                      0x00000417
#define MSR_XEON_D_MC6_MISC                      0x0000041B
#define MSR_XEON_D_MC7_MISC                      0x0000041F
#define MSR_XEON_D_MC8_MISC                      0x00000423
#define MSR_XEON_D_MC9_MISC                      0x00000427
#define MSR_XEON_D_MC10_MISC                     0x0000042B
#define MSR_XEON_D_MC11_MISC                     0x0000042F
#define MSR_XEON_D_MC12_MISC                     0x00000433
#define MSR_XEON_D_MC13_MISC                     0x00000437
#define MSR_XEON_D_MC14_MISC                     0x0000043B
#define MSR_XEON_D_MC15_MISC                     0x0000043F
#define MSR_XEON_D_MC16_MISC                     0x00000443
#define MSR_XEON_D_MC17_MISC                     0x00000447
#define MSR_XEON_D_MC18_MISC                     0x0000044B
#define MSR_XEON_D_MC19_MISC                     0x0000044F
#define MSR_XEON_D_MC20_MISC                     0x00000453
#define MSR_XEON_D_MC21_MISC                     0x00000457
/// @}


/**
  Package. Note: C-state values are processor specific C-state code names,
  unrelated to MWAIT extension C-state parameters or ACPI C-States.

  @param  ECX  MSR_XEON_D_PKG_C8_RESIDENCY (0x00000630)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C8_RESIDENCY_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C8_RESIDENCY_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PKG_C8_RESIDENCY_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PKG_C8_RESIDENCY);
  AsmWriteMsr64 (MSR_XEON_D_PKG_C8_RESIDENCY, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PKG_C8_RESIDENCY is defined as MSR_PKG_C8_RESIDENCY in SDM.
**/
#define MSR_XEON_D_PKG_C8_RESIDENCY              0x00000630

/**
  MSR information returned for MSR index #MSR_XEON_D_PKG_C8_RESIDENCY
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 31:0] Package C8 Residency Counter. (R/O) Value since last reset
    /// that this package is in processor-specific C8 states. Count at the
    /// same frequency as the TSC.
    ///
    UINT32  C8ResidencyCounter:32;
    ///
    /// [Bits 59:32] Package C8 Residency Counter. (R/O) Value since last
    /// reset that this package is in processor-specific C8 states. Count at
    /// the same frequency as the TSC.
    ///
    UINT32  C8ResidencyCounterHi:28;
    UINT32  Reserved:4;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PKG_C8_RESIDENCY_REGISTER;


/**
  Package. Note: C-state values are processor specific C-state code names,
  unrelated to MWAIT extension C-state parameters or ACPI C-States.

  @param  ECX  MSR_XEON_D_PKG_C9_RESIDENCY (0x00000631)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C9_RESIDENCY_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C9_RESIDENCY_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PKG_C9_RESIDENCY_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PKG_C9_RESIDENCY);
  AsmWriteMsr64 (MSR_XEON_D_PKG_C9_RESIDENCY, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PKG_C9_RESIDENCY is defined as MSR_PKG_C9_RESIDENCY in SDM.
**/
#define MSR_XEON_D_PKG_C9_RESIDENCY              0x00000631

/**
  MSR information returned for MSR index #MSR_XEON_D_PKG_C9_RESIDENCY
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 31:0] Package C9 Residency Counter. (R/O) Value since last reset
    /// that this package is in processor-specific C9 states. Count at the
    /// same frequency as the TSC.
    ///
    UINT32  C9ResidencyCounter:32;
    ///
    /// [Bits 59:32] Package C9 Residency Counter. (R/O) Value since last
    /// reset that this package is in processor-specific C9 states. Count at
    /// the same frequency as the TSC.
    ///
    UINT32  C9ResidencyCounterHi:28;
    UINT32  Reserved:4;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PKG_C9_RESIDENCY_REGISTER;


/**
  Package. Note: C-state values are processor specific C-state code names,
  unrelated to MWAIT extension C-state parameters or ACPI C-States.

  @param  ECX  MSR_XEON_D_PKG_C10_RESIDENCY (0x00000632)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C10_RESIDENCY_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_PKG_C10_RESIDENCY_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_PKG_C10_RESIDENCY_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_PKG_C10_RESIDENCY);
  AsmWriteMsr64 (MSR_XEON_D_PKG_C10_RESIDENCY, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_PKG_C10_RESIDENCY is defined as MSR_PKG_C10_RESIDENCY in SDM.
**/
#define MSR_XEON_D_PKG_C10_RESIDENCY             0x00000632

/**
  MSR information returned for MSR index #MSR_XEON_D_PKG_C10_RESIDENCY
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bits 31:0] Package C10 Residency Counter. (R/O) Value since last
    /// reset that this package is in processor-specific C10 states. Count at
    /// the same frequency as the TSC.
    ///
    UINT32  C10ResidencyCounter:32;
    ///
    /// [Bits 59:32] Package C10 Residency Counter. (R/O) Value since last
    /// reset that this package is in processor-specific C10 states. Count at
    /// the same frequency as the TSC.
    ///
    UINT32  C10ResidencyCounterHi:28;
    UINT32  Reserved:4;
  } Bits;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_PKG_C10_RESIDENCY_REGISTER;


/**
  Package. Cache Allocation Technology Configuration (R/W).

  @param  ECX  MSR_XEON_D_IA32_L3_QOS_CFG (0x00000C81)
  @param  EAX  Lower 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_L3_QOS_CFG_REGISTER.
  @param  EDX  Upper 32-bits of MSR value.
               Described by the type MSR_XEON_D_IA32_L3_QOS_CFG_REGISTER.

  <b>Example usage</b>
  @code
  MSR_XEON_D_IA32_L3_QOS_CFG_REGISTER  Msr;

  Msr.Uint64 = AsmReadMsr64 (MSR_XEON_D_IA32_L3_QOS_CFG);
  AsmWriteMsr64 (MSR_XEON_D_IA32_L3_QOS_CFG, Msr.Uint64);
  @endcode
  @note MSR_XEON_D_IA32_L3_QOS_CFG is defined as IA32_L3_QOS_CFG in SDM.
**/
#define MSR_XEON_D_IA32_L3_QOS_CFG               0x00000C81

/**
  MSR information returned for MSR index #MSR_XEON_D_IA32_L3_QOS_CFG
**/
typedef union {
  ///
  /// Individual bit fields
  ///
  struct {
    ///
    /// [Bit 0] CAT Enable. Set 1 to enable Cache Allocation Technology.
    ///
    UINT32  CAT:1;
    UINT32  Reserved1:31;
    UINT32  Reserved2:32;
  } Bits;
  ///
  /// All bit fields as a 32-bit value
  ///
  UINT32  Uint32;
  ///
  /// All bit fields as a 64-bit value
  ///
  UINT64  Uint64;
} MSR_XEON_D_IA32_L3_QOS_CFG_REGISTER;

#endif
