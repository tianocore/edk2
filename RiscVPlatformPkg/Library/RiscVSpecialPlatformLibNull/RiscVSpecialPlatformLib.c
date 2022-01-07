/** @file
  Library to provide platform_override for the special
  RISC-V platform. This module incorporates with
  OpensbiPlatformLib and RISC-V Opensbi library.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <Library/RiscVSpecialPlatformLib.h>

const struct platform_override  *special_platforms      = NULL;
const struct platform_override  *SpecialPlatformArray   = NULL;
INTN                            NumberOfPlaformsInArray = 0;
