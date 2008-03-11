/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  PxeDhcp4.h

Abstract:
  Common header for PxeDhcp4 protocol driver


**/
#ifndef _PXEDHCP4_H
#define _PXEDHCP4_H


#include <PiDxe.h>

#include <Protocol/PxeBaseCode.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/PxeDhcp4.h>
#include <Protocol/PxeDhcp4CallBack.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// PxeDhcp4 protocol instance data
//
typedef struct {
  //
  // Signature field used to locate beginning of containment record.
  //
  UINTN Signature;

#define PXE_DHCP4_PRIVATE_DATA_SIGNATURE  EFI_SIGNATURE_32 ('p', 'x', 'D', '4')
  //
  // Device handle the protocol is bound to.
  //
  EFI_HANDLE                      Handle;

  //
  // Public PxeDhcp4 protocol interface.
  //
  EFI_PXE_DHCP4_PROTOCOL          PxeDhcp4;

  //
  // Consumed PxeBc, Snp and PxeDhcp4Callback protocol interfaces.
  //
  EFI_PXE_BASE_CODE_PROTOCOL      *PxeBc;
  EFI_SIMPLE_NETWORK_PROTOCOL     *Snp;
  EFI_PXE_DHCP4_CALLBACK_PROTOCOL *callback;

  //
  // PxeDhcp4 called function for PxeDhcp4Callback.
  //
  EFI_PXE_DHCP4_FUNCTION          function;

  //
  // Timeout event and flag for PxeDhcp4Callback.
  //
  EFI_EVENT                       TimeoutEvent;
  BOOLEAN                         TimeoutOccurred;

  //
  // Periodic event and flag for PxeDhcp4Callback.
  //
  EFI_EVENT                       PeriodicEvent;
  BOOLEAN                         PeriodicOccurred;

  //
  // DHCP server IP address.
  //
  UINT32                          ServerIp;

  //
  // DHCP renewal and rebinding times, in seconds.
  //
  UINT32                          RenewTime;
  UINT32                          RebindTime;
  UINT32                          LeaseTime;

  //
  // Number of offers received & allocated offer list.
  //
  UINTN                           offers;
  DHCP4_PACKET                    *offer_list;

  //
  //
  //
  BOOLEAN                         StopPxeBc;

} PXE_DHCP4_PRIVATE_DATA;

#define PXE_DHCP4_PRIVATE_DATA_FROM_THIS(a) CR (a, PXE_DHCP4_PRIVATE_DATA, PxeDhcp4, PXE_DHCP4_PRIVATE_DATA_SIGNATURE)

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// Protocol function prototypes.
//
extern
EFI_STATUS 
EFIAPI
PxeDhcp4Run (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN OPTIONAL UINTN                  OpLen,
  IN OPTIONAL VOID                   *OpList
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Setup (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN EFI_PXE_DHCP4_DATA     *Data
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Init (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout,
  OUT UINTN                 *offer_list_entries,
  OUT DHCP4_PACKET          **offer_list
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Select (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN UINTN                  seconds_timeout,
  IN DHCP4_PACKET           *offer_list
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Renew (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  UINTN                     seconds_timeout
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Rebind (
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  UINTN                     seconds_timeout
  )
;

extern
EFI_STATUS 
EFIAPI
PxeDhcp4Release (
  IN EFI_PXE_DHCP4_PROTOCOL *This
  )
;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// Support function prototypes.
//
extern
UINT16 
htons (
  UINTN n
  )
;

extern
UINT32 
htonl (
  UINTN n
  )
;

extern
VOID 
EFIAPI
timeout_notify (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
;

extern
VOID 
EFIAPI
periodic_notify (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
;

extern
EFI_STATUS 
find_opt (
  IN DHCP4_PACKET *Packet,
  IN UINT8        OpCode,
  IN UINTN        Skip,
  OUT DHCP4_OP    **OpPtr
  )
;

extern
EFI_STATUS 
add_opt (
  IN DHCP4_PACKET *Packet,
  IN DHCP4_OP     *OpPtr
  )
;

extern
EFI_STATUS 
start_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN OPTIONAL EFI_IP_ADDRESS         *station_ip,
  IN OPTIONAL EFI_IP_ADDRESS         *subnet_mask
  )
;

extern
VOID 
stop_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private
  )
;

extern
EFI_STATUS 
start_receive_events (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN UINTN                  seconds_timeout
  )
;

extern
VOID 
stop_receive_events (
  IN PXE_DHCP4_PRIVATE_DATA *Private
  )
;

extern
EFI_STATUS 
tx_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  IN EFI_IP_ADDRESS         *dest_ip,
  IN OPTIONAL EFI_IP_ADDRESS         *gateway_ip,
  IN EFI_IP_ADDRESS         *src_ip,
  IN VOID                   *buffer,
  IN UINTN                  BufferSize
  )
;

extern
EFI_STATUS 
rx_udp (
  IN PXE_DHCP4_PRIVATE_DATA *Private,
  OUT VOID                  *buffer,
  OUT UINTN                 *BufferSize,
  IN OUT EFI_IP_ADDRESS     *dest_ip,
  IN OUT EFI_IP_ADDRESS     *src_ip,
  IN UINT16                 op_flags
  )
;

extern
EFI_STATUS 
tx_rx_udp (
  IN PXE_DHCP4_PRIVATE_DATA    *Private,
  IN OUT EFI_IP_ADDRESS        *ServerIp,
  IN OPTIONAL EFI_IP_ADDRESS   *gateway_ip,
  IN OPTIONAL EFI_IP_ADDRESS   *client_ip,
  IN OPTIONAL EFI_IP_ADDRESS   *subnet_mask,
  IN DHCP4_PACKET              *tx_pkt,
  OUT DHCP4_PACKET             *rx_pkt,
  IN INTN
    (
  *rx_vfy)
    (
      IN PXE_DHCP4_PRIVATE_DATA *Private,
      IN DHCP4_PACKET *tx_pkt,
      IN DHCP4_PACKET *rx_pkt,
      IN UINTN rx_pkt_size
    ),
  IN UINTN seconds_timeout
  )
;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// Global variable definitions.
//
extern EFI_COMPONENT_NAME_PROTOCOL    gPxeDhcp4ComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gPxeDhcp4ComponentName2;

EFI_STATUS
EFIAPI
PxeDhcp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:
  Register Driver Binding protocol for this driver.

Arguments:
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:
  EFI_SUCCESS - Driver loaded.
  other       - Driver not loaded.

--*/
;

#endif /* _PXEDHCP4_H */

/* EOF - PxeDhcp4.h */
