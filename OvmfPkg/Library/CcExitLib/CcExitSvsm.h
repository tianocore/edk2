/** @file
  Secure VM Service Module (SVSM) functions.

  Copyright (C) 2024, Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  Secure VM Service Module Specification

**/

#ifndef __CCEXITLIB_CCEXITSVSM_H__
#define __CCEXITLIB_CCEXITSVSM_H__

/**
  Return the physical address of SVSM Call Area (CAA).

  Determines the physical address of the SVSM CAA.

  @return                         The physical address of the SVSM CAA

**/
UINT64
EFIAPI
SvsmGetCaaPa (
  VOID
  );

#endif
