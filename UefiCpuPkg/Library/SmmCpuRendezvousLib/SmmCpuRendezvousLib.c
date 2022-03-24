/** @file
  SMM CPU Rendezvous sevice implement.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
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
STATIC VOID                               *mRegistration     = NULL;

/**
  Callback function to wait Smm cpu rendezvous service located.

  SmmCpuRendezvousLib need to support MM_STANDALONE and DXE_SMM_DRIVER driver.
  So do not use library constructor to locate the protocol.

  @param[in] Protocol   Points to the protocol's unique identifier.
  @param[in] Interface  Points to the interface instance.
  @param[in] Handle     The handle on which the interface was installed.

  @retval EFI_SUCCESS  Notification runs successfully.

**/
EFI_STATUS
EFIAPI
SmmCpuRendezvousProtocolNotify (
  IN CONST EFI_GUID    *Protocol,
  IN       VOID        *Interface,
  IN       EFI_HANDLE  Handle
  )
{
  EFI_STATUS  Status;

  Status = gMmst->MmLocateProtocol (
                    &gEdkiiSmmCpuRendezvousProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpuRendezvous
                    );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  This routine wait for all AP processors to arrive in SMM.

  @param[in] BlockingMode  Blocking mode or non-blocking mode.

  @retval EFI_SUCCESS  All avaiable APs arrived.
  @retval EFI_TIMEOUT  Wait for all APs until timeout.
  @retval OTHER        Fail to register SMM CPU Rendezvous service Protocol.
**/
EFI_STATUS
EFIAPI
SmmWaitForAllProcessor (
  IN BOOLEAN  BlockingMode
  )
{
  EFI_STATUS  Status;

  if ((mRegistration == NULL) && (mSmmCpuRendezvous == NULL)) {
    //
    // Locate SMM cpu rendezvous protocol for the first time execute the function.
    //
    Status = gMmst->MmLocateProtocol (
                      &gEdkiiSmmCpuRendezvousProtocolGuid,
                      NULL,
                      (VOID **)&mSmmCpuRendezvous
                      );
    if (EFI_ERROR (Status)) {
      Status = gMmst->MmRegisterProtocolNotify (
                        &gEdkiiSmmCpuRendezvousProtocolGuid,
                        SmmCpuRendezvousProtocolNotify,
                        &mRegistration
                        );
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // The platform have not set up. It doesn't need smm cpu rendezvous.
  //
  if (mSmmCpuRendezvous == NULL) {
    return EFI_SUCCESS;
  }

  Status = mSmmCpuRendezvous->WaitForAllProcessor (
                                mSmmCpuRendezvous,
                                BlockingMode
                                );
  return Status;
}
