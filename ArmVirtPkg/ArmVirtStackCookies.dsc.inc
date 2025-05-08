#
#  Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#

[Defines]

# Dynamic stack cookies are not supported on ARM
!if $(ARCH) == ARM
  DEFINE CUSTOM_STACK_CHECK_LIB = STATIC
!else
  DEFINE CUSTOM_STACK_CHECK_LIB = DYNAMIC
!endif

