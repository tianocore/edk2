/** @file
  Include file to support building the third-party cryptographic library.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef CRYPTO_CRT_STDIO_H_
#define CRYPTO_CRT_STDIO_H_
#include <CrtLibSupport.h>

//
// only use in Mbedlts. The Openssl has defined them internally.
//
#ifndef OPENSSL_SYS_UEFI
typedef INT8    int8_t;
typedef UINT8   uint8_t;
typedef INT16   int16_t;
typedef UINT16  uint16_t;
typedef INT32   int32_t;
typedef UINT32  uint32_t;
typedef INT64   int64_t;
typedef UINT64  uint64_t;
typedef UINTN   uintptr_t;
#endif

#endif
