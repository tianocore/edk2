/** @file

  Implementation of the SNP.Shutdown() function and its private helpers if any.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Resets a network adapter and leaves it in a state that is safe for  another
  driver to initialize.

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was shutdown.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/

EFI_STATUS
EFIAPI
VirtioNetShutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This
  )
{
  VNET_DEV   *Dev;
  EFI_TPL    OldTpl;
  EFI_STATUS Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  switch (Dev->Snm.State) {
  case EfiSimpleNetworkStopped:
    Status = EFI_NOT_STARTED;
    goto Exit;
  case EfiSimpleNetworkStarted:
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  default:
    break;
  }

  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);
  VirtioNetShutdownRx (Dev);
  VirtioNetShutdownTx (Dev);
  VirtioNetUninitRing (Dev, &Dev->TxRing, Dev->TxRingMap);
  VirtioNetUninitRing (Dev, &Dev->RxRing, Dev->RxRingMap);

  Dev->Snm.State = EfiSimpleNetworkStarted;
  Status = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
