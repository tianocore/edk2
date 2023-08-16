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
  The MbedTLS function f_rng, which MbedRand implements, is not
  documented well.

  @param[in]       RngState  RngState.
  @param[in]       Output    Output.
  @param[in]       Len       Len.

  @retval  0                 success.
  @retval  non-zero          failed.

**/
INT32
MbedRand (
  VOID   *RngState,
  UINT8  *OutPut,
  UINTN  Len
  );

#endif
