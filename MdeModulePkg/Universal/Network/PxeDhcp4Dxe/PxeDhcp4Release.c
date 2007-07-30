/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeDhcp4Release.c

Abstract:
  Transmit release packet, free allocations and shutdown PxeDhcp4.


**/


#include "PxeDhcp4.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4Release (
  IN EFI_PXE_DHCP4_PROTOCOL *This
  )
{
  PXE_DHCP4_PRIVATE_DATA  *Private;
  EFI_IP_ADDRESS          ServerIp;
  EFI_IP_ADDRESS          client_ip;
  EFI_IP_ADDRESS          gateway_ip;
  EFI_IP_ADDRESS          subnet_mask;
  EFI_STATUS              efi_status;
  DHCP4_OP                *op;
  UINT8                   op_list[20];

  //
  // Check for invalid parameters.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Release does nothing if the protocol has never been setup.
  //
  if (This->Data == NULL) {
    return EFI_NOT_STARTED;
  }
  //
  // Fail if we do not have valid instance data.
  //
  Private = PXE_DHCP4_PRIVATE_DATA_FROM_THIS (This);

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->PxeBc == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // If this is a BOOTP session and there is not a DHCP Ack
  // packet, just release storage and return.
  //
  if (This->Data->IsBootp || !This->Data->IsAck) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return EFI_SUCCESS;
  }
  //
  // Build option list for DHCP Release packet.
  // If any errors occur, just release storage and return.
  //
  //
  // Message type is first.
  //
  op_list[0]  = DHCP4_MESSAGE_TYPE;
  op_list[1]  = 1;
  op_list[2]  = DHCP4_MESSAGE_TYPE_RELEASE;

  //
  // Followed by server identifier.
  //
  efi_status = find_opt (
                &This->Data->Request,
                DHCP4_SERVER_IDENTIFIER,
                0,
                &op
                );

  if (EFI_ERROR (efi_status)) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return EFI_SUCCESS;
  }

  if (op->len != 4) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return EFI_SUCCESS;
  }

  CopyMem (&ServerIp, op->data, 4);

  op_list[3]  = DHCP4_SERVER_IDENTIFIER;
  op_list[4]  = 4;
  CopyMem (&op_list[5], &ServerIp, 4);

  //
  // Followed by end.
  //
  op_list[9] = DHCP4_END;

  //
  // We need a subnet mask for IP stack operation.
  //
  efi_status = find_opt (
                &This->Data->AckNak,
                DHCP4_SUBNET_MASK,
                0,
                &op
                );

  if (EFI_ERROR (efi_status)) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return EFI_SUCCESS;
  }

  if (op->len != 4) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return EFI_SUCCESS;
  }

  ZeroMem (&subnet_mask, sizeof (EFI_IP_ADDRESS));
  CopyMem (&subnet_mask, op->data, 4);

  //
  // Gateway IP address may be needed.
  //
  ZeroMem (&gateway_ip, sizeof (EFI_IP_ADDRESS));
  CopyMem (&gateway_ip, &This->Data->AckNak.dhcp4.giaddr, 4);

  //
  // Client IP address needed for IP stack operation.
  //
  ZeroMem (&client_ip, sizeof (EFI_IP_ADDRESS));
  CopyMem (&client_ip, &This->Data->AckNak.dhcp4.yiaddr, 4);

  //
  // Enable UDP...
  //
  efi_status = start_udp (Private, &client_ip, &subnet_mask);

  if (EFI_ERROR (efi_status)) {
    gBS->FreePool (This->Data);
    This->Data = NULL;

    if (Private->StopPxeBc) {
      Private->PxeBc->Stop (Private->PxeBc);
    }

    return efi_status;
  }
  //
  // Gather information out of DHCP request packet needed for
  // DHCP release packet.
  //
  //
  // Setup DHCP Release packet.
  //
  CopyMem (&This->Data->Request.dhcp4.ciaddr, &client_ip, 4);

  ZeroMem (&This->Data->Request.dhcp4.yiaddr, 12);

  ZeroMem (&This->Data->Request.dhcp4.sname, 64 + 128);

  This->Data->Request.dhcp4.hops  = 0;
  This->Data->Request.dhcp4.secs  = 0;
  This->Data->Request.dhcp4.flags = 0;

  ZeroMem (
    &This->Data->Request.dhcp4.options,
    sizeof This->Data->Request.dhcp4.options
    );

  CopyMem (&This->Data->Request.dhcp4.options, op_list, 10);

  //
  // Transmit DHCP Release packet.
  //
  tx_udp (
    Private,
    &ServerIp,
    &gateway_ip,
    &client_ip,
    &This->Data->Request,
    DHCP4_MAX_PACKET_SIZE - (DHCP4_UDP_HEADER_SIZE + DHCP4_IP_HEADER_SIZE)
    );

  gBS->Stall (1000000); /* 1/10th second */

  //
  // Shutdown PXE BaseCode and release local storage.
  //
  stop_udp (Private);

  gBS->FreePool (This->Data);
  This->Data = NULL;

  if (Private->StopPxeBc) {
    Private->PxeBc->Stop (Private->PxeBc);
  }

  return EFI_SUCCESS;
}

/* eof - PxeDhcp4Release.c */
