/** @file

Copyright (c) 2004, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeDhcp4Setup.c

Abstract:


**/

#include "PxeDhcp4.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4Setup (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN EFI_PXE_DHCP4_DATA     *Data
  )
{
  PXE_DHCP4_PRIVATE_DATA  *Private;
  DHCP4_HEADER            *Packet;
  EFI_STATUS              EfiStatus;
  UINT8                   *OpLen;
  UINT8                   *OpPtr;

  //
  // Return error if parameters are invalid.
  //
  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = PXE_DHCP4_PRIVATE_DATA_FROM_THIS (This);

  if (Private == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (This->Data != NULL) {
    return EFI_ALREADY_STARTED;
  }

  if (Private->PxeBc == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Check contents of provided Data structure.
  //
  if (Data != NULL) {
    //
    // Do protocol state checks first.
    //
    if (Data->SelectCompleted) {
      if (!Data->InitCompleted || !Data->SetupCompleted) {
        return EFI_INVALID_PARAMETER;
      }

      if (Data->IsBootp && !Data->IsAck) {
        return EFI_INVALID_PARAMETER;
      }
    } else if (Data->InitCompleted) {
      if (!Data->SetupCompleted || Data->IsBootp || Data->IsAck) {
        return EFI_INVALID_PARAMETER;
      }
    } else if (Data->SetupCompleted) {
      if (Data->IsBootp || Data->IsAck) {
        return EFI_INVALID_PARAMETER;
      }
    }
    //
    // Do packet content checks.
    //
    if (Data->SetupCompleted) {
      //
      // %%TBD - check discover packet
      //
    }

    if (Data->SelectCompleted) {
      if (Data->IsBootp) {
        //
        // %%TBD - check offer packet
        //
        if (CompareMem (
              &Data->Discover,
              &Data->Request,
              sizeof (DHCP4_PACKET)
              )) {
          return EFI_INVALID_PARAMETER;
        }

        if (CompareMem (
              &Data->Offer,
              &Data->AckNak,
              sizeof (DHCP4_PACKET)
              )) {
          return EFI_INVALID_PARAMETER;
        }
      } else {
        //
        // %%TBD - check offer, request & acknak packets
        //
      }
    }
  }
  //
  // Allocate data structure.  Return error
  // if there is not enough available memory.
  //
  This->Data = AllocatePool (sizeof (EFI_PXE_DHCP4_DATA));
  if (This->Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Start PxeBc because we want to use its UdpWrite, UdpRead and
  // SetFilter calls.
  //
  EfiStatus = Private->PxeBc->Start (Private->PxeBc, FALSE);

  if (EFI_ERROR (EfiStatus)) {
    if (EfiStatus != EFI_ALREADY_STARTED) {
      FreePool (This->Data);
      This->Data = NULL;
      Private->PxeBc->Stop (Private->PxeBc);
      return EfiStatus;
    }

    Private->StopPxeBc = FALSE;
  } else {
    Private->StopPxeBc = TRUE;
  }
  //
  // Use new data.
  //
  if (Data != NULL) {
    CopyMem (This->Data, Data, sizeof (EFI_PXE_DHCP4_DATA));
    return EFI_SUCCESS;
  }
  //
  // Initialize new public data structure.
  //
  ZeroMem (This->Data, sizeof (EFI_PXE_DHCP4_DATA));

  //
  // Fill in default DHCP discover packet.
  // Check for MAC addresses of strange lengths, just in case.
  //
  Packet        = &This->Data->Discover.dhcp4;

  Packet->op    = BOOTP_REQUEST;

  Packet->htype = Private->Snp->Mode->IfType;

  if (Private->Snp->Mode->HwAddressSize > 16) {
    Packet->hlen = 16;
  } else {
    Packet->hlen = (UINT8) Private->Snp->Mode->HwAddressSize;
  }

  Packet->hops = 0; /* Set to zero per RFC 2131. */

  if (Packet->hlen < sizeof Packet->xid) {
    if (Packet->hlen != 0) {
      CopyMem (
        &Packet->xid,
        &Private->Snp->Mode->CurrentAddress,
        Packet->hlen
        );
    }
  } else {
    CopyMem (
      &Packet->xid,
      &Private->Snp->Mode->CurrentAddress.Addr[Packet->hlen - sizeof Packet->xid],
      sizeof Packet->xid
      );
  }
  //
  // %%TBD - xid should be randomized
  //
  Packet->secs  = htons (DHCP4_INITIAL_SECONDS);

  Packet->flags = htons (DHCP4_BROADCAST_FLAG);

  if (Packet->hlen != 0) {
    CopyMem (Packet->chaddr, &Private->Snp->Mode->CurrentAddress, Packet->hlen);
  }

  Packet->magik               = htonl (DHCP4_MAGIK_NUMBER);

  OpPtr                       = Packet->options;

  *OpPtr++                    = DHCP4_MESSAGE_TYPE;
  *OpPtr++                    = 1;
  *OpPtr++                    = DHCP4_MESSAGE_TYPE_DISCOVER;

  *OpPtr++                    = DHCP4_MAX_MESSAGE_SIZE;
  *OpPtr++                    = 2;
  *OpPtr++                    = (UINT8) ((DHCP4_DEFAULT_MAX_MESSAGE_SIZE >> 8) & 0xFF);
  *OpPtr++                    = (UINT8) (DHCP4_DEFAULT_MAX_MESSAGE_SIZE & 0xFF);

  *OpPtr++                    = DHCP4_PARAMETER_REQUEST_LIST;
  OpLen                       = OpPtr;
  *OpPtr++                    = 0;
  *OpPtr++                    = DHCP4_SUBNET_MASK;
  *OpPtr++                    = DHCP4_TIME_OFFSET;
  *OpPtr++                    = DHCP4_ROUTER_LIST;
  *OpPtr++                    = DHCP4_TIME_SERVERS;
  *OpPtr++                    = DHCP4_NAME_SERVERS;
  *OpPtr++                    = DHCP4_DNS_SERVERS;
  *OpPtr++                    = DHCP4_HOST_NAME;
  *OpPtr++                    = DHCP4_BOOT_FILE_SIZE;
  *OpPtr++                    = DHCP4_MESSAGE_TYPE;
  *OpPtr++                    = DHCP4_DOMAIN_NAME;
  *OpPtr++                    = DHCP4_ROOT_PATH;
  *OpPtr++                    = DHCP4_EXTENSION_PATH;
  *OpPtr++                    = DHCP4_MAX_DATAGRAM_SIZE;
  *OpPtr++                    = DHCP4_DEFAULT_TTL;
  *OpPtr++                    = DHCP4_BROADCAST_ADDRESS;
  *OpPtr++                    = DHCP4_NIS_DOMAIN_NAME;
  *OpPtr++                    = DHCP4_NIS_SERVERS;
  *OpPtr++                    = DHCP4_NTP_SERVERS;
  *OpPtr++                    = DHCP4_VENDOR_SPECIFIC;
  *OpPtr++                    = DHCP4_REQUESTED_IP_ADDRESS;
  *OpPtr++                    = DHCP4_LEASE_TIME;
  *OpPtr++                    = DHCP4_SERVER_IDENTIFIER;
  *OpPtr++                    = DHCP4_RENEWAL_TIME;
  *OpPtr++                    = DHCP4_REBINDING_TIME;
  *OpPtr++                    = DHCP4_CLASS_IDENTIFIER;
  *OpPtr++                    = DHCP4_TFTP_SERVER_NAME;
  *OpPtr++                    = DHCP4_BOOTFILE;
  *OpPtr++                    = 128;
  *OpPtr++                    = 129;
  *OpPtr++                    = 130;
  *OpPtr++                    = 131;
  *OpPtr++                    = 132;
  *OpPtr++                    = 133;
  *OpPtr++                    = 134;
  *OpPtr++                    = 135;
  *OpLen                      = (UINT8) ((OpPtr - OpLen) - 1);

  *OpPtr++                    = DHCP4_END;

  This->Data->SetupCompleted  = TRUE;

  return EFI_SUCCESS;
}

/* eof - PxeDhcp4Setup.c */
