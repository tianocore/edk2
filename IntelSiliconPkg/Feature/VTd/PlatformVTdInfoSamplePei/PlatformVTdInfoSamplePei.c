/** @file
  Platform VTd Info Sample PEI driver.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Ppi/VTdInfo.h>

#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>

typedef struct {
  UINT64                                  Revision;
  UINT8                                   HostAddressWidth;
  UINT8                                   Reserved[3];
  UINT32                                  VTdEngineCount;
  UINT64                                  VTdEngineAddress[2];
} MY_VTD_INFO_PPI;

MY_VTD_INFO_PPI  mPlatformVTdSample = {
  EDKII_VTD_INFO_PPI_REVISION,
  0x26,
  {0},
  2,
  {0xFED90000, 0xFED91000},
};

EFI_PEI_PPI_DESCRIPTOR mPlatformVTdInfoSampleDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiVTdInfoPpiGuid,
  &mPlatformVTdSample
};

/**
  Platform VTd Info sample driver.

  @param[in] FileHandle  Handle of the file being invoked.
  @param[in] PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS if it completed successfully.
**/
EFI_STATUS
EFIAPI
PlatformVTdInfoSampleInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&mPlatformVTdInfoSampleDesc);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
