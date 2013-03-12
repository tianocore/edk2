/** @file
*
*  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __ARM_SMC_H__
#define __ARM_SMC_H__

#include <IndustryStandard/ArmTrustZoneSmc.h>

#define ARM_SMC_ID_PRESENCE       ARM_TRUSTZONE_ARM_FAST_SMC_ID_PRESENCE
#define ARM_SMC_ID_UID            ARM_TRUSTZONE_ARM_FAST_SMC_ID_UID
#define ARM_SMC_ID_REVISION       ARM_TRUSTZONE_ARM_FAST_SMC_ID_REVISION
#define ARM_SMC_ARM_CPU_SUSPEND   0x80100001
#define ARM_SMC_ARM_CPU_OFF       0x80100002
#define ARM_SMC_ARM_CPU_ON        0x80100003
#define ARM_SMC_ARM_MIGRATE       0x80100004

#define ARM_SMC_ARM_CPU_SUSPEND_STANDBY_STATE       (0 << 16)
#define ARM_SMC_ARM_CPU_SUSPEND_POWER_DOWN_STATE    (1 << 16)

#define ARM_SMC_ARM_CPU_SUSPEND_CURRENT_CPU         (0 << 24)
#define ARM_SMC_ARM_CPU_SUSPEND_CLUSTER_AFFINITY_1  (1 << 24)
#define ARM_SMC_ARM_CPU_SUSPEND_CLUSTER_AFFINITY_2  (2 << 24)
#define ARM_SMC_ARM_CPU_SUSPEND_CLUSTER_AFFINITY_3  (3 << 24)

#define ARM_SMC_ARM_CPU_OFF_MASK_STATE              (1 << 16)
#define ARM_SMC_ARM_CPU_OFF_STANDBY_STATE           (0 << 16)
#define ARM_SMC_ARM_CPU_OFF_POWER_DOWN_STATE        (1 << 16)

#define ARM_SMC_ARM_CPU_OFF_CURRENT_CPU             (0 << 24)
#define ARM_SMC_ARM_CPU_OFF_CLUSTER_AFFINITY_1      (1 << 24)
#define ARM_SMC_ARM_CPU_OFF_CLUSTER_AFFINITY_2      (2 << 24)
#define ARM_SMC_ARM_CPU_OFF_CLUSTER_AFFINITY_3      (3 << 24)


#define ARM_SMC_ARM_RETURN_SUCCESS            (UINTN)(0)
#define ARM_SMC_ARM_RETURN_NOT_IMPLEMENTED    (UINTN)(-1)
#define ARM_SMC_ARM_RETURN_INVALID_PARAMETER  (UINTN)(-2)
#define ARM_SMC_ARM_RETURN_DENIED             (UINTN)(-3)
#define ARM_SMC_ARM_RETURN_CORE_NOT_AVAILABLE (UINTN)(-3)

#endif
