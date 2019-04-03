/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

--*/

#ifndef _PLATFORM_FSP_LIB_H
#define _PLATFORM_FSP_LIB_H
#include <Base.h>
#include <Uefi.h>

extern
EFI_STATUS
PlatformHobCreateFromFsp (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  VOID                          *HobList
  );

#endif
