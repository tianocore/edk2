/** @file
SMM CPU Rendezvous library header file.

Copyright (c) 2021 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Base.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MmServicesTableLib.h>
#include <Protocol/SmmCpuService.h>
#include <Library/SmmCpuRendezvousLib.h>

STATIC EDKII_SMM_CPU_RENDEZVOUS_PROTOCOL  *mSmmCpuRendezvous = NULL;
STATIC VOID                               *mSmmCpuRendezvousRegistration = NULL;

/**
  Register status code callback function only when Report Status Code protocol
  is installed.

  @param Protocol       Points to the protocol's unique identifier.
  @param Interface      Points to the interface instance.
  @param Handle         The handle on which the interface was installed.

  @retval EFI_SUCCESS   Notification runs successfully.

**/
EFI_STATUS
EFIAPI
SmmCpuServiceProtocolNotify (
  IN CONST EFI_GUID        *Protocol,
  IN VOID                  *Interface,
  IN EFI_HANDLE            Handle
  )
{
  EFI_STATUS                   Status;

  Status = gMmst->MmLocateProtocol (
                    &gEdkiiSmmCpuRendezvousProtocolGuid,
                    NULL,
                    (VOID **) &mSmmCpuRendezvous
                    );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  This routine wait for all AP processors to arrive in SMM.

  @param  BlockingMode  Blocking mode or non-blocking mode.

  @retval EFI_SUCCESS            All avaiable APs arrived.
  @retval EFI_TIMEOUT            Wait for all APs until timeout.
  @retval Other                  Fail to register Smm cpu rendezvous services notify.
**/
EFI_STATUS
EFIAPI
SmmWaitForAllProcessor (
  IN  BOOLEAN  BlockingMode
  )
{
  EFI_STATUS  Status;

  if (mSmmCpuRendezvousRegistration == NULL && mSmmCpuRendezvous == NULL) {
    Status = gMmst->MmLocateProtocol (&gEdkiiSmmCpuRendezvousProtocolGuid, NULL, (VOID **) &mSmmCpuRendezvous);
    if (EFI_ERROR (Status)) {
      Status = gMmst->MmRegisterProtocolNotify (
                &gEdkiiSmmCpuRendezvousProtocolGuid,
                SmmCpuServiceProtocolNotify,
                &mSmmCpuRendezvousRegistration
                );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    } 
  }

  if (mSmmCpuRendezvous == NULL) {
    return EFI_SUCCESS;
  }

  Status = mSmmCpuRendezvous->WaitForAllProcessor (
              mSmmCpuRendezvous,
              BlockingMode
              );
  return Status;
}
