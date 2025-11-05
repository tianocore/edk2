/** @file
  Rng Algorithm

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef RNG_ALGORITHM_GUID_H_
#define RNG_ALGORITHM_GUID_H_

///
/// The implementation of a Random Number Generator might be unsafe, when using
/// a dummy implementation for instance. Allow identifying such implementation
/// with this GUID.
///
#define EDKII_RNG_ALGORITHM_UNSAFE \
  { \
    0x869f728c, 0x409d, 0x4ab4, {0xac, 0x03, 0x71, 0xd3, 0x09, 0xc1, 0xb3, 0xf4 } \
  }

extern EFI_GUID  gEdkiiRngAlgorithmUnSafe;

#endif // #ifndef RNG_ALGORITHM_GUID_H_
