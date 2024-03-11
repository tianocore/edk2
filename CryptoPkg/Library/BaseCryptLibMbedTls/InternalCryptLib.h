/** @file
  Internal include file for BaseCryptLib.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INTERNAL_CRYPT_LIB_H_
#define INTERNAL_CRYPT_LIB_H_

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>
#include <stdio.h>

//
// We should alwasy add mbedtls/config.h here
// to ensure the config override takes effect.
//
#include <mbedtls/mbedtls_config.h>

/**
  The MbedTLS function f_rng, which MbedtlsRand implements.

  @param[in]   RngState Not used, just for compatibility with mbedlts.
  @param[out]  Output  Pointer to buffer to receive random value.
  @param[in]   Len    Size of random bytes to generate.

  @retval 0      Pseudorandom byte stream generated successfully.
  @retval Non-0  Pseudorandom number generator fails to generate due to lack of entropy.
**/
INT32
MbedtlsRand (
  VOID   *RngState,
  UINT8  *Output,
  UINTN  Len
  );
#endif
