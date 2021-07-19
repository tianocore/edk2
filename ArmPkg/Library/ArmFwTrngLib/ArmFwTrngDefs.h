/** @file
  Arm Firmware TRNG definitions.

  Copyright (c) 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - [1] Arm True Random Number Generator Firmware, Interface 1.0,
        Platform Design Document.
        (https://developer.arm.com/documentation/den0098/latest/)

  @par Glossary:
    - TRNG - True Random Number Generator
    - FID  - Function ID
**/

#ifndef ARM_FW_TRNG_DEFS_H_
#define ARM_FW_TRNG_DEFS_H_

// Firmware TRNG interface Function IDs
#define FID_TRNG_VERSION      0x84000050
#define FID_TRNG_FEATURES     0x84000051
#define FID_TRNG_GET_UUID     0x84000052
#define FID_TRNG_RND_AARCH32  0x84000053
#define FID_TRNG_RND_AARCH64  0xC4000053

// Firmware TRNG revision mask and shift
#define TRNG_REV_MAJOR_MASK   0x7FFF
#define TRNG_REV_MINOR_MASK   0xFFFF
#define TRNG_REV_MAJOR_SHIFT  16
#define TRNG_REV_MINOR_SHIFT  0

// Firmware TRNG status codes
#define TRNG_STATUS_SUCCESS     (INT32)(0)
#define TRNG_NOT_SUPPORTED      (INT32)(-1)
#define TRNG_INVALID_PARAMETER  (INT32)(-2)
#define TRNG_NO_ENTROPY         (INT32)(-3)

#if defined (MDE_CPU_ARM)
/** FID to use on AArch32 platform to request entropy.
*/
#define FID_TRNG_RND        FID_TRNG_RND_AARCH32

/** Maximum bits of entropy supported on AArch32.
*/
#define MAX_ENTROPY_BITS    96
#elif defined (MDE_CPU_AARCH64)
/** FID to use on AArch64 platform to request entropy.
*/
#define FID_TRNG_RND        FID_TRNG_RND_AARCH64

/** Maximum bits of entropy supported on AArch64.
*/
#define MAX_ENTROPY_BITS    192
#else
#error "Firmware TRNG not supported. Unknown chipset."
#endif

/** Typedef for SMC or HVC arguments.
*/
typedef ARM_SMC_ARGS  ARM_MONITOR_ARGS;

#endif // ARM_FW_TRNG_DEFS_H_
