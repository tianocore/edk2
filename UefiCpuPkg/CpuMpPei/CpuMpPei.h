/** @file
  Definitions to install Multiple Processor PPI.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _CPU_MP_PEI_H_
#define _CPU_MP_PEI_H_

#include <PiPei.h>

#include <Ppi/MpServices.h>
#include <Ppi/SecPlatformInformation.h>
#include <Ppi/SecPlatformInformation2.h>
#include <Ppi/EndOfPeiPhase.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/LocalApicLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/MpInitLib.h>

extern EFI_PEI_PPI_DESCRIPTOR   mPeiCpuMpPpiDesc;


/**
  Collects BIST data from PPI.

  This function collects BIST data from Sec Platform Information2 PPI
  or SEC Platform Information PPI.

  @param PeiServices         Pointer to PEI Services Table

**/
VOID
CollectBistDataFromPpi (
  IN CONST EFI_PEI_SERVICES             **PeiServices
  );

/**
  Implementation of the PlatformInformation2 service in EFI_SEC_PLATFORM_INFORMATION2_PPI.

  @param  PeiServices                The pointer to the PEI Services Table.
  @param  StructureSize              The pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord2 The pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD2.

  @retval EFI_SUCCESS                The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL       The buffer was too small. The current buffer size needed to
                                     hold the record is returned in StructureSize.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation2 (
  IN CONST EFI_PEI_SERVICES                   **PeiServices,
  IN OUT UINT64                               *StructureSize,
     OUT EFI_SEC_PLATFORM_INFORMATION_RECORD2 *PlatformInformationRecord2
  );

#endif
