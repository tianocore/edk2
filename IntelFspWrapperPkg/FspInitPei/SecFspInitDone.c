/** @file
  Install FspInitDone PPI and GetFspHobList API.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "SecMain.h"

FSP_INIT_DONE_PPI gFspInitDonePpi = {
  FspInitDoneGetFspHobList
};

/**
  Return Hob list produced by FSP.

  @param[in]  PeiServices  The pointer to the PEI Services Table.
  @param[in]  This         The pointer to this instance of this PPI.
  @param[out] FspHobList   The pointer to Hob list produced by FSP.

  @return EFI_SUCCESS FReturn Hob list produced by FSP successfully.
**/
EFI_STATUS
EFIAPI
FspInitDoneGetFspHobList (
  IN  CONST EFI_PEI_SERVICES         **PeiServices,
  IN  FSP_INIT_DONE_PPI              *This,
  OUT VOID                           **FspHobList
  )
{
  VOID        *TopOfTemporaryRamPpi;
  EFI_STATUS  Status;

  Status = (*PeiServices)->LocatePpi (
                             PeiServices,
                             &gTopOfTemporaryRamPpiGuid,
                             0,
                             NULL,
                             (VOID **) &TopOfTemporaryRamPpi
                             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  *FspHobList = (VOID *)(UINTN)(*(UINT32 *)((UINTN)TopOfTemporaryRamPpi - sizeof(UINT32)));

  return EFI_SUCCESS;
}

