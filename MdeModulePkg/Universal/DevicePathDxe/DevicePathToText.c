/** @file
  DevicePathToText protocol as defined in the UEFI 2.0 specification.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

/**
  Concatenates a formatted unicode string to allocated pool. The caller must
  free the resulting buffer.

  @param Str             Tracks the allocated pool, size in use, and
                         amount of pool allocated.
  @param Fmt             The format string
  @param ...             Variable arguments based on the format string.

  @return Allocated buffer with the formatted string printed in it.
          The caller must free the allocated buffer. The buffer
          allocation is not packed.

**/
CHAR16 *
EFIAPI
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *Fmt,
  ...
  )
{
  UINT16  *AppendStr;
  UINTN   AppendCount;
  VA_LIST Args;

  AppendStr = AllocateZeroPool (0x1000);
  ASSERT (AppendStr != NULL);

  VA_START (Args, Fmt);
  AppendCount = UnicodeVSPrint (AppendStr, 0x1000, Fmt, Args);
  VA_END (Args);

  if (Str->Length + AppendCount * sizeof (CHAR16) > Str->Capacity) {
    Str->Capacity = Str->Length + (AppendCount + 1) * sizeof (CHAR16) * 2;
    Str->Str = ReallocatePool (
                 Str->Length,
                 Str->Capacity,
                 Str->Str
                 );
    ASSERT (Str->Str != NULL);
  }

  if (Str->Length == 0) {
    StrCpy (Str->Str, AppendStr);
    Str->Length = (AppendCount + 1) * sizeof (CHAR16);
  } else {
    StrCat (Str->Str, AppendStr);
    Str->Length += AppendCount * sizeof (CHAR16);
  }

  FreePool (AppendStr);
  return Str->Str;
}

/**
  Converts a PCI device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextPci (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  PCI_DEVICE_PATH *Pci;

  Pci = DevPath;
  CatPrint (Str, L"Pci(0x%x,0x%x)", Pci->Device, Pci->Function);
}

/**
  Converts a PC Card device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextPccard (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  PCCARD_DEVICE_PATH  *Pccard;

  Pccard = DevPath;
  CatPrint (Str, L"PcCard(0x%x)", Pccard->FunctionNumber);
}

/**
  Converts a Memory Map device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextMemMap (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MEMMAP_DEVICE_PATH  *MemMap;

  MemMap = DevPath;
  CatPrint (
    Str,
    L"MemoryMapped(0x%x,0x%lx,0x%lx)",
    MemMap->MemoryType,
    MemMap->StartingAddress,
    MemMap->EndingAddress
    );
}

/**
  Converts a Vendor device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextVendor (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  VENDOR_DEVICE_PATH  *Vendor;
  CHAR16              *Type;
  UINTN               Index;
  UINTN               DataLength;
  UINT32              FlowControlMap;
  UINT16              Info;

  Vendor = (VENDOR_DEVICE_PATH *) DevPath;
  switch (DevicePathType (&Vendor->Header)) {
  case HARDWARE_DEVICE_PATH:
    Type = L"Hw";
    break;

  case MESSAGING_DEVICE_PATH:
    Type = L"Msg";
    if (AllowShortcuts) {
      if (CompareGuid (&Vendor->Guid, &gEfiPcAnsiGuid)) {
        CatPrint (Str, L"VenPcAnsi()");
        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiVT100Guid)) {
        CatPrint (Str, L"VenVt100()");
        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiVT100PlusGuid)) {
        CatPrint (Str, L"VenVt100Plus()");
        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiVTUTF8Guid)) {
        CatPrint (Str, L"VenUft8()");
        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiUartDevicePathGuid)) {
        FlowControlMap = (((UART_FLOW_CONTROL_DEVICE_PATH *) Vendor)->FlowControlMap);
        switch (FlowControlMap & 0x00000003) {
        case 0:
          CatPrint (Str, L"UartFlowCtrl(%s)", L"None");
          break;

        case 1:
          CatPrint (Str, L"UartFlowCtrl(%s)", L"Hardware");
          break;

        case 2:
          CatPrint (Str, L"UartFlowCtrl(%s)", L"XonXoff");
          break;

        default:
          break;
        }

        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiSasDevicePathGuid)) {
        CatPrint (
          Str,
          L"SAS(0x%lx,0x%lx,0x%x,",
          ((SAS_DEVICE_PATH *) Vendor)->SasAddress,
          ((SAS_DEVICE_PATH *) Vendor)->Lun,
          ((SAS_DEVICE_PATH *) Vendor)->RelativeTargetPort
          );
        Info = (((SAS_DEVICE_PATH *) Vendor)->DeviceTopology);
        if ((Info & 0x0f) == 0) {
          CatPrint (Str, L"NoTopology,0,0,0,");
        } else if (((Info & 0x0f) == 1) || ((Info & 0x0f) == 2)) {
          CatPrint (
            Str,
            L"%s,%s,%s,",
            ((Info & (0x1 << 4)) != 0) ? L"SATA" : L"SAS",
            ((Info & (0x1 << 5)) != 0) ? L"External" : L"Internal",
            ((Info & (0x1 << 6)) != 0) ? L"Expanded" : L"Direct"
            );
          if ((Info & 0x0f) == 1) {
            CatPrint (Str, L"0,");
          } else {
            CatPrint (Str, L"0x%x,", (Info >> 8) & 0xff);
          }
        } else {
          CatPrint (Str, L"0,0,0,0,");
        }

        CatPrint (Str, L"0x%x)", ((SAS_DEVICE_PATH *) Vendor)->Reserved);
        return ;
      } else if (CompareGuid (&Vendor->Guid, &gEfiDebugPortProtocolGuid)) {
        CatPrint (Str, L"DebugPort()");
        return ;
      }
    }
    break;

  case MEDIA_DEVICE_PATH:
    Type = L"Media";
    break;

  default:
    Type = L"?";
    break;
  }

  DataLength = DevicePathNodeLength (&Vendor->Header) - sizeof (VENDOR_DEVICE_PATH);
  CatPrint (Str, L"Ven%s(%g", Type, &Vendor->Guid);
  if (DataLength != 0) {
    CatPrint (Str, L",");
    for (Index = 0; Index < DataLength; Index++) {
      CatPrint (Str, L"%02x", ((VENDOR_DEVICE_PATH_WITH_DATA *) Vendor)->VendorDefinedData[Index]);
    }
  }

  CatPrint (Str, L")");
}

/**
  Converts a Controller device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextController (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  CONTROLLER_DEVICE_PATH  *Controller;

  Controller = DevPath;
  CatPrint (
    Str,
    L"Ctrl(0x%x)",
    Controller->ControllerNumber
    );
}

/**
  Converts a ACPI device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextAcpi (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ACPI_HID_DEVICE_PATH  *Acpi;

  Acpi = DevPath;
  if ((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
    switch (EISA_ID_TO_NUM (Acpi->HID)) {
    case 0x0a03:
      CatPrint (Str, L"PciRoot(0x%x)", Acpi->UID);
      break;

    case 0x0a08:
      CatPrint (Str, L"PcieRoot(0x%x)", Acpi->UID);
      break;

    case 0x0604:
      CatPrint (Str, L"Floppy(0x%x)", Acpi->UID);
      break;

    case 0x0301:
      CatPrint (Str, L"Keyboard(0x%x)", Acpi->UID);
      break;

    case 0x0501:
      CatPrint (Str, L"Serial(0x%x)", Acpi->UID);
      break;

    case 0x0401:
      CatPrint (Str, L"ParallelPort(0x%x)", Acpi->UID);
      break;

    default:
      CatPrint (Str, L"Acpi(PNP%04x,0x%x)", EISA_ID_TO_NUM (Acpi->HID), Acpi->UID);
      break;
    }
  } else {
    CatPrint (Str, L"Acpi(0x%08x,0x%x)", Acpi->HID, Acpi->UID);
  }
}

/**
  Converts EISA identification to string.

  @param EisaId        The input EISA identification.
  @param Text          A pointer to the output string.

**/
VOID
EisaIdToText (
  IN UINT32         EisaId,
  IN OUT CHAR16     *Text
  )
{
  CHAR16 PnpIdStr[17];

  //
  //UnicodeSPrint ("%X", 0x0a03) => "0000000000000A03"
  //
  UnicodeSPrint (PnpIdStr, 17 * 2, L"%16X", EisaId >> 16);

  UnicodeSPrint (
    Text,
    sizeof (CHAR16) + sizeof (CHAR16) + sizeof (CHAR16) + sizeof (PnpIdStr),
    L"%c%c%c%s",
    '@' + ((EisaId >> 10) & 0x1f),
    '@' + ((EisaId >>  5) & 0x1f),
    '@' + ((EisaId >>  0) & 0x1f),
    PnpIdStr + (16 - 4)
    );
}

/**
  Converts a ACPI extended HID device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextAcpiEx (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ACPI_EXTENDED_HID_DEVICE_PATH  *AcpiEx;
  CHAR8                          *HIDStr;
  CHAR8                          *UIDStr;
  CHAR8                          *CIDStr;
  CHAR16                         HIDText[11];
  CHAR16                         CIDText[11];

  AcpiEx = DevPath;
  HIDStr = (CHAR8 *) (((UINT8 *) AcpiEx) + sizeof (ACPI_EXTENDED_HID_DEVICE_PATH));
  UIDStr = HIDStr + AsciiStrLen (HIDStr) + 1;
  CIDStr = UIDStr + AsciiStrLen (UIDStr) + 1;

  EisaIdToText (AcpiEx->HID, HIDText);
  EisaIdToText (AcpiEx->CID, CIDText);

  if ((*HIDStr == '\0') && (*CIDStr == '\0') && (AcpiEx->UID == 0)) {
    //
    // use AcpiExp()
    //
    CatPrint (
      Str,
      L"AcpiExp(%s,%s,%a)",
      HIDText,
      CIDText,
      UIDStr
      );
  } else {
    if (AllowShortcuts) {
      //
      // display only
      //
      if (AcpiEx->HID == 0) {
        CatPrint (Str, L"AcpiEx(%a,", HIDStr);
      } else {
        CatPrint (Str, L"AcpiEx(%s,", HIDText);
      }

      if (AcpiEx->UID == 0) {
        CatPrint (Str, L"%a,", UIDStr);
      } else {
        CatPrint (Str, L"0x%x,", AcpiEx->UID);
      }

      if (AcpiEx->CID == 0) {
        CatPrint (Str, L"%a)", CIDStr);
      } else {
        CatPrint (Str, L"%s)", CIDText);
      }
    } else {
      CatPrint (
        Str,
        L"AcpiEx(%s,%s,0x%x,%a,%a,%a)",
        HIDText,
        CIDText,
        AcpiEx->UID,
        HIDStr,
        CIDStr,
        UIDStr
        );
    }
  }
}

/**
  Converts a ACPI address device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextAcpiAdr (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ACPI_ADR_DEVICE_PATH    *AcpiAdr;
  UINT16                  Index;
  UINT16                  Length;
  UINT16                  AdditionalAdrCount;

  AcpiAdr            = DevPath;
  Length             = (UINT16) DevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) AcpiAdr);
  AdditionalAdrCount = (UINT16) ((Length - 8) / 4);

  CatPrint (Str, L"AcpiAdr(0x%x", AcpiAdr->ADR);
  for (Index = 0; Index < AdditionalAdrCount; Index++) {
    CatPrint (Str, L",0x%x", *(UINT32 *) ((UINT8 *) AcpiAdr + 8 + Index * 4));
  }
  CatPrint (Str, L")");
}

/**
  Converts a ATAPI device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextAtapi (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ATAPI_DEVICE_PATH *Atapi;

  Atapi = DevPath;

  if (DisplayOnly) {
    CatPrint (Str, L"Ata(0x%x)", Atapi->Lun);
  } else {
    CatPrint (
      Str,
      L"Ata(%s,%s,0x%x)",
      (Atapi->PrimarySecondary == 1) ? L"Secondary" : L"Primary",
      (Atapi->SlaveMaster == 1) ? L"Slave" : L"Master",
      Atapi->Lun
      );
  }
}

/**
  Converts a SCSI device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextScsi (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  SCSI_DEVICE_PATH  *Scsi;

  Scsi = DevPath;
  CatPrint (Str, L"Scsi(0x%x,0x%x)", Scsi->Pun, Scsi->Lun);
}

/**
  Converts a Fibre device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextFibre (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  FIBRECHANNEL_DEVICE_PATH  *Fibre;

  Fibre = DevPath;
  CatPrint (Str, L"Fibre(0x%lx,0x%lx)", Fibre->WWN, Fibre->Lun);
}

/**
  Converts a FibreEx device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextFibreEx (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  FIBRECHANNELEX_DEVICE_PATH  *FibreEx;
  UINTN                       Index;

  FibreEx = DevPath;
  CatPrint (Str, L"FibreEx(0x");
  for (Index = 0; Index < sizeof (FibreEx->WWN) / sizeof (FibreEx->WWN[0]); Index++) {
    CatPrint (Str, L"%02x", FibreEx->WWN[Index]);
  }
  CatPrint (Str, L",0x");
  for (Index = 0; Index < sizeof (FibreEx->Lun) / sizeof (FibreEx->Lun[0]); Index++) {
    CatPrint (Str, L"%02x", FibreEx->Lun[Index]);
  }
  CatPrint (Str, L")");
}

/**
  Converts a Sas Ex device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextSasEx (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  SASEX_DEVICE_PATH  *SasEx;
  UINTN              Index;

  SasEx = DevPath;
  CatPrint (Str, L"SasEx(0x");

  for (Index = 0; Index < sizeof (SasEx->SasAddress) / sizeof (SasEx->SasAddress[0]); Index++) {
    CatPrint (Str, L"%02x", SasEx->SasAddress[Index]);
  }
  CatPrint (Str, L",0x");
  for (Index = 0; Index < sizeof (SasEx->Lun) / sizeof (SasEx->Lun[0]); Index++) {
    CatPrint (Str, L"%02x", SasEx->Lun[Index]);
  }
  CatPrint (Str, L",0x%x,", SasEx->RelativeTargetPort);

  if ((SasEx->DeviceTopology & 0x0f) == 0) {
    CatPrint (Str, L"NoTopology,0,0,0");
  } else if (((SasEx->DeviceTopology & 0x0f) == 1) || ((SasEx->DeviceTopology & 0x0f) == 2)) {
    CatPrint (
      Str,
      L"%s,%s,%s,",
      ((SasEx->DeviceTopology & (0x1 << 4)) != 0) ? L"SATA" : L"SAS",
      ((SasEx->DeviceTopology & (0x1 << 5)) != 0) ? L"External" : L"Internal",
      ((SasEx->DeviceTopology & (0x1 << 6)) != 0) ? L"Expanded" : L"Direct"
      );
    if ((SasEx->DeviceTopology & 0x0f) == 1) {
      CatPrint (Str, L"0");
    } else {
      CatPrint (Str, L"0x%x", (SasEx->DeviceTopology >> 8) & 0xff);
    }
  } else {
    CatPrint (Str, L"0,0,0,0");
  }

  CatPrint (Str, L")");
  return ;

}

/**
  Converts a 1394 device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToText1394 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  F1394_DEVICE_PATH *F1394DevPath;

  F1394DevPath = DevPath;
  //
  // Guid has format of IEEE-EUI64
  //
  CatPrint (Str, L"I1394(%016lx)", F1394DevPath->Guid);
}

/**
  Converts a USB device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextUsb (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  USB_DEVICE_PATH *Usb;

  Usb = DevPath;
  CatPrint (Str, L"USB(0x%x,0x%x)", Usb->ParentPortNumber, Usb->InterfaceNumber);
}

/**
  Converts a USB WWID device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextUsbWWID (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  USB_WWID_DEVICE_PATH  *UsbWWId;
  CHAR16                *SerialNumberStr;
  CHAR16                *NewStr;
  UINT16                Length;

  UsbWWId = DevPath;

  SerialNumberStr = (CHAR16 *) ((UINT8 *) UsbWWId + sizeof (USB_WWID_DEVICE_PATH));
  Length = (UINT16) ((DevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) UsbWWId) - sizeof (USB_WWID_DEVICE_PATH)) / sizeof (CHAR16));
  if (SerialNumberStr [Length - 1] != 0) {
    //
    // In case no NULL terminator in SerialNumber, create a new one with NULL terminator
    //
    NewStr = AllocateCopyPool ((Length + 1) * sizeof (CHAR16), SerialNumberStr);
    ASSERT (NewStr != NULL);
    NewStr [Length] = 0;
    SerialNumberStr = NewStr;
  }

  CatPrint (
    Str,
    L"UsbWwid(0x%x,0x%x,0x%x,\"%s\")",
    UsbWWId->VendorId,
    UsbWWId->ProductId,
    UsbWWId->InterfaceNumber,
    SerialNumberStr
    );
}

/**
  Converts a Logic Unit device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextLogicalUnit (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  DEVICE_LOGICAL_UNIT_DEVICE_PATH *LogicalUnit;

  LogicalUnit = DevPath;
  CatPrint (Str, L"Unit(0x%x)", LogicalUnit->Lun);
}

/**
  Converts a USB class device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextUsbClass (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  USB_CLASS_DEVICE_PATH *UsbClass;
  BOOLEAN               IsKnownSubClass;


  UsbClass = DevPath;

  IsKnownSubClass = TRUE;
  switch (UsbClass->DeviceClass) {
  case USB_CLASS_AUDIO:
    CatPrint (Str, L"UsbAudio");
    break;

  case USB_CLASS_CDCCONTROL:
    CatPrint (Str, L"UsbCDCControl");
    break;

  case USB_CLASS_HID:
    CatPrint (Str, L"UsbHID");
    break;

  case USB_CLASS_IMAGE:
    CatPrint (Str, L"UsbImage");
    break;

  case USB_CLASS_PRINTER:
    CatPrint (Str, L"UsbPrinter");
    break;

  case USB_CLASS_MASS_STORAGE:
    CatPrint (Str, L"UsbMassStorage");
    break;

  case USB_CLASS_HUB:
    CatPrint (Str, L"UsbHub");
    break;

  case USB_CLASS_CDCDATA:
    CatPrint (Str, L"UsbCDCData");
    break;

  case USB_CLASS_SMART_CARD:
    CatPrint (Str, L"UsbSmartCard");
    break;

  case USB_CLASS_VIDEO:
    CatPrint (Str, L"UsbVideo");
    break;

  case USB_CLASS_DIAGNOSTIC:
    CatPrint (Str, L"UsbDiagnostic");
    break;

  case USB_CLASS_WIRELESS:
    CatPrint (Str, L"UsbWireless");
    break;

  default:
    IsKnownSubClass = FALSE;
    break;
  }

  if (IsKnownSubClass) {
    CatPrint (
      Str,
      L"(0x%x,0x%x,0x%x,0x%x)",
      UsbClass->VendorId,
      UsbClass->ProductId,
      UsbClass->DeviceSubClass,
      UsbClass->DeviceProtocol
      );
    return;
  }

  if (UsbClass->DeviceClass == USB_CLASS_RESERVE) {
    if (UsbClass->DeviceSubClass == USB_SUBCLASS_FW_UPDATE) {
      CatPrint (
        Str,
        L"UsbDeviceFirmwareUpdate(0x%x,0x%x,0x%x)",
        UsbClass->VendorId,
        UsbClass->ProductId,
        UsbClass->DeviceProtocol
        );
      return;
    } else if (UsbClass->DeviceSubClass == USB_SUBCLASS_IRDA_BRIDGE) {
      CatPrint (
        Str,
        L"UsbIrdaBridge(0x%x,0x%x,0x%x)",
        UsbClass->VendorId,
        UsbClass->ProductId,
        UsbClass->DeviceProtocol
        );
      return;
    } else if (UsbClass->DeviceSubClass == USB_SUBCLASS_TEST) {
      CatPrint (
        Str,
        L"UsbTestAndMeasurement(0x%x,0x%x,0x%x)",
        UsbClass->VendorId,
        UsbClass->ProductId,
        UsbClass->DeviceProtocol
        );
      return;
    }
  }

  CatPrint (
    Str,
    L"UsbClass(0x%x,0x%x,0x%x,0x%x,0x%x)",
    UsbClass->VendorId,
    UsbClass->ProductId,
    UsbClass->DeviceClass,
    UsbClass->DeviceSubClass,
    UsbClass->DeviceProtocol
    );
}

/**
  Converts a SATA device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextSata (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  SATA_DEVICE_PATH *Sata;

  Sata = DevPath;
  if ((Sata->PortMultiplierPortNumber & SATA_HBA_DIRECT_CONNECT_FLAG) != 0) {
    CatPrint (
      Str,
      L"Sata(0x%x,0x%x)",
      Sata->HBAPortNumber,
      Sata->Lun
      );
  } else {
    CatPrint (
      Str,
      L"Sata(0x%x,0x%x,0x%x)",
      Sata->HBAPortNumber,
      Sata->PortMultiplierPortNumber,
      Sata->Lun
      );
  }
}

/**
  Converts a I20 device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextI2O (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  I2O_DEVICE_PATH *I2ODevPath;

  I2ODevPath = DevPath;
  CatPrint (Str, L"I2O(0x%x)", I2ODevPath->Tid);
}

/**
  Converts a MAC address device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextMacAddr (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MAC_ADDR_DEVICE_PATH  *MacDevPath;
  UINTN                 HwAddressSize;
  UINTN                 Index;

  MacDevPath = DevPath;

  HwAddressSize = sizeof (EFI_MAC_ADDRESS);
  if (MacDevPath->IfType == 0x01 || MacDevPath->IfType == 0x00) {
    HwAddressSize = 6;
  }

  CatPrint (Str, L"MAC(");

  for (Index = 0; Index < HwAddressSize; Index++) {
    CatPrint (Str, L"%02x", MacDevPath->MacAddress.Addr[Index]);
  }

  CatPrint (Str, L",0x%x)", MacDevPath->IfType);
}

/**
  Converts network protocol string to its text representation.

  @param Str             The string representative of input device.
  @param Protocol        The network protocol ID.

**/
VOID
CatNetworkProtocol (
  IN OUT POOL_PRINT  *Str,
  IN UINT16          Protocol
  )
{
  if (Protocol == RFC_1700_TCP_PROTOCOL) {
    CatPrint (Str, L"TCP");
  } else if (Protocol == RFC_1700_UDP_PROTOCOL) {
    CatPrint (Str, L"UDP");
  } else {
    CatPrint (Str, L"0x%x", Protocol);
  }
}

/**
  Converts a IPv4 device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextIPv4 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  IPv4_DEVICE_PATH  *IPDevPath;

  IPDevPath = DevPath;
  if (DisplayOnly) {
    CatPrint (
      Str,
      L"IPv4(%d.%d.%d.%d)",
      IPDevPath->RemoteIpAddress.Addr[0],
      IPDevPath->RemoteIpAddress.Addr[1],
      IPDevPath->RemoteIpAddress.Addr[2],
      IPDevPath->RemoteIpAddress.Addr[3]
      );
    return ;
  }

  CatPrint (
    Str,
    L"IPv4(%d.%d.%d.%d,",
    IPDevPath->RemoteIpAddress.Addr[0],
    IPDevPath->RemoteIpAddress.Addr[1],
    IPDevPath->RemoteIpAddress.Addr[2],
    IPDevPath->RemoteIpAddress.Addr[3]
    );

  CatNetworkProtocol (
    Str,
    IPDevPath->Protocol
    );

  CatPrint (
    Str,
    L",%s,%d.%d.%d.%d",
    IPDevPath->StaticIpAddress ? L"Static" : L"DHCP",
    IPDevPath->LocalIpAddress.Addr[0],
    IPDevPath->LocalIpAddress.Addr[1],
    IPDevPath->LocalIpAddress.Addr[2],
    IPDevPath->LocalIpAddress.Addr[3]
    );
  if (DevicePathNodeLength (IPDevPath) == sizeof (IPv4_DEVICE_PATH)) {
    CatPrint (
      Str,
      L",%d.%d.%d.%d,%d.%d.%d.%d",
      IPDevPath->GatewayIpAddress.Addr[0],
      IPDevPath->GatewayIpAddress.Addr[1],
      IPDevPath->GatewayIpAddress.Addr[2],
      IPDevPath->GatewayIpAddress.Addr[3],
      IPDevPath->SubnetMask.Addr[0],
      IPDevPath->SubnetMask.Addr[1],
      IPDevPath->SubnetMask.Addr[2],
      IPDevPath->SubnetMask.Addr[3]
      );
  }
  CatPrint (Str, L")");
}

/**
  Converts a IPv6 device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextIPv6 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  IPv6_DEVICE_PATH  *IPDevPath;

  IPDevPath = DevPath;
  if (DisplayOnly) {
    CatPrint (
      Str,
      L"IPv6(%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x)",
      IPDevPath->RemoteIpAddress.Addr[0],
      IPDevPath->RemoteIpAddress.Addr[1],
      IPDevPath->RemoteIpAddress.Addr[2],
      IPDevPath->RemoteIpAddress.Addr[3],
      IPDevPath->RemoteIpAddress.Addr[4],
      IPDevPath->RemoteIpAddress.Addr[5],
      IPDevPath->RemoteIpAddress.Addr[6],
      IPDevPath->RemoteIpAddress.Addr[7],
      IPDevPath->RemoteIpAddress.Addr[8],
      IPDevPath->RemoteIpAddress.Addr[9],
      IPDevPath->RemoteIpAddress.Addr[10],
      IPDevPath->RemoteIpAddress.Addr[11],
      IPDevPath->RemoteIpAddress.Addr[12],
      IPDevPath->RemoteIpAddress.Addr[13],
      IPDevPath->RemoteIpAddress.Addr[14],
      IPDevPath->RemoteIpAddress.Addr[15]
      );
    return ;
  }

  CatPrint (
    Str,
    L"IPv6(%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x,",
    IPDevPath->RemoteIpAddress.Addr[0],
    IPDevPath->RemoteIpAddress.Addr[1],
    IPDevPath->RemoteIpAddress.Addr[2],
    IPDevPath->RemoteIpAddress.Addr[3],
    IPDevPath->RemoteIpAddress.Addr[4],
    IPDevPath->RemoteIpAddress.Addr[5],
    IPDevPath->RemoteIpAddress.Addr[6],
    IPDevPath->RemoteIpAddress.Addr[7],
    IPDevPath->RemoteIpAddress.Addr[8],
    IPDevPath->RemoteIpAddress.Addr[9],
    IPDevPath->RemoteIpAddress.Addr[10],
    IPDevPath->RemoteIpAddress.Addr[11],
    IPDevPath->RemoteIpAddress.Addr[12],
    IPDevPath->RemoteIpAddress.Addr[13],
    IPDevPath->RemoteIpAddress.Addr[14],
    IPDevPath->RemoteIpAddress.Addr[15]
    );
    
  CatNetworkProtocol (
    Str,
    IPDevPath->Protocol
    );

  switch (IPDevPath->IpAddressOrigin) {
    case 0:
      CatPrint (Str, L",Static");
      break;
    case 1:
      CatPrint (Str, L",StatelessAutoConfigure");
      break;
    default:
      CatPrint (Str, L",StatefulAutoConfigure");
      break;
  }

  CatPrint (
    Str,
    L",%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
    IPDevPath->LocalIpAddress.Addr[0],
    IPDevPath->LocalIpAddress.Addr[1],
    IPDevPath->LocalIpAddress.Addr[2],
    IPDevPath->LocalIpAddress.Addr[3],
    IPDevPath->LocalIpAddress.Addr[4],
    IPDevPath->LocalIpAddress.Addr[5],
    IPDevPath->LocalIpAddress.Addr[6],
    IPDevPath->LocalIpAddress.Addr[7],
    IPDevPath->LocalIpAddress.Addr[8],
    IPDevPath->LocalIpAddress.Addr[9],
    IPDevPath->LocalIpAddress.Addr[10],
    IPDevPath->LocalIpAddress.Addr[11],
    IPDevPath->LocalIpAddress.Addr[12],
    IPDevPath->LocalIpAddress.Addr[13],
    IPDevPath->LocalIpAddress.Addr[14],
    IPDevPath->LocalIpAddress.Addr[15]
    );

  if (DevicePathNodeLength (IPDevPath) == sizeof (IPv6_DEVICE_PATH)) {
    CatPrint (
      Str,
      L",0x%x,%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
      IPDevPath->PrefixLength,
      IPDevPath->GatewayIpAddress.Addr[0],
      IPDevPath->GatewayIpAddress.Addr[1],
      IPDevPath->GatewayIpAddress.Addr[2],
      IPDevPath->GatewayIpAddress.Addr[3],
      IPDevPath->GatewayIpAddress.Addr[4],
      IPDevPath->GatewayIpAddress.Addr[5],
      IPDevPath->GatewayIpAddress.Addr[6],
      IPDevPath->GatewayIpAddress.Addr[7],
      IPDevPath->GatewayIpAddress.Addr[8],
      IPDevPath->GatewayIpAddress.Addr[9],
      IPDevPath->GatewayIpAddress.Addr[10],
      IPDevPath->GatewayIpAddress.Addr[11],
      IPDevPath->GatewayIpAddress.Addr[12],
      IPDevPath->GatewayIpAddress.Addr[13],
      IPDevPath->GatewayIpAddress.Addr[14],
      IPDevPath->GatewayIpAddress.Addr[15]
      );
  }
  CatPrint (Str, L")");
}

/**
  Converts an Infini Band device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextInfiniBand (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  INFINIBAND_DEVICE_PATH  *InfiniBand;

  InfiniBand = DevPath;
  CatPrint (
    Str,
    L"Infiniband(0x%x,%g,0x%lx,0x%lx,0x%lx)",
    InfiniBand->ResourceFlags,
    InfiniBand->PortGid,
    InfiniBand->ServiceId,
    InfiniBand->TargetPortId,
    InfiniBand->DeviceId
    );
}

/**
  Converts a UART device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextUart (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  UART_DEVICE_PATH  *Uart;
  CHAR8             Parity;

  Uart = DevPath;
  switch (Uart->Parity) {
  case 0:
    Parity = 'D';
    break;

  case 1:
    Parity = 'N';
    break;

  case 2:
    Parity = 'E';
    break;

  case 3:
    Parity = 'O';
    break;

  case 4:
    Parity = 'M';
    break;

  case 5:
    Parity = 'S';
    break;

  default:
    Parity = 'x';
    break;
  }

  if (Uart->BaudRate == 0) {
    CatPrint (Str, L"Uart(DEFAULT,");
  } else {
    CatPrint (Str, L"Uart(%ld,", Uart->BaudRate);
  }

  if (Uart->DataBits == 0) {
    CatPrint (Str, L"DEFAULT,");
  } else {
    CatPrint (Str, L"%d,", Uart->DataBits);
  }

  CatPrint (Str, L"%c,", Parity);

  switch (Uart->StopBits) {
  case 0:
    CatPrint (Str, L"D)");
    break;

  case 1:
    CatPrint (Str, L"1)");
    break;

  case 2:
    CatPrint (Str, L"1.5)");
    break;

  case 3:
    CatPrint (Str, L"2)");
    break;

  default:
    CatPrint (Str, L"x)");
    break;
  }
}

/**
  Converts an iSCSI device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextiSCSI (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ISCSI_DEVICE_PATH_WITH_NAME *ISCSIDevPath;
  UINT16                      Options;

  ISCSIDevPath = DevPath;
  CatPrint (
    Str,
    L"iSCSI(%a,0x%x,0x%lx,",
    ISCSIDevPath->TargetName,
    ISCSIDevPath->TargetPortalGroupTag,
    ISCSIDevPath->Lun
    );

  Options = ISCSIDevPath->LoginOption;
  CatPrint (Str, L"%s,", (((Options >> 1) & 0x0001) != 0) ? L"CRC32C" : L"None");
  CatPrint (Str, L"%s,", (((Options >> 3) & 0x0001) != 0) ? L"CRC32C" : L"None");
  if (((Options >> 11) & 0x0001) != 0) {
    CatPrint (Str, L"%s,", L"None");
  } else if (((Options >> 12) & 0x0001) != 0) {
    CatPrint (Str, L"%s,", L"CHAP_UNI");
  } else {
    CatPrint (Str, L"%s,", L"CHAP_BI");

  }

  CatPrint (Str, L"%s)", (ISCSIDevPath->NetworkProtocol == 0) ? L"TCP" : L"reserved");
}

/**
  Converts a VLAN device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextVlan (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  VLAN_DEVICE_PATH  *Vlan;

  Vlan = DevPath;
  CatPrint (Str, L"Vlan(%d)", Vlan->VlanId);
}

/**
  Converts a Hard drive device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextHardDrive (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  HARDDRIVE_DEVICE_PATH *Hd;

  Hd = DevPath;
  switch (Hd->SignatureType) {
  case SIGNATURE_TYPE_MBR:
    CatPrint (
      Str,
      L"HD(%d,%s,0x%08x,",
      Hd->PartitionNumber,
      L"MBR",
      *((UINT32 *) (&(Hd->Signature[0])))
      );
    break;

  case SIGNATURE_TYPE_GUID:
    CatPrint (
      Str,
      L"HD(%d,%s,%g,",
      Hd->PartitionNumber,
      L"GPT",
      (EFI_GUID *) &(Hd->Signature[0])
      );
    break;

  default:
    CatPrint (
      Str,
      L"HD(%d,%d,0,",
      Hd->PartitionNumber,
      Hd->SignatureType
      );
    break;
  }

  CatPrint (Str, L"0x%lx,0x%lx)", Hd->PartitionStart, Hd->PartitionSize);
}

/**
  Converts a CDROM device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextCDROM (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  CDROM_DEVICE_PATH *Cd;

  Cd = DevPath;
  if (DisplayOnly) {
    CatPrint (Str, L"CDROM(0x%x)", Cd->BootEntry);
    return ;
  }

  CatPrint (Str, L"CDROM(0x%x,0x%lx,0x%lx)", Cd->BootEntry, Cd->PartitionStart, Cd->PartitionSize);
}

/**
  Converts a File device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextFilePath (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  FILEPATH_DEVICE_PATH  *Fp;

  Fp = DevPath;
  CatPrint (Str, L"%s", Fp->PathName);
}

/**
  Converts a Media protocol device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextMediaProtocol (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MEDIA_PROTOCOL_DEVICE_PATH  *MediaProt;

  MediaProt = DevPath;
  CatPrint (Str, L"Media(%g)", &MediaProt->Protocol);
}

/**
  Converts a Firmware Volume device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextFv (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MEDIA_FW_VOL_DEVICE_PATH  *Fv;

  Fv = DevPath;
  CatPrint (Str, L"Fv(%g)", &Fv->FvName);
}

/**
  Converts a Firmware Volume File device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextFvFile (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvFile;

  FvFile = DevPath;
  CatPrint (Str, L"FvFile(%g)", &FvFile->FvFileName);
}

/**
  Converts a Relative Offset device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathRelativeOffsetRange (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath,
  IN BOOLEAN              DisplayOnly,
  IN BOOLEAN              AllowShortcuts
  )
{
  MEDIA_RELATIVE_OFFSET_RANGE_DEVICE_PATH *Offset;

  Offset = DevPath;
  CatPrint (
    Str,
    L"Offset(0x%lx,0x%lx)",
    Offset->StartingOffset,
    Offset->EndingOffset
    );
}

/**
  Converts a BIOS Boot Specification device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextBBS (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  BBS_BBS_DEVICE_PATH *Bbs;
  CHAR16              *Type;

  Bbs = DevPath;
  switch (Bbs->DeviceType) {
  case BBS_TYPE_FLOPPY:
    Type = L"Floppy";
    break;

  case BBS_TYPE_HARDDRIVE:
    Type = L"HD";
    break;

  case BBS_TYPE_CDROM:
    Type = L"CDROM";
    break;

  case BBS_TYPE_PCMCIA:
    Type = L"PCMCIA";
    break;

  case BBS_TYPE_USB:
    Type = L"USB";
    break;

  case BBS_TYPE_EMBEDDED_NETWORK:
    Type = L"Network";
    break;

  default:
    Type = NULL;
    break;
  }

  if (Type != NULL) {
    CatPrint (Str, L"BBS(%s,%a", Type, Bbs->String);
  } else {
    CatPrint (Str, L"BBS(0x%x,%a", Bbs->DeviceType, Bbs->String);
  }

  if (DisplayOnly) {
    CatPrint (Str, L")");
    return ;
  }

  CatPrint (Str, L",0x%x)", Bbs->StatusFlag);
}

/**
  Converts an End-of-Device-Path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextEndInstance (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  CatPrint (Str, L",");
}

/**
  Converts an unknown device path structure to its string representative.

  @param Str             The string representative of input device.
  @param DevPath         The input device path structure.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

**/
VOID
DevPathToTextNodeUnknown (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  CatPrint (Str, L"?");
}

GLOBAL_REMOVE_IF_UNREFERENCED const DEVICE_PATH_TO_TEXT_TABLE DevPathToTextTable[] = {
  {HARDWARE_DEVICE_PATH, HW_PCI_DP, DevPathToTextPci},
  {HARDWARE_DEVICE_PATH, HW_PCCARD_DP, DevPathToTextPccard},
  {HARDWARE_DEVICE_PATH, HW_MEMMAP_DP, DevPathToTextMemMap},
  {HARDWARE_DEVICE_PATH, HW_VENDOR_DP, DevPathToTextVendor},
  {HARDWARE_DEVICE_PATH, HW_CONTROLLER_DP, DevPathToTextController},
  {ACPI_DEVICE_PATH, ACPI_DP, DevPathToTextAcpi},
  {ACPI_DEVICE_PATH, ACPI_EXTENDED_DP, DevPathToTextAcpiEx},
  {ACPI_DEVICE_PATH, ACPI_ADR_DP, DevPathToTextAcpiAdr},
  {MESSAGING_DEVICE_PATH, MSG_ATAPI_DP, DevPathToTextAtapi},
  {MESSAGING_DEVICE_PATH, MSG_SCSI_DP, DevPathToTextScsi},
  {MESSAGING_DEVICE_PATH, MSG_FIBRECHANNEL_DP, DevPathToTextFibre},
  {MESSAGING_DEVICE_PATH, MSG_FIBRECHANNELEX_DP, DevPathToTextFibreEx},
  {MESSAGING_DEVICE_PATH, MSG_SASEX_DP, DevPathToTextSasEx},
  {MESSAGING_DEVICE_PATH, MSG_1394_DP, DevPathToText1394},
  {MESSAGING_DEVICE_PATH, MSG_USB_DP, DevPathToTextUsb},
  {MESSAGING_DEVICE_PATH, MSG_USB_WWID_DP, DevPathToTextUsbWWID},
  {MESSAGING_DEVICE_PATH, MSG_DEVICE_LOGICAL_UNIT_DP, DevPathToTextLogicalUnit},
  {MESSAGING_DEVICE_PATH, MSG_USB_CLASS_DP, DevPathToTextUsbClass},
  {MESSAGING_DEVICE_PATH, MSG_SATA_DP, DevPathToTextSata},
  {MESSAGING_DEVICE_PATH, MSG_I2O_DP, DevPathToTextI2O},
  {MESSAGING_DEVICE_PATH, MSG_MAC_ADDR_DP, DevPathToTextMacAddr},
  {MESSAGING_DEVICE_PATH, MSG_IPv4_DP, DevPathToTextIPv4},
  {MESSAGING_DEVICE_PATH, MSG_IPv6_DP, DevPathToTextIPv6},
  {MESSAGING_DEVICE_PATH, MSG_INFINIBAND_DP, DevPathToTextInfiniBand},
  {MESSAGING_DEVICE_PATH, MSG_UART_DP, DevPathToTextUart},
  {MESSAGING_DEVICE_PATH, MSG_VENDOR_DP, DevPathToTextVendor},
  {MESSAGING_DEVICE_PATH, MSG_ISCSI_DP, DevPathToTextiSCSI},
  {MESSAGING_DEVICE_PATH, MSG_VLAN_DP, DevPathToTextVlan},
  {MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP, DevPathToTextHardDrive},
  {MEDIA_DEVICE_PATH, MEDIA_CDROM_DP, DevPathToTextCDROM},
  {MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP, DevPathToTextVendor},
  {MEDIA_DEVICE_PATH, MEDIA_PROTOCOL_DP, DevPathToTextMediaProtocol},
  {MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, DevPathToTextFilePath},
  {MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_VOL_DP, DevPathToTextFv},
  {MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_FILE_DP, DevPathToTextFvFile},
  {MEDIA_DEVICE_PATH, MEDIA_RELATIVE_OFFSET_RANGE_DP, DevPathRelativeOffsetRange},
  {BBS_DEVICE_PATH, BBS_BBS_DP, DevPathToTextBBS},
  {END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE, DevPathToTextEndInstance},
  {0, 0, NULL}
};

/**
  Converts a device node to its string representation.

  @param DeviceNode        A Pointer to the device node to be converted.
  @param DisplayOnly       If DisplayOnly is TRUE, then the shorter text representation
                           of the display node is used, where applicable. If DisplayOnly
                           is FALSE, then the longer text representation of the display node
                           is used.
  @param AllowShortcuts    If AllowShortcuts is TRUE, then the shortcut forms of text
                           representation for a device node can be used, where applicable.

  @return A pointer to the allocated text representation of the device node or NULL if DeviceNode
          is NULL or there was insufficient memory.

**/
CHAR16 *
EFIAPI
ConvertDeviceNodeToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
  IN BOOLEAN                         DisplayOnly,
  IN BOOLEAN                         AllowShortcuts
  )
{
  POOL_PRINT  Str;
  UINTN       Index;
  VOID        (*DumpNode)(POOL_PRINT *, VOID *, BOOLEAN, BOOLEAN);

  if (DeviceNode == NULL) {
    return NULL;
  }

  ZeroMem (&Str, sizeof (Str));

  //
  // Process the device path node
  //
  DumpNode = NULL;
  for (Index = 0; DevPathToTextTable[Index].Function != NULL; Index++) {
    if (DevicePathType (DeviceNode) == DevPathToTextTable[Index].Type &&
        DevicePathSubType (DeviceNode) == DevPathToTextTable[Index].SubType
        ) {
      DumpNode = DevPathToTextTable[Index].Function;
      break;
    }
  }
  //
  // If not found, use a generic function
  //
  if (DumpNode == NULL) {
    DumpNode = DevPathToTextNodeUnknown;
  }

  //
  // Print this node
  //
  DumpNode (&Str, (VOID *) DeviceNode, DisplayOnly, AllowShortcuts);

  ASSERT (Str.Str != NULL);
  return Str.Str;
}

/**
  Converts a device path to its text representation.

  @param DevicePath      A Pointer to the device to be converted.
  @param DisplayOnly     If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
  @param AllowShortcuts  If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  @return A pointer to the allocated text representation of the device path or
          NULL if DeviceNode is NULL or there was insufficient memory.

**/
CHAR16 *
EFIAPI
ConvertDevicePathToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN BOOLEAN                          DisplayOnly,
  IN BOOLEAN                          AllowShortcuts
  )
{
  POOL_PRINT                Str;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  EFI_DEVICE_PATH_PROTOCOL  *AlignedDevPathNode;
  UINTN                     Index;
  VOID                      (*DumpNode) (POOL_PRINT *, VOID *, BOOLEAN, BOOLEAN);

  if (DevicePath == NULL) {
    return NULL;
  }

  ZeroMem (&Str, sizeof (Str));

  //
  // Process each device path node
  //
  DevPathNode = (EFI_DEVICE_PATH_PROTOCOL *) DevicePath;
  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Find the handler to dump this device path node
    //
    DumpNode = NULL;
    for (Index = 0; DevPathToTextTable[Index].Function != NULL; Index += 1) {

      if (DevicePathType (DevPathNode) == DevPathToTextTable[Index].Type &&
          DevicePathSubType (DevPathNode) == DevPathToTextTable[Index].SubType
          ) {
        DumpNode = DevPathToTextTable[Index].Function;
        break;
      }
    }
    //
    // If not found, use a generic function
    //
    if (!DumpNode) {
      DumpNode = DevPathToTextNodeUnknown;
    }
    //
    //  Put a path separator in if needed
    //
    if ((Str.Length != 0) && (DumpNode != DevPathToTextEndInstance)) {
      if (*(Str.Str + Str.Length / sizeof (CHAR16) - 1) != L',') {
        CatPrint (&Str, L"/");
      }
    }
    
    AlignedDevPathNode = AllocateCopyPool (DevicePathNodeLength (DevPathNode), DevPathNode);
    //
    // Print this node of the device path
    //
    DumpNode (&Str, AlignedDevPathNode, DisplayOnly, AllowShortcuts);
    FreePool (AlignedDevPathNode);
    
    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }

  if (Str.Str == NULL) {
    return AllocateZeroPool (sizeof (CHAR16));
  } else {
    return Str.Str;
  }
}
