/** @file
  PCIe Data Object Exchange (DOE)

Copyright (c) 2024, Western Digital Corporation or its affiliates.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PciBus.h"
#include <IndustryStandard/PciExpress60.h>

/// DOE Header
#define DOE_HEADER1_OFST_VID   0
#define DOE_HEADER1_OFST_TYPE  15

#define DOE_HEADER2_OFST_LEN  0

// DOE Discovery Types
#define PCI_VENDOR_ID_PCI_SIG       0x01
#define PCI_DOE_PROTOCOL_DISCOVERY  0x00

#define POLL_INTERVAL_US  500

//
// Section "6.30.2 Operation" describes a range of operations
// that must complete within 1 second. The device could be busy
// for any number of reasons, so let's use a 1 second
// timeout to wait a reasonable amount of time, without stalling
// if an error occurs.
//
// Timeout is in microseconds
//
#define TIMEOUT_MS  1000000

/** Stalls the BSP for the minimum of POLL_INTERVAL_US and Timeout.

   @param[in]  Timeout    The time limit in microseconds remaining for
                          the DOE operation.

   @retval     StallTime  Time of execution stall.
**/
STATIC
UINTN
CalculateAndStallInterval (
  IN UINTN  Timeout
  )
{
  UINTN  StallTime;

  if ((Timeout < POLL_INTERVAL_US) && (Timeout != 0)) {
    StallTime = Timeout;
  } else {
    StallTime = POLL_INTERVAL_US;
  }

  gBS->Stall (StallTime);

  return StallTime;
}

/** Wait for the DOE Not Busy bit to be set

  @param[in]  PciIoDevice    A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_UNSUPPORTED  Hardware could not be programmed.
  @retval EFI_TIMEOUT  Did not complete in timeout period.
**/
EFI_STATUS
DoeWaitForNotBusy (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS                  Status;
  PCI_EXPRESS_REG_DOE_STATUS  DoeStatus;
  UINTN                       Timeout;

  //
  // Read DOE Status
  //
  Status = PciIoDevice->PciIo.Pci.Read (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint32,
                                    PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                                    1,
                                    &DoeStatus
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Timeout = TIMEOUT_MS;

  while (DoeStatus.Bits.DoeBusy) {
    Status = PciIoDevice->PciIo.Pci.Read (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint32,
                                      PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                                      1,
                                      &DoeStatus
                                      );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    if (Timeout == 0) {
      return EFI_TIMEOUT;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
  }

  return EFI_SUCCESS;
}

/** Set the DOE Go bit

  @param[in]  PciIoDevice    A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_UNSUPPORTED  Hardware could not be programmed.
**/
EFI_STATUS
DoeSetGoBit (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS                   Status;
  PCI_EXPRESS_REG_DOE_CONTROL  DoeControl;

  //
  // Read DOE Control
  //
  Status = PciIoDevice->PciIo.Pci.Read (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint32,
                                    PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET,
                                    1,
                                    &DoeControl
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  // Set Go bit
  DoeControl.Bits.DoeGo = 1;

  DEBUG ((DEBUG_INFO, "Setting DOE Go bit\n"));

  Status = PciIoDevice->PciIo.Pci.Write (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint32,
                                    PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET,
                                    1,
                                    &DoeControl
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Wait for the DOE Data Object Ready (DOR) bit to be set

  @param[in]  PciIoDevice    A pointer to the EFI_PCI_IO_PROTOCOL instance.

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_UNSUPPORTED  Hardware could not be programmed.
  @retval EFI_TIMEOUT  Did not complete in timeout period.
**/
EFI_STATUS
DoeWaitForStatusDor (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS                  Status;
  PCI_EXPRESS_REG_DOE_STATUS  DoeStatus;
  UINTN                       Timeout;

  //
  // Read DOE Status
  //
  Status = PciIoDevice->PciIo.Pci.Read (
                                    &PciIoDevice->PciIo,
                                    EfiPciIoWidthUint32,
                                    PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                                    1,
                                    &DoeStatus
                                    );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Waiting for DOE Data Object Read\n"));

  Timeout = TIMEOUT_MS;

  while (!DoeStatus.Bits.DataObjectReady) {
    Status = PciIoDevice->PciIo.Pci.Read (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint32,
                                      PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                                      1,
                                      &DoeStatus
                                      );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    if (Timeout == 0) {
      return EFI_TIMEOUT;
    }

    Timeout -= CalculateAndStallInterval (Timeout);
  }

  return EFI_SUCCESS;
}

/**
  Performs a send operation on a DOE Mailbox

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Count                 The number of bytes to use in the write operation or the maximum number
                                of bytes to read for a read operation.
                                For read operations it is set to the number of bytes read.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported

**/
EFI_STATUS
EFIAPI
PciIoDoeSend (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN     UINTN               Count,
  IN     VOID                *Buffer
  )
{
  EFI_STATUS     Status;
  PCI_IO_DEVICE  *PciIoDevice;
  UINT8          Index;
  UINTN          Operations;
  UINT32         *Data;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DOE_THIS (This);

  Status = DoeWaitForNotBusy (PciIoDevice);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Index      = 0;
  Operations = Count / 4;
  Data       = Buffer;

  while (Operations > 0) {
    Status = PciIoDevice->PciIo.Pci.Write (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint32,
                                      PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_WRITE_DATA_MAILBOX_OFFSET,
                                      1,
                                      &Data[Index]
                                      );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    Index++;
    Operations--;
  }

  // Set DOE Go bit
  Status = DoeSetGoBit (PciIoDevice);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return Status;
}

/**
  Performs an receive operation on a DOE Mailbox

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  Count                 The number of bytes to use in the write operation or the maximum number
                                of bytes to read for a read operation.
                                For read operations it is set to the number of bytes read.
  @param  Buffer                For read operations, the destination buffer to store the results. For write
                                operations, the source buffer to write data from.

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported

**/
EFI_STATUS
EFIAPI
PciIoDoeRecieve (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN OUT UINTN               *Count,
  IN OUT VOID                *Buffer
  )
{
  EFI_STATUS                  Status;
  PCI_IO_DEVICE               *PciIoDevice;
  PCI_EXPRESS_REG_DOE_STATUS  DoeStatus;
  UINTN                       Index;
  UINT32                      *Data;

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DOE_THIS (This);

  // Wait for a response
  Status = DoeWaitForStatusDor (PciIoDevice);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Read DOE Status
  //
  PciIoDevice->PciIo.Pci.Read (
                           &PciIoDevice->PciIo,
                           EfiPciIoWidthUint32,
                           PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                           1,
                           &DoeStatus
                           );

  DEBUG ((DEBUG_INFO, "Reading data from mailbox\n"));

  Index = 0;
  Data  = Buffer;

  while (DoeStatus.Bits.DataObjectReady && (Index * 4) < *Count) {
    // Read response
    Status = PciIoDevice->PciIo.Pci.Read (
                                      &PciIoDevice->PciIo,
                                      EfiPciIoWidthUint32,
                                      PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_READ_DATA_MAILBOX_OFFSET,
                                      1,
                                      &Data[Index]
                                      );
    // Clear the FIFO
    Status |= PciIoDevice->PciIo.Pci.Write (
                                       &PciIoDevice->PciIo,
                                       EfiPciIoWidthUint32,
                                       PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_WRITE_DATA_MAILBOX_OFFSET,
                                       1,
                                       &Data[Index]
                                       );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    Index++;
  }

  *Count = Index * 4;

  return Status;
}

/**
  Check if a specific Vendor ID and Data Object Type is supported

  @param  This                  A pointer to the EFI_PCI_IO_PROTOCOL instance.
  @param  VendorId              The Vendor ID to check against
  @param  Type                  The Data Object Type to check against

  @retval EFI_SUCCESS           The operation on the PCI controller's attributes was completed.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_UNSUPPORTED       DOE is not supported or the VendorId and Type are not supported

**/
EFI_STATUS
EFIAPI
PciIoDoeTypeCheck (
  IN EDKII_PCI_DOE_PROTOCOL  *This,
  IN     UINTN               VendorId,
  IN     UINTN               Type
  )
{
  EFI_STATUS                        Status;
  PCI_IO_DEVICE                     *PciIoDevice;
  PCI_EXPRESS_DOE_DISCOVERY_PACKET  Packet;
  UINT8                             Index;
  UINTN                             Count;
  UINT16                            ResponseVid, ResponseType;
  UINT32                            Response[2];

  PciIoDevice = PCI_IO_DEVICE_FROM_PCI_DOE_THIS (This);

  Status = DoeWaitForNotBusy (PciIoDevice);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Packet.Header1 = (PCI_DOE_PROTOCOL_DISCOVERY << DOE_HEADER1_OFST_TYPE) | (PCI_VENDOR_ID_PCI_SIG << DOE_HEADER1_OFST_VID);
  Packet.Header2 = 3 << DOE_HEADER2_OFST_LEN;

  Index = 0;

  do {
    Packet.Dw0 = Index;

    DEBUG ((DEBUG_INFO, "Sending DOE Discovery packet\n"));

    // Send the DOE discovery packet
    Count = 3 * 4;

    Status = PciIoDoeSend (This, Count, &Packet);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    // Set DOE Go bit
    Status = DoeSetGoBit (PciIoDevice);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    // Wait for a response
    Status = DoeWaitForStatusDor (PciIoDevice);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    // Read response
    Count  = 2 * 4;
    Status = PciIoDoeRecieve (This, &Count, &Response);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }

    DEBUG ((DEBUG_INFO, "Response is: 0x%x/0x%x\n", Response[0], Response[1]));

    // Bits 0:15
    ResponseVid = Response[0] & 0xFFFF;
    // Bits 16:23
    ResponseType = (Response[0] >> 16) & 0xFF;
    // Bits 24:31
    Index = Response[0] >> 24;

    if ((VendorId == ResponseVid) && (Type == ResponseType)) {
      return EFI_SUCCESS;
    }

    // Check if Index matches
  } while (Index > 0);

  return RETURN_UNSUPPORTED;
}

//
// Pci Io DOE Interface
//
EDKII_PCI_DOE_PROTOCOL  mPciIoDoeInterface = {
  PciIoDoeTypeCheck,
  PciIoDoeSend,
  PciIoDoeRecieve,
};

/**
  Probe if the PCIe device supports DOE and if it does populate the
  EDKII_PCI_DOE_PROTOCOL instance.

  @param PciIoDevice      PCI device instance.

  @retval EFI_UNSUPPORTED PCI Device does not support DOE.
  @retval EFI_SUCCESS     PCI Device does support DOE.

**/
EFI_STATUS
ProbeDoeSupport (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  EFI_STATUS  Status;

  Status = LocatePciExpressCapabilityRegBlock (
             PciIoDevice,
             EFI_PCIE_CAPABILITY_ID_DOE,
             &PciIoDevice->DoeCapabilityOffset,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Found DOE support!\n"));

  CopyMem (&PciIoDevice->PciDoe, &mPciIoDoeInterface, sizeof (EDKII_PCI_DOE_PROTOCOL));

  Status = PciIoDoeTypeCheck (&PciIoDevice->PciDoe, 0x01, 0x00);
  if (EFI_ERROR (Status)) {
    // DOE Discovery must be supported for DOE
    DEBUG ((DEBUG_INFO, "DOE Discovery is not supported, device is invalid\n"));
    PciIoDevice->DoeCapabilityOffset = 0;
    ZeroMem (&PciIoDevice->PciDoe, sizeof (EDKII_PCI_DOE_PROTOCOL));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "Found DOE Discovery support\n"));

  return Status;
}
