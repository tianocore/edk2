/** @file  
  Internal include file for BaseCryptLib.

Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __INTERNAL_CRYPT_LIB_H__
#define __INTERNAL_CRYPT_LIB_H__

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>

#include "OpenSslSupport.h"

#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define OBJ_get0_data(o) ((o)->data)
#define OBJ_length(o) ((o)->length)
#endif

#endif

