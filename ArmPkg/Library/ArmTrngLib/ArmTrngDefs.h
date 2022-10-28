/** @file
  Arm Firmware TRNG definitions.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

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

#include <IndustryStandard/ArmStdSmc.h>

// Firmware TRNG revision mask and shift
#define TRNG_REV_MAJOR_MASK   0x7FFF
#define TRNG_REV_MINOR_MASK   0xFFFF
#define TRNG_REV_MAJOR_SHIFT  16

#if defined (MDE_CPU_ARM)

/** FID to use on AArch32 platform to request entropy.
*/
#define ARM_SMC_ID_TRNG_RND  ARM_SMC_ID_TRNG_RND_AARCH32

/** Maximum bits of entropy supported on AArch32.
*/
#define MAX_ENTROPY_BITS  96
#elif defined (MDE_CPU_AARCH64)

/** FID to use on AArch64 platform to request entropy.
*/
#define ARM_SMC_ID_TRNG_RND  ARM_SMC_ID_TRNG_RND_AARCH64

/** Maximum bits of entropy supported on AArch64.
*/
#define MAX_ENTROPY_BITS  192
#else
  #error "Firmware TRNG not supported. Unknown chipset."
#endif

#endif // ARM_FW_TRNG_DEFS_H_
