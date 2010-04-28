/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  CpuIA32.h

Abstract:

--*/

#ifndef _CPU_IA32_H
#define _CPU_IA32_H

#include "Tiano.h"

typedef struct {
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;
} EFI_CPUID_REGISTER;

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

typedef struct {
  UINT32  Stepping       : 4;
  UINT32  Model          : 4;
  UINT32  Family         : 4;
  UINT32  Type           : 2;
  UINT32  Reserved1      : 2;
  UINT32  ExtendedModel  : 4;
  UINT32  ExtendedFamily : 8;
  UINT32  Reserved2      : 4;
} EFI_CPU_VERSION;

#define EFI_CPUID_SIGNATURE                   0x0
#define EFI_CPUID_VERSION_INFO                0x1
#define EFI_CPUID_CACHE_INFO                  0x2
#define EFI_CPUID_SERIAL_NUMBER               0x3
#define EFI_CPUID_EXTENDED_FUNCTION           0x80000000
#define EFI_CPUID_EXTENDED_CPU_SIG            0x80000001
#define EFI_CPUID_BRAND_STRING1               0x80000002
#define EFI_CPUID_BRAND_STRING2               0x80000003
#define EFI_CPUID_BRAND_STRING3               0x80000004

#define EFI_MSR_IA32_PLATFORM_ID              0x17
#define EFI_MSR_IA32_APIC_BASE                0x1B
#define EFI_MSR_EBC_HARD_POWERON              0x2A
#define EFI_MSR_EBC_SOFT_POWERON              0x2B
#define BINIT_DRIVER_DISABLE                  0x40
#define INTERNAL_MCERR_DISABLE                0x20
#define INITIATOR_MCERR_DISABLE               0x10
#define EFI_MSR_EBC_FREQUENCY_ID              0x2C
#define EFI_MSR_IA32_BIOS_UPDT_TRIG           0x79
#define EFI_MSR_IA32_BIOS_SIGN_ID             0x8B
#define EFI_MSR_PSB_CLOCK_STATUS              0xCD
#define EFI_APIC_GLOBAL_ENABLE                0x800
#define EFI_MSR_IA32_MISC_ENABLE              0x1A0
#define LIMIT_CPUID_MAXVAL_ENABLE_BIT         0x00400000
#define AUTOMATIC_THERMAL_CONTROL_ENABLE_BIT  0x00000008
#define COMPATIBLE_FPU_OPCODE_ENABLE_BIT      0x00000004
#define LOGICAL_PROCESSOR_PRIORITY_ENABLE_BIT 0x00000002
#define FAST_STRING_ENABLE_BIT                0x00000001

#define EFI_CACHE_VARIABLE_MTRR_BASE          0x200
#define EFI_CACHE_VARIABLE_MTRR_END           0x20F
#define EFI_CACHE_IA32_MTRR_DEF_TYPE          0x2FF
#define EFI_CACHE_MTRR_VALID                  0x800
#define EFI_CACHE_FIXED_MTRR_VALID            0x400
#define EFI_CACHE_VALID_ADDRESS               0xFFFFFF000
#define EFI_MSR_VALID_MASK                    0xFFFFFFFFF
#define EFI_CACHE_VALID_EXTENDED_ADDRESS      0xFFFFFFFFFF000
#define EFI_MSR_VALID_EXTENDED_MASK           0xFFFFFFFFFFFFF

#define EFI_IA32_MTRR_FIX64K_00000            0x250
#define EFI_IA32_MTRR_FIX16K_80000            0x258
#define EFI_IA32_MTRR_FIX16K_A0000            0x259
#define EFI_IA32_MTRR_FIX4K_C0000             0x268
#define EFI_IA32_MTRR_FIX4K_C8000             0x269
#define EFI_IA32_MTRR_FIX4K_D0000             0x26A
#define EFI_IA32_MTRR_FIX4K_D8000             0x26B
#define EFI_IA32_MTRR_FIX4K_E0000             0x26C
#define EFI_IA32_MTRR_FIX4K_E8000             0x26D
#define EFI_IA32_MTRR_FIX4K_F0000             0x26E
#define EFI_IA32_MTRR_FIX4K_F8000             0x26F

#define EFI_IA32_MCG_CAP                      0x179
#define EFI_IA32_MCG_CTL                      0x17B
#define EFI_IA32_MC0_CTL                      0x400
#define EFI_IA32_MC0_STATUS                   0x401

#define EFI_IA32_PERF_STATUS                  0x198
#define EFI_IA32_PERF_CTL                     0x199

#define EFI_CACHE_UNCACHEABLE                 0
#define EFI_CACHE_WRITECOMBINING              1
#define EFI_CACHE_WRITETHROUGH                4
#define EFI_CACHE_WRITEPROTECTED              5
#define EFI_CACHE_WRITEBACK                   6

//
// Combine f(FamilyId), m(Model), s(SteppingId) to a single 32 bit number
//
#define EfiMakeCpuVersion(f, m, s)         \
  (((UINT32) (f) << 16) | ((UINT32) (m) << 8) | ((UINT32) (s)))

VOID
EFIAPI
EfiHalt (
  VOID
  );

/*++                                                                                                                               
Routine Description:                                                
  Halt the Cpu    
Arguments:                
   None                                                          
Returns:                                                            
   None                                                
--*/
VOID
EFIAPI
EfiWbinvd (
  VOID
  );

/*++                                                                                                                               
Routine Description:                                                
  Write back and invalidate the Cpu cache
Arguments:                
   None                                                          
Returns:                                                            
   None                                                
--*/
VOID
EFIAPI
EfiInvd (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Invalidate the Cpu cache
Arguments:                
   None                                                          
Returns:                                                            
   None                                                
--*/
VOID
EFIAPI
EfiCpuid (
  IN  UINT32                 RegisterInEax,
  OUT EFI_CPUID_REGISTER     *Regs
  );

/*++                                                                                                                          
Routine Description:                                                
  Get the Cpu info by excute the CPUID instruction
Arguments:                
   RegisterInEax: -The input value to put into register EAX
   Regs:          -The Output value                      
Returns:                                                            
   None                                                
--*/

VOID
EFIAPI
EfiCpuidExt (
  IN  UINT32                 RegisterInEax,
  IN  UINT32                 CacheLevel,
  OUT EFI_CPUID_REGISTER     *Regs
  )
/*++                                                                                                                          
Routine Description:                                                
  When RegisterInEax != 4, the functionality is the same as EfiCpuid.
  When RegisterInEax == 4, the function return the deterministic cache
  parameters by excuting the CPUID instruction
Arguments:                
   RegisterInEax: - The input value to put into register EAX
   CacheLevel:      - The deterministic cache level
   Regs:          - The Output value                      
Returns:                                                            
   None                                                
--*/
;

UINT64
EFIAPI
EfiReadMsr (
  IN UINT32     Index
  );

/*++                                                                                                                          
Routine Description:                                                
  Read Cpu MSR
Arguments:                
  Index: -The index value to select the register
                       
Returns:                                                            
   Return the read data                                                
--*/
VOID
EFIAPI
EfiWriteMsr (
  IN UINT32     Index,
  IN UINT64     Value
  );

/*++                                                                                                                          
Routine Description:                                                
  Write Cpu MSR
Arguments:                
  Index: -The index value to select the register
  Value: -The value to write to the selected register                      
Returns:                                                            
   None                                                
--*/
UINT64
EFIAPI
EfiReadTsc (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Read Time stamp
Arguments:                
  None                 
Returns:                                                            
   Return the read data                                                
--*/
VOID
EFIAPI
EfiDisableCache (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Writing back and invalidate the cache,then diable it
Arguments:                
  None                 
Returns:                                                            
  None                                               
--*/
VOID
EFIAPI
EfiEnableCache (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Invalidate the cache,then Enable it
Arguments:                
  None                 
Returns:                                                            
  None                                               
--*/
UINT32
EFIAPI
EfiGetEflags (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Get Eflags
Arguments:                
  None                 
Returns:                                                            
  Return the Eflags value                                               
--*/
VOID
EFIAPI
EfiDisableInterrupts (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Disable Interrupts
Arguments:                
  None                 
Returns:                                                            
  None
--*/
VOID
EFIAPI
EfiEnableInterrupts (
  VOID
  );

/*++                                                                                                                          
Routine Description:                                                
  Enable Interrupts
Arguments:                
  None                 
Returns:                                                            
  None                                               
--*/


VOID
EFIAPI
EfiCpuVersion (
  IN   UINT16  *FamilyId,    OPTIONAL
  IN   UINT8   *Model,       OPTIONAL
  IN   UINT8   *SteppingId,  OPTIONAL
  IN   UINT8   *Processor    OPTIONAL
  )
/*++

Routine Description:
  Extract CPU detail version infomation

Arguments:
  FamilyId   - FamilyId, including ExtendedFamilyId
  Model      - Model, including ExtendedModel
  SteppingId - SteppingId
  Processor  - Processor

--*/
;

#endif
