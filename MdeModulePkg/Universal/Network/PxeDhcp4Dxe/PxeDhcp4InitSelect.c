/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeDhcp4InitSelect.c

Abstract:


**/


#include "PxeDhcp4.h"

#define DebugPrint(x)
//
// #define DebugPrint(x) Aprint x
//

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

**/
STATIC
INTN
offer_verify (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN DHCP4_PACKET           *tx_pkt,
  IN DHCP4_PACKET           *rx_pkt,
  IN UINTN                  rx_pkt_size
  )
{
  EFI_STATUS    EfiStatus;
  DHCP4_PACKET  *tmp;
  DHCP4_OP      *msg_type_op;
  DHCP4_OP      *srvid_op;
  UINT32        magik;

  //
  // Verify parameters.  Touch unused parameters to keep
  // compiler happy.
  //
  ASSERT (Private);
  ASSERT (rx_pkt);

  if (Private == NULL || rx_pkt == NULL) {
    return -2;
  }

  tx_pkt      = tx_pkt;
  rx_pkt_size = rx_pkt_size;

  //
  // This may be a BOOTP Reply or DHCP Offer packet.
  // If there is no DHCP magik number, assume that
  // this is a BOOTP Reply packet.
  //
  magik = htonl (DHCP4_MAGIK_NUMBER);

  while (!CompareMem (&rx_pkt->dhcp4.magik, &magik, 4)) {
    //
    // If there is no DHCP message type option, assume
    // this is a BOOTP reply packet and cache it.
    //
    EfiStatus = find_opt (rx_pkt, DHCP4_MESSAGE_TYPE, 0, &msg_type_op);

    if (EFI_ERROR (EfiStatus)) {
      break;
    }
    //
    // If there is a DHCP message type option, it must be a
    // DHCP offer packet
    //
    if (msg_type_op->len != 1) {
      return -1;
    }

    if (msg_type_op->data[0] != DHCP4_MESSAGE_TYPE_OFFER) {
      return -1;
    }
    //
    // There must be a server identifier option.
    //
    EfiStatus = find_opt (
                  rx_pkt,
                  DHCP4_SERVER_IDENTIFIER,
                  0,
                  &srvid_op
                  );

    if (EFI_ERROR (EfiStatus)) {
      return -1;
    }

    if (srvid_op->len != 4) {
      return -1;
    }
    //
    // Good DHCP offer packet.
    //
    break;
  }
  //
  // Good DHCP (or BOOTP) packet.  Cache it!
  //
  EfiStatus = gBS->AllocatePool (
                    EfiBootServicesData,
                    (Private->offers + 1) * sizeof (DHCP4_PACKET),
                    (VOID **) &tmp
                    );

  if (EFI_ERROR (EfiStatus)) {
    return -2;
  }

  ASSERT (tmp);

  if (Private->offers != 0) {
    CopyMem (
      tmp,
      Private->offer_list,
      Private->offers * sizeof (DHCP4_PACKET)
      );

    gBS->FreePool (Private->offer_list);
  }

  CopyMem (&tmp[Private->offers++], rx_pkt, sizeof (DHCP4_PACKET));

  Private->offer_list = tmp;

  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

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
  EFI_STATUS  EfiStatus;
  DHCP4_OP    *msg_type_op;
  DHCP4_OP    *srvid_op;
  DHCP4_OP    *renew_op;
  DHCP4_OP    *rebind_op;
  DHCP4_OP    *lease_time_op;
  UINT32      magik;

  //
  // Verify parameters.  Touch unused parameters to
  // keep compiler happy.
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

  EfiStatus = find_opt (rx_pkt, DHCP4_MESSAGE_TYPE, 0, &msg_type_op);

  if (EFI_ERROR (EfiStatus)) {
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
  EfiStatus = find_opt (rx_pkt, DHCP4_SERVER_IDENTIFIER, 0, &srvid_op);

  if (EFI_ERROR (EfiStatus)) {
    return -1;
  }

  if (srvid_op->len != 4) {
    return -1;
  }
  //
  // There should be a renewal time.
  // If there is not, we will default to the 7/8 of the rebinding time.
  //
  EfiStatus = find_opt (rx_pkt, DHCP4_RENEWAL_TIME, 0, &renew_op);

  if (EFI_ERROR (EfiStatus)) {
    renew_op = NULL;
  } else if (renew_op->len != 4) {
    renew_op = NULL;
  }
  //
  // There should be a rebinding time.
  // If there is not, we will default to 7/8 of the lease time.
  //
  EfiStatus = find_opt (rx_pkt, DHCP4_REBINDING_TIME, 0, &rebind_op);

  if (EFI_ERROR (EfiStatus)) {
    rebind_op = NULL;
  } else if (rebind_op->len != 4) {
    rebind_op = NULL;
  }
  //
  // There should be a lease time.
  // If there is not, we will default to one week.
  //
  EfiStatus = find_opt (rx_pkt, DHCP4_LEASE_TIME, 0, &lease_time_op);

  if (EFI_ERROR (EfiStatus)) {
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
EFI_STATUS
EFIAPI
PxeDhcp4Init (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout,
  OUT UINTN                 *Offers,
  OUT DHCP4_PACKET          **OfferList
  )
{
  PXE_DHCP4_PRIVATE_DATA  *Private;
  DHCP4_PACKET            offer;
  EFI_IP_ADDRESS          bcast_ip;
  EFI_STATUS              EfiStatus;

  //
  // Verify parameters and protocol state.
  //
  if (This == NULL ||
      seconds_timeout < DHCP4_MIN_SECONDS ||
      seconds_timeout > DHCP4_MAX_SECONDS ||
      Offers == NULL ||
      OfferList == NULL
      ) {
    //
    // Return parameters are not initialized when
    // parameters are invalid!
    //
    return EFI_INVALID_PARAMETER;
  }

  *Offers     = 0;
  *OfferList  = NULL;

  //
  // Check protocol state.
  //
  if (This->Data == NULL) {
    return EFI_NOT_STARTED;
  }

  if (!This->Data->SetupCompleted) {
    return EFI_NOT_READY;
  }

  //
  // Get pointer to our instance data.
  //
  Private = PXE_DHCP4_PRIVATE_DATA_FROM_THIS (This);

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Private->PxeBc == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Setup variables...
  //
  Private->offers     = 0;
  Private->offer_list = NULL;

  EfiStatus = gBS->HandleProtocol (
                    Private->Handle,
                    &gEfiPxeDhcp4CallbackProtocolGuid,
                    (VOID *) &Private->callback
                    );

  if (EFI_ERROR (EfiStatus)) {
    Private->callback = NULL;
  }

  Private->function = EFI_PXE_DHCP4_FUNCTION_INIT;

  //
  // Increment the transaction ID.
  //
  {
    UINT32  xid;

    CopyMem (&xid, &This->Data->Discover.dhcp4.xid, sizeof (UINT32));

    xid = htonl (htonl (xid) + 1);

    CopyMem (&This->Data->Discover.dhcp4.xid, &xid, sizeof (UINT32));
  }
  //
  // Transmit discover and wait for offers...
  //
  SetMem (&bcast_ip, sizeof (EFI_IP_ADDRESS), 0xFF);

  EfiStatus = tx_rx_udp (
                Private,
                &bcast_ip,
                NULL,
                NULL,
                NULL,
                &This->Data->Discover,
                &offer,
                &offer_verify,
                seconds_timeout
                );

  if (EFI_ERROR (EfiStatus)) {
    if (Private->offer_list) {
      gBS->FreePool (Private->offer_list);
    }

    Private->offers     = 0;
    Private->offer_list = NULL;
    Private->callback   = NULL;

    DebugPrint (("%a:%d:%r\n", __FILE__, __LINE__, EfiStatus));
    return EfiStatus;
  }

  *Offers                     = Private->offers;
  *OfferList                  = Private->offer_list;

  Private->offers             = 0;
  Private->offer_list         = NULL;
  Private->callback           = NULL;

  This->Data->InitCompleted   = TRUE;
  This->Data->SelectCompleted = FALSE;
  This->Data->IsBootp         = FALSE;
  This->Data->IsAck           = FALSE;

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4Select (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout,
  IN DHCP4_PACKET           *Offer
  )
{
  PXE_DHCP4_PRIVATE_DATA  *Private;
  EFI_STATUS              EfiStatus;
  DHCP4_PACKET            request;
  DHCP4_PACKET            acknak;
  EFI_IP_ADDRESS          bcast_ip;
  EFI_IP_ADDRESS          zero_ip;
  EFI_IP_ADDRESS          local_ip;
  DHCP4_OP                *srvid;
  DHCP4_OP                *op;
  UINT32                  dhcp4_magik;
  UINT8                   buf[16];
  BOOLEAN                 is_bootp;

  //
  // Verify parameters.
  //
  if (This == NULL || seconds_timeout < DHCP4_MIN_SECONDS || seconds_timeout > DHCP4_MAX_SECONDS || Offer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Check protocol state.
  //
  if (This->Data == NULL) {
    return EFI_NOT_STARTED;
  }

  if (!This->Data->SetupCompleted) {
    return EFI_NOT_READY;
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
  // Setup useful variables...
  //
  SetMem (&bcast_ip, sizeof (EFI_IP_ADDRESS), 0xFF);

  ZeroMem (&zero_ip, sizeof (EFI_IP_ADDRESS));

  ZeroMem (&local_ip, sizeof (EFI_IP_ADDRESS));
  local_ip.v4.Addr[0]         = 127;
  local_ip.v4.Addr[3]         = 1;

  This->Data->SelectCompleted = FALSE;
  This->Data->IsBootp         = FALSE;
  This->Data->IsAck           = FALSE;

  EfiStatus = gBS->HandleProtocol (
                    Private->Handle,
                    &gEfiPxeDhcp4CallbackProtocolGuid,
                    (VOID *) &Private->callback
                    );

  if (EFI_ERROR (EfiStatus)) {
    Private->callback = NULL;
  }

  Private->function = EFI_PXE_DHCP4_FUNCTION_SELECT;

  //
  // Verify offer packet fields.
  //
  if (Offer->dhcp4.op != BOOTP_REPLY) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (Offer->dhcp4.htype != This->Data->Discover.dhcp4.htype) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (Offer->dhcp4.hlen != This->Data->Discover.dhcp4.hlen) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (CompareMem (&Offer->dhcp4.xid, &This->Data->Discover.dhcp4.xid, 4)) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (!CompareMem (&Offer->dhcp4.yiaddr, &bcast_ip, 4)) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (!CompareMem (&Offer->dhcp4.yiaddr, &zero_ip, 4)) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (!CompareMem (&Offer->dhcp4.yiaddr, &local_ip, 4)) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (CompareMem (
        &Offer->dhcp4.chaddr,
        &This->Data->Discover.dhcp4.chaddr,
        16
        )) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }
  //
  // DHCP option checks
  //
  dhcp4_magik = htonl (DHCP4_MAGIK_NUMBER);
  is_bootp    = TRUE;

  if (!CompareMem (&Offer->dhcp4.magik, &dhcp4_magik, 4)) {
    //
    // If present, DHCP message type must be offer.
    //
    EfiStatus = find_opt (Offer, DHCP4_MESSAGE_TYPE, 0, &op);

    if (!EFI_ERROR (EfiStatus)) {
      if (op->len != 1 || op->data[0] != DHCP4_MESSAGE_TYPE_OFFER) {
        Private->callback = NULL;
        return EFI_INVALID_PARAMETER;
      }

      is_bootp = FALSE;
    }
    //
    // If present, DHCP max message size must be valid.
    //
    EfiStatus = find_opt (Offer, DHCP4_MAX_MESSAGE_SIZE, 0, &op);

    if (!EFI_ERROR (EfiStatus)) {
      if (op->len != 2 || ((op->data[0] << 8) | op->data[1]) < DHCP4_DEFAULT_MAX_MESSAGE_SIZE) {
        Private->callback = NULL;
        return EFI_INVALID_PARAMETER;
      }
    }
    //
    // If present, DHCP server identifier must be valid.
    //
    EfiStatus = find_opt (Offer, DHCP4_SERVER_IDENTIFIER, 0, &op);

    if (!EFI_ERROR (EfiStatus)) {
      if (op->len != 4 || !CompareMem (op->data, &bcast_ip, 4) || !CompareMem (op->data, &zero_ip, 4)) {
        Private->callback = NULL;
        return EFI_INVALID_PARAMETER;
      }
    }
    //
    // If present, DHCP subnet mask must be valid.
    //
    EfiStatus = find_opt (
                  Offer,
                  DHCP4_SUBNET_MASK,
                  0,
                  &op
                  );

    if (!EFI_ERROR (EfiStatus)) {
      if (op->len != 4) {
        Private->callback = NULL;
        return EFI_INVALID_PARAMETER;
      }
    }
  }
  //
  // Early out for BOOTP.
  //
  This->Data->IsBootp = is_bootp;
  if (is_bootp) {
    //
    // Copy offer packet to instance data.
    //
    CopyMem (&This->Data->Offer, Offer, sizeof (DHCP4_PACKET));

    //
    // Copy discover to request and offer to acknak.
    //
    CopyMem (
      &This->Data->Request,
      &This->Data->Discover,
      sizeof (DHCP4_PACKET)
      );

    CopyMem (
      &This->Data->AckNak,
      &This->Data->Offer,
      sizeof (DHCP4_PACKET)
      );

    //
    // Set state flags.
    //
    This->Data->SelectCompleted = TRUE;
    This->Data->IsAck           = TRUE;

    Private->callback           = NULL;
    return EFI_SUCCESS;
  }
  //
  // Copy discover packet contents to request packet.
  //
  CopyMem (&request, &This->Data->Discover, sizeof (DHCP4_PACKET));

  This->Data->IsAck = FALSE;

  //
  // Change DHCP message type from discover to request.
  //
  EfiStatus = find_opt (&request, DHCP4_MESSAGE_TYPE, 0, &op);

  if (EFI_ERROR (EfiStatus) && EfiStatus != EFI_NOT_FOUND) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (EfiStatus == EFI_NOT_FOUND) {
    EfiStatus = find_opt (&request, DHCP4_END, 0, &op);

    if (EFI_ERROR (EfiStatus)) {
      Private->callback = NULL;
      return EFI_INVALID_PARAMETER;
    }

    op->op      = DHCP4_MESSAGE_TYPE;
    op->len     = 1;

    op->data[1] = DHCP4_END;
  }

  op->data[0] = DHCP4_MESSAGE_TYPE_REQUEST;

  //
  // Copy server identifier option from offer to request.
  //
  EfiStatus = find_opt (Offer, DHCP4_SERVER_IDENTIFIER, 0, &srvid);

  if (EFI_ERROR (EfiStatus)) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  if (srvid->len != 4) {
    Private->callback = NULL;
    return EFI_INVALID_PARAMETER;
  }

  EfiStatus = add_opt (&request, srvid);

  if (EFI_ERROR (EfiStatus)) {
    DebugPrint (("%a:%d:%r\n", __FILE__, __LINE__, EfiStatus));
    Private->callback = NULL;
    return EfiStatus;
  }
  //
  // Add requested IP address option to request packet.
  //
  op      = (DHCP4_OP *) buf;
  op->op  = DHCP4_REQUESTED_IP_ADDRESS;
  op->len = 4;
  CopyMem (op->data, &Offer->dhcp4.yiaddr, 4);

  EfiStatus = add_opt (&request, op);

  if (EFI_ERROR (EfiStatus)) {
    DebugPrint (("%a:%d:%r\n", __FILE__, __LINE__, EfiStatus));
    Private->callback = NULL;
    return EfiStatus;
  }
  //
  // Transimit DHCP request and wait for DHCP ack...
  //
  SetMem (&bcast_ip, sizeof (EFI_IP_ADDRESS), 0xFF);

  EfiStatus = tx_rx_udp (
                Private,
                &bcast_ip,
                NULL,
                NULL,
                NULL,
                &request,
                &acknak,
                &acknak_verify,
                seconds_timeout
                );

  if (EFI_ERROR (EfiStatus)) {
    DebugPrint (("%a:%d:%r\n", __FILE__, __LINE__, EfiStatus));
    Private->callback = NULL;
    return EfiStatus;
  }
  //
  // Set Data->IsAck and return.
  //
  EfiStatus = find_opt (&acknak, DHCP4_MESSAGE_TYPE, 0, &op);

  if (EFI_ERROR (EfiStatus)) {
    Private->callback = NULL;
    return EFI_DEVICE_ERROR;
  }

  if (op->len != 1) {
    Private->callback = NULL;
    return EFI_DEVICE_ERROR;
  }

  switch (op->data[0]) {
  case DHCP4_MESSAGE_TYPE_ACK:
    This->Data->IsAck = TRUE;
    break;

  case DHCP4_MESSAGE_TYPE_NAK:
    This->Data->IsAck = FALSE;
    break;

  default:
    Private->callback = NULL;
    return EFI_DEVICE_ERROR;
  }
  //
  // Copy packets into instance data...
  //
  CopyMem (&This->Data->Offer, Offer, sizeof (DHCP4_PACKET));
  CopyMem (&This->Data->Request, &request, sizeof (DHCP4_PACKET));
  CopyMem (&This->Data->AckNak, &acknak, sizeof (DHCP4_PACKET));

  This->Data->SelectCompleted = TRUE;

  Private->callback           = NULL;
  return EFI_SUCCESS;
}

/* eof - PxeDhcp4InitSelect.c */
