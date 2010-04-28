// ++

// TODO: fix comment to start with /*++
//
// Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
// Module Name:
//
//  IpfDefines.h
//
// Abstract:
//
//  IPF Processor Defines.
//  NOTE: This file is included by assembly files as well.
//
// --
//
#ifndef _IPFDEFINES_H
#define _IPFDEFINES_H

//
//  IPI DElivery Methods
//
#define IPI_INT_DELIVERY    0x0
#define IPI_PMI_DELIVERY    0x2
#define IPI_NMI_DELIVERY    0x4
#define IPI_INIT_DELIVERY   0x5
#define IPI_ExtINT_DELIVERY 0x7

//
// Define Itanium-based system registers.
//
// Define Itanium-based system register bit field offsets.
//
// Processor Status Register (PSR) Bit positions
//
// User / System mask
//
#define PSR_RV0 0
#define PSR_BE  1
#define PSR_UP  2
#define PSR_AC  3
#define PSR_MFL 4
#define PSR_MFH 5

//
// PSR bits 6-12 reserved (must be zero)
//
#define PSR_MBZ0    6
#define PSR_MBZ0_V  0x1ffUL L

//
// System only mask
//
#define PSR_IC      13
#define PSR_IC_MASK (1 << 13)
#define PSR_I       14
#define PSR_PK      15
#define PSR_MBZ1    16
#define PSR_MBZ1_V  0x1UL L
#define PSR_DT      17
#define PSR_DFL     18
#define PSR_DFH     19
#define PSR_SP      20
#define PSR_PP      21
#define PSR_DI      22
#define PSR_SI      23
#define PSR_DB      24
#define PSR_LP      25
#define PSR_TB      26
#define PSR_RT      27

//
// PSR bits 28-31 reserved (must be zero)
//
#define PSR_MBZ2    28
#define PSR_MBZ2_V  0xfUL L

//
// Neither mask
//
#define PSR_CPL     32
#define PSR_CPL_LEN 2
#define PSR_IS      34
#define PSR_MC      35
#define PSR_IT      36
#define PSR_IT_MASK 0x1000000000
#define PSR_ID      37
#define PSR_DA      38
#define PSR_DD      39
#define PSR_SS      40
#define PSR_RI      41
#define PSR_RI_LEN  2
#define PSR_ED      43
#define PSR_BN      44

//
// PSR bits 45-63 reserved (must be zero)
//
#define PSR_MBZ3    45
#define PSR_MBZ3_V  0xfffffUL L

//
// Floating Point Status Register (FPSR) Bit positions
//
//
// Traps
//
#define FPSR_VD 0
#define FPSR_DD 1
#define FPSR_ZD 2
#define FPSR_OD 3
#define FPSR_UD 4
#define FPSR_ID 5

//
// Status Field 0 - Controls
//
#define FPSR0_FTZ0  6
#define FPSR0_WRE0  7
#define FPSR0_PC0   8
#define FPSR0_RC0   10
#define FPSR0_TD0   12

//
// Status Field 0 - Flags
//
#define FPSR0_V0  13
#define FPSR0_D0  14
#define FPSR0_Z0  15
#define FPSR0_O0  16
#define FPSR0_U0  17
#define FPSR0_I0  18

//
// Status Field 1 - Controls
//
#define FPSR1_FTZ0  19
#define FPSR1_WRE0  20
#define FPSR1_PC0   21
#define FPSR1_RC0   23
#define FPSR1_TD0   25

//
// Status Field 1 - Flags
//
#define FPSR1_V0  26
#define FPSR1_D0  27
#define FPSR1_Z0  28
#define FPSR1_O0  29
#define FPSR1_U0  30
#define FPSR1_I0  31

//
// Status Field 2 - Controls
//
#define FPSR2_FTZ0  32
#define FPSR2_WRE0  33
#define FPSR2_PC0   34
#define FPSR2_RC0   36
#define FPSR2_TD0   38

//
// Status Field 2 - Flags
//
#define FPSR2_V0  39
#define FPSR2_D0  40
#define FPSR2_Z0  41
#define FPSR2_O0  42
#define FPSR2_U0  43
#define FPSR2_I0  44

//
// Status Field 3 - Controls
//
#define FPSR3_FTZ0  45
#define FPSR3_WRE0  46
#define FPSR3_PC0   47
#define FPSR3_RC0   49
#define FPSR3_TD0   51

//
// Status Field 0 - Flags
//
#define FPSR3_V0  52
#define FPSR3_D0  53
#define FPSR3_Z0  54
#define FPSR3_O0  55
#define FPSR3_U0  56
#define FPSR3_I0  57

//
// FPSR bits 58-63 Reserved -- Must be zero
//
#define FPSR_MBZ0   58
#define FPSR_MBZ0_V 0x3fUL L

//
// For setting up FPSR on kernel entry
// All traps are disabled.
//
#define FPSR_FOR_KERNEL     0x3f

#define FP_REG_SIZE         16  // 16 byte spill size
#define HIGHFP_REGS_LENGTH  (96 * 16)

//
// Define hardware Task Priority Register (TPR)
//
//
// TPR bit positions
//
#define TPR_MIC     4   // Bits 0 - 3 ignored
#define TPR_MIC_LEN 4
#define TPR_MMI     16  // Mask Maskable Interrupt
//
// Define hardware Interrupt Status Register (ISR)
//
//
// ISR bit positions
//
#define ISR_CODE          0
#define ISR_CODE_LEN      16
#define ISR_CODE_MASK     0xFFFF
#define ISR_IA_VECTOR     16
#define ISR_IA_VECTOR_LEN 8
#define ISR_MBZ0          24
#define ISR_MBZ0_V        0xff
#define ISR_X             32
#define ISR_W             33
#define ISR_R             34
#define ISR_NA            35
#define ISR_SP            36
#define ISR_RS            37
#define ISR_IR            38
#define ISR_NI            39
#define ISR_MBZ1          40
#define ISR_EI            41
#define ISR_ED            43
#define ISR_MBZ2          44
#define ISR_MBZ2_V        0xfffff

//
// ISR codes
//
// For General exceptions: ISR{3:0}
//
#define ISR_ILLEGAL_OP  0 //  Illegal operation fault
#define ISR_PRIV_OP     1 //  Privileged operation fault
#define ISR_PRIV_REG    2 //  Privileged register fauls
#define ISR_RESVD_REG   3 //  Reserved register/field flt
#define ISR_ILLEGAL_ISA 4 // Disabled instruction set transition fault
//
// Define hardware Default Control Register (DCR)
//
//
// DCR bit positions
//
#define DCR_PP        0
#define DCR_BE        1
#define DCR_LC        2
#define DCR_MBZ0      4
#define DCR_MBZ0_V    0xf
#define DCR_DM        8
#define DCR_DP        9
#define DCR_DK        10
#define DCR_DX        11
#define DCR_DR        12
#define DCR_DA        13
#define DCR_DD        14
#define DCR_DEFER_ALL 0x7f00
#define DCR_MBZ1      2
#define DCR_MBZ1_V    0xffffffffffffUL L

//
// Define hardware RSE Configuration Register
//
// RS Configuration (RSC) bit field positions
//
#define RSC_MODE        0
#define RSC_PL          2
#define RSC_BE          4
#define RSC_MBZ0        5
#define RSC_MBZ0_V      0x3ff
#define RSC_LOADRS      16
#define RSC_LOADRS_LEN  14
#define RSC_MBZ1        30
#define RSC_MBZ1_V      0x3ffffffffUL L

//
// RSC modes
//
#define RSC_MODE_LY (0x0) // Lazy
#define RSC_MODE_SI (0x1) // Store intensive
#define RSC_MODE_LI (0x2) // Load intensive
#define RSC_MODE_EA (0x3) // Eager
//
// RSC Endian bit values
//
#define RSC_BE_LITTLE 0
#define RSC_BE_BIG    1

//
// Define Interruption Function State (IFS) Register
//
// IFS bit field positions
//
#define IFS_IFM     0
#define IFS_IFM_LEN 38
#define IFS_MBZ0    38
#define IFS_MBZ0_V  0x1ffffff
#define IFS_V       63
#define IFS_V_LEN   1

//
// IFS is valid when IFS_V = IFS_VALID
//
#define IFS_VALID 1

//
// Define Page Table Address (PTA)
//
#define PTA_VE        0
#define PTA_VF        8
#define PTA_SIZE      2
#define PTA_SIZE_LEN  6
#define PTA_BASE      15

//
// Define Region Register (RR)
//
//
// RR bit field positions
//
#define RR_VE       0
#define RR_MBZ0     1
#define RR_PS       2
#define RR_PS_LEN   6
#define RR_RID      8
#define RR_RID_LEN  24
#define RR_MBZ1     32

//
// SAL uses region register 0 and RID of 1000
//
#define SAL_RID     0x1000
#define SAL_RR_REG  0x0
#define SAL_TR      0x0

//
// Total number of region registers
//
#define RR_SIZE 8

//
// Define Protection Key Register (PKR)
//
// PKR bit field positions
//
#define PKR_V       0
#define PKR_WD      1
#define PKR_RD      2
#define PKR_XD      3
#define PKR_MBZ0    4
#define PKR_KEY     8
#define PKR_KEY_LEN 24
#define PKR_MBZ1    32

#define PKR_VALID   (1 << PKR_V)

//
// Number of protection key registers
//
#define PKRNUM  8

//
// Define Interruption TLB Insertion register (ITIR)
//
//
// Define Translation Insertion Format (TR)
//
// PTE0 bit field positions
//
#define PTE0_P    0
#define PTE0_MBZ0 1
#define PTE0_MA   2
#define PTE0_A    5
#define PTE0_D    6
#define PTE0_PL   7
#define PTE0_AR   9
#define PTE0_PPN  12
#define PTE0_MBZ1 48
#define PTE0_ED   52
#define PTE0_IGN0 53

//
// ITIR bit field positions
//
#define ITIR_MBZ0     0
#define ITIR_PS       2
#define ITIR_PS_LEN   6
#define ITIR_KEY      8
#define ITIR_KEY_LEN  24
#define ITIR_MBZ1     32
#define ITIR_MBZ1_LEN 16
#define ITIR_PPN      48
#define ITIR_PPN_LEN  15
#define ITIR_MBZ2     63

#define ATTR_IPAGE    0x661 // Access Rights = RWX (bits 11-9=011), PL 0(8-7=0)
#define ATTR_DEF_BITS 0x661 // Access Rights = RWX (bits 11-9=010), PL 0(8-7=0)
// Dirty (bit 6=1), Accessed (bit 5=1),
// MA WB (bits 4-2=000), Present (bit 0=1)
//
// Memory access rights
//
#define AR_UR_KR      0x0 // user/kernel read
#define AR_URX_KRX    0x1 // user/kernel read and execute
#define AR_URW_KRW    0x2 // user/kernel read & write
#define AR_URWX_KRWX  0x3 // user/kernel read,write&execute
#define AR_UR_KRW     0x4 // user read/kernel read,write
#define AR_URX_KRWX   0x5 // user read/execute, kernel all
#define AR_URWX_KRW   0x6 // user all, kernel read & write
#define AR_UX_KRX     0x7 // user execute only, kernel read and execute
//
// Memory attribute values
//
//
// The next 4 are all cached, non-sequential & speculative, coherent
//
#define MA_WBU  0x0 // Write back, unordered
//
// The next 3 are all non-cached, sequential & non-speculative
//
#define MA_UC   0x4 // Non-coalescing, sequential & non-speculative
#define MA_UCE  0x5 // Non-coalescing, sequential, non-speculative
// & fetchadd exported
//
#define MA_WC   0x6 // Non-cached, Coalescing,  non-seq., spec.
#define MA_NAT  0xf // NaT page
//
// Definition of the offset of TRAP/INTERRUPT/FAULT handlers from the
// base of IVA (Interruption Vector Address)
//
#define IVT_SIZE          0x8000
#define EXTRA_ALIGNMENT   0x1000

#define OFF_VHPTFLT       0x0000  // VHPT Translation fault
#define OFF_ITLBFLT       0x0400  // Instruction TLB fault
#define OFF_DTLBFLT       0x0800  // Data TLB fault
#define OFF_ALTITLBFLT    0x0C00  // Alternate ITLB fault
#define OFF_ALTDTLBFLT    0x1000  // Alternate DTLB fault
#define OFF_NESTEDTLBFLT  0x1400  // Nested TLB fault
#define OFF_IKEYMISSFLT   0x1800  // Inst Key Miss fault
#define OFF_DKEYMISSFLT   0x1C00  // Data Key Miss fault
#define OFF_DIRTYBITFLT   0x2000  // Dirty-Bit fault
#define OFF_IACCESSBITFLT 0x2400  // Inst Access-Bit fault
#define OFF_DACCESSBITFLT 0x2800  // Data Access-Bit fault
#define OFF_BREAKFLT      0x2C00  // Break Inst fault
#define OFF_EXTINT        0x3000  // External Interrupt
//
//  Offset 0x3400 to 0x0x4C00 are reserved
//
#define OFF_PAGENOTPFLT   0x5000  // Page Not Present fault
#define OFF_KEYPERMFLT    0x5100  // Key Permission fault
#define OFF_IACCESSRTFLT  0x5200  // Inst Access-Rights flt
#define OFF_DACCESSRTFLT  0x5300  // Data Access-Rights fault
#define OFF_GPFLT         0x5400  // General Exception fault
#define OFF_FPDISFLT      0x5500  // Disable-FP fault
#define OFF_NATFLT        0x5600  // NAT Consumption fault
#define OFF_SPECLNFLT     0x5700  // Speculation fault
#define OFF_DBGFLT        0x5900  // Debug fault
#define OFF_ALIGNFLT      0x5A00  // Unaligned Reference fault
#define OFF_LOCKDREFFLT   0x5B00  // Locked Data Reference fault
#define OFF_FPFLT         0x5C00  // Floating Point fault
#define OFF_FPTRAP        0x5D00  // Floating Point Trap
#define OFF_LOPRIVTRAP    0x5E00  // Lower-Privilege Transfer Trap
#define OFF_TAKENBRTRAP   0x5F00  // Taken Branch Trap
#define OFF_SSTEPTRAP     0x6000  // Single Step Trap
//
// Offset 0x6100 to 0x6800 are reserved
//
#define OFF_IA32EXCEPTN   0x6900  // iA32 Exception
#define OFF_IA32INTERCEPT 0x6A00  // iA32 Intercept
#define OFF_IA32INT       0x6B00  // iA32 Interrupt
#define NUMBER_OF_VECTORS 0x100
//
// Privilege levels
//
#define PL_KERNEL 0
#define PL_USER   3

//
// Instruction set (IS) bits
//
#define IS_IA64 0
#define IS_IA   1

//
// RSC while in kernel: enabled, little endian, PL = 0, eager mode
//
#define RSC_KERNEL  ((RSC_MODE_EA << RSC_MODE) | (RSC_BE_LITTLE << RSC_BE))

//
// Lazy RSC in kernel: enabled, little endian, pl = 0, lazy mode
//
#define RSC_KERNEL_LAZ  ((RSC_MODE_LY << RSC_MODE) | (RSC_BE_LITTLE << RSC_BE))

//
// RSE disabled: disabled, PL = 0, little endian, eager mode
//
#define RSC_KERNEL_DISABLED   ((RSC_MODE_LY << RSC_MODE) | (RSC_BE_LITTLE << RSC_BE))

#define NAT_BITS_PER_RNAT_REG 63

//
// Macros for generating PTE0 and PTE1 value
//
#define PTE0(ed, ppn12_47, ar, pl, d, a, ma, p) \
                ( ( ed << PTE0_ED )               |  \
                  ( ppn12_47 << PTE0_PPN )        |  \
                  ( ar << PTE0_AR )               |  \
                  ( pl << PTE0_PL )               |  \
                  ( d << PTE0_D )                 |  \
                  ( a << PTE0_A )                 |  \
                  ( ma << PTE0_MA )               |  \
                  ( p << PTE0_P )                    \
                )

#define ITIR(ppn48_63, key, ps)            \
                ( ( ps << ITIR_PS )       |  \
                  ( key << ITIR_KEY )     |  \
                  ( ppn48_63 << ITIR_PPN )         \
    )

//
// Macro to generate mask value from bit position. The result is a
// 64-bit.
//
#define BITMASK(bp, value)      (value << bp)

#define BUNDLE_SIZE             16
#define SPURIOUS_INT            0xF

#define FAST_DISABLE_INTERRUPTS rsm BITMASK (PSR_I, 1);;

#define FAST_ENABLE_INTERRUPTS  ssm BITMASK (PSR_I, 1);;

#endif
