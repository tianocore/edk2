/** @file

  The definition for UHCI register operation routines.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_UHCI_QUEUE_H_
#define _EFI_UHCI_QUEUE_H_

//
// Macroes used to set various links in UHCI's driver.
// In this UHCI driver, QH's horizontal link always pointers to other QH,
// and its vertical link always pointers to TD. TD's next pointer always
// pointers to other sibling TD. Frame link always pointers to QH because
// ISO transfer isn't supported.
//
// We should use UINT32 to access these pointers to void race conditions
// with hardware.
//
#define QH_HLINK(Pointer, Terminate)  \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | 0x02 | ((Terminate) ? 0x01 : 0))

#define QH_VLINK(Pointer, Terminate)  \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | ((Terminate) ? 0x01 : 0))

#define TD_LINK(Pointer, VertFirst, Terminate) \
        (((UINT32) ((UINTN) (Pointer)) & 0xFFFFFFF0) | \
         ((VertFirst) ? 0x04 : 0) | ((Terminate) ? 0x01 : 0))

#define LINK_TERMINATED(Link) (((Link) & 0x01) != 0)

#define UHCI_ADDR(QhOrTd)     ((VOID *) (UINTN) ((QhOrTd) & 0xFFFFFFF0))

#pragma pack(1)
//
// Both links in QH has this internal structure:
//   Next pointer: 28, Reserved: 2, NextIsQh: 1, Terminate: 1
// This is the same as frame list entry.
//
typedef struct {
  UINT32              HorizonLink;
  UINT32              VerticalLink;
} UHCI_QH_HW;

//
// Next link in TD has this internal structure:
//   Next pointer: 28, Reserved: 1, Vertical First: 1, NextIsQh: 1, Terminate: 1
//
typedef struct {
  UINT32              NextLink;
  UINT32              ActualLen   : 11;
  UINT32              Reserved1   : 5;
  UINT32              Status      : 8;
  UINT32              IntOnCpl    : 1;
  UINT32              IsIsoch     : 1;
  UINT32              LowSpeed    : 1;
  UINT32              ErrorCount  : 2;
  UINT32              ShortPacket : 1;
  UINT32              Reserved2   : 2;
  UINT32              PidCode     : 8;
  UINT32              DeviceAddr  : 7;
  UINT32              EndPoint    : 4;
  UINT32              DataToggle  : 1;
  UINT32              Reserved3   : 1;
  UINT32              MaxPacketLen: 11;
  UINT32              DataBuffer;
} UHCI_TD_HW;
#pragma pack()

typedef struct _UHCI_TD_SW  UHCI_TD_SW;
typedef struct _UHCI_QH_SW  UHCI_QH_SW;

struct _UHCI_QH_SW {
  UHCI_QH_HW        QhHw;
  UHCI_QH_SW        *NextQh;
  UHCI_TD_SW        *TDs;
  UINTN             Interval;
};

struct _UHCI_TD_SW {
  UHCI_TD_HW        TdHw;
  UHCI_TD_SW        *NextTd;
  UINT8             *Data;
  UINT16            DataLen;
};


/**
  Link the TD To QH.

  @param  Uhc         The UHCI device.
  @param  Qh          The queue head for the TD to link to.
  @param  Td          The TD to link.

**/
VOID
UhciLinkTdToQh (
  IN USB_HC_DEV           *Uhc,
  IN UHCI_QH_SW           *Qh,
  IN UHCI_TD_SW           *Td
  );


/**
  Unlink TD from the QH.

  @param  Qh          The queue head to unlink from.
  @param  Td          The TD to unlink.

  @return None.

**/
VOID
UhciUnlinkTdFromQh (
  IN UHCI_QH_SW           *Qh,
  IN UHCI_TD_SW           *Td
  );


/**
  Map address of request structure buffer.

  @param  Uhc                The UHCI device.
  @param  Request            The user request buffer.
  @param  MappedAddr         Mapped address of request.
  @param  Map                Identificaion of this mapping to return.

  @return EFI_SUCCESS        Success.
  @return EFI_DEVICE_ERROR   Fail to map the user request.

**/
EFI_STATUS
UhciMapUserRequest (
  IN  USB_HC_DEV          *Uhc,
  IN  OUT VOID            *Request,
  OUT UINT8               **MappedAddr,
  OUT VOID                **Map
  );


/**
  Map address of user data buffer.

  @param  Uhc                The UHCI device.
  @param  Direction          Direction of the data transfer.
  @param  Data               The user data buffer.
  @param  Len                Length of the user data.
  @param  PktId              Packet identificaion.
  @param  MappedAddr         Mapped address to return.
  @param  Map                Identificaion of this mapping to return.

  @return EFI_SUCCESS        Success.
  @return EFI_DEVICE_ERROR   Fail to map the user data.

**/
EFI_STATUS
UhciMapUserData (
  IN  USB_HC_DEV              *Uhc,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  VOID                    *Data,
  IN  OUT UINTN               *Len,
  OUT UINT8                   *PktId,
  OUT UINT8                   **MappedAddr,
  OUT VOID                    **Map
  );


/**
  Delete a list of TDs.

  @param  Uhc         The UHCI device.
  @param  FirstTd     TD link list head.

  @return None.

**/
VOID
UhciDestoryTds (
  IN USB_HC_DEV           *Uhc,
  IN UHCI_TD_SW           *FirstTd
  );


/**
  Create an initialize a new queue head.

  @param  Uhc         The UHCI device.
  @param  Interval    The polling interval for the queue.

  @return The newly created queue header.

**/
UHCI_QH_SW *
UhciCreateQh (
  IN  USB_HC_DEV        *Uhc,
  IN  UINTN             Interval
  );


/**
  Create Tds list for Control Transfer.

  @param  Uhc         The UHCI device.
  @param  DeviceAddr  The device address.
  @param  DataPktId   Packet Identification of Data Tds.
  @param  Request     A pointer to cpu memory address of request structure buffer to transfer.
  @param  RequestPhy  A pointer to pci memory address of request structure buffer to transfer.
  @param  Data        A pointer to cpu memory address of user data buffer to transfer.
  @param  DataPhy     A pointer to pci memory address of user data buffer to transfer.
  @param  DataLen     Length of user data to transfer.
  @param  MaxPacket   Maximum packet size for control transfer.
  @param  IsLow       Full speed or low speed.

  @return The Td list head for the control transfer.

**/
UHCI_TD_SW *
UhciCreateCtrlTds (
  IN USB_HC_DEV           *Uhc,
  IN UINT8                DeviceAddr,
  IN UINT8                DataPktId,
  IN UINT8                *Request,
  IN UINT8                *RequestPhy,
  IN UINT8                *Data,
  IN UINT8                *DataPhy,
  IN UINTN                DataLen,
  IN UINT8                MaxPacket,
  IN BOOLEAN              IsLow
  );


/**
  Create Tds list for Bulk/Interrupt Transfer.

  @param  Uhc         USB_HC_DEV.
  @param  DevAddr     Address of Device.
  @param  EndPoint    Endpoint Number.
  @param  PktId       Packet Identification of Data Tds.
  @param  Data        A pointer to cpu memory address of user data buffer to transfer.
  @param  DataPhy     A pointer to pci memory address of user data buffer to transfer.
  @param  DataLen     Length of user data to transfer.
  @param  DataToggle  Data Toggle Pointer.
  @param  MaxPacket   Maximum packet size for Bulk/Interrupt transfer.
  @param  IsLow       Is Low Speed Device.

  @return The Tds list head for the bulk transfer.

**/
UHCI_TD_SW *
UhciCreateBulkOrIntTds (
  IN USB_HC_DEV           *Uhc,
  IN UINT8                DevAddr,
  IN UINT8                EndPoint,
  IN UINT8                PktId,
  IN UINT8                *Data,
  IN UINT8                *DataPhy,
  IN UINTN                DataLen,
  IN OUT UINT8            *DataToggle,
  IN UINT8                MaxPacket,
  IN BOOLEAN              IsLow
  );

#endif
