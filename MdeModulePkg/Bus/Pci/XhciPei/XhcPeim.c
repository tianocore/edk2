/** @file
PEIM to produce gPeiUsb2HostControllerPpiGuid based on gPeiUsbControllerPpiGuid
which is used to enable recovery function from USB Drivers.

Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "XhcPeim.h"

//
// Two arrays used to translate the XHCI port state (change)
// to the UEFI protocol's port state (change).
//
USB_PORT_STATE_MAP  mUsbPortStateMap[] = {
  {XHC_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {XHC_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {XHC_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {XHC_PORTSC_PP,    USB_PORT_STAT_POWER},
  {XHC_PORTSC_RESET, USB_PORT_STAT_RESET}
};

USB_PORT_STATE_MAP  mUsbPortChangeMap[] = {
  {XHC_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {XHC_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {XHC_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {XHC_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

USB_CLEAR_PORT_MAP mUsbClearPortChangeMap[] = {
  {XHC_PORTSC_CSC, EfiUsbPortConnectChange},
  {XHC_PORTSC_PEC, EfiUsbPortEnableChange},
  {XHC_PORTSC_OCC, EfiUsbPortOverCurrentChange},
  {XHC_PORTSC_PRC, EfiUsbPortResetChange}
};

USB_PORT_STATE_MAP  mUsbHubPortStateMap[] = {
  {XHC_HUB_PORTSC_CCS,   USB_PORT_STAT_CONNECTION},
  {XHC_HUB_PORTSC_PED,   USB_PORT_STAT_ENABLE},
  {XHC_HUB_PORTSC_OCA,   USB_PORT_STAT_OVERCURRENT},
  {XHC_HUB_PORTSC_PP,    USB_PORT_STAT_POWER},
  {XHC_HUB_PORTSC_RESET, USB_PORT_STAT_RESET}
};

USB_PORT_STATE_MAP  mUsbHubPortChangeMap[] = {
  {XHC_HUB_PORTSC_CSC, USB_PORT_STAT_C_CONNECTION},
  {XHC_HUB_PORTSC_PEC, USB_PORT_STAT_C_ENABLE},
  {XHC_HUB_PORTSC_OCC, USB_PORT_STAT_C_OVERCURRENT},
  {XHC_HUB_PORTSC_PRC, USB_PORT_STAT_C_RESET}
};

USB_CLEAR_PORT_MAP mUsbHubClearPortChangeMap[] = {
  {XHC_HUB_PORTSC_CSC, EfiUsbPortConnectChange},
  {XHC_HUB_PORTSC_PEC, EfiUsbPortEnableChange},
  {XHC_HUB_PORTSC_OCC, EfiUsbPortOverCurrentChange},
  {XHC_HUB_PORTSC_PRC, EfiUsbPortResetChange},
  {XHC_HUB_PORTSC_BHRC, Usb3PortBHPortResetChange}
};

/**
  Read XHCI Operation register.

  @param Xhc            The XHCI device.
  @param Offset         The operation register offset.

  @retval the register content read.

**/
UINT32
XhcPeiReadOpReg (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset
  )
{
  UINT32                Data;

  ASSERT (Xhc->CapLength != 0);

  Data = MmioRead32 (Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset);
  return Data;
}

/**
  Write the data to the XHCI operation register.

  @param Xhc            The XHCI device.
  @param Offset         The operation register offset.
  @param Data           The data to write.

**/
VOID
XhcPeiWriteOpReg (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  ASSERT (Xhc->CapLength != 0);

  MmioWrite32 (Xhc->UsbHostControllerBaseAddress + Xhc->CapLength + Offset, Data);
}

/**
  Set one bit of the operational register while keeping other bits.

  @param  Xhc           The XHCI device.
  @param  Offset        The offset of the operational register.
  @param  Bit           The bit mask of the register to set.

**/
VOID
XhcPeiSetOpRegBit (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32                Data;

  Data  = XhcPeiReadOpReg (Xhc, Offset);
  Data |= Bit;
  XhcPeiWriteOpReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the operational register while keeping other bits.

  @param  Xhc           The XHCI device.
  @param  Offset        The offset of the operational register.
  @param  Bit           The bit mask of the register to clear.

**/
VOID
XhcPeiClearOpRegBit (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32                Data;

  Data  = XhcPeiReadOpReg (Xhc, Offset);
  Data &= ~Bit;
  XhcPeiWriteOpReg (Xhc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit
  to become set (or clear).

  @param  Xhc           The XHCI device.
  @param  Offset        The offset of the operational register.
  @param  Bit           The bit mask of the register to wait for.
  @param  WaitToSet     Wait the bit to set or clear.
  @param  Timeout       The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS   The bit successfully changed by host controller.
  @retval EFI_TIMEOUT   The time out occurred.

**/
EFI_STATUS
XhcPeiWaitOpRegBit (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit,
  IN BOOLEAN            WaitToSet,
  IN UINT32             Timeout
  )
{
  UINT64                Index;

  for (Index = 0; Index < Timeout * XHC_1_MILLISECOND; Index++) {
    if (XHC_REG_BIT_IS_SET (Xhc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    MicroSecondDelay (XHC_1_MICROSECOND);
  }

  return EFI_TIMEOUT;
}

/**
  Read XHCI capability register.

  @param Xhc        The XHCI device.
  @param Offset     Capability register address.

  @retval the register content read.

**/
UINT32
XhcPeiReadCapRegister (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset
  )
{
  UINT32                Data;

  Data = MmioRead32 (Xhc->UsbHostControllerBaseAddress + Offset);

  return Data;
}



/**
  Write the data to the XHCI door bell register.

  @param  Xhc           The XHCI device.
  @param  Offset        The offset of the door bell register.
  @param  Data          The data to write.

**/
VOID
XhcPeiWriteDoorBellReg (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  ASSERT (Xhc->DBOff != 0);

  MmioWrite32 (Xhc->UsbHostControllerBaseAddress + Xhc->DBOff + Offset, Data);
}

/**
  Read XHCI runtime register.

  @param  Xhc           The XHCI device.
  @param  Offset        The offset of the runtime register.

  @return The register content read

**/
UINT32
XhcPeiReadRuntimeReg (
  IN  PEI_XHC_DEV       *Xhc,
  IN  UINT32            Offset
  )
{
  UINT32                Data;

  ASSERT (Xhc->RTSOff != 0);

  Data = MmioRead32 (Xhc->UsbHostControllerBaseAddress + Xhc->RTSOff + Offset);

  return Data;
}

/**
  Write the data to the XHCI runtime register.

  @param  Xhc       The XHCI device.
  @param  Offset    The offset of the runtime register.
  @param  Data      The data to write.

**/
VOID
XhcPeiWriteRuntimeReg (
  IN PEI_XHC_DEV          *Xhc,
  IN UINT32               Offset,
  IN UINT32               Data
  )
{
  ASSERT (Xhc->RTSOff != 0);

  MmioWrite32 (Xhc->UsbHostControllerBaseAddress + Xhc->RTSOff + Offset, Data);
}

/**
  Set one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcPeiSetRuntimeRegBit (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32                Data;

  Data  = XhcPeiReadRuntimeReg (Xhc, Offset);
  Data |= Bit;
  XhcPeiWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI device.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcPeiClearRuntimeRegBit (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32                Data;

  Data  = XhcPeiReadRuntimeReg (Xhc, Offset);
  Data &= ~Bit;
  XhcPeiWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Check whether Xhc is halted.

  @param  Xhc           The XHCI device.

  @retval TRUE          The controller is halted.
  @retval FALSE         The controller isn't halted.

**/
BOOLEAN
XhcPeiIsHalt (
  IN PEI_XHC_DEV        *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT);
}

/**
  Check whether system error occurred.

  @param  Xhc           The XHCI device.

  @retval TRUE          System error happened.
  @retval FALSE         No system error.

**/
BOOLEAN
XhcPeiIsSysError (
  IN PEI_XHC_DEV        *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE);
}

/**
  Reset the host controller.

  @param  Xhc           The XHCI device.
  @param  Timeout       Time to wait before abort (in millisecond, ms).

  @retval EFI_TIMEOUT   The transfer failed due to time out.
  @retval Others        Failed to reset the host.

**/
EFI_STATUS
XhcPeiResetHC (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS            Status;

  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!XhcPeiIsHalt (Xhc)) {
    Status = XhcPeiHaltHC (Xhc, Timeout);

    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  XhcPeiSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET);
  //
  // Some XHCI host controllers require to have extra 1ms delay before accessing any MMIO register during reset.
  // Otherwise there may have the timeout case happened.
  // The below is a workaround to solve such problem.
  //
  MicroSecondDelay (1000);
  Status = XhcPeiWaitOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET, FALSE, Timeout);
ON_EXIT:
  DEBUG ((EFI_D_INFO, "XhcPeiResetHC: %r\n", Status));
  return Status;
}

/**
  Halt the host controller.

  @param  Xhc           The XHCI device.
  @param  Timeout       Time to wait before abort.

  @retval EFI_TIMEOUT   Failed to halt the controller before Timeout.
  @retval EFI_SUCCESS   The XHCI is halt.

**/
EFI_STATUS
XhcPeiHaltHC (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS            Status;

  XhcPeiClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcPeiWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, TRUE, Timeout);
  DEBUG ((EFI_D_INFO, "XhcPeiHaltHC: %r\n", Status));
  return Status;
}

/**
  Set the XHCI to run.

  @param  Xhc           The XHCI device.
  @param  Timeout       Time to wait before abort.

  @retval EFI_SUCCESS   The XHCI is running.
  @retval Others        Failed to set the XHCI to run.

**/
EFI_STATUS
XhcPeiRunHC (
  IN PEI_XHC_DEV        *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS            Status;

  XhcPeiSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcPeiWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, FALSE, Timeout);
  DEBUG ((EFI_D_INFO, "XhcPeiRunHC: %r\n", Status));
  return Status;
}

/**
  Submits control transfer to a target USB device.

  @param  PeiServices               The pointer of EFI_PEI_SERVICES.
  @param  This                      The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  DeviceAddress             The target device address.
  @param  DeviceSpeed               Target device speed.
  @param  MaximumPacketLength       Maximum packet size the default control transfer
                                    endpoint is capable of sending or receiving.
  @param  Request                   USB device request to send.
  @param  TransferDirection         Specifies the data direction for the data stage.
  @param  Data                      Data buffer to be transmitted or received from USB device.
  @param  DataLength                The size (in bytes) of the data buffer.
  @param  TimeOut                   Indicates the maximum timeout, in millisecond.
                                    If Timeout is 0, then the caller must wait for the function
                                    to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Translator                Transaction translator to be used by this device.
  @param  TransferResult            Return the result of this control transfer.

  @retval EFI_SUCCESS               Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES      The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER     Some parameters are invalid.
  @retval EFI_TIMEOUT               Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR          Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
XhcPeiControlTransfer (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  IN UINT8                                  DeviceAddress,
  IN UINT8                                  DeviceSpeed,
  IN UINTN                                  MaximumPacketLength,
  IN EFI_USB_DEVICE_REQUEST                 *Request,
  IN EFI_USB_DATA_DIRECTION                 TransferDirection,
  IN OUT VOID                               *Data,
  IN OUT UINTN                              *DataLength,
  IN UINTN                                  TimeOut,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR     *Translator,
  OUT UINT32                                *TransferResult
  )
{
  PEI_XHC_DEV                   *Xhc;
  URB                           *Urb;
  UINT8                         Endpoint;
  UINT8                         Index;
  UINT8                         DescriptorType;
  UINT8                         SlotId;
  UINT8                         TTT;
  UINT8                         MTT;
  UINT32                        MaxPacket0;
  EFI_USB_HUB_DESCRIPTOR        *HubDesc;
  EFI_STATUS                    Status;
  EFI_STATUS                    RecoveryStatus;
  UINTN                         MapSize;
  EFI_USB_PORT_STATUS           PortStatus;
  UINT32                        State;
  EFI_USB_DEVICE_REQUEST        ClearPortRequest;
  UINTN                         Len;

  //
  // Validate parameters
  //
  if ((Request == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbDataIn) &&
      (TransferDirection != EfiUsbDataOut) &&
      (TransferDirection != EfiUsbNoData)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection == EfiUsbNoData) &&
      ((Data != NULL) || (*DataLength != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((TransferDirection != EfiUsbNoData) &&
     ((Data == NULL) || (*DataLength == 0))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MaximumPacketLength != 8)  && (MaximumPacketLength != 16) &&
      (MaximumPacketLength != 32) && (MaximumPacketLength != 64) &&
      (MaximumPacketLength != 512)
      ) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) && (MaximumPacketLength != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength != 512)) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);

  Status          = EFI_DEVICE_ERROR;
  *TransferResult = EFI_USB_ERR_SYSTEM;
  Len             = 0;

  if (XhcPeiIsHalt (Xhc) || XhcPeiIsSysError (Xhc)) {
    DEBUG ((EFI_D_ERROR, "XhcPeiControlTransfer: HC is halted or has system error\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcPeiBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Hook the Set_Address request from UsbBus.
  // According to XHCI 1.0 spec, the Set_Address request is replaced by XHCI's Address_Device cmd.
  //
  if ((Request->Request     == USB_REQ_SET_ADDRESS) &&
      (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Reset the BusDevAddr field of all disabled entries in UsbDevContext array firstly.
    // This way is used to clean the history to avoid using wrong device address afterwards.
    //
    for (Index = 0; Index < 255; Index++) {
      if (!Xhc->UsbDevContext[Index + 1].Enabled &&
          (Xhc->UsbDevContext[Index + 1].SlotId == 0) &&
          (Xhc->UsbDevContext[Index + 1].BusDevAddr == (UINT8) Request->Value)) {
        Xhc->UsbDevContext[Index + 1].BusDevAddr = 0;
      }
    }

    if (Xhc->UsbDevContext[SlotId].XhciDevAddr == 0) {
      goto ON_EXIT;
    }
    //
    // The actual device address has been assigned by XHCI during initializing the device slot.
    // So we just need establish the mapping relationship between the device address requested from UsbBus
    // and the actual device address assigned by XHCI. The following invocations through EFI_USB2_HC_PROTOCOL interface
    // can find out the actual device address by it.
    //
    Xhc->UsbDevContext[SlotId].BusDevAddr = (UINT8) Request->Value;
    Status = EFI_SUCCESS;
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  // Note that we encode the direction in address although default control
  // endpoint is bidirectional. XhcPeiCreateUrb expects this
  // combination of Ep addr and its direction.
  //
  Endpoint = (UINT8) (0 | ((TransferDirection == EfiUsbDataIn) ? 0x80 : 0));
  Urb = XhcPeiCreateUrb (
          Xhc,
          DeviceAddress,
          Endpoint,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_CTRL_TRANSFER,
          Request,
          Data,
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "XhcPeiControlTransfer: failed to create URB"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcPeiExecTransfer (Xhc, FALSE, Urb, TimeOut);

  //
  // Get the status from URB. The result is updated in XhcPeiCheckUrbResult
  // which is called by XhcPeiExecTransfer
  //
  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (Status == EFI_TIMEOUT) {
    //
    // The transfer timed out. Abort the transfer by dequeueing of the TD.
    //
    RecoveryStatus = XhcPeiDequeueTrbFromEndpoint(Xhc, Urb);
    if (EFI_ERROR(RecoveryStatus)) {
      DEBUG((EFI_D_ERROR, "XhcPeiControlTransfer: XhcPeiDequeueTrbFromEndpoint failed\n"));
    }
    XhcPeiFreeUrb (Xhc, Urb);
    goto ON_EXIT;
  } else {
    if (*TransferResult == EFI_USB_NOERROR) {
      Status = EFI_SUCCESS;
    } else if ((*TransferResult == EFI_USB_ERR_STALL) || (*TransferResult == EFI_USB_ERR_BABBLE)) {
      RecoveryStatus = XhcPeiRecoverHaltedEndpoint(Xhc, Urb);
      if (EFI_ERROR (RecoveryStatus)) {
        DEBUG ((EFI_D_ERROR, "XhcPeiControlTransfer: XhcPeiRecoverHaltedEndpoint failed\n"));
      }
      Status = EFI_DEVICE_ERROR;
      XhcPeiFreeUrb (Xhc, Urb);
      goto ON_EXIT;
    } else {
      XhcPeiFreeUrb (Xhc, Urb);
      goto ON_EXIT;
    }
  }
  //
  // Unmap data before consume.
  //
  XhcPeiFreeUrb (Xhc, Urb);

  //
  // Hook Get_Descriptor request from UsbBus as we need evaluate context and configure endpoint.
  // Hook Get_Status request form UsbBus as we need trace device attach/detach event happened at hub.
  // Hook Set_Config request from UsbBus as we need configure device endpoint.
  //
  if ((Request->Request     == USB_REQ_GET_DESCRIPTOR) &&
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE)) ||
      ((Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_DEVICE))))) {
    DescriptorType = (UINT8) (Request->Value >> 8);
    if ((DescriptorType == USB_DESC_TYPE_DEVICE) && ((*DataLength == sizeof (EFI_USB_DEVICE_DESCRIPTOR)) || ((DeviceSpeed == EFI_USB_SPEED_FULL) && (*DataLength == 8)))) {
      ASSERT (Data != NULL);
      //
      // Store a copy of device scriptor as hub device need this info to configure endpoint.
      //
      CopyMem (&Xhc->UsbDevContext[SlotId].DevDesc, Data, *DataLength);
      if (Xhc->UsbDevContext[SlotId].DevDesc.BcdUSB >= 0x0300) {
        //
        // If it's a usb3.0 device, then its max packet size is a 2^n.
        //
        MaxPacket0 = 1 << Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
      } else {
        MaxPacket0 = Xhc->UsbDevContext[SlotId].DevDesc.MaxPacketSize0;
      }
      Xhc->UsbDevContext[SlotId].ConfDesc = AllocateZeroPool (Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations * sizeof (EFI_USB_CONFIG_DESCRIPTOR *));
      if (Xhc->UsbDevContext[SlotId].ConfDesc == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto ON_EXIT;
      }
      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcPeiEvaluateContext (Xhc, SlotId, MaxPacket0);
      } else {
        Status = XhcPeiEvaluateContext64 (Xhc, SlotId, MaxPacket0);
      }
    } else if (DescriptorType == USB_DESC_TYPE_CONFIG) {
      ASSERT (Data != NULL);
      if (*DataLength == ((UINT16 *) Data)[1]) {
        //
        // Get configuration value from request, store the configuration descriptor for Configure_Endpoint cmd.
        //
        Index = (UINT8) Request->Value;
        ASSERT (Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations);
        Xhc->UsbDevContext[SlotId].ConfDesc[Index] = AllocateZeroPool (*DataLength);
        if (Xhc->UsbDevContext[SlotId].ConfDesc[Index] == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto ON_EXIT;
        }
        CopyMem (Xhc->UsbDevContext[SlotId].ConfDesc[Index], Data, *DataLength);
      }
    } else if (((DescriptorType == USB_DESC_TYPE_HUB) ||
               (DescriptorType == USB_DESC_TYPE_HUB_SUPER_SPEED)) && (*DataLength > 2)) {
      ASSERT (Data != NULL);
      HubDesc = (EFI_USB_HUB_DESCRIPTOR *) Data;
      ASSERT (HubDesc->NumPorts <= 15);
      //
      // The bit 5,6 of HubCharacter field of Hub Descriptor is TTT.
      //
      TTT = (UINT8) ((HubDesc->HubCharacter & (BIT5 | BIT6)) >> 5);
      if (Xhc->UsbDevContext[SlotId].DevDesc.DeviceProtocol == 2) {
        //
        // Don't support multi-TT feature for super speed hub now.
        //
        MTT = 0;
        DEBUG ((EFI_D_ERROR, "XHCI: Don't support multi-TT feature for Hub now. (force to disable MTT)\n"));
      } else {
        MTT = 0;
      }

      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcPeiConfigHubContext (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      } else {
        Status = XhcPeiConfigHubContext64 (Xhc, SlotId, HubDesc->NumPorts, TTT, MTT);
      }
    }
  } else if ((Request->Request     == USB_REQ_SET_CONFIG) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, USB_TARGET_DEVICE))) {
    //
    // Hook Set_Config request from UsbBus as we need configure device endpoint.
    //
    for (Index = 0; Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
      if (Xhc->UsbDevContext[SlotId].ConfDesc[Index]->ConfigurationValue == (UINT8)Request->Value) {
        if (Xhc->HcCParams.Data.Csz == 0) {
          Status = XhcPeiSetConfigCmd (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        } else {
          Status = XhcPeiSetConfigCmd64 (Xhc, SlotId, DeviceSpeed, Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
        }
        break;
      }
    }
  } else if ((Request->Request     == USB_REQ_GET_STATUS) &&
             (Request->RequestType == USB_REQUEST_TYPE (EfiUsbDataIn, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER))) {
    ASSERT (Data != NULL);
    //
    // Hook Get_Status request from UsbBus to keep track of the port status change.
    //
    State                       = *(UINT32 *) Data;
    PortStatus.PortStatus       = 0;
    PortStatus.PortChangeStatus = 0;

    if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
      //
      // For super speed hub, its bit10~12 presents the attached device speed.
      //
      if ((State & XHC_PORTSC_PS) >> 10 == 0) {
        PortStatus.PortStatus |= USB_PORT_STAT_SUPER_SPEED;
      }
    } else {
      //
      // For high or full/low speed hub, its bit9~10 presents the attached device speed.
      //
      if (XHC_BIT_IS_SET (State, BIT9)) {
        PortStatus.PortStatus |= USB_PORT_STAT_LOW_SPEED;
      } else if (XHC_BIT_IS_SET (State, BIT10)) {
        PortStatus.PortStatus |= USB_PORT_STAT_HIGH_SPEED;
      }
    }

    //
    // Convert the XHCI port/port change state to UEFI status
    //
    MapSize = sizeof (mUsbHubPortStateMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortStateMap[Index].HwState)) {
        PortStatus.PortStatus = (UINT16) (PortStatus.PortStatus | mUsbHubPortStateMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubPortChangeMap) / sizeof (USB_PORT_STATE_MAP);
    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubPortChangeMap[Index].HwState)) {
        PortStatus.PortChangeStatus = (UINT16) (PortStatus.PortChangeStatus | mUsbHubPortChangeMap[Index].UefiState);
      }
    }

    MapSize = sizeof (mUsbHubClearPortChangeMap) / sizeof (USB_CLEAR_PORT_MAP);

    for (Index = 0; Index < MapSize; Index++) {
      if (XHC_BIT_IS_SET (State, mUsbHubClearPortChangeMap[Index].HwState)) {
        ZeroMem (&ClearPortRequest, sizeof (EFI_USB_DEVICE_REQUEST));
        ClearPortRequest.RequestType  = USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_CLASS, USB_TARGET_OTHER);
        ClearPortRequest.Request      = (UINT8) USB_REQ_CLEAR_FEATURE;
        ClearPortRequest.Value        = mUsbHubClearPortChangeMap[Index].Selector;
        ClearPortRequest.Index        = Request->Index;
        ClearPortRequest.Length       = 0;

        XhcPeiControlTransfer (
          PeiServices,
          This,
          DeviceAddress,
          DeviceSpeed,
          MaximumPacketLength,
          &ClearPortRequest,
          EfiUsbNoData,
          NULL,
          &Len,
          TimeOut,
          Translator,
          TransferResult
          );
      }
    }

    XhcPeiPollPortStatusChange (Xhc, Xhc->UsbDevContext[SlotId].RouteString, (UINT8)Request->Index, &PortStatus);

    *(UINT32 *) Data = *(UINT32 *) &PortStatus;
  }

ON_EXIT:

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcPeiControlTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  DeviceAddress         Target device address.
  @param  EndPointAddress       Endpoint number and its direction in bit 7.
  @param  DeviceSpeed           Device speed, Low speed device doesn't support
                                bulk transfer.
  @param  MaximumPacketLength   Maximum packet size the endpoint is capable of
                                sending or receiving.
  @param  Data                  Array of pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  DataToggle            On input, the initial data toggle for the transfer;
                                On output, it is updated to to next data toggle to use of
                                the subsequent bulk transfer.
  @param  TimeOut               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
                                If Timeout is 0, then the caller must wait for the function
                                to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Translator            A pointr to the transaction translator data.
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
XhcPeiBulkTransfer (
  IN EFI_PEI_SERVICES                       **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI           *This,
  IN UINT8                                  DeviceAddress,
  IN UINT8                                  EndPointAddress,
  IN UINT8                                  DeviceSpeed,
  IN UINTN                                  MaximumPacketLength,
  IN OUT VOID                               *Data[EFI_USB_MAX_BULK_BUFFER_NUM],
  IN OUT UINTN                              *DataLength,
  IN OUT UINT8                              *DataToggle,
  IN UINTN                                  TimeOut,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR     *Translator,
  OUT UINT32                                *TransferResult
  )
{
  PEI_XHC_DEV                   *Xhc;
  URB                           *Urb;
  UINT8                         SlotId;
  EFI_STATUS                    Status;
  EFI_STATUS                    RecoveryStatus;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) ||
      (Data == NULL) || (Data[0] == NULL) || (TransferResult == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*DataToggle != 0) && (*DataToggle != 1)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DeviceSpeed == EFI_USB_SPEED_LOW) ||
      ((DeviceSpeed == EFI_USB_SPEED_FULL) && (MaximumPacketLength > 64)) ||
      ((DeviceSpeed == EFI_USB_SPEED_HIGH) && (MaximumPacketLength > 512)) ||
      ((DeviceSpeed == EFI_USB_SPEED_SUPER) && (MaximumPacketLength > 1024))) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc             = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);

  *TransferResult = EFI_USB_ERR_SYSTEM;
  Status          = EFI_DEVICE_ERROR;

  if (XhcPeiIsHalt (Xhc) || XhcPeiIsSysError (Xhc)) {
    DEBUG ((EFI_D_ERROR, "XhcPeiBulkTransfer: HC is halted or has system error\n"));
    goto ON_EXIT;
  }

  //
  // Check if the device is still enabled before every transaction.
  //
  SlotId = XhcPeiBusDevAddrToSlotId (Xhc, DeviceAddress);
  if (SlotId == 0) {
    goto ON_EXIT;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = XhcPeiCreateUrb (
          Xhc,
          DeviceAddress,
          EndPointAddress,
          DeviceSpeed,
          MaximumPacketLength,
          XHC_BULK_TRANSFER,
          NULL,
          Data[0],
          *DataLength,
          NULL,
          NULL
          );

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "XhcPeiBulkTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status = XhcPeiExecTransfer (Xhc, FALSE, Urb, TimeOut);

  *TransferResult = Urb->Result;
  *DataLength     = Urb->Completed;

  if (Status == EFI_TIMEOUT) {
    //
    // The transfer timed out. Abort the transfer by dequeueing of the TD.
    //
    RecoveryStatus = XhcPeiDequeueTrbFromEndpoint(Xhc, Urb);
    if (EFI_ERROR(RecoveryStatus)) {
      DEBUG((EFI_D_ERROR, "XhcPeiBulkTransfer: XhcPeiDequeueTrbFromEndpoint failed\n"));
    }
  } else {
    if (*TransferResult == EFI_USB_NOERROR) {
      Status = EFI_SUCCESS;
    } else if ((*TransferResult == EFI_USB_ERR_STALL) || (*TransferResult == EFI_USB_ERR_BABBLE)) {
      RecoveryStatus = XhcPeiRecoverHaltedEndpoint(Xhc, Urb);
      if (EFI_ERROR (RecoveryStatus)) {
        DEBUG ((EFI_D_ERROR, "XhcPeiBulkTransfer: XhcPeiRecoverHaltedEndpoint failed\n"));
      }
      Status = EFI_DEVICE_ERROR;
    }
  }

  XhcPeiFreeUrb (Xhc, Urb);

ON_EXIT:

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XhcPeiBulkTransfer: error - %r, transfer - %x\n", Status, *TransferResult));
  }

  return Status;
}

/**
  Retrieves the number of root hub ports.

  @param[in]  PeiServices           The pointer to the PEI Services Table.
  @param[in]  This                  The pointer to this instance of the
                                    PEI_USB2_HOST_CONTROLLER_PPI.
  @param[out] PortNumber            The pointer to the number of the root hub ports.

  @retval EFI_SUCCESS               The port number was retrieved successfully.
  @retval EFI_INVALID_PARAMETER     PortNumber is NULL.

**/
EFI_STATUS
EFIAPI
XhcPeiGetRootHubPortNumber (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *This,
  OUT UINT8                         *PortNumber
  )
{
  PEI_XHC_DEV           *XhcDev;
  XhcDev = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);

  if (PortNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *PortNumber = XhcDev->HcSParams1.Data.MaxPorts;
  DEBUG ((EFI_D_INFO, "XhcPeiGetRootHubPortNumber: PortNumber = %x\n", *PortNumber));
  return EFI_SUCCESS;
}

/**
  Clears a feature for the specified root hub port.

  @param  PeiServices               The pointer of EFI_PEI_SERVICES.
  @param  This                      The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  PortNumber                Specifies the root hub port whose feature
                                    is requested to be cleared.
  @param  PortFeature               Indicates the feature selector associated with the
                                    feature clear request.

  @retval EFI_SUCCESS               The feature specified by PortFeature was cleared
                                    for the USB root hub port specified by PortNumber.
  @retval EFI_INVALID_PARAMETER     PortNumber is invalid or PortFeature is invalid.

**/
EFI_STATUS
EFIAPI
XhcPeiClearRootHubPortFeature (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *This,
  IN UINT8                          PortNumber,
  IN EFI_USB_PORT_FEATURE           PortFeature
  )
{
  PEI_XHC_DEV           *Xhc;
  UINT32                Offset;
  UINT32                State;
  EFI_STATUS            Status;

  Xhc = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);
  Status = EFI_SUCCESS;

  if (PortNumber >= Xhc->HcSParams1.Data.MaxPorts) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = (UINT32) (XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State = XhcPeiReadOpReg (Xhc, Offset);
  DEBUG ((EFI_D_INFO, "XhcPeiClearRootHubPortFeature: Port: %x State: %x\n", PortNumber, State));

  //
  // Mask off the port status change bits, these bits are
  // write clean bits
  //
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
    case EfiUsbPortEnable:
      //
      // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
      // A port may be disabled by software writing a '1' to this flag.
      //
      State |= XHC_PORTSC_PED;
      State &= ~XHC_PORTSC_RESET;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortSuspend:
      State |= XHC_PORTSC_LWS;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      State &= ~XHC_PORTSC_PLS;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortReset:
      //
      // PORTSC_RESET BIT(4) bit is RW1S attribute, which means Write-1-to-set status:
      // Register bits indicate status when read, a clear bit may be set by
      // writing a '1'. Writing a '0' to RW1S bits has no effect.
      //
      break;

    case EfiUsbPortPower:
      if (Xhc->HcCParams.Data.Ppc) {
        //
        // Port Power Control supported
        //
        State &= ~XHC_PORTSC_PP;
        XhcPeiWriteOpReg (Xhc, Offset, State);
      }
      break;

    case EfiUsbPortOwner:
      //
      // XHCI root hub port don't has the owner bit, ignore the operation
      //
      break;

    case EfiUsbPortConnectChange:
      //
      // Clear connect status change
      //
      State |= XHC_PORTSC_CSC;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortEnableChange:
      //
      // Clear enable status change
      //
      State |= XHC_PORTSC_PEC;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortOverCurrentChange:
      //
      // Clear PortOverCurrent change
      //
      State |= XHC_PORTSC_OCC;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortResetChange:
      //
      // Clear Port Reset change
      //
      State |= XHC_PORTSC_PRC;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortSuspendChange:
      //
      // Not supported or not related operation
      //
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, "XhcPeiClearRootHubPortFeature: PortFeature: %x Status = %r\n", PortFeature, Status));
  return Status;
}

/**
  Sets a feature for the specified root hub port.

  @param  PeiServices               The pointer of EFI_PEI_SERVICES
  @param  This                      The pointer of PEI_USB2_HOST_CONTROLLER_PPI
  @param  PortNumber                Root hub port to set.
  @param  PortFeature               Feature to set.

  @retval EFI_SUCCESS               The feature specified by PortFeature was set.
  @retval EFI_INVALID_PARAMETER     PortNumber is invalid or PortFeature is invalid.
  @retval EFI_TIMEOUT               The time out occurred.

**/
EFI_STATUS
EFIAPI
XhcPeiSetRootHubPortFeature (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *This,
  IN UINT8                          PortNumber,
  IN EFI_USB_PORT_FEATURE           PortFeature
  )
{
  PEI_XHC_DEV           *Xhc;
  UINT32                Offset;
  UINT32                State;
  EFI_STATUS            Status;

  Xhc = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);
  Status = EFI_SUCCESS;

  if (PortNumber >= Xhc->HcSParams1.Data.MaxPorts) {
    Status = EFI_INVALID_PARAMETER;
    goto ON_EXIT;
  }

  Offset = (UINT32) (XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State = XhcPeiReadOpReg (Xhc, Offset);
  DEBUG ((EFI_D_INFO, "XhcPeiSetRootHubPortFeature: Port: %x State: %x\n", PortNumber, State));

  //
  // Mask off the port status change bits, these bits are
  // write clean bits
  //
  State &= ~ (BIT1 | BIT17 | BIT18 | BIT19 | BIT20 | BIT21 | BIT22 | BIT23);

  switch (PortFeature) {
    case EfiUsbPortEnable:
      //
      // Ports may only be enabled by the xHC. Software cannot enable a port by writing a '1' to this flag.
      // A port may be disabled by software writing a '1' to this flag.
      //
      break;

    case EfiUsbPortSuspend:
      State |= XHC_PORTSC_LWS;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      State &= ~XHC_PORTSC_PLS;
      State |= (3 << 5) ;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      break;

    case EfiUsbPortReset:
      //
      // Make sure Host Controller not halt before reset it
      //
      if (XhcPeiIsHalt (Xhc)) {
        Status = XhcPeiRunHC (Xhc, XHC_GENERIC_TIMEOUT);
        if (EFI_ERROR (Status)) {
          break;
        }
      }

      //
      // 4.3.1 Resetting a Root Hub Port
      // 1) Write the PORTSC register with the Port Reset (PR) bit set to '1'.
      // 2) Wait for a successful Port Status Change Event for the port, where the Port Reset Change (PRC)
      //    bit in the PORTSC field is set to '1'.
      //
      State |= XHC_PORTSC_RESET;
      XhcPeiWriteOpReg (Xhc, Offset, State);
      XhcPeiWaitOpRegBit(Xhc, Offset, XHC_PORTSC_PRC, TRUE, XHC_GENERIC_TIMEOUT);
      break;

    case EfiUsbPortPower:
      if (Xhc->HcCParams.Data.Ppc) {
        //
        // Port Power Control supported
        //
        State |= XHC_PORTSC_PP;
        XhcPeiWriteOpReg (Xhc, Offset, State);
      }
      break;

    case EfiUsbPortOwner:
      //
      // XHCI root hub port don't has the owner bit, ignore the operation
      //
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
  }

ON_EXIT:
  DEBUG ((EFI_D_INFO, "XhcPeiSetRootHubPortFeature: PortFeature: %x Status = %r\n", PortFeature, Status));
  return Status;
}

/**
  Retrieves the current status of a USB root hub port.

  @param  PeiServices               The pointer of EFI_PEI_SERVICES.
  @param  This                      The pointer of PEI_USB2_HOST_CONTROLLER_PPI.
  @param  PortNumber                The root hub port to retrieve the state from.
  @param  PortStatus                Variable to receive the port state.

  @retval EFI_SUCCESS               The status of the USB root hub port specified.
                                    by PortNumber was returned in PortStatus.
  @retval EFI_INVALID_PARAMETER     PortNumber is invalid.

**/
EFI_STATUS
EFIAPI
XhcPeiGetRootHubPortStatus (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB2_HOST_CONTROLLER_PPI   *This,
  IN UINT8                          PortNumber,
  OUT EFI_USB_PORT_STATUS           *PortStatus
  )
{
  PEI_XHC_DEV               *Xhc;
  UINT32                    Offset;
  UINT32                    State;
  UINTN                     Index;
  UINTN                     MapSize;
  USB_DEV_ROUTE             ParentRouteChart;

  if (PortStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Xhc = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS (This);

  if (PortNumber >= Xhc->HcSParams1.Data.MaxPorts) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Clear port status.
  //
  PortStatus->PortStatus        = 0;
  PortStatus->PortChangeStatus  = 0;

  Offset                        = (UINT32) (XHC_PORTSC_OFFSET + (0x10 * PortNumber));
  State                         = XhcPeiReadOpReg (Xhc, Offset);
  DEBUG ((EFI_D_INFO, "XhcPeiGetRootHubPortStatus: Port: %x State: %x\n", PortNumber, State));

  //
  // According to XHCI 1.1 spec November 2017,
  // bit 10~13 of the root port status register identifies the speed of the attached device.
  //
  switch ((State & XHC_PORTSC_PS) >> 10) {
    case 2:
      PortStatus->PortStatus |= USB_PORT_STAT_LOW_SPEED;
      break;

    case 3:
      PortStatus->PortStatus |= USB_PORT_STAT_HIGH_SPEED;
      break;

    case 4:
    case 5:
      PortStatus->PortStatus |= USB_PORT_STAT_SUPER_SPEED;
      break;

    default:
      break;
  }

  //
  // Convert the XHCI port/port change state to UEFI status
  //
  MapSize = sizeof (mUsbPortStateMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortStateMap[Index].HwState)) {
      PortStatus->PortStatus = (UINT16) (PortStatus->PortStatus | mUsbPortStateMap[Index].UefiState);
    }
  }
  //
  // Bit5~8 reflects its current link state.
  //
  if ((State & XHC_PORTSC_PLS) >> 5 == 3) {
    PortStatus->PortStatus |= USB_PORT_STAT_SUSPEND;
  }

  MapSize = sizeof (mUsbPortChangeMap) / sizeof (USB_PORT_STATE_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbPortChangeMap[Index].HwState)) {
      PortStatus->PortChangeStatus = (UINT16) (PortStatus->PortChangeStatus | mUsbPortChangeMap[Index].UefiState);
    }
  }

  MapSize = sizeof (mUsbClearPortChangeMap) / sizeof (USB_CLEAR_PORT_MAP);

  for (Index = 0; Index < MapSize; Index++) {
    if (XHC_BIT_IS_SET (State, mUsbClearPortChangeMap[Index].HwState)) {
      XhcPeiClearRootHubPortFeature (PeiServices, This, PortNumber, (EFI_USB_PORT_FEATURE)mUsbClearPortChangeMap[Index].Selector);
    }
  }

  //
  // Poll the root port status register to enable/disable corresponding device slot if there is a device attached/detached.
  // For those devices behind hub, we get its attach/detach event by hooking Get_Port_Status request at control transfer for those hub.
  //
  ParentRouteChart.Dword = 0;
  XhcPeiPollPortStatusChange (Xhc, ParentRouteChart, PortNumber, PortStatus);

  DEBUG ((EFI_D_INFO, "XhcPeiGetRootHubPortStatus: PortChangeStatus: %x PortStatus: %x\n", PortStatus->PortChangeStatus, PortStatus->PortStatus));
  return EFI_SUCCESS;
}

/**
  One notified function to stop the Host Controller at the end of PEI

  @param[in]  PeiServices        Pointer to PEI Services Table.
  @param[in]  NotifyDescriptor   Pointer to the descriptor for the Notification event that
                                 caused this function to execute.
  @param[in]  Ppi                Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully
  @retval     others
**/
EFI_STATUS
EFIAPI
XhcEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  PEI_XHC_DEV    *Xhc;

  Xhc = PEI_RECOVERY_USB_XHC_DEV_FROM_THIS_NOTIFY(NotifyDescriptor);

  XhcPeiHaltHC (Xhc, XHC_GENERIC_TIMEOUT);

  XhcPeiFreeSched (Xhc);

  return EFI_SUCCESS;
}

/**
  @param FileHandle     Handle of the file being invoked.
  @param PeiServices    Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   PPI successfully installed.

**/
EFI_STATUS
EFIAPI
XhcPeimEntry (
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN CONST EFI_PEI_SERVICES **PeiServices
  )
{
  PEI_USB_CONTROLLER_PPI      *UsbControllerPpi;
  EFI_STATUS                  Status;
  UINT8                       Index;
  UINTN                       ControllerType;
  UINTN                       BaseAddress;
  UINTN                       MemPages;
  PEI_XHC_DEV                 *XhcDev;
  EFI_PHYSICAL_ADDRESS        TempPtr;
  UINT32                      PageSize;

  //
  // Shadow this PEIM to run from memory.
  //
  if (!EFI_ERROR (PeiServicesRegisterForShadow (FileHandle))) {
    return EFI_SUCCESS;
  }

  Status = PeiServicesLocatePpi (
             &gPeiUsbControllerPpiGuid,
             0,
             NULL,
             (VOID **) &UsbControllerPpi
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  IoMmuInit ();

  Index = 0;
  while (TRUE) {
    Status = UsbControllerPpi->GetUsbController (
                                 (EFI_PEI_SERVICES **) PeiServices,
                                 UsbControllerPpi,
                                 Index,
                                 &ControllerType,
                                 &BaseAddress
                                 );
    //
    // When status is error, it means no controller is found.
    //
    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // This PEIM is for XHC type controller.
    //
    if (ControllerType != PEI_XHCI_CONTROLLER) {
      Index++;
      continue;
    }

    MemPages = EFI_SIZE_TO_PAGES (sizeof (PEI_XHC_DEV));
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               MemPages,
               &TempPtr
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }
    ZeroMem ((VOID *) (UINTN) TempPtr, EFI_PAGES_TO_SIZE (MemPages));
    XhcDev = (PEI_XHC_DEV *) ((UINTN) TempPtr);

    XhcDev->Signature = USB_XHC_DEV_SIGNATURE;
    XhcDev->UsbHostControllerBaseAddress = (UINT32) BaseAddress;
    XhcDev->CapLength           = (UINT8) (XhcPeiReadCapRegister (XhcDev, XHC_CAPLENGTH_OFFSET) & 0x0FF);
    XhcDev->HcSParams1.Dword    = XhcPeiReadCapRegister (XhcDev, XHC_HCSPARAMS1_OFFSET);
    XhcDev->HcSParams2.Dword    = XhcPeiReadCapRegister (XhcDev, XHC_HCSPARAMS2_OFFSET);
    XhcDev->HcCParams.Dword     = XhcPeiReadCapRegister (XhcDev, XHC_HCCPARAMS_OFFSET);
    XhcDev->DBOff               = XhcPeiReadCapRegister (XhcDev, XHC_DBOFF_OFFSET);
    XhcDev->RTSOff              = XhcPeiReadCapRegister (XhcDev, XHC_RTSOFF_OFFSET);

    //
    // This PageSize field defines the page size supported by the xHC implementation.
    // This xHC supports a page size of 2^(n+12) if bit n is Set. For example,
    // if bit 0 is Set, the xHC supports 4k byte page sizes.
    //
    PageSize         = XhcPeiReadOpReg (XhcDev, XHC_PAGESIZE_OFFSET) & XHC_PAGESIZE_MASK;
    XhcDev->PageSize = 1 << (HighBitSet32 (PageSize) + 12);

    DEBUG ((EFI_D_INFO, "XhciPei: UsbHostControllerBaseAddress: %x\n", XhcDev->UsbHostControllerBaseAddress));
    DEBUG ((EFI_D_INFO, "XhciPei: CapLength:                    %x\n", XhcDev->CapLength));
    DEBUG ((EFI_D_INFO, "XhciPei: HcSParams1:                   %x\n", XhcDev->HcSParams1.Dword));
    DEBUG ((EFI_D_INFO, "XhciPei: HcSParams2:                   %x\n", XhcDev->HcSParams2.Dword));
    DEBUG ((EFI_D_INFO, "XhciPei: HcCParams:                    %x\n", XhcDev->HcCParams.Dword));
    DEBUG ((EFI_D_INFO, "XhciPei: DBOff:                        %x\n", XhcDev->DBOff));
    DEBUG ((EFI_D_INFO, "XhciPei: RTSOff:                       %x\n", XhcDev->RTSOff));
    DEBUG ((EFI_D_INFO, "XhciPei: PageSize:                     %x\n", XhcDev->PageSize));

    XhcPeiResetHC (XhcDev, XHC_RESET_TIMEOUT);
    ASSERT (XhcPeiIsHalt (XhcDev));

    //
    // Initialize the schedule
    //
    XhcPeiInitSched (XhcDev);

    //
    // Start the Host Controller
    //
    XhcPeiRunHC (XhcDev, XHC_GENERIC_TIMEOUT);

    //
    // Wait for root port state stable
    //
    MicroSecondDelay (XHC_ROOT_PORT_STATE_STABLE);

    XhcDev->Usb2HostControllerPpi.ControlTransfer           = XhcPeiControlTransfer;
    XhcDev->Usb2HostControllerPpi.BulkTransfer              = XhcPeiBulkTransfer;
    XhcDev->Usb2HostControllerPpi.GetRootHubPortNumber      = XhcPeiGetRootHubPortNumber;
    XhcDev->Usb2HostControllerPpi.GetRootHubPortStatus      = XhcPeiGetRootHubPortStatus;
    XhcDev->Usb2HostControllerPpi.SetRootHubPortFeature     = XhcPeiSetRootHubPortFeature;
    XhcDev->Usb2HostControllerPpi.ClearRootHubPortFeature   = XhcPeiClearRootHubPortFeature;

    XhcDev->PpiDescriptor.Flags = (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    XhcDev->PpiDescriptor.Guid = &gPeiUsb2HostControllerPpiGuid;
    XhcDev->PpiDescriptor.Ppi = &XhcDev->Usb2HostControllerPpi;

    XhcDev->EndOfPeiNotifyList.Flags = (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST);
    XhcDev->EndOfPeiNotifyList.Guid = &gEfiEndOfPeiSignalPpiGuid;
    XhcDev->EndOfPeiNotifyList.Notify = XhcEndOfPei;

    PeiServicesInstallPpi (&XhcDev->PpiDescriptor);
    PeiServicesNotifyPpi (&XhcDev->EndOfPeiNotifyList);

    Index++;
  }

  return EFI_SUCCESS;
}

