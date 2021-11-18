/** @file
  Status Code Definitions, according to Intel Platform Innovation Framework
  for EFI Status Codes Specification

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  Intel Platform Innovation Framework for EFI Status Codes Specification
  Version 0.92.

**/

#ifndef _FRAMEWORK_STATUS_CODE_H_
#define _FRAMEWORK_STATUS_CODE_H_

//
// Required for X64 defines for CPU exception types
//
#include <Protocol/DebugSupport.h>

///
/// Software Class DXE BS Driver Subclass Progress Code definitions.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS  (EFI_SUBCLASS_SPECIFIC | 0x00000005)
#define EFI_SW_DXE_BS_PC_VERIFYING_PASSWORD        (EFI_SUBCLASS_SPECIFIC | 0x00000006)
///@}

///
/// Software Class DXE RT Driver Subclass Progress Code definitions.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_SW_DXE_RT_PC_S0  (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_SW_DXE_RT_PC_S1  (EFI_SUBCLASS_SPECIFIC | 0x00000001)
#define EFI_SW_DXE_RT_PC_S2  (EFI_SUBCLASS_SPECIFIC | 0x00000002)
#define EFI_SW_DXE_RT_PC_S3  (EFI_SUBCLASS_SPECIFIC | 0x00000003)
#define EFI_SW_DXE_RT_PC_S4  (EFI_SUBCLASS_SPECIFIC | 0x00000004)
#define EFI_SW_DXE_RT_PC_S5  (EFI_SUBCLASS_SPECIFIC | 0x00000005)
///@}

///
/// Software Subclass definitions.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
#define EFI_SOFTWARE_X64_EXCEPTION  (EFI_SOFTWARE | 0x00130000)

///
/// Software Class X64 Exception Subclass Error Code definitions.
/// These exceptions are derived from the debug protocol definitions in the EFI
/// specification.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_SW_EC_X64_DIVIDE_ERROR     EXCEPT_X64_DIVIDE_ERROR
#define EFI_SW_EC_X64_DEBUG            EXCEPT_X64_DEBUG
#define EFI_SW_EC_X64_NMI              EXCEPT_X64_NMI
#define EFI_SW_EC_X64_BREAKPOINT       EXCEPT_X64_BREAKPOINT
#define EFI_SW_EC_X64_OVERFLOW         EXCEPT_X64_OVERFLOW
#define EFI_SW_EC_X64_BOUND            EXCEPT_X64_BOUND
#define EFI_SW_EC_X64_INVALID_OPCODE   EXCEPT_X64_INVALID_OPCODE
#define EFI_SW_EC_X64_DOUBLE_FAULT     EXCEPT_X64_DOUBLE_FAULT
#define EFI_SW_EC_X64_INVALID_TSS      EXCEPT_X64_INVALID_TSS
#define EFI_SW_EC_X64_SEG_NOT_PRESENT  EXCEPT_X64_SEG_NOT_PRESENT
#define EFI_SW_EC_X64_STACK_FAULT      EXCEPT_X64_STACK_FAULT
#define EFI_SW_EC_X64_GP_FAULT         EXCEPT_X64_GP_FAULT
#define EFI_SW_EC_X64_PAGE_FAULT       EXCEPT_X64_PAGE_FAULT
#define EFI_SW_EC_X64_FP_ERROR         EXCEPT_X64_FP_ERROR
#define EFI_SW_EC_X64_ALIGNMENT_CHECK  EXCEPT_X64_ALIGNMENT_CHECK
#define EFI_SW_EC_X64_MACHINE_CHECK    EXCEPT_X64_MACHINE_CHECK
#define EFI_SW_EC_X64_SIMD             EXCEPT_X64_SIMD
///@}

///
/// Software Class EFI After Life Subclass Progress Code definitions.
///
///@{
#define EFI_SW_AL_PC_ENTRY_POINT     (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_SW_AL_PC_RETURN_TO_LAST  (EFI_SUBCLASS_SPECIFIC | 0x00000001)
///@}

///
/// Software Class DXE Core Subclass Error Code definitions.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
#define EFI_SW_CSM_LEGACY_ROM_INIT  (EFI_SUBCLASS_SPECIFIC | 0x00000000)

///
/// IO Bus Class ATA/ATAPI Subclass Progress Code definitions.
///
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_IOB_ATA_BUS_SMART_ENABLE          (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_IOB_ATA_BUS_SMART_DISABLE         (EFI_SUBCLASS_SPECIFIC | 0x00000001)
#define EFI_IOB_ATA_BUS_SMART_OVERTHRESHOLD   (EFI_SUBCLASS_SPECIFIC | 0x00000002)
#define EFI_IOB_ATA_BUS_SMART_UNDERTHRESHOLD  (EFI_SUBCLASS_SPECIFIC | 0x00000003)
///@}

///
/// IO Bus Class ATA/ATAPI Subclass Error Code definitions.
///
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_IOB_ATA_BUS_SMART_NOTSUPPORTED  (EFI_SUBCLASS_SPECIFIC | 0x00000000)
#define EFI_IOB_ATA_BUS_SMART_DISABLED      (EFI_SUBCLASS_SPECIFIC | 0x00000001)
///@}

///
/// The reason that the processor was disabled.
///
/// Inconsistent with specification here:
/// The Framework Specification, StatusCodes 0.92, does not define the macros.
///
///@{
#define EFI_CPU_CAUSE_NOT_DISABLED  0x0000
///@}

///
/// Software Class PEI Module Subclass Progress Code definitions.
///
///@{
#define EFI_SW_PEIM_PC_RECOVERY_BEGIN  EFI_SW_PEI_PC_RECOVERY_BEGIN
#define EFI_SW_PEIM_PC_CAPSULE_LOAD    EFI_SW_PEI_PC_CAPSULE_LOAD
#define EFI_SW_PEIM_PC_CAPSULE_START   EFI_SW_PEI_PC_CAPSULE_START
#define EFI_SW_PEIM_PC_RECOVERY_USER   EFI_SW_PEI_PC_RECOVERY_USER
#define EFI_SW_PEIM_PC_RECOVERY_AUTO   EFI_SW_PEI_PC_RECOVERY_AUTO
///@}

///
/// Software Class PEI Core Subclass Error Code definitions.
///
///@{
#define EFI_SW_PEIM_CORE_EC_DXE_CORRUPT       EFI_SW_PEI_CORE_EC_DXE_CORRUPT
#define EFI_SW_PEIM_CORE_EC_DXEIPL_NOT_FOUND  EFI_SW_PEI_CORE_EC_DXEIPL_NOT_FOUND
///@}

#endif
