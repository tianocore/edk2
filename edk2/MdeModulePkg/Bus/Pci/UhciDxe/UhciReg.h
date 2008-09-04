/** @file

  The definition for UHCI register operation routines.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_UHCI_REG_H_
#define _EFI_UHCI_REG_H_

#define BIT(a)  (1 << (a))

typedef enum {
  UHCI_FRAME_NUM        = 1024,

  //
  // Register offset and PCI related staff
  //
  CLASSC_OFFSET         = 0x09,
  USBBASE_OFFSET        = 0x20,
  USB_BAR_INDEX         = 4,
  PCI_CLASSC_PI_UHCI    = 0x00,

  USBCMD_OFFSET         = 0,
  USBSTS_OFFSET         = 2,
  USBINTR_OFFSET        = 4,
  USBPORTSC_OFFSET      = 0x10,
  USB_FRAME_NO_OFFSET   = 6,
  USB_FRAME_BASE_OFFSET = 8,
  USB_EMULATION_OFFSET  = 0xC0,

  //
  // Packet IDs
  //
  SETUP_PACKET_ID       = 0x2D,
  INPUT_PACKET_ID       = 0x69,
  OUTPUT_PACKET_ID      = 0xE1,
  ERROR_PACKET_ID       = 0x55,

  //
  // USB port status and control bit definition.
  //
  USBPORTSC_CCS         = BIT(0),  // Current Connect Status
  USBPORTSC_CSC         = BIT(1),  // Connect Status Change
  USBPORTSC_PED         = BIT(2),  // Port Enable / Disable
  USBPORTSC_PEDC        = BIT(3),  // Port Enable / Disable Change
  USBPORTSC_LSL         = BIT(4),  // Line Status Low BIT
  USBPORTSC_LSH         = BIT(5),  // Line Status High BIT
  USBPORTSC_RD          = BIT(6),  // Resume Detect
  USBPORTSC_LSDA        = BIT(8),  // Low Speed Device Attached
  USBPORTSC_PR          = BIT(9),  // Port Reset
  USBPORTSC_SUSP        = BIT(12), // Suspend

  //
  // UHCI Spec said it must implement 2 ports each host at least,
  // and if more, check whether the bit7 of PORTSC is always 1.
  // So here assume the max of port number each host is 16.
  //
  USB_MAX_ROOTHUB_PORT  = 0x0F,
  
  //
  // Command register bit definitions
  //
  USBCMD_RS             = BIT(0),  // Run/Stop
  USBCMD_HCRESET        = BIT(1),  // Host reset
  USBCMD_GRESET         = BIT(2),  // Global reset
  USBCMD_EGSM           = BIT(3),  // Global Suspend Mode
  USBCMD_FGR            = BIT(4),  // Force Global Resume
  USBCMD_SWDBG          = BIT(5),  // SW Debug mode
  USBCMD_CF             = BIT(6),  // Config Flag (sw only)
  USBCMD_MAXP           = BIT(7),  // Max Packet (0 = 32, 1 = 64)

  //
  // USB Status register bit definitions
  //
  USBSTS_USBINT         = BIT(0),  // Interrupt due to IOC
  USBSTS_ERROR          = BIT(1),  // Interrupt due to error
  USBSTS_RD             = BIT(2),  // Resume Detect
  USBSTS_HSE            = BIT(3),  // Host System Error
  USBSTS_HCPE           = BIT(4),  // Host Controller Process Error
  USBSTS_HCH            = BIT(5),  // HC Halted

  USBTD_ACTIVE          = BIT(7),  // TD is still active
  USBTD_STALLED         = BIT(6),  // TD is stalled
  USBTD_BUFFERR         = BIT(5),  // Buffer underflow or overflow
  USBTD_BABBLE          = BIT(4),  // Babble condition
  USBTD_NAK             = BIT(3),  // NAK is received
  USBTD_CRC             = BIT(2),  // CRC/Time out error
  USBTD_BITSTUFF        = BIT(1)  // Bit stuff error
}UHCI_REGISTER_OFFSET;


/**
  Read a UHCI register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.

  @return Content of register.

**/
UINT16
UhciReadReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  Offset
  );



/**
  Write data to UHCI register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Data         Data to write.

  @return None.

**/
VOID
UhciWriteReg (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  Offset,
  IN UINT16                  Data
  );



/**
  Set a bit of the UHCI Register.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Bit          The bit to set.

  @return None.

**/
VOID
UhciSetRegBit (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  Offset,
  IN UINT16                  Bit
  );



/**
  Clear a bit of the UHCI Register.

  @param  PciIo        The PCI_IO protocol to access the PCI.
  @param  Offset       Register offset to USB_BAR_INDEX.
  @param  Bit          The bit to clear.

  @return None.

**/
VOID
UhciClearRegBit (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN UINT32                  Offset,
  IN UINT16                  Bit
  );


/**
  Clear all the interrutp status bits, these bits
  are Write-Clean.

  @param  Uhc          The UHCI device.

  @return None.

**/
VOID
UhciAckAllInterrupt (
  IN  USB_HC_DEV          *Uhc
  );


/**
  Stop the host controller.

  @param  Uhc          The UHCI device.
  @param  Timeout      Max time allowed.

  @retval EFI_SUCCESS  The host controller is stopped.
  @retval EFI_TIMEOUT  Failed to stop the host controller.

**/
EFI_STATUS
UhciStopHc (
  IN USB_HC_DEV         *Uhc,
  IN UINTN              Timeout
  );



/**
  Check whether the host controller operates well.

  @param  PciIo        The PCI_IO protocol to use.

  @retval TRUE         Host controller is working.
  @retval FALSE        Host controller is halted or system error.

**/
BOOLEAN
UhciIsHcWorking (
  IN EFI_PCI_IO_PROTOCOL     *PciIo
  );


/**
  Set the UHCI frame list base address. It can't use
  UhciWriteReg which access memory in UINT16.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL to use.
  @param  Addr         Address to set.

  @return None.

**/
VOID
UhciSetFrameListBaseAddr (
  IN EFI_PCI_IO_PROTOCOL     *PciIo,
  IN VOID                    *Addr
  );


/**
  Disable USB Emulation.

  @param  PciIo        The EFI_PCI_IO_PROTOCOL protocol to use.

  @return None.

**/
VOID
UhciTurnOffUsbEmulation (
  IN EFI_PCI_IO_PROTOCOL     *PciIo
  );
#endif
