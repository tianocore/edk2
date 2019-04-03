/** @file
  This is PEIM header file.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FSP_INIT_PEI_H_
#define _FSP_INIT_PEI_H_

#include <PiPei.h>

#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/FspPlatformInfoLib.h>
#include <Library/FspPlatformSecLib.h>
#include <Library/FspHobProcessLib.h>
#include <Library/FspApiLib.h>

#include <Ppi/FspInitDone.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/TemporaryRamDone.h>

extern EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc;

/**
  Do FSP initialization based on FspApi version 1.

  @param[in] FspHeader FSP header pointer.

  @return FSP initialization status.
**/
EFI_STATUS
PeiFspInitV1 (
  IN FSP_INFO_HEADER *FspHeader
  );

/**
  Do FSP initialization based on FspApi version 2.

  @param[in] FspHeader FSP header pointer.

  @return FSP initialization status.
**/
EFI_STATUS
PeiFspInitV2 (
  IN FSP_INFO_HEADER *FspHeader
  );

#endif
