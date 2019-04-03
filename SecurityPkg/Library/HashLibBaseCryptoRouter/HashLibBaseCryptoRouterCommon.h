/** @file
  This is BaseCrypto router support function definition.

Copyright (c) 2013 - 2016, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _HASH_LIB_BASE_CRYPTO_ROUTER_COMMON_H_
#define _HASH_LIB_BASE_CRYPTO_ROUTER_COMMON_H_

/**
  The function get hash mask info from algorithm.

  @param HashGuid Hash Guid

  @return HashMask
**/
UINT32
EFIAPI
Tpm2GetHashMaskFromAlgo (
  IN EFI_GUID  *HashGuid
  );

/**
  The function set digest to digest list.

  @param DigestList digest list
  @param Digest     digest data
**/
VOID
EFIAPI
Tpm2SetHashToDigestList (
  IN OUT TPML_DIGEST_VALUES *DigestList,
  IN TPML_DIGEST_VALUES     *Digest
  );

#endif
