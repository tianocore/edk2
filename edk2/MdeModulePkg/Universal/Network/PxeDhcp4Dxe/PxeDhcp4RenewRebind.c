/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeDhcp4RenewRebind.c

Abstract:


**/


#include "PxeDhcp4.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**
  Parameters:

  @return -2 = ignore, stop waiting
  @return -1 = ignore, keep waiting
  @return 0 = accept, keep waiting
  @return 1 = accept, stop waiting

**/
STATIC
INTN
acknak_verify (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN DHCP4_PACKET           *tx_pkt,
  IN DHCP4_PACKET           *rx_pkt,
  IN UINTN                  rx_pkt_size
  )
{
  EFI_STATUS  efi_status;
  DHCP4_OP    *msg_type_op;
  DHCP4_OP    *srvid_op;
  DHCP4_OP    *renew_op;
  DHCP4_OP    *rebind_op;
  DHCP4_OP    *lease_time_op;
  UINT32      magik;

  //
  // Verify parameters.  Unused parameters are also touched
  // to make the compiler happy.
  //
  ASSERT (Private);
  ASSERT (rx_pkt);

  if (Private == NULL || rx_pkt == NULL) {
    return -2;
  }

  tx_pkt      = tx_pkt;
  rx_pkt_size = rx_pkt_size;

  //
  // This must be a DHCP Ack message.
  //
  magik = htonl (DHCP4_MAGIK_NUMBER);

  if (CompareMem (&rx_pkt->dhcp4.magik, &magik, 4)) {
    return -1;
  }

  efi_status = find_opt (rx_pkt, DHCP4_MESSAGE_TYPE, 0, &msg_type_op);

  if (EFI_ERROR (efi_status)) {
    return -1;
  }

  if (msg_type_op->len != 1) {
    return -1;
  }

  if (msg_type_op->data[0] != DHCP4_MESSAGE_TYPE_ACK) {
    return -1;
  }
  //
  // There must be a server identifier.
  //
  efi_status = find_opt (rx_pkt, DHCP4_SERVER_IDENTIFIER, 0, &srvid_op);

  if (EFI_ERROR (efi_status)) {
    return -1;
  }

  if (srvid_op->len != 4) {
    return -1;
  }
  //
  // There should be a renewal time.
  // If there is not, we will default to the 7/8 of the rebinding time.
  //
  efi_status = find_opt (rx_pkt, DHCP4_RENEWAL_TIME, 0, &renew_op);

  if (EFI_ERROR (efi_status)) {
    renew_op = NULL;
  } else if (renew_op->len != 4) {
    renew_op = NULL;
  }
  //
  // There should be a rebinding time.
  // If there is not, we will default to 7/8 of the lease time.
  //
  efi_status = find_opt (rx_pkt, DHCP4_REBINDING_TIME, 0, &rebind_op);

  if (EFI_ERROR (efi_status)) {
    rebind_op = NULL;
  } else if (rebind_op->len != 4) {
    rebind_op = NULL;
  }
  //
  // There should be a lease time.
  // If there is not, we will default to one week.
  //
  efi_status = find_opt (rx_pkt, DHCP4_LEASE_TIME, 0, &lease_time_op);

  if (EFI_ERROR (efi_status)) {
    lease_time_op = NULL;
  } else if (lease_time_op->len != 4) {
    lease_time_op = NULL;
  }
  //
  // Packet looks good.  Double check the renew, rebind and lease times.
  //
  CopyMem (&Private->ServerIp, srvid_op->data, 4);

  if (renew_op != NULL) {
    CopyMem (&Private->RenewTime, renew_op->data, 4);
    Private->RenewTime = htonl (Private->RenewTime);
  } else {
    Private->RenewTime = 0;
  }

  if (rebind_op != NULL) {
    CopyMem (&Private->RebindTime, rebind_op->data, 4);
    Private->RebindTime = htonl (Private->RebindTime);
  } else {
    Private->RebindTime = 0;
  }

  if (lease_time_op != NULL) {
    CopyMem (&Private->LeaseTime, lease_time_op->data, 4);
    Private->LeaseTime = htonl (Private->LeaseTime);
  } else {
    Private->LeaseTime = 0;
  }

  if (Private->LeaseTime < 60) {
    Private->LeaseTime = 7 * 86400;
  }

  if (Private->RebindTime < 52 || Private->RebindTime >= Private->LeaseTime) {
    Private->RebindTime = Private->LeaseTime / 2 + Private->LeaseTime / 4 + Private->LeaseTime / 8;
  }

  if (Private->RenewTime < 45 || Private->RenewTime >= Private->RebindTime) {
    Private->RenewTime = Private->RebindTime / 2 + Private->RebindTime / 4 + Private->RebindTime / 8;
  }

  return 1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
STATIC
EFI_STATUS
EFIAPI
renew_rebind (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout,
  IN BOOLEAN                renew
  )
{
  PXE_DHCP4_PRIVATE_DATA  *Private;
  EFI_IP_ADDRESS          ServerIp;
  EFI_IP_ADDRESS          client_ip;
  EFI_IP_ADDRESS          subnet_mask;
  EFI_IP_ADDRESS          gateway_ip;
  DHCP4_PACKET            Request;
  DHCP4_PACKET            AckNak;
  DHCP4_OP                *op;
  EFI_STATUS              efi_status;

  //
  // Check for invalid parameters.
  //
  if (This == NULL || seconds_timeout < DHCP4_MIN_SECONDS || seconds_timeout > DHCP4_MAX_SECONDS) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check for proper protocol state.
  //
  if (This->Data == NULL) {
    return EFI_NOT_STARTED;
  }

  if (!This->Data->SelectCompleted) {
    return EFI_NOT_READY;
  }

  if (This->Data->IsBootp) {
    return EFI_SUCCESS;
  }

  if (!This->Data->IsAck) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get pointer to instance data.
  //
  Private = PXE_DHCP4_PRIVATE_DATA_FROM_THIS (This);

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->PxeBc == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Copy Discover packet to temporary request packet
  // to be used for Renew/Rebind operation.
  //
  CopyMem (&Request, &This->Data->Discover, sizeof (DHCP4_PACKET));

  CopyMem (&Request.dhcp4.ciaddr, &This->Data->AckNak.dhcp4.yiaddr, 4);

  Request.dhcp4.flags = 0;  /* Reply does not need to be broadcast. */

  //
  // Change message type from discover to request.
  //
  efi_status = find_opt (&Request, DHCP4_MESSAGE_TYPE, 0, &op);

  if (EFI_ERROR (efi_status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (op->len != 1) {
    return EFI_INVALID_PARAMETER;
  }

  op->data[0] = DHCP4_MESSAGE_TYPE_REQUEST;

  //
  // Need a subnet mask.
  //
  efi_status = find_opt (
                &This->Data->AckNak,
                DHCP4_SUBNET_MASK,
                0,
                &op
                );

  if (EFI_ERROR (efi_status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (op->len != 4) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&subnet_mask, sizeof (EFI_IP_ADDRESS));
  CopyMem (&subnet_mask, op->data, 4);

  //
  // Need a server IP address (renew) or a broadcast
  // IP address (rebind).
  //
  ZeroMem (&gateway_ip, sizeof (EFI_IP_ADDRESS));

  if (renew) {
    efi_status = find_opt (
                  &This->Data->AckNak,
                  DHCP4_SERVER_IDENTIFIER,
                  0,
                  &op
                  );

    if (EFI_ERROR (efi_status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (op->len != 4) {
      return EFI_INVALID_PARAMETER;
    }

    ZeroMem (&ServerIp, sizeof (EFI_IP_ADDRESS));
    CopyMem (&ServerIp, op->data, 4);

    //
    //
    //
    if (CompareMem (&This->Data->AckNak.dhcp4.giaddr, &gateway_ip, 4)) {
      CopyMem (&gateway_ip, &This->Data->AckNak.dhcp4.giaddr, 4);
    }
  } else {
    SetMem (&ServerIp, sizeof (EFI_IP_ADDRESS), 0xFF);
  }
  //
  // Need a client IP address.
  //
  ZeroMem (&client_ip, sizeof (EFI_IP_ADDRESS));
  CopyMem (&client_ip, &Request.dhcp4.ciaddr, 4);

  //
  //
  //
  efi_status = gBS->HandleProtocol (
                      Private->Handle,
                      &gEfiPxeDhcp4CallbackProtocolGuid,
                      (VOID *) &Private->callback
                      );

  if (EFI_ERROR (efi_status)) {
    Private->callback = NULL;
  }

  Private->function = renew ? EFI_PXE_DHCP4_FUNCTION_RENEW : EFI_PXE_DHCP4_FUNCTION_REBIND;

  //
  // Transimit DHCP request and wait for DHCP ack...
  //
  efi_status = tx_rx_udp (
                Private,
                &ServerIp,
                &gateway_ip,
                &client_ip,
                &subnet_mask,
                &Request,
                &AckNak,
                &acknak_verify,
                seconds_timeout
                );

  if (EFI_ERROR (efi_status)) {
    Private->callback = NULL;
    return efi_status;
  }
  //
  // Copy server identifier, renewal time and rebinding time
  // from temporary ack/nak packet into cached ack/nak packet.
  //
  efi_status = find_opt (
                &This->Data->AckNak,
                DHCP4_SERVER_IDENTIFIER,
                0,
                &op
                );

  if (!EFI_ERROR (efi_status)) {
    if (op->len == 4) {
      CopyMem (op->data, &Private->ServerIp, 4);
    }
  }

  efi_status = find_opt (&This->Data->AckNak, DHCP4_RENEWAL_TIME, 0, &op);

  if (!EFI_ERROR (efi_status)) {
    if (op->len == 4) {
      CopyMem (op->data, &Private->RenewTime, 4);
    }
  }

  efi_status = find_opt (&This->Data->AckNak, DHCP4_REBINDING_TIME, 0, &op);

  if (!EFI_ERROR (efi_status)) {
    if (op->len == 4) {
      CopyMem (op->data, &Private->RebindTime, 4);
    }
  }

  Private->callback = NULL;
  return efi_status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4Renew (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout
  )
{
  return renew_rebind (This, seconds_timeout, TRUE);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4Rebind (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout
  )
{
  return renew_rebind (This, seconds_timeout, FALSE);
}

/* eof - PxeDhcp4RenewRebind.c */
