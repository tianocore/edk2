/** @file
  This PPI provides Crypto services to PEIMs

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_CRYPTO_PPI_H__
#define __EDKII_CRYPTO_PPI_H__

#include <Protocol/Crypto.h>

///
/// EDK II Crypto PPI is identical to EDK II Crypto Protocol
///
typedef EDKII_CRYPTO_PROTOCOL EDKII_CRYPTO_PPI;

extern GUID gEdkiiCryptoPpiGuid;

#endif
