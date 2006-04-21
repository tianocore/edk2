/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    Hub.h
    
  Abstract:

    Constants definitions for Usb Hub

  Revision History

--*/

#ifndef _HUB_H
#define _HUB_H

//
// Hub feature numbers
//
#define C_HUB_LOCAL_POWER   0
#define C_HUB_OVER_CURRENT  1

//
// Hub class code & sub class code
//
#define CLASS_CODE_HUB      0x09
#define SUB_CLASS_CODE_HUB  0

//
// Hub Status & Hub Change bit masks
//
#define HUB_STATUS_LOCAL_POWER  0x0001
#define HUB_STATUS_OVERCURRENT  0x0002

#define HUB_CHANGE_LOCAL_POWER  0x0001
#define HUB_CHANGE_OVERCURRENT  0x0002

//
// Hub Characteristics
//
#define HUB_CHAR_LPSM     0x0003
#define HUB_CHAR_COMPOUND 0x0004
#define HUB_CHAR_OCPM     0x0018

//
// Hub specific request
//
#define HUB_CLEAR_FEATURE               0x01
#define HUB_CLEAR_FEATURE_REQ_TYPE      0x20

#define HUB_CLEAR_FEATURE_PORT          0x01
#define HUB_CLEAR_FEATURE_PORT_REQ_TYPE 0x23

#define HUB_GET_BUS_STATE               0x02
#define HUB_GET_BUS_STATE_REQ_TYPE      0xA3

#define HUB_GET_DESCRIPTOR              0x06
#define HUB_GET_DESCRIPTOR_REQ_TYPE     0xA0

#define HUB_GET_HUB_STATUS              0x00
#define HUB_GET_HUB_STATUS_REQ_TYPE     0xA0

#define HUB_GET_PORT_STATUS             0x00
#define HUB_GET_PORT_STATUS_REQ_TYPE    0xA3

#define HUB_SET_DESCRIPTOR              0x07
#define HUB_SET_DESCRIPTOR_REQ_TYPE     0x20

#define HUB_SET_HUB_FEATURE             0x03
#define HUB_SET_HUB_FEATURE_REQ_TYPE    0x20

#define HUB_SET_PORT_FEATURE            0x03
#define HUB_SET_PORT_FEATURE_REQ_TYPE   0x23

#pragma pack(1)
typedef struct usb_hub_status {
  UINT16  HubStatus;
  UINT16  HubChange;
} EFI_USB_HUB_STATUS;
#pragma pack()

EFI_STATUS
HubGetPortStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  IN  UINT8                   Port,
  OUT UINT32                  *PortStatus
  );

EFI_STATUS
HubSetPortFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Port,
  IN UINT8                   Value
  );

EFI_STATUS
HubSetHubFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Value
  );

EFI_STATUS
HubGetHubStatus (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo,
  OUT UINT32                  *HubStatus
  );

EFI_STATUS
HubClearPortFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Port,
  IN UINT8                   Value
  );

EFI_STATUS
HubClearHubFeature (
  IN EFI_USB_IO_PROTOCOL     *UsbIo,
  IN UINT8                   Value
  );

EFI_STATUS
GetHubDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *UsbIo,
  IN  UINTN                      DescriptorSize,
  OUT EFI_USB_HUB_DESCRIPTOR     *HubDescriptor
  );

#endif
