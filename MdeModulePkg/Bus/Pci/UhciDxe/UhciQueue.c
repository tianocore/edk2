/** @file

  The UHCI register operation routines.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Uhci.h"


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
  )
{
  EFI_STATUS            Status;
  UINTN                 Len;
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  Len    = sizeof (EFI_USB_DEVICE_REQUEST);
  Status = Uhc->PciIo->Map (
                         Uhc->PciIo,
                         EfiPciIoOperationBusMasterRead,
                         Request,
                         &Len,
                         &PhyAddr,
                         Map
                         );

  if (!EFI_ERROR (Status)) {
    *MappedAddr = (UINT8 *) (UINTN) PhyAddr;
  }

  return Status;
}


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
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  Status = EFI_SUCCESS;

  switch (Direction) {
  case EfiUsbDataIn:
    //
    // BusMasterWrite means cpu read
    //
    *PktId = INPUT_PACKET_ID;
    Status = Uhc->PciIo->Map (
                           Uhc->PciIo,
                           EfiPciIoOperationBusMasterWrite,
                           Data,
                           Len,
                           &PhyAddr,
                           Map
                           );

    if (EFI_ERROR (Status)) {
      goto EXIT;
    }

    *MappedAddr = (UINT8 *) (UINTN) PhyAddr;
    break;

  case EfiUsbDataOut:
    *PktId = OUTPUT_PACKET_ID;
    Status = Uhc->PciIo->Map (
                           Uhc->PciIo,
                           EfiPciIoOperationBusMasterRead,
                           Data,
                           Len,
                           &PhyAddr,
                           Map
                           );

    if (EFI_ERROR (Status)) {
      goto EXIT;
    }

    *MappedAddr = (UINT8 *) (UINTN) PhyAddr;
    break;

  case EfiUsbNoData:
    if ((Len != NULL) && (*Len != 0)) {
      Status    = EFI_INVALID_PARAMETER;
      goto EXIT;
    }

    *PktId      = OUTPUT_PACKET_ID;
    *MappedAddr = NULL;
    *Map        = NULL;
    break;

  default:
    Status      = EFI_INVALID_PARAMETER;
  }

EXIT:
  return Status;
}


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
  )
{
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  PhyAddr = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Td, sizeof (UHCI_TD_HW));

  ASSERT ((Qh != NULL) && (Td != NULL));

  Qh->QhHw.VerticalLink = QH_VLINK (PhyAddr, FALSE);
  Qh->TDs               = (VOID *) Td;
}


/**
  Unlink TD from the QH.

  @param  Qh          The queue head to unlink from.
  @param  Td          The TD to unlink.

**/
VOID
UhciUnlinkTdFromQh (
  IN UHCI_QH_SW           *Qh,
  IN UHCI_TD_SW           *Td
  )
{
  ASSERT ((Qh != NULL) && (Td != NULL));

  Qh->QhHw.VerticalLink = QH_VLINK (NULL, TRUE);
  Qh->TDs               = NULL;
}


/**
  Append a new TD To the previous TD.

  @param  Uhc         The UHCI device.
  @param  PrevTd      Previous UHCI_TD_SW to be linked to.
  @param  ThisTd      TD to link.

**/
VOID
UhciAppendTd (
  IN USB_HC_DEV     *Uhc,
  IN UHCI_TD_SW     *PrevTd,
  IN UHCI_TD_SW     *ThisTd
  )
{
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  PhyAddr = UsbHcGetPciAddressForHostMem (Uhc->MemPool, ThisTd, sizeof (UHCI_TD_HW));

  ASSERT ((PrevTd != NULL) && (ThisTd != NULL));

  PrevTd->TdHw.NextLink = TD_LINK (PhyAddr, TRUE, FALSE);
  PrevTd->NextTd        = (VOID *) ThisTd;
}


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
  )
{
  UHCI_TD_SW            *NextTd;
  UHCI_TD_SW            *ThisTd;

  NextTd = FirstTd;

  while (NextTd != NULL) {
    ThisTd  = NextTd;
    NextTd  = ThisTd->NextTd;
    UsbHcFreeMem (Uhc->MemPool, ThisTd, sizeof (UHCI_TD_SW));
  }
}


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
  )
{
  UHCI_QH_SW            *Qh;

  Qh = UsbHcAllocateMem (Uhc->MemPool, sizeof (UHCI_QH_SW));

  if (Qh == NULL) {
    return NULL;
  }

  Qh->QhHw.HorizonLink  = QH_HLINK (NULL, TRUE);
  Qh->QhHw.VerticalLink = QH_VLINK (NULL, TRUE);
  Qh->Interval          = UhciConvertPollRate(Interval);
  Qh->TDs               = NULL;
  Qh->NextQh            = NULL;

  return Qh;
}


/**
  Create and intialize a TD.

  @param  Uhc         The UHCI device.

  @return The newly allocated and initialized TD.

**/
UHCI_TD_SW *
UhciCreateTd (
  IN  USB_HC_DEV          *Uhc
  )
{
  UHCI_TD_SW              *Td;

  Td     = UsbHcAllocateMem (Uhc->MemPool, sizeof (UHCI_TD_SW));
  if (Td == NULL) {
    return NULL;
  }

  Td->TdHw.NextLink = TD_LINK (NULL, FALSE, TRUE);
  Td->NextTd        = NULL;
  Td->Data          = NULL;
  Td->DataLen       = 0;

  return Td;
}


/**
  Create and initialize a TD for Setup Stage of a control transfer.

  @param  Uhc         The UHCI device.
  @param  DevAddr     Device address.
  @param  Request     A pointer to cpu memory address of Device request.
  @param  RequestPhy  A pointer to pci memory address of Device request.
  @param  IsLow       Full speed or low speed.

  @return The created setup Td Pointer.

**/
UHCI_TD_SW *
UhciCreateSetupTd (
  IN  USB_HC_DEV          *Uhc,
  IN  UINT8               DevAddr,
  IN  UINT8               *Request,
  IN  UINT8               *RequestPhy,
  IN  BOOLEAN             IsLow
  )
{
  UHCI_TD_SW              *Td;

  Td = UhciCreateTd (Uhc);

  if (Td == NULL) {
    return NULL;
  }

  Td->TdHw.NextLink     = TD_LINK (NULL, TRUE, TRUE);
  Td->TdHw.ShortPacket  = FALSE;
  Td->TdHw.IsIsoch      = FALSE;
  Td->TdHw.IntOnCpl     = FALSE;
  Td->TdHw.ErrorCount   = 0x03;
  Td->TdHw.Status      |= USBTD_ACTIVE;
  Td->TdHw.DataToggle   = 0;
  Td->TdHw.EndPoint     = 0;
  Td->TdHw.LowSpeed     = IsLow ? 1 : 0;
  Td->TdHw.DeviceAddr   = DevAddr & 0x7F;
  Td->TdHw.MaxPacketLen = (UINT32) (sizeof (EFI_USB_DEVICE_REQUEST) - 1);
  Td->TdHw.PidCode      = SETUP_PACKET_ID;
  Td->TdHw.DataBuffer   = (UINT32) (UINTN) RequestPhy;

  Td->Data              = Request;
  Td->DataLen           = (UINT16) sizeof (EFI_USB_DEVICE_REQUEST);

  return Td;
}


/**
  Create a TD for data.

  @param  Uhc         The UHCI device.
  @param  DevAddr     Device address.
  @param  Endpoint    Endpoint number.
  @param  DataPtr     A pointer to cpu memory address of Data buffer.
  @param  DataPhyPtr  A pointer to pci memory address of Data buffer.
  @param  Len         Data length.
  @param  PktId       Packet ID.
  @param  Toggle      Data toggle value.
  @param  IsLow       Full speed or low speed.

  @return Data Td pointer if success, otherwise NULL.

**/
UHCI_TD_SW *
UhciCreateDataTd (
  IN  USB_HC_DEV          *Uhc,
  IN  UINT8               DevAddr,
  IN  UINT8               Endpoint,
  IN  UINT8               *DataPtr,
  IN  UINT8               *DataPhyPtr,
  IN  UINTN               Len,
  IN  UINT8               PktId,
  IN  UINT8               Toggle,
  IN  BOOLEAN             IsLow
  )
{
  UHCI_TD_SW  *Td;

  //
  // Code as length - 1, and the max valid length is 0x500
  //
  ASSERT (Len <= 0x500);

  Td  = UhciCreateTd (Uhc);

  if (Td == NULL) {
    return NULL;
  }

  Td->TdHw.NextLink     = TD_LINK (NULL, TRUE, TRUE);
  Td->TdHw.ShortPacket  = FALSE;
  Td->TdHw.IsIsoch      = FALSE;
  Td->TdHw.IntOnCpl     = FALSE;
  Td->TdHw.ErrorCount   = 0x03;
  Td->TdHw.Status       = USBTD_ACTIVE;
  Td->TdHw.LowSpeed     = IsLow ? 1 : 0;
  Td->TdHw.DataToggle   = Toggle & 0x01;
  Td->TdHw.EndPoint     = Endpoint & 0x0F;
  Td->TdHw.DeviceAddr   = DevAddr & 0x7F;
  Td->TdHw.MaxPacketLen = (UINT32) (Len - 1);
  Td->TdHw.PidCode      = (UINT8) PktId;
  Td->TdHw.DataBuffer   = (UINT32) (UINTN) DataPhyPtr;

  Td->Data              = DataPtr;
  Td->DataLen           = (UINT16) Len;

  return Td;
}


/**
  Create TD for the Status Stage of control transfer.

  @param  Uhc         The UHCI device.
  @param  DevAddr     Device address.
  @param  PktId       Packet ID.
  @param  IsLow       Full speed or low speed.

  @return Status Td Pointer.

**/
UHCI_TD_SW *
UhciCreateStatusTd (
  IN  USB_HC_DEV          *Uhc,
  IN  UINT8               DevAddr,
  IN  UINT8               PktId,
  IN  BOOLEAN             IsLow
  )
{
  UHCI_TD_SW              *Td;

  Td = UhciCreateTd (Uhc);

  if (Td == NULL) {
    return NULL;
  }

  Td->TdHw.NextLink     = TD_LINK (NULL, TRUE, TRUE);
  Td->TdHw.ShortPacket  = FALSE;
  Td->TdHw.IsIsoch      = FALSE;
  Td->TdHw.IntOnCpl     = FALSE;
  Td->TdHw.ErrorCount   = 0x03;
  Td->TdHw.Status      |= USBTD_ACTIVE;
  Td->TdHw.MaxPacketLen = 0x7FF;      //0x7FF: there is no data (refer to UHCI spec)
  Td->TdHw.DataToggle   = 1;
  Td->TdHw.EndPoint     = 0;
  Td->TdHw.LowSpeed     = IsLow ? 1 : 0;
  Td->TdHw.DeviceAddr   = DevAddr & 0x7F;
  Td->TdHw.PidCode      = (UINT8) PktId;
  Td->TdHw.DataBuffer   = (UINT32) (UINTN) NULL;

  Td->Data              = NULL;
  Td->DataLen           = 0;

  return Td;
}


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
  )
{
  UHCI_TD_SW                *SetupTd;
  UHCI_TD_SW                *FirstDataTd;
  UHCI_TD_SW                *DataTd;
  UHCI_TD_SW                *PrevDataTd;
  UHCI_TD_SW                *StatusTd;
  UINT8                     DataToggle;
  UINT8                     StatusPktId;
  UINTN                     ThisTdLen;


  DataTd      = NULL;
  SetupTd     = NULL;
  FirstDataTd = NULL;
  PrevDataTd  = NULL;
  StatusTd    = NULL;

  //
  // Create setup packets for the transfer
  //
  SetupTd = UhciCreateSetupTd (Uhc, DeviceAddr, Request, RequestPhy, IsLow);

  if (SetupTd == NULL) {
    return NULL;
  }

  //
  // Create data packets for the transfer
  //
  DataToggle = 1;

  while (DataLen > 0) {
    //
    // PktSize is the data load size in each Td.
    //
    ThisTdLen = (DataLen > MaxPacket ? MaxPacket : DataLen);

    DataTd = UhciCreateDataTd (
               Uhc,
               DeviceAddr,
               0,
               Data,  //cpu memory address
               DataPhy, //Pci memory address
               ThisTdLen,
               DataPktId,
               DataToggle,
               IsLow
               );

    if (DataTd == NULL) {
      goto FREE_TD;
    }

    if (FirstDataTd == NULL) {
      FirstDataTd         = DataTd;
      FirstDataTd->NextTd = NULL;
    } else {
      UhciAppendTd (Uhc, PrevDataTd, DataTd);
    }

    DataToggle ^= 1;
    PrevDataTd = DataTd;
    Data += ThisTdLen;
    DataPhy += ThisTdLen;
    DataLen -= ThisTdLen;
  }

  //
  // Status packet is on the opposite direction to data packets
  //
  if (OUTPUT_PACKET_ID == DataPktId) {
    StatusPktId = INPUT_PACKET_ID;
  } else {
    StatusPktId = OUTPUT_PACKET_ID;
  }

  StatusTd = UhciCreateStatusTd (Uhc, DeviceAddr, StatusPktId, IsLow);

  if (StatusTd == NULL) {
    goto FREE_TD;
  }

  //
  // Link setup Td -> data Tds -> status Td together
  //
  if (FirstDataTd != NULL) {
    UhciAppendTd (Uhc, SetupTd, FirstDataTd);
    UhciAppendTd (Uhc, PrevDataTd, StatusTd);
  } else {
    UhciAppendTd (Uhc, SetupTd, StatusTd);
  }

  return SetupTd;

FREE_TD:
  if (SetupTd != NULL) {
    UhciDestoryTds (Uhc, SetupTd);
  }

  if (FirstDataTd != NULL) {
    UhciDestoryTds (Uhc, FirstDataTd);
  }

  return NULL;
}


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
  )
{
  UHCI_TD_SW              *DataTd;
  UHCI_TD_SW              *FirstDataTd;
  UHCI_TD_SW              *PrevDataTd;
  UINTN                   ThisTdLen;

  DataTd      = NULL;
  FirstDataTd = NULL;
  PrevDataTd  = NULL;

  //
  // Create data packets for the transfer
  //
  while (DataLen > 0) {
    //
    // PktSize is the data load size that each Td.
    //
    ThisTdLen = DataLen;

    if (DataLen > MaxPacket) {
      ThisTdLen = MaxPacket;
    }

    DataTd = UhciCreateDataTd (
               Uhc,
               DevAddr,
               EndPoint,
               Data,
               DataPhy,
               ThisTdLen,
               PktId,
               *DataToggle,
               IsLow
               );

    if (DataTd == NULL) {
      goto FREE_TD;
    }

    if (PktId == INPUT_PACKET_ID) {
      DataTd->TdHw.ShortPacket = TRUE;
    }

    if (FirstDataTd == NULL) {
      FirstDataTd         = DataTd;
      FirstDataTd->NextTd = NULL;
    } else {
      UhciAppendTd (Uhc, PrevDataTd, DataTd);
    }

    *DataToggle ^= 1;
    PrevDataTd   = DataTd;
    Data        += ThisTdLen;
    DataPhy     += ThisTdLen;
    DataLen     -= ThisTdLen;
  }

  return FirstDataTd;

FREE_TD:
  if (FirstDataTd != NULL) {
    UhciDestoryTds (Uhc, FirstDataTd);
  }

  return NULL;
}
