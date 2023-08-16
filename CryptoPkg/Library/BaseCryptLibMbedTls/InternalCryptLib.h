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

#endif
