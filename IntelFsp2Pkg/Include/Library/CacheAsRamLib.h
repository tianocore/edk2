/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  This function disable CAR.

  @param[in] DisableCar       TRUE means use INVD, FALSE means use WBINVD

**/
VOID
EFIAPI
DisableCacheAsRam (
  IN BOOLEAN  DisableCar
  );
