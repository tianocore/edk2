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

VOID
DoeWaitForNotBusy (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  PCI_EXPRESS_REG_DOE_STATUS  DoeStatus;

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

  while (DoeStatus.Bits.DoeBusy) {
    PciIoDevice->PciIo.Pci.Read (
                             &PciIoDevice->PciIo,
                             EfiPciIoWidthUint32,
                             PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                             1,
                             &DoeStatus
                             );
  }
}

VOID
DoeSetGoBit (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  PCI_EXPRESS_REG_DOE_CONTROL  DoeControl;

  //
  // Read DOE Control
  //
  PciIoDevice->PciIo.Pci.Read (
                           &PciIoDevice->PciIo,
                           EfiPciIoWidthUint32,
                           PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET,
                           1,
                           &DoeControl
                           );

  // Set Go bit
  DoeControl.Bits.DoeGo = 1;

  DEBUG ((DEBUG_INFO, "Setting DOE Go bit\n"));

  PciIoDevice->PciIo.Pci.Write (
                           &PciIoDevice->PciIo,
                           EfiPciIoWidthUint32,
                           PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_CONTROL_OFFSET,
                           1,
                           &DoeControl
                           );
}

VOID
DoeWaitForStatusDor (
  IN PCI_IO_DEVICE  *PciIoDevice
  )
{
  PCI_EXPRESS_REG_DOE_STATUS  DoeStatus;

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

  DEBUG ((DEBUG_INFO, "Waiting for DOE Data Object Read\n"));

  while (!DoeStatus.Bits.DataObjectReady) {
    PciIoDevice->PciIo.Pci.Read (
                             &PciIoDevice->PciIo,
                             EfiPciIoWidthUint32,
                             PciIoDevice->DoeCapabilityOffset + PCI_EXPRESS_REG_DOE_STATUS_OFFSET,
                             1,
                             &DoeStatus
                             );
  }
}

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

  DoeWaitForNotBusy (PciIoDevice);

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
  DoeSetGoBit (PciIoDevice);

  return Status;
}

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
  DoeWaitForStatusDor (PciIoDevice);

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

  DoeWaitForNotBusy (PciIoDevice);

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
    DoeSetGoBit (PciIoDevice);

    // Wait for a response
    DoeWaitForStatusDor (PciIoDevice);

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
