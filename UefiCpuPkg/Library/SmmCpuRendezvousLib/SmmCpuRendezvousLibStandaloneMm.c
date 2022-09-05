/** @file
SMM CPU Rendezvous library header file.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
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
SmmCpuRendezvousProtocolNotify (
  IN CONST EFI_GUID  *Protocol,
  IN VOID            *Interface,
  IN EFI_HANDLE      Handle
  )
{
  EFI_STATUS  Status;

  Status = gMmst->MmLocateProtocol (
                    &gEdkiiSmmCpuRendezvousProtocolGuid,
                    NULL,
                    (VOID **)&mSmmCpuRendezvous
                    );

  return EFI_SUCCESS;
}

/**
  The constructor function

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS      The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCpuRendezvousLibStandaloneConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  Status = gMmst->MmLocateProtocol (&gEdkiiSmmCpuRendezvousProtocolGuid, NULL, (VOID **)&mSmmCpuRendezvous);
  if (EFI_ERROR (Status)) {
    Status = gMmst->MmRegisterProtocolNotify (
                      &gEdkiiSmmCpuRendezvousProtocolGuid,
                      SmmCpuRendezvousProtocolNotify,
                      &Registration
                      );
  }

  return EFI_SUCCESS;
}
