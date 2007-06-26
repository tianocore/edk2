/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DevicePath.c

Abstract:

  BDS internal function define the default device path string, it can be
  replaced by platform device path.

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

EFI_GUID  mEfiWinNtThunkProtocolGuid  = EFI_WIN_NT_THUNK_PROTOCOL_GUID;
EFI_GUID  mEfiWinNtUgaGuid            = EFI_WIN_NT_UGA_GUID;
EFI_GUID  mEfiWinNtGopGuid            = EFI_WIN_NT_GOP_GUID;
EFI_GUID  mEfiWinNtSerialPortGuid     = EFI_WIN_NT_SERIAL_PORT_GUID;
EFI_GUID  mEfiMsgPcAnsiGuid           = DEVICE_PATH_MESSAGING_PC_ANSI;
EFI_GUID  mEfiMsgVt100Guid            = DEVICE_PATH_MESSAGING_VT_100;
EFI_GUID  mEfiMsgVt100PlusGuid        = DEVICE_PATH_MESSAGING_VT_100_PLUS;
EFI_GUID  mEfiMsgVt100Utf8Guid        = DEVICE_PATH_MESSAGING_VT_UTF8;

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

CHAR16 *
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *fmt,
  ...
  )
/*++

Routine Description:

    Concatenates a formatted unicode string to allocated pool.
    The caller must free the resulting buffer.

Arguments:

    Str         - Tracks the allocated pool, size in use, and
                  amount of pool allocated.

    fmt         - The format string

Returns:

    Allocated buffer with the formatted string printed in it.
    The caller must free the allocated buffer.   The buffer
    allocation is not packed.

--*/
{
  UINT16  *AppendStr;
  VA_LIST args;
  UINTN   strsize;

  AppendStr = AllocateZeroPool (0x1000);
  if (AppendStr == NULL) {
    return Str->str;
  }

  VA_START (args, fmt);
  UnicodeVSPrint (AppendStr, 0x1000, fmt, args);
  VA_END (args);
  if (NULL == Str->str) {
    strsize   = StrSize (AppendStr);
    Str->str  = AllocateZeroPool (strsize);
    ASSERT (Str->str != NULL);
  } else {
    strsize = StrSize (AppendStr) + StrSize (Str->str) - sizeof (UINT16);
    Str->str = ReallocatePool (
                Str->str,
                StrSize (Str->str),
                strsize
                );
    ASSERT (Str->str != NULL);
  }

  Str->maxlen = MAX_CHAR * sizeof (UINT16);
  if (strsize < Str->maxlen) {
    StrCat (Str->str, AppendStr);
    Str->len = strsize - sizeof (UINT16);
  }

  FreePool (AppendStr);
  return Str->str;
}

EFI_DEVICE_PATH_PROTOCOL *
BdsLibUnpackDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:

  Function unpacks a device path data structure so that all the nodes
  of a device path are naturally aligned.

Arguments:

  DevPath        - A pointer to a device path data structure

Returns:

  If the memory for the device path is successfully allocated, then a
  pointer to the new device path is returned.  Otherwise, NULL is returned.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *Src;
  EFI_DEVICE_PATH_PROTOCOL  *Dest;
  EFI_DEVICE_PATH_PROTOCOL  *NewPath;
  UINTN                     Size;

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

    Src = NextDevicePathNode (Src);
  }
  //
  // Allocate space for the unpacked path
  //
  NewPath = AllocateZeroPool (Size);
  if (NewPath) {

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

      Src = NextDevicePathNode (Src);
    }
  }

  return NewPath;
}

VOID
DevPathPci (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  PCI_DEVICE_PATH *Pci;

  Pci = DevPath;
  CatPrint (Str, L"Pci(%x|%x)", Pci->Device, Pci->Function);
}

VOID
DevPathPccard (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  PCCARD_DEVICE_PATH  *Pccard;

  Pccard = DevPath;
  CatPrint (Str, L"Pcmcia(Function%x)", Pccard->FunctionNumber);
}

VOID
DevPathMemMap (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  MEMMAP_DEVICE_PATH  *MemMap;

  MemMap = DevPath;
  CatPrint (
    Str,
    L"MemMap(%d:%.lx-%.lx)",
    MemMap->MemoryType,
    MemMap->StartingAddress,
    MemMap->EndingAddress
    );
}

VOID
DevPathController (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  CONTROLLER_DEVICE_PATH  *Controller;

  Controller = DevPath;
  CatPrint (Str, L"Ctrl(%d)", Controller->ControllerNumber);
}

VOID
DevPathVendor (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
/*++

Routine Description:

  Convert Vendor device path to device name

Arguments:

  Str     - The buffer store device name
  DevPath - Pointer to vendor device path

Returns:

  When it return, the device name have been stored in *Str.

--*/
{
  VENDOR_DEVICE_PATH  *Vendor;
  CHAR16              *Type;
  INT32               *Temp;

  Vendor  = DevPath;
  Temp    = (INT32 *) (&Vendor->Guid);

  switch (DevicePathType (&Vendor->Header)) {
  case HARDWARE_DEVICE_PATH:
    //
    // If the device is a winntbus device, we will give it a readable device name.
    //
    if (CompareGuid (&Vendor->Guid, &mEfiWinNtThunkProtocolGuid)) {
      CatPrint (Str, L"%s", L"WinNtBus");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiWinNtUgaGuid)) {
      CatPrint (Str, L"%s", L"UGA");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiWinNtGopGuid)) {
      CatPrint (Str, L"%s", L"GOP");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiWinNtSerialPortGuid)) {
      CatPrint (Str, L"%s", L"Serial");
      return ;
    } else {
      Type = L"Hw";
      break;
    }

  case MESSAGING_DEVICE_PATH:
    //
    // If the device is a winntbus device, we will give it a readable device name.
    //
    if (CompareGuid (&Vendor->Guid, &mEfiMsgPcAnsiGuid)) {
      CatPrint (Str, L"%s", L"PC-ANSI");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiMsgVt100Guid)) {
      CatPrint (Str, L"%s", L"VT100");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiMsgVt100PlusGuid)) {
      CatPrint (Str, L"%s", L"VT100+");
      return ;
    } else if (CompareGuid (&Vendor->Guid, &mEfiMsgVt100Utf8Guid)) {
      CatPrint (Str, L"%s", L"VT100-UTF8");
      return ;
    } else {
      Type = L"Msg";
      break;
    }

  case MEDIA_DEVICE_PATH:
    Type = L"Media";
    break;

  default:
    Type = L"?";
    break;
  }

  CatPrint (Str, L"Ven%s(%g)", Type, &Vendor->Guid);
}

VOID
DevPathAcpi (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  ACPI_HID_DEVICE_PATH  *Acpi;

  Acpi = DevPath;
  if ((Acpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
    CatPrint (Str, L"Acpi(PNP%04x,%x)", EISA_ID_TO_NUM (Acpi->HID), Acpi->UID);
  } else {
    CatPrint (Str, L"Acpi(%08x,%x)", Acpi->HID, Acpi->UID);
  }
}

VOID
DevPathExtendedAcpi (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  ACPI_EXTENDED_HID_DEVICE_PATH   *ExtendedAcpi;
  //
  // Index for HID, UID and CID strings, 0 for non-exist
  //
  UINT16                          HIDSTRIdx;
  UINT16                          UIDSTRIdx;
  UINT16                          CIDSTRIdx;
  UINT16                          Index;
  UINT16                          Length;
  UINT16                          Anchor;
  CHAR8                           *AsChar8Array;

  ASSERT (Str != NULL);
  ASSERT (DevPath != NULL);

  HIDSTRIdx    = 0;
  UIDSTRIdx    = 0;
  CIDSTRIdx    = 0;
  ExtendedAcpi = DevPath;
  Length       = DevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) ExtendedAcpi);

  ASSERT (Length >= 19);
  AsChar8Array = (CHAR8 *) ExtendedAcpi;

  //
  // find HIDSTR
  //
  Anchor = 16;
  for (Index = Anchor; Index < Length && AsChar8Array[Index]; Index++) {
    ;
  }
  if (Index > Anchor) {
    HIDSTRIdx = Anchor;
  }
  //
  // find UIDSTR
  //
  Anchor = Index + 1;
  for (Index = Anchor; Index < Length && AsChar8Array[Index]; Index++) {
    ;
  }
  if (Index > Anchor) {
    UIDSTRIdx = Anchor;
  }
  //
  // find CIDSTR
  //
  Anchor = Index + 1;
  for (Index = Anchor; Index < Length && AsChar8Array[Index]; Index++) {
    ;
  }
  if (Index > Anchor) {
    CIDSTRIdx = Anchor;
  }

  if (HIDSTRIdx == 0 && CIDSTRIdx == 0 && ExtendedAcpi->UID == 0) {
    CatPrint (Str, L"AcpiExp(");
    if ((ExtendedAcpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
      CatPrint (Str, L"PNP%04x,", EISA_ID_TO_NUM (ExtendedAcpi->HID));
    } else {
      CatPrint (Str, L"%08x,", ExtendedAcpi->HID);
    }
    if ((ExtendedAcpi->CID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
      CatPrint (Str, L"PNP%04x,", EISA_ID_TO_NUM (ExtendedAcpi->CID));
    } else {
      CatPrint (Str, L"%08x,", ExtendedAcpi->CID);
    }
    if (UIDSTRIdx != 0) {
      CatPrint (Str, L"%a)", AsChar8Array + UIDSTRIdx);
    } else {
      CatPrint (Str, L"\"\")");
    }
  } else {
    CatPrint (Str, L"AcpiEx(");
    if ((ExtendedAcpi->HID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
      CatPrint (Str, L"PNP%04x,", EISA_ID_TO_NUM (ExtendedAcpi->HID));
    } else {
      CatPrint (Str, L"%08x,", ExtendedAcpi->HID);
    }
    if ((ExtendedAcpi->CID & PNP_EISA_ID_MASK) == PNP_EISA_ID_CONST) {
      CatPrint (Str, L"PNP%04x,", EISA_ID_TO_NUM (ExtendedAcpi->CID));
    } else {
      CatPrint (Str, L"%08x,", ExtendedAcpi->CID);
    }
    CatPrint (Str, L"%x,", ExtendedAcpi->UID);

    if (HIDSTRIdx != 0) {
      CatPrint (Str, L"%a,", AsChar8Array + HIDSTRIdx);
    } else {
      CatPrint (Str, L"\"\",");
    }
    if (CIDSTRIdx != 0) {
      CatPrint (Str, L"%a,", AsChar8Array + CIDSTRIdx);
    } else {
      CatPrint (Str, L"\"\",");
    }
    if (UIDSTRIdx != 0) {
      CatPrint (Str, L"%a)", AsChar8Array + UIDSTRIdx);
    } else {
      CatPrint (Str, L"\"\")");
    }
  }

}

VOID
DevPathAdrAcpi (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  ACPI_ADR_DEVICE_PATH    *AcpiAdr;
  UINT16                  Index;
  UINT16                  Length;
  UINT16                  AdditionalAdrCount;

  AcpiAdr            = DevPath;
  Length             = DevicePathNodeLength ((EFI_DEVICE_PATH_PROTOCOL *) AcpiAdr);
  AdditionalAdrCount = (Length - 8) / 4;

  CatPrint (Str, L"AcpiAdr(%x", AcpiAdr->ADR);
  for (Index = 0; Index < AdditionalAdrCount; Index++) {
    CatPrint (Str, L",%x", *(UINT32 *) ((UINT8 *) AcpiAdr + 8 + Index * 4));
  }
  CatPrint (Str, L")");
}

VOID
DevPathAtapi (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  ATAPI_DEVICE_PATH *Atapi;

  Atapi = DevPath;
  CatPrint (
    Str,
    L"Ata(%s,%s)",
    Atapi->PrimarySecondary ? L"Secondary" : L"Primary",
    Atapi->SlaveMaster ? L"Slave" : L"Master"
    );
}

VOID
DevPathScsi (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  SCSI_DEVICE_PATH  *Scsi;

  Scsi = DevPath;
  CatPrint (Str, L"Scsi(Pun%x,Lun%x)", Scsi->Pun, Scsi->Lun);
}

VOID
DevPathFibre (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  FIBRECHANNEL_DEVICE_PATH  *Fibre;

  Fibre = DevPath;
  CatPrint (Str, L"Fibre(Wwn%lx,Lun%x)", Fibre->WWN, Fibre->Lun);
}

VOID
DevPath1394 (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  F1394_DEVICE_PATH *F1394;

  F1394 = DevPath;
  CatPrint (Str, L"1394(%g)", &F1394->Guid);
}

VOID
DevPathUsb (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  USB_DEVICE_PATH *Usb;

  Usb = DevPath;
  CatPrint (Str, L"Usb(%x, %x)", Usb->ParentPortNumber, Usb->InterfaceNumber);
}

VOID
DevPathUsbClass (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  USB_CLASS_DEVICE_PATH *UsbClass;

  UsbClass = DevPath;
  CatPrint (
    Str,
    L"Usb Class(%x, %x, %x, %x, %x)",
    UsbClass->VendorId,
    UsbClass->ProductId,
    UsbClass->DeviceClass,
    UsbClass->DeviceSubClass,
    UsbClass->DeviceProtocol
    );
}

VOID
DevPathI2O (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  I2O_DEVICE_PATH *I2O;

  I2O = DevPath;
  CatPrint (Str, L"I2O(%x)", I2O->Tid);
}

VOID
DevPathMacAddr (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
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

  CatPrint (Str, L"Mac(");

  for (Index = 0; Index < HwAddressSize; Index++) {
    CatPrint (Str, L"%02x", MAC->MacAddress.Addr[Index]);
  }

  CatPrint (Str, L")");
}

VOID
DevPathIPv4 (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  IPv4_DEVICE_PATH  *IP;

  IP = DevPath;
  CatPrint (
    Str,
    L"IPv4(%d.%d.%d.%d:%d)",
    IP->RemoteIpAddress.Addr[0],
    IP->RemoteIpAddress.Addr[1],
    IP->RemoteIpAddress.Addr[2],
    IP->RemoteIpAddress.Addr[3],
    IP->RemotePort
    );
}

VOID
DevPathIPv6 (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  IPv6_DEVICE_PATH  *IP;

  IP = DevPath;
  CatPrint (Str, L"IP-v6(not-done)");
}

VOID
DevPathInfiniBand (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  INFINIBAND_DEVICE_PATH  *InfiniBand;

  InfiniBand = DevPath;
  CatPrint (Str, L"InfiniBand(not-done)");
}

VOID
DevPathUart (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
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
    CatPrint (Str, L"Uart(DEFAULT %c", Parity);
  } else {
    CatPrint (Str, L"Uart(%d %c", Uart->BaudRate, Parity);
  }

  if (Uart->DataBits == 0) {
    CatPrint (Str, L"D");
  } else {
    CatPrint (Str, L"%d", Uart->DataBits);
  }

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

VOID
DevPathHardDrive (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  HARDDRIVE_DEVICE_PATH *Hd;

  Hd = DevPath;
  switch (Hd->SignatureType) {
  case SIGNATURE_TYPE_MBR:
    CatPrint (
      Str,
      L"HD(Part%d,Sig%08x)",
      Hd->PartitionNumber,
      *((UINT32 *) (&(Hd->Signature[0])))
      );
    break;

  case SIGNATURE_TYPE_GUID:
    CatPrint (
      Str,
      L"HD(Part%d,Sig%g)",
      Hd->PartitionNumber,
      (EFI_GUID *) &(Hd->Signature[0])
      );
    break;

  default:
    CatPrint (
      Str,
      L"HD(Part%d,MBRType=%02x,SigType=%02x)",
      Hd->PartitionNumber,
      Hd->MBRType,
      Hd->SignatureType
      );
    break;
  }
}

VOID
DevPathCDROM (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  CDROM_DEVICE_PATH *Cd;

  Cd = DevPath;
  CatPrint (Str, L"CDROM(Entry%x)", Cd->BootEntry);
}

VOID
DevPathFilePath (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  FILEPATH_DEVICE_PATH  *Fp;

  Fp = DevPath;
  CatPrint (Str, L"%s", Fp->PathName);
}

VOID
DevPathMediaProtocol (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  MEDIA_PROTOCOL_DEVICE_PATH  *MediaProt;

  MediaProt = DevPath;
  CatPrint (Str, L"%g", &MediaProt->Protocol);
}

VOID
DevPathFvFilePath (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *FvFilePath;

  FvFilePath = DevPath;
  CatPrint (Str, L"%g", &FvFilePath->NameGuid);
}

VOID
DevPathBssBss (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
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
    Type = L"Harddrive";
    break;

  case BBS_TYPE_CDROM:
    Type = L"CDROM";
    break;

  case BBS_TYPE_PCMCIA:
    Type = L"PCMCIA";
    break;

  case BBS_TYPE_USB:
    Type = L"Usb";
    break;

  case BBS_TYPE_EMBEDDED_NETWORK:
    Type = L"Net";
    break;

  default:
    Type = L"?";
    break;
  }
  //
  // Since current Print function hasn't implemented %a (for ansi string)
  // we will only print Unicode strings.
  //
  CatPrint (Str, L"Legacy-%s", Type);
}

VOID
DevPathEndInstance (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  CatPrint (Str, L",");
}

VOID
DevPathNodeUnknown (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
{
  CatPrint (Str, L"?");
}

DEVICE_PATH_STRING_TABLE  DevPathTable[] = {
  HARDWARE_DEVICE_PATH,
  HW_PCI_DP,
  DevPathPci,
  HARDWARE_DEVICE_PATH,
  HW_PCCARD_DP,
  DevPathPccard,
  HARDWARE_DEVICE_PATH,
  HW_MEMMAP_DP,
  DevPathMemMap,
  HARDWARE_DEVICE_PATH,
  HW_VENDOR_DP,
  DevPathVendor,
  HARDWARE_DEVICE_PATH,
  HW_CONTROLLER_DP,
  DevPathController,
  ACPI_DEVICE_PATH,
  ACPI_DP,
  DevPathAcpi,
  ACPI_DEVICE_PATH,
  ACPI_EXTENDED_DP,
  DevPathExtendedAcpi,
  ACPI_DEVICE_PATH,
  ACPI_ADR_DP,
  DevPathAdrAcpi,
  MESSAGING_DEVICE_PATH,
  MSG_ATAPI_DP,
  DevPathAtapi,
  MESSAGING_DEVICE_PATH,
  MSG_SCSI_DP,
  DevPathScsi,
  MESSAGING_DEVICE_PATH,
  MSG_FIBRECHANNEL_DP,
  DevPathFibre,
  MESSAGING_DEVICE_PATH,
  MSG_1394_DP,
  DevPath1394,
  MESSAGING_DEVICE_PATH,
  MSG_USB_DP,
  DevPathUsb,
  MESSAGING_DEVICE_PATH,
  MSG_USB_CLASS_DP,
  DevPathUsbClass,
  MESSAGING_DEVICE_PATH,
  MSG_I2O_DP,
  DevPathI2O,
  MESSAGING_DEVICE_PATH,
  MSG_MAC_ADDR_DP,
  DevPathMacAddr,
  MESSAGING_DEVICE_PATH,
  MSG_IPv4_DP,
  DevPathIPv4,
  MESSAGING_DEVICE_PATH,
  MSG_IPv6_DP,
  DevPathIPv6,
  MESSAGING_DEVICE_PATH,
  MSG_INFINIBAND_DP,
  DevPathInfiniBand,
  MESSAGING_DEVICE_PATH,
  MSG_UART_DP,
  DevPathUart,
  MESSAGING_DEVICE_PATH,
  MSG_VENDOR_DP,
  DevPathVendor,
  MEDIA_DEVICE_PATH,
  MEDIA_HARDDRIVE_DP,
  DevPathHardDrive,
  MEDIA_DEVICE_PATH,
  MEDIA_CDROM_DP,
  DevPathCDROM,
  MEDIA_DEVICE_PATH,
  MEDIA_VENDOR_DP,
  DevPathVendor,
  MEDIA_DEVICE_PATH,
  MEDIA_FILEPATH_DP,
  DevPathFilePath,
  MEDIA_DEVICE_PATH,
  MEDIA_PROTOCOL_DP,
  DevPathMediaProtocol,
  BBS_DEVICE_PATH,
  BBS_BBS_DP,
  DevPathBssBss,
  END_DEVICE_PATH_TYPE,
  END_INSTANCE_DEVICE_PATH_SUBTYPE,
  DevPathEndInstance,
  0,
  0,
  NULL
};

CHAR16 *
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
/*++

  Turns the Device Path into a printable string.  Allcoates
  the string from pool.  The caller must SafeFreePool the returned
  string.

--*/
{
  POOL_PRINT                Str;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  VOID (*DumpNode) (POOL_PRINT *, VOID *);

  UINTN Index;
  UINTN NewSize;

  ZeroMem (&Str, sizeof (Str));

  if (DevPath == NULL) {
    goto Done;
  }
  //
  // Unpacked the device path
  //
  DevPath = BdsLibUnpackDevicePath (DevPath);
  ASSERT (DevPath);

  //
  // Process each device path node
  //
  DevPathNode = DevPath;
  while (!IsDevicePathEnd (DevPathNode)) {
    //
    // Find the handler to dump this device path node
    //
    DumpNode = NULL;
    for (Index = 0; DevPathTable[Index].Function; Index += 1) {

      if (DevicePathType (DevPathNode) == DevPathTable[Index].Type &&
          DevicePathSubType (DevPathNode) == DevPathTable[Index].SubType
          ) {
        DumpNode = DevPathTable[Index].Function;
        break;
      }
    }
    //
    // If not found, use a generic function
    //
    if (!DumpNode) {
      DumpNode = DevPathNodeUnknown;
    }
    //
    //  Put a path seperator in if needed
    //
    if (Str.len && DumpNode != DevPathEndInstance) {
      CatPrint (&Str, L"/");
    }
    //
    // Print this node of the device path
    //
    DumpNode (&Str, DevPathNode);

    //
    // Next device path node
    //
    DevPathNode = NextDevicePathNode (DevPathNode);
  }
  //
  // Shrink pool used for string allocation
  //
  FreePool (DevPath);

Done:
  NewSize = (Str.len + 1) * sizeof (CHAR16);
  Str.str = ReallocatePool (Str.str, NewSize, NewSize);
  ASSERT (Str.str != NULL);
  Str.str[Str.len] = 0;
  return Str.str;
}

EFI_DEVICE_PATH_PROTOCOL *
LibDuplicateDevicePathInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:

  Function creates a device path data structure that identically matches the
  device path passed in.

Arguments:

  DevPath      - A pointer to a device path data structure.

Returns:

  The new copy of DevPath is created to identically match the input.
  Otherwise, NULL is returned.

--*/
{
  EFI_DEVICE_PATH_PROTOCOL  *NewDevPath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathInst;
  EFI_DEVICE_PATH_PROTOCOL  *Temp;
  UINTN                     Size;

  //
  // get the size of an instance from the input
  //
  Temp            = DevPath;
  DevicePathInst  = GetNextDevicePathInstance (&Temp, &Size);

  //
  // Make a copy
  //
  NewDevPath = NULL;
  if (Size) {
    NewDevPath = AllocateZeroPool (Size);
    ASSERT (NewDevPath != NULL);
  }

  if (NewDevPath) {
    CopyMem (NewDevPath, DevicePathInst, Size);
  }

  return NewDevPath;
}
