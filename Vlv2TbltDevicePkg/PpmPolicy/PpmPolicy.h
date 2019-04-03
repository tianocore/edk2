/** 
  Copyright (c) 2009 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent



Module Name:

  PpmPolicy.h

Abstract:

  Header file for the PpmPolicyInitDxe Driver.

--*/
#include <PiDxe.h>
//
// Driver Produced Protocol Prototypes
//
#include <Protocol/PpmPlatformPolicy.h>

PPM_PLATFORM_POLICY_PROTOCOL    mDxePlatformPpmPolicy;

// Function Definition
#define  ICH_DEVICE_ENABLE       1
#define  ICH_DEVICE_DISABLE      0

#define POWER_STATE_SWITCH_SMI                       43
#define ENABLE_C_STATE_IO_REDIRECTION_SMI            70
#define DISABLE_C_STATE_IO_REDIRECTION_SMI           71
#define ENABLE_SMI_C_STATE_COORDINATION_SMI          72
#define DISABLE_SMI_C_STATE_COORDINATION_SMI         73
#define ENABLE_P_STATE_HARDWARE_COORDINATION_SMI     74
#define DISABLE_P_STATE_HARDWARE_COORDINATION_SMI    75
#define S3_RESTORE_MSR_SW_SMI                        48
#define ENABLE_C6_RESIDENCY_SMI                      76
