/** @file
  Arm MMU library instance internal header file.

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

typedef
VOID(
 EFIAPI  *ARM_REPLACE_LIVE_TRANSLATION_ENTRY
 )(
  IN  UINT64  *Entry,
  IN  UINT64  Value,
  IN  UINT64  RegionStart,
  IN  BOOLEAN DisableMmu
  );
