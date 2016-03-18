/** @file

  Implementation of the SNP.ReceiveFilters() function and its private helpers
  if any.

  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiBootServicesTableLib.h>

#include "VirtioNet.h"

/**
  Manages the multicast receive filters of a network interface.

  @param  This             The protocol instance pointer.
  @param  Enable           A bit mask of receive filters to enable on the
                           network interface.
  @param  Disable          A bit mask of receive filters to disable on the
                           network interface.
  @param  ResetMCastFilter Set to TRUE to reset the contents of the multicast
                           receive filters on the network interface to their
                           default values.
  @param  McastFilterCnt   Number of multicast HW MAC addresses in the new
                           MCastFilter list. This value must be less than or
                           equal to the MCastFilterCnt field of
                           EFI_SIMPLE_NETWORK_MODE. This field is optional if
                           ResetMCastFilter is TRUE.
  @param  MCastFilter      A pointer to a list of new multicast receive filter
                           HW MAC addresses. This list will replace any
                           existing multicast HW MAC address list. This field
                           is optional if ResetMCastFilter is TRUE.

  @retval EFI_SUCCESS           The multicast receive filter list was updated.
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
VirtioNetReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN UINT32                      Enable,
  IN UINT32                      Disable,
  IN BOOLEAN                     ResetMCastFilter,
  IN UINTN                       MCastFilterCnt    OPTIONAL,
  IN EFI_MAC_ADDRESS             *MCastFilter      OPTIONAL
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

  //
  // MNP apparently fails to initialize on top of us if we simply return
  // EFI_UNSUPPORTED in this function.
  //
  // Hence we openly refuse multicast functionality, and fake the rest by
  // selecting a no stricter filter setting than whatever is requested. The
  // UEFI-2.3.1+errC spec allows this. In practice we don't change our current
  // (default) filter. Additionally, receiving software is responsible for
  // discarding any packets getting through the filter.
  //
  Status = (
    ((Enable | Disable) & ~Dev->Snm.ReceiveFilterMask) != 0 ||
    (!ResetMCastFilter && MCastFilterCnt > Dev->Snm.MaxMCastFilterCount)
    ) ? EFI_INVALID_PARAMETER : EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
