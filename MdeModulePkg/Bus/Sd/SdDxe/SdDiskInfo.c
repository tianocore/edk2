/** @file
  Implement the EFI_DISK_INFO_PROTOCOL interface on SD memory card devices.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SdDxe.h"

/**
  Provides inquiry information for the controller type.

  This function is used by the driver entity to get inquiry data. Data format of
  Identify data is defined by the Interface GUID.

  @param[in]     This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in,out] InquiryData       Pointer to a buffer for the inquiry data.
  @param[in,out] InquiryDataSize   Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device.
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough.

**/
EFI_STATUS
EFIAPI
SdDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *InquiryData,
  IN OUT UINT32                  *InquiryDataSize
  )
{
  EFI_STATUS    Status;
  SD_DEVICE     *Device;

  Device = SD_DEVICE_DATA_FROM_DISKINFO (This);

  if (*InquiryDataSize >= sizeof (Device->Cid)) {
    Status = EFI_SUCCESS;
    CopyMem (InquiryData, &Device->Cid, sizeof (Device->Cid));
  } else {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  *InquiryDataSize = sizeof (Device->Cid);

  return Status;
}

/**
  Provides identify information for the controller type.

  This function is used by the driver entity to get identify data. Data format
  of Identify data is defined by the Interface GUID.

  @param[in]     This               Pointer to the EFI_DISK_INFO_PROTOCOL
                                    instance.
  @param[in,out] IdentifyData       Pointer to a buffer for the identify data.
  @param[in,out] IdentifyDataSize   Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device.
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough.

**/
EFI_STATUS
EFIAPI
SdDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *IdentifyData,
  IN OUT UINT32                  *IdentifyDataSize
  )
{
  return EFI_NOT_FOUND;
}

/**
  Provides sense data information for the controller type.

  This function is used by the driver entity to get sense data. Data format of
  Sense data is defined by the Interface GUID.

  @param[in]     This              Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in,out] SenseData         Pointer to the SenseData.
  @param[in,out] SenseDataSize     Size of SenseData in bytes.
  @param[out]    SenseDataNumber   Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
SdDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL  *This,
  IN OUT VOID                    *SenseData,
  IN OUT UINT32                  *SenseDataSize,
  OUT    UINT8                   *SenseDataNumber
  )
{
  return EFI_NOT_FOUND;
}

/**
  Provides IDE channel and device information for the interface.

  This function is used by the driver entity to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
SdDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL  *This,
  OUT UINT32                  *IdeChannel,
  OUT UINT32                  *IdeDevice
  )
{
  return EFI_UNSUPPORTED;
}
