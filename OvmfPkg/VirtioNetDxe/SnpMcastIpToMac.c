/** @file

  Implementation of the SNP.McastIpToMac() function and its private helpers if
  any.

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
  Converts a multicast IP address to a multicast HW MAC address.

  @param  This The protocol instance pointer.
  @param  IPv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460]. Set
               to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param  IP   The multicast IP address that is to be converted to a multicast
               HW MAC address.
  @param  MAC  The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the
                                multicast HW MAC address.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_BUFFER_TOO_SMALL  The Statistics buffer was too small. The
                                current buffer size needed to hold the
                                statistics is returned in StatisticsSize.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an
                                unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/

EFI_STATUS
EFIAPI
VirtioNetMcastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL *This,
  IN BOOLEAN                     IPv6,
  IN EFI_IP_ADDRESS              *Ip,
  OUT EFI_MAC_ADDRESS            *Mac
  )
{
  VNET_DEV   *Dev;
  EFI_TPL    OldTpl;
  EFI_STATUS Status;

  //
  // http://en.wikipedia.org/wiki/Multicast_address
  //
  if (This == NULL || Ip == NULL || Mac == NULL ||
      ( IPv6 && (Ip->v6.Addr[0]       ) != 0xFF) || // invalid IPv6 mcast addr
      (!IPv6 && (Ip->v4.Addr[0] & 0xF0) != 0xE0)    // invalid IPv4 mcast addr
      ) {
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
  // http://en.wikipedia.org/wiki/IP_multicast#Layer_2_delivery
  //
  if (IPv6) {
    Mac->Addr[0] = 0x33;
    Mac->Addr[1] = 0x33;
    Mac->Addr[2] = Ip->v6.Addr[12];
    Mac->Addr[3] = Ip->v6.Addr[13];
    Mac->Addr[4] = Ip->v6.Addr[14];
    Mac->Addr[5] = Ip->v6.Addr[15];
  }
  else {
    Mac->Addr[0] = 0x01;
    Mac->Addr[1] = 0x00;
    Mac->Addr[2] = 0x5E;
    Mac->Addr[3] = Ip->v4.Addr[1] & 0x7F;
    Mac->Addr[4] = Ip->v4.Addr[2];
    Mac->Addr[5] = Ip->v4.Addr[3];
  }
  Status = EFI_SUCCESS;

Exit:
  gBS->RestoreTPL (OldTpl);
  return Status;
}
