/** @file

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PRE_FSP_SEC_H_
#define _PRE_FSP_SEC_H_

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

/**
  This interface gets FspInfoHeader pointer

  @return   FSP info header.

**/
UINTN
EFIAPI
AsmGetFspInfoHeader (
  VOID
  );

/**
  This interface gets FspBaseAddress

  @return   FspBaseAddress.

**/
UINTN
EFIAPI
AsmGetRuntimeFspBaseAddress (
  VOID
  );

/**
  This interface gets SecCoreAddress

  @return   SecCoreAddress, or zero if no patch in build script

**/
UINTN
EFIAPI
AsmGetRuntimeSecCoreAddress (
  VOID
  );

/**
  This interface gets PeiCoreAddress

  @return   PeiCoreAddress, or zero if no patch in build script

**/
UINTN
EFIAPI
AsmGetRuntimePeiCoreAddress (
  VOID
  );

#endif
