/** @file
  Implementation of the USB mass storage Control/Bulk/Interrupt transport,
  according to USB Mass Storage Class Control/Bulk/Interrupt (CBI) Transport, Revision 1.1.
  Notice: it is being obsoleted by the standard body in favor of the BOT
  (Bulk-Only Transport).

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMass.h"

//
// Definition of USB CBI0 Transport Protocol
//
USB_MASS_TRANSPORT mUsbCbi0Transport = {
  USB_MASS_STORE_CBI0,
  UsbCbiInit,
  UsbCbiExecCommand,
  UsbCbiResetDevice,
  NULL,
  UsbCbiCleanUp
};

//
// Definition of USB CBI1 Transport Protocol
//
USB_MASS_TRANSPORT mUsbCbi1Transport = {
  USB_MASS_STORE_CBI1,
  UsbCbiInit,
  UsbCbiExecCommand,
  UsbCbiResetDevice,
  NULL,
  UsbCbiCleanUp
};

/**
  Initializes USB CBI protocol.

  This function initializes the USB mass storage class CBI protocol.
  It will save its context which is a USB_CBI_PROTOCOL structure
  in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB CBI initialization fails.

**/
EFI_STATUS
UsbCbiInit (
  IN  EFI_USB_IO_PROTOCOL   *UsbIo,
  OUT VOID                  **Context       OPTIONAL
  )
{
  USB_CBI_PROTOCOL              *UsbCbi;
  EFI_USB_INTERFACE_DESCRIPTOR  *Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndPoint;
  EFI_STATUS                    Status;
  UINT8                         Index;

  //
  // Allocate the CBI context for USB_CBI_PROTOCOL and 3 endpoint descriptors.
  //
  UsbCbi = AllocateZeroPool (
             sizeof (USB_CBI_PROTOCOL) + 3 * sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
             );
  ASSERT (UsbCbi != NULL);

  UsbCbi->UsbIo = UsbIo;

  //
  // Get the interface descriptor and validate that it
  // is a USB Mass Storage CBI interface.
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &UsbCbi->Interface);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Interface = &UsbCbi->Interface;
  if ((Interface->InterfaceProtocol != USB_MASS_STORE_CBI0)
      && (Interface->InterfaceProtocol != USB_MASS_STORE_CBI1)) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  //
  // Locate and save the bulk-in, bulk-out, and interrupt endpoint
  //
  for (Index = 0; Index < Interface->NumEndpoints; Index++) {
    Status = UsbIo->UsbGetEndpointDescriptor (UsbIo, Index, &EndPoint);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (USB_IS_BULK_ENDPOINT (EndPoint.Attributes)) {
      //
      // Use the first Bulk-In and Bulk-Out endpoints
      //
      if (USB_IS_IN_ENDPOINT (EndPoint.EndpointAddress) &&
         (UsbCbi->BulkInEndpoint == NULL)) {

        UsbCbi->BulkInEndpoint  = (EFI_USB_ENDPOINT_DESCRIPTOR *) (UsbCbi + 1);
        CopyMem(UsbCbi->BulkInEndpoint, &EndPoint, sizeof (EndPoint));;
      }

      if (USB_IS_OUT_ENDPOINT (EndPoint.EndpointAddress) &&
         (UsbCbi->BulkOutEndpoint == NULL)) {

        UsbCbi->BulkOutEndpoint   = (EFI_USB_ENDPOINT_DESCRIPTOR *) (UsbCbi + 1) + 1;
        CopyMem(UsbCbi->BulkOutEndpoint, &EndPoint, sizeof (EndPoint));
      }
    } else if (USB_IS_INTERRUPT_ENDPOINT (EndPoint.Attributes)) {
      //
      // Use the first interrupt endpoint if it is CBI0
      //
      if ((Interface->InterfaceProtocol == USB_MASS_STORE_CBI0) &&
          (UsbCbi->InterruptEndpoint == NULL)) {

        UsbCbi->InterruptEndpoint   = (EFI_USB_ENDPOINT_DESCRIPTOR *) (UsbCbi + 1) + 2;
        CopyMem(UsbCbi->InterruptEndpoint, &EndPoint, sizeof (EndPoint));
      }
    }
  }

  if ((UsbCbi->BulkInEndpoint == NULL) || (UsbCbi->BulkOutEndpoint == NULL)) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }
  if ((Interface->InterfaceProtocol == USB_MASS_STORE_CBI0) && (UsbCbi->InterruptEndpoint == NULL)) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  if (Context != NULL) {
    *Context = UsbCbi;
  } else {
    FreePool (UsbCbi);
  }
 
  return EFI_SUCCESS;

ON_ERROR:
  FreePool (UsbCbi);
  return Status;
}

/**
  Send the command to the device using class specific control transfer.

  This function sends command to the device using class specific control transfer.
  The CBI contains three phases: Command, Data, and Status. This is Command phase.

  @param  UsbCbi                The USB CBI protocol
  @param  Cmd                   The high level command to transfer to device
  @param  CmdLen                The length of the command
  @param  Timeout               The time to wait the command to finish

  @retval EFI_SUCCESS           The command is sent to the device.
  @retval Others                The command failed to transfer to device

**/
EFI_STATUS
UsbCbiSendCommand (
  IN USB_CBI_PROTOCOL       *UsbCbi,
  IN UINT8                  *Cmd,
  IN UINT8                  CmdLen,
  IN UINT32                 Timeout
  )
{
  EFI_USB_DEVICE_REQUEST  Request;
  EFI_STATUS              Status;
  UINT32                  TransStatus;
  UINTN                   DataLen;
  INTN                    Retry;

  //
  // Fill in the device request, CBI use the "Accept Device-Specific
  // Cmd" (ADSC) class specific request to send commands.
  //
  Request.RequestType = 0x21;
  Request.Request     = 0;
  Request.Value       = 0;
  Request.Index       = UsbCbi->Interface.InterfaceNumber;
  Request.Length      = CmdLen;

  Status              = EFI_SUCCESS;
  Timeout             = Timeout / USB_MASS_1_MILLISECOND;

  for (Retry = 0; Retry < USB_CBI_MAX_RETRY; Retry++) {
    //
    // Use USB I/O Protocol to send the command to the device
    //
    TransStatus = 0;
    DataLen     = CmdLen;

    Status = UsbCbi->UsbIo->UsbControlTransfer (
                              UsbCbi->UsbIo,
                              &Request,
                              EfiUsbDataOut,
                              Timeout,
                              Cmd,
                              DataLen,
                              &TransStatus
                              );
    //
    // The device can fail the command by STALL the control endpoint.
    // It can delay the command by NAK the data or status stage, this
    // is a "class-specific exemption to the USB specification". Retry
    // if the command is NAKed.
    //
    if (EFI_ERROR (Status) && (TransStatus == EFI_USB_ERR_NAK)) {
      continue;
    }

    break;
  }

  return Status;
}


/**
  Transfer data between the device and host.

  This function transfers data between the device and host.
  The CBI contains three phases: Command, Data, and Status. This is Data phase.

  @param  UsbCbi                The USB CBI device
  @param  DataDir               The direction of the data transfer
  @param  Data                  The buffer to hold the data for input or output.
  @param  TransLen              On input, the expected transfer length.
                                On output, the length of data actually transferred.
  @param  Timeout               The time to wait for the command to execute

  @retval EFI_SUCCESS           The data transferred successfully.
  @retval EFI_SUCCESS           No data to transfer
  @retval Others                Failed to transfer all the data

**/
EFI_STATUS
UsbCbiDataTransfer (
  IN USB_CBI_PROTOCOL         *UsbCbi,
  IN EFI_USB_DATA_DIRECTION   DataDir,
  IN OUT UINT8                *Data,
  IN OUT UINTN                *TransLen,
  IN UINT32                   Timeout
  )
{
  EFI_USB_ENDPOINT_DESCRIPTOR *Endpoint;
  EFI_STATUS                  Status;
  UINT32                      TransStatus;
  UINTN                       Remain;
  UINTN                       Increment;
  UINT8                       *Next;
  UINTN                       Retry;

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
    Endpoint = UsbCbi->BulkInEndpoint;
  } else {
    Endpoint = UsbCbi->BulkOutEndpoint;
  }

  Next    = Data;
  Remain  = *TransLen;
  Retry   = 0;
  Status  = EFI_SUCCESS;
  Timeout = Timeout / USB_MASS_1_MILLISECOND;

  //
  // Transfer the data with a loop. The length of data transferred once is restricted.
  //
  while (Remain > 0) {
    TransStatus = 0;

    if (Remain > (UINTN) USB_CBI_MAX_PACKET_NUM * Endpoint->MaxPacketSize) {
      Increment = USB_CBI_MAX_PACKET_NUM * Endpoint->MaxPacketSize;
    } else {
      Increment = Remain;
    }

    Status = UsbCbi->UsbIo->UsbBulkTransfer (
                              UsbCbi->UsbIo,
                              Endpoint->EndpointAddress,
                              Next,
                              &Increment,
                              Timeout,
                              &TransStatus
                              );
    if (EFI_ERROR (Status)) {
      if (TransStatus == EFI_USB_ERR_NAK) {
        //
        // The device can NAK the host if either the data/buffer isn't
        // aviable or the command is in-progress.
        // If data are partially transferred, we just ignore NAK and continue.
        // If all data have been transferred and status is NAK, then we retry for several times.
        // If retry exceeds the USB_CBI_MAX_RETRY, then return error status.
        //
        if (Increment == 0) {
          if (++Retry > USB_CBI_MAX_RETRY) {
            goto ON_EXIT;
          }
        } else {
          Next   += Increment;
          Remain -= Increment;
          Retry   = 0;
        }

        continue;
      }

      //
      // The device can fail the command by STALL the bulk endpoint.
      // Clear the stall if that is the case.
      //
      if (TransStatus == EFI_USB_ERR_STALL) {
        UsbClearEndpointStall (UsbCbi->UsbIo, Endpoint->EndpointAddress);
      }

      goto ON_EXIT;
    }

    Next += Increment;
    Remain -= Increment;
  }

ON_EXIT:
  *TransLen -= Remain;
  return Status;
}


/**
  Gets the result of high level command execution from interrupt endpoint.

  This function returns the USB transfer status, and put the high level
  command execution result in Result.
  The CBI contains three phases: Command, Data, and Status. This is Status phase.

  @param  UsbCbi                The USB CBI protocol
  @param  Timeout               The time to wait for the command to execute
  @param  Result                The result of the command execution.

  @retval EFI_SUCCESS           The high level command execution result is
                                retrieved in Result.
  @retval Others                Failed to retrieve the result.

**/
EFI_STATUS
UsbCbiGetStatus (
  IN  USB_CBI_PROTOCOL        *UsbCbi,
  IN  UINT32                  Timeout,
  OUT USB_CBI_STATUS          *Result
  )
{
  UINTN                     Len;
  UINT8                     Endpoint;
  EFI_STATUS                Status;
  UINT32                    TransStatus;
  INTN                      Retry;

  Endpoint  = UsbCbi->InterruptEndpoint->EndpointAddress;
  Status    = EFI_SUCCESS;
  Timeout   = Timeout / USB_MASS_1_MILLISECOND;

  //
  // Attemp to the read the result from interrupt endpoint
  //
  for (Retry = 0; Retry < USB_CBI_MAX_RETRY; Retry++) {
    TransStatus = 0;
    Len         = sizeof (USB_CBI_STATUS);

    Status = UsbCbi->UsbIo->UsbSyncInterruptTransfer (
                              UsbCbi->UsbIo,
                              Endpoint,
                              Result,
                              &Len,
                              Timeout,
                              &TransStatus
                              );
    //
    // The CBI can NAK the interrupt endpoint if the command is in-progress.
    //
    if (EFI_ERROR (Status) && (TransStatus == EFI_USB_ERR_NAK)) {
      continue;
    }

    break;
  }

  return Status;
}


/**
  Execute USB mass storage command through the CBI0/CBI1 transport protocol.

  @param  Context               The USB CBI Protocol.
  @param  Cmd                   The command to transfer to device
  @param  CmdLen                The length of the command
  @param  DataDir               The direction of data transfer
  @param  Data                  The buffer to hold the data
  @param  DataLen               The length of the buffer
  @param  Lun                   Should be 0, this field for bot only
  @param  Timeout               The time to wait
  @param  CmdStatus             The result of the command execution

  @retval EFI_SUCCESS           The command is executed successfully.
  @retval Other                 Failed to execute the command

**/
EFI_STATUS
UsbCbiExecCommand (
  IN  VOID                    *Context,
  IN  VOID                    *Cmd,
  IN  UINT8                   CmdLen,
  IN  EFI_USB_DATA_DIRECTION  DataDir,
  IN  VOID                    *Data,
  IN  UINT32                  DataLen,
  IN  UINT8                   Lun,
  IN  UINT32                  Timeout,
  OUT UINT32                  *CmdStatus
  )
{
  USB_CBI_PROTOCOL          *UsbCbi;
  USB_CBI_STATUS            Result;
  EFI_STATUS                Status;
  UINTN                     TransLen;

  *CmdStatus  = USB_MASS_CMD_SUCCESS;
  UsbCbi      = (USB_CBI_PROTOCOL *) Context;

  //
  // Send the command to the device. Return immediately if device
  // rejects the command.
  //
  Status = UsbCbiSendCommand (UsbCbi, Cmd, CmdLen, Timeout);
  if (EFI_ERROR (Status)) {
    gBS->Stall(10 * USB_MASS_1_MILLISECOND);
    DEBUG ((EFI_D_ERROR, "UsbCbiExecCommand: UsbCbiSendCommand (%r)\n",Status));
    return Status;
  }

  //
  // Transfer the data. Return this status if no interrupt endpoint
  // is used to report the transfer status.
  //
  TransLen = (UINTN) DataLen;

  Status   = UsbCbiDataTransfer (UsbCbi, DataDir, Data, &TransLen, Timeout);
  if (UsbCbi->InterruptEndpoint == NULL) {
    DEBUG ((EFI_D_ERROR, "UsbCbiExecCommand: UsbCbiDataTransfer (%r)\n",Status));
    return Status;
  }

  //
  // Get the status. If it succeeds, interpret the result.
  //
  Status = UsbCbiGetStatus (UsbCbi, Timeout, &Result);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UsbCbiExecCommand: UsbCbiGetStatus (%r)\n",Status));
    return Status;
  }

  if (UsbCbi->Interface.InterfaceSubClass == USB_MASS_STORE_UFI) {
    //
    // For UFI device, ASC and ASCQ are returned.
    //
    // Do not set the USB_MASS_CMD_FAIL for a request sense command
    // as a bad result type doesn't mean a cmd failure
    //
    if (Result.Type != 0 && *(UINT8*)Cmd != 0x03) {
      *CmdStatus = USB_MASS_CMD_FAIL;
    }
  } else {
    //
    // Check page 27, CBI spec 1.1 for vaious reture status.
    //
    switch (Result.Value & 0x03) {
    case 0x00:
      //
      // Pass
      //
      *CmdStatus = USB_MASS_CMD_SUCCESS;
      break;

    case 0x02:
      //
      // Phase Error, response with reset.
      // No break here to fall through to "Fail".
      //
      UsbCbiResetDevice (UsbCbi, FALSE);

    case 0x01:
      //
      // Fail
      //
      *CmdStatus = USB_MASS_CMD_FAIL;
      break;

    case 0x03:
      //
      // Persistent Fail. Need to send REQUEST SENSE.
      //
      *CmdStatus = USB_MASS_CMD_PERSISTENT;
      break;
    }
  }

  return EFI_SUCCESS;
}


/**
  Reset the USB mass storage device by CBI protocol.

  This function resets the USB mass storage device by CBI protocol.
  The reset is defined as a non-data command. Don't use UsbCbiExecCommand
  to send the command to device because that may introduce recursive loop.

  @param  Context               The USB CBI protocol
  @param  ExtendedVerification  The flag controlling the rule of reset.
                                Not used here.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device.

**/
EFI_STATUS
UsbCbiResetDevice (
  IN  VOID                    *Context,
  IN  BOOLEAN                  ExtendedVerification
  )
{
  UINT8                     ResetCmd[USB_CBI_RESET_CMD_LEN];
  USB_CBI_PROTOCOL          *UsbCbi;
  USB_CBI_STATUS            Result;
  EFI_STATUS                Status;
  UINT32                    Timeout;

  UsbCbi = (USB_CBI_PROTOCOL *) Context;

  //
  // Fill in the reset command.
  //
  SetMem (ResetCmd, USB_CBI_RESET_CMD_LEN, 0xFF);

  ResetCmd[0] = 0x1D;
  ResetCmd[1] = 0x04;
  Timeout     = USB_CBI_RESET_DEVICE_TIMEOUT / USB_MASS_1_MILLISECOND;

  //
  // Send the command to the device. Don't use UsbCbiExecCommand here.
  //
  Status = UsbCbiSendCommand (UsbCbi, ResetCmd, USB_CBI_RESET_CMD_LEN, Timeout);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Just retrieve the status and ignore that. Then stall
  // 50ms to wait for it to complete.
  //
  UsbCbiGetStatus (UsbCbi, Timeout, &Result);
  gBS->Stall (USB_CBI_RESET_DEVICE_STALL);

  //
  // Clear the Bulk-In and Bulk-Out stall condition and init data toggle.
  //
  UsbClearEndpointStall (UsbCbi->UsbIo, UsbCbi->BulkInEndpoint->EndpointAddress);
  UsbClearEndpointStall (UsbCbi->UsbIo, UsbCbi->BulkOutEndpoint->EndpointAddress);

  return Status;
}


/**
  Clean up the CBI protocol's resource.

  @param  Context               The instance of CBI protocol.

  @retval EFI_SUCCESS           The resource is cleaned up.

**/
EFI_STATUS
UsbCbiCleanUp (
  IN  VOID                   *Context
  )
{
  FreePool (Context);
  return EFI_SUCCESS;
}
