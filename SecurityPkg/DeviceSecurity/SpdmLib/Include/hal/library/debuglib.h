/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/** @file
  Provides services to print debug and assert messages to a debug output device.

  The Debug library supports debug print and asserts based on a combination of macros and code.
  The debug library can be turned on and off so that the debug code does not increase the size of an image.

  Note that a reserved macro named MDEPKG_NDEBUG is introduced for the intention
  of size reduction when compiler optimization is disabled. If MDEPKG_NDEBUG is
  defined, then debug and assert related macros wrapped by it are the NULL implementations.
**/

#ifndef DEBUG_LIB_H
#define DEBUG_LIB_H

#include <Library/DebugLib.h>

#define LIBSPDM_DEBUG_INFO     DEBUG_INFO
#define LIBSPDM_DEBUG_VERBOSE  DEBUG_VERBOSE
#define LIBSPDM_DEBUG_ERROR    DEBUG_ERROR

#define LIBSPDM_DEBUG                DEBUG
#define LIBSPDM_ASSERT               ASSERT
#define LIBSPDM_ASSERT_RETURN_ERROR  ASSERT_RETURN_ERROR

#define LIBSPDM_DEBUG_CODE_BEGIN  DEBUG_CODE_BEGIN
#define LIBSPDM_DEBUG_CODE_END    DEBUG_CODE_END

#define LIBSPDM_DEBUG_CODE  DEBUG_CODE

#endif /* DEBUG_LIB_H */
