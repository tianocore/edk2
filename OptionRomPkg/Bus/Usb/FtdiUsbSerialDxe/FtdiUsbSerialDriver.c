/** @file
  USB Serial Driver that manages USB to Serial and produces Serial IO Protocol.

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.
Portions Copyright 2012 Ashley DeSimone
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//

// Tested with VEND_ID 0x0403, DEVICE_ID 0x6001
//
// Driver starts the device with the following values:
// 115200, No parity, 8 data bits, 1 stop bit, No Flow control
//

#include "FtdiUsbSerialDriver.h"

//
// Table of supported devices. This is the device information that this
// driver was developed with. Add other FTDI devices as needed.
//
USB_DEVICE gUSBDeviceList[] = {
  {VID_FTDI, DID_FTDI_FT232},
  {0,0}
};

//
// USB Serial Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL  gUsbSerialDriverBinding = {
  UsbSerialDriverBindingSupported,
  UsbSerialDriverBindingStart,
  UsbSerialDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// Table with the nearest power of 2 for the numbers 0-15
//
UINT8 gRoundedPowersOf2[16] = { 0, 2, 2, 4, 4, 4, 8, 8, 8, 8, 8, 8, 16, 16, 16, 16 };

/**
  Check to see if the device path node is the Flow control node

  @param[in] FlowControl    The device path node to be checked

  @retval    TRUE           It is the flow control node
  @retval    FALSE          It is not the flow control node

**/
BOOLEAN
IsUartFlowControlNode (
  IN UART_FLOW_CONTROL_DEVICE_PATH *FlowControl
  )
{
  return (BOOLEAN) (
           (DevicePathType (FlowControl) == MESSAGING_DEVICE_PATH) &&
           (DevicePathSubType (FlowControl) == MSG_VENDOR_DP) &&
           (CompareGuid (&FlowControl->Guid, &gEfiUartDevicePathGuid))
           );
}

/**
  Checks the device path to see if it contains flow control.

  @param[in] DevicePath    The device path to be checked

  @retval    TRUE          It contains flow control
  @retval    FALSE         It does not contain flow control

**/
BOOLEAN
ContainsFlowControl (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  while (!IsDevicePathEnd (DevicePath)) {
    if (IsUartFlowControlNode ((UART_FLOW_CONTROL_DEVICE_PATH *) DevicePath)) {
      return TRUE;
    }
    DevicePath = NextDevicePathNode (DevicePath);
  }
  return FALSE;
}

/**
  Transfer the data between the device and host.

  This function transfers the data between the device and host.
  BOT transfer is composed of three phases: Command, Data, and Status.
  This is the Data phase.

  @param  UsbBot[in]                     The USB BOT device
  @param  DataDir[in]                    The direction of the data
  @param  Data[in, out]                  The buffer to hold data
  @param  TransLen[in, out]              The expected length of the data
  @param  Timeout[in]                    The time to wait the command to complete

  @retval EFI_SUCCESS                    The data is transferred
  @retval EFI_SUCCESS                    No data to transfer
  @retval EFI_NOT_READY                  The device return NAK to the transfer
  @retval Others                         Failed to transfer data

**/
EFI_STATUS
UsbSerialDataTransfer (
  IN USB_SER_DEV             *UsbBot,
  IN EFI_USB_DATA_DIRECTION  DataDir,
  IN OUT VOID                *Data,
  IN OUT UINTN               *TransLen,
  IN UINT32                  Timeout
  )
{
  EFI_USB_ENDPOINT_DESCRIPTOR  *Endpoint;
  EFI_STATUS                   Status;
  UINT32                       Result;

  //
  // If no data to transfer, just return EFI_SUCCESS.
  //
  if ((DataDir == EfiUsbNoData) || (*TransLen == 0)) {
    return EFI_SUCCESS;
  }

  //
  // Select the endpoint then issue the transfer
  //
  if (DataDir == EfiUsbDataIn) {
    Endpoint = &UsbBot->InEndpointDescriptor;
  } else {
    Endpoint = &UsbBot->OutEndpointDescriptor;
  }

  Result = 0;
  Status = UsbBot->UsbIo->UsbBulkTransfer (
                            UsbBot->UsbIo,
                            Endpoint->EndpointAddress,
                            Data,
                            TransLen,
                            Timeout,
                            &Result
                            );
  if (EFI_ERROR (Status)) {
    if (USB_IS_ERROR (Result, EFI_USB_ERR_NAK)) {
      Status = EFI_NOT_READY;
    } else {
      UsbBot->Shutdown = TRUE; // Fixes infinite loop in older EFI
    }
    return Status;
  }
  return Status;
}

/**
  Sets the status values of the Usb Serial Device.

  @param  UsbSerialDevice[in]  Handle to the Usb Serial Device to set the status
                               for
  @param  StatusBuffer[in]     Buffer holding the status values

  @retval EFI_SUCCESS          The status values were read and set correctly

**/
EFI_STATUS
EFIAPI
SetStatusInternal (
  IN USB_SER_DEV  *UsbSerialDevice,
  IN UINT8        *StatusBuffer
  )
{
  UINT8  Msr;

  Msr = (StatusBuffer[0] & MSR_MASK);

  //
  // set the Status values to disabled
  //
  UsbSerialDevice->StatusValues.CtsState = FALSE;
  UsbSerialDevice->StatusValues.DsrState = FALSE;
  UsbSerialDevice->StatusValues.RiState  = FALSE;
  UsbSerialDevice->StatusValues.SdState  = FALSE;

  //
  // Check the values from the status buffer and set the appropriate status
  // values to enabled
  //
  if ((Msr & CTS_MASK) == CTS_MASK) {
    UsbSerialDevice->StatusValues.CtsState = TRUE;
  }
  if ((Msr & DSR_MASK) == DSR_MASK) {
    UsbSerialDevice->StatusValues.DsrState = TRUE;
  }
  if ((Msr & RI_MASK) == RI_MASK) {
    UsbSerialDevice->StatusValues.RiState = TRUE;
  }
  if ((Msr & SD_MASK) == SD_MASK) {
    UsbSerialDevice->StatusValues.SdState = TRUE;
  }
  return EFI_SUCCESS;
}

/**
  Initiates a read operation on the Usb Serial Device.

  @param  UsbSerialDevice[in]        Handle to the USB device to read
  @param  BufferSize[in, out]        On input, the size of the Buffer. On output,
                                     the amount of data returned in Buffer.
                                     Setting this to zero will initiate a read
                                     and store all data returned in the internal
                                     buffer.
  @param  Buffer [out]               The buffer to return the data into.

  @retval EFI_SUCCESS                The data was read.
  @retval EFI_DEVICE_ERROR           The device reported an error.
  @retval EFI_TIMEOUT                The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
ReadDataFromUsb (
  IN USB_SER_DEV  *UsbSerialDevice,
  IN OUT UINTN    *BufferSize,
  OUT VOID        *Buffer
  )
{
  EFI_STATUS  Status;
  UINTN       ReadBufferSize;
  UINT8       *ReadBuffer;
  UINTN       Index;
  EFI_TPL     Tpl;
  UINT8       StatusBuffer[2]; // buffer to store the status bytes

  ReadBufferSize = 512;
  ReadBuffer     = &(UsbSerialDevice->ReadBuffer[0]);

  if (UsbSerialDevice->Shutdown) {
    return EFI_DEVICE_ERROR;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = UsbSerialDataTransfer (
             UsbSerialDevice,
             EfiUsbDataIn,
             ReadBuffer,
             &ReadBufferSize,
             FTDI_TIMEOUT*2  //Padded because timers won't be exactly aligned
             );
  if (EFI_ERROR (Status)) {
    gBS->RestoreTPL (Tpl);
    if (Status == EFI_TIMEOUT) {
      return EFI_TIMEOUT;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  //
  // Store the status bytes in the status buffer
  //
  for (Index = 0; Index < 2; Index++) {//only the first 2 bytes are status bytes
    StatusBuffer[Index] = ReadBuffer[Index];
  }
  //
  // update the statusvalue field of the usbserialdevice
  //
  Status = SetStatusInternal (UsbSerialDevice, StatusBuffer);
  if (Status != EFI_SUCCESS) {
  }

  //
  // Store the read data in the read buffer, start at 2 to ignore status bytes
  //
  for (Index = 2; Index < ReadBufferSize; Index++) {
    if (((UsbSerialDevice->DataBufferTail + 1) % SW_FIFO_DEPTH) == UsbSerialDevice->DataBufferHead) {
      break;
    }
    if (ReadBuffer[Index] == 0x00) {
      //
      // This is null, do not add
      //
    } else {
      UsbSerialDevice->DataBuffer[UsbSerialDevice->DataBufferTail] = ReadBuffer[Index];
      UsbSerialDevice->DataBufferTail = (UsbSerialDevice->DataBufferTail + 1) % SW_FIFO_DEPTH;
    }
  }

  //
  // Read characters out of the buffer to satisfy caller's request.
  //
  for (Index = 0; Index < *BufferSize; Index++) {
    if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
      break;
    }
    //
    // Still have characters in the buffer to return
    //
    ((UINT8 *)Buffer)[Index]        = UsbSerialDevice->DataBuffer[UsbSerialDevice->DataBufferHead];
    UsbSerialDevice->DataBufferHead = (UsbSerialDevice->DataBufferHead + 1) % SW_FIFO_DEPTH;
  }
  //
  // Return actual number of bytes returned.
  //
  *BufferSize = Index;
  gBS->RestoreTPL (Tpl);
  return EFI_SUCCESS;
}

/**
  Sets the initial status values of the Usb Serial Device by reading the status
  bytes from the device.

  @param  UsbSerialDevice[in]  Handle to the Usb Serial Device that needs its
                               initial status values set

  @retval EFI_SUCCESS          The status bytes were read successfully and the
                               initial status values were set correctly
  @retval EFI_TIMEOUT          The read of the status bytes was stopped due to a
                               timeout
  @retval EFI_DEVICE_ERROR     The device reported an error during the read of
                               the status bytes

**/
EFI_STATUS
EFIAPI
SetInitialStatus (
  IN USB_SER_DEV          *UsbSerialDevice
  )
{
  EFI_STATUS      Status;
  UINTN           BufferSize;
  EFI_TPL         Tpl;
  UINT8           StatusBuffer[2];

  Status          = EFI_UNSUPPORTED;
  BufferSize      = sizeof (StatusBuffer);

  if (UsbSerialDevice->Shutdown) {
    return EFI_DEVICE_ERROR;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = UsbSerialDataTransfer (
             UsbSerialDevice,
             EfiUsbDataIn,
             StatusBuffer,
             &BufferSize,
             40    //Slightly more than 2x the FTDI polling frequency to make sure that data will be returned
             );

  Status = SetStatusInternal (UsbSerialDevice, StatusBuffer);

  gBS->RestoreTPL (Tpl);

  return Status;
}

/**
  UsbSerialDriverCheckInput.
  attempts to read data in from the device periodically, stores any read data
  and updates the control attributes.

  @param  Event[in]
  @param  Context[in]....The current instance of the USB serial device

**/
VOID
EFIAPI
UsbSerialDriverCheckInput (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  UINTN        BufferSize;
  USB_SER_DEV  *UsbSerialDevice;

  UsbSerialDevice = (USB_SER_DEV*)Context;

  if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
    //
    // Data buffer is empty, try to read from device
    //
    BufferSize = 0;
    ReadDataFromUsb (UsbSerialDevice, &BufferSize, NULL);
    if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
      //
      // Data buffer still has no data, set the EFI_SERIAL_INPUT_BUFFER_EMPTY
      // flag
      //
      UsbSerialDevice->ControlBits |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
    } else {
      //
      // Read has returned some data, clear the EFI_SERIAL_INPUT_BUFFER_EMPTY
      // flag
      //
      UsbSerialDevice->ControlBits &= ~(EFI_SERIAL_INPUT_BUFFER_EMPTY);
    }
  } else {
    //
    // Data buffer has data, no read attempt required
    //
    UsbSerialDevice->ControlBits &= ~(EFI_SERIAL_INPUT_BUFFER_EMPTY);
  }
}

/**
  Encodes the baud rate into the format expected by the Ftdi device.

  @param  BaudRate[in]                The baudrate to be set on the device
  @param  EncodedBaudRate[out]        The baud rate encoded in the format
                                      expected by the Ftdi device

  @return EFI_SUCCESS                 Baudrate encoding was calculated
                                      successfully
  @return EFI_INVALID_PARAMETER       An invalid value of BaudRate was received

**/
EFI_STATUS
EFIAPI
EncodeBaudRateForFtdi (
  IN  UINT64  BaudRate,
  OUT UINT16  *EncodedBaudRate
  )
{
  UINT32 Divisor;
  UINT32 AdjustedFrequency;
  UINT16 Result;

  //
  // Check to make sure we won't get an integer overflow
  //
  if ((BaudRate < 178) || ( BaudRate > ((FTDI_UART_FREQUENCY * 100) / 97))) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Baud Rates of 2000000 and 3000000 are special cases
  //
  if ((BaudRate >= FTDI_SPECIAL_CASE_300_MIN) && (BaudRate <= FTDI_SPECIAL_CASE_300_MAX)) {
    *EncodedBaudRate = 0;
    return EFI_SUCCESS;
  }
  if ((BaudRate >= FTDI_SPECIAL_CASE_200_MIN) && (BaudRate <= FTDI_SPECIAL_CASE_200_MAX)) {
    *EncodedBaudRate = 1;
    return EFI_SUCCESS;
  }

  //
  // Compute divisor
  //
  Divisor = (FTDI_UART_FREQUENCY << 4) / (UINT32)BaudRate;

  //
  // Round the last 4 bits to the nearest power of 2
  //
  Divisor = (Divisor & ~(0xF)) + (gRoundedPowersOf2[Divisor & 0xF]);

  //
  // Check to make sure computed divisor is within 
  // the min and max that FTDI controller will accept
  //
  if (Divisor < FTDI_MIN_DIVISOR) {
    Divisor = FTDI_MIN_DIVISOR;
  } else if (Divisor > FTDI_MAX_DIVISOR) {
    Divisor = FTDI_MAX_DIVISOR;
  }

  //
  // Check to make sure the frequency that the FTDI chip will need to
  // generate to attain the requested Baud Rate is within 3% of the
  // 3MHz clock frequency that the FTDI chip runs at.
  //
  // (3MHz * 1600) / 103 = 46601941
  // (3MHz * 1600) / 97  = 49484536
  //
  AdjustedFrequency = (((UINT32)BaudRate) * Divisor);
  if ((AdjustedFrequency < FTDI_MIN_FREQUENCY) || (AdjustedFrequency > FTDI_MAX_FREQUENCY)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Encode the Divisor into the format FTDI expects
  //
  Result = (UINT16)(Divisor >> 4);
  if ((Divisor & 0x8) != 0) {
    Result |= 0x4000;
  } else if ((Divisor & 0x4) != 0) {
    Result |= 0x8000;
  } else if ((Divisor & 0x2) != 0) {
    Result |= 0xC000;
  }

  *EncodedBaudRate = Result;
  return EFI_SUCCESS;
}

/**
  Uses USB I/O to check whether the device is a USB Serial device.

  @param  UsbIo[in]    Pointer to a USB I/O protocol instance.

  @retval TRUE         Device is a USB Serial device.
  @retval FALSE        Device is a not USB Serial device.

**/
BOOLEAN
IsUsbSerial (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                 Status;
  EFI_USB_DEVICE_DESCRIPTOR  DeviceDescriptor;
  CHAR16                     *StrMfg;
  BOOLEAN                    Found;
  UINT32                     Index;

  //
  // Get the default device descriptor
  //
  Status = UsbIo->UsbGetDeviceDescriptor (
                    UsbIo,
                    &DeviceDescriptor
                    );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Found = FALSE;
  Index = 0;
  while (gUSBDeviceList[Index].VendorId != 0 &&
         gUSBDeviceList[Index].DeviceId != 0 &&
         !Found                                  ) {
    if (DeviceDescriptor.IdProduct == gUSBDeviceList[Index].DeviceId &&
        DeviceDescriptor.IdVendor  == gUSBDeviceList[Index].VendorId      ){
        //
        // Checks to see if a string descriptor can be pulled from the device in
        // the selected language. If not False is returned indicating that this
        // is not a Usb Serial Device that can be managegd by this driver
        //
        StrMfg = NULL;
        Status = UsbIo->UsbGetStringDescriptor (
                          UsbIo,
                          USB_US_LANG_ID, // LANGID selector, should make this
                                          // more robust to verify lang support
                                          // for device
                          DeviceDescriptor.StrManufacturer,
                          &StrMfg
                          );
        if (StrMfg != NULL) {
          FreePool (StrMfg);
        }
        if (EFI_ERROR (Status)) {
          return FALSE;
        }
        return TRUE;
    }
    Index++;
  }
  return FALSE;
}

/**
  Internal function that sets the Data Bits, Stop Bits and Parity values on the
  Usb Serial Device with a single usb control transfer.

  @param  UsbIo[in]                  Usb Io Protocol instance pointer
  @param  DataBits[in]               The data bits value to be set on the Usb
                                     Serial Device
  @param  Parity[in]                 The parity type that will be set on the Usb
                                     Serial Device
  @param  StopBits[in]               The stop bits type that will be set on the
                                     Usb Serial Device
  @param  LastSettings[in]           A pointer to the Usb Serial Device's
                                     PREVIOUS_ATTRIBUTES item

  @retval EFI_SUCCESS                The data items were correctly set on the
                                     USB Serial Device
  @retval EFI_INVALID_PARAMETER      An invalid data parameter or an invalid
                                     combination or parameters was used
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly and
                                     the data values were unable to be set

**/
EFI_STATUS
EFIAPI
SetDataInternal (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN UINT8                DataBits,
  IN EFI_PARITY_TYPE      Parity,
  IN EFI_STOP_BITS_TYPE   StopBits,
  IN PREVIOUS_ATTRIBUTES  *LastSettings
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  ReturnValue;
  UINT8                   ConfigurationValue;

  //
  // Since data bits settings of 6,7,8 cannot be set with a stop bits setting of
  // 1.5 check to see if this happens when the values of last settings are used
  //
  if ((DataBits == 0) && (StopBits == OneFiveStopBits)) {
    if ((LastSettings->DataBits == 6) || (LastSettings->DataBits == 7) || (LastSettings->DataBits == 8)) {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((StopBits == DefaultStopBits) && ((DataBits == 6) || (DataBits == 7) || (DataBits == 8))) {
    if (LastSettings->StopBits == OneFiveStopBits) {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((DataBits == 0) && (StopBits == DefaultStopBits)) {
    if (LastSettings->StopBits == OneFiveStopBits) {
      if ((LastSettings->DataBits == 6) || (LastSettings->DataBits == 7) || (LastSettings->DataBits == 8)) {
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  //
  // set the DevReq.Value for the usb control transfer to the correct value
  // based on the seleceted number of data bits if there is an invalid number of
  // data bits requested return EFI_INVALID_PARAMETER
  //
  if (((DataBits < 5 ) || (DataBits > 8)) && (DataBits != 0)) {
    return EFI_INVALID_PARAMETER;
  }
  if (DataBits == 0) {
    //
    // use the value of LastDataBits
    //
    DevReq.Value = SET_DATA_BITS (LastSettings->DataBits);
  } else {
    //
    // use the value of DataBits
    //
    DevReq.Value = SET_DATA_BITS (DataBits);
  }

  //
  // Set Parity
  //
  if (Parity == DefaultParity) {
    Parity = LastSettings->Parity;
  }

  if (Parity == NoParity) {
    DevReq.Value |= SET_PARITY_NONE;
  } else if (Parity == EvenParity) {
    DevReq.Value |= SET_PARITY_EVEN;
  } else if (Parity == OddParity){
    DevReq.Value |= SET_PARITY_ODD;
  } else if (Parity == MarkParity) {
    DevReq.Value |= SET_PARITY_MARK;
  } else if (Parity == SpaceParity) {
    DevReq.Value |= SET_PARITY_SPACE;
  }

  //
  // Set Stop Bits
  //
  if (StopBits == DefaultStopBits) {
    StopBits = LastSettings->StopBits;
  }

  if (StopBits == OneStopBit) {
    DevReq.Value |= SET_STOP_BITS_1;
  } else if (StopBits == OneFiveStopBits) {
    DevReq.Value |= SET_STOP_BITS_15;
  } else if (StopBits == TwoStopBits) {
    DevReq.Value |= SET_STOP_BITS_2;
  }

  //
  // set the rest of the DevReq parameters and perform the usb control transfer
  // to set the data bits on the device
  //
  DevReq.Request     = FTDI_COMMAND_SET_DATA;
  DevReq.RequestType = USB_REQ_TYPE_VENDOR;
  DevReq.Index       = FTDI_PORT_IDENTIFIER;
  DevReq.Length      = 0; // indicates that there is no data phase in this request

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbDataOut,
                    WDR_SHORT_TIMEOUT,
                    &ConfigurationValue,
                    1,
                    &ReturnValue
                    );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }
  return Status;

StatusError:
  if ((Status != EFI_INVALID_PARAMETER) || (Status != EFI_DEVICE_ERROR)) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that sets the baudrate on the Usb Serial Device.

  @param  UsbIo[in]                  Usb Io Protocol instance pointer
  @param  BaudRate[in]               The baudrate value to be set on the device.
                                     If this value is 0 the value of LastBaudRate
                                     will be used instead
  @param  LastBaudRate[in]           The baud rate value that was previously set
                                     on the Usb Serial Device

  @retval EFI_SUCCESS                The baudrate was set succesfully
  @retval EFI_INVALID_PARAMETER      An invalid baudrate was used
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly and
                                     the baudrate was unable to be set

**/
EFI_STATUS
EFIAPI
SetBaudRateInternal (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN UINT64               BaudRate,
  IN UINT64               LastBaudRate
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  ReturnValue;
  UINT8                   ConfigurationValue;
  UINT16                  EncodedBaudRate;
  EFI_TPL                 Tpl;

  Tpl    = gBS->RaiseTPL(TPL_NOTIFY);

  //
  // set the value of DevReq.Value based on the value of BaudRate
  // if 0 is selected as baud rate use the value of LastBaudRate
  //
  if (BaudRate == 0) {
    Status = EncodeBaudRateForFtdi (LastBaudRate, &EncodedBaudRate);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (Tpl);
      //
      // EncodeBaudRateForFtdi returns EFI_INVALID_PARAMETER when not
      // succesfull
      //
      return Status;
    }
    DevReq.Value = EncodedBaudRate;
  } else {
    Status = EncodeBaudRateForFtdi (BaudRate, &EncodedBaudRate);
    if (EFI_ERROR (Status)) {
      gBS->RestoreTPL (Tpl);
      //
      // EncodeBaudRateForFtdi returns EFI_INVALID_PARAMETER when not
      // successfull
      //
      return Status;
    }
    DevReq.Value = EncodedBaudRate;
  }

  //
  // set the remaining parameters of DevReq and perform the usb control transfer
  // to set the device
  //
  DevReq.Request     = FTDI_COMMAND_SET_BAUDRATE;
  DevReq.RequestType = USB_REQ_TYPE_VENDOR;
  DevReq.Index       = FTDI_PORT_IDENTIFIER;
  DevReq.Length      = 0; // indicates that there is no data phase in this request

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbDataOut,
                    WDR_SHORT_TIMEOUT,
                    &ConfigurationValue,
                    1,
                    &ReturnValue
                    );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }
  gBS->RestoreTPL (Tpl);
  return Status;

StatusError:
  gBS->RestoreTPL (Tpl);
  if ((Status != EFI_INVALID_PARAMETER) || (Status != EFI_DEVICE_ERROR)) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Sets the baud rate, receive FIFO depth, transmit/receice time out, parity,
  data bits, and stop bits on a serial device.

  @param  UsbSerialDevice[in]  Pointer to the current instance of the USB Serial
                               Device.
  @param  BaudRate[in]         The requested baud rate. A BaudRate value of 0
                               will use the device's default interface speed.
  @param  ReveiveFifoDepth[in] The requested depth of the FIFO on the receive
                               side of the serial interface. A ReceiveFifoDepth
                               value of 0 will use the device's default FIFO
                               depth.
  @param  Timeout[in]          The requested time out for a single character in
                               microseconds.This timeout applies to both the
                               transmit and receive side of the interface.A
                               Timeout value of 0 will use the device's default
                               time out value.
  @param  Parity[in]           The type of parity to use on this serial device.
                               A Parity value of DefaultParity will use the
                               device's default parity value.
  @param  DataBits[in]         The number of data bits to use on the serial
                               device. A DataBits value of 0 will use the
                               device's default data bit setting.
  @param  StopBits[in]         The number of stop bits to use on this serial
                               device. A StopBits value of DefaultStopBits will
                               use the device's default number of stop bits.

  @retval EFI_SUCCESS          The attributes were set
  @retval EFI_DEVICE_ERROR     The attributes were not able to be set

**/
EFI_STATUS
EFIAPI
SetAttributesInternal (
  IN USB_SER_DEV         *UsbSerialDevice,
  IN UINT64              BaudRate,
  IN UINT32              ReceiveFifoDepth,
  IN UINT32              Timeout,
  IN EFI_PARITY_TYPE     Parity,
  IN UINT8               DataBits,
  IN EFI_STOP_BITS_TYPE  StopBits
  )
{
  EFI_STATUS                Status;
  EFI_TPL                   Tpl;
  UART_DEVICE_PATH          *Uart;
  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath;

  Status = EFI_UNSUPPORTED;
  Tpl    = gBS->RaiseTPL(TPL_NOTIFY);
  Uart   = NULL;

  //
  // check for invalid combinations of parameters
  //
  if (((DataBits >= 6) && (DataBits <= 8)) && (StopBits == OneFiveStopBits)) {
    return  EFI_INVALID_PARAMETER;
  }

  //
  // set data bits, parity and stop bits
  //
  Status = SetDataInternal (
             UsbSerialDevice->UsbIo,
             DataBits,
             Parity,
             StopBits,
             &(UsbSerialDevice->LastSettings)
             );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }
  //
  // set baudrate
  //
  Status = SetBaudRateInternal (
             UsbSerialDevice->UsbIo,
             BaudRate,
             UsbSerialDevice->LastSettings.BaudRate
             );
  if (EFI_ERROR (Status)){
    goto StatusError;
  }

  //
  // update the values of UsbSerialDevice->LastSettings and UsbSerialDevice->SerialIo.Mode
  //
  if (BaudRate == 0) {
    UsbSerialDevice->LastSettings.BaudRate   = UsbSerialDevice->LastSettings.BaudRate;
    UsbSerialDevice->SerialIo.Mode->BaudRate = UsbSerialDevice->LastSettings.BaudRate;
  } else {
    UsbSerialDevice->LastSettings.BaudRate   = BaudRate;
    UsbSerialDevice->SerialIo.Mode->BaudRate = BaudRate;
  }

  UsbSerialDevice->LastSettings.Timeout          = FTDI_TIMEOUT;
  UsbSerialDevice->LastSettings.ReceiveFifoDepth = FTDI_MAX_RECEIVE_FIFO_DEPTH;

  if (Parity == DefaultParity) {
    UsbSerialDevice->LastSettings.Parity   = UsbSerialDevice->LastSettings.Parity;
    UsbSerialDevice->SerialIo.Mode->Parity = UsbSerialDevice->LastSettings.Parity;
  } else {
    UsbSerialDevice->LastSettings.Parity   = Parity;
    UsbSerialDevice->SerialIo.Mode->Parity = Parity;
  }
  if (DataBits == 0) {
    UsbSerialDevice->LastSettings.DataBits   = UsbSerialDevice->LastSettings.DataBits;
    UsbSerialDevice->SerialIo.Mode->DataBits = UsbSerialDevice->LastSettings.DataBits;
  } else {
    UsbSerialDevice->LastSettings.DataBits   = DataBits;
    UsbSerialDevice->SerialIo.Mode->DataBits = DataBits;
  }
  if (StopBits == DefaultStopBits) {
    UsbSerialDevice->LastSettings.StopBits   = UsbSerialDevice->LastSettings.StopBits;
    UsbSerialDevice->SerialIo.Mode->StopBits = UsbSerialDevice->LastSettings.StopBits;
  } else {
    UsbSerialDevice->LastSettings.StopBits   = StopBits;
    UsbSerialDevice->SerialIo.Mode->StopBits = StopBits;
  }

  //
  // See if the device path node has changed
  //
  if (UsbSerialDevice->UartDevicePath.BaudRate == BaudRate &&
      UsbSerialDevice->UartDevicePath.DataBits == DataBits &&
      UsbSerialDevice->UartDevicePath.StopBits == StopBits &&
      UsbSerialDevice->UartDevicePath.Parity == Parity
      ) {
    gBS->RestoreTPL (Tpl);
    return EFI_SUCCESS;
  }

  //
  // Update the device path
  //
  UsbSerialDevice->UartDevicePath.BaudRate = BaudRate;
  UsbSerialDevice->UartDevicePath.DataBits = DataBits;
  UsbSerialDevice->UartDevicePath.StopBits = (UINT8) StopBits;
  UsbSerialDevice->UartDevicePath.Parity   = (UINT8) Parity;

  Status = EFI_SUCCESS;
  if (UsbSerialDevice->ControllerHandle != NULL) {
    RemainingDevicePath = UsbSerialDevice->DevicePath;
    while (!IsDevicePathEnd (RemainingDevicePath)) {
      Uart = (UART_DEVICE_PATH *) NextDevicePathNode (RemainingDevicePath);
      if (Uart->Header.Type == MESSAGING_DEVICE_PATH &&
          Uart->Header.SubType == MSG_UART_DP &&
          sizeof (UART_DEVICE_PATH) == DevicePathNodeLength ((EFI_DEVICE_PATH *) Uart)) {
        Uart->BaudRate = BaudRate;
        Uart->DataBits = DataBits;
        Uart->StopBits = (UINT8)StopBits;
        Uart->Parity   = (UINT8) Parity;
        break;
        }
        RemainingDevicePath = NextDevicePathNode (RemainingDevicePath);
    }
  }

  gBS->RestoreTPL (Tpl);
  return Status;

StatusError:
  gBS->RestoreTPL (Tpl);
  if ((Status != EFI_INVALID_PARAMETER) || (Status != EFI_DEVICE_ERROR)) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that performs a Usb Control Transfer to set the flow control
  on the Usb Serial Device.

  @param  UsbIo[in]                  Usb Io Protocol instance pointer
  @param  FlowControlEnable[in]      Data on the Enable/Disable status of Flow
                                     Control on the Usb Serial Device

  @retval EFI_SUCCESS                The flow control was set on the Usb Serial
                                     device
  @retval EFI_INVALID_PARAMETER      An invalid flow control value was used
  @retval EFI_EFI_UNSUPPORTED        The operation is not supported
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly

**/
EFI_STATUS
EFIAPI
SetFlowControlInternal (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN BOOLEAN              FlowControlEnable
  )
{
  EFI_STATUS               Status;
  EFI_USB_DEVICE_REQUEST   DevReq;
  UINT32                   ReturnValue;
  UINT8                    ConfigurationValue;

  //
  // set DevReq.Value based on the value of FlowControlEnable
  //
  if (!FlowControlEnable) {
    DevReq.Value = NO_FLOW_CTRL;
  }
  if (FlowControlEnable) {
    DevReq.Value = XON_XOFF_CTRL;
  }
  //
  // set the remaining DevReq parameters and perform the usb control transfer to
  // set the flow control on the device
  //
  DevReq.Request      = FTDI_COMMAND_SET_FLOW_CTRL;
  DevReq.RequestType  = USB_REQ_TYPE_VENDOR;
  DevReq.Index        = FTDI_PORT_IDENTIFIER;
  DevReq.Length       = 0; // indicates that this transfer has no data phase
  Status              = UsbIo->UsbControlTransfer (
                                 UsbIo,
                                 &DevReq,
                                 EfiUsbDataOut,
                                 WDR_TIMEOUT,
                                 &ConfigurationValue,
                                 1,
                                 &ReturnValue
                                 );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }

  return Status;

StatusError:
  if ((Status != EFI_INVALID_PARAMETER) ||
      (Status != EFI_DEVICE_ERROR)      ||
      (Status != EFI_UNSUPPORTED)          ) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that performs a Usb Control Transfer to set the Dtr value on
  the Usb Serial Device.

  @param  UsbIo[in]                  Usb Io Protocol instance pointer
  @param  DtrEnable[in]              Data on the Enable/Disable status of the
                                     Dtr for the Usb Serial Device

  @retval EFI_SUCCESS                The Dtr value was set on the Usb Serial
                                     Device
  @retval EFI_INVALID_PARAMETER      An invalid Dtr value was used
  @retval EFI_UNSUPPORTED            The operation is not supported
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly

**/
EFI_STATUS
EFIAPI
SetDtrInternal (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN BOOLEAN              DtrEnable
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  ReturnValue;
  UINT8                   ConfigurationValue;

  //
  // set the value of DevReq.Value based on the value of DtrEnable
  //
  if (!DtrEnable) {
    DevReq.Value = SET_DTR_LOW;
  }
  if (DtrEnable) {
    DevReq.Value = SET_DTR_HIGH;
  }
  //
  // set the remaining attributes of DevReq and perform the usb control transfer
  // to set the device
  //
  DevReq.Request      = FTDI_COMMAND_MODEM_CTRL;
  DevReq.RequestType  = USB_REQ_TYPE_VENDOR;
  DevReq.Index        = FTDI_PORT_IDENTIFIER;
  DevReq.Length       = 0; // indicates that there is no data phase in this transfer

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbDataOut,
                    WDR_TIMEOUT,
                    &ConfigurationValue,
                    1,
                    &ReturnValue
                    );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }
  return Status;

StatusError:
  if ((Status != EFI_INVALID_PARAMETER) ||
      (Status != EFI_DEVICE_ERROR)      ||
      (Status != EFI_UNSUPPORTED)          ) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that performs a Usb Control Transfer to set the Dtr value on
  the Usb Serial Device.
  
  @param  UsbIo[in]                  Usb Io Protocol instance pointer
  @param  RtsEnable[in]              Data on the Enable/Disable status of the
                                     Rts for the Usb Serial Device

  @retval EFI_SUCCESS                The Rts value was set on the Usb Serial
                                     Device
  @retval EFI_INVALID_PARAMETER      An invalid Rts value was used
  @retval EFI_UNSUPPORTED            The operation is not supported
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly

**/
EFI_STATUS
EFIAPI
SetRtsInternal (
  IN EFI_USB_IO_PROTOCOL  *UsbIo,
  IN BOOLEAN              RtsEnable
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  ReturnValue;
  UINT8                   ConfigurationValue;

  //
  // set DevReq.Value based on the value of RtsEnable
  //
  if (!RtsEnable) {
    DevReq.Value = SET_RTS_LOW;
  }
  if (RtsEnable) {
    DevReq.Value = SET_RTS_HIGH;
  }

  //
  // set the remaining parameters of DevReq and perform the usb control transfer
  // to set the values on the device
  //
  DevReq.Request     = FTDI_COMMAND_MODEM_CTRL;
  DevReq.RequestType = USB_REQ_TYPE_VENDOR;
  DevReq.Index       = FTDI_PORT_IDENTIFIER;
  DevReq.Length      = 0; // indicates that there is no data phase in this request

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbDataOut,
                    WDR_TIMEOUT,
                    &ConfigurationValue,
                    1,
                    &ReturnValue
                    );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }

  return Status;

StatusError:
  if ((Status != EFI_INVALID_PARAMETER) ||
      (Status != EFI_DEVICE_ERROR)      ||
      (Status != EFI_UNSUPPORTED)          ) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that checks for valid control values and sets the control
  bits on the Usb Serial Device.

  @param  UsbSerialDevice[in]        Handle to the Usb Serial Device whose
                                     control bits are being set
  @param  Control[in]                The control value passed to the function
                                     that contains the values of the control
                                     bits that are being set

  @retval EFI_SUCCESS                The control bits were set on the Usb Serial
                                     Device
  @retval EFI_INVALID_PARAMETER      An invalid control value was encountered
  @retval EFI_EFI_UNSUPPORTED        The operation is not supported
  @retval EFI_DEVICE_ERROR           The device is not functioning correctly

**/
EFI_STATUS
EFIAPI
SetControlBitsInternal (
  IN USB_SER_DEV   *UsbSerialDevice,
  IN CONTROL_BITS  *Control
  )
{
  EFI_STATUS                    Status;
  UART_FLOW_CONTROL_DEVICE_PATH *FlowControl;
  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath;

  //
  // check for invalid control parameters hardware and software loopback enabled
  // must always be set to FALSE
  //
  Control->HardwareLoopBack = FALSE;
  Control->SoftwareLoopBack = FALSE;

  //
  // set hardware flow control
  //
  Status  = SetFlowControlInternal (
              UsbSerialDevice->UsbIo,
              Control->HardwareFlowControl
              );
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }

  //
  // set Dtr state
  //
  Status = SetDtrInternal (UsbSerialDevice->UsbIo, Control->DtrState);
  if (EFI_ERROR (Status)) {
    goto StatusError;
  }

  //
  // set Rts state
  //
  Status = SetRtsInternal (UsbSerialDevice->UsbIo, Control->RtsState);
  if (EFI_ERROR (Status)){
    goto StatusError;
  }

  //
  // update the remaining control values for UsbSerialDevice->ControlValues
  //
  UsbSerialDevice->ControlValues.DtrState            = Control->DtrState;
  UsbSerialDevice->ControlValues.RtsState            = Control->RtsState;
  UsbSerialDevice->ControlValues.HardwareFlowControl = Control->HardwareFlowControl;
  UsbSerialDevice->ControlValues.HardwareLoopBack    = FALSE;
  UsbSerialDevice->ControlValues.SoftwareLoopBack    = FALSE;

  Status = EFI_SUCCESS;
  //
  // Update the device path to have the correct flow control values
  //
  if (UsbSerialDevice->ControllerHandle != NULL) {
    RemainingDevicePath = UsbSerialDevice->DevicePath;
    while (!IsDevicePathEnd (RemainingDevicePath)) {
      FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (RemainingDevicePath);
      if (FlowControl->Header.Type == MESSAGING_DEVICE_PATH &&
          FlowControl->Header.SubType == MSG_VENDOR_DP &&
          sizeof (UART_FLOW_CONTROL_DEVICE_PATH) == DevicePathNodeLength ((EFI_DEVICE_PATH *) FlowControl)){
        if (UsbSerialDevice->ControlValues.HardwareFlowControl == TRUE) {
          FlowControl->FlowControlMap = UART_FLOW_CONTROL_HARDWARE;
        } else if (UsbSerialDevice->ControlValues.HardwareFlowControl == FALSE) {
          FlowControl->FlowControlMap = 0;
        }
        break;
      }
      RemainingDevicePath = NextDevicePathNode (RemainingDevicePath);
    }
  }

  return Status;

StatusError:
  if ((Status != EFI_INVALID_PARAMETER) ||
      (Status != EFI_DEVICE_ERROR)      ||
      (Status != EFI_UNSUPPORTED)          ) {
    return EFI_DEVICE_ERROR;
  } else {
    return Status;
  }
}

/**
  Internal function that calculates the Control value used by GetControlBits()
  based on the status and control values of the Usb Serial Device.

  @param  UsbSerialDevice[in]        Handle to the Usb Serial Devie whose status
                                     and control values are being used to set
                                     Control
  @param  Control[out]               On output the formated value of Control
                                     that has been calculated based on the
                                     control and status values of the Usb Serial
                                     Device

  @retval EFI_SUCCESS                The value of Control was successfully
                                     calculated

**/
EFI_STATUS
EFIAPI
GetControlBitsInternal (
  IN USB_SER_DEV  *UsbSerialDevice,
  OUT UINT32      *Control
  )
{
  *Control = 0;

  //
  // Check the values of UsbSerialDevice->Status Values and modify control
  // accordingly these values correspond to the modem status register
  //
  if (UsbSerialDevice->StatusValues.CtsState) {
    *Control |= EFI_SERIAL_CLEAR_TO_SEND;
  }
  if (UsbSerialDevice->StatusValues.DsrState) {
    *Control |= EFI_SERIAL_DATA_SET_READY;
  }
  if (UsbSerialDevice->StatusValues.RiState) {
    *Control |= EFI_SERIAL_RING_INDICATE;
  }
  if (UsbSerialDevice->StatusValues.SdState) {
    *Control |= EFI_SERIAL_CARRIER_DETECT;
  }

  //
  // check the values of UsbSerialDevice->ControlValues and modify control
  // accordingly these values correspond to the values of the Modem Control
  // Register
  //
  if (UsbSerialDevice->ControlValues.DtrState) {
    *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
  }
  if (UsbSerialDevice->ControlValues.RtsState) {
    *Control |= EFI_SERIAL_REQUEST_TO_SEND;
  }
  if (UsbSerialDevice->ControlValues.HardwareLoopBack) {
    *Control |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
  }
  if (UsbSerialDevice->ControlValues.HardwareFlowControl) {
    *Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }
  //
  // check if the buffer is empty since only one is being used if it is empty
  // set both the receive and transmit buffers to empty
  //
  if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }
  //
  // check for software loopback enable in UsbSerialDevice->ControlValues
  //
  if (UsbSerialDevice->ControlValues.SoftwareLoopBack) {
    *Control |= EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;
  }

  return EFI_SUCCESS;
}

/**
  Resets the USB Serial Device

  This function is the internal method for resetting the device and is called by
  SerialReset()

  @param  UsbSerialDevice[in]  A pointer to the USB Serial device

  @retval EFI_SUCCESS          The device was reset
  @retval EFI_DEVICE_ERROR     The device could not be reset

**/
EFI_STATUS
EFIAPI
ResetInternal (
  IN USB_SER_DEV  *UsbSerialDevice
  )
{
  EFI_STATUS              Status;
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT8                   ConfigurationValue;
  UINT32                  ReturnValue;

  DevReq.Request     = FTDI_COMMAND_RESET_PORT;
  DevReq.RequestType = USB_REQ_TYPE_VENDOR;
  DevReq.Value       = RESET_PORT_PURGE_RX;
  DevReq.Index       = FTDI_PORT_IDENTIFIER;
  DevReq.Length      = 0; //indicates that there is not data phase in this request

  Status = UsbSerialDevice->UsbIo->UsbControlTransfer (
                                     UsbSerialDevice->UsbIo,
                                     &DevReq,
                                     EfiUsbDataIn,
                                     WDR_TIMEOUT,
                                     &ConfigurationValue,
                                     1,
                                     &ReturnValue
                                     );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DevReq.Request     = FTDI_COMMAND_RESET_PORT;
  DevReq.RequestType = USB_REQ_TYPE_VENDOR;
  DevReq.Value       = RESET_PORT_PURGE_TX;
  DevReq.Index       = FTDI_PORT_IDENTIFIER;
  DevReq.Length      = 0; //indicates that there is no data phase in this request

  Status = UsbSerialDevice->UsbIo->UsbControlTransfer (
                                     UsbSerialDevice->UsbIo,
                                     &DevReq,
                                     EfiUsbDataIn,
                                     WDR_TIMEOUT,
                                     &ConfigurationValue,
                                     1,
                                     &ReturnValue
                                     );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  return Status;
}

/**
  Entrypoint of USB Serial Driver.

  This function is the entrypoint of USB Serial Driver. It installs
  Driver Binding Protocols together with Component Name Protocols.

  @param  ImageHandle[in]       The firmware allocated handle for the EFI image.
  @param  SystemTable[in]       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
FtdiUsbSerialEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbSerialDriverBinding,
             ImageHandle,
             &gUsbSerialComponentName,
             &gUsbSerialComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;
}

/**
  Unload function for the Usb Serial Driver.

  @param  ImageHandle[in]    The allocated handle for the EFI image

  @retval EFI_SUCCESS        The driver was unloaded successfully
**/
EFI_STATUS
EFIAPI
FtdiUsbSerialUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;
  UINTN       Index;

  //
  // Retrieve all handles in the handle database
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Disconnect the driver from the handles in the handle database
  //
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->DisconnectController (
                    HandleBuffer[Index],
                    gImageHandle,
                    NULL
                    );
  }

  //
  // Free the handle array
  //
  FreePool (HandleBuffer);

  //
  // Uninstall protocols installed by the driver in its entrypoint
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gUsbSerialDriverBinding,
                  &gEfiComponentNameProtocolGuid,
                  &gUsbSerialComponentName,
                  &gEfiComponentName2ProtocolGuid,
                  &gUsbSerialComponentName2,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Check whether USB Serial driver supports this device.

  @param  This[in]                   The USB Serial driver binding protocol.
  @param  Controller[in]             The controller handle to check.
  @param  RemainingDevicePath[in]    The remaining device path.

  @retval EFI_SUCCESS                The driver supports this controller.
  @retval other                      This device isn't supported.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_USB_IO_PROTOCOL  *UsbIo;
  UART_DEVICE_PATH     *UartNode;
  UART_FLOW_CONTROL_DEVICE_PATH        *FlowControlNode;
  UINTN                                Index;
  UINTN                                EntryCount;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  BOOLEAN                              HasFlowControl;
  EFI_DEVICE_PATH_PROTOCOL             *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL             *ParentDevicePath;

  if (RemainingDevicePath != NULL) {
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      Status = EFI_UNSUPPORTED;
      UartNode = (UART_DEVICE_PATH *) NextDevicePathNode (RemainingDevicePath);
      if (UartNode->Header.Type != MESSAGING_DEVICE_PATH ||
          UartNode->Header.SubType != MSG_UART_DP ||
          sizeof (UART_DEVICE_PATH) != DevicePathNodeLength ((EFI_DEVICE_PATH *) UartNode)) {
        goto Error;
      }
      FlowControlNode = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (UartNode);
      if ((ReadUnaligned32 (&FlowControlNode->FlowControlMap) & ~UART_FLOW_CONTROL_HARDWARE) != 0) {
        goto Error;
      }
    }
  }

  //
  // Check if USB I/O Protocol is attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    if (RemainingDevicePath == NULL || IsDevicePathEnd (RemainingDevicePath)) {
      return EFI_SUCCESS;
    }
    Status = gBS->OpenProtocolInformation (
                    Controller,
                    &gEfiUsbIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    for (Index = 0; Index < EntryCount; Index++) {
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiDevicePathProtocolGuid,
                        (VOID **) &DevicePath,
                        This->DriverBindingHandle,
                        Controller,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (!EFI_ERROR (Status)) {
          HasFlowControl = ContainsFlowControl (RemainingDevicePath);
          if (HasFlowControl ^ ContainsFlowControl (DevicePath)) {
            Status = EFI_UNSUPPORTED;
          }
        }
        break;
      }
    }
    FreePool (OpenInfoBuffer);
    return Status;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a USB Serial device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUsbSerial (UsbIo)) {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

Error:
  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

/**
  Starts the USB Serial device with this driver.

  This function produces initializes the USB Serial device and
  produces the Serial IO Protocol.

  @param  This[in]                   The USB Serial driver binding instance.
  @param  Controller[in]             Handle of device to bind driver to.
  @param  RemainingDevicePath[in]    Optional parameter use to pick a specific
                                     child device to start.

  @retval EFI_SUCCESS                The controller is controlled by the usb USB
                                     Serial driver.
  @retval EFI_UNSUPPORTED            No interrupt endpoint can be found.
  @retval Other                      This controller cannot be started.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                          Status;
  EFI_USB_IO_PROTOCOL                 *UsbIo;
  USB_SER_DEV                         *UsbSerialDevice;
  UINT8                               EndpointNumber;
  EFI_USB_ENDPOINT_DESCRIPTOR         EndpointDescriptor;
  UINT8                               Index;
  BOOLEAN                             FoundIn;
  BOOLEAN                             FoundOut;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfoBuffer;
  UINTN                               EntryCount;
  EFI_SERIAL_IO_PROTOCOL              *SerialIo;
  UART_DEVICE_PATH                    *Uart;
  UART_FLOW_CONTROL_DEVICE_PATH       *FlowControl;
  UINT32                              Control;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;

  UsbSerialDevice = AllocateZeroPool (sizeof (USB_SER_DEV));
  ASSERT (UsbSerialDevice != NULL);

  //
  // Get the Parent Device path
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    goto ErrorExit1;
  }

  //
  // Open USB I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    goto ErrorExit1;
  }

  if (Status == EFI_ALREADY_STARTED) {
    if (RemainingDevicePath == NULL || IsDevicePathEnd (RemainingDevicePath)) {
      FreePool (UsbSerialDevice);
      return EFI_SUCCESS;
    }

    //
    // Check to see if a child handle exists
    //
    Status = gBS->OpenProtocolInformation (
                    Controller,
                    &gEfiSerialIoProtocolGuid,
                    &OpenInfoBuffer,
                    &EntryCount
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorExit1;
    }

    Status = EFI_ALREADY_STARTED;
    for (Index = 0; Index < EntryCount; Index++) {
      if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
        Status = gBS->OpenProtocol (
                        OpenInfoBuffer[Index].ControllerHandle,
                        &gEfiSerialIoProtocolGuid,
                        (VOID **) &SerialIo,
                        This->DriverBindingHandle,
                        Controller,
                        EFI_OPEN_PROTOCOL_GET_PROTOCOL
                        );
        if (EFI_ERROR (Status)) {
        }
        if (!EFI_ERROR (Status)) {
          Uart = (UART_DEVICE_PATH *) RemainingDevicePath;
          Status = SerialIo->SetAttributes (
                               SerialIo,
                               Uart->BaudRate,
                               SerialIo->Mode->ReceiveFifoDepth,
                               SerialIo->Mode->Timeout,
                               (EFI_PARITY_TYPE) Uart->Parity,
                               Uart->DataBits,
                               (EFI_STOP_BITS_TYPE) Uart->StopBits
                               );
          FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (Uart);
          if (!EFI_ERROR (Status) && IsUartFlowControlNode (FlowControl)) {
            Status = SerialIo->GetControl (
                                 SerialIo,
                                 &Control
                                 );
            if (!EFI_ERROR (Status)) {
              if (ReadUnaligned32 (&FlowControl->FlowControlMap) == UART_FLOW_CONTROL_HARDWARE) {
                Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
              } else {
                Control &= ~EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
              }
              //
              // Clear bits that are not allowed to be passed to SetControl
              //
              Control &= (EFI_SERIAL_REQUEST_TO_SEND | 
                          EFI_SERIAL_DATA_TERMINAL_READY |
                          EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE |
                          EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE |
                          EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE);
              Status = SerialIo->SetControl (SerialIo, Control);
            }
          }
        }
        break;
      }
    }
    FreePool (OpenInfoBuffer);
    return Status;
  }

  if (RemainingDevicePath != NULL) {
    if (IsDevicePathEnd (RemainingDevicePath)) {
      return EFI_SUCCESS;
    }
  }

  UsbSerialDevice->UsbIo = UsbIo;

  //
  // Get interface & endpoint descriptor
  //
  UsbIo->UsbGetInterfaceDescriptor (
           UsbIo,
           &UsbSerialDevice->InterfaceDescriptor
           );

  EndpointNumber = UsbSerialDevice->InterfaceDescriptor.NumEndpoints;

  //
  // Traverse endpoints to find the IN and OUT endpoints that will send and
  // receive data.
  //
  FoundIn = FALSE;
  FoundOut = FALSE;
  for (Index = 0; Index < EndpointNumber; Index++) {

    Status = UsbIo->UsbGetEndpointDescriptor (
                      UsbIo,
                      Index,
                      &EndpointDescriptor
                      );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (EndpointDescriptor.EndpointAddress == FTDI_ENDPOINT_ADDRESS_OUT) {
      //
      // Set the Out endpoint device
      //
      CopyMem (
        &UsbSerialDevice->OutEndpointDescriptor,
        &EndpointDescriptor,
        sizeof(EndpointDescriptor)
        );
      FoundOut = TRUE;
    }

    if (EndpointDescriptor.EndpointAddress == FTDI_ENDPOINT_ADDRESS_IN) {
      //
      // Set the In endpoint device
      //
      CopyMem (
        &UsbSerialDevice->InEndpointDescriptor,
        &EndpointDescriptor,
        sizeof(EndpointDescriptor)
        );
      FoundIn = TRUE;
    }
  }

  if (!FoundIn || !FoundOut) {
    //
    // No interrupt endpoint found, then return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    goto ErrorExit;
  }
  //
  // set the initial values of UsbSerialDevice->LastSettings to the default
  // values
  //
  UsbSerialDevice->LastSettings.BaudRate         = 115200;
  UsbSerialDevice->LastSettings.DataBits         = 8;
  UsbSerialDevice->LastSettings.Parity           = NoParity;
  UsbSerialDevice->LastSettings.ReceiveFifoDepth = FTDI_MAX_RECEIVE_FIFO_DEPTH;
  UsbSerialDevice->LastSettings.StopBits         = OneStopBit;
  UsbSerialDevice->LastSettings.Timeout          = FTDI_TIMEOUT;

  //
  // set the initial values of UsbSerialDevice->ControlValues
  //
  UsbSerialDevice->ControlValues.DtrState            = FALSE;
  UsbSerialDevice->ControlValues.RtsState            = FALSE;
  UsbSerialDevice->ControlValues.HardwareFlowControl = FALSE;
  UsbSerialDevice->ControlValues.HardwareLoopBack    = FALSE;
  UsbSerialDevice->ControlValues.SoftwareLoopBack    = FALSE;

  //
  // set the values of UsbSerialDevice->UartDevicePath
  //
  UsbSerialDevice->UartDevicePath.Header.Type    = MESSAGING_DEVICE_PATH;
  UsbSerialDevice->UartDevicePath.Header.SubType = MSG_UART_DP;
  UsbSerialDevice->UartDevicePath.Header.Length[0] = (UINT8) (sizeof (UART_DEVICE_PATH));
  UsbSerialDevice->UartDevicePath.Header.Length[1] = (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8);

  //
  // set the values of UsbSerialDevice->FlowControlDevicePath
  UsbSerialDevice->FlowControlDevicePath.Header.Type = MESSAGING_DEVICE_PATH;
  UsbSerialDevice->FlowControlDevicePath.Header.SubType = MSG_VENDOR_DP;
  UsbSerialDevice->FlowControlDevicePath.Header.Length[0] = (UINT8) (sizeof (UART_FLOW_CONTROL_DEVICE_PATH));
  UsbSerialDevice->FlowControlDevicePath.Header.Length[1] = (UINT8) ((sizeof (UART_FLOW_CONTROL_DEVICE_PATH)) >> 8);
  UsbSerialDevice->FlowControlDevicePath.FlowControlMap = 0;

  Status = SetAttributesInternal (
             UsbSerialDevice, 
             UsbSerialDevice->LastSettings.BaudRate,
             UsbSerialDevice->LastSettings.ReceiveFifoDepth, 
             UsbSerialDevice->LastSettings.Timeout,
             UsbSerialDevice->LastSettings.Parity, 
             UsbSerialDevice->LastSettings.DataBits,
             UsbSerialDevice->LastSettings.StopBits
             );

  ASSERT_EFI_ERROR (Status);

  Status = SetControlBitsInternal (
             UsbSerialDevice,
             &(UsbSerialDevice->ControlValues)
             );

  ASSERT_EFI_ERROR (Status);

  //
  // Publish Serial GUID and protocol
  //

  UsbSerialDevice->Signature              = USB_SER_DEV_SIGNATURE;
  UsbSerialDevice->SerialIo.Reset         = SerialReset;
  UsbSerialDevice->SerialIo.SetControl    = SetControlBits;
  UsbSerialDevice->SerialIo.SetAttributes = SetAttributes;
  UsbSerialDevice->SerialIo.GetControl    = GetControlBits;
  UsbSerialDevice->SerialIo.Read          = ReadSerialIo;
  UsbSerialDevice->SerialIo.Write         = WriteSerialIo;

  //
  // Set the static Serial IO modes that will display when running
  // "sermode" within the UEFI shell.
  //

  UsbSerialDevice->SerialIo.Mode->Timeout  = 0;
  UsbSerialDevice->SerialIo.Mode->BaudRate = 115200;
  UsbSerialDevice->SerialIo.Mode->DataBits = 8;
  UsbSerialDevice->SerialIo.Mode->Parity   = 1;
  UsbSerialDevice->SerialIo.Mode->StopBits = 1;

  UsbSerialDevice->ParentDevicePath = ParentDevicePath;
  UsbSerialDevice->ControllerHandle = NULL;
  FlowControl                       = NULL;

  //
  // Allocate space for the receive buffer
  //
  UsbSerialDevice->DataBuffer = AllocateZeroPool (SW_FIFO_DEPTH);

  //
  // Initialize data buffer pointers.
  // Head==Tail = true means buffer is empty.
  //
  UsbSerialDevice->DataBufferHead = 0;
  UsbSerialDevice->DataBufferTail = 0;

  UsbSerialDevice->ControllerNameTable = NULL;
  AddUnicodeString2 (
    "eng",
    gUsbSerialComponentName.SupportedLanguages,
    &UsbSerialDevice->ControllerNameTable,
    L"FTDI USB Serial Adapter",
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gUsbSerialComponentName2.SupportedLanguages,
    &UsbSerialDevice->ControllerNameTable,
    L"FTDI USB Serial Adapter",
    FALSE
    );

  Status = SetInitialStatus (UsbSerialDevice);
  ASSERT_EFI_ERROR (Status);

  //
  // Create a polling loop to check for input
  //

  gBS->CreateEvent (
         EVT_TIMER | EVT_NOTIFY_SIGNAL,
         TPL_CALLBACK,
         UsbSerialDriverCheckInput,
         UsbSerialDevice,
         &(UsbSerialDevice->PollingLoop)
         );
  //
  // add code to set trigger time based on baud rate
  // setting to 0.5s for now
  //
  gBS->SetTimer (
         UsbSerialDevice->PollingLoop,
         TimerPeriodic,
         EFI_TIMER_PERIOD_MILLISECONDS (500)
         );

  //
  // Check if the remaining device path is null. If it is not null change the settings
  // of the device to match those on the device path
  //
  if (RemainingDevicePath != NULL) {
    CopyMem (
      &UsbSerialDevice->UartDevicePath,
      RemainingDevicePath,
      sizeof (UART_DEVICE_PATH)
      );
    FlowControl = (UART_FLOW_CONTROL_DEVICE_PATH *) NextDevicePathNode (RemainingDevicePath);
    if (IsUartFlowControlNode (FlowControl)) {
      UsbSerialDevice->FlowControlDevicePath.FlowControlMap = ReadUnaligned32 (&FlowControl->FlowControlMap);
    } else {
      FlowControl = NULL;
    }
  }

  //
  // Build the device path by appending the UART node to the parent device path
  //
  UsbSerialDevice->DevicePath = AppendDevicePathNode (
                                  ParentDevicePath,
                                  (EFI_DEVICE_PATH_PROTOCOL *) &UsbSerialDevice->UartDevicePath
                                  );
  //
  // Continue building the device path by appending the flow control node
  //
  TempDevicePath = UsbSerialDevice->DevicePath;
  UsbSerialDevice->DevicePath = AppendDevicePathNode (
                                  TempDevicePath,
                                  (EFI_DEVICE_PATH_PROTOCOL *) &UsbSerialDevice->FlowControlDevicePath
                                  );
  FreePool (TempDevicePath);

  if (UsbSerialDevice->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  //
  // Install protocol interfaces for the device
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &UsbSerialDevice->ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  UsbSerialDevice->DevicePath,
                  &gEfiSerialIoProtocolGuid,
                  &UsbSerialDevice->SerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)){
    goto ErrorExit;
  }

  //
  // Open for child device
  //
  Status = gBS->OpenProtocol (
                 Controller,
                 &gEfiUsbIoProtocolGuid,
                 (VOID **) &UsbIo,
                 This->DriverBindingHandle,
                 UsbSerialDevice->ControllerHandle,
                 EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                 );

  UsbSerialDevice->Shutdown = FALSE;

  return EFI_SUCCESS;

ErrorExit:
  //
  // Error handler
  //

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiSerialIoProtocolGuid,
                  &UsbSerialDevice->SerialIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit1;
  }

  FreePool (UsbSerialDevice->DataBuffer);
  FreePool (UsbSerialDevice);

  UsbSerialDevice = NULL;
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

ErrorExit1:
  return Status;
}

/**
  Stop the USB Serial device handled by this driver.

  @param  This[in]                   The USB Serial driver binding protocol.
  @param  Controller[in]             The controller to release.
  @param  NumberOfChildren[in]       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer[in]      The array of child handle.

  @retval EFI_SUCCESS                The device was stopped.
  @retval EFI_UNSUPPORTED            Serial IO Protocol is not installed on
                                     Controller.
  @retval EFI_DEVICE_ERROR           The device could not be stopped due to a
                                     device error.
  @retval Others                     Fail to uninstall protocols attached on the
                                     device.

**/
EFI_STATUS
EFIAPI
UsbSerialDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  EFI_SERIAL_IO_PROTOCOL    *SerialIo;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  USB_SER_DEV               *UsbSerialDevice;
  UINTN                     Index;
  BOOLEAN                   AllChildrenStopped;

  Status = EFI_SUCCESS;
  UsbSerialDevice = NULL;

  if (NumberOfChildren == 0) {
    //
    // Close the driver
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiUsbIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    return Status;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren ;Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSerialIoProtocolGuid,
                    (VOID **) &SerialIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (Status == EFI_SUCCESS) {//!EFI_ERROR (Status)) {
      UsbSerialDevice = USB_SER_DEV_FROM_THIS (SerialIo);
      Status = gBS->CloseProtocol (
                      Controller,
                      &gEfiUsbIoProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      ChildHandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      UsbSerialDevice->DevicePath,
                      &gEfiSerialIoProtocolGuid,
                      &UsbSerialDevice->SerialIo,
                      NULL
                      );

      if (EFI_ERROR (Status)) {
        gBS->OpenProtocol (
               Controller,
               &gEfiUsbIoProtocolGuid,
               (VOID **) &UsbIo,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );
      } else {
        if (UsbSerialDevice->DevicePath != NULL) {
          gBS->FreePool (UsbSerialDevice->DevicePath);
        }
        gBS->SetTimer (
               UsbSerialDevice->PollingLoop,
               TimerCancel,
               0
               );
        gBS->CloseEvent (UsbSerialDevice->PollingLoop);
        UsbSerialDevice->Shutdown = TRUE;
        FreeUnicodeStringTable (UsbSerialDevice->ControllerNameTable);
        FreePool (UsbSerialDevice->DataBuffer);
        FreePool (UsbSerialDevice);
      }
    }
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

//
// Serial IO Member Functions
//

/**
  Reset the serial device.

  @param  This[in]              Protocol instance pointer.

  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The serial device could not be reset.

**/
EFI_STATUS
EFIAPI
SerialReset (
  IN EFI_SERIAL_IO_PROTOCOL  *This
  )
{
  EFI_STATUS    Status;
  USB_SER_DEV  *UsbSerialDevice;

  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);
  Status          = ResetInternal (UsbSerialDevice);
  if (EFI_ERROR (Status)){
    return EFI_DEVICE_ERROR;
  }
  return Status;
}

/**
  Set the control bits on a serial device.

  @param  This[in]             Protocol instance pointer.
  @param  Control[in]          Set the bits of Control that are settable.

  @retval EFI_SUCCESS          The new control bits were set on the serial device.
  @retval EFI_UNSUPPORTED      The serial device does not support this operation.
  @retval EFI_DEVICE_ERROR     The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
SetControlBits (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT32                  Control
  )
{
  EFI_STATUS    Status;
  USB_SER_DEV   *UsbSerialDevice;
  CONTROL_BITS  ControlBits;
  
  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);
  
  //
  // check for invalid control parameters 
  //
  if ((Control & (~(EFI_SERIAL_REQUEST_TO_SEND          |
                    EFI_SERIAL_DATA_TERMINAL_READY      |
                    EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE |
                    EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE |
                    EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE))) != 0 ) {
    return EFI_UNSUPPORTED;
  }

  //
  // check the control parameters and set the correct setting for
  // the paramerts of ControlBits
  // both loopback enables are always set to FALSE
  //
  ControlBits.HardwareLoopBack = FALSE;
  ControlBits.SoftwareLoopBack = FALSE;
  //
  // check for hardware flow control
  //
  if ((Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) == EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) {
    ControlBits.HardwareFlowControl = TRUE;
  } else {
    ControlBits.HardwareFlowControl = FALSE;
  }
  //
  // check for DTR enabled
  //
  if ((Control & EFI_SERIAL_DATA_TERMINAL_READY) == EFI_SERIAL_DATA_TERMINAL_READY) {
    ControlBits.DtrState = TRUE;
  } else {
    ControlBits.DtrState = FALSE;
  }
  //
  // check for RTS enabled
  //
  if ((Control & EFI_SERIAL_REQUEST_TO_SEND) == EFI_SERIAL_REQUEST_TO_SEND) {
    ControlBits.RtsState = TRUE;
  } else {
    ControlBits.RtsState = FALSE;
  }

  //
  // set the control values with a call to SetControlBitsInternal()
  //
  Status = SetControlBitsInternal (UsbSerialDevice, &ControlBits);

  return Status;
}

/**
  calls SetAttributesInternal() to set the baud rate, receive FIFO depth,
  transmit/receive time out, parity, data buts, and stop bits on a serial
  device.

  @param  This[in]             Protocol instance pointer.
  @param  BaudRate[in]         The requested baud rate. A BaudRate value of 0
                               will use the device's default interface speed.
  @param  ReveiveFifoDepth[in] The requested depth of the FIFO on the receive
                               side of the serial interface. A ReceiveFifoDepth
                               value of 0 will use the device's default FIFO
                               depth.
  @param  Timeout[in]          The requested time out for a single character in
                               microseconds.This timeout applies to both the
                               transmit and receive side of the interface. A
                               Timeout value of 0 will use the device's default
                               time out value.
  @param  Parity[in]           The type of parity to use on this serial device.
                               A Parity value of DefaultParity will use the
                               device's default parity value.
  @param  DataBits[in]         The number of data bits to use on the serial
                               device. A DataBit vaule of 0 will use the
                               device's default data bit setting.
  @param  StopBits[in]         The number of stop bits to use on this serial
                               device. A StopBits value of DefaultStopBits will
                               use the device's default number of stop bits.

  @retval EFI_SUCCESS          The attributes were set
  @retval EFI_DEVICE_ERROR     The attributes were not able to be

**/
EFI_STATUS
EFIAPI
SetAttributes (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN UINT64                  BaudRate,
  IN UINT32                  ReceiveFifoDepth,
  IN UINT32                  Timeout,
  IN EFI_PARITY_TYPE         Parity,
  IN UINT8                   DataBits,
  IN EFI_STOP_BITS_TYPE      StopBits
  )
{

  EFI_STATUS   Status;
  USB_SER_DEV  *UsbSerialDevice;

  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);

  Status = SetAttributesInternal (
             UsbSerialDevice,
             BaudRate,
             ReceiveFifoDepth,
             Timeout,
             Parity,
             DataBits,
             StopBits
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return Status;
}


/**
  Retrieves the status of the control bits on a serial device.

  @param  This[in]               Protocol instance pointer.
  @param  Control[out]           A pointer to return the current Control signals
                                 from the serial device.

  @retval EFI_SUCCESS            The control bits were read from the serial
                                 device.
  @retval EFI_DEVICE_ERROR       The serial device is not functioning correctly.

**/
EFI_STATUS
EFIAPI
GetControlBits (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  OUT UINT32                 *Control
  )
{
  USB_SER_DEV  *UsbSerialDevice;
  EFI_STATUS   Status;

  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);

  *Control        = 0;

  Status = GetControlBitsInternal (UsbSerialDevice, Control);

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  return Status;
}

/**
  Reads data from a serial device.

  @param  This[in]                   Protocol instance pointer.
  @param  BufferSize[in, out]        On input, the size of the Buffer. On output,
                                     the amount of data returned in Buffer.
  @param  Buffer[out]                The buffer to return the data into.

  @retval EFI_SUCCESS                The data was read.
  @retval EFI_DEVICE_ERROR           The device reported an error.
  @retval EFI_TIMEOUT                The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
ReadSerialIo (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  OUT VOID                   *Buffer
  )
{
  UINTN        Index;
  UINTN        RemainingCallerBufferSize;
  USB_SER_DEV  *UsbSerialDevice;
  EFI_STATUS   Status;


  if (*BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (Buffer == NULL) {
    return EFI_DEVICE_ERROR;
  }

  Status          = EFI_SUCCESS;
  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);

  //
  // Clear out any data that we already have in our internal buffer
  //
  for (Index = 0; Index < *BufferSize; Index++) {
    if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
      break;
    }

    //
    // Still have characters in the buffer to return
    //
    ((UINT8 *)Buffer)[Index] = UsbSerialDevice->DataBuffer[UsbSerialDevice->DataBufferHead];
    UsbSerialDevice->DataBufferHead = (UsbSerialDevice->DataBufferHead + 1) % SW_FIFO_DEPTH;
  }

  //
  // If we haven't filled the caller's buffer using data that we already had on
  // hand We need to generate an additional USB request to try and fill the
  // caller's buffer
  //
  if (Index != *BufferSize) {
    RemainingCallerBufferSize = *BufferSize - Index;
    Status = ReadDataFromUsb (
               UsbSerialDevice,
               &RemainingCallerBufferSize,
               (VOID *)(((CHAR8 *)Buffer) + Index)
               );
    if (!EFI_ERROR (Status)) {
      *BufferSize = RemainingCallerBufferSize + Index;
    } else {
      *BufferSize = Index;
    }
  }

  if (UsbSerialDevice->DataBufferHead == UsbSerialDevice->DataBufferTail) {
    //
    // Data buffer has no data, set the EFI_SERIAL_INPUT_BUFFER_EMPTY flag
    //
    UsbSerialDevice->ControlBits |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  } else {
    //
    // There is some leftover data, clear EFI_SERIAL_INPUT_BUFFER_EMPTY flag
    //
    UsbSerialDevice->ControlBits &= ~(EFI_SERIAL_INPUT_BUFFER_EMPTY);
  }
  return Status;
}

/**
  Writes data to a serial device.

  @param  This[in]                   Protocol instance pointer.
  @param  BufferSize[in, out]        On input, the size of the Buffer. On output,
                                     the amount of data actually written.
  @param  Buffer[in]                 The buffer of data to write

  @retval EFI_SUCCESS                The data was written.
  @retval EFI_DEVICE_ERROR           The device reported an error.
  @retval EFI_TIMEOUT                The data write was stopped due to a timeout.

**/
EFI_STATUS
EFIAPI
WriteSerialIo (
  IN EFI_SERIAL_IO_PROTOCOL  *This,
  IN OUT UINTN               *BufferSize,
  IN VOID                    *Buffer
  )
{
  EFI_STATUS   Status;
  USB_SER_DEV  *UsbSerialDevice;
  EFI_TPL      Tpl;

  UsbSerialDevice = USB_SER_DEV_FROM_THIS (This);

  if (UsbSerialDevice->Shutdown) {
    return EFI_DEVICE_ERROR;
  }

  Tpl = gBS->RaiseTPL (TPL_NOTIFY);

  Status = UsbSerialDataTransfer (
             UsbSerialDevice,
             EfiUsbDataOut,
             Buffer,
             BufferSize,
             FTDI_TIMEOUT
             );

  gBS->RestoreTPL (Tpl);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_TIMEOUT){
      return Status;
    } else {
      return EFI_DEVICE_ERROR;
    }
  }

  return EFI_SUCCESS;
}
