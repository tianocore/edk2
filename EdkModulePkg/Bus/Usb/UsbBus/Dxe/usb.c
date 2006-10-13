/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:

    Usb.c

  Abstract:

    Parse usb device configurations.

  Revision History

--*/

#include "usbbus.h"

//
// Here are some internal helper functions
//
STATIC
EFI_STATUS
GetExpectedDescriptor (
  IN  UINT8     *Buffer,
  IN  UINTN     Length,
  IN  UINT8     DescType,
  IN  UINT8     DescLength,
  OUT UINTN     *ParsedBytes
  );

STATIC
EFI_STATUS
ParseThisEndpoint (
  IN  ENDPOINT_DESC_LIST_ENTRY     *EndpointEntry,
  IN  UINT8                        *Buffer,
  IN  UINTN                        BufferLength,
  OUT UINTN                        *ParsedBytes
  );

STATIC
EFI_STATUS
ParseThisInterface (
  IN  INTERFACE_DESC_LIST_ENTRY      *InterfaceEntry,
  IN  UINT8                          *Buffer,
  IN  UINTN                          *BufferLen,
  OUT UINTN                          *ParsedBytes
  );

STATIC
EFI_STATUS
ParseThisConfig (
  IN CONFIG_DESC_LIST_ENTRY     *ConfigDescEntry,
  IN UINT8                      *Buffer,
  IN UINTN                      Length
  );

//
// Implementations
//
BOOLEAN
IsHub (
  IN USB_IO_CONTROLLER_DEVICE     *Dev
  )
/*++
  
  Routine Description:
    Tell if a usb controller is a hub controller.
    
  Arguments:
    Dev - UsbIoController device structure.
    
  Returns:
    TRUE/FALSE
--*/
{
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
  UINT8                         Index;

  if (Dev == NULL) {
    return FALSE;
  }

  UsbIo = &Dev->UsbIo;

  UsbIo->UsbGetInterfaceDescriptor (
          UsbIo,
          &Interface
          );

  //
  // Check classcode
  //
  if (Interface.InterfaceClass != 0x09) {
    return FALSE;
  }
  
  //
  // Check protocol
  //
  if (Interface.InterfaceProtocol != 0x0) {
    return FALSE;
  }

  for (Index = 0; Index < Interface.NumEndpoints; Index++) {
    UsbIo->UsbGetEndpointDescriptor (
            UsbIo,
            Index,
            &EndpointDescriptor
            );

    if ((EndpointDescriptor.EndpointAddress & 0x80) == 0) {
      continue;
    }

    if (EndpointDescriptor.Attributes != 0x03) {
      continue;
    }

    Dev->HubEndpointAddress = EndpointDescriptor.EndpointAddress;
    return TRUE;
  }

  return FALSE;
}

EFI_STATUS
UsbGetStringtable (
  IN USB_IO_DEVICE     *Dev
  )
/*++
  
  Routine Description:
    Get the string table stored in a usb device.
    
  Arguments:
    Dev     -     UsbIoController device structure.
    
  Returns:
    EFI_SUCCESS
    EFI_UNSUPPORTED
    EFI_OUT_OF_RESOURCES
    
--*/
{
  EFI_STATUS                  Result;
  UINT32                      Status;
  EFI_USB_SUPPORTED_LANGUAGES *LanguageTable;
  UINT8                       *Buffer;
  UINT8                       *ptr;
  UINTN                       Index;
  UINTN                       LangTableSize;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  UINT16                      TempBuffer;

  UsbIo = &(Dev->UsbController[0]->UsbIo);

  //
  // We get first 2 byte of langID table,
  // so we can have the whole table length
  //
  Result = UsbGetString (
            UsbIo,
            0,
            0,
            &TempBuffer,
            2,
            &Status
            );
  if (EFI_ERROR (Result)) {
    return EFI_UNSUPPORTED;
  }

  LanguageTable = (EFI_USB_SUPPORTED_LANGUAGES *) &TempBuffer;

  if (LanguageTable->Length == 0) {
    return EFI_UNSUPPORTED;
  }
  //
  // If length is 2, then there is no string table
  //
  if (LanguageTable->Length == 2) {
    return EFI_UNSUPPORTED;
  }

  Buffer = AllocateZeroPool (LanguageTable->Length);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Now we get the whole LangID table
  //
  Result = UsbGetString (
            UsbIo,
            0,
            0,
            Buffer,
            LanguageTable->Length,
            &Status
            );
  if (EFI_ERROR (Result)) {
    gBS->FreePool (Buffer);
    return EFI_UNSUPPORTED;
  }

  LanguageTable = (EFI_USB_SUPPORTED_LANGUAGES *) Buffer;

  //
  // ptr point to the LangID table
  //
  ptr           = Buffer + 2;
  LangTableSize = (LanguageTable->Length - 2) / 2;

  for (Index = 0; Index < LangTableSize && Index < USB_MAXLANID; Index++) {
    Dev->LangID[Index] = *((UINT16 *) ptr);
    ptr += 2;
  }

  gBS->FreePool (Buffer);
  LanguageTable = NULL;

  return EFI_SUCCESS;
}


EFI_STATUS
UsbGetAllConfigurations (
  IN USB_IO_DEVICE     *UsbIoDevice
  )
/*++

  Routine Description:
    This function is to parse all the configuration descriptor.
    
  Arguments:
    UsbIoDevice  -  USB_IO_DEVICE device structure.
    
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    EFI_OUT_OF_RESOURCES  

--*/
{
  EFI_STATUS              Result;
  UINT32                  Status;
  UINTN                   Index;
  UINTN                   TotalLength;
  UINT8                   *Buffer;
  CONFIG_DESC_LIST_ENTRY  *ConfigDescEntry;
  EFI_USB_IO_PROTOCOL     *UsbIo;

  InitializeListHead (&UsbIoDevice->ConfigDescListHead);
  UsbIo = &(UsbIoDevice->UsbController[0]->UsbIo);

  for (Index = 0; Index < UsbIoDevice->DeviceDescriptor.NumConfigurations; Index++) {
    ConfigDescEntry = NULL;

    ConfigDescEntry = AllocateZeroPool (sizeof (CONFIG_DESC_LIST_ENTRY));
    if (ConfigDescEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // 1st only get 1st 4 bytes config descriptor,
    // so we can know the whole length
    //
    Result = UsbGetDescriptor (
              UsbIo,
              (UINT16) ((USB_DT_CONFIG << 8) | Index),
              0,
              4,
              &ConfigDescEntry->CongfigDescriptor,
              &Status
              );
    if (EFI_ERROR (Result)) {
      DEBUG ((gUSBErrorLevel, "First get config descriptor error\n"));
      gBS->FreePool (ConfigDescEntry);
      return EFI_DEVICE_ERROR;
    }

    TotalLength = ConfigDescEntry->CongfigDescriptor.TotalLength;

    Buffer      = AllocateZeroPool (TotalLength);
    if (Buffer == NULL) {
      gBS->FreePool (ConfigDescEntry);
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Then we get the total descriptors for this configuration
    //
    Result = UsbGetDescriptor (
              UsbIo,
              (UINT16) ((USB_DT_CONFIG << 8) | Index),
              0,
              (UINT16) TotalLength,
              Buffer,
              &Status
              );
    if (EFI_ERROR (Result)) {
      DEBUG ((gUSBErrorLevel, "Get whole config descriptor error\n"));
      gBS->FreePool (ConfigDescEntry);
      gBS->FreePool (Buffer);
      return EFI_DEVICE_ERROR;
    }

    InitializeListHead (&ConfigDescEntry->InterfaceDescListHead);

    //
    // Parse this whole configuration
    //
    Result = ParseThisConfig (ConfigDescEntry, Buffer, TotalLength);

    if (EFI_ERROR (Result)) {
      //
      // Ignore this configuration, parse next one
      //
      gBS->FreePool (ConfigDescEntry);
      gBS->FreePool (Buffer);
      continue;
    }

    InsertTailList (&UsbIoDevice->ConfigDescListHead, &ConfigDescEntry->Link);

    gBS->FreePool (Buffer);

  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
GetExpectedDescriptor (
  IN  UINT8     *Buffer,
  IN  UINTN     Length,
  IN  UINT8     DescType,
  IN  UINT8     DescLength,
  OUT UINTN     *ParsedBytes
  )
/*++
  
  Routine Description:
    Get the start position of next wanted descriptor.
    
  Arguments:
    Buffer      - Buffer to parse
    Length      - Buffer length 
    DescType    - Descriptor type 
    DescLength  - Descriptor length
    ParsedBytes - Parsed Bytes to return
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  UINT16  DescriptorHeader;
  UINT8   Len;
  UINT8   *ptr;
  UINTN   Parsed;

  Parsed  = 0;
  ptr     = Buffer;

  while (TRUE) {
    //
    // Buffer length should not less than Desc length
    //
    if (Length < DescLength) {
      return EFI_DEVICE_ERROR;
    }
    //
    // DescriptorHeader = *((UINT16 *)ptr), compatible with IPF
    //
    DescriptorHeader  = (UINT16) ((*(ptr + 1) << 8) | *ptr);

    Len               = ptr[0];

    //
    // Check to see if it is a start of expected descriptor
    //
    if (DescriptorHeader == ((DescType << 8) | DescLength)) {
      break;
    }

    if ((UINT8) (DescriptorHeader >> 8) == DescType) {
      if (Len > DescLength) {
        return EFI_DEVICE_ERROR;
      }
    }
    //
    // Descriptor length should be at least 2
    // and should not exceed the buffer length
    //
    if (Len < 2) {
      return EFI_DEVICE_ERROR;
    }

    if (Len > Length) {
      return EFI_DEVICE_ERROR;
    }
    //
    // Skip this mismatch descriptor
    //
    Length -= Len;
    ptr += Len;
    Parsed += Len;
  }

  *ParsedBytes = Parsed;

  return EFI_SUCCESS;
}


STATIC
EFI_STATUS
ParseThisEndpoint (
  IN  ENDPOINT_DESC_LIST_ENTRY     *EndpointEntry,
  IN  UINT8                        *Buffer,
  IN  UINTN                        BufferLength,
  OUT UINTN                        *ParsedBytes
  )
/*++

  Routine Description:
    Get the start position of next wanted endpoint descriptor.

  Arguments:
    EndpointEntry - ENDPOINT_DESC_LIST_ENTRY
    Buffer        - Buffer to parse 
    BufferLength  - Buffer Length
    ParsedBytes   - Parsed Bytes to return
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  UINT8       *ptr;
  EFI_STATUS  Status;
  UINTN       SkipBytes;

  //
  // Skip some data for this interface
  //
  Status = GetExpectedDescriptor (
            Buffer,
            BufferLength,
            USB_DT_ENDPOINT,
            sizeof (EFI_USB_ENDPOINT_DESCRIPTOR),
            &SkipBytes
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ptr           = Buffer + SkipBytes;
  *ParsedBytes  = SkipBytes;

  CopyMem (
    &EndpointEntry->EndpointDescriptor,
    ptr,
    sizeof (EFI_USB_ENDPOINT_DESCRIPTOR)
    );

  *ParsedBytes += sizeof (EFI_USB_ENDPOINT_DESCRIPTOR);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ParseThisInterface (
  IN  INTERFACE_DESC_LIST_ENTRY     *InterfaceEntry,
  IN  UINT8                         *Buffer,
  IN  UINTN                         *BufferLen,
  OUT UINTN                         *ParsedBytes
  )
/*++

  Routine Description:
    Get the start position of next wanted interface descriptor.

  Arguments:
    InterfaceEntry - INTERFACE_DESC_LIST_ENTRY
    Buffer         - Buffer to parse 
    BufferLength   - Buffer Length
    ParsedBytes    - Parsed Bytes to return

  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  UINT8                     *ptr;
  UINTN                     SkipBytes;
  UINTN                     Index;
  UINTN                     Length;
  UINTN                     Parsed;
  ENDPOINT_DESC_LIST_ENTRY  *EndpointEntry;
  EFI_STATUS                Status;

  Parsed = 0;

  //
  // Skip some data for this interface
  //
  Status = GetExpectedDescriptor (
            Buffer,
            *BufferLen,
            USB_DT_INTERFACE,
            sizeof (EFI_USB_INTERFACE_DESCRIPTOR),
            &SkipBytes
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ptr           = Buffer + SkipBytes;
  *ParsedBytes  = SkipBytes;

  //
  // Copy the interface descriptor
  //
  CopyMem (
    &InterfaceEntry->InterfaceDescriptor,
    ptr,
    sizeof (EFI_USB_INTERFACE_DESCRIPTOR)
    );

  ptr = Buffer + sizeof (EFI_USB_INTERFACE_DESCRIPTOR);
  *ParsedBytes += sizeof (EFI_USB_INTERFACE_DESCRIPTOR);

  InitializeListHead (&InterfaceEntry->EndpointDescListHead);

  Length = *BufferLen - SkipBytes - sizeof (EFI_USB_INTERFACE_DESCRIPTOR);

  for (Index = 0; Index < InterfaceEntry->InterfaceDescriptor.NumEndpoints; Index++) {
    EndpointEntry = AllocateZeroPool (sizeof (ENDPOINT_DESC_LIST_ENTRY));
    if (EndpointEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    
    //
    // Parses all the endpoint descriptors within this interface.
    //
    Status = ParseThisEndpoint (EndpointEntry, ptr, Length, &Parsed);

    if (EFI_ERROR (Status)) {
      gBS->FreePool (EndpointEntry);
      return Status;
    }

    InsertTailList (
      &InterfaceEntry->EndpointDescListHead,
      &EndpointEntry->Link
      );

    Length -= Parsed;
    ptr += Parsed;
    *ParsedBytes += Parsed;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ParseThisConfig (
  IN CONFIG_DESC_LIST_ENTRY     *ConfigDescEntry,
  IN UINT8                      *Buffer,
  IN UINTN                      Length
  )
/*++

  Routine Description:
    Parse the current configuration descriptior.

  Arguments:
    ConfigDescEntry - CONFIG_DESC_LIST_ENTRY
    Buffer          - Buffer to parse 
    Length          - Buffer Length

  Returns
    EFI_SUCCESS
    EFI_DEVICE_ERROR

--*/
{
  UINT8                     *ptr;
  UINT8                     NumInterface;
  UINTN                     Index;
  INTERFACE_DESC_LIST_ENTRY *InterfaceEntry;
  UINTN                     SkipBytes;
  UINTN                     Parsed;
  EFI_STATUS                Status;
  UINTN                     LengthLeft;

  Parsed = 0;

  //
  //  First skip the current config descriptor;
  //
  Status = GetExpectedDescriptor (
            Buffer,
            Length,
            USB_DT_CONFIG,
            sizeof (EFI_USB_CONFIG_DESCRIPTOR),
            &SkipBytes
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ptr = Buffer + SkipBytes;

  CopyMem (
    &ConfigDescEntry->CongfigDescriptor,
    ptr,
    sizeof (EFI_USB_CONFIG_DESCRIPTOR)
    );

  NumInterface = ConfigDescEntry->CongfigDescriptor.NumInterfaces;

  //
  // Skip size of Configuration Descriptor
  //
  ptr += sizeof (EFI_USB_CONFIG_DESCRIPTOR);

  LengthLeft = Length - SkipBytes - sizeof (EFI_USB_CONFIG_DESCRIPTOR);

  for (Index = 0; Index < NumInterface; Index++) {
    //
    // Parse all Interface
    //
    InterfaceEntry = AllocateZeroPool (sizeof (INTERFACE_DESC_LIST_ENTRY));
    if (InterfaceEntry == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = ParseThisInterface (InterfaceEntry, ptr, &LengthLeft, &Parsed);
    if (EFI_ERROR (Status)) {
      gBS->FreePool (InterfaceEntry);
      return Status;
    }

    InsertTailList (
      &ConfigDescEntry->InterfaceDescListHead,
      &InterfaceEntry->Link
      );

    //
    // Parsed for next interface
    //
    LengthLeft -= Parsed;
    ptr += Parsed;
  }
  //
  // Parse for additional alt setting;
  //
  return EFI_SUCCESS;
}

EFI_STATUS
UsbSetConfiguration (
  IN USB_IO_DEVICE     *UsbIoDev,
  IN UINTN             ConfigurationValue
  )
/*++

  Routine Description:
    Set the device to a configuration value.
    
  Arguments:
    UsbIoDev            -   USB_IO_DEVICE to be set configuration
    ConfigrationValue   -   The configuration value to be set to that device
    
  Returns:
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/
{
  LIST_ENTRY              *NextEntry;
  CONFIG_DESC_LIST_ENTRY  *ConfigEntry;
  UINT32                  Status;
  EFI_STATUS              Result;
  EFI_USB_IO_PROTOCOL     *UsbIo;

  UsbIo     = &(UsbIoDev->UsbController[0]->UsbIo);
  NextEntry = UsbIoDev->ConfigDescListHead.ForwardLink;

  while (NextEntry != &UsbIoDev->ConfigDescListHead) {
    //
    // Get one entry
    //
    ConfigEntry = (CONFIG_DESC_LIST_ENTRY *) NextEntry;
    if (ConfigEntry->CongfigDescriptor.ConfigurationValue == ConfigurationValue) {
      //
      // Find one, set to the active configuration
      //
      UsbIoDev->ActiveConfig = ConfigEntry;
      break;
    }

    NextEntry = NextEntry->ForwardLink;
  }
  //
  // Next Entry should not be null
  //
  Result = UsbSetDeviceConfiguration (
            UsbIo,
            (UINT16) ConfigurationValue,
            &Status
            );

  return Result;
}

EFI_STATUS
UsbSetDefaultConfiguration (
  IN  USB_IO_DEVICE      *UsbIoDev
  )
/*++

  Routine Description:
    Set the device to a default configuration value.
    
  Arguments:
    UsbIoDev       -    USB_IO_DEVICE to be set configuration
    
  Returns
    EFI_SUCCESS
    EFI_DEVICE_ERROR
    
--*/
{
  CONFIG_DESC_LIST_ENTRY  *ConfigEntry;
  UINT16                  ConfigValue;
  LIST_ENTRY              *NextEntry;

  if (IsListEmpty (&UsbIoDev->ConfigDescListHead)) {
    return EFI_DEVICE_ERROR;
  }

  NextEntry   = UsbIoDev->ConfigDescListHead.ForwardLink;

  ConfigEntry = (CONFIG_DESC_LIST_ENTRY *) NextEntry;
  ConfigValue = ConfigEntry->CongfigDescriptor.ConfigurationValue;

  return UsbSetConfiguration (UsbIoDev, ConfigValue);
}

VOID
UsbDestroyAllConfiguration (
  IN  USB_IO_DEVICE      *UsbIoDevice
  )
/*++

  Routine Description:
    Delete all configuration data when device is not used.
    
  Arguments:
    UsbIoDevice  - USB_IO_DEVICE to be set configuration
  
  Returns:
    N/A
    
--*/
{
  CONFIG_DESC_LIST_ENTRY    *ConfigEntry;
  INTERFACE_DESC_LIST_ENTRY *InterfaceEntry;
  ENDPOINT_DESC_LIST_ENTRY  *EndpointEntry;
  LIST_ENTRY                *NextEntry;

  //
  // Delete all configuration descriptor data
  //
  ConfigEntry = (CONFIG_DESC_LIST_ENTRY *) UsbIoDevice->ConfigDescListHead.ForwardLink;

  while (ConfigEntry != (CONFIG_DESC_LIST_ENTRY *) &UsbIoDevice->ConfigDescListHead) {
    //
    // Delete all its interface descriptors
    //
    InterfaceEntry = (INTERFACE_DESC_LIST_ENTRY *) ConfigEntry->InterfaceDescListHead.ForwardLink;

    while (InterfaceEntry != (INTERFACE_DESC_LIST_ENTRY *) &ConfigEntry->InterfaceDescListHead) {
      //
      // Delete all its endpoint descriptors
      //
      EndpointEntry = (ENDPOINT_DESC_LIST_ENTRY *) InterfaceEntry->EndpointDescListHead.ForwardLink;
      while (EndpointEntry != (ENDPOINT_DESC_LIST_ENTRY *) &InterfaceEntry->EndpointDescListHead) {
        NextEntry = ((LIST_ENTRY *) EndpointEntry)->ForwardLink;
        RemoveEntryList ((LIST_ENTRY *) EndpointEntry);
        gBS->FreePool (EndpointEntry);
        EndpointEntry = (ENDPOINT_DESC_LIST_ENTRY *) NextEntry;
      }

      NextEntry = ((LIST_ENTRY *) InterfaceEntry)->ForwardLink;
      RemoveEntryList ((LIST_ENTRY *) InterfaceEntry);
      gBS->FreePool (InterfaceEntry);
      InterfaceEntry = (INTERFACE_DESC_LIST_ENTRY *) NextEntry;
    }

    NextEntry = ((LIST_ENTRY *) ConfigEntry)->ForwardLink;
    RemoveEntryList ((LIST_ENTRY *) ConfigEntry);
    gBS->FreePool (ConfigEntry);
    ConfigEntry = (CONFIG_DESC_LIST_ENTRY *) NextEntry;
  }
}
