/** @file
*
*  Copyright (c) 2012-2017, ARM Limited. All rights reserved.
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

#ifndef __ARM_MM_SVC_H__
#define __ARM_MM_SVC_H__

/*
 * SVC IDs to allow the MM secure partition to initialise itself, handle
 * delegated events and request the Secure partition manager to perform
 * privileged operations on its behalf.
 */
#define ARM_SVC_ID_SPM_VERSION_AARCH64             0xC4000060
#define ARM_SVC_ID_SP_EVENT_COMPLETE_AARCH64       0xC4000061
#define ARM_SVC_ID_SP_GET_MEM_ATTRIBUTES_AARCH64   0xC4000064
#define ARM_SVC_ID_SP_SET_MEM_ATTRIBUTES_AARCH64   0xC4000065

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

#endif
