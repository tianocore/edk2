/** @file
  This Protocol provides Crypto services to SMM modules

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_SMM_CRYPTO_PROTOCOL_H__
#define __EDKII_SMM_CRYPTO_PROTOCOL_H__

#include <Protocol/Crypto.h>

///
/// EDK II SMM Crypto Protocol is identical to EDK II Crypto Protocol
///
typedef EDKII_CRYPTO_PROTOCOL EDKII_SMM_CRYPTO_PROTOCOL;

extern GUID gEdkiiSmmCryptoProtocolGuid;

#endif
