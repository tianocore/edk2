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
// We should always add mbedtls/config.h here
// to ensure the config override takes effect.
//
#include <mbedtls/mbedtls_config.h>

/**
  The MbedTLS function f_rng, which MbedtlsRand implements.

  @param[in]   RngState Not used, just for compatibility with mbedtls.
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

/**
  Check input P7Data is a wrapped ContentInfo structure or not. If not construct
  a new structure to wrap P7Data.

  Caution: This function may receive untrusted input.
  UEFI Authenticated Variable is external input, so this function will do basic
  check for PKCS#7 data structure.

  @param[in]  P7Data       Pointer to the PKCS#7 message to verify.
  @param[in]  P7Length     Length of the PKCS#7 message in bytes.
  @param[out] WrapFlag     If TRUE P7Data is a ContentInfo structure, otherwise
                           return FALSE.
  @param[out] WrapData     If return status of this function is TRUE:
                           1) when WrapFlag is TRUE, pointer to P7Data.
                           2) when WrapFlag is FALSE, pointer to a new ContentInfo
                           structure. It's caller's responsibility to free this
                           buffer.
  @param[out] WrapDataSize Length of ContentInfo structure in bytes.

  @retval     TRUE         The operation is finished successfully.
  @retval     FALSE        The operation is failed due to lack of resources.

**/
BOOLEAN
WrapPkcs7Data (
  IN  CONST UINT8  *P7Data,
  IN  UINTN        P7Length,
  OUT BOOLEAN      *WrapFlag,
  OUT UINT8        **WrapData,
  OUT UINTN        *WrapDataSize
  );

#endif
