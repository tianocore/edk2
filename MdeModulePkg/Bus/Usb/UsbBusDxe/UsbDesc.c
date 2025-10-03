/** @file

    Manage Usb Descriptor List

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbBus.h"

#define INTERFACE_NOT_SET  0xFF

/**
  Free the interface setting descriptor.

  @param  Setting               The descriptor to free.

**/
VOID
UsbFreeInterfaceDesc (
  IN USB_INTERFACE_SETTING  *Setting
  )
{
  USB_ENDPOINT_DESC  *Ep;
  UINTN              Index;

  if (Setting->Endpoints != NULL) {
    //
    // Each interface setting may have several endpoints, free them first.
    //
    for (Index = 0; Index < Setting->Desc.NumEndpoints; Index++) {
      Ep = Setting->Endpoints[Index];

      if (Ep != NULL) {
        FreePool (Ep);
      }
    }

    //
    // Only call FreePool() if NumEndpoints > 0.
    //
    if (Setting->Desc.NumEndpoints > 0) {
      FreePool (Setting->Endpoints);
    }
  }

  FreePool (Setting);
}

/**
  Free a configuration descriptor with its interface
  descriptors. It may be initialized partially.

  @param  Config                The configuration descriptor to free.

**/
VOID
UsbFreeConfigDesc (
  IN USB_CONFIG_DESC  *Config
  )
{
  USB_INTERFACE_DESC  *Interface;
  UINTN               Index;
  UINTN               SetIndex;

  if (Config->Interfaces != NULL) {
    //
    // A configuration may have several interfaces, free the interface
    //
    for (Index = 0; Index < Config->Desc.NumInterfaces; Index++) {
      Interface = Config->Interfaces[Index];

      if (Interface == NULL) {
        continue;
      }

      //
      // Each interface may have several settings, free the settings
      //
      for (SetIndex = 0; SetIndex < Interface->NumOfSetting; SetIndex++) {
        if (Interface->Settings[SetIndex] != NULL) {
          UsbFreeInterfaceDesc (Interface->Settings[SetIndex]);
        }
      }

      FreePool (Interface);
    }

    FreePool (Config->Interfaces);
  }

  FreePool (Config);
}

/**
  Free a device descriptor with its configurations.

  @param  DevDesc               The device descriptor.

**/
VOID
UsbFreeDevDesc (
  IN USB_DEVICE_DESC  *DevDesc
  )
{
  UINTN  Index;

  if (DevDesc->Configs != NULL) {
    for (Index = 0; Index < DevDesc->Desc.NumConfigurations; Index++) {
      if (DevDesc->Configs[Index] != NULL) {
        UsbFreeConfigDesc (DevDesc->Configs[Index]);
      }
    }

    FreePool (DevDesc->Configs);
  }

  if (DevDesc->BOSDesc != NULL) {
    FreePool (DevDesc->BOSDesc);
  }

  if (DevDesc->StrDescManufacturerUS != NULL) {
    FreePool (DevDesc->StrDescManufacturerUS);
  }

  if (DevDesc->StrDescProductUS != NULL) {
    FreePool (DevDesc->StrDescProductUS);
  }

  if (DevDesc->StrDescSerialNumberUS != NULL) {
    FreePool (DevDesc->StrDescSerialNumberUS);
  }

  FreePool (DevDesc);
}

/**
  Create a descriptor.

  @param  DescBuf               The buffer of raw descriptor.
  @param  Len                   The length of the raw descriptor buffer.
  @param  Type                  The type of descriptor to create.
  @param  Consumed              Number of bytes consumed.

  @return Created descriptor or NULL.

**/
VOID *
UsbCreateDesc (
  IN  UINT8  *DescBuf,
  IN  UINTN  Len,
  IN  UINT8  Type,
  OUT UINTN  *Consumed
  )
{
  USB_DESC_HEAD  *Head;
  UINTN          DescLen;
  UINTN          CtrlLen;
  UINTN          Offset;
  VOID           *Desc;

  DescLen   = 0;
  CtrlLen   = 0;
  *Consumed = 0;

  switch (Type) {
    case USB_DESC_TYPE_DEVICE:
      DescLen = sizeof (EFI_USB_DEVICE_DESCRIPTOR);
      CtrlLen = sizeof (USB_DEVICE_DESC);
      break;

    case USB_DESC_TYPE_CONFIG:
      DescLen = sizeof (EFI_USB_CONFIG_DESCRIPTOR);
      CtrlLen = sizeof (USB_CONFIG_DESC);
      break;

    case USB_DESC_TYPE_INTERFACE:
      DescLen = sizeof (EFI_USB_INTERFACE_DESCRIPTOR);
      CtrlLen = sizeof (USB_INTERFACE_SETTING);
      break;

    case USB_DESC_TYPE_ENDPOINT:
      DescLen = sizeof (EFI_USB_ENDPOINT_DESCRIPTOR);
      CtrlLen = sizeof (USB_ENDPOINT_DESC);
      break;

    default:
      ASSERT (FALSE);
      return NULL;
  }

  //
  // Total length is too small that cannot hold the single descriptor header plus data.
  //
  if (Len <= sizeof (USB_DESC_HEAD)) {
    DEBUG ((DEBUG_ERROR, "UsbCreateDesc: met mal-format descriptor, total length = %d!\n", Len));
    return NULL;
  }

  //
  // All the descriptor has a common LTV (Length, Type, Value)
  // format. Skip the descriptor that isn't of this Type
  //
  Offset = 0;
  Head   = (USB_DESC_HEAD *)DescBuf;
  while (Offset < Len - sizeof (USB_DESC_HEAD)) {
    //
    // Above condition make sure Head->Len and Head->Type are safe to access
    //
    Head = (USB_DESC_HEAD *)&DescBuf[Offset];

    if (Head->Len == 0) {
      DEBUG ((DEBUG_ERROR, "UsbCreateDesc: met mal-format descriptor, Head->Len = 0!\n"));
      return NULL;
    }

    //
    // Make sure no overflow when adding Head->Len to Offset.
    //
    if (Head->Len > MAX_UINTN - Offset) {
      DEBUG ((DEBUG_ERROR, "UsbCreateDesc: met mal-format descriptor, Head->Len = %d!\n", Head->Len));
      return NULL;
    }

    Offset += Head->Len;

    if (Head->Type == Type) {
      break;
    }
  }

  //
  // Head->Len is invalid resulting data beyond boundary, or
  // Descriptor cannot be found: No such type.
  //
  if (Len < Offset) {
    DEBUG ((DEBUG_ERROR, "UsbCreateDesc: met mal-format descriptor, Offset/Len = %d/%d!\n", Offset, Len));
    return NULL;
  }

  if ((Head->Type != Type) || (Head->Len < DescLen)) {
    DEBUG ((DEBUG_ERROR, "UsbCreateDesc: descriptor cannot be found, Header(T/L) = %d/%d!\n", Head->Type, Head->Len));
    return NULL;
  }

  Desc = AllocateZeroPool ((UINTN)CtrlLen);
  if (Desc == NULL) {
    return NULL;
  }

  CopyMem (Desc, Head, (UINTN)DescLen);

  *Consumed = Offset;

  return Desc;
}

/**
  Parse an interface descriptor and its endpoints.

  @param  DescBuf               The buffer of raw descriptor.
  @param  Len                   The length of the raw descriptor buffer.
  @param  Consumed              The number of raw descriptor consumed.

  @return The create interface setting or NULL if failed.

**/
USB_INTERFACE_SETTING *
UsbParseInterfaceDesc (
  IN  UINT8  *DescBuf,
  IN  UINTN  Len,
  OUT UINTN  *Consumed
  )
{
  USB_INTERFACE_SETTING  *Setting;
  USB_ENDPOINT_DESC      *Ep;
  UINTN                  Index;
  UINTN                  NumEp;
  UINTN                  Used;
  UINTN                  Offset;

  *Consumed = 0;
  Setting   = UsbCreateDesc (DescBuf, Len, USB_DESC_TYPE_INTERFACE, &Used);

  if (Setting == NULL) {
    DEBUG ((DEBUG_ERROR, "UsbParseInterfaceDesc: failed to create interface descriptor\n"));
    return NULL;
  }

  Offset = Used;

  //
  // Create an array to hold the interface's endpoints
  //
  NumEp = Setting->Desc.NumEndpoints;

  DEBUG ((
    DEBUG_INFO,
    "UsbParseInterfaceDesc: interface %d(setting %d) has %d endpoints\n",
    Setting->Desc.InterfaceNumber,
    Setting->Desc.AlternateSetting,
    (UINT32)NumEp
    ));

  if (NumEp == 0) {
    goto ON_EXIT;
  }

  Setting->Endpoints = AllocateZeroPool (sizeof (USB_ENDPOINT_DESC *) * NumEp);

  if (Setting->Endpoints == NULL) {
    goto ON_ERROR;
  }

  //
  // Create the endpoints for this interface
  //
  for (Index = 0; (Index < NumEp) && (Offset < Len); Index++) {
    Ep = UsbCreateDesc (DescBuf + Offset, Len - Offset, USB_DESC_TYPE_ENDPOINT, &Used);

    if (Ep == NULL) {
      DEBUG ((DEBUG_ERROR, "UsbParseInterfaceDesc: failed to create endpoint(index %d)\n", (UINT32)Index));
      goto ON_ERROR;
    }

    Setting->Endpoints[Index] = Ep;
    Offset                   += Used;
  }

ON_EXIT:
  *Consumed = Offset;
  return Setting;

ON_ERROR:
  UsbFreeInterfaceDesc (Setting);
  return NULL;
}

/**
  Parse the BOS descriptor and check if it is a SS device.

  @param  DescBuf               The buffer of raw descriptor.
  @param  Len                   The length of the raw descriptor buffer.

  @return TRUE                  The device is a SS device
          FALSE                 The device is not a SS device

**/
BOOLEAN
UsbIsSSDevice (
  IN UINT8  *DescBuf,
  IN UINTN  Len
  )
{
  UINT8             *NextCap;
  UINT8             Index;
  UINT8             NumCapDesc;
  USB_BOS_DESC      *BOSDesc;
  USB_DEV_CAP_DESC  *DevCapDesc;

  BOSDesc = (USB_BOS_DESC *)DescBuf;
  if ((BOSDesc->DescriptorType != USB_DESC_TYPE_BOS) || (Len == 0)) {
    return FALSE;
  }

  NumCapDesc = BOSDesc->NumDeviceCaps;
  NextCap    = DescBuf + BOSDesc->Length;

  for (Index = 0; Index < NumCapDesc; Index++, NextCap += DevCapDesc->Length) {
    DevCapDesc = (USB_DEV_CAP_DESC  *)NextCap;
    if ((DevCapDesc->DescriptorType == USB_BOS_DESC_TYPE_CAP) && (DevCapDesc->DevCapabilityType == USB_BOS_CAP_SS_USB)) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Parse the configuration descriptor and its interfaces.

  @param  DescBuf               The buffer of raw descriptor.
  @param  Len                   The length of the raw descriptor buffer.

  @return The created configuration descriptor.

**/
USB_CONFIG_DESC *
UsbParseConfigDesc (
  IN UINT8  *DescBuf,
  IN UINTN  Len
  )
{
  USB_CONFIG_DESC        *Config;
  USB_INTERFACE_SETTING  *Setting;
  USB_INTERFACE_DESC     *Interface;
  UINTN                  Index;
  UINTN                  NumIf;
  UINTN                  Consumed;
  UINT8                  FirstIntfNum = INTERFACE_NOT_SET;

  ASSERT (DescBuf != NULL);

  Config = UsbCreateDesc (DescBuf, Len, USB_DESC_TYPE_CONFIG, &Consumed);

  if (Config == NULL) {
    return NULL;
  }

  //
  // Initialize an array of setting for the configuration's interfaces.
  //
  NumIf              = Config->Desc.NumInterfaces;
  Config->Interfaces = AllocateZeroPool (sizeof (USB_INTERFACE_DESC *) * NumIf);

  if (Config->Interfaces == NULL) {
    goto ON_ERROR;
  }

  DEBUG ((
    DEBUG_INFO,
    "UsbParseConfigDesc: config %d has %d interfaces\n",
    Config->Desc.ConfigurationValue,
    (UINT32)NumIf
    ));

  for (Index = 0; Index < NumIf; Index++) {
    Interface = AllocateZeroPool (sizeof (USB_INTERFACE_DESC));

    if (Interface == NULL) {
      goto ON_ERROR;
    }

    Config->Interfaces[Index] = Interface;
  }

  //
  // If a configuration has several interfaces, these interfaces are
  // numbered from zero to n. If a interface has several settings,
  // these settings are also number from zero to m. The interface
  // setting must be organized as |interface 0, setting 0|interface 0
  // setting 1|interface 1, setting 0|interface 2, setting 0|. Check
  // USB2.0 spec, page 267.
  //
  DescBuf += Consumed;
  Len     -= Consumed;

  //
  // Make allowances for devices that return extra data at the
  // end of their config descriptors
  //
  while (Len >= sizeof (EFI_USB_INTERFACE_DESCRIPTOR)) {
    Setting = UsbParseInterfaceDesc (DescBuf, Len, &Consumed);

    // Usb standard spec expects the interface number to start from 0.
    // Some devices might have the first interface number not equal to zero.
    // This is a work around for these devices. For example, Dell WWAN DW2811e
    if (Setting != NULL) {
      if (FirstIntfNum == INTERFACE_NOT_SET) {
        FirstIntfNum = Setting->Desc.InterfaceNumber;
      }
    }

    if (Setting == NULL) {
      DEBUG ((DEBUG_ERROR, "UsbParseConfigDesc: warning: failed to get interface setting, stop parsing now.\n"));
      break;
    } else if (Setting->Desc.InterfaceNumber >= (NumIf + FirstIntfNum)) {
      DEBUG ((DEBUG_ERROR, "UsbParseConfigDesc: malformatted interface descriptor\n"));

      UsbFreeInterfaceDesc (Setting);
      goto ON_ERROR;
    }

    //
    // Insert the descriptor to the corresponding set.
    //
    Interface = Config->Interfaces[(Setting->Desc.InterfaceNumber - FirstIntfNum)];

    if (Interface->NumOfSetting >= USB_MAX_INTERFACE_SETTING) {
      goto ON_ERROR;
    }

    Interface->Settings[Interface->NumOfSetting] = Setting;
    Interface->NumOfSetting++;

    DescBuf += Consumed;
    Len     -= Consumed;
  }

  return Config;

ON_ERROR:
  UsbFreeConfigDesc (Config);
  return NULL;
}

/**
  USB standard control transfer support routine. This
  function is used by USB device. It is possible that
  the device's interfaces are still waiting to be
  enumerated.

  @param  UsbDev                The usb device.
  @param  Direction             The direction of data transfer.
  @param  Type                  Standard / class specific / vendor specific.
  @param  Target                The receiving target.
  @param  Request               Which request.
  @param  Value                 The wValue parameter of the request.
  @param  Index                 The wIndex parameter of the request.
  @param  Buf                   The buffer to receive data into / transmit from.
  @param  Length                The length of the buffer.

  @retval EFI_SUCCESS           The control request is executed.
  @retval EFI_DEVICE_ERROR      Failed to execute the control transfer.

**/
EFI_STATUS
UsbCtrlRequest (
  IN USB_DEVICE              *UsbDev,
  IN EFI_USB_DATA_DIRECTION  Direction,
  IN UINTN                   Type,
  IN UINTN                   Target,
  IN UINTN                   Request,
  IN UINT16                  Value,
  IN UINT16                  Index,
  IN OUT VOID                *Buf,
  IN UINTN                   Length
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  EFI_STATUS              Status;
  UINT32                  Result;
  UINTN                   Len;

  ASSERT ((UsbDev != NULL) && (UsbDev->Bus != NULL));

  DevReq.RequestType = USB_REQUEST_TYPE (Direction, Type, Target);
  DevReq.Request     = (UINT8)Request;
  DevReq.Value       = Value;
  DevReq.Index       = Index;
  DevReq.Length      = (UINT16)Length;

  Len    = Length;
  Status = UsbHcControlTransfer (
             UsbDev->Bus,
             UsbDev->Address,
             UsbDev->Speed,
             UsbDev->MaxPacket0,
             &DevReq,
             Direction,
             Buf,
             &Len,
             USB_GENERAL_DEVICE_REQUEST_TIMEOUT,
             &UsbDev->Translator,
             &Result
             );

  return Status;
}

/**
  Get the standard descriptors.

  @param  UsbDev                The USB device to read descriptor from.
  @param  DescType              The type of descriptor to read.
  @param  DescIndex             The index of descriptor to read.
  @param  LangId                Language ID, only used to get string, otherwise set
                                it to 0.
  @param  Buf                   The buffer to hold the descriptor read.
  @param  Length                The length of the buffer.

  @retval EFI_SUCCESS           The descriptor is read OK.
  @retval Others                Failed to retrieve the descriptor.

**/
EFI_STATUS
UsbCtrlGetDesc (
  IN  USB_DEVICE  *UsbDev,
  IN  UINTN       DescType,
  IN  UINTN       DescIndex,
  IN  UINT16      LangId,
  OUT VOID        *Buf,
  IN  UINTN       Length
  )
{
  EFI_STATUS  Status;

  Status = UsbCtrlRequest (
             UsbDev,
             EfiUsbDataIn,
             USB_REQ_TYPE_STANDARD,
             USB_TARGET_DEVICE,
             USB_REQ_GET_DESCRIPTOR,
             (UINT16)((DescType << 8) | DescIndex),
             LangId,
             Buf,
             Length
             );

  return Status;
}

/**
  Return the max packet size for endpoint zero. This function
  is the first function called to get descriptors during bus
  enumeration.

  @param  UsbDev                The usb device.

  @retval EFI_SUCCESS           The max packet size of endpoint zero is retrieved.
  @retval EFI_DEVICE_ERROR      Failed to retrieve it.

**/
EFI_STATUS
UsbGetMaxPacketSize0 (
  IN USB_DEVICE  *UsbDev
  )
{
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;
  EFI_STATUS                 Status;
  UINTN                      Index;

  //
  // Get the first 8 bytes of the device descriptor which contains
  // max packet size for endpoint 0, which is at least 8.
  //
  for (Index = 0; Index < 3; Index++) {
    Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_DEVICE, 0, 0, &DevDesc, 8);

    if (!EFI_ERROR (Status)) {
      if ((DevDesc.BcdUSB >= 0x0300) && (DevDesc.MaxPacketSize0 == 9)) {
        UsbDev->MaxPacket0 = 1 << 9;
        return EFI_SUCCESS;
      }

      UsbDev->MaxPacket0 = DevDesc.MaxPacketSize0;
      return EFI_SUCCESS;
    }

    gBS->Stall (USB_RETRY_MAX_PACK_SIZE_STALL);
  }

  return EFI_DEVICE_ERROR;
}

/**
  Get the device descriptor for the device.

  @param  UsbDev                The Usb device to retrieve descriptor from.

  @retval EFI_SUCCESS           The device descriptor is returned.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.

**/
EFI_STATUS
UsbGetDevDesc (
  IN USB_DEVICE  *UsbDev
  )
{
  USB_DEVICE_DESC  *DevDesc;
  EFI_STATUS       Status;

  DevDesc = AllocateZeroPool (sizeof (USB_DEVICE_DESC));

  if (DevDesc == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UsbCtrlGetDesc (
             UsbDev,
             USB_DESC_TYPE_DEVICE,
             0,
             0,
             DevDesc,
             sizeof (EFI_USB_DEVICE_DESCRIPTOR)
             );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (DevDesc);
  } else {
    // Do DevDesc sanity check
    if (  (DevDesc->Desc.DescriptorType != USB_DESC_TYPE_DEVICE)
       || (DevDesc->Desc.Length != sizeof (EFI_USB_DEVICE_DESCRIPTOR))
       || (  (UsbDev->Speed != EFI_USB_SPEED_SUPER)
          && (DevDesc->Desc.MaxPacketSize0 != 8)
          && (DevDesc->Desc.MaxPacketSize0 != 16)
          && (DevDesc->Desc.MaxPacketSize0 != 32)
          && (DevDesc->Desc.MaxPacketSize0 != 64))
       || (DevDesc->Desc.NumConfigurations == 0))
    {
      gBS->FreePool (DevDesc);
      Status = EFI_DEVICE_ERROR;
      return Status;
    }

    UsbDev->DevDesc = DevDesc;
  }

  return Status;
}

/**
  Get the device BOS descriptor for the device.

  @param  UsbDev                The Usb device to retrieve descriptor from.

  @retval EFI_SUCCESS           The device descriptor is returned.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory.

**/
EFI_STATUS
UsbGetDevBOSDesc (
  IN USB_DEVICE  *UsbDev
  )
{
  UINT8       *Buf;
  UINTN       TotalLength;
  EFI_STATUS  Status;

  Buf = AllocateZeroPool (sizeof (USB_BOS_DESC));
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = UsbCtrlGetDesc (
             UsbDev,
             USB_DESC_TYPE_BOS,
             0,
             0,
             Buf,
             sizeof (USB_BOS_DESC)
             );
  if (!EFI_ERROR (Status)) {
    TotalLength = ((USB_BOS_DESC *)Buf)->TotalLength;
    gBS->FreePool (Buf);
    Buf = NULL;
    Buf = AllocateZeroPool (TotalLength);
    if (Buf == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = UsbCtrlGetDesc (
               UsbDev,
               USB_DESC_TYPE_BOS,
               0,
               0,
               Buf,
               TotalLength
               );

    if (EFI_ERROR (Status)) {
      gBS->FreePool (Buf);
      Buf = NULL;
    }

    UsbDev->DevDesc->BOSDesc = Buf;
  }

  return Status;
}

/**
  Retrieve the indexed string for the language. It requires two
  steps to get a string, first to get the string's length. Then
  the string itself.

  @param  UsbDev                The usb device.
  @param  Index                 The index the string to retrieve.
  @param  LangId                Language ID.

  @return The created string descriptor or NULL.

**/
EFI_USB_STRING_DESCRIPTOR *
UsbGetOneString (
  IN     USB_DEVICE  *UsbDev,
  IN     UINT8       Index,
  IN     UINT16      LangId
  )
{
  EFI_USB_STRING_DESCRIPTOR  Desc;
  EFI_STATUS                 Status;
  UINT8                      *Buf;

  EFI_USB_STRING_DESCRIPTOR  *CachedDesc = NULL;

  //
  //  If the String is cached and LangId = US, just return the cached string descriptor
  //
  if ((LangId == USB_US_LANG_ID) && (Index > 0)) {
    Buf = NULL;

    if (Index == UsbDev->DevDesc->Desc.StrManufacturer) {
      if (UsbDev->DevDesc->StrDescManufacturerUS != NULL) {
        CachedDesc = (EFI_USB_STRING_DESCRIPTOR *)UsbDev->DevDesc->StrDescManufacturerUS;
        Buf        = AllocateZeroPool (CachedDesc->Length);
        CopyMem (Buf, (UINT8 *)CachedDesc, CachedDesc->Length);
      }
    } else if (Index == UsbDev->DevDesc->Desc.StrProduct) {
      if (UsbDev->DevDesc->StrDescProductUS != NULL) {
        CachedDesc = (EFI_USB_STRING_DESCRIPTOR *)UsbDev->DevDesc->StrDescProductUS;
        Buf        = AllocateZeroPool (CachedDesc->Length);
        CopyMem (Buf, (UINT8 *)CachedDesc, CachedDesc->Length);
      }
    } else if (Index == UsbDev->DevDesc->Desc.StrSerialNumber) {
      if (UsbDev->DevDesc->StrDescSerialNumberUS != NULL) {
        CachedDesc = (EFI_USB_STRING_DESCRIPTOR *)UsbDev->DevDesc->StrDescSerialNumberUS;
        Buf        = AllocateZeroPool (CachedDesc->Length);
        CopyMem (Buf, (UINT8 *)CachedDesc, CachedDesc->Length);
      }
    } else {
      Buf = NULL;
    }

    if (Buf != NULL) {
      return (EFI_USB_STRING_DESCRIPTOR *)Buf;
    }
  }

  //
  // Copy the mechanism from Linux Driver to get the better compatibility. see usb_string_sub.
  //
  Buf    = AllocateZeroPool (256);
  Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_STRING, Index, LangId, Buf, 255);
  if (EFI_ERROR (Status) ||
      (((EFI_USB_STRING_DESCRIPTOR *)Buf)->Length < OFFSET_OF (EFI_USB_STRING_DESCRIPTOR, Length) + sizeof (((EFI_USB_STRING_DESCRIPTOR *)Buf)->Length)) ||
      (((EFI_USB_STRING_DESCRIPTOR *)Buf)->Length % 2 != 0))
  {
    DEBUG ((DEBUG_ERROR, "UsbGetOneString: Get 255 bytes path failed, Status = %r\n", Status));
    FreePool (Buf);
    Buf = NULL;

    //
    // First get two bytes which contains the string length.
    //
    Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_STRING, Index, LangId, &Desc, 2);

    //
    // Reject if Length even cannot cover itself, or odd because Unicode string byte length should be even.
    //
    if (EFI_ERROR (Status) ||
        (Desc.Length < OFFSET_OF (EFI_USB_STRING_DESCRIPTOR, Length) + sizeof (Desc.Length)) ||
        (Desc.Length % 2 != 0)
        )
    {
      return NULL;
    }

    Buf = AllocateZeroPool (Desc.Length);

    if (Buf == NULL) {
      return NULL;
    }

    Status = UsbCtrlGetDesc (
               UsbDev,
               USB_DESC_TYPE_STRING,
               Index,
               LangId,
               Buf,
               Desc.Length
               );

    if (EFI_ERROR (Status)) {
      FreePool (Buf);
      return NULL;
    }
  }

  return (EFI_USB_STRING_DESCRIPTOR *)Buf;
}

/**
  Build the language ID table for string descriptors.

  @param  UsbDev                The Usb device.

  @retval EFI_UNSUPPORTED       This device doesn't support string table.

**/
EFI_STATUS
UsbBuildLangTable (
  IN USB_DEVICE  *UsbDev
  )
{
  EFI_USB_STRING_DESCRIPTOR  *Desc;
  EFI_STATUS                 Status;
  UINTN                      Index;
  UINTN                      Max;
  UINT16                     *Point;

  //
  // The string of language ID zero returns the supported languages
  //
  Desc = UsbGetOneString (UsbDev, 0, 0);

  if (Desc == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (Desc->Length < 4) {
    Status = EFI_UNSUPPORTED;
    goto ON_EXIT;
  }

  Status = EFI_SUCCESS;

  Max = (Desc->Length - 2) / 2;
  Max = MIN (Max, USB_MAX_LANG_ID);

  Point = Desc->String;
  for (Index = 0; Index < Max; Index++) {
    UsbDev->LangId[Index] = *Point;
    Point++;
  }

  UsbDev->TotalLangId = (UINT16)Max;

  //
  // Some SMART Technologies key says that it supports LangId=0 only, but it
  // responds to USB_US_LANG_ID (English). This is a workaround for all such keys.
  //
  if ((UsbDev->TotalLangId == 1) && (UsbDev->LangId[0] == 0)) {
    UsbDev->LangId[0] = USB_US_LANG_ID;
  }

  //
  // Some devices need to get the string immediately after SW get the first String descriptor
  // for supported language.
  //
  gBS->FreePool (Desc);
  Desc = NULL;
  if (UsbDev->DevDesc->Desc.StrManufacturer != 0) {
    Desc = UsbGetOneString (UsbDev, UsbDev->DevDesc->Desc.StrManufacturer, UsbDev->LangId[0]);
    if (UsbDev->LangId[0] == USB_US_LANG_ID) {
      UsbDev->DevDesc->StrDescManufacturerUS = (UINT8 *)Desc;
    }

    Desc = NULL;
  }

  if (UsbDev->DevDesc->Desc.StrProduct != 0) {
    Desc = UsbGetOneString (UsbDev, UsbDev->DevDesc->Desc.StrProduct, UsbDev->LangId[0]);
    if (UsbDev->LangId[0] == USB_US_LANG_ID) {
      UsbDev->DevDesc->StrDescProductUS = (UINT8 *)Desc;
    }

    Desc = NULL;
  }

  if (UsbDev->DevDesc->Desc.StrSerialNumber != 0) {
    Desc = UsbGetOneString (UsbDev, UsbDev->DevDesc->Desc.StrSerialNumber, UsbDev->LangId[0]);
    if (UsbDev->LangId[0] == USB_US_LANG_ID) {
      UsbDev->DevDesc->StrDescSerialNumberUS = (UINT8 *)Desc;
    }

    Desc = NULL;
  }

  if (Desc == NULL) {
    return Status;
  }

ON_EXIT:
  gBS->FreePool (Desc);
  return Status;
}

/**
  Retrieve the indexed configure for the device. USB device
  returns the configuration together with the interfaces for
  this configuration. Configuration descriptor is also of
  variable length.

  @param  UsbDev                The Usb interface.
  @param  Index                 The index of the configuration.

  @return The created configuration descriptor.

**/
EFI_USB_CONFIG_DESCRIPTOR *
UsbGetOneConfig (
  IN USB_DEVICE  *UsbDev,
  IN UINT8       Index
  )
{
  EFI_USB_CONFIG_DESCRIPTOR  Desc;
  EFI_STATUS                 Status;
  VOID                       *Buf;
  UINT8                      BufDesc[USB_CONFIG_DESC_DEF_ALLOC_LEN];

  //
  // First get four bytes which contains the total length
  // for this configuration.
  //
  switch (UsbDev->EnumScript) {
    case UsbEnumScriptWin:
      ZeroMem (BufDesc, USB_CONFIG_DESC_DEF_ALLOC_LEN);
      Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_CONFIG, Index, 0, BufDesc, USB_CONFIG_DESC_DEF_ALLOC_LEN);
      if (!EFI_ERROR (Status)) {
        CopyMem (&Desc, BufDesc, sizeof (EFI_USB_CONFIG_DESCRIPTOR));
      }

      break;
    case UsbEnumScriptLinux:
      Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_CONFIG, Index, 0, &Desc, sizeof (EFI_USB_CONFIG_DESCRIPTOR));
      break;
    case UsbEnumScriptRsrv:
    case UsbEnumScriptEdk2:
    case UsbEnumScriptUnknown:
    default:
      Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_CONFIG, Index, 0, &Desc, 8);
      break;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "UsbGetOneConfig: failed to get descript length(%d) - %r\n",
      Desc.TotalLength,
      Status
      ));

    return NULL;
  }

  DEBUG ((DEBUG_INFO, "UsbGetOneConfig: total length is %d\n", Desc.TotalLength));

  //
  // Reject if TotalLength even cannot cover itself.
  //
  if (Desc.TotalLength < OFFSET_OF (EFI_USB_CONFIG_DESCRIPTOR, TotalLength) + sizeof (Desc.TotalLength)) {
    return NULL;
  }

  Buf = AllocateZeroPool (Desc.TotalLength);

  if (Buf == NULL) {
    return NULL;
  }

  if ((UsbDev->EnumScript == UsbEnumScriptWin) && (Desc.TotalLength <= USB_CONFIG_DESC_DEF_ALLOC_LEN)) {
    CopyMem (Buf, BufDesc, Desc.TotalLength);
  } else {
    Status = UsbCtrlGetDesc (UsbDev, USB_DESC_TYPE_CONFIG, Index, 0, Buf, Desc.TotalLength);

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "UsbGetOneConfig: failed to get full descript - %r\n", Status));

      FreePool (Buf);
      return NULL;
    }
  }

  return Buf;
}

/**
  Build the whole array of descriptors. This function must
  be called after UsbGetMaxPacketSize0 returns the max packet
  size correctly for endpoint 0.

  @param  UsbDev                The Usb device.

  @retval EFI_SUCCESS           The descriptor table is build.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for the descriptor.

**/
EFI_STATUS
UsbBuildDescTable (
  IN USB_DEVICE  *UsbDev
  )
{
  EFI_USB_CONFIG_DESCRIPTOR  *Config;
  USB_DEVICE_DESC            *DevDesc;
  USB_CONFIG_DESC            *ConfigDesc;
  UINT8                      NumConfig;
  EFI_STATUS                 Status;
  UINT8                      Index;

  //
  // Get the device descriptor, then allocate the configure
  // descriptor pointer array to hold configurations.
  //
  Status = UsbGetDevDesc (UsbDev);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UsbBuildDescTable: failed to get device descriptor - %r\n", Status));
    return Status;
  }

  DevDesc   = UsbDev->DevDesc;
  NumConfig = DevDesc->Desc.NumConfigurations;
  if (NumConfig == 0) {
    return EFI_DEVICE_ERROR;
  }

  DevDesc->Configs = AllocateZeroPool (NumConfig * sizeof (USB_CONFIG_DESC *));
  if (DevDesc->Configs == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, "UsbBuildDescTable: device has %d configures\n", NumConfig));

  //
  // Read each configurations, then parse them
  //
  for (Index = 0; Index < NumConfig; Index++) {
    Config = UsbGetOneConfig (UsbDev, Index);

    if (Config == NULL) {
      DEBUG ((DEBUG_ERROR, "UsbBuildDescTable: failed to get configure (index %d)\n", Index));

      //
      // If we can get the default descriptor, it is likely that the
      // device is still operational.
      //
      if (Index == 0) {
        return EFI_DEVICE_ERROR;
      }

      break;
    }

    ConfigDesc = UsbParseConfigDesc ((UINT8 *)Config, Config->TotalLength);

    FreePool (Config);

    if (ConfigDesc == NULL) {
      DEBUG ((DEBUG_ERROR, "UsbBuildDescTable: failed to parse configure (index %d)\n", Index));

      //
      // If we can get the default descriptor, it is likely that the
      // device is still operational.
      //
      if (Index == 0) {
        return EFI_DEVICE_ERROR;
      }

      break;
    }

    DevDesc->Configs[Index] = ConfigDesc;
  }

  if (DevDesc->Desc.BcdUSB >= 0x210) {
    Status = UsbGetDevBOSDesc (UsbDev);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "UsbBuildDescTable: get BOS descriptor %r\n", Status));
    } else {
      UsbDev->IsSSDev = UsbIsSSDevice (UsbDev->DevDesc->BOSDesc, (UINTN)((USB_BOS_DESC *)(UsbDev->DevDesc->BOSDesc))->TotalLength);
      DEBUG ((DEBUG_INFO, "UsbBuildDescTable: get BOS descriptor %r, UsbDev->IsSSDev = %d\n", Status, UsbDev->IsSSDev));
    }
  }

  //
  // Don't return error even this function failed because
  // it is possible for the device to not support strings.
  //
  Status = UsbBuildLangTable (UsbDev);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "UsbBuildDescTable: get language ID table - %r\n", Status));
  }

  return EFI_SUCCESS;
}

/**
  Set the device's address.

  @param  UsbDev                The device to set address to.
  @param  Address               The address to set.

  @retval EFI_SUCCESS           The device is set to the address.
  @retval Others                Failed to set the device address.

**/
EFI_STATUS
UsbSetAddress (
  IN USB_DEVICE  *UsbDev,
  IN UINT8       Address
  )
{
  EFI_STATUS  Status;

  Status = UsbCtrlRequest (
             UsbDev,
             EfiUsbNoData,
             USB_REQ_TYPE_STANDARD,
             USB_TARGET_DEVICE,
             USB_REQ_SET_ADDRESS,
             Address,
             0,
             NULL,
             0
             );

  return Status;
}

/**
  Set the device's configuration. This function changes
  the device's internal state. UsbSelectConfig changes
  the Usb bus's internal state.

  @param  UsbDev                The USB device to set configure to.
  @param  ConfigIndex           The configure index to set.

  @retval EFI_SUCCESS           The device is configured now.
  @retval Others                Failed to set the device configure.

**/
EFI_STATUS
UsbSetConfig (
  IN USB_DEVICE  *UsbDev,
  IN UINT8       ConfigIndex
  )
{
  EFI_STATUS  Status;

  Status = UsbCtrlRequest (
             UsbDev,
             EfiUsbNoData,
             USB_REQ_TYPE_STANDARD,
             USB_TARGET_DEVICE,
             USB_REQ_SET_CONFIG,
             ConfigIndex,
             0,
             NULL,
             0
             );

  return Status;
}

/**
  Usb UsbIo interface to clear the feature. This is should
  only be used by HUB which is considered a device driver
  on top of the UsbIo interface.

  @param  UsbIo                 The UsbIo interface.
  @param  Target                The target of the transfer: endpoint/device.
  @param  Feature               The feature to clear.
  @param  Index                 The wIndex parameter.

  @retval EFI_SUCCESS           The device feature is cleared.
  @retval Others                Failed to clear the feature.

**/
EFI_STATUS
UsbIoClearFeature (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo,
  IN  UINTN                Target,
  IN  UINT16               Feature,
  IN  UINT16               Index
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  UINT32                  UsbResult;
  EFI_STATUS              Status;

  DevReq.RequestType = USB_REQUEST_TYPE (EfiUsbNoData, USB_REQ_TYPE_STANDARD, Target);
  DevReq.Request     = USB_REQ_CLEAR_FEATURE;
  DevReq.Value       = Feature;
  DevReq.Index       = Index;
  DevReq.Length      = 0;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &DevReq,
                    EfiUsbNoData,
                    USB_CLEAR_FEATURE_REQUEST_TIMEOUT,
                    NULL,
                    0,
                    &UsbResult
                    );

  return Status;
}
