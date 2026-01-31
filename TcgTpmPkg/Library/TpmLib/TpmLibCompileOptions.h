/** @file
  This file specifies additional compile options to build TPM reference code.

  For useful build option, see:
    - https://github.com/TrustedComputingGroup/TPM/blob/main/docs/architecture/Tpm.Crypto.Libraries.md

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TPMLIB_COMPILE_OPTIONS_H_
#define TPMLIB_COMPILE_OPTIONS_H_

#include <CrtLibSupport.h>
#include <Library/PcdLib.h>

/*
 * IS_ALIGNED() macro is defined in TPM/TPMCmd/tpm/include/tpm_public/tpm_radix.h
 * for TCG TPM v2.0 implementation.
 * So, undefine IS_ALIGNED() defined in MdePkg/Include/Base.h.
 */
#undef IS_ALIGNED

#define HASH_LIB     Ossl
#define SYM_LIB      Ossl
#define MATH_LIB     TpmBigNum
#define BN_MATH_LIB  Ossl

#define USE_PLATFORM_EPS  YES

#endif
