/** @file
*
*  Copyright (c) 2012-2017, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_MM_SVC_H_
#define ARM_MM_SVC_H_

/*
 * SVC IDs to allow the MM secure partition to initialise itself, handle
 * delegated events and request the Secure partition manager to perform
 * privileged operations on its behalf.
 */
#define ARM_SVC_ID_SPM_VERSION_AARCH32             0x84000060
#define ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH32       0x84000061
#define ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH32   0x84000064
#define ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH32   0x84000065
#define ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64       0xC4000061
#define ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH64   0xC4000064
#define ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH64   0xC4000065

/* Generic IDs when using AArch32 or AArch64 execution state */
#ifdef MDE_CPU_AARCH64
#define ARM_SVC_ID_SP_EVENT_COMPLETE               ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64
#define ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES       ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH64
#define ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES       ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH64
#endif
#ifdef MDE_CPU_ARM
#define ARM_SVC_ID_SP_EVENT_COMPLETE               ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH32
#define ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES       ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH32
#define ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES       ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH32
#endif

#define SET_MEM_ATTR_DATA_PERM_MASK       0x3
#define SET_MEM_ATTR_DATA_PERM_SHIFT        0
#define SET_MEM_ATTR_DATA_PERM_NO_ACCESS    0
#define SET_MEM_ATTR_DATA_PERM_RW           1
#define SET_MEM_ATTR_DATA_PERM_RO           3

#define SET_MEM_ATTR_CODE_PERM_MASK   0x1
#define SET_MEM_ATTR_CODE_PERM_SHIFT    2
#define SET_MEM_ATTR_CODE_PERM_X        0
#define SET_MEM_ATTR_CODE_PERM_XN       1

#define SET_MEM_ATTR_MAKE_PERM_REQUEST(d_perm, c_perm)                            \
    ((((c_perm) & SET_MEM_ATTR_CODE_PERM_MASK) << SET_MEM_ATTR_CODE_PERM_SHIFT) | \
    (( (d_perm) & SET_MEM_ATTR_DATA_PERM_MASK) << SET_MEM_ATTR_DATA_PERM_SHIFT))

/* MM SVC Return error codes */
#define ARM_SVC_SPM_RET_SUCCESS               0
#define ARM_SVC_SPM_RET_NOT_SUPPORTED        -1
#define ARM_SVC_SPM_RET_INVALID_PARAMS       -2
#define ARM_SVC_SPM_RET_DENIED               -3
#define ARM_SVC_SPM_RET_NO_MEMORY            -5

#define SPM_MAJOR_VERSION                     0
#define SPM_MINOR_VERSION                     1

#endif // ARM_MM_SVC_H_
