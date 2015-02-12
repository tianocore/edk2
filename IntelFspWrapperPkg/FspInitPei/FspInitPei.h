/** @file
  This is PEIM header file.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
