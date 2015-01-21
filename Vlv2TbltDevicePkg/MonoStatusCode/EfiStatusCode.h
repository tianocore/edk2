/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:

  EfiStatusCode.h

Abstract:

  Status Code Definitions, according to Intel Platform Innovation Framework
  for EFI Status Codes Specification
  Revision 0.92

  The file is divided into sections for ease of use.

  Section:    Contents:
    1           General Status Code Definitions
    2           Class definitions
    3           Computing Unit Subclasses, Progress and Error Codes
    4           Peripheral Subclasses, Progress and Error Codes.
    5           IO Bus Subclasses, Progress and Error Codes.
    6           Software Subclasses, Progress and Error Codes.
    7           Debug Codes

--*/

#ifndef _EFI_STATUS_CODE_H_
#define _EFI_STATUS_CODE_H_



#define EFI_SW_PEIM_EC_NO_RECOVERY_CAPSULE        (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_SW_PEIM_EC_INVALID_CAPSULE_DESCRIPTOR (EFI_SUBCLASS_SPECIFIC | 0x00000001)



#define EFI_SW_PEIM_PC_RECOVERY_BEGIN (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_SW_PEIM_PC_CAPSULE_LOAD   (EFI_SUBCLASS_SPECIFIC | 0x00000001)
#define EFI_SW_PEIM_PC_CAPSULE_START  (EFI_SUBCLASS_SPECIFIC | 0x00000002)
#define EFI_SW_PEIM_PC_RECOVERY_USER  (EFI_SUBCLASS_SPECIFIC | 0x00000003)
#define EFI_SW_PEIM_PC_RECOVERY_AUTO  (EFI_SUBCLASS_SPECIFIC | 0x00000004)



#define EFI_CU_MEMORY_PC_SPD_READ         (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_CU_MEMORY_PC_PRESENCE_DETECT  (EFI_SUBCLASS_SPECIFIC | 0x00000001)
#define EFI_CU_MEMORY_PC_TIMING           (EFI_SUBCLASS_SPECIFIC | 0x00000002)
#define EFI_CU_MEMORY_PC_CONFIGURING      (EFI_SUBCLASS_SPECIFIC | 0x00000003)
#define EFI_CU_MEMORY_PC_OPTIMIZING       (EFI_SUBCLASS_SPECIFIC | 0x00000004)
#define EFI_CU_MEMORY_PC_INIT             (EFI_SUBCLASS_SPECIFIC | 0x00000005)
#define EFI_CU_MEMORY_PC_TEST             (EFI_SUBCLASS_SPECIFIC | 0x00000006)
#define EFI_CU_MEMORY_PC_COMPLETE         (EFI_SUBCLASS_SPECIFIC | 0x00000007)
#define EFI_CU_MEMORY_PC_INIT_BEGIN       (EFI_SUBCLASS_SPECIFIC | 0x00000008)


#define EFI_DC_UNSPECIFIED  0x0

//
// CPU PEI
//
#define EFI_CU_HP_PC_PEI_INIT                     (EFI_SUBCLASS_SPECIFIC | 0x00000010)
#define EFI_CU_HP_PC_PEI_STEP1                    (EFI_SUBCLASS_SPECIFIC | 0x00000011)
#define EFI_CU_HP_PC_PEI_STEP2                    (EFI_SUBCLASS_SPECIFIC | 0x00000012)
#define EFI_CU_HP_PC_PEI_STEP3                    (EFI_SUBCLASS_SPECIFIC | 0x00000013)
#define EFI_CU_HP_PC_PEI_STEP4                    (EFI_SUBCLASS_SPECIFIC | 0x00000014)
#define EFI_CU_HP_PC_PEI_STEP5                    (EFI_SUBCLASS_SPECIFIC | 0x00000015)
#define EFI_CU_HP_PC_PEI_STEP6                    (EFI_SUBCLASS_SPECIFIC | 0x00000016)
#define EFI_CU_HP_PC_PEI_STEP7                    (EFI_SUBCLASS_SPECIFIC | 0x00000017)
#define EFI_CU_HP_PC_PEI_STEP8                    (EFI_SUBCLASS_SPECIFIC | 0x00000018)
#define EFI_CU_HP_PC_PEI_STEP9                    (EFI_SUBCLASS_SPECIFIC | 0x00000019)
#define EFI_CU_HP_PC_PEI_STEP10                   (EFI_SUBCLASS_SPECIFIC | 0x0000001A)
#define EFI_CU_HP_PC_PEI_STEP11                   (EFI_SUBCLASS_SPECIFIC | 0x0000001B)
#define EFI_CU_HP_PC_PEI_STEP12                   (EFI_SUBCLASS_SPECIFIC | 0x0000001C)
#define EFI_CU_HP_PC_PEI_STEP13                   (EFI_SUBCLASS_SPECIFIC | 0x0000001D)
#define EFI_CU_HP_PC_PEI_STEP14                   (EFI_SUBCLASS_SPECIFIC | 0x0000001E)
#define EFI_CU_HP_PC_PEI_END                      (EFI_SUBCLASS_SPECIFIC | 0x0000001F)

//
// CPU DXE
//
#define EFI_CU_HP_PC_DXE_INIT                     (EFI_SUBCLASS_SPECIFIC | 0x00000020)
#define EFI_CU_HP_PC_DXE_STEP1                    (EFI_SUBCLASS_SPECIFIC | 0x00000021)
#define EFI_CU_HP_PC_DXE_STEP2                    (EFI_SUBCLASS_SPECIFIC | 0x00000022)
#define EFI_CU_HP_PC_DXE_STEP3                    (EFI_SUBCLASS_SPECIFIC | 0x00000023)
#define EFI_CU_HP_PC_DXE_STEP4                    (EFI_SUBCLASS_SPECIFIC | 0x00000024)
#define EFI_CU_HP_PC_DXE_STEP5                    (EFI_SUBCLASS_SPECIFIC | 0x00000025)
#define EFI_CU_HP_PC_DXE_STEP6                    (EFI_SUBCLASS_SPECIFIC | 0x00000026)
#define EFI_CU_HP_PC_DXE_STEP7                    (EFI_SUBCLASS_SPECIFIC | 0x00000027)
#define EFI_CU_HP_PC_DXE_STEP8                    (EFI_SUBCLASS_SPECIFIC | 0x00000028)
#define EFI_CU_HP_PC_DXE_STEP9                    (EFI_SUBCLASS_SPECIFIC | 0x00000029)
#define EFI_CU_HP_PC_DXE_STEP10                   (EFI_SUBCLASS_SPECIFIC | 0x0000002A)
#define EFI_CU_HP_PC_DXE_STEP11                   (EFI_SUBCLASS_SPECIFIC | 0x0000002B)
#define EFI_CU_HP_PC_DXE_STEP12                   (EFI_SUBCLASS_SPECIFIC | 0x0000002C)
#define EFI_CU_HP_PC_DXE_STEP13                   (EFI_SUBCLASS_SPECIFIC | 0x0000002D)
#define EFI_CU_HP_PC_DXE_STEP14                   (EFI_SUBCLASS_SPECIFIC | 0x0000002E)
#define EFI_CU_HP_PC_DXE_END                      (EFI_SUBCLASS_SPECIFIC | 0x0000002F)

//
// CPU SMM PEI
//
#define EFI_CU_HP_PC_SMM_PEI_INIT                 (EFI_SUBCLASS_SPECIFIC | 0x00000030)
#define EFI_CU_HP_PC_SMM_PEI_STEP1                (EFI_SUBCLASS_SPECIFIC | 0x00000031)
#define EFI_CU_HP_PC_SMM_PEI_STEP2                (EFI_SUBCLASS_SPECIFIC | 0x00000032)
#define EFI_CU_HP_PC_SMM_PEI_STEP3                (EFI_SUBCLASS_SPECIFIC | 0x00000033)
#define EFI_CU_HP_PC_SMM_PEI_STEP4                (EFI_SUBCLASS_SPECIFIC | 0x00000034)
#define EFI_CU_HP_PC_SMM_PEI_STEP5                (EFI_SUBCLASS_SPECIFIC | 0x00000035)
#define EFI_CU_HP_PC_SMM_PEI_STEP6                (EFI_SUBCLASS_SPECIFIC | 0x00000036)
#define EFI_CU_HP_PC_SMM_PEI_END                  (EFI_SUBCLASS_SPECIFIC | 0x0000003F)

//
// CPU SMM DXE
//
#define EFI_CU_HP_PC_SMM_DXE_INIT                 (EFI_SUBCLASS_SPECIFIC | 0x00000040)
#define EFI_CU_HP_PC_SMM_DXE_STEP1                (EFI_SUBCLASS_SPECIFIC | 0x00000041)
#define EFI_CU_HP_PC_SMM_DXE_STEP2                (EFI_SUBCLASS_SPECIFIC | 0x00000042)
#define EFI_CU_HP_PC_SMM_DXE_STEP3                (EFI_SUBCLASS_SPECIFIC | 0x00000043)
#define EFI_CU_HP_PC_SMM_DXE_STEP4                (EFI_SUBCLASS_SPECIFIC | 0x00000044)
#define EFI_CU_HP_PC_SMM_DXE_STEP5                (EFI_SUBCLASS_SPECIFIC | 0x00000045)
#define EFI_CU_HP_PC_SMM_DXE_STEP6                (EFI_SUBCLASS_SPECIFIC | 0x00000046)
#define EFI_CU_HP_PC_SMM_DXE_END                  (EFI_SUBCLASS_SPECIFIC | 0x0000004F)

//
// PEI before memory initialization
//
#define EFI_CU_PLATFORM_PEI_INIT                     (EFI_OEM_SPECIFIC | 0x00000001)
#define EFI_CU_PLATFORM_PEI_STEP1                    (EFI_OEM_SPECIFIC | 0x00000002)
#define EFI_CU_PLATFORM_PEI_STEP2                    (EFI_OEM_SPECIFIC | 0x00000003)
#define EFI_CU_PLATFORM_PEI_STEP3                    (EFI_OEM_SPECIFIC | 0x00000004)
#define EFI_CU_PLATFORM_PEI_STEP4                    (EFI_OEM_SPECIFIC | 0x00000005)
#define EFI_CU_SMBUS_PEI_INIT                        (EFI_OEM_SPECIFIC | 0x00000006)
#define EFI_CU_SMBUS_PEI_EXEC_ENTRY                  (EFI_OEM_SPECIFIC | 0x00000007)
#define EFI_CU_SMBUS_PEI_EXEC_EXIT                   (EFI_OEM_SPECIFIC | 0x00000008)
#define EFI_CU_CLOCK_PEI_INIT_ENTRY                  (EFI_OEM_SPECIFIC | 0x00000009)
#define EFI_CU_CLOCK_PEI_INIT_EXIT                   (EFI_OEM_SPECIFIC | 0x0000000A)
#define EFI_CU_MEMORY_PC_PROG_MTRR                   (EFI_OEM_SPECIFIC | 0x0000000B)
#define EFI_CU_MEMORY_PC_PROG_MTRR_END               (EFI_OEM_SPECIFIC | 0x0000000C)
#define EFI_CU_PLATFORM_PEI_STEP12                   (EFI_OEM_SPECIFIC | 0x0000000D)
#define EFI_CU_PLATFORM_PEI_STEP13                   (EFI_OEM_SPECIFIC | 0x0000000E)
#define EFI_CU_PLATFORM_PEI_END                      (EFI_OEM_SPECIFIC | 0x0000000F)

#define EFI_CU_PLATFORM_DXE_INIT                     (EFI_OEM_SPECIFIC | 0x00000011)
#define EFI_CU_PLATFORM_DXE_STEP1                    (EFI_OEM_SPECIFIC | 0x00000012)
#define EFI_CU_PLATFORM_DXE_STEP2                    (EFI_OEM_SPECIFIC | 0x00000013)
#define EFI_CU_PLATFORM_DXE_STEP3                    (EFI_OEM_SPECIFIC | 0x00000014)
#define EFI_CU_PLATFORM_DXE_STEP4                    (EFI_OEM_SPECIFIC | 0x00000015)
#define EFI_CU_PLATFORM_DXE_INIT_DONE                (EFI_OEM_SPECIFIC | 0x00000016)

#define EFI_CU_OVERCLOCK_PEI_INIT_ENTRY              (EFI_OEM_SPECIFIC | 0x00000017)
#define EFI_CU_OVERCLOCK_PEI_INIT_EXIT               (EFI_OEM_SPECIFIC | 0x00000018)

//
// BDS
//
#define EFI_CU_BDS_INIT                              (EFI_OEM_SPECIFIC | 0x00000060)
#define EFI_CU_BDS_STEP1                             (EFI_OEM_SPECIFIC | 0x00000061)
#define EFI_CU_BDS_STEP2                             (EFI_OEM_SPECIFIC | 0x00000062)
#define EFI_CU_BDS_STEP3                             (EFI_OEM_SPECIFIC | 0x00000063)
#define EFI_CU_BDS_STEP4                             (EFI_OEM_SPECIFIC | 0x00000064)
#define EFI_CU_BDS_STEP5                             (EFI_OEM_SPECIFIC | 0x00000065)
#define EFI_CU_BDS_STEP6                             (EFI_OEM_SPECIFIC | 0x00000066)
#define EFI_CU_BDS_STEP7                             (EFI_OEM_SPECIFIC | 0x00000067)
#define EFI_CU_BDS_STEP8                             (EFI_OEM_SPECIFIC | 0x00000068)
#define EFI_CU_BDS_STEP9                             (EFI_OEM_SPECIFIC | 0x00000069)
#define EFI_CU_BDS_STEP10                            (EFI_OEM_SPECIFIC | 0x0000006A)
#define EFI_CU_BDS_STEP11                            (EFI_OEM_SPECIFIC | 0x0000006B)
#define EFI_CU_BDS_STEP12                            (EFI_OEM_SPECIFIC | 0x0000006C)
#define EFI_CU_BDS_STEP13                            (EFI_OEM_SPECIFIC | 0x0000006D)
#define EFI_CU_BDS_STEP14                            (EFI_OEM_SPECIFIC | 0x0000006E)
#define EFI_CU_BDS_END                               (EFI_OEM_SPECIFIC | 0x0000006F)



#endif
