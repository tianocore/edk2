/** @file
  Header file for FF-A ABI's that will be used for
  communication between S-EL0 and the Secure Partition
  Manager(SPM)

  Copyright (c) 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - FF-A Version 1.0


**/

#ifndef ARM_FFA_SVC_H_
#define ARM_FFA_SVC_H_

#define ARM_SVC_ID_FFA_VERSION_AARCH32                  0x84000063
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ_AARCH32      0x8400006F
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP_AARCH32     0x84000070
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ_AARCH64      0xC400006F
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP_AARCH64     0xC4000070

/* Generic IDs when using AArch32 or AArch64 execution state */
#ifdef MDE_CPU_AARCH64
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ     ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ_AARCH64
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP    ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP_AARCH64
#endif
#ifdef MDE_CPU_ARM
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ     ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ_AARCH32
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP    ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP_AARCH32
#endif

#define SPM_MAJOR_VERSION_FFA                           1
#define SPM_MINOR_VERSION_FFA                           0

#define ARM_FFA_SPM_RET_SUCCESS                          0
#define ARM_FFA_SPM_RET_NOT_SUPPORTED                   -1
#define ARM_FFA_SPM_RET_INVALID_PARAMETERS              -2
#define ARM_FFA_SPM_RET_NO_MEMORY                       -3
#define ARM_FFA_SPM_RET_BUSY                            -4
#define ARM_FFA_SPM_RET_INTERRUPTED                     -5
#define ARM_FFA_SPM_RET_DENIED                          -6
#define ARM_FFA_SPM_RET_RETRY                           -7
#define ARM_FFA_SPM_RET_ABORTED                         -8

// For now, the destination id to be used in the FF-A calls
// is being hard-coded. Subsequently, support will be added
// to get the endpoint id's dynamically
// This is the endpoint id used by the optee os's implementation
// of the spmc.
// https://github.com/OP-TEE/optee_os/blob/master/core/arch/arm/kernel/stmm_sp.c#L66
#define ARM_FFA_DESTINATION_ENDPOINT_ID                  3

#endif // ARM_FFA_SVC_H_
