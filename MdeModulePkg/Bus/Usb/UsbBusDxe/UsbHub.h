/** @file

    The definition for USB hub.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _USB_HUB_H_
#define _USB_HUB_H_

#include <IndustryStandard/Usb.h>

#define USB_ENDPOINT_ADDR(EpAddr) ((EpAddr) & 0x7F)
#define USB_ENDPOINT_TYPE(Desc)   ((Desc)->Attributes & USB_ENDPOINT_TYPE_MASK)


#define USB_DESC_TYPE_HUB     0x29

#define USB_DESC_TYPE_HUB_SUPER_SPEED  0x2a

//
// Hub class control transfer target
//
#define USB_HUB_TARGET_HUB    0
#define USB_HUB_TARGET_PORT   3
//
// HUB class specific contrl transfer request type
//
#define USB_HUB_REQ_GET_STATUS      0
#define USB_HUB_REQ_CLEAR_FEATURE   1
#define USB_HUB_REQ_SET_FEATURE     3
#define USB_HUB_REQ_GET_DESC        6
#define USB_HUB_REQ_SET_DESC        7
#define USB_HUB_REQ_CLEAR_TT        8
#define USB_HUB_REQ_RESET_TT        9
#define USB_HUB_REQ_GET_TT_STATE    10
#define USB_HUB_REQ_STOP_TT         11

#define USB_HUB_REQ_SET_DEPTH       12

//
// USB hub class feature selector
//
#define USB_HUB_C_HUB_LOCAL_POWER   0
#define USB_HUB_C_HUB_OVER_CURRENT  1
#define USB_HUB_PORT_CONNECTION     0
#define USB_HUB_PORT_ENABLE         1
#define USB_HUB_PORT_SUSPEND        2
#define USB_HUB_PORT_OVER_CURRENT   3
#define USB_HUB_PORT_RESET          4

#define USB_HUB_PORT_LINK_STATE     5

#define USB_HUB_PORT_POWER          8
#define USB_HUB_PORT_LOW_SPEED      9
#define USB_HUB_C_PORT_CONNECT      16
#define USB_HUB_C_PORT_ENABLE       17
#define USB_HUB_C_PORT_SUSPEND      18
#define USB_HUB_C_PORT_OVER_CURRENT 19
#define USB_HUB_C_PORT_RESET        20
#define USB_HUB_PORT_TEST           21
#define USB_HUB_PORT_INDICATOR      22

#define USB_HUB_C_PORT_LINK_STATE     25
#define USB_HUB_PORT_REMOTE_WAKE_MASK 27
#define USB_HUB_BH_PORT_RESET         28
#define USB_HUB_C_BH_PORT_RESET       29

//
// Constant value for Port Status & Port Change Status of SuperSpeed port
//
#define USB_SS_PORT_STAT_C_BH_RESET         0x0020
#define USB_SS_PORT_STAT_C_PORT_LINK_STATE  0x0040
//
// USB hub power control method. In gang power control
//
#define USB_HUB_GANG_POWER_CTRL     0
#define USB_HUB_PORT_POWER_CTRL     0x01
//
// USB hub status bits
//
#define USB_HUB_STAT_LOCAL_POWER    0x01
#define USB_HUB_STAT_OVER_CURRENT   0x02
#define USB_HUB_STAT_C_LOCAL_POWER  0x01
#define USB_HUB_STAT_C_OVER_CURRENT 0x02

#define USB_HUB_CLASS_CODE          0x09
#define USB_HUB_SUBCLASS_CODE       0x00

//
// Host software return timeout if port status doesn't change
// after 500ms(LOOP * STALL = 5000 * 0.1ms), set by experience
//
#define USB_WAIT_PORT_STS_CHANGE_LOOP  5000

#pragma pack(1)
//
// Hub descriptor, the last two fields are of variable length.
//
typedef struct {
  UINT8           Length;
  UINT8           DescType;
  UINT8           NumPorts;
  UINT16          HubCharacter;
  UINT8           PwrOn2PwrGood;
  UINT8           HubContrCurrent;
  UINT8           Filler[16];
} EFI_USB_HUB_DESCRIPTOR;

#pragma pack()


typedef struct {
  UINT16                ChangedBit;
  EFI_USB_PORT_FEATURE  Feature;
} USB_CHANGE_FEATURE_MAP;


/**
  Clear the transaction translate buffer if full/low
  speed control/bulk transfer failed and the transfer
  uses this hub as translator.Remember to clear the TT
  buffer of transaction translator, not that of the
  parent.

  @param  UsbDev                The Usb device.
  @param  Port                  The port of the hub.
  @param  DevAddr               Address of the failed transaction.
  @param  EpNum                 The endpoint number of the failed transaction.
  @param  EpType                The type of failed transaction.

  @retval EFI_SUCCESS           The TT buffer is cleared.
  @retval Others                Failed to clear the TT buffer.

**/
EFI_STATUS
UsbHubCtrlClearTTBuffer (
  IN USB_DEVICE           *UsbDev,
  IN UINT8                Port,
  IN UINT16               DevAddr,
  IN UINT16               EpNum,
  IN UINT16               EpType
  );


/**
  Test whether the interface is a hub interface.

  @param  UsbIf                 The interface to test.

  @retval TRUE                  The interface is a hub interface.
  @retval FALSE                 The interface isn't a hub interface.

**/
BOOLEAN
UsbIsHubInterface (
  IN USB_INTERFACE        *UsbIf
  );


/**
  Ack the hub change bits. If these bits are not ACKed, Hub will
  always return changed bit map from its interrupt endpoint.

  @param  UsbDev                The Usb device.

  @retval EFI_SUCCESS           The hub change status is ACKed.
  @retval Others                Failed to ACK the hub status.

**/
EFI_STATUS
UsbHubAckHubStatus (
  IN  USB_DEVICE         *UsbDev
  );

extern USB_HUB_API        mUsbHubApi;
extern USB_HUB_API        mUsbRootHubApi;
#endif

