/** @file
  OpensslLib class with APIs from the openssl project

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef OPENSSL_LIB_H_
#define OPENSSL_LIB_H_

#include <openssl/opensslv.h>

#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:__ftoul2_legacy=__ftol2")
#endif

#endif
