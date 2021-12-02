/** @file

  Implementation of the SNP.Start() function and its private helpers if any.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Changes the state of a network interface from "stopped" to "started".

  @param  This Protocol instance pointer.

  @retval EFI_SUCCESS           The network interface was started.
  @retval EFI_ALREADY_STARTED   The network interface is already in the started
                                state.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.
**/
EFI_STATUS
EFIAPI
VirtioNetStart (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  )
{
  VNET_DEV    *Dev;
  EFI_TPL     OldTpl;
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev    = VIRTIO_NET_FROM_SNP (This);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (Dev->Snm.State != EfiSimpleNetworkStopped) {
    Status = EFI_ALREADY_STARTED;
  } else {
    Dev->Snm.State = EfiSimpleNetworkStarted;
    Status         = EFI_SUCCESS;
  }

  gBS->RestoreTPL (OldTpl);
  return Status;
}
