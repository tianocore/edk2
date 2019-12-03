/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CACHE_LIB_INTERNAL_H_
#define _CACHE_LIB_INTERNAL_H_

#define EFI_MSR_CACHE_VARIABLE_MTRR_BASE       0x00000200
#define EFI_MSR_CACHE_VARIABLE_MTRR_END        0x0000020F
#define   V_EFI_FIXED_MTRR_NUMBER                                      11

#define EFI_MSR_IA32_MTRR_FIX64K_00000         0x00000250
#define EFI_MSR_IA32_MTRR_FIX16K_80000         0x00000258
#define EFI_MSR_IA32_MTRR_FIX16K_A0000         0x00000259
#define EFI_MSR_IA32_MTRR_FIX4K_C0000          0x00000268
#define EFI_MSR_IA32_MTRR_FIX4K_C8000          0x00000269
#define EFI_MSR_IA32_MTRR_FIX4K_D0000          0x0000026A
#define EFI_MSR_IA32_MTRR_FIX4K_D8000          0x0000026B
#define EFI_MSR_IA32_MTRR_FIX4K_E0000          0x0000026C
#define EFI_MSR_IA32_MTRR_FIX4K_E8000          0x0000026D
#define EFI_MSR_IA32_MTRR_FIX4K_F0000          0x0000026E
#define EFI_MSR_IA32_MTRR_FIX4K_F8000          0x0000026F
#define EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE       0x000002FF
#define   B_EFI_MSR_CACHE_MTRR_VALID                                   BIT11
#define   B_EFI_MSR_GLOBAL_MTRR_ENABLE                                 BIT11
#define   B_EFI_MSR_FIXED_MTRR_ENABLE                                  BIT10
#define   B_EFI_MSR_CACHE_MEMORY_TYPE                                  (BIT2 | BIT1 | BIT0)

#define EFI_MSR_VALID_MASK                     0xFFFFFFFFF
#define EFI_CACHE_VALID_ADDRESS                0xFFFFFF000
#define EFI_SMRR_CACHE_VALID_ADDRESS           0xFFFFF000
#define EFI_CACHE_VALID_EXTENDED_ADDRESS       0xFFFFFFFFFF000

// Leave one MTRR pairs for OS use
#define EFI_CACHE_NUM_VAR_MTRR_PAIRS_FOR_OS   1
#define EFI_CACHE_LAST_VARIABLE_MTRR_FOR_BIOS (EFI_MSR_CACHE_VARIABLE_MTRR_END) - \
        (EFI_CACHE_NUM_VAR_MTRR_PAIRS_FOR_OS * 2)

#define EFI_MSR_IA32_MTRR_CAP                  0x000000FE
#define   B_EFI_MSR_IA32_MTRR_CAP_EMRR_SUPPORT                         BIT12
#define   B_EFI_MSR_IA32_MTRR_CAP_SMRR_SUPPORT                         BIT11
#define   B_EFI_MSR_IA32_MTRR_CAP_WC_SUPPORT                           BIT10
#define   B_EFI_MSR_IA32_MTRR_CAP_FIXED_SUPPORT                        BIT8
#define   B_EFI_MSR_IA32_MTRR_CAP_VARIABLE_SUPPORT                     (BIT7 | BIT6 | BIT5 | BIT4 | BIT3 | BIT2 | BIT1 | BIT0)

#define CPUID_VIR_PHY_ADDRESS_SIZE                                    0x80000008
#define CPUID_EXTENDED_FUNCTION                                       0x80000000

#endif

