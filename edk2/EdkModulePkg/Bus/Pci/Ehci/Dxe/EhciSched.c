/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    EhciSched.c

Abstract:


Revision History
--*/

#include "Ehci.h"

STATIC
EFI_STATUS
SetAndWaitDoorBell (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINTN           Timeout
  )
/*++

Routine Description:

  Set DoorBell and wait it to complete

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;
  UINT32      Data;
  UINTN       Delay;

  Status = ReadEhcOperationalReg (
             HcDev,
             USBCMD,
             &Data
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Data |= USBCMD_IAAD;
  Status = WriteEhcOperationalReg (
             HcDev,
             USBCMD,
             Data
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
  }

  //
  // Timeout is in US unit
  //
  Delay = (Timeout / 50) + 1;
  do {
    Status = ReadEhcOperationalReg (
               HcDev,
               USBSTS,
               &Data
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    if ((Data & USBSTS_IAA) == USBSTS_IAA) {
      break;
    }

    gBS->Stall (EHCI_GENERIC_RECOVERY_TIME);

  } while (Delay--);

  Data = Data & 0xFFFFFFC0;
  Data |= USBSTS_IAA;
  Status = WriteEhcOperationalReg (
             HcDev,
             USBSTS,
             Data
             );

exit:
  return Status;
}





EFI_STATUS
CreateNULLQH (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Create the NULL QH to make it as the Async QH header

Arguments:

  HcDev   - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
--*/
{
  EFI_STATUS            Status;
  EHCI_QH_ENTITY        *NULLQhPtr;
  //
  // Allocate  memory for Qh structure
  //
  Status = EhciAllocatePool (
             HcDev,
             (UINT8 **) &NULLQhPtr,
             sizeof (EHCI_QH_ENTITY)
             );
  if (EFI_ERROR (Status)) {
     return Status;
  }

  NULLQhPtr->Qh.Status = QTD_STATUS_HALTED;
  NULLQhPtr->Qh.HeadReclamationFlag = 1;
  NULLQhPtr->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&(NULLQhPtr->Qh) >> 5));
  NULLQhPtr->Qh.SelectType = QH_SELECT_TYPE;
  NULLQhPtr->Qh.NextQtdTerminate = 1;

  NULLQhPtr->Next = NULLQhPtr;
  NULLQhPtr->Prev = NULLQhPtr;

  HcDev->NULLQH = NULLQhPtr;

  return Status;
}



VOID
DestroyNULLQH (
  IN  USB2_HC_DEV     *HcDev
  )
{

  if (HcDev->NULLQH != NULL) {
    EhciFreePool (HcDev, (UINT8 *)HcDev->NULLQH, sizeof (EHCI_QH_ENTITY));
    HcDev->NULLQH = NULL;
  }
}



EFI_STATUS
InitialPeriodicFrameList (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINTN           Length
  )
/*++

Routine Description:

  Initialize Periodic Schedule Frame List

Arguments:

  HcDev   - USB2_HC_DEV
  Length  - Frame List Length

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS            Status;
  VOID                  *CommonBuffer;
  EFI_PHYSICAL_ADDRESS  FrameBuffer;
  VOID                  *Map;
  UINTN                 BufferSizeInPages;
  UINTN                 BufferSizeInBytes;
  UINTN                 FrameIndex;
  FRAME_LIST_ENTRY      *FrameEntryPtr;

  //
  // The Frame List is a common buffer that will be
  // accessed by both the cpu and the usb bus master
  // at the same time.
  // The Frame List ocupies 4K bytes,
  // and must be aligned on 4-Kbyte boundaries.
  //
  if (EHCI_MAX_FRAME_LIST_LENGTH != Length && IsFrameListProgrammable (HcDev)) {
    Status = SetFrameListLen (HcDev, Length);
    if (EFI_ERROR (Status)) {
      Status = EFI_INVALID_PARAMETER;
      goto exit;
    }
  }

  BufferSizeInBytes = EFI_PAGE_SIZE;
  BufferSizeInPages = EFI_SIZE_TO_PAGES (BufferSizeInBytes);
  Status = HcDev->PciIo->AllocateBuffer (
                          HcDev->PciIo,
                          AllocateAnyPages,
                          EfiBootServicesData,
                          BufferSizeInPages,
                          &CommonBuffer,
                          0
                          );
  if (EFI_ERROR (Status)) {
    DEBUG ((gEHCErrorLevel, "EHCI: PciIo->AllocateBuffer Failed\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  Status = HcDev->PciIo->Map (
                          HcDev->PciIo,
                          EfiPciIoOperationBusMasterCommonBuffer,
                          CommonBuffer,
                          &BufferSizeInBytes,
                          &FrameBuffer,
                          &Map
                          );
  if (EFI_ERROR (Status) || (BufferSizeInBytes != EFI_PAGE_SIZE)) {
    DEBUG ((gEHCErrorLevel, "EHCI: PciIo->MapBuffer Failed\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto free_buffer;
  }

  //
  // Put high 32bit into CtrlDataStructSeg reg
  // when 64bit addressing range capability
  //
  if (HcDev->Is64BitCapable != 0) {
  	HcDev->High32BitAddr = (UINT32) GET_32B_TO_63B (FrameBuffer);

  	Status = SetCtrlDataStructSeg (HcDev);
    if (EFI_ERROR (Status)) {
      DEBUG ((gEHCErrorLevel, "EHCI: SetCtrlDataStructSeg Failed\n"));
      Status = EFI_DEVICE_ERROR;
      goto unmap_buffer;
    }
  }

  //
  // Tell the Host Controller where the Frame List lies,
  // by set the Frame List Base Address Register.
  //
  Status = SetFrameListBaseAddr (HcDev, (UINT32) FrameBuffer);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto unmap_buffer;
  }

  HcDev->PeriodicFrameListLength  = Length;
  HcDev->PeriodicFrameListBuffer  = (VOID *) ((UINTN) FrameBuffer);
  HcDev->PeriodicFrameListMap     = Map;

  //
  // Init Frame List Array fields
  //
  FrameEntryPtr = (FRAME_LIST_ENTRY *) HcDev->PeriodicFrameListBuffer;
  for (FrameIndex = 0; FrameIndex < HcDev->PeriodicFrameListLength; FrameIndex++) {
    FrameEntryPtr->LinkPointer    = 0;
    FrameEntryPtr->Rsvd           = 0;
    FrameEntryPtr->SelectType     = 0;
    FrameEntryPtr->LinkTerminate  = TRUE;
    FrameEntryPtr++;
  }

  goto exit;

unmap_buffer:
  HcDev->PciIo->Unmap (HcDev->PciIo, Map);
free_buffer:
  HcDev->PciIo->FreeBuffer (HcDev->PciIo, BufferSizeInPages, CommonBuffer);
exit:
  return Status;
}

VOID
DeinitialPeriodicFrameList (
  IN  USB2_HC_DEV     *HcDev
  )
/*++

Routine Description:

  Deinitialize Periodic Schedule Frame List

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  VOID

--*/
{
  HcDev->PciIo->Unmap (HcDev->PciIo, HcDev->PeriodicFrameListMap);
  HcDev->PciIo->FreeBuffer (HcDev->PciIo, EFI_SIZE_TO_PAGES (EFI_PAGE_SIZE), HcDev->PeriodicFrameListBuffer);
  return ;
}

EFI_STATUS
CreatePollingTimer (
  IN  USB2_HC_DEV      *HcDev,
  IN  EFI_EVENT_NOTIFY NotifyFunction
  )
/*++

Routine Description:

  Create Async Request Polling Timer

Arguments:

  HcDev          - USB2_HC_DEV
  NotifyFunction - Timer Notify Function

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  return gBS->CreateEvent (
                EVT_TIMER | EVT_NOTIFY_SIGNAL,
                TPL_NOTIFY,
                NotifyFunction,
                HcDev,
                &HcDev->AsyncRequestEvent
                );
}

EFI_STATUS
DestoryPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Destory Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  return gBS->CloseEvent (HcDev->AsyncRequestEvent);
}

EFI_STATUS
StartPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Start Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  return gBS->SetTimer (
                HcDev->AsyncRequestEvent,
                TimerPeriodic,
                EHCI_ASYNC_REQUEST_POLLING_TIME
                );
}

EFI_STATUS
StopPollingTimer (
  IN  USB2_HC_DEV *HcDev
  )
/*++

Routine Description:

  Stop Async Request Polling Timer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  return gBS->SetTimer (
                HcDev->AsyncRequestEvent,
                TimerCancel,
                EHCI_ASYNC_REQUEST_POLLING_TIME
                );
}

EFI_STATUS
CreateQh (
  IN  USB2_HC_DEV         *HcDev,
  IN  UINT8               DeviceAddr,
  IN  UINT8               Endpoint,
  IN  UINT8               DeviceSpeed,
  IN  UINTN               MaxPacketLen,
  OUT EHCI_QH_ENTITY      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh Structure and Pre-Initialize

Arguments:

  HcDev        - USB2_HC_DEV
  DeviceAddr   - Address of Device
  Endpoint     - Endpoint Number
  DeviceSpeed  - Device Speed
  MaxPacketLen - Max Length of one Packet
  QhPtrPtr     - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS  Status;
  EHCI_QH_HW  *QhHwPtr;

  ASSERT (HcDev);
  ASSERT (QhPtrPtr);

  *QhPtrPtr = NULL;

  //
  // Allocate  memory for Qh structure
  //
  Status = EhciAllocatePool (
             HcDev,
             (UINT8 **) QhPtrPtr,
             sizeof (EHCI_QH_ENTITY)
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  // Software field
  //
  (*QhPtrPtr)->Next         = NULL;
  (*QhPtrPtr)->Prev         = NULL;
  (*QhPtrPtr)->FirstQtdPtr  = NULL;
  (*QhPtrPtr)->AltQtdPtr    = NULL;
  (*QhPtrPtr)->LastQtdPtr   = NULL;

  //
  // Hardware field
  //
  QhHwPtr                       = &((*QhPtrPtr)->Qh);
  QhHwPtr->QhHorizontalPointer  = 0;
  QhHwPtr->SelectType           = 0;
  QhHwPtr->MaxPacketLen         = (UINT32) MaxPacketLen;
  QhHwPtr->EndpointSpeed        = (DeviceSpeed & 0x3);
  QhHwPtr->EndpointNum          = (Endpoint & 0x0F);
  QhHwPtr->DeviceAddr           = (DeviceAddr & 0x7F);
  QhHwPtr->Multiplier           = HIGH_BANDWIDTH_PIPE_MULTIPLIER;
  QhHwPtr->Rsvd1                = 0;
  QhHwPtr->Rsvd2                = 0;
  QhHwPtr->Rsvd3                = 0;
  QhHwPtr->Rsvd4                = 0;
  QhHwPtr->Rsvd5                = 0;
  QhHwPtr->Rsvd6                = 0;

exit:
  return Status;
}

VOID
DestoryQh (
  IN USB2_HC_DEV         *HcDev,
  IN EHCI_QH_ENTITY      *QhPtr
  )
/*++

Routine Description:

  Destory Qh Structure

Arguments:

  HcDev - USB2_HC_DEV
  QhPtr - A pointer to Qh

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  ASSERT (HcDev);
  ASSERT (QhPtr);

  EhciFreePool (HcDev, (UINT8 *) QhPtr, sizeof (EHCI_QH_ENTITY));
  return ;
}

EFI_STATUS
CreateControlQh (
  IN  USB2_HC_DEV                         *HcDev,
  IN  UINT8                               DeviceAddr,
  IN  UINT8                               DeviceSpeed,
  IN  UINTN                               MaxPacketLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QH_ENTITY                      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Control Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  DeviceAddr   - Address of Device
  DeviceSpeed  - Device Speed
  MaxPacketLen - Max Length of one Packet
  Translator   - Translator Transaction for SplitX
  QhPtrPtr     - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS  Status;

  //
  // Create and init Control Qh
  //
  Status = CreateQh (
             HcDev,
             DeviceAddr,
             0,
             DeviceSpeed,
             MaxPacketLen,
             QhPtrPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }
  //
  // Software field
  //
  (*QhPtrPtr)->Next         = (*QhPtrPtr);
  (*QhPtrPtr)->Prev         = (*QhPtrPtr);
  (*QhPtrPtr)->TransferType = CONTROL_TRANSFER;

  //
  // Hardware field
  //
  // Control Transfer use DataToggleControl
  //
  (*QhPtrPtr)->Qh.DataToggleControl   = TRUE;
  (*QhPtrPtr)->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&((*QhPtrPtr)->Qh)) >> 5);
  (*QhPtrPtr)->Qh.SelectType          = QH_SELECT_TYPE;
  (*QhPtrPtr)->Qh.QhTerminate         = FALSE;
  if (EFI_USB_SPEED_HIGH != DeviceSpeed)  {
    (*QhPtrPtr)->Qh.ControlEndpointFlag = TRUE;
  }
  (*QhPtrPtr)->Qh.NakCountReload      = NAK_COUNT_RELOAD;
  if (NULL != Translator) {
    (*QhPtrPtr)->Qh.PortNum = Translator->TranslatorPortNumber;
    (*QhPtrPtr)->Qh.HubAddr = Translator->TranslatorHubAddress;
    (*QhPtrPtr)->Qh.Status |= QTD_STATUS_DO_START_SPLIT;
  }

exit:
  return Status;
}

EFI_STATUS
CreateBulkQh (
  IN  USB2_HC_DEV                         *HcDev,
  IN  UINT8                               DeviceAddr,
  IN  UINT8                               EndPointAddr,
  IN  UINT8                               DeviceSpeed,
  IN  UINT8                               DataToggle,
  IN  UINTN                               MaxPacketLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QH_ENTITY                      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Bulk Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  DeviceAddr   - Address of Device
  EndPointAddr - Address of Endpoint
  DeviceSpeed  - Device Speed
  MaxPacketLen - Max Length of one Packet
  Translator   - Translator Transaction for SplitX
  QhPtrPtr     - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS  Status;

  //
  // Create and init Bulk Qh
  //
  Status = CreateQh (
             HcDev,
             DeviceAddr,
             EndPointAddr,
             DeviceSpeed,
             MaxPacketLen,
             QhPtrPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  // Software fields
  //
  (*QhPtrPtr)->Next         = (*QhPtrPtr);
  (*QhPtrPtr)->Prev         = (*QhPtrPtr);
  (*QhPtrPtr)->TransferType = BULK_TRANSFER;

  //
  // Hardware fields
  //
  // BulkTransfer don't use DataToggleControl
  //
  (*QhPtrPtr)->Qh.DataToggleControl   = FALSE;
  (*QhPtrPtr)->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&((*QhPtrPtr)->Qh)) >> 5);
  (*QhPtrPtr)->Qh.SelectType          = QH_SELECT_TYPE;
  (*QhPtrPtr)->Qh.QhTerminate         = FALSE;
  (*QhPtrPtr)->Qh.NakCountReload      = NAK_COUNT_RELOAD;
  (*QhPtrPtr)->Qh.DataToggle          = DataToggle;
  if (NULL != Translator) {
    (*QhPtrPtr)->Qh.PortNum = Translator->TranslatorPortNumber;
    (*QhPtrPtr)->Qh.HubAddr = Translator->TranslatorHubAddress;
    (*QhPtrPtr)->Qh.Status |= QTD_STATUS_DO_START_SPLIT;
  }

exit:
  return Status;
}

EFI_STATUS
CreateInterruptQh (
  IN  USB2_HC_DEV                         *HcDev,
  IN  UINT8                               DeviceAddr,
  IN  UINT8                               EndPointAddr,
  IN  UINT8                               DeviceSpeed,
  IN  UINT8                               DataToggle,
  IN  UINTN                               MaxPacketLen,
  IN  UINTN                               Interval,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QH_ENTITY                      **QhPtrPtr
  )
/*++

Routine Description:

  Create Qh for Control Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  DeviceAddr   - Address of Device
  EndPointAddr - Address of Endpoint
  DeviceSpeed  - Device Speed
  MaxPacketLen - Max Length of one Packet
  Interval     - value of interval
  Translator   - Translator Transaction for SplitX
  QhPtrPtr     - A pointer of pointer to Qh for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS  Status;

  //
  // Create and init InterruptQh
  //
  Status = CreateQh (
             HcDev,
             DeviceAddr,
             EndPointAddr,
             DeviceSpeed,
             MaxPacketLen,
             QhPtrPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  // Software fields
  //
  if (Interval == 0) {
    (*QhPtrPtr)->TransferType = SYNC_INTERRUPT_TRANSFER;
  } else {
    (*QhPtrPtr)->TransferType = ASYNC_INTERRUPT_TRANSFER;
  }
  (*QhPtrPtr)->Interval = GetApproxiOfInterval (Interval);

  //
  // Hardware fields
  //
  // InterruptTranfer don't use DataToggleControl
  //
  (*QhPtrPtr)->Qh.DataToggleControl     = FALSE;
  (*QhPtrPtr)->Qh.QhHorizontalPointer   = 0;
  (*QhPtrPtr)->Qh.QhTerminate           = TRUE;
  (*QhPtrPtr)->Qh.NakCountReload        = 0;
  (*QhPtrPtr)->Qh.InerruptScheduleMask  = MICRO_FRAME_0_CHANNEL;
  (*QhPtrPtr)->Qh.SplitComletionMask    = (MICRO_FRAME_2_CHANNEL | MICRO_FRAME_3_CHANNEL | MICRO_FRAME_4_CHANNEL);
  (*QhPtrPtr)->Qh.DataToggle            = DataToggle;
  if (NULL != Translator) {
    (*QhPtrPtr)->Qh.PortNum = Translator->TranslatorPortNumber;
    (*QhPtrPtr)->Qh.HubAddr = Translator->TranslatorHubAddress;
    (*QhPtrPtr)->Qh.Status |= QTD_STATUS_DO_START_SPLIT;
  }

exit:
  return Status;
}

EFI_STATUS
CreateQtd (
  IN  USB2_HC_DEV          *HcDev,
  IN  UINT8                *DataPtr,
  IN  UINTN                DataLen,
  IN  UINT8                PktId,
  IN  UINT8                Toggle,
  IN  UINT8                QtdStatus,
  OUT EHCI_QTD_ENTITY      **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure and Pre-Initialize it

Arguments:

  HcDev       - USB2_HC_DEV
  DataPtr     - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  PktId       - Packet Identification of this Qtd
  Toggle      - Data Toggle of this Qtd
  QtdStatus   - Default value of status of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS  Status;
  EHCI_QTD_HW *QtdHwPtr;

  ASSERT (HcDev);
  ASSERT (QtdPtrPtr);

  //
  // Create memory for Qtd structure
  //
  Status = EhciAllocatePool (
             HcDev,
             (UINT8 **) QtdPtrPtr,
             sizeof (EHCI_QTD_ENTITY)
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  // Software field
  //
  (*QtdPtrPtr)->TotalBytes        = (UINT32) DataLen;
  (*QtdPtrPtr)->StaticTotalBytes  = (UINT32) DataLen;
  (*QtdPtrPtr)->Prev              = NULL;
  (*QtdPtrPtr)->Next              = NULL;

  //
  // Hardware field
  //
  QtdHwPtr                      = &((*QtdPtrPtr)->Qtd);
  QtdHwPtr->NextQtdPointer      = 0;
  QtdHwPtr->NextQtdTerminate    = TRUE;
  QtdHwPtr->AltNextQtdPointer   = 0;
  QtdHwPtr->AltNextQtdTerminate = TRUE;
  QtdHwPtr->DataToggle          = Toggle;
  QtdHwPtr->TotalBytes          = (UINT32) DataLen;
  QtdHwPtr->CurrentPage         = 0;
  QtdHwPtr->ErrorCount          = QTD_ERROR_COUNTER;
  QtdHwPtr->Status              = QtdStatus;
  QtdHwPtr->Rsvd1               = 0;
  QtdHwPtr->Rsvd2               = 0;
  QtdHwPtr->Rsvd3               = 0;
  QtdHwPtr->Rsvd4               = 0;
  QtdHwPtr->Rsvd5               = 0;
  QtdHwPtr->Rsvd6               = 0;

  //
  // Set PacketID [Setup/Data/Status]
  //
  switch (PktId) {
  case SETUP_PACKET_ID:
    QtdHwPtr->PidCode = SETUP_PACKET_PID_CODE;
    break;

  case INPUT_PACKET_ID:
    QtdHwPtr->PidCode = INPUT_PACKET_PID_CODE;
    break;

  case OUTPUT_PACKET_ID:
    QtdHwPtr->PidCode = OUTPUT_PACKET_PID_CODE;
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  //
  // Set Data Buffer Pointers
  //
  if (NULL != DataPtr) {
    SetQtdBufferPointer (
      QtdHwPtr,
      DataPtr,
      DataLen
      );
    (*QtdPtrPtr)->StaticCurrentOffset = QtdHwPtr->CurrentOffset;
  }

exit:
  return Status;
}

EFI_STATUS
CreateSetupQtd (
  IN  USB2_HC_DEV          *HcDev,
  IN  UINT8                *DevReqPtr,
  OUT EHCI_QTD_ENTITY      **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for Setup

Arguments:

  HcDev      - USB2_HC_DEV
  DevReqPtr  - A pointer to Device Request Data
  QtdPtrPtr  - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  return CreateQtd (
           HcDev,
           DevReqPtr,
           sizeof (EFI_USB_DEVICE_REQUEST),
           SETUP_PACKET_ID,
           DATA0,
           QTD_STATUS_ACTIVE,
           QtdPtrPtr
           );
}

EFI_STATUS
CreateDataQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 *DataPtr,
  IN  UINTN                 DataLen,
  IN  UINT8                 PktId,
  IN  UINT8                 Toggle,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for data

Arguments:

  HcDev       - USB2_HC_DEV
  DataPtr     - A pointer to user data buffer to transfer
  DataLen     - Length of user data to transfer
  PktId       - Packet Identification of this Qtd
  Toggle      - Data Toggle of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  return CreateQtd (
           HcDev,
           DataPtr,
           DataLen,
           PktId,
           Toggle,
           QTD_STATUS_ACTIVE,
           QtdPtrPtr
           );
}

EFI_STATUS
CreateAltQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 PktId,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for Alternative

Arguments:

  HcDev      - USB2_HC_DEV
  PktId      - Packet Identification of this Qtd
  QtdPtrPtr  - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  return CreateQtd (
           HcDev,
           NULL,
           0,
           PktId,
           0,
           QTD_STATUS_ACTIVE,
           QtdPtrPtr
           );
}

EFI_STATUS
CreateStatusQtd (
  IN  USB2_HC_DEV           *HcDev,
  IN  UINT8                 PktId,
  OUT EHCI_QTD_ENTITY       **QtdPtrPtr
  )
/*++

Routine Description:

  Create Qtd Structure for status

Arguments:

  HcDev       - USB2_HC_DEV
  PktId       - Packet Identification of this Qtd
  QtdPtrPtr   - A pointer of pointer to Qtd for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  return CreateQtd (
           HcDev,
           NULL,
           0,
           PktId,
           DATA1,
           QTD_STATUS_ACTIVE,
           QtdPtrPtr
           );
}

EFI_STATUS
CreateControlQtds (
  IN  USB2_HC_DEV                         *HcDev,
  IN UINT8                                DataPktId,
  IN UINT8                                *RequestCursor,
  IN UINT8                                *DataCursor,
  IN UINTN                                DataLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QTD_ENTITY                     **ControlQtdsHead
  )
/*++

Routine Description:

  Create Qtds list for Control Transfer

Arguments:

  HcDev           - USB2_HC_DEV
  DataPktId       - Packet Identification of Data Qtds
  RequestCursor   - A pointer to request structure buffer to transfer
  DataCursor      - A pointer to user data buffer to transfer
  DataLen         - Length of user data to transfer
  ControlQtdsHead - A pointer of pointer to first Qtd for control tranfer for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS      Status;
  EHCI_QTD_ENTITY *QtdPtr;
  EHCI_QTD_ENTITY *PreQtdPtr;
  EHCI_QTD_ENTITY *SetupQtdPtr;
  EHCI_QTD_ENTITY *FirstDataQtdPtr;
  EHCI_QTD_ENTITY *LastDataQtdPtr;
  EHCI_QTD_ENTITY *StatusQtdPtr;
  UINT8           DataToggle;
  UINT8           StatusPktId;
  UINTN           CapacityOfQtd;
  UINTN           SizePerQtd;
  UINTN           DataCount;

  QtdPtr          = NULL;
  PreQtdPtr       = NULL;
  SetupQtdPtr     = NULL;
  FirstDataQtdPtr = NULL;
  LastDataQtdPtr  = NULL;
  StatusQtdPtr    = NULL;
  CapacityOfQtd = 0;

  //
  //  Setup Stage of Control Transfer
  //
  Status = CreateSetupQtd (
             HcDev,
             RequestCursor,
             &SetupQtdPtr
             );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //
  //  Data Stage of Control Transfer
  //
  DataToggle  = 1;
  DataCount   = DataLen;

  //
  // Create Qtd structure and link together
  //
  while (DataCount > 0) {
    //
    // PktSize is the data load size that each Qtd.
    //
    CapacityOfQtd = GetCapacityOfQtd (DataCursor);
    SizePerQtd    = DataCount;
    if (DataCount > CapacityOfQtd) {
      SizePerQtd = CapacityOfQtd;
    }

    Status = CreateDataQtd (
               HcDev,
               DataCursor,
               SizePerQtd,
               DataPktId,
               DataToggle,
               &QtdPtr
               );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      if (NULL == FirstDataQtdPtr) {
        goto destory_setup_qtd;
      } else {
        goto destory_qtds;
      }
    }

    if (NULL == FirstDataQtdPtr) {
      FirstDataQtdPtr = QtdPtr;
    } else {
      LinkQtdToQtd (PreQtdPtr, QtdPtr);
    }

    DataToggle ^= 1;

    PreQtdPtr = QtdPtr;
    DataCursor += SizePerQtd;
    DataCount -= SizePerQtd;
  }

  LastDataQtdPtr = QtdPtr;

  //
  // Status Stage of Control Transfer
  //
  if (OUTPUT_PACKET_ID == DataPktId) {
    StatusPktId = INPUT_PACKET_ID;
  } else {
    StatusPktId = OUTPUT_PACKET_ID;
  }

  Status = CreateStatusQtd (
            HcDev,
            StatusPktId,
            &StatusQtdPtr
            );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto destory_qtds;
  }

  //
  // Link setup Qtd -> data Qtds -> status Qtd
  //
  if (FirstDataQtdPtr != NULL) {
    LinkQtdToQtd (SetupQtdPtr, FirstDataQtdPtr);
    LinkQtdToQtd (LastDataQtdPtr, StatusQtdPtr);
  } else {
    LinkQtdToQtd (SetupQtdPtr, StatusQtdPtr);
  }

  *ControlQtdsHead = SetupQtdPtr;

  goto exit;

destory_qtds:
  DestoryQtds (HcDev, FirstDataQtdPtr);
destory_setup_qtd:
  DestoryQtds (HcDev, SetupQtdPtr);
exit:
  return Status;
}

EFI_STATUS
CreateBulkOrInterruptQtds (
  IN  USB2_HC_DEV                         *HcDev,
  IN  UINT8                               PktId,
  IN  UINT8                               *DataCursor,
  IN  UINTN                               DataLen,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT EHCI_QTD_ENTITY                     **QtdsHead
  )
/*++

Routine Description:

  Create Qtds list for Bulk or Interrupt Transfer

Arguments:

  HcDev        - USB2_HC_DEV
  PktId        - Packet Identification of Qtds
  DataCursor   - A pointer to user data buffer to transfer
  DataLen      - Length of user data to transfer
  DataToggle   - Data Toggle to start
  Translator   - Translator Transaction for SplitX
  QtdsHead     - A pointer of pointer to first Qtd for control tranfer for return

Returns:

  EFI_SUCCESS            Success
  EFI_OUT_OF_RESOURCES   Cannot allocate resources

--*/
{
  EFI_STATUS      Status;
  EHCI_QTD_ENTITY *QtdPtr;
  EHCI_QTD_ENTITY *PreQtdPtr;
  EHCI_QTD_ENTITY *FirstQtdPtr;
  EHCI_QTD_ENTITY *AltQtdPtr;
  UINTN           DataCount;
  UINTN           CapacityOfQtd;
  UINTN           SizePerQtd;

  Status        = EFI_SUCCESS;
  QtdPtr        = NULL;
  PreQtdPtr     = NULL;
  FirstQtdPtr   = NULL;
  AltQtdPtr     = NULL;
  CapacityOfQtd = 0;

  DataCount   = DataLen;
  while (DataCount > 0) {

    CapacityOfQtd = GetCapacityOfQtd (DataCursor);
    SizePerQtd    = DataCount;
    if (DataCount > CapacityOfQtd) {
      SizePerQtd = CapacityOfQtd;
    }

    Status = CreateDataQtd (
              HcDev,
              DataCursor,
              SizePerQtd,
              PktId,
              0,
              &QtdPtr
              );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      if (NULL == FirstQtdPtr) {
        goto exit;
      } else {
        goto destory_qtds;
      }
    }

    if (NULL == FirstQtdPtr) {
      FirstQtdPtr = QtdPtr;
    } else {
      LinkQtdToQtd (PreQtdPtr, QtdPtr);
    }

    PreQtdPtr = QtdPtr;
    DataCursor += SizePerQtd;
    DataCount -= SizePerQtd;
  }

  //
  // Set Alternate Qtd
  //
  if (INPUT_PACKET_ID == PktId && 0 < GetNumberOfQtd (FirstQtdPtr)) {
    Status = CreateAltQtd (
              HcDev,
              PktId,
              &AltQtdPtr
              );
    if (EFI_ERROR (Status)) {
      Status = EFI_OUT_OF_RESOURCES;
      goto destory_qtds;
    }

    LinkQtdsToAltQtd (FirstQtdPtr, AltQtdPtr);
  }

  *QtdsHead = FirstQtdPtr;
  goto exit;

destory_qtds:
  DestoryQtds (HcDev, FirstQtdPtr);
exit:
  return Status;
}

VOID
DestoryQtds (
  IN USB2_HC_DEV          *HcDev,
  IN EHCI_QTD_ENTITY      *FirstQtdPtr
  )
/*++

Routine Description:

  Destory all Qtds in the list

Arguments:

  HcDev         - USB2_HC_DEV
  FirstQtdPtr   - A pointer to first Qtd in the list

Returns:

  VOID

--*/
{
  EHCI_QTD_ENTITY *PrevQtd;
  EHCI_QTD_ENTITY *NextQtd;

  if (!FirstQtdPtr) {
    goto exit;
  }

  PrevQtd = FirstQtdPtr;

  //
  // Delete all the Qtds.
  //
  do {
    NextQtd = PrevQtd->Next;
    EhciFreePool (HcDev, (UINT8 *) PrevQtd, sizeof (EHCI_QTD_ENTITY));
    PrevQtd = NextQtd;
  } while (NULL != PrevQtd);

exit:
  return ;
}

UINTN
GetNumberOfQtd (
  IN EHCI_QTD_ENTITY    *FirstQtdPtr
  )
/*++

Routine Description:

  Number of Qtds in the list

Arguments:

  FirstQtdPtr - A pointer to first Qtd in the list

Returns:

  Number of Qtds in the list

--*/
{
  UINTN           Count;
  EHCI_QTD_ENTITY *QtdPtr;
  Count   = 0;
  QtdPtr  = FirstQtdPtr;

  while (NULL != QtdPtr) {
    Count++;
    QtdPtr = QtdPtr->Next;
  }

  return Count;
}

UINTN
GetCapacityOfQtd (
  IN UINT8    *BufferCursor
  )
/*++

Routine Description:

  Get Size of First Qtd

Arguments:

  BufferCursor       - BufferCursor of the Qtd

Returns:

  Size of First Qtd

--*/
{

 if (EFI_PAGE_MASK & GET_0B_TO_31B (BufferCursor)) {
   return EFI_PAGE_SIZE * 4;
 } else {
   return EFI_PAGE_SIZE * 5;
 }

}

UINTN
GetApproxiOfInterval (
  IN UINTN  Interval
  )
/*++

Routine Description:

  Get the approximate value in the 2 index sequence

Arguments:

  Interval  - the value of interval

Returns:

  approximate value of interval in the 2 index sequence

--*/
{
  UINTN Orignate;
  UINTN Approxi;

  Orignate  = Interval;
  Approxi   = 1;

  while (Orignate != 1 && Orignate != 0) {
    Orignate  = Orignate >> 1;
    Approxi   = Approxi << 1;
  }

  if (Interval & (Approxi >> 1)) {
    Approxi = Approxi << 1;
  }

  return Approxi;
}

EHCI_QTD_HW *
GetQtdAlternateNextPointer (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Get Qtd alternate next pointer field

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  A pointer to hardware alternate Qtd

--*/
{
  EHCI_QTD_HW *Value;

  Value = NULL;

  if (!HwQtdPtr->AltNextQtdTerminate) {
    Value = (EHCI_QTD_HW *) GET_0B_TO_31B (HwQtdPtr->AltNextQtdPointer << 5);
  }

  return Value;
}

EHCI_QTD_HW *
GetQtdNextPointer (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Get Qtd next pointer field

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  A pointer to next hardware Qtd structure

--*/
{
  EHCI_QTD_HW *Value;

  Value = NULL;

  if (!HwQtdPtr->NextQtdTerminate) {
    Value = (EHCI_QTD_HW *) GET_0B_TO_31B (HwQtdPtr->NextQtdPointer << 5);
  }

  return Value;
}

VOID
LinkQtdToQtd (
  IN EHCI_QTD_ENTITY * PreQtdPtr,
  IN EHCI_QTD_ENTITY * QtdPtr
  )
/*++

Routine Description:

  Link Qtds together

Arguments:

  PreQtdPtr   - A pointer to pre Qtd
  QtdPtr      - A pointer to next Qtd

Returns:

  VOID

--*/
{
  EHCI_QTD_HW *QtdHwPtr;

  ASSERT(PreQtdPtr);
  ASSERT(QtdPtr);

  //
  // Software link
  //
  PreQtdPtr->Next = QtdPtr;
  QtdPtr->Prev    = PreQtdPtr;

  //
  // Hardware link
  //
  QtdHwPtr                        = &(QtdPtr->Qtd);
  PreQtdPtr->Qtd.NextQtdPointer   = (UINT32) (GET_0B_TO_31B(QtdHwPtr) >> 5);
  PreQtdPtr->Qtd.NextQtdTerminate = FALSE;

  return ;
}


VOID
LinkQtdsToAltQtd (
  IN EHCI_QTD_ENTITY  * FirstQtdPtr,
  IN EHCI_QTD_ENTITY  * AltQtdPtr
  )
/*++

Routine Description:

  Link AlterQtds together

Arguments:

  FirstQtdPtr  - A pointer to first Qtd in the list
  AltQtdPtr    - A pointer to alternative Qtd

Returns:

  VOID

--*/
{
  EHCI_QTD_ENTITY *QtdPtr;
  EHCI_QTD_HW     *AltQtdHwPtr;

  ASSERT(FirstQtdPtr);
  ASSERT(AltQtdPtr);

  AltQtdHwPtr = &(AltQtdPtr->Qtd);
  QtdPtr      = FirstQtdPtr;

  while (NULL != QtdPtr) {
    //
    // Software link
    //
    QtdPtr->AltNext = AltQtdPtr;
    //
    // Hardware link
    //
    QtdPtr->Qtd.AltNextQtdPointer   = (UINT32) (GET_0B_TO_31B(AltQtdHwPtr) >> 5);
    QtdPtr->Qtd.AltNextQtdTerminate = FALSE;
    QtdPtr                          = QtdPtr->Next;
  }

  return ;
}

VOID
LinkQtdToQh (
  IN EHCI_QH_ENTITY      *QhPtr,
  IN EHCI_QTD_ENTITY     *QtdPtr
  )
/*++

Routine Description:

  Link Qtds list to Qh

Arguments:

  QhPtr    - A pointer to Qh
  QtdPtr   - A pointer to first Qtd in the list

Returns:

  VOID

--*/
{
  EHCI_QTD_ENTITY *Cursor;
  EHCI_QTD_HW     *QtdHwPtr;

  ASSERT (QhPtr);
  ASSERT (QtdPtr);

  QhPtr->FirstQtdPtr = QtdPtr;
  if (NULL != QtdPtr->AltNext) {
    QhPtr->AltQtdPtr = QtdPtr->AltNext;
  }

  Cursor = QtdPtr;
  while (NULL != Cursor) {
    Cursor->SelfQh = QhPtr;
    if (NULL == Cursor->Next) {
      QhPtr->LastQtdPtr = Cursor;
    }

    Cursor = Cursor->Next;
  }

  QtdHwPtr                    = &(QtdPtr->Qtd);
  QhPtr->Qh.NextQtdPointer    = (UINT32) (GET_0B_TO_31B (QtdHwPtr) >> 5);
  QhPtr->Qh.NextQtdTerminate  = FALSE;

  QhPtr->Qh.AltNextQtdPointer    = 0;
  QhPtr->Qh.AltNextQtdTerminate  = TRUE;


  if ((QtdPtr->Qtd.PidCode == OUTPUT_PACKET_PID_CODE) &&
      (QhPtr->TransferType == BULK_TRANSFER)) {
      //
      //Start PING first
      //
      QhPtr->Qh.Status |= QTD_STATUS_DO_PING;
    }

  return ;
}

EFI_STATUS
LinkQhToAsyncList (
  IN  USB2_HC_DEV       *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Link Qh to Async Schedule List

Arguments:

  HcDev  - USB2_HC_DEV
  QhPtr  - A pointer to Qh

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;

  ASSERT (HcDev);
  ASSERT (QhPtr);


  //
  // NULL QH created before
  //

  HcDev->NULLQH->Next = QhPtr;
  HcDev->NULLQH->Prev = QhPtr;

  QhPtr->Next = HcDev->NULLQH;
  QhPtr->Prev = HcDev->NULLQH;


  HcDev->NULLQH->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&(QhPtr->Qh) >> 5));
  QhPtr->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&(HcDev->NULLQH->Qh) >> 5));


  Status = SetAsyncListAddr (HcDev, HcDev->NULLQH);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  if (!IsAsyncScheduleEnabled (HcDev)) {

    Status = EnableAsynchronousSchedule (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    Status = WaitForAsyncScheduleEnable (HcDev, EHCI_GENERIC_TIMEOUT);
    if (EFI_ERROR (Status)) {
      DEBUG ((gEHCDebugLevel, "EHCI: WaitForAsyncScheduleEnable TimeOut"));
      Status = EFI_TIMEOUT;
      goto exit;
    }

    if (IsEhcHalted (HcDev)) {
      Status = StartScheduleExecution (HcDev);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
      }
    }

  }

exit:
  return Status;
}

EFI_STATUS
UnlinkQhFromAsyncList (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Unlink Qh from Async Schedule List

Arguments:

  HcDev  - USB2_HC_DEV
  QhPtr  - A pointer to Qh

Returns:

  EFI_SUCCESS       Success
  EFI_DEVICE_ERROR  Fail

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  ASSERT (HcDev);
  ASSERT (QhPtr);


  HcDev->NULLQH->Next = HcDev->NULLQH;
  HcDev->NULLQH->Prev = HcDev->NULLQH;


  QhPtr->Next = QhPtr;
  QhPtr->Prev = QhPtr;

  HcDev->NULLQH->Qh.QhHorizontalPointer = (UINT32) (GET_0B_TO_31B (&(HcDev->NULLQH->Qh) >> 5));


  SetAndWaitDoorBell (HcDev, 2 * EHCI_GENERIC_TIMEOUT);

  QhPtr->Qh.QhTerminate         = 1;
  QhPtr->Qh.QhHorizontalPointer = 0;


  Status = DisableAsynchronousSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForAsyncScheduleDisable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    DEBUG ((gEHCErrorLevel, "EHCI: WaitForAsyncScheduleDisable TimeOut\n"));
    Status = EFI_TIMEOUT;
    goto exit;
  }


exit:
  return Status;
}

VOID
LinkQhToPeriodicList (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr
  )
/*++

Routine Description:

  Link Qh to Periodic Schedule List

Arguments:

  HcDev  - USB2_HC_DEV
  QhPtr  - A pointer to Qh

Returns:

  VOID

--*/
{
  FRAME_LIST_ENTRY  *FrameEntryPtr;
  EHCI_QH_ENTITY    *FindQhPtr;
  EHCI_QH_HW        *FindQhHwPtr;
  UINTN             FrameIndex;

  ASSERT (HcDev);
  ASSERT (QhPtr);

  FindQhPtr                     = NULL;
  FindQhHwPtr                   = NULL;
  FrameIndex                    = 0;
  FrameEntryPtr                 = (FRAME_LIST_ENTRY *) HcDev->PeriodicFrameListBuffer;

  QhPtr->Qh.HeadReclamationFlag = FALSE;

  if (QhPtr->TransferType == ASYNC_INTERRUPT_TRANSFER) {

    //
    // AsyncInterruptTransfer Qh
    //

    //
    // Link to Frame[0] List
    //
    if (!FrameEntryPtr->LinkTerminate) {
      //
      // Not Null FrameList
      //
      FindQhHwPtr = (EHCI_QH_HW *) GET_0B_TO_31B (FrameEntryPtr->LinkPointer << 5);
      FindQhPtr   = (EHCI_QH_ENTITY *) GET_QH_ENTITY_ADDR (FindQhHwPtr);
      //
      // FindQh is Left/Right to Qh
      //
      while ((NULL != FindQhPtr->Next) && (FindQhPtr->Interval > QhPtr->Interval)) {
        FindQhPtr = FindQhPtr->Next;
      }

      if (FindQhPtr->Interval == QhPtr->Interval) {
        //
        // Link Qh after FindQh
        //
        if (NULL != FindQhPtr->Next) {
          FindQhPtr->Next->Prev         = QhPtr;
          QhPtr->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(FindQhPtr->Next->Qh) >> 5);
          QhPtr->Qh.SelectType          = QH_SELECT_TYPE;
          QhPtr->Qh.QhTerminate         = FALSE;
        }

        FindQhPtr->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
        FindQhPtr->Qh.SelectType          = QH_SELECT_TYPE;
        FindQhPtr->Qh.QhTerminate         = FALSE;

        QhPtr->Prev                       = FindQhPtr;
        QhPtr->Next                       = FindQhPtr->Next;
        FindQhPtr->Next                   = QhPtr;
      } else if (FindQhPtr->Interval < QhPtr->Interval) {
        //
        // Link Qh before FindQh
        //
        if (NULL == FindQhPtr->Prev) {
          //
          // Qh is the First one in Frame[0] List
          //
          FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
          FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
          FrameEntryPtr->LinkTerminate  = FALSE;
        } else {
          //
          // Qh is not the First one in Frame[0] List
          //
          FindQhPtr->Prev->Next                   = QhPtr;
          FindQhPtr->Prev->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
          FindQhPtr->Prev->Qh.SelectType          = QH_SELECT_TYPE;
          FindQhPtr->Prev->Qh.QhTerminate         = FALSE;
        }

        QhPtr->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(FindQhPtr->Qh) >> 5);
        QhPtr->Qh.SelectType          = QH_SELECT_TYPE;
        QhPtr->Qh.QhTerminate         = FALSE;

        QhPtr->Next                   = FindQhPtr;
        QhPtr->Prev                   = FindQhPtr->Prev;
        FindQhPtr->Prev               = QhPtr;
      } else {
        //
        // Link Qh after FindQh, Qh is the Last one
        //
        FindQhPtr->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
        FindQhPtr->Prev->Qh.SelectType    = QH_SELECT_TYPE;
        FindQhPtr->Qh.QhTerminate         = FALSE;

        QhPtr->Prev                       = FindQhPtr;
        QhPtr->Next                       = NULL;
        FindQhPtr->Next                   = QhPtr;
      }
    } else {
      //
      // Null FrameList
      //
      FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
      FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
      FrameEntryPtr->LinkTerminate  = FALSE;
    }
    //
    // Other Frame[X]
    //
    if (NULL == QhPtr->Prev) {
      //
      // Qh is the First one in Frame[0] List
      //
      FrameIndex += QhPtr->Interval;
      while (FrameIndex < HcDev->PeriodicFrameListLength) {
        FrameEntryPtr                 = (FRAME_LIST_ENTRY *) (FrameEntryPtr + QhPtr->Interval);
        FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
        FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
        FrameEntryPtr->LinkTerminate  = FALSE;
        FrameIndex += QhPtr->Interval;
      }
    } else if (QhPtr->Interval < QhPtr->Prev->Interval) {
      //
      // Qh is not the First one in Frame[0] List, and Prev.interval > Qh.interval
      //
      FrameIndex += QhPtr->Interval;
      while (FrameIndex < HcDev->PeriodicFrameListLength) {
        FrameEntryPtr = (FRAME_LIST_ENTRY *) (FrameEntryPtr + QhPtr->Interval);
        if ((FrameIndex % QhPtr->Prev->Interval) != 0) {
          FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
          FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
          FrameEntryPtr->LinkTerminate  = FALSE;
        }

        FrameIndex += QhPtr->Interval;
      }
    }
  } else {

    //
    // SyncInterruptTransfer Qh
    //

    if (!FrameEntryPtr->LinkTerminate) {
      //
      // Not Null FrameList
      //
      FindQhHwPtr = (EHCI_QH_HW *) GET_0B_TO_31B (FrameEntryPtr->LinkPointer << 5);
      FindQhPtr   = (EHCI_QH_ENTITY *) GET_QH_ENTITY_ADDR (FindQhHwPtr);
      //
      // FindQh is Last Qh in the Asynchronous List, Link Qh after FindQh
      //
      while (NULL != FindQhPtr->Next) {
        FindQhPtr = FindQhPtr->Next;
      }

      FindQhPtr->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
      FindQhPtr->Qh.SelectType          = QH_SELECT_TYPE;
      FindQhPtr->Qh.QhTerminate         = FALSE;

      FindQhPtr->Next                   = QhPtr;
      QhPtr->Prev                       = FindQhPtr;
    } else {
      //
      // Null FrameList
      //
      FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Qh) >> 5);
      FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
      FrameEntryPtr->LinkTerminate  = FALSE;
    }
  }

  return ;
}

VOID
UnlinkQhFromPeriodicList (
  IN USB2_HC_DEV        *HcDev,
  IN EHCI_QH_ENTITY     *QhPtr,
  IN UINTN              Interval
  )
/*++

Routine Description:

  Unlink Qh from Periodic Schedule List

Arguments:

  HcDev     - USB2_HC_DEV
  QhPtr     - A pointer to Qh
  Interval  - Interval of this periodic transfer

Returns:

  VOID

--*/
{
  FRAME_LIST_ENTRY  *FrameEntryPtr;
  UINTN             FrameIndex;

  FrameIndex = 0;

  ASSERT (HcDev);
  ASSERT (QhPtr);

  FrameIndex    = 0;
  FrameEntryPtr = (FRAME_LIST_ENTRY *) HcDev->PeriodicFrameListBuffer;

  if (QhPtr->TransferType == ASYNC_INTERRUPT_TRANSFER) {

    //
    // AsyncInterruptTransfer Qh
    //

    if (NULL == QhPtr->Prev) {
      //
      // Qh is the First one on  Frame[0] List
      //
      if (NULL == QhPtr->Next) {
        //
        // Only one on  Frame[0] List
        //
        while (FrameIndex < HcDev->PeriodicFrameListLength) {
          FrameEntryPtr->LinkPointer    = 0;
          FrameEntryPtr->SelectType     = 0;
          FrameEntryPtr->LinkTerminate  = TRUE;
          FrameEntryPtr                += Interval;
          FrameIndex                   += Interval;
        }
      } else {
        while (FrameIndex < HcDev->PeriodicFrameListLength) {
          FrameEntryPtr->LinkPointer    = (UINT32) GET_0B_TO_31B (&(QhPtr->Next->Qh) >> 5);
          FrameEntryPtr->SelectType     = QH_SELECT_TYPE;
          FrameEntryPtr->LinkTerminate  = FALSE;
          FrameEntryPtr += Interval;
          FrameIndex += Interval;
        }
      }
    } else {

      //
      // Not First one on  Frame[0] List
      //
      if (NULL == QhPtr->Next) {
        //
        // Qh is the Last one on  Frame[0] List
        //
        QhPtr->Prev->Qh.QhHorizontalPointer = 0;
        QhPtr->Prev->Qh.SelectType          = 0;
        QhPtr->Prev->Qh.QhTerminate         = TRUE;
      } else {
        QhPtr->Prev->Qh.QhHorizontalPointer = (UINT32) GET_0B_TO_31B (&(QhPtr->Next->Qh) >> 5);
        QhPtr->Prev->Qh.SelectType          = QH_SELECT_TYPE;
        QhPtr->Prev->Qh.QhTerminate         = FALSE;
      }

      if (Interval == QhPtr->Prev->Interval) {
        //
        // Interval is the same as Prev
        // Not involed Frame[X]
        //
      } else {
        //
        // Other Frame[X]
        //
        while (FrameIndex < HcDev->PeriodicFrameListLength) {
          if ((FrameIndex % QhPtr->Prev->Interval) != 0) {
            FrameEntryPtr->LinkPointer    = QhPtr->Prev->Qh.QhHorizontalPointer;
            FrameEntryPtr->SelectType     = QhPtr->Prev->Qh.SelectType;
            FrameEntryPtr->LinkTerminate  = QhPtr->Prev->Qh.QhTerminate;
          }
          FrameEntryPtr += Interval;
          FrameIndex += Interval;
        }
      }
    }

    if (NULL != QhPtr->Next) {
      QhPtr->Next->Prev = QhPtr->Prev;
    }

    if (NULL != QhPtr->Prev) {
      QhPtr->Prev->Next = QhPtr->Next;
    }
  } else {
    //
    // SyncInterruptTransfer Qh
    //
    if (NULL == QhPtr->Prev) {
      //
      // Qh is the only one Qh on  Frame[0] List
      //
      FrameEntryPtr->LinkPointer    = 0;
      FrameEntryPtr->SelectType     = 0;
      FrameEntryPtr->LinkTerminate  = TRUE;
    } else {
      QhPtr->Prev->Qh.QhHorizontalPointer = 0;
      QhPtr->Prev->Qh.SelectType          = 0;
      QhPtr->Prev->Qh.QhTerminate         = TRUE;
    }

    if (NULL != QhPtr->Prev) {
      QhPtr->Prev->Next = NULL;
    }
  }

  return ;
}

VOID
LinkToAsyncReqeust (
  IN  USB2_HC_DEV        *HcDev,
  IN  EHCI_ASYNC_REQUEST *AsyncRequestPtr
  )
/*++

Routine Description:

  Llink AsyncRequest Entry to Async Request List

Arguments:

  HcDev             - USB2_HC_DEV
  AsyncRequestPtr   - A pointer to Async Request Entry

Returns:

  VOID

--*/
{
  EHCI_ASYNC_REQUEST  *CurrentPtr;

  CurrentPtr              = HcDev->AsyncRequestList;
  HcDev->AsyncRequestList = AsyncRequestPtr;
  AsyncRequestPtr->Prev   = NULL;
  AsyncRequestPtr->Next   = CurrentPtr;

  if (NULL != CurrentPtr) {
    CurrentPtr->Prev = AsyncRequestPtr;
  }

  return ;
}

VOID
UnlinkFromAsyncReqeust (
  IN  USB2_HC_DEV        *HcDev,
  IN  EHCI_ASYNC_REQUEST *AsyncRequestPtr
  )
/*++

Routine Description:

  Unlink AsyncRequest Entry from Async Request List

Arguments:

  HcDev            - USB2_HC_DEV
  AsyncRequestPtr  - A pointer to Async Request Entry

Returns:

  VOID

--*/
{
  if (NULL == AsyncRequestPtr->Prev) {
    HcDev->AsyncRequestList = AsyncRequestPtr->Next;
    if (NULL != AsyncRequestPtr->Next) {
      AsyncRequestPtr->Next->Prev = NULL;
    }
  } else {
    AsyncRequestPtr->Prev->Next = AsyncRequestPtr->Next;
    if (NULL != AsyncRequestPtr->Next) {
      AsyncRequestPtr->Next->Prev = AsyncRequestPtr->Prev;
    }
  }

  return ;
}

VOID
SetQtdBufferPointer (
  IN EHCI_QTD_HW  *QtdHwPtr,
  IN VOID         *DataPtr,
  IN UINTN        DataLen
  )
/*++

Routine Description:

  Set data buffer pointers in Qtd

Arguments:

  QtdHwPtr  - A pointer to Qtd hardware structure
  DataPtr   - A pointer to user data buffer
  DataLen   - Length of the user data buffer

Returns:

  VOID

--*/
{
  UINTN RemainLen;

  ASSERT (QtdHwPtr);
  ASSERT (DataLen <= 5 * EFI_PAGE_SIZE);

  RemainLen = DataLen;
  //
  // Allow buffer address range across 4G.
  // But EFI_USB_MAX_BULK_BUFFER_NUM = 1, so don't allow
  // seperate buffer array.
  //
  //
  // Set BufferPointer0, ExtBufferPointer0 and Offset
  //
  QtdHwPtr->BufferPointer0    = (UINT32) (GET_0B_TO_31B (DataPtr) >> EFI_PAGE_SHIFT);
  QtdHwPtr->CurrentOffset     = (UINT32) (GET_0B_TO_31B (DataPtr) & EFI_PAGE_MASK);

  //
  // Set BufferPointer1 and ExtBufferPointer1
  //
  RemainLen = RemainLen > (EFI_PAGE_SIZE - QtdHwPtr->CurrentOffset) ? (RemainLen - (EFI_PAGE_SIZE - QtdHwPtr->CurrentOffset)) : 0;
  if (RemainLen == 0) {
    goto exit;
  }

  QtdHwPtr->BufferPointer1    = QtdHwPtr->BufferPointer0 + 1;

  //
  // Set BufferPointer2 and ExtBufferPointer2
  //
  RemainLen = RemainLen > EFI_PAGE_SIZE ? (RemainLen - EFI_PAGE_SIZE) : 0;
  if (RemainLen == 0) {
    goto exit;
  }

  QtdHwPtr->BufferPointer2    = QtdHwPtr->BufferPointer1 + 1;

  //
  // Set BufferPointer3 and ExtBufferPointer3
  //
  RemainLen = RemainLen > EFI_PAGE_SIZE ? (RemainLen - EFI_PAGE_SIZE) : 0;
  if (RemainLen == 0) {
    goto exit;
  }

  QtdHwPtr->BufferPointer3    = QtdHwPtr->BufferPointer2 + 1;

  //
  // Set BufferPointer4 and ExtBufferPointer4
  //
  RemainLen = RemainLen > EFI_PAGE_SIZE ? (RemainLen - EFI_PAGE_SIZE) : 0;
  if (RemainLen == 0) {
    goto exit;
  }

  QtdHwPtr->BufferPointer4    = QtdHwPtr->BufferPointer3 + 1;

exit:
  return ;
}

BOOLEAN
IsQtdStatusActive (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is active or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Active
  FALSE   Inactive

--*/
{
  UINT8   QtdStatus;
  BOOLEAN Value;

  QtdStatus = (UINT8) (HwQtdPtr->Status);
  Value     = (BOOLEAN) (QtdStatus & QTD_STATUS_ACTIVE);
  //DEBUG ((gEHCErrorLevel, "EHCI: IsQtdStatusActive 0x%X, Address = %x\r\n",HwQtdPtr->Status, HwQtdPtr));

  return Value;
}

BOOLEAN
IsQtdStatusHalted (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is halted or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Halted
  FALSE   Not halted

--*/
{
  UINT8   QtdStatus;
  BOOLEAN Value;

  QtdStatus = (UINT8) (HwQtdPtr->Status);
  Value     = (BOOLEAN) (QtdStatus & QTD_STATUS_HALTED);

  return Value;
}

BOOLEAN
IsQtdStatusBufferError (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is buffer error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Buffer error
  FALSE   No buffer error

--*/
{
  UINT8   QtdStatus;
  BOOLEAN Value;

  QtdStatus = (UINT8) (HwQtdPtr->Status);
  Value     = (BOOLEAN) (QtdStatus & QTD_STATUS_BUFFER_ERR);

  return Value;
}

BOOLEAN
IsQtdStatusBabbleError (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is babble error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Babble error
  FALSE   No babble error

--*/
{
  UINT8   QtdStatus;
  BOOLEAN Value;

  QtdStatus = (UINT8) (HwQtdPtr->Status);
  Value     = (BOOLEAN) (QtdStatus & QTD_STATUS_BABBLE_ERR);

  return Value;
}

BOOLEAN
IsQtdStatusTransactionError (
  IN EHCI_QTD_HW  *HwQtdPtr
  )
/*++

Routine Description:

  Whether Qtd status is transaction error or not

Arguments:

  HwQtdPtr - A pointer to hardware Qtd structure

Returns:

  TRUE    Transaction error
  FALSE   No transaction error

--*/
{
  UINT8   QtdStatus;
  BOOLEAN Value;

  QtdStatus = (UINT8) (HwQtdPtr->Status);
  Value     = (BOOLEAN) (QtdStatus & QTD_STATUS_TRANSACTION_ERR);

  return Value;
}

BOOLEAN
IsDataInTransfer (
  IN UINT8     EndPointAddress
  )
/*++

Routine Description:

  Whether is a DataIn direction transfer

Arguments:

  EndPointAddress - address of the endpoint

Returns:

  TRUE    DataIn
  FALSE   DataOut

--*/
{
  BOOLEAN Value;

  if (EndPointAddress & 0x80) {
    Value = TRUE;
  } else {
    Value = FALSE;
  }

  return Value;
}

EFI_STATUS
MapDataBuffer (
  IN  USB2_HC_DEV             *HcDev,
  IN  EFI_USB_DATA_DIRECTION  TransferDirection,
  IN  VOID                    *Data,
  IN  OUT UINTN               *DataLength,
  OUT UINT8                   *PktId,
  OUT UINT8                   **DataCursor,
  OUT VOID                    **DataMap
  )
/*++

Routine Description:

  Map address of user data buffer

Arguments:

  HcDev              - USB2_HC_DEV
  TransferDirection  - direction of transfer
  Data               - A pointer to user data buffer
  DataLength         - length of user data
  PktId              - Packte Identificaion
  DataCursor         - mapped address to return
  DataMap            - identificaion of this mapping to return

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  TempPhysicalAddr;

  Status = EFI_SUCCESS;

  switch (TransferDirection) {

  case EfiUsbDataIn:

    *PktId = INPUT_PACKET_ID;
    //
    // BusMasterWrite means cpu read
    //
    Status = HcDev->PciIo->Map (
                            HcDev->PciIo,
                            EfiPciIoOperationBusMasterWrite,
                            Data,
                            DataLength,
                            &TempPhysicalAddr,
                            DataMap
                            );
    if (EFI_ERROR (Status)) {
      DEBUG ((gEHCDebugLevel, "EHCI: MapDataBuffer Failed\n"));
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    *DataCursor = (UINT8 *) ((UINTN) TempPhysicalAddr);
    break;

  case EfiUsbDataOut:

    *PktId = OUTPUT_PACKET_ID;
    //
    // BusMasterRead means cpu write
    //
    Status = HcDev->PciIo->Map (
                            HcDev->PciIo,
                            EfiPciIoOperationBusMasterRead,
                            Data,
                            DataLength,
                            &TempPhysicalAddr,
                            DataMap
                            );
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    *DataCursor = (UINT8 *) ((UINTN) TempPhysicalAddr);
    break;

  case EfiUsbNoData:

    *PktId      = OUTPUT_PACKET_ID;
    Data        = NULL;
    *DataLength = 0;
    *DataCursor = NULL;
    *DataMap    = NULL;
    break;

  default:

    Status = EFI_INVALID_PARAMETER;
  }

exit:
  return Status;
}

EFI_STATUS
MapRequestBuffer (
  IN  USB2_HC_DEV             *HcDev,
  IN  OUT VOID                *Request,
  OUT UINT8                   **RequestCursor,
  OUT VOID                    **RequestMap
  )
/*++

Routine Description:

  Map address of request structure buffer

Arguments:

  HcDev           - USB2_HC_DEV
  Request         - A pointer to request structure
  RequestCursor   - Mapped address of request structure to return
  RequestMap      - Identificaion of this mapping to return

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS            Status;
  UINTN                 RequestLen;
  EFI_PHYSICAL_ADDRESS  TempPhysicalAddr;

  RequestLen = sizeof (EFI_USB_DEVICE_REQUEST);
  Status = HcDev->PciIo->Map (
                           HcDev->PciIo,
                           EfiPciIoOperationBusMasterRead,
                           (UINT8 *) Request,
                           (UINTN *) &RequestLen,
                           &TempPhysicalAddr,
                           RequestMap
                           );
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  *RequestCursor = (UINT8 *) ((UINTN) TempPhysicalAddr);

exit:
  return Status;
}

EFI_STATUS
DeleteAsyncRequestTransfer (
  IN  USB2_HC_DEV     *HcDev,
  IN  UINT8           DeviceAddress,
  IN  UINT8           EndPointAddress,
  OUT UINT8           *DataToggle
  )
/*++

Routine Description:

  Delete all asynchronous request transfer

Arguments:

  HcDev           - USB2_HC_DEV
  DeviceAddress   - address of usb device
  EndPointAddress - address of endpoint
  DataToggle      - stored data toggle

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS          Status;
  EHCI_ASYNC_REQUEST  *AsyncRequestPtr;
  EHCI_ASYNC_REQUEST  *MatchPtr;
  EHCI_QH_HW          *QhHwPtr;
  UINT8               EndPointNum;

  if (NULL == HcDev->AsyncRequestList) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  MatchPtr        = NULL;
  QhHwPtr         = NULL;
  EndPointNum     = (UINT8) (EndPointAddress & 0x0f);
  AsyncRequestPtr = HcDev->AsyncRequestList;

  //
  // Find QH of AsyncRequest by DeviceAddress and EndPointNum
  //
  do {

    QhHwPtr = &(AsyncRequestPtr->QhPtr->Qh);
    if (QhHwPtr->DeviceAddr == DeviceAddress && QhHwPtr->EndpointNum == EndPointNum) {
      MatchPtr = AsyncRequestPtr;
      break;
    }

    AsyncRequestPtr = AsyncRequestPtr->Next;

  } while (NULL != AsyncRequestPtr);

  if (NULL == MatchPtr) {
    Status = EFI_INVALID_PARAMETER;
    goto exit;
  }

  Status = DisablePeriodicSchedule (HcDev);
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto exit;
  }

  Status = WaitForPeriodicScheduleDisable (HcDev, EHCI_GENERIC_TIMEOUT);
  if (EFI_ERROR (Status)) {
    DEBUG ((gEHCErrorLevel, "EHCI: WaitForPeriodicScheduleDisable TimeOut\n"));
    Status = EFI_TIMEOUT;
    goto exit;
  }

  *DataToggle = (UINT8) MatchPtr->QhPtr->Qh.DataToggle;
  UnlinkQhFromPeriodicList (HcDev, MatchPtr->QhPtr, MatchPtr->QhPtr->Interval);
  UnlinkFromAsyncReqeust (HcDev, MatchPtr);

  if (NULL == HcDev->AsyncRequestList) {

    Status = StopPollingTimer (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

  } else {

    Status = EnablePeriodicSchedule (HcDev);
    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto exit;
    }

    Status = WaitForPeriodicScheduleEnable (HcDev, EHCI_GENERIC_TIMEOUT);
    if (EFI_ERROR (Status)) {
      DEBUG ((gEHCErrorLevel, "EHCI: WaitForPeriodicScheduleEnable TimeOut\n"));
      Status = EFI_TIMEOUT;
      goto exit;
    }

    if (IsEhcHalted (HcDev)) {
      Status = StartScheduleExecution (HcDev);
      if (EFI_ERROR (Status)) {
        Status = EFI_DEVICE_ERROR;
        goto exit;
      }
    }

  }

  DestoryQtds (HcDev, MatchPtr->QhPtr->FirstQtdPtr);
  DestoryQh (HcDev, MatchPtr->QhPtr);
  EhciFreePool (HcDev, (UINT8 *) MatchPtr, sizeof (EHCI_ASYNC_REQUEST));

exit:
  return Status;
}

VOID
CleanUpAllAsyncRequestTransfer (
  IN USB2_HC_DEV  *HcDev
  )
/*++

Routine Description:

  Clean up all asynchronous request transfer

Arguments:

  HcDev - USB2_HC_DEV

Returns:

  VOID

--*/
{
  EHCI_ASYNC_REQUEST  *AsyncRequestPtr;
  EHCI_ASYNC_REQUEST  *FreePtr;

  AsyncRequestPtr = NULL;
  FreePtr         = NULL;

  StopPollingTimer (HcDev);

  AsyncRequestPtr = HcDev->AsyncRequestList;
  while (NULL != AsyncRequestPtr) {

    FreePtr         = AsyncRequestPtr;
    AsyncRequestPtr = AsyncRequestPtr->Next;
    UnlinkFromAsyncReqeust (HcDev, FreePtr);
    UnlinkQhFromPeriodicList (HcDev, FreePtr->QhPtr, FreePtr->QhPtr->Interval);
    DestoryQtds (HcDev, FreePtr->QhPtr->FirstQtdPtr);
    DestoryQh (HcDev, FreePtr->QhPtr);
    EhciFreePool (HcDev, (UINT8 *) FreePtr, sizeof (EHCI_ASYNC_REQUEST));

  }

  return ;
}

VOID
ZeroOutQhOverlay (
  IN EHCI_QH_ENTITY  *QhPtr
  )
/*++

Routine Description:

  Zero out the fields in Qh structure

Arguments:

  QhPtr - A pointer to Qh structure

Returns:

  VOID

--*/
{
  QhPtr->Qh.CurrentQtdPointer   = 0;
  QhPtr->Qh.AltNextQtdPointer   = 0;
  QhPtr->Qh.NakCount            = 0;
  QhPtr->Qh.AltNextQtdTerminate = 0;
  QhPtr->Qh.TotalBytes          = 0;
  QhPtr->Qh.InterruptOnComplete = 0;
  QhPtr->Qh.CurrentPage         = 0;
  QhPtr->Qh.ErrorCount          = 0;
  QhPtr->Qh.PidCode             = 0;
  QhPtr->Qh.Status              = 0;
  QhPtr->Qh.BufferPointer0      = 0;
  QhPtr->Qh.CurrentOffset       = 0;
  QhPtr->Qh.BufferPointer1      = 0;
  QhPtr->Qh.CompleteSplitMask   = 0;
  QhPtr->Qh.BufferPointer2      = 0;
  QhPtr->Qh.SplitBytes          = 0;
  QhPtr->Qh.FrameTag            = 0;
  QhPtr->Qh.BufferPointer3      = 0;
  QhPtr->Qh.BufferPointer4      = 0;
}

VOID
UpdateAsyncRequestTransfer (
  IN EHCI_ASYNC_REQUEST *AsyncRequestPtr,
  IN UINT32             TransferResult,
  IN UINTN              ErrQtdPos
  )
/*++

Routine Description:

  Update asynchronous request transfer

Arguments:

  AsyncRequestPtr  - A pointer to async request
  TransferResult   - transfer result
  ErrQtdPos        - postion of error Qtd

Returns:

  VOID

--*/
{
  EHCI_QTD_ENTITY *QtdPtr;

  QtdPtr      = NULL;

  if (EFI_USB_NOERROR == TransferResult) {

    //
    // Update Qh for next trigger
    //

    QtdPtr = AsyncRequestPtr->QhPtr->FirstQtdPtr;

    //
    // Update fields in Qh
    //

    //
    // Get DataToggle from Overlay in Qh
    //
    // ZeroOut Overlay in Qh except DataToggle, HostController will update this field
    //
    ZeroOutQhOverlay (AsyncRequestPtr->QhPtr);
    AsyncRequestPtr->QhPtr->Qh.NextQtdPointer   = (UINT32) (GET_0B_TO_31B (&(QtdPtr->Qtd)) >> 5);
    AsyncRequestPtr->QhPtr->Qh.NextQtdTerminate = FALSE;

    //
    // Update fields in Qtd
    //
    while (NULL != QtdPtr) {
      QtdPtr->Qtd.TotalBytes    = QtdPtr->StaticTotalBytes;
      QtdPtr->Qtd.CurrentOffset = QtdPtr->StaticCurrentOffset;
      QtdPtr->Qtd.CurrentPage   = 0;
      QtdPtr->Qtd.ErrorCount    = QTD_ERROR_COUNTER;
      QtdPtr->Qtd.Status        = QTD_STATUS_ACTIVE;

      QtdPtr->TotalBytes        = QtdPtr->StaticTotalBytes;
      QtdPtr                    = QtdPtr->Next;
    }
  }

  return ;
}

BOOLEAN
CheckQtdsTransferResult (
  IN  BOOLEAN            IsControl,
  IN  EHCI_QH_ENTITY     *QhPtr,
  OUT UINT32             *Result,
  OUT UINTN              *ErrQtdPos,
  OUT UINTN              *ActualLen
  )
/*++

Routine Description:

  Check transfer result of Qtds

Arguments:

  IsControl    - Is control transfer or not
  QhPtr        - A pointer to Qh
  Result       - Transfer result
  ErrQtdPos    - Error TD Position
  ActualLen    - Actual Transfer Size

Returns:

  TRUE    Qtds finished
  FALSE   Not finish

--*/
{
  UINTN           ActualLenPerQtd;
  EHCI_QTD_ENTITY *QtdPtr;
  EHCI_QTD_HW     *QtdHwPtr;
  BOOLEAN         Value;

  ASSERT (QhPtr);
  ASSERT (Result);
  ASSERT (ErrQtdPos);
  ASSERT (ActualLen);

  Value     = TRUE;
  QtdPtr    = QhPtr->FirstQtdPtr;
  QtdHwPtr  = &(QtdPtr->Qtd);

  while (NULL != QtdHwPtr) {
    if (IsQtdStatusActive (QtdHwPtr)) {
      *Result |= EFI_USB_ERR_NOTEXECUTE;
    }

    if (IsQtdStatusHalted (QtdHwPtr)) {

      DEBUG ((gEHCErrorLevel, "EHCI: QTD_STATUS_HALTED 0x%X\n", QtdHwPtr->Status));
      *Result |= EFI_USB_ERR_STALL;
    }

    if (IsQtdStatusBufferError (QtdHwPtr)) {
      DEBUG ((gEHCErrorLevel, "EHCI: QTD_STATUS_BUFFER_ERR 0x%X\n", QtdHwPtr->Status));
      *Result |= EFI_USB_ERR_BUFFER;
    }

    if (IsQtdStatusBabbleError (QtdHwPtr)) {
      DEBUG ((gEHCErrorLevel, "EHCI: StatusBufferError 0x%X\n", QtdHwPtr->Status));
      *Result |= EFI_USB_ERR_BABBLE;
    }

    if (IsQtdStatusTransactionError (QtdHwPtr)) {

      //
      //Exclude Special Case
      //
      if (((QtdHwPtr->Status & QTD_STATUS_HALTED) == QTD_STATUS_HALTED) ||
          ((QtdHwPtr->Status & QTD_STATUS_ACTIVE) == QTD_STATUS_ACTIVE) ||
          ((QtdHwPtr->ErrorCount != QTD_ERROR_COUNTER))) {
        *Result |= EFI_USB_ERR_TIMEOUT;
        DEBUG ((gEHCErrorLevel, "EHCI: QTD_STATUS_TRANSACTION_ERR: 0x%X\n", QtdHwPtr->Status));
      }
    }

    ActualLenPerQtd     = QtdPtr->TotalBytes - QtdHwPtr->TotalBytes;
    QtdPtr->TotalBytes  = QtdHwPtr->TotalBytes;
    //
    // Accumulate actual transferred data length in each DataQtd.
    //
    if (SETUP_PACKET_PID_CODE != QtdHwPtr->PidCode) {
      *ActualLen += ActualLenPerQtd;
    }

    if (*Result) {
      Value = FALSE;
      break;
    }

    if ((INPUT_PACKET_PID_CODE == QtdHwPtr->PidCode)&& (QtdPtr->TotalBytes > 0)) {
      //
      // Short Packet: IN, Short
      //
      DEBUG ((gEHCDebugLevel, "EHCI: Short Packet Status: 0x%x\n", QtdHwPtr->Status));
      break;
    }

    if (QtdPtr->Next != NULL) {
      (*ErrQtdPos)++;
      QtdPtr   = QtdPtr->Next;
      QtdHwPtr = &(QtdPtr->Qtd);
    } else {
      QtdHwPtr = NULL;
    }

  }

  return Value;
}

EFI_STATUS
ExecuteTransfer (
  IN  USB2_HC_DEV         *HcDev,
  IN  BOOLEAN             IsControl,
  IN  EHCI_QH_ENTITY      *QhPtr,
  IN  OUT UINTN           *ActualLen,
  OUT UINT8               *DataToggle,
  IN  UINTN               TimeOut,
  OUT UINT32              *TransferResult
  )
/*++

Routine Description:

  Execute Bulk or SyncInterrupt Transfer

Arguments:

  HcDev            - USB2_HC_DEV
  IsControl        - Is control transfer or not
  QhPtr            - A pointer to Qh
  ActualLen        - Actual transfered Len
  DataToggle       - Data Toggle
  TimeOut          - TimeOut threshold
  TransferResult   - Transfer result

Returns:

  EFI_SUCCESS      Sucess
  EFI_DEVICE_ERROR Fail

--*/
{
  EFI_STATUS  Status;
  UINTN       ErrQtdPos;
  UINTN       Delay;
  BOOLEAN     Finished;

  Status          = EFI_SUCCESS;
  ErrQtdPos       = 0;
  *TransferResult = EFI_USB_NOERROR;
  *ActualLen      = 0;
  Finished        = FALSE;

  Delay           = (TimeOut * STALL_1_MILLI_SECOND / 50);

  do {
    *TransferResult = 0;
    Finished = CheckQtdsTransferResult (
                 IsControl,
                 QhPtr,
                 TransferResult,
                 &ErrQtdPos,
                 ActualLen
                 );
    if (Finished) {
      break;
    }
    //
    // Qtd is inactive, which means bulk or interrupt transfer's end.
    //
    if (!(*TransferResult & EFI_USB_ERR_NOTEXECUTE)) {
      break;
    }

    gBS->Stall (EHCI_SYNC_REQUEST_POLLING_TIME);

  } while (--Delay);

  if (EFI_USB_NOERROR != *TransferResult) {
    if (0 == Delay) {
      DEBUG((gEHCErrorLevel, "EHCI: QTDS TimeOut\n"));
      Status = EFI_TIMEOUT;
    } else {
      Status = EFI_DEVICE_ERROR;
    }
  }

  //
  // Special for Bulk and Interrupt Transfer
  //
  *DataToggle = (UINT8) QhPtr->Qh.DataToggle;

  return Status;
}

EFI_STATUS
AsyncRequestMoniter (
  IN EFI_EVENT     Event,
  IN VOID          *Context
  )
/*++
Routine Description:

  Interrupt transfer periodic check handler

Arguments:
  Event    - Interrupt event
  Context  - Pointer to USB2_HC_DEV

Returns:

  EFI_SUCCESS        Success
  EFI_DEVICE_ERROR   Fail

--*/
{
  EFI_STATUS          Status;
  USB2_HC_DEV         *HcDev;
  EHCI_ASYNC_REQUEST  *AsyncRequestPtr;
  EHCI_ASYNC_REQUEST  *NextPtr;
  EHCI_QTD_HW         *QtdHwPtr;
  UINTN               ErrQtdPos;
  UINTN               ActualLen;
  UINT32              TransferResult;
  UINT8               *ReceiveBuffer;
  UINT8               *ProcessBuffer;

  Status          = EFI_SUCCESS;
  QtdHwPtr        = NULL;
  ReceiveBuffer   = NULL;
  ProcessBuffer   = NULL;
  HcDev           = (USB2_HC_DEV *) Context;
  AsyncRequestPtr = HcDev->AsyncRequestList;

  while (NULL != AsyncRequestPtr) {

    TransferResult  = 0;
    ErrQtdPos       = 0;
    ActualLen       = 0;

    CheckQtdsTransferResult (
      FALSE,
      AsyncRequestPtr->QhPtr,
      &TransferResult,
      &ErrQtdPos,
      &ActualLen
      );

    if ((TransferResult & EFI_USB_ERR_NAK) || (TransferResult & EFI_USB_ERR_NOTEXECUTE)) {
      AsyncRequestPtr = AsyncRequestPtr->Next;
      continue;
    }
    //
    // Allocate memory for EHC private data structure
    //
    ProcessBuffer = AllocateZeroPool (ActualLen);
    if (NULL == ProcessBuffer) {
      Status = EFI_OUT_OF_RESOURCES;
      goto exit;
    }

    QtdHwPtr = &(AsyncRequestPtr->QhPtr->FirstQtdPtr->Qtd);
    ReceiveBuffer = (UINT8 *) GET_0B_TO_31B ((QtdHwPtr->BufferPointer0 << EFI_PAGE_SHIFT) | AsyncRequestPtr->QhPtr->FirstQtdPtr->StaticCurrentOffset);
    CopyMem (
      ProcessBuffer,
      ReceiveBuffer,
      ActualLen
      );

    UpdateAsyncRequestTransfer (AsyncRequestPtr, TransferResult, ErrQtdPos);

    NextPtr = AsyncRequestPtr->Next;

    if (EFI_USB_NOERROR == TransferResult) {

      if (AsyncRequestPtr->CallBackFunc != NULL) {
        (AsyncRequestPtr->CallBackFunc) (ProcessBuffer, ActualLen, AsyncRequestPtr->Context, TransferResult);
      }

    } else {

      //
      // leave error recovery to its related device driver. A common case of
      // the error recovery is to re-submit the interrupt transfer.
      // When an interrupt transfer is re-submitted, its position in the linked
      // list is changed. It is inserted to the head of the linked list, while
      // this function scans the whole list from head to tail. Thus, the
      // re-submitted interrupt transfer's callback function will not be called
      // again in this round.
      //
      if (AsyncRequestPtr->CallBackFunc != NULL) {
        (AsyncRequestPtr->CallBackFunc) (NULL, 0, AsyncRequestPtr->Context, TransferResult);
      }

    }

    if (NULL != ProcessBuffer) {
      gBS->FreePool (ProcessBuffer);
    }

    AsyncRequestPtr = NextPtr;
  }

exit:
  return Status;
}

