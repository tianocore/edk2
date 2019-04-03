/** @file

  The UHCI driver model and HC protocol routines.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Uhci.h"


EFI_DRIVER_BINDING_PROTOCOL gUhciDriverBinding = {
  UhciDriverBindingSupported,
  UhciDriverBindingStart,
  UhciDriverBindingStop,
  0x20,
  NULL,
  NULL
};

/**
  Provides software reset for the USB host controller according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  Attributes             A bit mask of the reset operation to perform.  See
                                 below for a list of the supported bit mask values.

  @return EFI_SUCCESS            The reset operation succeeded.
  @return EFI_INVALID_PARAMETER  Attributes is not valid.
  @return EFI_UNSUPPORTED        This type of reset is not currently supported.
  @return EFI_DEVICE_ERROR       Other errors.

**/
EFI_STATUS
EFIAPI
Uhci2Reset (
  IN EFI_USB2_HC_PROTOCOL   *This,
  IN UINT16                 Attributes
  )
{
  USB_HC_DEV          *Uhc;
  EFI_TPL             OldTpl;

  if ((Attributes == EFI_USB_HC_RESET_GLOBAL_WITH_DEBUG) ||
      (Attributes == EFI_USB_HC_RESET_HOST_WITH_DEBUG)) {
    return EFI_UNSUPPORTED;
  }

  Uhc = UHC_FROM_USB2_HC_PROTO (This);

  if (Uhc->DevicePath != NULL) {
    //
    // Report Status Code to indicate reset happens
    //
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
      EFI_PROGRESS_CODE,
      (EFI_IO_BUS_USB | EFI_IOB_PC_RESET),
      Uhc->DevicePath
      );
  }

  OldTpl  = gBS->RaiseTPL (UHCI_TPL);

  switch (Attributes) {
  case EFI_USB_HC_RESET_GLOBAL:
    //
    // Stop schedule and set the Global Reset bit in the command register
    //
    UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
    UhciSetRegBit (Uhc->PciIo, USBCMD_OFFSET, USBCMD_GRESET);

    gBS->Stall (UHC_ROOT_PORT_RESET_STALL);

    //
    // Clear the Global Reset bit to zero.
    //
    UhciClearRegBit (Uhc->PciIo, USBCMD_OFFSET, USBCMD_GRESET);

    gBS->Stall (UHC_ROOT_PORT_RECOVERY_STALL);
    break;

  case EFI_USB_HC_RESET_HOST_CONTROLLER:
    //
    // Stop schedule and set Host Controller Reset bit to 1
    //
    UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
    UhciSetRegBit (Uhc->PciIo, USBCMD_OFFSET, USBCMD_HCRESET);

    gBS->Stall (UHC_ROOT_PORT_RECOVERY_STALL);
    break;

  default:
    goto ON_INVAILD_PARAMETER;
  }

  //
  // Delete all old transactions on the USB bus, then
  // reinitialize the frame list
  //
  UhciFreeAllAsyncReq (Uhc);
  UhciDestoryFrameList (Uhc);
  UhciInitFrameList (Uhc);

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_INVAILD_PARAMETER:

  gBS->RestoreTPL (OldTpl);

  return EFI_INVALID_PARAMETER;
}


/**
  Retrieves current state of the USB host controller according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  State                  Variable to receive current device state.

  @return EFI_SUCCESS            The state is returned.
  @return EFI_INVALID_PARAMETER  State is not valid.
  @return EFI_DEVICE_ERROR       Other errors.

**/
EFI_STATUS
EFIAPI
Uhci2GetState (
  IN   EFI_USB2_HC_PROTOCOL   *This,
  OUT  EFI_USB_HC_STATE       *State
  )
{
  USB_HC_DEV          *Uhc;
  UINT16              UsbSts;
  UINT16              UsbCmd;

  if (State == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Uhc     = UHC_FROM_USB2_HC_PROTO (This);

  UsbCmd  = UhciReadReg (Uhc->PciIo, USBCMD_OFFSET);
  UsbSts  = UhciReadReg (Uhc->PciIo, USBSTS_OFFSET);

  if ((UsbCmd & USBCMD_EGSM) !=0 ) {
    *State = EfiUsbHcStateSuspend;

  } else if ((UsbSts & USBSTS_HCH) != 0) {
    *State = EfiUsbHcStateHalt;

  } else {
    *State = EfiUsbHcStateOperational;
  }

  return EFI_SUCCESS;
}


/**
  Sets the USB host controller to a specific state according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  State                  Indicates the state of the host controller that will
                                 be set.

  @return EFI_SUCCESS            Host controller was successfully placed in the state.
  @return EFI_INVALID_PARAMETER  State is invalid.
  @return EFI_DEVICE_ERROR       Failed to set the state.

**/
EFI_STATUS
EFIAPI
Uhci2SetState (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN EFI_USB_HC_STATE        State
  )
{
  EFI_USB_HC_STATE    CurState;
  USB_HC_DEV          *Uhc;
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  UINT16              UsbCmd;

  Uhc     = UHC_FROM_USB2_HC_PROTO (This);
  Status  = Uhci2GetState (This, &CurState);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (CurState == State) {
    return EFI_SUCCESS;
  }

  Status  = EFI_SUCCESS;
  OldTpl  = gBS->RaiseTPL (UHCI_TPL);

  switch (State) {
  case EfiUsbHcStateHalt:
    Status = UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
    break;

  case EfiUsbHcStateOperational:
    UsbCmd = UhciReadReg (Uhc->PciIo, USBCMD_OFFSET);

    if (CurState == EfiUsbHcStateHalt) {
      //
      // Set Run/Stop bit to 1, also set the bandwidht reclamation
      // point to 64 bytes
      //
      UsbCmd |= USBCMD_RS | USBCMD_MAXP;
      UhciWriteReg (Uhc->PciIo, USBCMD_OFFSET, UsbCmd);

    } else if (CurState == EfiUsbHcStateSuspend) {
      //
      // If FGR(Force Global Resume) bit is 0, set it
      //
      if ((UsbCmd & USBCMD_FGR) == 0) {
        UsbCmd |= USBCMD_FGR;
        UhciWriteReg (Uhc->PciIo, USBCMD_OFFSET, UsbCmd);
      }

      //
      // wait 20ms to let resume complete (20ms is specified by UHCI spec)
      //
      gBS->Stall (UHC_FORCE_GLOBAL_RESUME_STALL);

      //
      // Write FGR bit to 0 and EGSM(Enter Global Suspend Mode) bit to 0
      //
      UsbCmd &= ~USBCMD_FGR;
      UsbCmd &= ~USBCMD_EGSM;
      UsbCmd |= USBCMD_RS;
      UhciWriteReg (Uhc->PciIo, USBCMD_OFFSET, UsbCmd);
    }

    break;

  case EfiUsbHcStateSuspend:
    Status = Uhci2SetState (This, EfiUsbHcStateHalt);

    if (EFI_ERROR (Status)) {
      Status = EFI_DEVICE_ERROR;
      goto ON_EXIT;
    }

    //
    // Set Enter Global Suspend Mode bit to 1.
    //
    UsbCmd = UhciReadReg (Uhc->PciIo, USBCMD_OFFSET);
    UsbCmd |= USBCMD_EGSM;
    UhciWriteReg (Uhc->PciIo, USBCMD_OFFSET, UsbCmd);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Retrieves capabilities of USB host controller according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  MaxSpeed               A pointer to the max speed USB host controller
                                 supports.
  @param  PortNumber             A pointer to the number of root hub ports.
  @param  Is64BitCapable         A pointer to an integer to show whether USB host
                                 controller supports 64-bit memory addressing.

  @return EFI_SUCCESS            capabilities were retrieved successfully.
  @return EFI_INVALID_PARAMETER  MaxSpeed or PortNumber or Is64BitCapable is NULL.
  @return EFI_DEVICE_ERROR       An error was encountered.

**/
EFI_STATUS
EFIAPI
Uhci2GetCapability (
  IN  EFI_USB2_HC_PROTOCOL  *This,
  OUT UINT8                 *MaxSpeed,
  OUT UINT8                 *PortNumber,
  OUT UINT8                 *Is64BitCapable
  )
{
  USB_HC_DEV          *Uhc;
  UINT32              Offset;
  UINT16              PortSC;
  UINT32              Index;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);

  if ((NULL == MaxSpeed) || (NULL == PortNumber) || (NULL == Is64BitCapable)) {
    return EFI_INVALID_PARAMETER;
  }

  *MaxSpeed       = EFI_USB_SPEED_FULL;
  *Is64BitCapable = (UINT8) FALSE;

  *PortNumber = 0;

  for (Index = 0; Index < USB_MAX_ROOTHUB_PORT; Index++) {
    Offset  = USBPORTSC_OFFSET + Index * 2;
    PortSC  = UhciReadReg (Uhc->PciIo, Offset);

    //
    // Port status's bit 7 is reserved and always returns 1 if
    // the port number is valid. Intel's UHCI (in EHCI controller)
    // returns 0 in this bit if port number is invalid. Also, if
    // PciIo IoRead returns error, 0xFFFF is returned to caller.
    //
    if (((PortSC & 0x80) == 0) || (PortSC == 0xFFFF)) {
      break;
    }
    (*PortNumber)++;
  }

  Uhc->RootPorts = *PortNumber;

  DEBUG ((EFI_D_INFO, "Uhci2GetCapability: %d ports\n", (UINT32)Uhc->RootPorts));
  return EFI_SUCCESS;
}


/**
  Retrieves the current status of a USB root hub port according to UEFI 2.0 spec.

  @param  This                    A pointer to the EFI_USB2_HC_PROTOCOL.
  @param  PortNumber              The port to get status.
  @param  PortStatus              A pointer to the current port status bits and  port
                                  status change bits.

  @return EFI_SUCCESS             status of the USB root hub port was returned in PortStatus.
  @return EFI_INVALID_PARAMETER   PortNumber is invalid.
  @return EFI_DEVICE_ERROR        Can't read register.

**/
EFI_STATUS
EFIAPI
Uhci2GetRootHubPortStatus (
  IN   EFI_USB2_HC_PROTOCOL   *This,
  IN   UINT8                  PortNumber,
  OUT  EFI_USB_PORT_STATUS    *PortStatus
  )
{
  USB_HC_DEV          *Uhc;
  UINT32              Offset;
  UINT16              PortSC;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (PortNumber >= Uhc->RootPorts) {
    return EFI_INVALID_PARAMETER;
  }

  Offset                        = USBPORTSC_OFFSET + PortNumber * 2;
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  PortSC                        = UhciReadReg (Uhc->PciIo, Offset);

  if ((PortSC & USBPORTSC_CCS) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_CONNECTION;
  }

  if ((PortSC & USBPORTSC_PED) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_ENABLE;
  }

  if ((PortSC & USBPORTSC_SUSP) != 0) {
    DEBUG ((EFI_D_INFO, "Uhci2GetRootHubPortStatus: port %d is suspended\n", PortNumber));
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  if ((PortSC & USBPORTSC_PR) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_RESET;
  }

  if ((PortSC & USBPORTSC_LSDA) != 0) {
    PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
  }

  //
  // CHC will always return one in port owner bit
  //
  PortStatus->PortStatus |= USB_PORT_STAT_OWNER;

  if ((PortSC & USBPORTSC_CSC) != 0) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_CONNECTION;
  }

  if ((PortSC & USBPORTSC_PEDC) != 0) {
    PortStatus->PortChangeStatus |= USB_PORT_STAT_C_ENABLE;
  }

  return EFI_SUCCESS;
}


/**
  Sets a feature for the specified root hub port according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL.
  @param  PortNumber             Specifies the root hub port whose feature  is
                                 requested to be set.
  @param  PortFeature            Indicates the feature selector associated  with the
                                 feature set request.

  @return EFI_SUCCESS            PortFeature was set for the root port.
  @return EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.
  @return EFI_DEVICE_ERROR       Can't read register.

**/
EFI_STATUS
EFIAPI
Uhci2SetRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  USB_HC_DEV          *Uhc;
  EFI_TPL             OldTpl;
  UINT32              Offset;
  UINT16              PortSC;
  UINT16              Command;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);

  if (PortNumber >= Uhc->RootPorts) {
    return EFI_INVALID_PARAMETER;
  }

  Offset  = USBPORTSC_OFFSET + PortNumber * 2;

  OldTpl  = gBS->RaiseTPL (UHCI_TPL);
  PortSC  = UhciReadReg (Uhc->PciIo, Offset);

  switch (PortFeature) {
  case EfiUsbPortSuspend:
    Command = UhciReadReg (Uhc->PciIo, USBCMD_OFFSET);
    if ((Command & USBCMD_EGSM) == 0) {
      //
      // if global suspend is not active, can set port suspend
      //
      PortSC &= 0xfff5;
      PortSC |= USBPORTSC_SUSP;
    }
    break;

  case EfiUsbPortReset:
    PortSC &= 0xfff5;
    PortSC |= USBPORTSC_PR;
    break;

  case EfiUsbPortPower:
    //
    // No action
    //
    break;

  case EfiUsbPortEnable:
    PortSC &= 0xfff5;
    PortSC |= USBPORTSC_PED;
    break;

  default:
    gBS->RestoreTPL (OldTpl);
    return EFI_INVALID_PARAMETER;
  }

  UhciWriteReg (Uhc->PciIo, Offset, PortSC);
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}


/**
  Clears a feature for the specified root hub port according to Uefi 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  PortNumber             Specifies the root hub port whose feature  is
                                 requested to be cleared.
  @param  PortFeature            Indicates the feature selector associated with the
                                 feature clear request.

  @return EFI_SUCCESS            PortFeature was cleared for the USB root hub port.
  @return EFI_INVALID_PARAMETER  PortNumber is invalid or PortFeature is invalid.
  @return EFI_DEVICE_ERROR       Can't read register.

**/
EFI_STATUS
EFIAPI
Uhci2ClearRootHubPortFeature (
  IN EFI_USB2_HC_PROTOCOL    *This,
  IN UINT8                   PortNumber,
  IN EFI_USB_PORT_FEATURE    PortFeature
  )
{
  USB_HC_DEV          *Uhc;
  EFI_TPL             OldTpl;
  UINT32              Offset;
  UINT16              PortSC;

  Uhc = UHC_FROM_USB2_HC_PROTO (This);

  if (PortNumber >= Uhc->RootPorts) {
    return EFI_INVALID_PARAMETER;
  }

  Offset  = USBPORTSC_OFFSET + PortNumber * 2;

  OldTpl  = gBS->RaiseTPL (UHCI_TPL);
  PortSC  = UhciReadReg (Uhc->PciIo, Offset);

  switch (PortFeature) {
  case EfiUsbPortEnable:
    PortSC &= 0xfff5;
    PortSC &= ~USBPORTSC_PED;
    break;

  case EfiUsbPortSuspend:
    //
    // Cause a resume on the specified port if in suspend mode.
    //
    PortSC &= 0xfff5;
    PortSC &= ~USBPORTSC_SUSP;
    break;

  case EfiUsbPortPower:
    //
    // No action
    //
    break;

  case EfiUsbPortReset:
    PortSC &= 0xfff5;
    PortSC &= ~USBPORTSC_PR;
    break;

  case EfiUsbPortConnectChange:
    PortSC &= 0xfff5;
    PortSC |= USBPORTSC_CSC;
    break;

  case EfiUsbPortEnableChange:
    PortSC &= 0xfff5;
    PortSC |= USBPORTSC_PEDC;
    break;

  case EfiUsbPortSuspendChange:
    //
    // Root hub does not support this
    //
    break;

  case EfiUsbPortOverCurrentChange:
    //
    // Root hub does not support this
    //
    break;

  case EfiUsbPortResetChange:
    //
    // Root hub does not support this
    //
    break;

  default:
    gBS->RestoreTPL (OldTpl);
    return EFI_INVALID_PARAMETER;
  }

  UhciWriteReg (Uhc->PciIo, Offset, PortSC);
  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}


/**
  Submits control transfer to a target USB device according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress          Target device address.
  @param  DeviceSpeed            Device speed.
  @param  MaximumPacketLength    Maximum packet size of the target endpoint.
  @param  Request                USB device request to send.
  @param  TransferDirection      Data direction of the Data stage in control transfer.
  @param  Data                   Data to transmit/receive in data stage.
  @param  DataLength             Length of the data.
  @param  TimeOut                Maximum time, in microseconds, for transfer to complete.
  @param  Translator             Transaction translator to be used by this device.
  @param  TransferResult         Variable to receive the transfer result.

  @return EFI_SUCCESS            The control transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES   Failed due to lack of resource.
  @return EFI_INVALID_PARAMETER  Some parameters are invalid.
  @return EFI_TIMEOUT            Failed due to timeout.
  @return EFI_DEVICE_ERROR       Failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
Uhci2ControlTransfer (
  IN     EFI_USB2_HC_PROTOCOL                 *This,
  IN     UINT8                                DeviceAddress,
  IN     UINT8                                DeviceSpeed,
  IN     UINTN                                MaximumPacketLength,
  IN     EFI_USB_DEVICE_REQUEST               *Request,
  IN     EFI_USB_DATA_DIRECTION               TransferDirection,
  IN OUT VOID                                 *Data,
  IN OUT UINTN                                *DataLength,
  IN     UINTN                                TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR   *Translator,
  OUT    UINT32                               *TransferResult
  )
{
  USB_HC_DEV              *Uhc;
  UHCI_TD_SW              *TDs;
  EFI_TPL                 OldTpl;
  EFI_STATUS              Status;
  UHCI_QH_RESULT          QhResult;
  UINT8                   PktId;
  UINT8                   *RequestPhy;
  VOID                    *RequestMap;
  UINT8                   *DataPhy;
  VOID                    *DataMap;
  BOOLEAN                 IsSlowDevice;
  UINTN                   TransferDataLength;

  Uhc         = UHC_FROM_USB2_HC_PROTO (This);
  TDs         = NULL;
  DataPhy     = NULL;
  DataMap     = NULL;
  RequestPhy  = NULL;
  RequestMap  = NULL;

  IsSlowDevice  = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);

  //
  // Parameters Checking
  //
  if (Request == NULL || TransferResult == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsSlowDevice && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8) &&  (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {

    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) && (Data == NULL || DataLength == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (TransferDirection == EfiUsbNoData) {
    TransferDataLength = 0;
  } else {
    TransferDataLength = *DataLength;
  }

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  //
  // If errors exist that cause host controller halt,
  // clear status then return EFI_DEVICE_ERROR.
  //
  UhciAckAllInterrupt (Uhc);

  if (!UhciIsHcWorking (Uhc->PciIo)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (UHCI_TPL);

  //
  // Map the Request and data for bus master access,
  // then create a list of TD for this transfer
  //
  Status = UhciMapUserRequest (Uhc, Request, &RequestPhy, &RequestMap);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = UhciMapUserData (Uhc, TransferDirection, Data, DataLength, &PktId, &DataPhy, &DataMap);

  if (EFI_ERROR (Status)) {
    Uhc->PciIo->Unmap (Uhc->PciIo, RequestMap);
    goto ON_EXIT;
  }

  TDs = UhciCreateCtrlTds (
          Uhc,
          DeviceAddress,
          PktId,
          (UINT8*)Request,
          RequestPhy,
          (UINT8*)Data,
          DataPhy,
          TransferDataLength,
          (UINT8) MaximumPacketLength,
          IsSlowDevice
          );

  if (TDs == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto UNMAP_DATA;
  }

  //
  // According to the speed of the end point, link
  // the TD to corrosponding queue head, then check
  // the execution result
  //
  UhciLinkTdToQh (Uhc, Uhc->CtrlQh, TDs);
  Status = UhciExecuteTransfer (Uhc, Uhc->CtrlQh, TDs, TimeOut, IsSlowDevice, &QhResult);
  UhciUnlinkTdFromQh (Uhc->CtrlQh, TDs);

  Uhc->PciIo->Flush (Uhc->PciIo);

  *TransferResult = QhResult.Result;

  if (DataLength != NULL) {
    *DataLength = QhResult.Complete;
  }

  UhciDestoryTds (Uhc, TDs);

UNMAP_DATA:
  Uhc->PciIo->Unmap (Uhc->PciIo, DataMap);
  Uhc->PciIo->Unmap (Uhc->PciIo, RequestMap);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress          Target device address.
  @param  EndPointAddress        Endpoint number and direction.
  @param  DeviceSpeed            Device speed.
  @param  MaximumPacketLength    Maximum packet size of the target endpoint.
  @param  DataBuffersNumber      Number of data buffers prepared for the transfer.
  @param  Data                   Array of pointers to the buffers of data.
  @param  DataLength             On input, size of the data buffer, On output,
                                 actually transferred data size.
  @param  DataToggle             On input, data toggle to use; On output, next data toggle.
  @param  TimeOut                Maximum time out, in microseconds.
  @param  Translator             A pointr to the transaction translator data.
  @param  TransferResult         Variable to receive transfer result.

  @return EFI_SUCCESS            The bulk transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES   Failed due to lack of resource.
  @return EFI_INVALID_PARAMETER  Some parameters are invalid.
  @return EFI_TIMEOUT            Failed due to timeout.
  @return EFI_DEVICE_ERROR       Failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
Uhci2BulkTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
{
  EFI_USB_DATA_DIRECTION  Direction;
  EFI_TPL                 OldTpl;
  USB_HC_DEV              *Uhc;
  UHCI_TD_SW              *TDs;
  UHCI_QH_SW              *BulkQh;
  UHCI_QH_RESULT          QhResult;
  EFI_STATUS              Status;
  UINT8                   PktId;
  UINT8                   *DataPhy;
  VOID                    *DataMap;

  Uhc     = UHC_FROM_USB2_HC_PROTO (This);
  DataPhy = NULL;
  DataMap = NULL;

  if (DeviceSpeed == EFI_USB_SPEED_LOW) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataLength == NULL) || (*DataLength == 0) || (Data == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8) && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64)) {
    return EFI_INVALID_PARAMETER;
  }

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_OUT_OF_RESOURCES;

  //
  // If has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  UhciAckAllInterrupt (Uhc);

  if (!UhciIsHcWorking (Uhc->PciIo)) {
    return EFI_DEVICE_ERROR;
  }

  OldTpl = gBS->RaiseTPL (UHCI_TPL);

  //
  // Map the source data buffer for bus master access,
  // then create a list of TDs
  //
  if ((EndPointAddress & 0x80) != 0) {
    Direction = EfiUsbDataIn;
  } else {
    Direction = EfiUsbDataOut;
  }

  Status = UhciMapUserData (Uhc, Direction, *Data, DataLength, &PktId, &DataPhy, &DataMap);

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  Status = EFI_OUT_OF_RESOURCES;
  TDs    = UhciCreateBulkOrIntTds (
             Uhc,
             DeviceAddress,
             EndPointAddress,
             PktId,
             (UINT8 *)*Data,
             DataPhy,
             *DataLength,
             DataToggle,
             (UINT8) MaximumPacketLength,
             FALSE
             );

  if (TDs == NULL) {
    Uhc->PciIo->Unmap (Uhc->PciIo, DataMap);
    goto ON_EXIT;
  }


  //
  // Link the TDs to bulk queue head. According to the platfore
  // defintion of UHCI_NO_BW_RECLAMATION, BulkQh is either configured
  // to do full speed bandwidth reclamation or not.
  //
  BulkQh = Uhc->BulkQh;

  UhciLinkTdToQh (Uhc, BulkQh, TDs);
  Status = UhciExecuteTransfer (Uhc, BulkQh, TDs, TimeOut, FALSE, &QhResult);
  UhciUnlinkTdFromQh (BulkQh, TDs);

  Uhc->PciIo->Flush (Uhc->PciIo);

  *TransferResult = QhResult.Result;
  *DataToggle     = QhResult.NextToggle;
  *DataLength     = QhResult.Complete;

  UhciDestoryTds (Uhc, TDs);
  Uhc->PciIo->Unmap (Uhc->PciIo, DataMap);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Submits an asynchronous interrupt transfer to an
  interrupt endpoint of a USB device according to UEFI 2.0 spec.

  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress          Target device address.
  @param  EndPointAddress        Endpoint number and direction.
  @param  DeviceSpeed            Device speed.
  @param  MaximumPacketLength    Maximum packet size of the target endpoint.
  @param  IsNewTransfer          If TRUE, submit a new transfer, if FALSE cancel old transfer.
  @param  DataToggle             On input, data toggle to use; On output, next data toggle.
  @param  PollingInterval        Interrupt poll rate in milliseconds.
  @param  DataLength             On input, size of the data buffer, On output,
                                 actually transferred data size.
  @param  Translator             A pointr to the transaction translator data.
  @param  CallBackFunction       Function to call periodically.
  @param  Context                User context.

  @return EFI_SUCCESS            Transfer was submitted.
  @return EFI_INVALID_PARAMETER  Some parameters are invalid.
  @return EFI_OUT_OF_RESOURCES   Failed due to a lack of resources.
  @return EFI_DEVICE_ERROR       Can't read register.

**/
EFI_STATUS
EFIAPI
Uhci2AsyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     BOOLEAN                            IsNewTransfer,
  IN OUT UINT8                              *DataToggle,
  IN     UINTN                              PollingInterval,
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK    CallBackFunction,
  IN     VOID                               *Context
  )
{
  USB_HC_DEV          *Uhc;
  BOOLEAN             IsSlowDevice;
  UHCI_QH_SW          *Qh;
  UHCI_TD_SW          *IntTds;
  EFI_TPL             OldTpl;
  EFI_STATUS          Status;
  UINT8               *DataPtr;
  UINT8               *DataPhy;
  UINT8               PktId;

  Uhc       = UHC_FROM_USB2_HC_PROTO (This);
  Qh        = NULL;
  IntTds    = NULL;
  DataPtr   = NULL;
  DataPhy   = NULL;

  IsSlowDevice  = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);

  if ((EndPointAddress & 0x80) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Delete Async interrupt transfer request
  //
  if (!IsNewTransfer) {
    OldTpl = gBS->RaiseTPL (UHCI_TPL);
    Status = UhciRemoveAsyncReq (Uhc, DeviceAddress, EndPointAddress, DataToggle);

    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  if (PollingInterval < 1 || PollingInterval > 255) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataLength == 0) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If has errors that cause host controller halt,
  // then return EFI_DEVICE_ERROR directly.
  //
  UhciAckAllInterrupt (Uhc);

  if (!UhciIsHcWorking (Uhc->PciIo)) {
    return EFI_DEVICE_ERROR;
  }

  if ((EndPointAddress & 0x80) == 0) {
    PktId = OUTPUT_PACKET_ID;
  } else {
    PktId = INPUT_PACKET_ID;
  }

  //
  // Allocate and map source data buffer for bus master access.
  //
  DataPtr = UsbHcAllocateMem (Uhc->MemPool, DataLength);

  if (DataPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DataPhy = (UINT8 *) (UINTN) UsbHcGetPciAddressForHostMem (Uhc->MemPool, DataPtr, DataLength);

  OldTpl = gBS->RaiseTPL (UHCI_TPL);

  Qh = UhciCreateQh (Uhc, PollingInterval);

  if (Qh == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_DATA;
  }

  IntTds = UhciCreateBulkOrIntTds (
             Uhc,
             DeviceAddress,
             EndPointAddress,
             PktId,
             DataPtr,
             DataPhy,
             DataLength,
             DataToggle,
             (UINT8) MaximumPacketLength,
             IsSlowDevice
             );

  if (IntTds == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto DESTORY_QH;
  }

  UhciLinkTdToQh (Uhc, Qh, IntTds);

  //
  // Save QH-TD structures to async Interrupt transfer list,
  // for monitor interrupt transfer execution routine use.
  //
  Status = UhciCreateAsyncReq (
             Uhc,
             Qh,
             IntTds,
             DeviceAddress,
             EndPointAddress,
             DataLength,
             PollingInterval,
             DataPtr,
             CallBackFunction,
             Context,
             IsSlowDevice
             );

  if (EFI_ERROR (Status)) {
    goto DESTORY_QH;
  }

  UhciLinkQhToFrameList (Uhc, Qh);

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

DESTORY_QH:
  UsbHcFreeMem (Uhc->MemPool, Qh, sizeof (UHCI_QH_SW));

FREE_DATA:
  UsbHcFreeMem (Uhc->MemPool, DataPtr, DataLength);
  Uhc->PciIo->Flush (Uhc->PciIo);

  gBS->RestoreTPL (OldTpl);
  return Status;
}

/**
  Submits synchronous interrupt transfer to an interrupt endpoint
  of a USB device according to UEFI 2.0 spec.


  @param  This                   A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress          Target device address.
  @param  EndPointAddress        Endpoint number and direction.
  @param  DeviceSpeed            Device speed.
  @param  MaximumPacketLength    Maximum packet size of the target endpoint.
  @param  Data                   Array of pointers to the buffers of data.
  @param  DataLength             On input, size of the data buffer, On output,
                                 actually transferred data size.
  @param  DataToggle             On input, data toggle to use; On output, next data toggle.
  @param  TimeOut                Maximum time out, in microseconds.
  @param  Translator             A pointr to the transaction translator data.
  @param  TransferResult         Variable to receive transfer result.

  @return EFI_SUCCESS            The transfer was completed successfully.
  @return EFI_OUT_OF_RESOURCES   Failed due to lack of resource.
  @return EFI_INVALID_PARAMETER  Some parameters are invalid.
  @return EFI_TIMEOUT            Failed due to timeout.
  @return EFI_DEVICE_ERROR       Failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
Uhci2SyncInterruptTransfer (
  IN     EFI_USB2_HC_PROTOCOL                      *This,
  IN     UINT8                                     DeviceAddress,
  IN     UINT8                                     EndPointAddress,
  IN     UINT8                                     DeviceSpeed,
  IN     UINTN                                     MaximumPacketLength,
  IN OUT VOID                                      *Data,
  IN OUT UINTN                                     *DataLength,
  IN OUT UINT8                                     *DataToggle,
  IN     UINTN                                     TimeOut,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR        *Translator,
  OUT    UINT32                                    *TransferResult
  )
{
  EFI_STATUS          Status;
  USB_HC_DEV          *Uhc;
  UHCI_TD_SW          *TDs;
  UHCI_QH_RESULT      QhResult;
  EFI_TPL             OldTpl;
  UINT8               *DataPhy;
  VOID                *DataMap;
  UINT8               PktId;
  BOOLEAN             IsSlowDevice;

  Uhc     = UHC_FROM_USB2_HC_PROTO (This);
  DataPhy = NULL;
  DataMap = NULL;
  TDs     = NULL;

  if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
    return EFI_INVALID_PARAMETER;
  }

  IsSlowDevice  = (BOOLEAN) ((EFI_USB_SPEED_LOW == DeviceSpeed) ? TRUE : FALSE);

  if ((DataLength == NULL) || (Data == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 1) && (*DataToggle != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataLength == 0) || (MaximumPacketLength > 64)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsSlowDevice && (MaximumPacketLength > 8)) {
    return EFI_INVALID_PARAMETER;
  }

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;


  UhciAckAllInterrupt (Uhc);

  if (!UhciIsHcWorking (Uhc->PciIo)) {
    return Status;
  }

  OldTpl = gBS->RaiseTPL (UHCI_TPL);

  //
  // Map the source data buffer for bus master access.
  // Create Tds list, then link it to the UHC's interrupt list
  //
  Status = UhciMapUserData (
             Uhc,
             EfiUsbDataIn,
             Data,
             DataLength,
             &PktId,
             &DataPhy,
             &DataMap
             );

  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  TDs = UhciCreateBulkOrIntTds (
          Uhc,
          DeviceAddress,
          EndPointAddress,
          PktId,
          (UINT8 *)Data,
          DataPhy,
          *DataLength,
          DataToggle,
          (UINT8) MaximumPacketLength,
          IsSlowDevice
          );

  if (TDs == NULL) {
    Uhc->PciIo->Unmap (Uhc->PciIo, DataMap);

    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }


  UhciLinkTdToQh (Uhc, Uhc->SyncIntQh, TDs);

  Status = UhciExecuteTransfer (Uhc, Uhc->SyncIntQh, TDs, TimeOut, IsSlowDevice, &QhResult);

  UhciUnlinkTdFromQh (Uhc->SyncIntQh, TDs);
  Uhc->PciIo->Flush (Uhc->PciIo);

  *TransferResult = QhResult.Result;
  *DataToggle     = QhResult.NextToggle;
  *DataLength     = QhResult.Complete;

  UhciDestoryTds (Uhc, TDs);
  Uhc->PciIo->Unmap (Uhc->PciIo, DataMap);

ON_EXIT:
  gBS->RestoreTPL (OldTpl);
  return Status;
}


/**
  Submits isochronous transfer to a target USB device according to UEFI 2.0 spec.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and direction.
  @param  DeviceSpeed           Device speed.
  @param  MaximumPacketLength   Maximum packet size of the target endpoint.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data.
  @param  DataLength            On input, size of the data buffer, On output,
                                actually transferred data size.
  @param  Translator            A pointr to the transaction translator data.
  @param  TransferResult        Variable to receive transfer result.

  @return EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
Uhci2IsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL               *This,
  IN     UINT8                              DeviceAddress,
  IN     UINT8                              EndPointAddress,
  IN     UINT8                              DeviceSpeed,
  IN     UINTN                              MaximumPacketLength,
  IN     UINT8                              DataBuffersNumber,
  IN OUT VOID                               *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                              DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR *Translator,
  OUT    UINT32                             *TransferResult
  )
{
  return EFI_UNSUPPORTED;
}


/**
  Submits Async isochronous transfer to a target USB device according to UEFI 2.0 spec.

  @param  This                  A pointer to the EFI_USB2_HC_PROTOCOL instance.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and direction.
  @param  DeviceSpeed           Device speed.
  @param  MaximumPacketLength   Maximum packet size of the target endpoint.
  @param  DataBuffersNumber     Number of data buffers prepared for the transfer.
  @param  Data                  Array of pointers to the buffers of data.
  @param  DataLength            On input, size of the data buffer, On output,
                                actually transferred data size.
  @param  Translator            A pointr to the transaction translator data.
  @param  IsochronousCallBack   Function to call when the transfer complete.
  @param  Context               Pass to the call back function as parameter.

  @return EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
Uhci2AsyncIsochronousTransfer (
  IN     EFI_USB2_HC_PROTOCOL                *This,
  IN     UINT8                               DeviceAddress,
  IN     UINT8                               EndPointAddress,
  IN     UINT8                               DeviceSpeed,
  IN     UINTN                               MaximumPacketLength,
  IN     UINT8                               DataBuffersNumber,
  IN OUT VOID                                *Data[EFI_USB_MAX_ISO_BUFFER_NUM],
  IN     UINTN                               DataLength,
  IN     EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN     EFI_ASYNC_USB_TRANSFER_CALLBACK     IsochronousCallBack,
  IN     VOID                                *Context
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Entry point for EFI drivers.

  @param  ImageHandle      EFI_HANDLE.
  @param  SystemTable      EFI_SYSTEM_TABLE.

  @retval EFI_SUCCESS      Driver is successfully loaded.
  @return Others           Failed.

**/
EFI_STATUS
EFIAPI
UhciDriverEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gUhciDriverBinding,
           ImageHandle,
           &gUhciComponentName,
           &gUhciComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. Any
  ControllerHandle that has UsbHcProtocol installed will be supported.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @return EFI_SUCCESS          This driver supports this device.
  @return EFI_UNSUPPORTED      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
UhciDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS            OpenStatus;
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  USB_CLASSC            UsbClassCReg;

  //
  // Test whether there is PCI IO Protocol attached on the controller handle.
  //
  OpenStatus = gBS->OpenProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      (VOID **) &PciIo,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

  if (EFI_ERROR (OpenStatus)) {
    return OpenStatus;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (USB_CLASSC) / sizeof (UINT8),
                        &UsbClassCReg
                        );

  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  //
  // Test whether the controller belongs to UHCI type
  //
  if ((UsbClassCReg.BaseCode != PCI_CLASS_SERIAL) ||
      (UsbClassCReg.SubClassCode != PCI_CLASS_SERIAL_USB) ||
      (UsbClassCReg.ProgInterface != PCI_IF_UHCI)
      ) {

    Status = EFI_UNSUPPORTED;
  }

ON_EXIT:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;

}


/**
  Allocate and initialize the empty UHCI device.

  @param  PciIo                  The PCIIO to use.
  @param  DevicePath             The device path of host controller.
  @param  OriginalPciAttributes  The original PCI attributes.

  @return Allocated UHCI device. If err, return NULL.

**/
USB_HC_DEV *
UhciAllocateDev (
  IN EFI_PCI_IO_PROTOCOL       *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN UINT64                    OriginalPciAttributes
  )
{
  USB_HC_DEV  *Uhc;
  EFI_STATUS  Status;

  Uhc = AllocateZeroPool (sizeof (USB_HC_DEV));

  if (Uhc == NULL) {
    return NULL;
  }

  //
  // This driver supports both USB_HC_PROTOCOL and USB2_HC_PROTOCOL.
  // USB_HC_PROTOCOL is for EFI 1.1 backward compability.
  //
  Uhc->Signature                        = USB_HC_DEV_SIGNATURE;
  Uhc->Usb2Hc.GetCapability             = Uhci2GetCapability;
  Uhc->Usb2Hc.Reset                     = Uhci2Reset;
  Uhc->Usb2Hc.GetState                  = Uhci2GetState;
  Uhc->Usb2Hc.SetState                  = Uhci2SetState;
  Uhc->Usb2Hc.ControlTransfer           = Uhci2ControlTransfer;
  Uhc->Usb2Hc.BulkTransfer              = Uhci2BulkTransfer;
  Uhc->Usb2Hc.AsyncInterruptTransfer    = Uhci2AsyncInterruptTransfer;
  Uhc->Usb2Hc.SyncInterruptTransfer     = Uhci2SyncInterruptTransfer;
  Uhc->Usb2Hc.IsochronousTransfer       = Uhci2IsochronousTransfer;
  Uhc->Usb2Hc.AsyncIsochronousTransfer  = Uhci2AsyncIsochronousTransfer;
  Uhc->Usb2Hc.GetRootHubPortStatus      = Uhci2GetRootHubPortStatus;
  Uhc->Usb2Hc.SetRootHubPortFeature     = Uhci2SetRootHubPortFeature;
  Uhc->Usb2Hc.ClearRootHubPortFeature   = Uhci2ClearRootHubPortFeature;
  Uhc->Usb2Hc.MajorRevision             = 0x1;
  Uhc->Usb2Hc.MinorRevision             = 0x1;

  Uhc->PciIo                 = PciIo;
  Uhc->DevicePath            = DevicePath;
  Uhc->OriginalPciAttributes = OriginalPciAttributes;
  Uhc->MemPool               = UsbHcInitMemPool (PciIo, TRUE, 0);

  if (Uhc->MemPool == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  InitializeListHead (&Uhc->AsyncIntList);

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UhciMonitorAsyncReqList,
                  Uhc,
                  &Uhc->AsyncIntMonitor
                  );

  if (EFI_ERROR (Status)) {
    UsbHcFreeMemPool (Uhc->MemPool);
    goto ON_ERROR;
  }

  return Uhc;

ON_ERROR:
  FreePool (Uhc);
  return NULL;
}


/**
  Free the UHCI device and release its associated resources.

  @param  Uhc     The UHCI device to release.

**/
VOID
UhciFreeDev (
  IN USB_HC_DEV           *Uhc
  )
{
  if (Uhc->AsyncIntMonitor != NULL) {
    gBS->CloseEvent (Uhc->AsyncIntMonitor);
  }

  if (Uhc->ExitBootServiceEvent != NULL) {
    gBS->CloseEvent (Uhc->ExitBootServiceEvent);
  }

  if (Uhc->MemPool != NULL) {
    UsbHcFreeMemPool (Uhc->MemPool);
  }

  if (Uhc->CtrlNameTable != NULL) {
    FreeUnicodeStringTable (Uhc->CtrlNameTable);
  }

  FreePool (Uhc);
}


/**
  Uninstall all Uhci Interface.

  @param  Controller           Controller handle.
  @param  This                 Protocol instance pointer.

**/
VOID
UhciCleanDevUp (
  IN  EFI_HANDLE           Controller,
  IN  EFI_USB2_HC_PROTOCOL *This
  )
{
  USB_HC_DEV          *Uhc;
  EFI_STATUS          Status;

  //
  // Uninstall the USB_HC and USB_HC2 protocol, then disable the controller
  //
  Uhc = UHC_FROM_USB2_HC_PROTO (This);


  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  &Uhc->Usb2Hc
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);
  UhciFreeAllAsyncReq (Uhc);
  UhciDestoryFrameList (Uhc);

  //
  // Restore original PCI attributes
  //
  Uhc->PciIo->Attributes (
                  Uhc->PciIo,
                  EfiPciIoAttributeOperationSet,
                  Uhc->OriginalPciAttributes,
                  NULL
                  );

  UhciFreeDev (Uhc);
}

/**
  One notified function to stop the Host Controller when gBS->ExitBootServices() called.

  @param  Event                   Pointer to this event
  @param  Context                 Event handler private data

**/
VOID
EFIAPI
UhcExitBootService (
  EFI_EVENT                      Event,
  VOID                           *Context
  )
{
  USB_HC_DEV   *Uhc;

  Uhc = (USB_HC_DEV *) Context;

  //
  // Stop the Host Controller
  //
  UhciStopHc (Uhc, UHC_GENERIC_TIMEOUT);

  //
  // Reset the Host Controller
  //
  UhciSetRegBit (Uhc->PciIo, USBCMD_OFFSET, USBCMD_HCRESET);
  gBS->Stall (UHC_ROOT_PORT_RECOVERY_STALL);
}

/**
  Starting the Usb UHCI Driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to test.
  @param  RemainingDevicePath  Not used.

  @retval EFI_SUCCESS          This driver supports this device.
  @retval EFI_UNSUPPORTED      This driver does not support this device.
  @retval EFI_DEVICE_ERROR     This driver cannot be started due to device Error.
                               EFI_OUT_OF_RESOURCES- Failed due to resource shortage.

**/
EFI_STATUS
EFIAPI
UhciDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  USB_HC_DEV          *Uhc;
  UINT64              Supports;
  UINT64              OriginalPciAttributes;
  BOOLEAN             PciAttributesSaved;
  EFI_DEVICE_PATH_PROTOCOL  *HcDevicePath;

  //
  // Open PCIIO, then enable the EHC device and turn off emulation
  //
  Uhc = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open Device Path Protocol for on USB host controller
  //
  HcDevicePath = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &HcDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  PciAttributesSaved = FALSE;
  //
  // Save original PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &OriginalPciAttributes
                    );

  if (EFI_ERROR (Status)) {
    goto CLOSE_PCIIO;
  }
  PciAttributesSaved = TRUE;

  //
  // Robustnesss improvement such as for UoL
  // Default is not required.
  //
  if (FeaturePcdGet (PcdTurnOffUsbLegacySupport)) {
    UhciTurnOffUsbEmulation (PciIo);
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (!EFI_ERROR (Status)) {
    Supports &= (UINT64)EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes (
                      PciIo,
                      EfiPciIoAttributeOperationEnable,
                      Supports,
                      NULL
                      );
  }

  if (EFI_ERROR (Status)) {
    goto CLOSE_PCIIO;
  }

  Uhc = UhciAllocateDev (PciIo, HcDevicePath, OriginalPciAttributes);

  if (Uhc == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto CLOSE_PCIIO;
  }

  //
  // Allocate and Init Host Controller's Frame List Entry
  //
  Status = UhciInitFrameList (Uhc);

  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_UHC;
  }

  Status = gBS->SetTimer (
                  Uhc->AsyncIntMonitor,
                  TimerPeriodic,
                  UHC_ASYNC_POLL_INTERVAL
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_UHC;
  }

  //
  // Install USB2_HC_PROTOCOL
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiUsb2HcProtocolGuid,
                  &Uhc->Usb2Hc,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto FREE_UHC;
  }

  //
  // Create event to stop the HC when exit boot service.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  UhcExitBootService,
                  Uhc,
                  &gEfiEventExitBootServicesGuid,
                  &Uhc->ExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_USBHC;
  }

  //
  // Install the component name protocol
  //
  Uhc->CtrlNameTable = NULL;

  AddUnicodeString2 (
    "eng",
    gUhciComponentName.SupportedLanguages,
    &Uhc->CtrlNameTable,
    L"Usb Universal Host Controller",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUhciComponentName2.SupportedLanguages,
    &Uhc->CtrlNameTable,
    L"Usb Universal Host Controller",
    FALSE
    );


  //
  // Start the UHCI hardware, also set its reclamation point to 64 bytes
  //
  UhciWriteReg (Uhc->PciIo, USBCMD_OFFSET, USBCMD_RS | USBCMD_MAXP);

  return EFI_SUCCESS;

UNINSTALL_USBHC:
  gBS->UninstallMultipleProtocolInterfaces (
         Controller,
         &gEfiUsb2HcProtocolGuid,
         &Uhc->Usb2Hc,
         NULL
         );

FREE_UHC:
  UhciFreeDev (Uhc);

CLOSE_PCIIO:
  if (PciAttributesSaved) {
    //
    // Restore original PCI attributes
    //
    PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSet,
                    OriginalPciAttributes,
                    NULL
                    );
  }

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return Status;
}


/**
  Stop this driver on ControllerHandle. Support stopping any child handles
  created by this driver.

  @param  This                 Protocol instance pointer.
  @param  Controller           Handle of device to stop driver on.
  @param  NumberOfChildren     Number of Children in the ChildHandleBuffer.
  @param  ChildHandleBuffer    List of handles for the children we need to stop.

  @return EFI_SUCCESS
  @return others

**/
EFI_STATUS
EFIAPI
UhciDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      Controller,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_USB2_HC_PROTOCOL  *Usb2Hc;
  EFI_STATUS            Status;

   Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsb2HcProtocolGuid,
                  (VOID **) &Usb2Hc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  //
  // Test whether the Controller handler passed in is a valid
  // Usb controller handle that should be supported, if not,
  // return the error status directly
  //
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UhciCleanDevUp (Controller, Usb2Hc);

  gBS->CloseProtocol (
        Controller,
        &gEfiPciIoProtocolGuid,
        This->DriverBindingHandle,
        Controller
        );

  return EFI_SUCCESS;
}

