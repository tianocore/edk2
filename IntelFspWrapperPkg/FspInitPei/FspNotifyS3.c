/** @file
  In EndOfPei notify, it will call FspNotifyPhase API.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "FspInitPei.h"

/**
  This function handles S3 resume task at the end of PEI

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  );

EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  S3EndOfPeiNotify
};

/**
  This function handles S3 resume task at the end of PEI

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  NOTIFY_PHASE_PARAMS NotifyPhaseParams;
  FSP_STATUS          FspStatus;
  FSP_INFO_HEADER     *FspHeader;

  FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  FspStatus = CallFspNotifyPhase (FspHeader, &NotifyPhaseParams);
  if (FspStatus != FSP_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP S3NotifyPhase AfterPciEnumeration failed, status: 0x%x\n", FspStatus));
  } else {
    DEBUG((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration Success.\n"));
  }

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  FspStatus = CallFspNotifyPhase (FspHeader, &NotifyPhaseParams);
  if (FspStatus != FSP_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP S3NotifyPhase ReadyToBoot failed, status: 0x%x\n", FspStatus));
  } else {
    DEBUG((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot Success.\n"));
  }

  return EFI_SUCCESS;
}
