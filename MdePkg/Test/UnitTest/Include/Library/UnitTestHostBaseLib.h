/** @file
  Unit Test Host BaseLib hooks.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <ProcessorBind.h>

#if defined (MDE_CPU_X64) || defined (MDE_CPU_IA32)
  #include "./X86/X86UnitTestHostBaseLib.h"
#elif defined (MDE_CPU_AARCH64)
  #include "./AArch64/AArch64UnitTestHostBaseLib.h"
#else
  #error "Unsupported architecture for HostUnitTest."
#endif
