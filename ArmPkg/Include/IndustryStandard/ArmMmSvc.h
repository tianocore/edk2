/** @file
*
*  Copyright (c) 2012-2024, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_MM_SVC_H_
#define ARM_MM_SVC_H_

/*
 * SPM_MM FIDs to allow the secure partition to initialise itself, handle
 * delegated events and request the Secure partition manager to perform
 * privileged operations on its behalf.
 */
#define ARM_FID_SPM_MM_VERSION_AARCH32                0x84000060
#define ARM_FID_SPM_MM_SP_EVENT_COMPLETE_AARCH32      0x84000061
#define ARM_FID_SPM_MM_SP_GET_MEM_ATTRIBUTES_AARCH32  0x84000064
#define ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES_AARCH32  0x84000065
#define ARM_FID_SPM_MM_SP_EVENT_COMPLETE_AARCH64      0xC4000061
#define ARM_FID_SPM_MM_SP_GET_MEM_ATTRIBUTES_AARCH64  0xC4000064
#define ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES_AARCH64  0xC4000065

/* Generic IDs for AArch64 execution state */
#define ARM_FID_SPM_MM_SP_EVENT_COMPLETE      ARM_FID_SPM_MM_SP_EVENT_COMPLETE_AARCH64
#define ARM_FID_SPM_MM_SP_GET_MEM_ATTRIBUTES  ARM_FID_SPM_MM_SP_GET_MEM_ATTRIBUTES_AARCH64
#define ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES  ARM_FID_SPM_MM_SP_SET_MEM_ATTRIBUTES_AARCH64

#define ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_MASK       0x3
#define ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_SHIFT      0
#define ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_NO_ACCESS  0
#define ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RW         1
#define ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_RO         3

#define ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_MASK   0x1
#define ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_SHIFT  2
#define ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_X      0
#define ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_XN     1

#define ARM_SPM_MM_SET_MEM_ATTR_MAKE_PERM_REQUEST(dataperm, codeperm)   \
    ((((codeperm) & ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_MASK) <<        \
      ARM_SPM_MM_SET_MEM_ATTR_CODE_PERM_SHIFT) |                    \
    (( (dataperm) & ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_MASK) <<        \
     ARM_SPM_MM_SET_MEM_ATTR_DATA_PERM_SHIFT))

/* SPM_MM SVC Return error codes */
#define ARM_SPM_MM_RET_SUCCESS         0
#define ARM_SPM_MM_RET_NOT_SUPPORTED   -1
#define ARM_SPM_MM_RET_INVALID_PARAMS  -2
#define ARM_SPM_MM_RET_DENIED          -3
#define ARM_SPM_MM_RET_NO_MEMORY       -5

#define ARM_SPM_MM_SUPPORT_MAJOR_VERSION  0
#define ARM_SPM_MM_SUPPORT_MINOR_VERSION  1
#define ARM_SPM_MM_VERSION_MASK           0xFFFF
#define ARM_SPM_MM_MAJOR_VERSION_SHIFT    16
#define ARM_SPM_MM_MINOR_VERSION_SHIFT    0

#define ARM_SPM_MM_MAJOR_VERSION_GET(version) \
  (((version) >> ARM_SPM_MM_MAJOR_VERSION_SHIFT) & ARM_SPM_MM_VERSION_MASK)

#define ARM_SPM_MM_MINOR_VERSION_GET(version) \
  (((version) >> ARM_SPM_MM_MINOR_VERSION_SHIFT) & ARM_SPM_MM_VERSION_MASK)

#endif // ARM_MM_SVC_H_
