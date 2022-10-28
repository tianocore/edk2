/** @file
*
*  Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>
*  Copyright (c) 2012 - 2022, Arm Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
* @par Revision Reference:
*  - [1] SMC Calling Convention version 1.2
*    (https://developer.arm.com/documentation/den0028/c/?lang=en)
*  - [2] Arm True Random Number Generator Firmware, Interface 1.0,
*    Platform Design Document.
*    (https://developer.arm.com/documentation/den0098/latest/)
*
*  @par Glossary:
*    - TRNG - True Random Number Generator
*
**/

#ifndef ARM_STD_SMC_H_
#define ARM_STD_SMC_H_

/*
 * SMC function IDs for Standard Service queries
 */

#define ARM_SMC_ID_STD_CALL_COUNT  0x8400ff00
#define ARM_SMC_ID_STD_UID         0x8400ff01
/*                                    0x8400ff02 is reserved */
#define ARM_SMC_ID_STD_REVISION  0x8400ff03

/*
 * The 'Standard Service Call UID' is supposed to return the Standard
 * Service UUID. This is a 128-bit value.
 */
#define ARM_SMC_STD_UUID0  0x108d905b
#define ARM_SMC_STD_UUID1  0x47e8f863
#define ARM_SMC_STD_UUID2  0xfbc02dae
#define ARM_SMC_STD_UUID3  0xe2f64156

/*
 * ARM Standard Service Calls revision numbers
 * The current revision is:  0.1
 */
#define ARM_SMC_STD_REVISION_MAJOR  0x0
#define ARM_SMC_STD_REVISION_MINOR  0x1

/*
 * Management Mode (MM) calls cover a subset of the Standard Service Call range.
 * The list below is not exhaustive.
 */
#define ARM_SMC_ID_MM_VERSION_AARCH32  0x84000040
#define ARM_SMC_ID_MM_VERSION_AARCH64  0xC4000040

// Request service from secure standalone MM environment
#define ARM_SMC_ID_MM_COMMUNICATE_AARCH32  0x84000041
#define ARM_SMC_ID_MM_COMMUNICATE_AARCH64  0xC4000041

/* Generic ID when using AArch32 or AArch64 execution state */
#ifdef MDE_CPU_AARCH64
#define ARM_SMC_ID_MM_COMMUNICATE  ARM_SMC_ID_MM_COMMUNICATE_AARCH64
#endif
#ifdef MDE_CPU_ARM
#define ARM_SMC_ID_MM_COMMUNICATE  ARM_SMC_ID_MM_COMMUNICATE_AARCH32
#endif

/* MM return error codes */
#define ARM_SMC_MM_RET_SUCCESS         0
#define ARM_SMC_MM_RET_NOT_SUPPORTED   -1
#define ARM_SMC_MM_RET_INVALID_PARAMS  -2
#define ARM_SMC_MM_RET_DENIED          -3
#define ARM_SMC_MM_RET_NO_MEMORY       -4

// ARM Architecture Calls
#define SMCCC_VERSION            0x80000000
#define SMCCC_ARCH_FEATURES      0x80000001
#define SMCCC_ARCH_SOC_ID        0x80000002
#define SMCCC_ARCH_WORKAROUND_1  0x80008000
#define SMCCC_ARCH_WORKAROUND_2  0x80007FFF

#define SMC_ARCH_CALL_SUCCESS            0
#define SMC_ARCH_CALL_NOT_SUPPORTED      -1
#define SMC_ARCH_CALL_NOT_REQUIRED       -2
#define SMC_ARCH_CALL_INVALID_PARAMETER  -3

/*
 * Power State Coordination Interface (PSCI) calls cover a subset of the
 * Standard Service Call range.
 * The list below is not exhaustive.
 */
#define ARM_SMC_ID_PSCI_VERSION                0x84000000
#define ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH64    0xc4000001
#define ARM_SMC_ID_PSCI_CPU_SUSPEND_AARCH32    0x84000001
#define ARM_SMC_ID_PSCI_CPU_OFF                0x84000002
#define ARM_SMC_ID_PSCI_CPU_ON_AARCH64         0xc4000003
#define ARM_SMC_ID_PSCI_CPU_ON_AARCH32         0x84000003
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_AARCH64  0xc4000004
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_AARCH32  0x84000004
#define ARM_SMC_ID_PSCI_MIGRATE_AARCH64        0xc4000005
#define ARM_SMC_ID_PSCI_MIGRATE_AARCH32        0x84000005
#define ARM_SMC_ID_PSCI_SYSTEM_OFF             0x84000008
#define ARM_SMC_ID_PSCI_SYSTEM_RESET           0x84000009
#define ARM_SMC_ID_PSCI_FEATURES               0x8400000A
#define ARM_SMC_ID_PSCI_SYSTEM_RESET2_AARCH64  0xC4000012

/* The current PSCI version is:  0.2 */
#define ARM_SMC_PSCI_VERSION_MAJOR  0
#define ARM_SMC_PSCI_VERSION_MINOR  2
#define ARM_SMC_PSCI_VERSION  \
  ((ARM_SMC_PSCI_VERSION_MAJOR << 16) | ARM_SMC_PSCI_VERSION_MINOR)

/* PSCI return error codes */
#define ARM_SMC_PSCI_RET_SUCCESS         0
#define ARM_SMC_PSCI_RET_NOT_SUPPORTED   -1
#define ARM_SMC_PSCI_RET_INVALID_PARAMS  -2
#define ARM_SMC_PSCI_RET_DENIED          -3
#define ARM_SMC_PSCI_RET_ALREADY_ON      -4
#define ARM_SMC_PSCI_RET_ON_PENDING      -5
#define ARM_SMC_PSCI_RET_INTERN_FAIL     -6
#define ARM_SMC_PSCI_RET_NOT_PRESENT     -7
#define ARM_SMC_PSCI_RET_DISABLED        -8

#define ARM_SMC_PSCI_TARGET_CPU32(Aff2, Aff1, Aff0) \
  ((((Aff2) & 0xFF) << 16) | (((Aff1) & 0xFF) << 8) | ((Aff0) & 0xFF))

#define ARM_SMC_PSCI_TARGET_CPU64(Aff3, Aff2, Aff1, Aff0) \
  ((((Aff3) & 0xFFULL) << 32) | (((Aff2) & 0xFF) << 16) | (((Aff1) & 0xFF) << 8) | ((Aff0) & 0xFF))

#define ARM_SMC_PSCI_TARGET_GET_AFF0(TargetId)  ((TargetId) & 0xFF)
#define ARM_SMC_PSCI_TARGET_GET_AFF1(TargetId)  (((TargetId) >> 8) & 0xFF)

#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_0  0
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_1  1
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_2  2
#define ARM_SMC_ID_PSCI_AFFINITY_LEVEL_3  3

#define ARM_SMC_ID_PSCI_AFFINITY_INFO_ON          0
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_OFF         1
#define ARM_SMC_ID_PSCI_AFFINITY_INFO_ON_PENDING  2

/*
 * SMC function IDs for Trusted OS Service queries
 */
#define ARM_SMC_ID_TOS_CALL_COUNT  0xbf00ff00
#define ARM_SMC_ID_TOS_UID         0xbf00ff01
/*                                    0xbf00ff02 is reserved */
#define ARM_SMC_ID_TOS_REVISION  0xbf00ff03

// Firmware TRNG interface Function IDs

/*
  SMC/HVC call to get the version of the TRNG backend,
  Cf. [2], 2.1 TRNG_VERSION
  Input values:
    W0    0x8400_0050
    W1-W7 Reserved (MBZ)
  Return values:
    Success (W0 > 0) W0[31] MBZ
      W0[30:16] Major revision
      W0[15:0] Minor revision
      W1 - W3 Reserved (MBZ)
    Error (W0 < 0)
      NOT_SUPPORTED Function not implemented
*/
#define ARM_SMC_ID_TRNG_VERSION  0x84000050

/*
  SMC/HVC call to check if a TRNG function ID is implemented by the backend,
  Cf. [2], Section 2.2 TRNG_FEATURES
  Input Values
    W0    0x8400_0051
    W1    trng_func_id
    W2-W7 Reserved (MBZ)
  Return values:
    Success (W0 >= 0):
      SUCCESS Function is implemented.
        > 0     Function is implemented and
                has specific capabilities,
                see function definition.
    Error (W0 < 0)
      NOT_SUPPORTED Function with FID=trng_func_id
      is not implemented
*/
#define ARM_SMC_ID_TRNG_FEATURES  0x84000051

/*
  SMC/HVC call to get the UUID of the TRNG backend,
  Cf. [2], Section 2.3 TRNG_GET_UUID
  Input Values:
    W0    0x8400_0052
    W1-W7 Reserved (MBZ)
  Return Values:
    Success (W0 != -1)
        W0 UUID[31:0]
        W1 UUID[63:32]
        W2 UUID[95:64]
        W3 UUID[127:96]
    Error (W0 = -1)
        W0 NOT_SUPPORTED
*/
#define ARM_SMC_ID_TRNG_GET_UUID  0x84000052

/*
  AARCH32 SMC/HVC call to get entropy bits, Cf. [2], Section 2.4 TRNG_RND.
  Input values:
    W0    0x8400_0053
    W2-W7 Reserved (MBZ)
  Return values:
    Success (W0 = 0):
      W0 MBZ
      W1 Entropy[95:64]
      W2 Entropy[63:32]
      W3 Entropy[31:0]
    Error (W0 < 0)
          W0 NOT_SUPPORTED
          NO_ENTROPY
          INVALID_PARAMETERS
          W1 - W3 Reserved (MBZ)
*/
#define ARM_SMC_ID_TRNG_RND_AARCH32  0x84000053

/*
  AARCH64 SMC/HVC call to get entropy bits, Cf. [2], Section 2.4 TRNG_RND.
  Input values:
      X0    0xC400_0053
      X2-X7 Reserved (MBZ)
  Return values:
    Success (X0 = 0):
      X0 MBZ
      X1 Entropy[191:128]
      X2 Entropy[127:64]
      X3 Entropy[63:0]
    Error (X0 < 0)
          X0 NOT_SUPPORTED
          NO_ENTROPY
          INVALID_PARAMETERS
          X1 - X3 Reserved (MBZ)
*/
#define ARM_SMC_ID_TRNG_RND_AARCH64  0xC4000053

// Firmware TRNG status codes
#define TRNG_STATUS_SUCCESS            (INT32)(0)
#define TRNG_STATUS_NOT_SUPPORTED      (INT32)(-1)
#define TRNG_STATUS_INVALID_PARAMETER  (INT32)(-2)
#define TRNG_STATUS_NO_ENTROPY         (INT32)(-3)

#endif // ARM_STD_SMC_H_
