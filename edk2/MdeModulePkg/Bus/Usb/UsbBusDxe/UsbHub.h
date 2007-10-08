/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbHub.h

  Abstract:

    The definition for USB hub

  Revision History


**/

#ifndef _USB_HUB_H_
#define _USB_HUB_H_

#include <IndustryStandard/Usb.h>

#define USB_ENDPOINT_ADDR(EpAddr) ((EpAddr) & 0x7F)
#define USB_ENDPOINT_TYPE(Desc)   ((Desc)->Attributes & USB_ENDPOINT_TYPE_MASK)

enum {
  USB_DESC_TYPE_HUB           = 0x29,

  //
  // Hub class control transfer target
  //
  USB_HUB_TARGET_HUB          = 0,
  USB_HUB_TARGET_PORT         = 3,

  //
  // HUB class specific contrl transfer request type
  //
  USB_HUB_REQ_GET_STATUS      = 0,
  USB_HUB_REQ_CLEAR_FEATURE   = 1,
  USB_HUB_REQ_SET_FEATURE     = 3,
  USB_HUB_REQ_GET_DESC        = 6,
  USB_HUB_REQ_SET_DESC        = 7,
  USB_HUB_REQ_CLEAR_TT        = 8,
  USB_HUB_REQ_RESET_TT        = 9,
  USB_HUB_REQ_GET_TT_STATE    = 10,
  USB_HUB_REQ_STOP_TT         = 11,


  //
  // USB hub class feature selector
  //
  USB_HUB_C_HUB_LOCAL_POWER   = 0,
  USB_HUB_C_HUB_OVER_CURRENT  = 1,
  USB_HUB_PORT_CONNECTION     = 0,
  USB_HUB_PORT_ENABLE         = 1,
  USB_HUB_PORT_SUSPEND        = 2,
  USB_HUB_PORT_OVER_CURRENT   = 3,
  USB_HUB_PORT_RESET          = 4,
  USB_HUB_PORT_POWER          = 8,
  USB_HUB_PORT_LOW_SPEED      = 9,
  USB_HUB_C_PORT_CONNECT      = 16,
  USB_HUB_C_PORT_ENABLE       = 17,
  USB_HUB_C_PORT_SUSPEND      = 18,
  USB_HUB_C_PORT_OVER_CURRENT = 19,
  USB_HUB_C_PORT_RESET        = 20,
  USB_HUB_PORT_TEST           = 21,
  USB_HUB_PORT_INDICATOR      = 22,

  //
  // USB hub power control method. In gang power control
  //
  USB_HUB_GANG_POWER_CTRL     = 0,
  USB_HUB_PORT_POWER_CTRL     = 0x01,

  //
  // USB hub status bits
  //
  USB_HUB_STAT_LOCAL_POWER    = 0x01,
  USB_HUB_STAT_OVER_CURRENT   = 0x02,
  USB_HUB_STAT_C_LOCAL_POWER  = 0x01,
  USB_HUB_STAT_C_OVER_CURRENT = 0x02,

  USB_HUB_CLASS_CODE          = 0x09,
  USB_HUB_SUBCLASS_CODE       = 0x00,

  //
  // Host software return timeout if port status doesn't change 
  // after 500ms(LOOP * STALL = 100 * 5ms), set by experience
  //
  USB_WAIT_PORT_STS_CHANGE_LOOP   = 100
};

#pragma pack(1)
//
// Hub descriptor, the last two fields are of variable lenght.
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


EFI_STATUS
UsbHubCtrlClearTTBuffer (
  IN USB_DEVICE           *UsbDev,
  IN UINT8                Port,
  IN UINT16               DevAddr,
  IN UINT16               EpNum,
  IN UINT16               EpType
  );


BOOLEAN
UsbIsHubInterface (
  IN USB_INTERFACE        *UsbIf
  );

EFI_STATUS
UsbHubAckHubStatus (
  IN  USB_DEVICE         *UsbDev
  );

extern USB_HUB_API        mUsbHubApi;
extern USB_HUB_API        mUsbRootHubApi;
#endif

