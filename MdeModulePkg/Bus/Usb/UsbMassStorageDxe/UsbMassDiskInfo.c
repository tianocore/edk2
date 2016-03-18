/** @file
  This file is used to implement the EFI_DISK_INFO_PROTOCOL interface.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbMass.h"

EFI_DISK_INFO_PROTOCOL gUsbDiskInfoProtocolTemplate = {
  EFI_DISK_INFO_USB_INTERFACE_GUID,
  UsbDiskInfoInquiry,
  UsbDiskInfoIdentify,
  UsbDiskInfoSenseData,
  UsbDiskInfoWhichIde
};

/**
  Initialize the installation of DiskInfo protocol.

  This function prepares for the installation of DiskInfo protocol on the child handle.
  By default, it installs DiskInfo protocol with USB interface GUID. 

  @param[in]  UsbMass  The pointer of USB_MASS_DEVICE.

**/
VOID
InitializeDiskInfo (
  IN  USB_MASS_DEVICE   *UsbMass
  )
{
  CopyMem (&UsbMass->DiskInfo, &gUsbDiskInfoProtocolTemplate, sizeof (gUsbDiskInfoProtocolTemplate));
}


/**
  Provides inquiry information for the controller type.
  
  This function is used to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData       Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize   Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class 
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough 

**/
EFI_STATUS
EFIAPI
UsbDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  )
{
  EFI_STATUS        Status;
  USB_MASS_DEVICE   *UsbMass;

  UsbMass  = USB_MASS_DEVICE_FROM_DISK_INFO (This);

  Status = EFI_BUFFER_TOO_SMALL;
  if (*InquiryDataSize >= sizeof (UsbMass->InquiryData)) {
    Status = EFI_SUCCESS;
    CopyMem (InquiryData, &UsbMass->InquiryData, sizeof (UsbMass->InquiryData));
  }
  *InquiryDataSize = sizeof (UsbMass->InquiryData);
  return Status;
}


/**
  Provides identify information for the controller type.

  This function is used to get identify data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL 
                                    instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class 
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device 
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough 

**/
EFI_STATUS
EFIAPI
UsbDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  )
{
  return EFI_NOT_FOUND;
}

/**
  Provides sense data information for the controller type.
  
  This function is used to get sense data. 
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData         Pointer to the SenseData.
  @param[in, out] SenseDataSize     Size of SenseData in bytes.
  @param[out]     SenseDataNumber   Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
UsbDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT    UINT8                    *SenseDataNumber
  )
{
  return EFI_NOT_FOUND;
}


/**
  This function is used to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
UsbDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL   *This,
  OUT UINT32                   *IdeChannel,
  OUT UINT32                   *IdeDevice
  )
{
  return EFI_UNSUPPORTED;
}

