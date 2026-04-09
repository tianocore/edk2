/** @file

  The UHCI register operation routines.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Uhci.h"

/**
  Read a UHCI register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.

  @return Content of register.

**/
UINT16
UhciReadReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset
  )
{
  UINT16      Data;
  EFI_STATUS  Status;

  Status = PciIo->Io.Read (
                       PciIo,
                       EfiPciIoWidthUint16,
                       USB_BAR_INDEX,
                       Offset,
                       1,
                       &Data
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UhciReadReg: PciIo Io.Read error: %r at offset %d\n", Status, Offset));

    Data = 0xFFFF;
  }

  return Data;
}

/**
  Write data to UHCI register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Data         Data to write.

**/
VOID
UhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT16               Data
  )
{
  EFI_STATUS  Status;

  Status = PciIo->Io.Write (
                       PciIo,
                       EfiPciIoWidthUint16,
                       USB_BAR_INDEX,
                       Offset,
                       1,
                       &Data
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UhciWriteReg: PciIo Io.Write error: %r at offset %d\n", Status, Offset));
  }
}

/**
  Set a bit of the UHCI Register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Bit          The bit to set.

**/
VOID
UhciSetRegBit (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT16               Bit
  )
{
  UINT16  Data;

  Data = UhciReadReg (PciIo, Offset);
  Data = (UINT16)(Data |Bit);
  UhciWriteReg (PciIo, Offset, Data);
}

/**
  Clear a bit of the UHCI Register.

  @param  PciIo        The PCI_IO protocol to access the PCI.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Bit          The bit to clear.

**/
VOID
UhciClearRegBit (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT32               Offset,
  IN UINT16               Bit
  )
{
  UINT16  Data;

  Data = UhciReadReg (PciIo, Offset);
  Data = (UINT16)(Data & ~Bit);
  UhciWriteReg (PciIo, Offset, Data);
}

/**
  Clear all the interrutp status bits, these bits
  are Write-Clean.

  @param  Uhc          The UHCI device.

**/
VOID
UhciAckAllInterrupt (
  IN  USB_HC_DEV  *Uhc
  )
{
  UhciWriteReg (Uhc->PciIo, USBSTS_OFFSET, 0x3F);

  //
  // If current HC is halted, re-enable it. Host Controller Process Error
  // is a temporary error status.
  //
  if (!UhciIsHcWorking (Uhc->PciIo)) {
    DEBUG ((DEBUG_ERROR, "UhciAckAllInterrupt: re-enable the UHCI from system error\n"));
    Uhc->Usb2Hc.SetState (&Uhc->Usb2Hc, EfiUsbHcStateOperational);
  }
}

/**
  Stop the host controller.

  @param  Uhc          The UHCI device.
  @param  Timeout      Max time allowed.

  @retval EFI_SUCCESS  The host controller is stopped.
  @retval EFI_TIMEOUT  Failed to stop the host controller.

**/
EFI_STATUS
UhciStopHc (
  IN USB_HC_DEV  *Uhc,
  IN UINTN       Timeout
  )
{
  UINT16  UsbSts;
  UINTN   Index;

  UhciClearRegBit (Uhc->PciIo, USBCMD_OFFSET, USBCMD_RS);

  //
  // ensure the HC is in halt status after send the stop command
  // Timeout is in us unit.
  //
  for (Index = 0; Index < (Timeout / 50) + 1; Index++) {
    UsbSts = UhciReadReg (Uhc->PciIo, USBSTS_OFFSET);

    if ((UsbSts & USBSTS_HCH) == USBSTS_HCH) {
      return EFI_SUCCESS;
    }

    gBS->Stall (50);
  }

  return EFI_TIMEOUT;
}

/**
  Check whether the host controller operates well.

  @param  PciIo        The PCI_IO protocol to use.

  @retval TRUE         Host controller is working.
  @retval FALSE        Host controller is halted or system error.

**/
BOOLEAN
UhciIsHcWorking (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINT16  UsbSts;

  UsbSts = UhciReadReg (PciIo, USBSTS_OFFSET);

  if ((UsbSts & (USBSTS_HCPE | USBSTS_HSE | USBSTS_HCH)) != 0) {
    DEBUG ((DEBUG_ERROR, "UhciIsHcWorking: current USB state is %x\n", UsbSts));
    return FALSE;
  }

  return TRUE;
}

/**
  Set the UHCI frame list base address. It can't use
  UhciWriteReg which access memory in UINT16.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Addr         Address to set.

**/
VOID
UhciSetFrameListBaseAddr (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN VOID                 *Addr
  )
{
  EFI_STATUS  Status;
  UINT32      Data;

  Data = (UINT32)((UINTN)Addr & 0xFFFFF000);

  Status = PciIo->Io.Write (
                       PciIo,
                       EfiPciIoWidthUint32,
                       USB_BAR_INDEX,
                       (UINT64)USB_FRAME_BASE_OFFSET,
                       1,
                       &Data
                       );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UhciSetFrameListBaseAddr: PciIo Io.Write error: %r\n", Status));
  }
}

/**
  Disable USB Emulation.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL protocol to use.

**/
VOID
UhciTurnOffUsbEmulation (
  IN EFI_PCI_IO_PROTOCOL  *PciIo
  )
{
  UINT16  Command;

  Command = 0;

  PciIo->Pci.Write (
               PciIo,
               EfiPciIoWidthUint16,
               USB_EMULATION_OFFSET,
               1,
               &Command
               );
}
