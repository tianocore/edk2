/** @file
  Arm MMU library instance internal header file.

  Copyright (C) Microsoft Corporation. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

// Maximum VA bit definitions.
#define MAX_VA_BITS_48    48
#define MAX_VA_BITS_LPA2  52

typedef
VOID(
 EFIAPI  *ARM_REPLACE_LIVE_TRANSLATION_ENTRY
 )(
  IN  UINT64  *Entry,
  IN  UINT64  Value,
  IN  UINT64  RegionStart,
  IN  BOOLEAN DisableMmu
  );
