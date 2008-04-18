/** @file
  DevicePathToText protocol as defined in the UEFI 2.0 specification.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DevicePath.h"

STATIC
EFI_DEVICE_PATH_PROTOCOL *
UnpackDevicePath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

  Routine Description:
    Function unpacks a device path data structure so that all the nodes of a device path 
    are naturally aligned.

  Arguments:
    DevPath        - A pointer to a device path data structure

  Returns:
    If the memory for the device path is successfully allocated, then a pointer to the 
    new device path is returned.  Otherwise, NULL is returned.

--*/
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Src;
  EFI_DEVICE_PATH_PROTOCOL  *Dest;
  EFI_DEVICE_PATH_PROTOCOL  *NewPath;
  UINTN                     Size;

  if (DevPath == NULL) {
    return NULL;
  }
  //
  // Walk device path and round sizes to valid boundries
  //
  Src   = DevPath;
  Size  = 0;
  for (;;) {
    Size += DevicePathNodeLength (Src);
    Size += ALIGN_SIZE (Size);

    if (IsDevicePathEnd (Src)) {
      break;
    }

    Src = (EFI_DEVICE_PATH_PROTOCOL *) NextDevicePathNode (Src);
  }
  //
  // Allocate space for the unpacked path
  //
  NewPath = AllocateZeroPool (Size);
  if (NewPath != NULL) {

    ASSERT (((UINTN) NewPath) % MIN_ALIGNMENT_SIZE == 0);

    //
    // Copy each node
    //
    Src   = DevPath;
    Dest  = NewPath;
    for (;;) {
      Size = DevicePathNodeLength (Src);
      CopyMem (Dest, Src, Size);
      Size += ALIGN_SIZE (Size);
      SetDevicePathNodeLength (Dest, Size);
      Dest->Type |= EFI_DP_TYPE_UNPACKED;
      Dest = (EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) Dest) + Size);

      if (IsDevicePathEnd (Src)) {
        break;
      }

      Src = (EFI_DEVICE_PATH_PROTOCOL *) NextDevicePathNode (Src);
    }
  }

  return NewPath;
}

STATIC
VOID *
ReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  )
/*++

  Routine Description:
    Adjusts the size of a previously allocated buffer.

  Arguments:
    OldPool               - A pointer to the buffer whose size is being adjusted.
    OldSize               - The size of the current buffer.
    NewSize               - The size of the new buffer.

  Returns:
    EFI_SUCEESS           - The requested number of bytes were allocated.
    EFI_OUT_OF_RESOURCES  - The pool requested could not be allocated.
    EFI_INVALID_PARAMETER - The buffer was invalid.

--*/
{
  VOID  *NewPool;

  NewPool = NULL;
  if (NewSize) {
    NewPool = AllocateZeroPool (NewSize);
  }

  if (OldPool) {
    if (NewPool) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    FreePool (OldPool);
  }

  return NewPool;
}

STATIC
CHAR16 *
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *Fmt,
  ...
  )
/*++

  Routine Description:
    Concatenates a formatted unicode string to allocated pool.  
    The caller must free the resulting buffer.

  Arguments:
    Str         - Tracks the allocated pool, size in use, and 
                  amount of pool allocated.
    Fmt         - The format string

  Returns:
    Allocated buffer with the formatted string printed in it.  
    The caller must free the allocated buffer.   The buffer
    allocation is not packed.

--*/
{
  UINT16  *AppendStr;
  VA_LIST Args;
  UINTN   Size;

  AppendStr = AllocateZeroPool (0x1000);
  if (AppendStr == NULL) {
    return Str->Str;
  }

  VA_START (Args, Fmt);
  UnicodeVSPrint (AppendStr, 0x1000, Fmt, Args);
  VA_END (Args);
  if (NULL == Str->Str) {
    Size   = StrSize (AppendStr);
    Str->Str  = AllocateZeroPool (Size);
    ASSERT (Str->Str != NULL);
  } else {
    Size = StrSize (AppendStr)  - sizeof (UINT16);
    Size = Size + StrSize (Str->Str);
    Str->Str = ReallocatePool (
                Str->Str,
                StrSize (Str->Str),
                Size
                );
    ASSERT (Str->Str != NULL);
  }

  Str->MaxLen = MAX_CHAR * sizeof (UINT16);
  if (Size < Str->MaxLen) {
    StrCat (Str->Str, AppendStr);
    Str->Len = Size - sizeof (UINT16);
  }

  FreePool (AppendStr);
  return Str->Str;
}

STATIC
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

STATIC
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

STATIC
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

STATIC
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
      } else if (CompareGuid (&Vendor->Guid, &mEfiDevicePathMessagingUartFlowControlGuid)) {
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
      } else if (CompareGuid (&Vendor->Guid, &mEfiDevicePathMessagingSASGuid)) {
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
            (Info & (0x1 << 4)) ? L"SATA" : L"SAS",
            (Info & (0x1 << 5)) ? L"External" : L"Internal",
            (Info & (0x1 << 6)) ? L"Expanded" : L"Direct"
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

STATIC
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

STATIC
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

STATIC
VOID
EisaIdToText (
  IN UINT32         EisaId,
  IN OUT CHAR16    *Text
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

STATIC
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

STATIC
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

STATIC
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
      Atapi->PrimarySecondary ? L"Secondary" : L"Primary",
      Atapi->SlaveMaster ? L"Slave" : L"Master",
      Atapi->Lun
      );
  }
}

STATIC
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

STATIC
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

STATIC
VOID
DevPathToText1394 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  F1394_DEVICE_PATH *F1394;

  F1394 = DevPath;
  //
  // Guid has format of IEEE-EUI64
  //
  CatPrint (Str, L"I1394(%016lx)", F1394->Guid);
}

STATIC
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

STATIC
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

STATIC
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

STATIC
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

STATIC
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
  CatPrint (
    Str,
    L"Sata(0x%x,0x%x,0x%x)",
    (UINTN) Sata->HBAPortNumber,
    (UINTN) Sata->PortMultiplierPortNumber,
    (UINTN) Sata->Lun
    );
}

STATIC
VOID
DevPathToTextI2O (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  I2O_DEVICE_PATH *I2O;

  I2O = DevPath;
  CatPrint (Str, L"I2O(0x%x)", I2O->Tid);
}

STATIC
VOID
DevPathToTextMacAddr (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  MAC_ADDR_DEVICE_PATH  *MAC;
  UINTN                 HwAddressSize;
  UINTN                 Index;

  MAC           = DevPath;

  HwAddressSize = sizeof (EFI_MAC_ADDRESS);
  if (MAC->IfType == 0x01 || MAC->IfType == 0x00) {
    HwAddressSize = 6;
  }

  CatPrint (Str, L"MAC(");

  for (Index = 0; Index < HwAddressSize; Index++) {
    CatPrint (Str, L"%02x", MAC->MacAddress.Addr[Index]);
  }

  CatPrint (Str, L",0x%x)", MAC->IfType);
}

STATIC
VOID
DevPathToTextIPv4 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  IPv4_DEVICE_PATH  *IP;

  IP = DevPath;
  if (DisplayOnly == TRUE) {
    CatPrint (
      Str,
      L"IPv4(%d.%d.%d.%d)",
      IP->RemoteIpAddress.Addr[0],
      IP->RemoteIpAddress.Addr[1],
      IP->RemoteIpAddress.Addr[2],
      IP->RemoteIpAddress.Addr[3]
      );
    return ;
  }

  CatPrint (
    Str,
    L"IPv4(%d.%d.%d.%d,%s,%s,%d.%d.%d.%d)",
    IP->RemoteIpAddress.Addr[0],
    IP->RemoteIpAddress.Addr[1],
    IP->RemoteIpAddress.Addr[2],
    IP->RemoteIpAddress.Addr[3],
    IP->Protocol ? L"TCP" : L"UDP",
    (IP->StaticIpAddress == TRUE) ? L"Static" : L"DHCP",
    IP->LocalIpAddress.Addr[0],
    IP->LocalIpAddress.Addr[1],
    IP->LocalIpAddress.Addr[2],
    IP->LocalIpAddress.Addr[3]
    );
}

STATIC
VOID
DevPathToTextIPv6 (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  IPv6_DEVICE_PATH  *IP;

  IP = DevPath;
  if (DisplayOnly == TRUE) {
    CatPrint (
      Str,
      L"IPv6(%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x)",
      IP->RemoteIpAddress.Addr[0],
      IP->RemoteIpAddress.Addr[1],
      IP->RemoteIpAddress.Addr[2],
      IP->RemoteIpAddress.Addr[3],
      IP->RemoteIpAddress.Addr[4],
      IP->RemoteIpAddress.Addr[5],
      IP->RemoteIpAddress.Addr[6],
      IP->RemoteIpAddress.Addr[7],
      IP->RemoteIpAddress.Addr[8],
      IP->RemoteIpAddress.Addr[9],
      IP->RemoteIpAddress.Addr[10],
      IP->RemoteIpAddress.Addr[11],
      IP->RemoteIpAddress.Addr[12],
      IP->RemoteIpAddress.Addr[13],
      IP->RemoteIpAddress.Addr[14],
      IP->RemoteIpAddress.Addr[15]
      );
    return ;
  }

  CatPrint (
    Str,
    L"IPv6(%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x,%s,%s,%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x)",
    IP->RemoteIpAddress.Addr[0],
    IP->RemoteIpAddress.Addr[1],
    IP->RemoteIpAddress.Addr[2],
    IP->RemoteIpAddress.Addr[3],
    IP->RemoteIpAddress.Addr[4],
    IP->RemoteIpAddress.Addr[5],
    IP->RemoteIpAddress.Addr[6],
    IP->RemoteIpAddress.Addr[7],
    IP->RemoteIpAddress.Addr[8],
    IP->RemoteIpAddress.Addr[9],
    IP->RemoteIpAddress.Addr[10],
    IP->RemoteIpAddress.Addr[11],
    IP->RemoteIpAddress.Addr[12],
    IP->RemoteIpAddress.Addr[13],
    IP->RemoteIpAddress.Addr[14],
    IP->RemoteIpAddress.Addr[15],
    IP->Protocol ? L"TCP" : L"UDP",
    (IP->StaticIpAddress == TRUE) ? L"Static" : L"DHCP",
    IP->LocalIpAddress.Addr[0],
    IP->LocalIpAddress.Addr[1],
    IP->LocalIpAddress.Addr[2],
    IP->LocalIpAddress.Addr[3],
    IP->LocalIpAddress.Addr[4],
    IP->LocalIpAddress.Addr[5],
    IP->LocalIpAddress.Addr[6],
    IP->LocalIpAddress.Addr[7],
    IP->LocalIpAddress.Addr[8],
    IP->LocalIpAddress.Addr[9],
    IP->LocalIpAddress.Addr[10],
    IP->LocalIpAddress.Addr[11],
    IP->LocalIpAddress.Addr[12],
    IP->LocalIpAddress.Addr[13],
    IP->LocalIpAddress.Addr[14],
    IP->LocalIpAddress.Addr[15]
    );
}

STATIC
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

STATIC
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

STATIC
VOID
DevPathToTextiSCSI (
  IN OUT POOL_PRINT  *Str,
  IN VOID            *DevPath,
  IN BOOLEAN         DisplayOnly,
  IN BOOLEAN         AllowShortcuts
  )
{
  ISCSI_DEVICE_PATH_WITH_NAME *iSCSI;
  UINT16                      Options;

  iSCSI = DevPath;
  CatPrint (
    Str,
    L"iSCSI(%a,0x%x,0x%lx,",
    iSCSI->iSCSITargetName,
    iSCSI->TargetPortalGroupTag,
    iSCSI->Lun
    );

  Options = iSCSI->LoginOption;
  CatPrint (Str, L"%s,", ((Options >> 1) & 0x0001) ? L"CRC32C" : L"None");
  CatPrint (Str, L"%s,", ((Options >> 3) & 0x0001) ? L"CRC32C" : L"None");
  if ((Options >> 11) & 0x0001) {
    CatPrint (Str, L"%s,", L"None");
  } else if ((Options >> 12) & 0x0001) {
    CatPrint (Str, L"%s,", L"CHAP_UNI");
  } else {
    CatPrint (Str, L"%s,", L"CHAP_BI");

  }

  CatPrint (Str, L"%s)", (iSCSI->NetworkProtocol == 0) ? L"TCP" : L"reserved");
}

STATIC
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

STATIC
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
  if (DisplayOnly == TRUE) {
    CatPrint (Str, L"CDROM(0x%x)", Cd->BootEntry);
    return ;
  }

  CatPrint (Str, L"CDROM(0x%x,0x%lx,0x%lx)", Cd->BootEntry, Cd->PartitionStart, Cd->PartitionSize);
}

STATIC
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

STATIC
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

STATIC
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

STATIC
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

STATIC
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

  if (DisplayOnly == TRUE) {
    CatPrint (Str, L")");
    return ;
  }

  CatPrint (Str, L",0x%x)", Bbs->StatusFlag);
}

STATIC
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

STATIC
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
  {MEDIA_DEVICE_PATH, MEDIA_HARDDRIVE_DP, DevPathToTextHardDrive},
  {MEDIA_DEVICE_PATH, MEDIA_CDROM_DP, DevPathToTextCDROM},
  {MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP, DevPathToTextVendor},
  {MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, DevPathToTextFilePath},
  {MEDIA_DEVICE_PATH, MEDIA_PROTOCOL_DP, DevPathToTextMediaProtocol},
  {MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, DevPathToTextFilePath},
  {MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_VOL_DP, DevPathToTextFv},
  {MEDIA_DEVICE_PATH, MEDIA_PIWG_FW_FILE_DP, DevPathToTextFvFile},
  {BBS_DEVICE_PATH, BBS_BBS_DP, DevPathToTextBBS},
  {END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE, DevPathToTextEndInstance},
  {0, 0, NULL}
};

CHAR16 *
ConvertDeviceNodeToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DeviceNode,
  IN BOOLEAN                         DisplayOnly,
  IN BOOLEAN                         AllowShortcuts
  )
/*++

  Routine Description:
    Convert a device node to its text representation.

  Arguments:
    DeviceNode       -   Points to the device node to be converted.
    DisplayOnly      -   If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
    AllowShortcuts   -   If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  Returns:
    A pointer        -   a pointer to the allocated text representation of the device node.
    NULL             -   if DeviceNode is NULL or there was insufficient memory.

--*/
{
  POOL_PRINT  Str;
  UINTN       Index;
  UINTN       NewSize;
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

  //
  // Shrink pool used for string allocation
  //
  NewSize = (Str.Len + 1) * sizeof (CHAR16);
  Str.Str = ReallocatePool (Str.Str, NewSize, NewSize);
  ASSERT (Str.Str != NULL);
  Str.Str[Str.Len] = 0;
  return Str.Str;
}

CHAR16 *
ConvertDevicePathToText (
  IN CONST EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN BOOLEAN                          DisplayOnly,
  IN BOOLEAN                          AllowShortcuts
  )
/*++

  Routine Description:
    Convert a device path to its text representation.

  Arguments:
    DeviceNode       -   Points to the device path to be converted.
    DisplayOnly      -   If DisplayOnly is TRUE, then the shorter text representation
                         of the display node is used, where applicable. If DisplayOnly
                         is FALSE, then the longer text representation of the display node
                         is used.
    AllowShortcuts   -   If AllowShortcuts is TRUE, then the shortcut forms of text
                         representation for a device node can be used, where applicable.

  Returns:
    A pointer        -   a pointer to the allocated text representation of the device path.
    NULL             -   if DeviceNode is NULL or there was insufficient memory.

--*/
{
  POOL_PRINT                Str;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  EFI_DEVICE_PATH_PROTOCOL  *UnpackDevPath;
  UINTN                     Index;
  UINTN                     NewSize;
  VOID                      (*DumpNode) (POOL_PRINT *, VOID *, BOOLEAN, BOOLEAN);

  if (DevicePath == NULL) {
    return NULL;
  }

  ZeroMem (&Str, sizeof (Str));

  //
  // Unpacked the device path
  //
  UnpackDevPath = UnpackDevicePath ((EFI_DEVICE_PATH_PROTOCOL *) DevicePath);
  ASSERT (UnpackDevPath != NULL);

  //
  // Process each device path node
  //
  DevPathNode = UnpackDevPath;
  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Find the handler to dump this device path node
    //
    DumpNode = NULL;
    for (Index = 0; DevPathToTextTable[Index].Function; Index += 1) {

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
    //  Put a path seperator in if needed
    //
    if (Str.Len && DumpNode != DevPathToTextEndInstance) {
      if (*(Str.Str + Str.Len / sizeof (CHAR16) - 1) != L',') {
        CatPrint (&Str, L"/");
      }
    }
    //
    // Print this node of the device path
    //
    DumpNode (&Str, DevPathNode, DisplayOnly, AllowShortcuts);

    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }
  //
  // Shrink pool used for string allocation
  //
  FreePool (UnpackDevPath);

  NewSize = (Str.Len + 1) * sizeof (CHAR16);
  Str.Str = ReallocatePool (Str.Str, NewSize, NewSize);
  ASSERT (Str.Str != NULL);
  Str.Str[Str.Len] = 0;
  return Str.Str;
}
