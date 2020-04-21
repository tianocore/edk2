/** @file
  This Protocol provides Crypto services to DXE modules

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EDKII_CRYPTO_PROTOCOL_H__
#define __EDKII_CRYPTO_PROTOCOL_H__

#include <Base.h>
#include <Library/BaseCryptLib.h>
#include <Library/PcdLib.h>

///
/// The version of the EDK II Crypto Protocol.
/// As APIs are added to BaseCryptLib, the EDK II Crypto Protocol is extended
/// with new APIs at the end of the EDK II Crypto Protocol structure.  Each time
/// the EDK II Crypto Protocol is extended, this version define must be
/// increased.
///
#define EDKII_CRYPTO_VERSION 6
#endif
