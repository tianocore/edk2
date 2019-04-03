/** @file
This file contains the implementation of Usb Hc Protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "OhcPeim.h"

/**
  Submits control transfer to a target USB device.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress          The target device address.
  @param  DeviceSpeed            Target device speed.
  @param  MaximumPacketLength    Maximum packet size the default control transfer
                                 endpoint is capable of sending or receiving.
  @param  Request                USB device request to send.
  @param  TransferDirection      Specifies the data direction for the data stage.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.
  @param  TimeOut                Indicates the maximum timeout, in millisecond.
  @param  TransferResult         Return the result of this control transfer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
OhciControlTransfer (
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                        DeviceAddress,
  IN  UINT8                        DeviceSpeed,
  IN  UINT8                        MaxPacketLength,
  IN  EFI_USB_DEVICE_REQUEST       *Request,
  IN  EFI_USB_DATA_DIRECTION       TransferDirection,
  IN  OUT VOID                     *Data,
  IN  OUT UINTN                    *DataLength,
  IN  UINTN                        TimeOut,
  OUT UINT32                       *TransferResult
  )
{
  USB_OHCI_HC_DEV               *Ohc;
  ED_DESCRIPTOR                 *Ed;
  TD_DESCRIPTOR                 *HeadTd;
  TD_DESCRIPTOR                 *SetupTd;
  TD_DESCRIPTOR                 *DataTd;
  TD_DESCRIPTOR                 *StatusTd;
  TD_DESCRIPTOR                 *EmptyTd;
  EFI_STATUS                    Status;
  UINT32                        DataPidDir;
  UINT32                        StatusPidDir;
  UINTN                         TimeCount;
  UINT32                        ErrorCode;

  UINTN                         ActualSendLength;
  UINTN                         LeftLength;
  UINT8                         DataToggle;

  EFI_PHYSICAL_ADDRESS          ReqMapPhyAddr = 0;

  UINTN                         DataMapLength = 0;
  EFI_PHYSICAL_ADDRESS          DataMapPhyAddr = 0;

  HeadTd = NULL;
  DataTd = NULL;

  if ((TransferDirection != EfiUsbDataOut && TransferDirection != EfiUsbDataIn &&
       TransferDirection != EfiUsbNoData) ||
      Request == NULL || DataLength == NULL || TransferResult == NULL ||
      (TransferDirection == EfiUsbNoData && (*DataLength != 0 || Data != NULL)) ||
      (TransferDirection != EfiUsbNoData && (*DataLength == 0 || Data == NULL)) ||
      (DeviceSpeed != EFI_USB_SPEED_LOW && DeviceSpeed != EFI_USB_SPEED_FULL) ||
      (MaxPacketLength != 8 && MaxPacketLength != 16 &&
       MaxPacketLength != 32 && MaxPacketLength != 64)) {
    DEBUG ((EFI_D_INFO, "OhciControlTransfer: EFI_INVALID_PARAMETER\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (*DataLength > MAX_BYTES_PER_TD) {
    DEBUG ((EFI_D_ERROR, "OhciControlTransfer: Request data size is too large\n"));
    return EFI_INVALID_PARAMETER;
  }

  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS(This);

  if (TransferDirection == EfiUsbDataIn) {
    DataPidDir = TD_IN_PID;
    StatusPidDir = TD_OUT_PID;
  } else {
    DataPidDir = TD_OUT_PID;
    StatusPidDir = TD_IN_PID;
  }

  OhciSetHcControl (Ohc, CONTROL_ENABLE, 0);
  if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 0) {
    MicroSecondDelay (HC_1_MILLISECOND);
    if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 0) {
      *TransferResult = EFI_USB_ERR_SYSTEM;
      DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to disable CONTROL transfer\n"));
      return EFI_DEVICE_ERROR;
    }
  }
  OhciSetMemoryPointer (Ohc, HC_CONTROL_HEAD, NULL);
  Ed = OhciCreateED (Ohc);
  if (Ed == NULL) {
    DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to allocate ED buffer\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  OhciSetEDField (Ed, ED_SKIP, 1);
  OhciSetEDField (Ed, ED_FUNC_ADD, DeviceAddress);
  OhciSetEDField (Ed, ED_ENDPT_NUM, 0);
  OhciSetEDField (Ed, ED_DIR, ED_FROM_TD_DIR);
  OhciSetEDField (Ed, ED_SPEED, DeviceSpeed);
  OhciSetEDField (Ed, ED_FORMAT | ED_HALTED | ED_DTTOGGLE, 0);
  OhciSetEDField (Ed, ED_MAX_PACKET, MaxPacketLength);
  OhciSetEDField (Ed, ED_PDATA, 0);
  OhciSetEDField (Ed, ED_ZERO, 0);
  OhciSetEDField (Ed, ED_TDHEAD_PTR, (UINT32) NULL);
  OhciSetEDField (Ed, ED_TDTAIL_PTR, (UINT32) NULL);
  OhciSetEDField (Ed, ED_NEXT_EDPTR, (UINT32) NULL);
  OhciAttachEDToList (Ohc, CONTROL_LIST, Ed, NULL);
  //
  // Setup Stage
  //
  if(Request != NULL) {
    ReqMapPhyAddr = (EFI_PHYSICAL_ADDRESS)(UINTN)Request;
  }
  SetupTd = OhciCreateTD (Ohc);
  if (SetupTd == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to allocate Setup TD buffer\n"));
    goto FREE_ED_BUFF;
  }
  HeadTd = SetupTd;
  OhciSetTDField (SetupTd, TD_PDATA, 0);
  OhciSetTDField (SetupTd, TD_BUFFER_ROUND, 1);
  OhciSetTDField (SetupTd, TD_DIR_PID, TD_SETUP_PID);
  OhciSetTDField (SetupTd, TD_DELAY_INT, TD_NO_DELAY);
  OhciSetTDField (SetupTd, TD_DT_TOGGLE, 2);
  OhciSetTDField (SetupTd, TD_ERROR_CNT, 0);
  OhciSetTDField (SetupTd, TD_COND_CODE, TD_TOBE_PROCESSED);
  OhciSetTDField (SetupTd, TD_CURR_BUFFER_PTR, (UINTN)ReqMapPhyAddr);
  OhciSetTDField (SetupTd, TD_NEXT_PTR, (UINT32) NULL);
  OhciSetTDField (SetupTd, TD_BUFFER_END_PTR, (UINTN)ReqMapPhyAddr + sizeof (EFI_USB_DEVICE_REQUEST) - 1);
  SetupTd->ActualSendLength = 0;
  SetupTd->DataBuffer = NULL;
  SetupTd->NextTDPointer = NULL;

  DataMapLength = *DataLength;
  if ((Data != NULL) && (DataMapLength != 0)) {
    DataMapPhyAddr = (EFI_PHYSICAL_ADDRESS)(UINTN)Data;
  }
  //
  //Data Stage
  //
  LeftLength = DataMapLength;
  ActualSendLength = DataMapLength;
  DataToggle = 1;
  while (LeftLength > 0) {
    ActualSendLength = LeftLength;
    if (LeftLength > MaxPacketLength) {
      ActualSendLength = MaxPacketLength;
    }
    DataTd = OhciCreateTD (Ohc);
    if (DataTd == NULL) {
      DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to allocate Data TD buffer\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto FREE_TD_BUFF;
    }
    OhciSetTDField (DataTd, TD_PDATA, 0);
    OhciSetTDField (DataTd, TD_BUFFER_ROUND, 1);
    OhciSetTDField (DataTd, TD_DIR_PID, DataPidDir);
    OhciSetTDField (DataTd, TD_DELAY_INT, TD_NO_DELAY);
    OhciSetTDField (DataTd, TD_DT_TOGGLE, DataToggle);
    OhciSetTDField (DataTd, TD_ERROR_CNT, 0);
    OhciSetTDField (DataTd, TD_COND_CODE, TD_TOBE_PROCESSED);
    OhciSetTDField (DataTd, TD_CURR_BUFFER_PTR, (UINT32) DataMapPhyAddr);
    OhciSetTDField (DataTd, TD_BUFFER_END_PTR, (UINT32) DataMapPhyAddr + ActualSendLength - 1);
    OhciSetTDField (DataTd, TD_NEXT_PTR, (UINT32) NULL);
    DataTd->ActualSendLength = ActualSendLength;
    DataTd->DataBuffer = (UINT8 *)(UINTN)DataMapPhyAddr;
    DataTd->NextTDPointer = 0;
    OhciLinkTD (HeadTd, DataTd);
    DataToggle ^= 1;
    DataMapPhyAddr += ActualSendLength;
    LeftLength -= ActualSendLength;
  }
  //
  // Status Stage
  //
  StatusTd = OhciCreateTD (Ohc);
  if (StatusTd == NULL) {
    DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to allocate Status TD buffer\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_TD_BUFF;
  }
  OhciSetTDField (StatusTd, TD_PDATA, 0);
  OhciSetTDField (StatusTd, TD_BUFFER_ROUND, 1);
  OhciSetTDField (StatusTd, TD_DIR_PID, StatusPidDir);
  OhciSetTDField (StatusTd, TD_DELAY_INT, 7);
  OhciSetTDField (StatusTd, TD_DT_TOGGLE, 3);
  OhciSetTDField (StatusTd, TD_ERROR_CNT, 0);
  OhciSetTDField (StatusTd, TD_COND_CODE, TD_TOBE_PROCESSED);
  OhciSetTDField (StatusTd, TD_CURR_BUFFER_PTR, (UINT32) NULL);
  OhciSetTDField (StatusTd, TD_NEXT_PTR, (UINT32) NULL);
  OhciSetTDField (StatusTd, TD_BUFFER_END_PTR, (UINT32) NULL);
  StatusTd->ActualSendLength = 0;
  StatusTd->DataBuffer = NULL;
  StatusTd->NextTDPointer = NULL;
  OhciLinkTD (HeadTd, StatusTd);
  //
  // Empty Stage
  //
  EmptyTd = OhciCreateTD (Ohc);
  if (EmptyTd == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to allocate Empty TD buffer\n"));
    goto FREE_TD_BUFF;
  }
  OhciSetTDField (EmptyTd, TD_PDATA, 0);
  OhciSetTDField (EmptyTd, TD_BUFFER_ROUND, 0);
  OhciSetTDField (EmptyTd, TD_DIR_PID, 0);
  OhciSetTDField (EmptyTd, TD_DELAY_INT, 0);
  //OhciSetTDField (EmptyTd, TD_DT_TOGGLE, CurrentToggle);
  EmptyTd->Word0.DataToggle = 0;
  OhciSetTDField (EmptyTd, TD_ERROR_CNT, 0);
  OhciSetTDField (EmptyTd, TD_COND_CODE, 0);
  OhciSetTDField (EmptyTd, TD_CURR_BUFFER_PTR, 0);
  OhciSetTDField (EmptyTd, TD_BUFFER_END_PTR, 0);
  OhciSetTDField (EmptyTd, TD_NEXT_PTR, 0);
  EmptyTd->ActualSendLength = 0;
  EmptyTd->DataBuffer = NULL;
  EmptyTd->NextTDPointer = NULL;
  OhciLinkTD (HeadTd, EmptyTd);
  Ed->TdTailPointer = EmptyTd;
  OhciAttachTDListToED (Ed, HeadTd);
  //
  OhciSetEDField (Ed, ED_SKIP, 0);
  MicroSecondDelay (20 * HC_1_MILLISECOND);
  OhciSetHcCommandStatus (Ohc, CONTROL_LIST_FILLED, 1);
  OhciSetHcControl (Ohc, CONTROL_ENABLE, 1);
  MicroSecondDelay (20 * HC_1_MILLISECOND);
  if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 1) {
  MicroSecondDelay (HC_1_MILLISECOND);
    if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 1) {
      *TransferResult = EFI_USB_ERR_SYSTEM;
      Status = EFI_DEVICE_ERROR;
      DEBUG ((EFI_D_INFO, "OhciControlTransfer: Fail to enable CONTROL transfer\n"));
      goto FREE_TD_BUFF;
    }
  }

  TimeCount = 0;
  Status = CheckIfDone (Ohc, CONTROL_LIST, Ed, HeadTd, &ErrorCode);

  while (Status == EFI_NOT_READY && TimeCount <= TimeOut) {
    MicroSecondDelay (HC_1_MILLISECOND);
    TimeCount++;
    Status = CheckIfDone (Ohc, CONTROL_LIST, Ed, HeadTd, &ErrorCode);
  }
  //
  *TransferResult = ConvertErrorCode (ErrorCode);

  if (ErrorCode != TD_NO_ERROR) {
    if (ErrorCode == TD_TOBE_PROCESSED) {
      DEBUG ((EFI_D_INFO, "Control pipe timeout, > %d mS\r\n", TimeOut));
    } else {
      DEBUG ((EFI_D_INFO, "Control pipe broken\r\n"));
    }

    *DataLength = 0;
  }

  OhciSetHcControl (Ohc, CONTROL_ENABLE, 0);
  if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 0) {
  MicroSecondDelay (HC_1_MILLISECOND);
    if (OhciGetHcControl (Ohc, CONTROL_ENABLE) != 0) {
      *TransferResult = EFI_USB_ERR_SYSTEM;
      DEBUG ((EFI_D_INFO, "OhciControlTransfer: Cannot disable CONTROL_ENABLE transfer\n"));
      goto FREE_TD_BUFF;
    }
  }

FREE_TD_BUFF:
  while (HeadTd) {
    DataTd = HeadTd;
    HeadTd = HeadTd->NextTDPointer;
    UsbHcFreeMem(Ohc->MemPool, DataTd, sizeof(TD_DESCRIPTOR));
  }

FREE_ED_BUFF:
  UsbHcFreeMem(Ohc->MemPool, Ed, sizeof(ED_DESCRIPTOR));

  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  MaxiPacketLength      Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
OhciBulkTransfer (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                       DeviceAddress,
  IN  UINT8                       EndPointAddress,
  IN  UINT8                       MaxPacketLength,
  IN  OUT VOID                    *Data,
  IN  OUT UINTN                   *DataLength,
  IN  OUT UINT8                   *DataToggle,
  IN  UINTN                       TimeOut,
  OUT UINT32                      *TransferResult
  )
{
  USB_OHCI_HC_DEV                *Ohc;
  ED_DESCRIPTOR                  *Ed;
  UINT32                         DataPidDir;
  TD_DESCRIPTOR                  *HeadTd;
  TD_DESCRIPTOR                  *DataTd;
  TD_DESCRIPTOR                  *EmptyTd;
  EFI_STATUS                     Status;
  UINT8                          EndPointNum;
  UINTN                          TimeCount;
  UINT32                         ErrorCode;

  UINT8                          CurrentToggle;
  UINTN                          MapLength;
  EFI_PHYSICAL_ADDRESS           MapPyhAddr;
  UINTN                          LeftLength;
  UINTN                          ActualSendLength;
  BOOLEAN                        FirstTD;

  MapLength = 0;
  MapPyhAddr = 0;
  LeftLength = 0;
  Status = EFI_SUCCESS;

  if (Data == NULL || DataLength == NULL || DataToggle == NULL || TransferResult == NULL ||
      *DataLength == 0 || (*DataToggle != 0 && *DataToggle != 1) ||
      (MaxPacketLength != 8 && MaxPacketLength != 16 &&
       MaxPacketLength != 32 && MaxPacketLength != 64)) {
    return EFI_INVALID_PARAMETER;
  }

  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS (This);

  if ((EndPointAddress & 0x80) != 0) {
    DataPidDir = TD_IN_PID;
  } else {
    DataPidDir = TD_OUT_PID;
  }

  EndPointNum = (EndPointAddress & 0xF);

  OhciSetHcControl (Ohc, BULK_ENABLE, 0);
  if (OhciGetHcControl (Ohc, BULK_ENABLE) != 0) {
    MicroSecondDelay (HC_1_MILLISECOND);
    if (OhciGetHcControl (Ohc, BULK_ENABLE) != 0) {
      *TransferResult = EFI_USB_ERR_SYSTEM;
      return EFI_DEVICE_ERROR;
    }
  }

  OhciSetMemoryPointer (Ohc, HC_BULK_HEAD, NULL);

  Ed = OhciCreateED (Ohc);
  if (Ed == NULL) {
    DEBUG ((EFI_D_INFO, "OhcBulkTransfer: Fail to allocate ED buffer\r\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  OhciSetEDField (Ed, ED_SKIP, 1);
  OhciSetEDField (Ed, ED_FUNC_ADD, DeviceAddress);
  OhciSetEDField (Ed, ED_ENDPT_NUM, EndPointNum);
  OhciSetEDField (Ed, ED_DIR, ED_FROM_TD_DIR);
  OhciSetEDField (Ed, ED_SPEED, HI_SPEED);
  OhciSetEDField (Ed, ED_FORMAT | ED_HALTED | ED_DTTOGGLE, 0);
  OhciSetEDField (Ed, ED_MAX_PACKET, MaxPacketLength);
  OhciSetEDField (Ed, ED_PDATA, 0);
  OhciSetEDField (Ed, ED_ZERO, 0);
  OhciSetEDField (Ed, ED_TDHEAD_PTR, (UINT32) NULL);
  OhciSetEDField (Ed, ED_TDTAIL_PTR, (UINT32) NULL);
  OhciSetEDField (Ed, ED_NEXT_EDPTR, (UINT32) NULL);
  OhciAttachEDToList (Ohc, BULK_LIST, Ed, NULL);

  if(Data != NULL) {
    MapLength = *DataLength;
    MapPyhAddr = (EFI_PHYSICAL_ADDRESS)(UINTN)Data;
  }
  //
  //Data Stage
  //
  LeftLength = MapLength;
  ActualSendLength = MapLength;
  CurrentToggle = *DataToggle;
  HeadTd = NULL;
  FirstTD = TRUE;
  while (LeftLength > 0) {
    ActualSendLength = LeftLength;
    if (LeftLength > MaxPacketLength) {
      ActualSendLength = MaxPacketLength;
    }
    DataTd = OhciCreateTD (Ohc);
    if (DataTd == NULL) {
      DEBUG ((EFI_D_INFO, "OhcBulkTransfer: Fail to allocate Data TD buffer\r\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto FREE_TD_BUFF;
    }
    OhciSetTDField (DataTd, TD_PDATA, 0);
    OhciSetTDField (DataTd, TD_BUFFER_ROUND, 1);
    OhciSetTDField (DataTd, TD_DIR_PID, DataPidDir);
    OhciSetTDField (DataTd, TD_DELAY_INT, TD_NO_DELAY);
    OhciSetTDField (DataTd, TD_DT_TOGGLE, CurrentToggle);
    OhciSetTDField (DataTd, TD_ERROR_CNT, 0);
    OhciSetTDField (DataTd, TD_COND_CODE, TD_TOBE_PROCESSED);
    OhciSetTDField (DataTd, TD_CURR_BUFFER_PTR, (UINT32) MapPyhAddr);
    OhciSetTDField (DataTd, TD_BUFFER_END_PTR, (UINT32) MapPyhAddr + ActualSendLength - 1);
    OhciSetTDField (DataTd, TD_NEXT_PTR, (UINT32) NULL);
    DataTd->ActualSendLength = ActualSendLength;
    DataTd->DataBuffer = (UINT8 *)(UINTN)MapPyhAddr;
    DataTd->NextTDPointer = 0;
    if (FirstTD) {
      HeadTd = DataTd;
      FirstTD = FALSE;
    } else {
      OhciLinkTD (HeadTd, DataTd);
    }
    CurrentToggle ^= 1;
    MapPyhAddr += ActualSendLength;
    LeftLength -= ActualSendLength;
  }
  //
  // Empty Stage
  //
  EmptyTd = OhciCreateTD (Ohc);
  if (EmptyTd == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((EFI_D_INFO, "OhcBulkTransfer: Fail to allocate Empty TD buffer\r\n"));
    goto FREE_TD_BUFF;
  }
  OhciSetTDField (EmptyTd, TD_PDATA, 0);
  OhciSetTDField (EmptyTd, TD_BUFFER_ROUND, 0);
  OhciSetTDField (EmptyTd, TD_DIR_PID, 0);
  OhciSetTDField (EmptyTd, TD_DELAY_INT, 0);
  //OhciSetTDField (EmptyTd, TD_DT_TOGGLE, CurrentToggle);
  EmptyTd->Word0.DataToggle = 0;
  OhciSetTDField (EmptyTd, TD_ERROR_CNT, 0);
  OhciSetTDField (EmptyTd, TD_COND_CODE, 0);
  OhciSetTDField (EmptyTd, TD_CURR_BUFFER_PTR, 0);
  OhciSetTDField (EmptyTd, TD_BUFFER_END_PTR, 0);
  OhciSetTDField (EmptyTd, TD_NEXT_PTR, 0);
  EmptyTd->ActualSendLength = 0;
  EmptyTd->DataBuffer = NULL;
  EmptyTd->NextTDPointer = NULL;
  OhciLinkTD (HeadTd, EmptyTd);
  Ed->TdTailPointer = EmptyTd;
  OhciAttachTDListToED (Ed, HeadTd);

  OhciSetEDField (Ed, ED_SKIP, 0);
  OhciSetHcCommandStatus (Ohc, BULK_LIST_FILLED, 1);
  OhciSetHcControl (Ohc, BULK_ENABLE, 1);
  if (OhciGetHcControl (Ohc, BULK_ENABLE) != 1) {
    MicroSecondDelay (HC_1_MILLISECOND);
    if (OhciGetHcControl (Ohc, BULK_ENABLE) != 1) {
      *TransferResult = EFI_USB_ERR_SYSTEM;
      goto FREE_TD_BUFF;
    }
  }

  TimeCount = 0;
  Status = CheckIfDone (Ohc, BULK_LIST, Ed, HeadTd, &ErrorCode);

  while (Status == EFI_NOT_READY && TimeCount <= TimeOut) {
    MicroSecondDelay (HC_1_MILLISECOND);
    TimeCount++;
    Status = CheckIfDone (Ohc, BULK_LIST, Ed, HeadTd, &ErrorCode);
  }

  *TransferResult = ConvertErrorCode (ErrorCode);

  if (ErrorCode != TD_NO_ERROR) {
    if (ErrorCode == TD_TOBE_PROCESSED) {
      DEBUG ((EFI_D_INFO, "Bulk pipe timeout, > %d mS\r\n", TimeOut));
    } else {
      DEBUG ((EFI_D_INFO, "Bulk pipe broken\r\n"));
    }
    *DataLength = 0;
  }
    *DataToggle = (UINT8) OhciGetEDField (Ed, ED_DTTOGGLE);

FREE_TD_BUFF:
  while (HeadTd) {
    DataTd = HeadTd;
    HeadTd = HeadTd->NextTDPointer;
    UsbHcFreeMem(Ohc->MemPool, DataTd, sizeof(TD_DESCRIPTOR));
  }
  UsbHcFreeMem(Ohc->MemPool, Ed, sizeof(ED_DESCRIPTOR));

  return Status;
}
/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices       The pointer to the PEI Services Table.
  @param[in]  This              The pointer to this instance of the
                                PEI_USB_HOST_CONTROLLER_PPI.
  @param[out] NumOfPorts        The pointer to the number of the root hub ports.

  @retval EFI_SUCCESS           The port number was retrieved successfully.
  @retval EFI_INVALID_PARAMETER PortNumber is NULL.

**/

EFI_STATUS
EFIAPI
OhciGetRootHubNumOfPorts (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  OUT UINT8                       *NumOfPorts
  )
{
  USB_OHCI_HC_DEV                *Ohc;
  if (NumOfPorts == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS (This);
  *NumOfPorts = (UINT8)OhciGetRootHubDescriptor(Ohc, RH_NUM_DS_PORTS);

  return EFI_SUCCESS;
}
/**
  Retrieves the current status of a USB root hub port.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber             The root hub port to retrieve the state from.
  @param  PortStatus             Variable to receive the port state.

  @retval EFI_SUCCESS            The status of the USB root hub port specified.
                                 by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid.

**/

EFI_STATUS
EFIAPI
OhciGetRootHubPortStatus (
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                        PortNumber,
  OUT EFI_USB_PORT_STATUS          *PortStatus
  )
{
  USB_OHCI_HC_DEV  *Ohc;
  UINT8            NumOfPorts;

  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS (This);

  OhciGetRootHubNumOfPorts (PeiServices, This, &NumOfPorts);
  if (PortNumber >= NumOfPorts) {
    return EFI_INVALID_PARAMETER;
  }
  PortStatus->PortStatus = 0;
  PortStatus->PortChangeStatus = 0;

  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_CURR_CONNECT_STAT)) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_PORT_ENABLE_STAT)) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_PORT_SUSPEND_STAT)) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_PORT_OC_INDICATOR)) {
    PortStatus->PortStatus |= USB_PORT_STAT_OVERCURRENT;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_PORT_RESET_STAT)) {
    PortStatus->PortStatus |= USB_PORT_STAT_RESET;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_PORT_POWER_STAT)) {
    PortStatus->PortStatus |= USB_PORT_STAT_POWER;
  }
  if (OhciReadRootHubPortStatus (Ohc,PortNumber, RH_LSDEVICE_ATTACHED)) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
  }
  if (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_ENABLE_STAT_CHANGE)) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }
  if (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_CONNECT_STATUS_CHANGE)) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }
  if (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_SUSPEND_STAT_CHANGE)) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_SUSPEND;
  }
  if (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_OC_INDICATOR_CHANGE)) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_OVERCURRENT;
  }
  if (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT_CHANGE)) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_RESET;
  }

  return EFI_SUCCESS;
}
/**
  Sets a feature for the specified root hub port.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI
  @param  PortNumber            Root hub port to set.
  @param  PortFeature           Feature to set.

  @retval EFI_SUCCESS            The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.
  @retval EFI_TIMEOUT            The time out occurred.

**/

EFI_STATUS
EFIAPI
OhciSetRootHubPortFeature (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  )
{
  USB_OHCI_HC_DEV         *Ohc;
  EFI_STATUS              Status;
  UINT8                   NumOfPorts;
  UINTN                   RetryTimes;

  OhciGetRootHubNumOfPorts (PeiServices, This, &NumOfPorts);
  if (PortNumber >= NumOfPorts) {
    return EFI_INVALID_PARAMETER;
  }

  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS (This);

  Status = EFI_SUCCESS;


  switch (PortFeature) {
    case EfiUsbPortPower:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_SET_PORT_POWER);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_POWER_STAT) == 0 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortReset:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_SET_PORT_RESET);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while ((OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT_CHANGE) == 0 ||
                OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT) == 1) &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }

      OhciSetRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT_CHANGE);
      break;

    case EfiUsbPortEnable:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_SET_PORT_ENABLE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);;
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_ENABLE_STAT) == 0 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;


    case EfiUsbPortSuspend:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_SET_PORT_SUSPEND);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);;
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_SUSPEND_STAT) == 0 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Clears a feature for the specified root hub port.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  PortNumber            Specifies the root hub port whose feature
                                is requested to be cleared.
  @param  PortFeature           Indicates the feature selector associated with the
                                feature clear request.

  @retval EFI_SUCCESS            The feature specified by PortFeature was cleared
                                 for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.

**/

EFI_STATUS
EFIAPI
OhciClearRootHubPortFeature (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN UINT8                        PortNumber,
  IN EFI_USB_PORT_FEATURE         PortFeature
  )
{
  USB_OHCI_HC_DEV         *Ohc;
  EFI_STATUS              Status;
  UINT8                   NumOfPorts;
  UINTN                   RetryTimes;


  OhciGetRootHubNumOfPorts (PeiServices, This, &NumOfPorts);
  if (PortNumber >= NumOfPorts) {
    return EFI_INVALID_PARAMETER;
  }

  Ohc = PEI_RECOVERY_USB_OHC_DEV_FROM_EHCI_THIS (This);

  Status = EFI_SUCCESS;

  switch (PortFeature) {
    case EfiUsbPortEnable:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_CLEAR_PORT_ENABLE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_ENABLE_STAT) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortSuspend:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_CLEAR_SUSPEND_STATUS);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_SUSPEND_STAT) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortReset:
      break;

    case EfiUsbPortPower:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_CLEAR_PORT_POWER);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_POWER_STAT) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortConnectChange:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_CONNECT_STATUS_CHANGE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_CONNECT_STATUS_CHANGE) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortResetChange:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT_CHANGE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_RESET_STAT_CHANGE) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;


    case EfiUsbPortEnableChange:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_PORT_ENABLE_STAT_CHANGE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_ENABLE_STAT_CHANGE) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortSuspendChange:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_PORT_SUSPEND_STAT_CHANGE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_PORT_SUSPEND_STAT_CHANGE) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    case EfiUsbPortOverCurrentChange:
      Status = OhciSetRootHubPortStatus (Ohc, PortNumber, RH_OC_INDICATOR_CHANGE);

      //
      // Verify the state
      //
      RetryTimes = 0;
      do {
        MicroSecondDelay (HC_1_MILLISECOND);
        RetryTimes++;
      } while (OhciReadRootHubPortStatus (Ohc, PortNumber, RH_OC_INDICATOR_CHANGE) == 1 &&
               RetryTimes < MAX_RETRY_TIMES);

      if (RetryTimes >= MAX_RETRY_TIMES) {
        return EFI_DEVICE_ERROR;
      }
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return Status;
}
/**
  Provides software reset for the USB host controller.

  @param  This                  This EFI_USB_HC_PROTOCOL instance.
  @param  Attributes            A bit mask of the reset operation to perform.

  @retval EFI_SUCCESS           The reset operation succeeded.
  @retval EFI_INVALID_PARAMETER Attributes is not valid.
  @retval EFI_UNSUPPOURTED      The type of reset specified by Attributes is
                                not currently supported by the host controller.
  @retval EFI_DEVICE_ERROR      Host controller isn't halted to reset.

**/
EFI_STATUS
InitializeUsbHC (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN USB_OHCI_HC_DEV            *Ohc,
  IN UINT16                     Attributes
  )
{
  EFI_STATUS              Status;
  UINT8                   Index;
  UINT8                   NumOfPorts;
  UINT32                  PowerOnGoodTime;
  UINT32                  Data32;
  BOOLEAN                 Flag = FALSE;

  if ((Attributes & ~(EFI_USB_HC_RESET_GLOBAL | EFI_USB_HC_RESET_HOST_CONTROLLER)) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  Status = EFI_SUCCESS;

  if ((Attributes & EFI_USB_HC_RESET_HOST_CONTROLLER) != 0) {
    MicroSecondDelay (50 * HC_1_MILLISECOND);
    Status = OhciSetHcCommandStatus (Ohc, HC_RESET, HC_RESET);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    MicroSecondDelay (50 * HC_1_MILLISECOND);
    //
    // Wait for host controller reset.
    //
    PowerOnGoodTime = 50;
    do {
      MicroSecondDelay (HC_1_MILLISECOND);
      Data32 = OhciGetOperationalReg (Ohc, HC_COMMAND_STATUS );
      if ((Data32 & HC_RESET) == 0) {
        Flag = TRUE;
        break;
      }
    }while(PowerOnGoodTime--);
    if (!Flag){
      return EFI_DEVICE_ERROR;
    }
  }

  OhciSetFrameInterval (Ohc, FRAME_INTERVAL, 0x2edf);
  if ((Attributes &  EFI_USB_HC_RESET_GLOBAL) != 0) {
    Status = OhciSetHcControl (Ohc, HC_FUNCTIONAL_STATE, HC_STATE_RESET);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }
    MicroSecondDelay (50 * HC_1_MILLISECOND);
  }
  //
  // Initialize host controller operational registers
  //
  OhciSetFrameInterval (Ohc, FS_LARGEST_DATA_PACKET, 0x2778);
  OhciSetFrameInterval (Ohc, FRAME_INTERVAL, 0x2edf);
  OhciSetPeriodicStart (Ohc, 0x2a2f);
  OhciSetHcControl (Ohc, CONTROL_BULK_RATIO, 0x0);
  OhciSetHcCommandStatus (Ohc, CONTROL_LIST_FILLED | BULK_LIST_FILLED, 0);
  OhciSetRootHubDescriptor (Ohc, RH_PSWITCH_MODE, 0);
  OhciSetRootHubDescriptor (Ohc, RH_NO_PSWITCH | RH_NOC_PROT, 1);
  //OhciSetRootHubDescriptor (Hc, RH_PSWITCH_MODE | RH_NO_PSWITCH, 0);
  //OhciSetRootHubDescriptor (Hc, RH_PSWITCH_MODE | RH_NOC_PROT, 1);

  OhciSetRootHubDescriptor (Ohc, RH_DEV_REMOVABLE, 0);
  OhciSetRootHubDescriptor (Ohc, RH_PORT_PWR_CTRL_MASK, 0xffff);
  OhciSetRootHubStatus (Ohc, RH_LOCAL_PSTAT_CHANGE);
  OhciSetRootHubPortStatus (Ohc, 0, RH_SET_PORT_POWER);
  OhciGetRootHubNumOfPorts (PeiServices, &Ohc->UsbHostControllerPpi, &NumOfPorts);
  for (Index = 0; Index < NumOfPorts; Index++) {
    if (!EFI_ERROR (OhciSetRootHubPortFeature (PeiServices, &Ohc->UsbHostControllerPpi, Index, EfiUsbPortReset))) {
      MicroSecondDelay (200 * HC_1_MILLISECOND);
      OhciClearRootHubPortFeature (PeiServices, &Ohc->UsbHostControllerPpi, Index, EfiUsbPortReset);
      MicroSecondDelay (HC_1_MILLISECOND);
      OhciSetRootHubPortFeature (PeiServices, &Ohc->UsbHostControllerPpi, Index, EfiUsbPortEnable);
      MicroSecondDelay (HC_1_MILLISECOND);
    }
  }

  Ohc->MemPool = UsbHcInitMemPool(TRUE, 0);
  if(Ohc->MemPool == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  OhciSetMemoryPointer (Ohc, HC_CONTROL_HEAD, NULL);
  OhciSetMemoryPointer (Ohc, HC_BULK_HEAD, NULL);
  OhciSetHcControl (Ohc, CONTROL_ENABLE | BULK_ENABLE, 1);
  OhciSetHcControl (Ohc, HC_FUNCTIONAL_STATE, HC_STATE_OPERATIONAL);
  MicroSecondDelay (50 * HC_1_MILLISECOND);
  //
  // Wait till first SOF occurs, and then clear it
  //
  while (OhciGetHcInterruptStatus (Ohc, START_OF_FRAME) == 0);
  OhciClearInterruptStatus (Ohc, START_OF_FRAME);
  MicroSecondDelay (HC_1_MILLISECOND);

  return EFI_SUCCESS;
}

/**
  Submits control transfer to a target USB device.

  Calls underlying OhciControlTransfer to do work. This wrapper routine required
  on Quark so that USB DMA transfers do not cause an IMR violation.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress          The target device address.
  @param  DeviceSpeed            Target device speed.
  @param  MaximumPacketLength    Maximum packet size the default control transfer
                                 endpoint is capable of sending or receiving.
  @param  Request                USB device request to send.
  @param  TransferDirection      Specifies the data direction for the data stage.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.
  @param  TimeOut                Indicates the maximum timeout, in millisecond.
  @param  TransferResult         Return the result of this control transfer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
RedirectOhciControlTransfer (
  IN  EFI_PEI_SERVICES             **PeiServices,
  IN  PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                        DeviceAddress,
  IN  UINT8                        DeviceSpeed,
  IN  UINT8                        MaxPacketLength,
  IN  EFI_USB_DEVICE_REQUEST       *Request,
  IN  EFI_USB_DATA_DIRECTION       TransferDirection,
  IN  OUT VOID                     *Data,
  IN  OUT UINTN                    *DataLength,
  IN  UINTN                        TimeOut,
  OUT UINT32                       *TransferResult
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  *NewRequest;
  VOID                    *NewData;
  UINT8                   *Alloc;

  //
  // Allocate memory external to IMR protected region for transfer data.
  //
  Status = PeiServicesAllocatePool (
                             sizeof(EFI_USB_DEVICE_REQUEST) + *DataLength,
                             (VOID **) &Alloc
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Setup pointers to transfer buffers.
  //
  NewRequest = (EFI_USB_DEVICE_REQUEST *) Alloc;
  Alloc += sizeof(EFI_USB_DEVICE_REQUEST);
  NewData = (VOID *) Alloc;

  //
  // Copy callers request packet into transfer request packet.
  //
  if (Request != NULL) {
    CopyMem (NewRequest,Request,sizeof(EFI_USB_DEVICE_REQUEST));
  } else {
    NewRequest = NULL;
  }
  //
  // Copy callers data into transfer data buffer.
  //
  if (Data != NULL) {
    if (DataLength > 0) {
      CopyMem (NewData,Data,*DataLength);
    }
  } else {
    NewData = NULL;
  }

  //
  // Call underlying OhciControlTransfer to do work.
  //
  Status = OhciControlTransfer (
             PeiServices,
             This,
             DeviceAddress,
             DeviceSpeed,
             MaxPacketLength,
             NewRequest,
             TransferDirection,
             NewData,
             DataLength,
             TimeOut,
             TransferResult
             );

  //
  // Copy transfer buffer back into callers buffer.
  //
  if (Data != NULL && *DataLength > 0) {
    CopyMem (Data, NewData, *DataLength);
  }

  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  Calls underlying OhciBulkTransfer to do work. This wrapper routine required
  on Quark so that USB DMA transfers do not cause an IMR violation.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  MaxiPacketLength      Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  Data                  A pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
  @param  TransferResult        A pointer to the detailed result information of the
                                bulk transfer.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
RedirectOhciBulkTransfer (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_USB_HOST_CONTROLLER_PPI  *This,
  IN  UINT8                       DeviceAddress,
  IN  UINT8                       EndPointAddress,
  IN  UINT8                       MaxPacketLength,
  IN  OUT VOID                    *Data,
  IN  OUT UINTN                   *DataLength,
  IN  OUT UINT8                   *DataToggle,
  IN  UINTN                       TimeOut,
  OUT UINT32                      *TransferResult
  )
{
  EFI_STATUS              Status;
  UINT8                   *NewData;

  //
  // Allocate memory external to IMR protected region for transfer data.
  //
  Status = PeiServicesAllocatePool (
                             *DataLength,
                             (VOID **) &NewData
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Copy callers data into transfer buffer.
  //
  if (Data != NULL) {
    if (DataLength > 0) {
      CopyMem (NewData,Data,*DataLength);
    }
  } else {
    NewData = NULL;
  }

  //
  // Call underlying OhciBulkTransfer to do work.
  //
  Status = OhciBulkTransfer (
             PeiServices,
             This,
             DeviceAddress,
             EndPointAddress,
             MaxPacketLength,
             NewData,
             DataLength,
             DataToggle,
             TimeOut,
             TransferResult
             );

  //
  // Copy transfer buffer back into callers buffer.
  //
  if (Data != NULL && *DataLength > 0) {
    CopyMem (Data, NewData, *DataLength);
  }

  return Status;
}

/**
  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            PPI successfully installed.

**/
EFI_STATUS
OhcPeimEntry (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{

  PEI_USB_CONTROLLER_PPI  *ChipSetUsbControllerPpi;
  EFI_STATUS              Status;
  UINT8                   Index;
  UINTN                   ControllerType;
  UINTN                   BaseAddress;
  UINTN                   MemPages;
  USB_OHCI_HC_DEV         *Ohc;
  EFI_PHYSICAL_ADDRESS    TempPtr;


  //
  // Shadow this PEIM to run from memory
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }
  Status = PeiServicesLocatePpi (
             &gPeiUsbControllerPpiGuid,
             0,
             NULL,
             (VOID **) &ChipSetUsbControllerPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Index = 0;
  while (TRUE) {
    Status = ChipSetUsbControllerPpi->GetUsbController (
                                        (EFI_PEI_SERVICES **) PeiServices,
                                        ChipSetUsbControllerPpi,
                                        Index,
                                        &ControllerType,
                                        &BaseAddress
                                        );
    //
    // When status is error, meant no controller is found
    //
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // This PEIM is for OHC type controller.
    //
    if (ControllerType != PEI_OHCI_CONTROLLER) {
      Index++;
      continue;
    }

    MemPages = sizeof (USB_OHCI_HC_DEV) / PAGESIZE + 1;
    Status = PeiServicesAllocatePages (
               EfiBootServicesCode,
               MemPages,
               &TempPtr
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "OhcPeimEntry: Fail to allocate buffer for the %dth OHCI ControllerPpi\n", Index));
      return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem((VOID *)(UINTN)TempPtr, MemPages*PAGESIZE);
    Ohc = (USB_OHCI_HC_DEV *) ((UINTN) TempPtr);

    Ohc->Signature = USB_OHCI_HC_DEV_SIGNATURE;

    Ohc->UsbHostControllerBaseAddress = (UINT32) BaseAddress;

    //
    // Initialize Uhc's hardware
    //
    Status = InitializeUsbHC (
               (EFI_PEI_SERVICES **)PeiServices,
               Ohc,
               EFI_USB_HC_RESET_GLOBAL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_INFO, "OhcPeimEntry: Fail to init %dth OHCI ControllerPpi\n", Index));
      return Status;
    }
    //
    // Control & Bulk transfer services are accessed via their Redirect
    // routine versions on Quark so that USB DMA transfers do not cause an
    // IMR violation.
    //
    Ohc->UsbHostControllerPpi.ControlTransfer          = RedirectOhciControlTransfer;
    Ohc->UsbHostControllerPpi.BulkTransfer             = RedirectOhciBulkTransfer;
    Ohc->UsbHostControllerPpi.GetRootHubPortNumber     = OhciGetRootHubNumOfPorts;
    Ohc->UsbHostControllerPpi.GetRootHubPortStatus     = OhciGetRootHubPortStatus;
    Ohc->UsbHostControllerPpi.SetRootHubPortFeature    = OhciSetRootHubPortFeature;
    Ohc->UsbHostControllerPpi.ClearRootHubPortFeature  = OhciClearRootHubPortFeature;

    Ohc->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    Ohc->PpiDescriptor.Guid  = &gPeiUsbHostControllerPpiGuid;
    Ohc->PpiDescriptor.Ppi   = &Ohc->UsbHostControllerPpi;

    Status = PeiServicesInstallPpi (&Ohc->PpiDescriptor);
    if (EFI_ERROR (Status)) {
      Index++;
      continue;
    }
    Index++;
  }
  return EFI_SUCCESS;
}

