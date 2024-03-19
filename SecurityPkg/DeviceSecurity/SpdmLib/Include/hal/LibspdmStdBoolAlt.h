/**
 *  Copyright Notice:
 *  Copyright 2021-2022 DMTF. All rights reserved.
 *  License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libspdm/blob/main/LICENSE.md
 **/

/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBSPDM_STDBOOL_ALT_H
#define LIBSPDM_STDBOOL_ALT_H

typedef BOOLEAN bool;

#ifndef true
#define true  TRUE
#endif

#ifndef false
#define false  FALSE
#endif

#endif /* LIBSPDM_STDBOOL_ALT */
