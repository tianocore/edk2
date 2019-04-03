/**@file
  Jumper setting for multiplatform.

  This file includes package header files, library classes.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   
**/

#ifndef _BOARDJUMPERS_H_
#define _BOARDJUMPERS_H_

#include <PiPei.h>
#include "PchAccess.h"
#include "PlatformBaseAddresses.h"

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Guid/PlatformInfo.h>

BOOLEAN
IsRecoveryJumper (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN OUT EFI_PLATFORM_INFO_HOB *PlatformInfoHob
);

#endif
