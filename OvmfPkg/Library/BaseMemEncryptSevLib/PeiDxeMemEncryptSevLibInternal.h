/** @file

  Internal interfaces for the PEI and DXE MemEncryptSevLib instances.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <WorkArea.h>

/**
   Read the workarea to determine whether SEV is enabled. If enabled,
   then return the SevEsWorkArea pointer.

  **/
SEC_SEV_ES_WORK_AREA *
EFIAPI
GetSevEsWorkArea (
  VOID
  );
