/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuFuncs.h

Abstract:

--*/

#ifndef _CPU_FUNCS_H
#define _CPU_FUNCS_H

#define EFI_CPUID_SIGNATURE           0x0
#define EFI_CPUID_VERSION_INFO        0x1
#define EFI_CPUID_CACHE_INFO          0x2
#define EFI_CPUID_SERIAL_NUMBER       0x3
#define EFI_CPUID_EXTENDED_FUNCTION   0x80000000
#define EFI_CPUID_EXTENDED_CPU_SIG    0x80000001
#define EFI_CPUID_BRAND_STRING1       0x80000002
#define EFI_CPUID_BRAND_STRING2       0x80000003
#define EFI_CPUID_BRAND_STRING3       0x80000004

#define EFI_MSR_IA32_APIC_BASE        0x1B
#define EFI_MSR_EBC_HARD_POWERON      0x2A
#define EFI_MSR_EBC_SOFT_POWERON      0x2B
#define EFI_MSR_EBC_FREQUENCY_ID      0x2C
#define EFI_MSR_IA32_BIOS_UPDT_TRIG   0x79
#define EFI_MSR_IA32_BIOS_SIGN_ID     0x8B
#define EFI_APIC_GLOBAL_ENABLE        0x800

#define EFI_CACHE_VARIABLE_MTRR_BASE  0x200
#define EFI_CACHE_VARIABLE_MTRR_END   0x20F
#define EFI_CACHE_IA32_MTRR_DEF_TYPE  0x2FF
#define EFI_CACHE_VALID_ADDRESS       0xFFFFFF000
#define EFI_CACHE_MTRR_VALID          0x800
#define EFI_CACHE_FIXED_MTRR_VALID    0x400
#define EFI_MSR_VALID_MASK            0xFFFFFFFFF

#define EFI_IA32_MTRR_FIX64K_00000    0x250
#define EFI_IA32_MTRR_FIX16K_80000    0x258
#define EFI_IA32_MTRR_FIX16K_A0000    0x259
#define EFI_IA32_MTRR_FIX4K_C0000     0x268
#define EFI_IA32_MTRR_FIX4K_C8000     0x269
#define EFI_IA32_MTRR_FIX4K_D0000     0x26A
#define EFI_IA32_MTRR_FIX4K_D8000     0x26B
#define EFI_IA32_MTRR_FIX4K_E0000     0x26C
#define EFI_IA32_MTRR_FIX4K_E8000     0x26D
#define EFI_IA32_MTRR_FIX4K_F0000     0x26E
#define EFI_IA32_MTRR_FIX4K_F8000     0x26F

#define EFI_IA32_MCG_CAP              0x179
#define EFI_IA32_MCG_CTL              0x17B
#define EFI_IA32_MC0_CTL              0x400
#define EFI_IA32_MC0_STATUS           0x401

#define EFI_CACHE_UNCACHEABLE         0
#define EFI_CACHE_WRITECOMBINING      1
#define EFI_CACHE_WRITETHROUGH        4
#define EFI_CACHE_WRITEPROTECTED      5
#define EFI_CACHE_WRITEBACK           6

UINT64
EfiReadTsc (
  VOID
  )
/*++                   
                                                                                                       
Routine Description:                                                

  Read Time stamp.
  
Arguments:                

  None                 
  
Returns:                                                            

   Return the read data                                                
   
--*/
;

#endif
