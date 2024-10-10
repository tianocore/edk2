/** @file
  This header file defines common macros for the use in RedfishPkg.

  Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef REDFISH_COMMON_H_
#define REDFISH_COMMON_H_

#ifndef IS_EMPTY_STRING
#define IS_EMPTY_STRING(a)  ((a) == NULL || (a)[0] == '\0')
#endif

#endif
