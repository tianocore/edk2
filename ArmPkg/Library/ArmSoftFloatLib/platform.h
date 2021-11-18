/*
 * Copyright (c) 2019, Linaro Limited
 * Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#ifndef ARM_SOFT_FLOAT_LIB_H_
#define ARM_SOFT_FLOAT_LIB_H_

#define LITTLEENDIAN           1
#define INLINE                 static inline
#define SOFTFLOAT_BUILTIN_CLZ  1
#define SOFTFLOAT_FAST_INT64
#include "opts-GCC.h"

#endif // ARM_SOFT_FLOAT_LIB_H_
