/** @file
  In EndOfPei notify, it will call FspNotifyPhase API.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  EFI_STATUS          Status;
  FSP_INFO_HEADER     *FspHeader;

  FspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  if (FspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }
  
  DEBUG ((DEBUG_INFO, "S3EndOfPeiNotify enter\n"));
  
  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  Status = CallFspNotifyPhase (FspHeader, &NotifyPhaseParams);
  DEBUG((DEBUG_INFO, "FSP S3NotifyPhase AfterPciEnumeration status: 0x%x\n", Status));

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  Status = CallFspNotifyPhase (FspHeader, &NotifyPhaseParams);
  DEBUG((DEBUG_INFO, "FSP S3NotifyPhase ReadyToBoot status: 0x%x\n", Status));

  return EFI_SUCCESS;
}
