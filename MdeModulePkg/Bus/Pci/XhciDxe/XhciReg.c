/** @file

  The XHCI register operation routines.

(C) Copyright 2023 Hewlett Packard Enterprise Development LP<BR>
Copyright (c) 2011 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Xhci.h"

/**
  Read 1-byte width XHCI capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 1-byte width capability register.

  @return The register content read.
  @retval If err, return 0xFF.

**/
UINT8
XhcReadCapReg8 (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT32             Offset
  )
{
  UINT8       Data;
  EFI_STATUS  Status;

  Data = 0;

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint8,
                             XHC_BAR_INDEX,
                             (UINT64)Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFF;
  }

  return Data;
}

/**
  Read 4-bytes width XHCI capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 4-bytes width capability register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadCapReg (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT32             Offset
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             (UINT64)Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcReadCapReg: Pci Io read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Read 4-bytes width XHCI Operational register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the 4-bytes width operational register.

  @return The register content read.
  @retval If err, return 0xFFFFFFFF.

**/
UINT32
XhcReadOpReg (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT32             Offset
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->CapLength + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcReadOpReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the 4-bytes width XHCI operational register.

  @param  Xhc      The XHCI Instance.
  @param  Offset   The offset of the 4-bytes width operational register.
  @param  Data     The data to write.

**/
VOID
XhcWriteOpReg (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  EFI_STATUS  Status;

  ASSERT (Xhc->CapLength != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->CapLength + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Write the data to the XHCI door bell register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the door bell register.
  @param  Data         The data to write.

**/
VOID
XhcWriteDoorBellReg (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  EFI_STATUS  Status;

  ASSERT (Xhc->DBOff != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->DBOff + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcWriteOpReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read XHCI runtime register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.

  @return The register content read

**/
UINT32
XhcReadRuntimeReg (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT32             Offset
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->RTSOff + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcReadRuntimeReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the XHCI runtime register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Data         The data to write.

**/
VOID
XhcWriteRuntimeReg (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  EFI_STATUS  Status;

  ASSERT (Xhc->RTSOff != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->RTSOff + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcWriteRuntimeReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Read XHCI extended capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the extended capability register.

  @return The register content read

**/
UINT32
XhcReadExtCapReg (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT32             Offset
  )
{
  UINT32      Data;
  EFI_STATUS  Status;

  ASSERT (Xhc->ExtCapRegBase != 0);

  Status = Xhc->PciIo->Mem.Read (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->ExtCapRegBase + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcReadExtCapReg: Pci Io Read error - %r at %d\n", Status, Offset));
    Data = 0xFFFFFFFF;
  }

  return Data;
}

/**
  Write the data to the XHCI extended capability register.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the extended capability register.
  @param  Data         The data to write.

**/
VOID
XhcWriteExtCapReg (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Data
  )
{
  EFI_STATUS  Status;

  ASSERT (Xhc->ExtCapRegBase != 0);

  Status = Xhc->PciIo->Mem.Write (
                             Xhc->PciIo,
                             EfiPciIoWidthUint32,
                             XHC_BAR_INDEX,
                             Xhc->ExtCapRegBase + Offset,
                             1,
                             &Data
                             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XhcWriteExtCapReg: Pci Io Write error: %r at %d\n", Status, Offset));
  }
}

/**
  Set one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetRuntimeRegBit (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the runtime register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the runtime register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcClearRuntimeRegBit (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32  Data;

  Data  = XhcReadRuntimeReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteRuntimeReg (Xhc, Offset, Data);
}

/**
  Set one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to set.

**/
VOID
XhcSetOpRegBit (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data |= Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}

/**
  Clear one bit of the operational register while keeping other bits.

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operational register.
  @param  Bit          The bit mask of the register to clear.

**/
VOID
XhcClearOpRegBit (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit
  )
{
  UINT32  Data;

  Data  = XhcReadOpReg (Xhc, Offset);
  Data &= ~Bit;
  XhcWriteOpReg (Xhc, Offset, Data);
}

/**
  Wait the operation register's bit as specified by Bit
  to become set (or clear).

  @param  Xhc          The XHCI Instance.
  @param  Offset       The offset of the operation register.
  @param  Bit          The bit of the register to wait for.
  @param  WaitToSet    Wait the bit to set or clear.
  @param  Timeout      The time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The bit successfully changed by host controller.
  @retval EFI_TIMEOUT  The time out occurred.

**/
EFI_STATUS
XhcWaitOpRegBit (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Offset,
  IN UINT32             Bit,
  IN BOOLEAN            WaitToSet,
  IN UINT32             Timeout
  )
{
  UINT64  TimeoutTicks;
  UINT64  ElapsedTicks;
  UINT64  TicksDelta;
  UINT64  CurrentTick;

  if (Timeout == 0) {
    return EFI_TIMEOUT;
  }

  TimeoutTicks = XhcConvertTimeToTicks (XHC_MICROSECOND_TO_NANOSECOND (Timeout * XHC_1_MILLISECOND));
  ElapsedTicks = 0;
  CurrentTick  = GetPerformanceCounter ();
  do {
    if (XHC_REG_BIT_IS_SET (Xhc, Offset, Bit) == WaitToSet) {
      return EFI_SUCCESS;
    }

    gBS->Stall (XHC_1_MICROSECOND);
    TicksDelta = XhcGetElapsedTicks (&CurrentTick);
    // Ensure that ElapsedTicks is always incremented to avoid indefinite hangs
    if (TicksDelta == 0) {
      TicksDelta = XhcConvertTimeToTicks (XHC_MICROSECOND_TO_NANOSECOND (XHC_1_MICROSECOND));
    }

    ElapsedTicks += TicksDelta;
  } while (ElapsedTicks < TimeoutTicks);

  return EFI_TIMEOUT;
}

/**
  Set Bios Ownership

  @param  Xhc          The XHCI Instance.

**/
VOID
XhcSetBiosOwnership (
  IN USB_XHCI_INSTANCE  *Xhc
  )
{
  UINT32  Buffer;

  if (Xhc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  DEBUG ((DEBUG_INFO, "XhcSetBiosOwnership: called to set BIOS ownership\n"));

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_OS_SEMAPHORE)) | USBLEGSP_BIOS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Clear Bios Ownership

  @param  Xhc       The XHCI Instance.

**/
VOID
XhcClearBiosOwnership (
  IN USB_XHCI_INSTANCE  *Xhc
  )
{
  UINT32  Buffer;

  if (Xhc->UsbLegSupOffset == 0xFFFFFFFF) {
    return;
  }

  DEBUG ((DEBUG_INFO, "XhcClearBiosOwnership: called to clear BIOS ownership\n"));

  Buffer = XhcReadExtCapReg (Xhc, Xhc->UsbLegSupOffset);
  Buffer = ((Buffer & (~USBLEGSP_BIOS_SEMAPHORE)) | USBLEGSP_OS_SEMAPHORE);
  XhcWriteExtCapReg (Xhc, Xhc->UsbLegSupOffset, Buffer);
}

/**
  Calculate the offset of the XHCI capability.

  @param  Xhc     The XHCI Instance.
  @param  CapId   The XHCI Capability ID.

  @return The offset of XHCI legacy support capability register.

**/
UINT32
XhcGetCapabilityAddr (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT8              CapId
  )
{
  UINT32  ExtCapOffset;
  UINT8   NextExtCapReg;
  UINT32  Data;

  ExtCapOffset = 0;

  do {
    //
    // Check if the extended capability register's capability id is USB Legacy Support.
    //
    Data = XhcReadExtCapReg (Xhc, ExtCapOffset);
    if ((Data & 0xFF) == CapId) {
      return ExtCapOffset;
    }

    //
    // If not, then traverse all of the ext capability registers till finding out it.
    //
    NextExtCapReg = (UINT8)((Data >> 8) & 0xFF);
    ExtCapOffset += (NextExtCapReg << 2);
  } while (NextExtCapReg != 0);

  return 0xFFFFFFFF;
}

/**
  Calculate the offset of the xHCI Supported Protocol Capability.

  @param  Xhc           The XHCI Instance.
  @param  MajorVersion  The USB Major Version in xHCI Support Protocol Capability Field

  @return The offset of xHCI Supported Protocol capability register.

**/
UINT32
XhcGetSupportedProtocolCapabilityAddr (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT8              MajorVersion
  )
{
  UINT32                      ExtCapOffset;
  UINT8                       NextExtCapReg;
  UINT32                      Data;
  UINT32                      NameString;
  XHC_SUPPORTED_PROTOCOL_DW0  UsbSupportDw0;

  if (Xhc == NULL) {
    return 0;
  }

  ExtCapOffset = 0;

  do {
    //
    // Check if the extended capability register's capability id is USB Legacy Support.
    //
    Data                = XhcReadExtCapReg (Xhc, ExtCapOffset);
    UsbSupportDw0.Dword = Data;
    if ((Data & 0xFF) == XHC_CAP_USB_SUPPORTED_PROTOCOL) {
      if (UsbSupportDw0.Data.RevMajor == MajorVersion) {
        NameString = XhcReadExtCapReg (Xhc, ExtCapOffset + XHC_SUPPORTED_PROTOCOL_NAME_STRING_OFFSET);
        if (NameString == XHC_SUPPORTED_PROTOCOL_NAME_STRING_VALUE) {
          //
          // Ensure Name String field is xHCI supported protocols in xHCI Supported Protocol Capability Offset 04h
          //
          return ExtCapOffset;
        }
      }
    }

    //
    // If not, then traverse all of the ext capability registers till finding out it.
    //
    NextExtCapReg = (UINT8)((Data >> 8) & 0xFF);
    ExtCapOffset += (NextExtCapReg << 2);
  } while (NextExtCapReg != 0);

  return 0xFFFFFFFF;
}

/**
  Find PortSpeed value match Protocol Speed ID Value (PSIV).

  @param  Xhc            The XHCI Instance.
  @param  ExtCapOffset   The USB Major Version in xHCI Support Protocol Capability Field
  @param  PortSpeed      The Port Speed Field in USB PortSc register
  @param  PortNumber     The Port Number (0-indexed)

  @return The Protocol Speed ID (PSI) from xHCI Supported Protocol capability register.

**/
UINT32
XhciPsivGetPsid (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             ExtCapOffset,
  IN UINT8              PortSpeed,
  IN UINT8              PortNumber
  )
{
  XHC_SUPPORTED_PROTOCOL_DW2                PortId;
  XHC_SUPPORTED_PROTOCOL_PROTOCOL_SPEED_ID  Reg;
  UINT32                                    Count;
  UINT32                                    MinPortIndex;
  UINT32                                    MaxPortIndex;

  if ((Xhc == NULL) || (ExtCapOffset == 0xFFFFFFFF)) {
    return 0;
  }

  //
  // According to XHCI 1.1 spec November 2017,
  // Section 7.2 xHCI Supported Protocol Capability
  // 1. Get the PSIC(Protocol Speed ID Count) value.
  // 2. The PSID register boundary should be Base address + PSIC * 0x04
  //
  PortId.Dword = XhcReadExtCapReg (Xhc, ExtCapOffset + XHC_SUPPORTED_PROTOCOL_DW2_OFFSET);

  //
  // According to XHCI 1.1 spec November 2017, valid values
  // for CompPortOffset are 1 to CompPortCount - 1.
  //
  // PortNumber is zero-indexed, so subtract 1.
  //
  if ((PortId.Data.CompPortOffset == 0) || (PortId.Data.CompPortCount == 0)) {
    return 0;
  }

  MinPortIndex = PortId.Data.CompPortOffset - 1;
  MaxPortIndex = MinPortIndex + PortId.Data.CompPortCount - 1;

  if ((PortNumber < MinPortIndex) || (PortNumber > MaxPortIndex)) {
    return 0;
  }

  for (Count = 0; Count < PortId.Data.Psic; Count++) {
    Reg.Dword = XhcReadExtCapReg (Xhc, ExtCapOffset + XHC_SUPPORTED_PROTOCOL_PSI_OFFSET + (Count << 2));
    if (Reg.Data.Psiv == PortSpeed) {
      return Reg.Dword;
    }
  }

  return 0;
}

/**
  Find PortSpeed value match case in XHCI Supported Protocol Capability

  @param  Xhc         The XHCI Instance.
  @param  PortSpeed   The Port Speed Field in USB PortSc register
  @param  PortNumber  The Port Number (0-indexed)

  @return The USB Port Speed.

**/
UINT16
XhcCheckUsbPortSpeedUsedPsic (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT8              PortSpeed,
  IN UINT8              PortNumber
  )
{
  XHC_SUPPORTED_PROTOCOL_PROTOCOL_SPEED_ID  SpField;
  UINT16                                    UsbSpeedIdMap;

  if (Xhc == NULL) {
    return 0;
  }

  SpField.Dword = 0;
  UsbSpeedIdMap = 0;

  //
  // Check xHCI Supported Protocol Capability, find the PSIV field to match
  // PortSpeed definition when the Major Revision is 03h.
  //
  if (Xhc->Usb3SupOffset != 0xFFFFFFFF) {
    SpField.Dword = XhciPsivGetPsid (Xhc, Xhc->Usb3SupOffset, PortSpeed, PortNumber);
    if (SpField.Dword != 0) {
      //
      // Found the corresponding PORTSC value in PSIV field of USB3 offset.
      //
      UsbSpeedIdMap = USB_PORT_STAT_SUPER_SPEED;
    }
  }

  //
  // Check xHCI Supported Protocol Capability, find the PSIV field to match
  // PortSpeed definition when the Major Revision is 02h.
  //
  if ((UsbSpeedIdMap == 0) && (Xhc->Usb2SupOffset != 0xFFFFFFFF)) {
    SpField.Dword = XhciPsivGetPsid (Xhc, Xhc->Usb2SupOffset, PortSpeed, PortNumber);
    if (SpField.Dword != 0) {
      //
      // Found the corresponding PORTSC value in PSIV field of USB2 offset.
      //
      if (SpField.Data.Psie == 2) {
        //
        // According to XHCI 1.1 spec November 2017,
        // Section 7.2.1 the Protocol Speed ID Exponent (PSIE) field definition,
        // PSIE value shall be applied to Protocol Speed ID Mantissa when calculating, value 2 shall represent bit rate in Mb/s
        //
        if (SpField.Data.Psim == XHC_SUPPORTED_PROTOCOL_USB2_HIGH_SPEED_PSIM) {
          //
          // PSIM shows as default High-speed protocol, apply to High-speed mapping
          //
          UsbSpeedIdMap = USB_PORT_STAT_HIGH_SPEED;
        }
      } else if (SpField.Data.Psie == 1) {
        //
        // According to XHCI 1.1 spec November 2017,
        // Section 7.2.1 the Protocol Speed ID Exponent (PSIE) field definition,
        // PSIE value shall be applied to Protocol Speed ID Mantissa when calculating, value 1 shall represent bit rate in Kb/s
        //
        if (SpField.Data.Psim == XHC_SUPPORTED_PROTOCOL_USB2_LOW_SPEED_PSIM) {
          //
          // PSIM shows as default Low-speed protocol, apply to Low-speed mapping
          //
          UsbSpeedIdMap = USB_PORT_STAT_LOW_SPEED;
        }
      }
    }
  }

  return UsbSpeedIdMap;
}

/**
  Whether the XHCI host controller is halted.

  @param  Xhc     The XHCI Instance.

  @retval TRUE    The controller is halted.
  @retval FALSE   It isn't halted.

**/
BOOLEAN
XhcIsHalt (
  IN USB_XHCI_INSTANCE  *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT);
}

/**
  Whether system error occurred.

  @param  Xhc      The XHCI Instance.

  @retval TRUE     System error happened.
  @retval FALSE    No system error.

**/
BOOLEAN
XhcIsSysError (
  IN USB_XHCI_INSTANCE  *Xhc
  )
{
  return XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HSE);
}

/**
  Set USBCMD Host System Error Enable(HSEE) Bit if PCICMD SERR# Enable Bit is set.

  The USBCMD HSEE Bit will be reset to default 0 by USBCMD Host Controller Reset(HCRST).
  This function is to set USBCMD HSEE Bit if PCICMD SERR# Enable Bit is set.

  @param Xhc            The XHCI Instance.

**/
VOID
XhcSetHsee (
  IN USB_XHCI_INSTANCE  *Xhc
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               XhciCmd;

  PciIo  = Xhc->PciIo;
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint16,
                        PCI_COMMAND_OFFSET,
                        sizeof (XhciCmd) / sizeof (UINT16),
                        &XhciCmd
                        );
  if (!EFI_ERROR (Status)) {
    if ((XhciCmd & EFI_PCI_COMMAND_SERR) != 0) {
      XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_HSEE);
    }
  }
}

/**
  Reset the XHCI host controller.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @retval EFI_SUCCESS  The XHCI host controller is reset.
  @return Others       Failed to reset the XHCI before Timeout.

**/
EFI_STATUS
XhcResetHC (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  DEBUG ((DEBUG_INFO, "XhcResetHC!\n"));
  //
  // Host can only be reset when it is halt. If not so, halt it
  //
  if (!XHC_REG_BIT_IS_SET (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT)) {
    Status = XhcHaltHC (Xhc, Timeout);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if ((Xhc->DebugCapSupOffset == 0xFFFFFFFF) || ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset) & 0xFF) != XHC_CAP_USB_DEBUG) ||
      ((XhcReadExtCapReg (Xhc, Xhc->DebugCapSupOffset + XHC_DC_DCCTRL) & BIT0) == 0))
  {
    XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET);
    //
    // Some XHCI host controllers require to have extra 1ms delay before accessing any MMIO register during reset.
    // Otherwise there may have the timeout case happened.
    // The below is a workaround to solve such problem.
    //
    gBS->Stall (PcdGet16 (PcdDelayXhciHCReset));
    Status = XhcWaitOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RESET, FALSE, Timeout);

    if (!EFI_ERROR (Status)) {
      //
      // The USBCMD HSEE Bit will be reset to default 0 by USBCMD HCRST.
      // Set USBCMD HSEE Bit if PCICMD SERR# Enable Bit is set.
      //
      XhcSetHsee (Xhc);
    }
  }

  return Status;
}

/**
  Halt the XHCI host controller.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is halt.
  @return EFI_TIMEOUT  Failed to halt the XHCI before Timeout.

**/
EFI_STATUS
XhcHaltHC (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS  Status;

  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, TRUE, Timeout);
  return Status;
}

/**
  Set the XHCI host controller to run.

  @param  Xhc          The XHCI Instance.
  @param  Timeout      Time to wait before abort (in millisecond, ms).

  @return EFI_SUCCESS  The XHCI host controller is running.
  @return EFI_TIMEOUT  Failed to set the XHCI to run before Timeout.

**/
EFI_STATUS
XhcRunHC (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN UINT32             Timeout
  )
{
  EFI_STATUS  Status;

  XhcSetOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_RUN);
  Status = XhcWaitOpRegBit (Xhc, XHC_USBSTS_OFFSET, XHC_USBSTS_HALT, FALSE, Timeout);
  return Status;
}
