/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBSPDM_STDBOOL_ALT_H
#define LIBSPDM_STDBOOL_ALT_H

// In C23, bool is a built-in type
#if __STDC_VERSION__ < 202311L
typedef BOOLEAN bool;
#endif

#ifndef true
#define true  TRUE
#endif

#ifndef false
#define false  FALSE
#endif

#endif /* LIBSPDM_STDBOOL_ALT */
