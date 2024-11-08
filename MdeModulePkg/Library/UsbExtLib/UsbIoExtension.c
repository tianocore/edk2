/** @file

  UsbIo Library implementation.

Copyright (c) 2024, American Megatrends International LLC. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UsbExtLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

STATIC UINT8                      *mConfigData;
STATIC EFI_USB_DEVICE_DESCRIPTOR  mDeviceDescriptor;

/**
  Get the standard descriptors.

  @param  UsbIo                 The USB IO protocol to use for the data transfer.
  @param  DescType              The type of descriptor to read.
  @param  DescIndex             The index of descriptor to read.
  @param  LangId                Language ID, only used to get string, otherwise set it to 0.
  @param  Buf                   The caller-allocated buffer to hold the descriptor read.
  @param  Length                The length of the buffer.

  @retval EFI_SUCCESS           The descriptor is read OK.
  @retval Others                Failed to retrieve the descriptor.

**/
EFI_STATUS
UsbIoGetDesc (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo,
  IN  UINTN                DescType,
  IN  UINTN                DescIndex,
  IN  UINT16               LangId,
  OUT VOID                 *Buf,
  IN  UINTN                Length
  )
{
  EFI_STATUS              Status;
  UINT32                  UsbStatus;
  EFI_USB_DEVICE_REQUEST  Req;

  Req.RequestType = 0x80 | USB_REQ_TYPE_STANDARD | USB_TARGET_DEVICE;
  Req.Request     = USB_REQ_GET_DESCRIPTOR;
  Req.Value       = (UINT16)((DescType << 8) | DescIndex);
  Req.Index       = LangId;
  Req.Length      = (UINT16)Length;

  Status = UsbIo->UsbControlTransfer (
                    UsbIo,
                    &Req,
                    EfiUsbDataIn,
                    0,
                    Buf,
                    Length,
                    &UsbStatus
                    );

  return Status;
}

/**
  Report if the static configuration data needs to be udpated.

  Check the given device descriptor and configuration descriptor against the ones stored globally.

  @param  DevDesc               The USB device descriptor.
  @param  CnfDesc               The USB configuration descriptor.

  @retval TRUE                  The descriptors do not match to the global data.
  @retval FALSE                 The descriptors match, no update is needed.

**/
BOOLEAN
ConfigUpdateNeeded (
  EFI_USB_DEVICE_DESCRIPTOR  *DevDesc,
  EFI_USB_CONFIG_DESCRIPTOR  *CnfDesc
  )
{
  return (  (CompareMem (DevDesc, &mDeviceDescriptor, sizeof (EFI_USB_DEVICE_DESCRIPTOR)) != 0)
         || (CompareMem (CnfDesc, mConfigData, sizeof (EFI_USB_CONFIG_DESCRIPTOR)) != 0));
}

/**
  Global data initialization.

  Library public functions' input is the instance of UsbIo protocol. Check if the global
  data relevant to the UsbIo. If not, read the device and update the global data.

  @param  UsbIo           The instance of EFI_USB_IO_PROTOCOL.

  @retval EFI_SUCCESS     The global data is updated.
  @retval EFI_NOT_FOUND   The UsbIo configuration was not found.

**/
EFI_STATUS
InitUsbConfigDescriptorData (
  EFI_USB_IO_PROTOCOL  *UsbIo
  )
{
  EFI_STATUS                 Status;
  EFI_USB_DEVICE_DESCRIPTOR  DevDesc;
  EFI_USB_CONFIG_DESCRIPTOR  CnfDesc;
  UINT8                      ConfigNum;
  UINT8                      ConfigValue;

  //
  // Get UsbIo device and configuration descriptors.
  //
  Status = UsbIo->UsbGetDeviceDescriptor (UsbIo, &DevDesc);
  ASSERT_EFI_ERROR (Status);

  Status = UsbIo->UsbGetConfigDescriptor (UsbIo, &CnfDesc);
  ASSERT_EFI_ERROR (Status);

  if (mConfigData != NULL) {
    if (!ConfigUpdateNeeded (&DevDesc, &CnfDesc)) {
      return EFI_SUCCESS;
    }

    gBS->FreePool (mConfigData);
    mConfigData = NULL;
  }

  CopyMem (&mDeviceDescriptor, &DevDesc, sizeof (EFI_USB_DEVICE_DESCRIPTOR));

  //
  // Examine device with multiple configurations: find configuration index of UsbIo config descriptor.
  //
  // Use EFI_USB_DEVICE_DESCRIPTOR.NumConfigurations to loop through configuration descriptors, match
  // EFI_USB_CONFIG_DESCRIPTOR.ConfigurationValue to the configuration value reported by UsbIo->UsbGetConfigDescriptor.
  // The index of the matched configuration is used in wValue of the following GET_DESCRIPTOR request.
  //
  ConfigValue = CnfDesc.ConfigurationValue;
  for (ConfigNum = 0; ConfigNum < DevDesc.NumConfigurations; ConfigNum++) {
    Status = UsbIoGetDesc (UsbIo, USB_DESC_TYPE_CONFIG, ConfigNum, 0, &CnfDesc, sizeof (EFI_USB_CONFIG_DESCRIPTOR));
    ASSERT_EFI_ERROR (Status);

    if (CnfDesc.ConfigurationValue == ConfigValue) {
      break;
    }
  }

  ASSERT (ConfigNum < DevDesc.NumConfigurations);
  if (ConfigNum == DevDesc.NumConfigurations) {
    return EFI_NOT_FOUND;
  }

  //
  // ConfigNum has zero based index of the configuration that UsbIo belongs to. Use this index to retrieve
  // full configuration descriptor data.
  //
  Status = gBS->AllocatePool (EfiBootServicesData, CnfDesc.TotalLength, (VOID **)&mConfigData);
  ASSERT_EFI_ERROR (Status);

  Status = UsbIoGetDesc (UsbIo, USB_DESC_TYPE_CONFIG, ConfigNum, 0, mConfigData, CnfDesc.TotalLength);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Find descriptor of a given type within data area pointed by mConfigData.

  The following are the assumptions of the configuration descriptor layout:
  - mConfigData is populated with the configuration data that contains USB interface referenced by UsbIo.
  - Endpoint may have only one class specific descriptor that immediately follows the endpoint descriptor.

  @param[in]  UsbIo             A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in]  DescType          Type of descriptor to look for.
  @param[in]  Setting           Interface alternate setting.
  @param[in]  Index             Index of the descriptor. This descriptor index is used to find a specific
                                descriptor (only for endpoint descriptors and class specific interface descriptors)
                                when several descriptors of the same type are implemented in a device. For other
                                descriptor types, a descriptor index of zero must be used.
  @param[out]  Data             A pointer to the caller allocated Descriptor.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_UNSUPPORTED       Setting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of descriptors of the requested type in this
                                interface.
**/
EFI_STATUS
FindUsbDescriptor (
  EFI_USB_IO_PROTOCOL  *UsbIo,
  UINT8                DescType,
  UINTN                Setting,
  UINTN                Index,
  VOID                 **Data
  )
{
  EFI_USB_INTERFACE_DESCRIPTOR  IntfDesc;
  EFI_STATUS                    Status;
  UINT8                         *BufferPtr;
  UINT8                         *BufferEnd;
  UINT8                         *ConfigEnd;
  UINTN                         Idx;

  //
  // Find the interface descriptor referenced by UsbIo in the current configuration
  //
  Status = UsbIo->UsbGetInterfaceDescriptor (UsbIo, &IntfDesc);
  ASSERT_EFI_ERROR (Status);

  ConfigEnd = mConfigData + ((EFI_USB_CONFIG_DESCRIPTOR *)mConfigData)->TotalLength;

  for (BufferPtr = mConfigData; BufferPtr < ConfigEnd; BufferPtr += BufferPtr[0]) {
    if (BufferPtr[1] == USB_DESC_TYPE_INTERFACE) {
      if ((BufferPtr[2] == IntfDesc.InterfaceNumber) && (BufferPtr[3] == Setting)) {
        break;
      }
    }
  }

  if (BufferPtr >= ConfigEnd) {
    return EFI_UNSUPPORTED;
  }

  //
  // Found the beginning of the interface, find the ending
  //
  for (BufferEnd = BufferPtr + BufferPtr[0]; BufferEnd < ConfigEnd; BufferEnd += BufferEnd[0]) {
    if (BufferEnd[1] == USB_DESC_TYPE_INTERFACE) {
      break;
    }
  }

  Idx = 0;

  if (DescType == USB_DESC_TYPE_INTERFACE) {
    *Data = BufferPtr;
    return EFI_SUCCESS;
  }

  if ((DescType == USB_DESC_TYPE_ENDPOINT) || (DescType == USB_DESC_TYPE_CS_ENDPOINT)) {
    while (BufferPtr < BufferEnd) {
      BufferPtr += BufferPtr[0];
      if (BufferPtr[1] == USB_DESC_TYPE_ENDPOINT) {
        if (Idx == Index) {
          if (DescType == USB_DESC_TYPE_CS_ENDPOINT) {
            BufferPtr += BufferPtr[0];
            if (BufferPtr[1] != USB_DESC_TYPE_CS_ENDPOINT) {
              break;
            }
          }

          *Data = BufferPtr;
          return EFI_SUCCESS;
        }

        Idx++;
      }
    }
  }

  if (DescType  == USB_DESC_TYPE_CS_INTERFACE) {
    while (BufferPtr < BufferEnd) {
      BufferPtr += BufferPtr[0];
      if (BufferPtr[1] == USB_DESC_TYPE_CS_INTERFACE) {
        if (Idx == Index) {
          *Data = BufferPtr;
          return EFI_SUCCESS;
        }

        Idx++;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Retrieve the number of class specific interface descriptors.

  @param[in]  Data    A pointer to the USB interface descriptor that may contain class code descriptors.

  @retval UINT8       Number of the class code interface descriptors.

**/
UINT8
FindNumberOfCsInterfaces (
  VOID  *Data
  )
{
  UINT8  *Buffer;
  UINT8  *ConfigEnd;
  UINT8  Index;

  Buffer    = Data;
  ConfigEnd = mConfigData + ((EFI_USB_CONFIG_DESCRIPTOR *)mConfigData)->TotalLength;

  Index = 0;

  for (Buffer += Buffer[0]; Buffer < ConfigEnd; Buffer += Buffer[0]) {
    if (Buffer[1] == USB_DESC_TYPE_INTERFACE) {
      break;
    }

    if (Buffer[1] == USB_DESC_TYPE_CS_INTERFACE) {
      Index++;
    }
  }

  return Index;
}

/**
  Retrieve the interface descriptor details from the interface setting.

  This is an extended version of UsbIo->GetInterfaceDescriptor. It returns the interface
  descriptor for an alternate setting of the interface without executing SET_INTERFACE
  transfer. It also returns the number of class specific interfaces.

  @param[in]  This              A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in]  Setting           Interface alternate setting.
  @param[out]  Descriptor       The caller allocated buffer to return the contents of the Interface descriptor.
  @param[out]  CsInterfaceNumber  Number of class specific interfaces for this interface setting.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor or CsInterfaceNumber is NULL.
  @retval EFI_UNSUPPORTED       Setting is greater than the number of alternate settings in this interface.
  @retval EFI_DEVICE_ERROR      Error reading device data.

**/
EFI_STATUS
EFIAPI
UsbGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  IN  UINTN                         Setting,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor,
  OUT UINTN                         *CsInterfacesNumber
  )
{
  EFI_STATUS  Status;
  VOID        *Data;

  if ((Descriptor == NULL) || (CsInterfacesNumber == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InitUsbConfigDescriptorData (This);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = FindUsbDescriptor (This, USB_DESC_TYPE_INTERFACE, Setting, 0, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *CsInterfacesNumber = FindNumberOfCsInterfaces (Data);
  CopyMem (Descriptor, Data, sizeof (EFI_USB_INTERFACE_DESCRIPTOR));

  return EFI_SUCCESS;
}

/**
  Retrieve the endpoint descriptor from the interface setting.

  This is an extended version of UsbIo->GetEndpointDescriptor. It returns the endpoint
  descriptor for an alternate setting of a given interface.
  Note: The total number of endpoints can be retrieved from the interface descriptor
  returned by EDKII_USBIO_EXT_GET_INTERFACE_DESCRIPTOR function.

  @param[in]  This              A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in]  Setting           Interface alternate setting.
  @param[in]  Index             Index of the endpoint to retrieve. The valid range is 0..15.
  @param[out]  Descriptor       A pointer to the caller allocated USB Interface Descriptor.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER Descriptor is NULL.
  @retval EFI_UNSUPPORTED       Setting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of endpoints in this interface.
  @retval EFI_DEVICE_ERROR      Error reading device data.

**/
EFI_STATUS
EFIAPI
UsbGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL          *This,
  IN  UINTN                        Setting,
  IN  UINTN                        Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR  *Descriptor
  )
{
  EFI_STATUS  Status;
  VOID        *Data;

  if (Descriptor == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InitUsbConfigDescriptorData (This);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = FindUsbDescriptor (This, USB_DESC_TYPE_ENDPOINT, Setting, Index, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CopyMem (Descriptor, Data, sizeof (EFI_USB_ENDPOINT_DESCRIPTOR));

  return EFI_SUCCESS;
}

/**
  Retrieve class specific interface descriptor.

  @param[in]  This              A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in]  Setting           Interface alternate setting.
  @param[in]  Index             Zero-based index of the class specific interface.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific interface descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_UNSUPPORTED       Setting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of class specific interfaces.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
  @retval EFI_DEVICE_ERROR      Error reading device data.

**/
EFI_STATUS
EFIAPI
UsbGetCsInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINTN                Setting,
  IN  UINTN                Index,
  IN OUT UINTN             *BufferSize,
  OUT VOID                 *Buffer
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINT8       DescLength;

  if ((BufferSize == NULL) || ((Buffer == NULL) && (*BufferSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InitUsbConfigDescriptorData (This);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = FindUsbDescriptor (This, USB_DESC_TYPE_CS_INTERFACE, Setting, Index, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescLength = ((UINT8 *)Data)[0];

  if ((Buffer == NULL) || (DescLength > *BufferSize)) {
    *BufferSize = DescLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, Data, DescLength);

  return EFI_SUCCESS;
}

/**
  Retrieve class specific endpoint descriptor.

  @param[in]  This              A pointer to the EFI_USB_IO_PROTOCOL instance.
  @param[in]  Setting           Interface alternate setting.
  @param[in]  Index             Zero-based index of the non-zero endpoint.
  @param[in][out]  BufferSize   On input, the size in bytes of the return Descriptor buffer.
                                On output the size of data returned in Descriptor.
  @param[out]  Descriptor       The buffer to return the contents of the class specific endpoint descriptor. May
                                be NULL with a zero BufferSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS           Output parameters were updated successfully.
  @retval EFI_INVALID_PARAMETER BufferSize is NULL.
                                Buffer is NULL and *BufferSize is not zero.
  @retval EFI_UNSUPPORTED       Setting is greater than the number of alternate settings in this interface.
  @retval EFI_NOT_FOUND         Index is greater than the number of endpoints in this interface.
                                Endpoint does not have class specific endpoint descriptor.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize is too small for the result. BufferSize has been updated with the size
                                needed to complete the request.
  @retval EFI_DEVICE_ERROR      Error reading device data.

**/
EFI_STATUS
EFIAPI
UsbGetCsEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINTN                Setting,
  IN  UINTN                Index,
  IN OUT UINTN             *BufferSize,
  OUT VOID                 *Buffer
  )
{
  EFI_STATUS  Status;
  VOID        *Data;
  UINT8       DescLength;

  if ((BufferSize == NULL) || ((Buffer == NULL) && (*BufferSize != 0))) {
    return EFI_INVALID_PARAMETER;
  }

  Status = InitUsbConfigDescriptorData (This);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Status = FindUsbDescriptor (This, USB_DESC_TYPE_CS_ENDPOINT, Setting, Index, &Data);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DescLength = ((UINT8 *)Data)[0];

  if ((Buffer == NULL) || (DescLength > *BufferSize)) {
    *BufferSize = DescLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  CopyMem (Buffer, Data, DescLength);

  return EFI_SUCCESS;
}

/**
  The constructor function initializes global pointer to the configuration data.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
UsbExtLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mConfigData = NULL;
  DEBUG ((DEBUG_ERROR, "UsbExtLibConstructor\n"));

  return EFI_SUCCESS;
}

/**
  Destructor frees memory which was possibly allocated by the library functions.

  @param ImageHandle       Handle that identifies the image to be unloaded.
  @param  SystemTable      The system table.

  @retval EFI_SUCCESS      The image has been unloaded.

**/
EFI_STATUS
EFIAPI
UsbExtLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (mConfigData != NULL) {
    gBS->FreePool (mConfigData);
  }

  DEBUG ((DEBUG_ERROR, "UsbExtLibDestructor\n"));
  return EFI_SUCCESS;
}
