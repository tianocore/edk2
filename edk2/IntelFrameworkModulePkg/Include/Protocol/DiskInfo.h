/** @file
  Disk Info protocol Provides the basic interfaces to abstract 
  platform information regarding an IDE controller.

Copyright (c) 2006 - 2009, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DISK_INFO_H__
#define __DISK_INFO_H__

#define EFI_DISK_INFO_PROTOCOL_GUID \
  { \
    0xd432a67f, 0x14dc, 0x484b, {0xb3, 0xbb, 0x3f, 0x2, 0x91, 0x84, 0x93, 0x27 } \
  }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_DISK_INFO_PROTOCOL  EFI_DISK_INFO_PROTOCOL;

/**
  This function is used by the IDE bus driver to get inquiry data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  InquiryData           Pointer to a buffer for the inquiry data.
  @param  InquiryDataSize       Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  InquiryDataSize not big enough 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_INFO_INQUIRY)(
  IN EFI_DISK_INFO_PROTOCOL           *This,
  IN OUT VOID                         *InquiryData,
  IN OUT UINT32                       *InquiryDataSize
  );


/**
  This function is used by the IDE bus driver to get identify data. 
  Data format of Identify data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param  IdentifyData          Pointer to a buffer for the identify data.
  @param  IdentifyDataSize      Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading IdentifyData from device 
  @retval EFI_BUFFER_TOO_SMALL  IdentifyDataSize not big enough 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_INFO_IDENTIFY)(
  IN EFI_DISK_INFO_PROTOCOL           *This,
  IN OUT VOID                         *IdentifyData,
  IN OUT UINT32                       *IdentifyDataSize
  );


/**
  This function is used by the IDE bus driver to get sense data. 
  Data format of Sense data is defined by the Interface GUID.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  SenseData             Pointer to the SenseData. 
  @param  SenseDataSize         Size of SenseData in bytes. 
  @param  SenseDataNumber       Pointer to the value for the identify data size.

  @retval EFI_SUCCESS           The command was accepted without any errors.
  @retval EFI_NOT_FOUND         Device does not support this data class 
  @retval EFI_DEVICE_ERROR      Error reading InquiryData from device 
  @retval EFI_BUFFER_TOO_SMALL  SenseDataSize not big enough 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_INFO_SENSE_DATA)(
  IN EFI_DISK_INFO_PROTOCOL           *This,
  IN OUT VOID                         *SenseData,
  IN OUT UINT32                       *SenseDataSize,
  OUT UINT8                           *SenseDataNumber
  );

/**
  This function is used by the IDE bus driver to get controller information.

  @param  This                  Pointer to the EFI_DISK_INFO_PROTOCOL instance. 
  @param  IdeChannel            Pointer to the Ide Channel number. Primary or secondary.
  @param  IdeDevice             Pointer to the Ide Device number. Master or slave.

  @retval EFI_SUCCESS           IdeChannel and IdeDevice are valid 
  @retval EFI_UNSUPPORTED       This is not an IDE device 

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DISK_INFO_WHICH_IDE)(
  IN EFI_DISK_INFO_PROTOCOL           *This,
  OUT UINT32                          *IdeChannel,
  OUT UINT32                          *IdeDevice
  );

//
// GUID of the type of interfaces
//
#define EFI_DISK_INFO_IDE_INTERFACE_GUID \
  { \
    0x5e948fe3, 0x26d3, 0x42b5, {0xaf, 0x17, 0x61, 0x2, 0x87, 0x18, 0x8d, 0xec } \
  }
extern EFI_GUID gEfiDiskInfoIdeInterfaceGuid;

#define EFI_DISK_INFO_SCSI_INTERFACE_GUID \
  { \
    0x8f74baa, 0xea36, 0x41d9, {0x95, 0x21, 0x21, 0xa7, 0xf, 0x87, 0x80, 0xbc } \
  }
extern EFI_GUID gEfiDiskInfoScsiInterfaceGuid;

#define EFI_DISK_INFO_USB_INTERFACE_GUID \
  { \
    0xcb871572, 0xc11a, 0x47b5, {0xb4, 0x92, 0x67, 0x5e, 0xaf, 0xa7, 0x77, 0x27 } \
  }
extern EFI_GUID gEfiDiskInfoUsbInterfaceGuid;

#define EFI_DISK_INFO_AHCI_INTERFACE_GUID \
  { \
    0x9e498932, 0x4abc, 0x45af, {0xa3, 0x4d, 0x2, 0x47, 0x78, 0x7b, 0xe7, 0xc6} \
  }
extern EFI_GUID gEfiDiskInfoAhciInterfaceGuid;

struct _EFI_DISK_INFO_PROTOCOL {
  ///
  /// A GUID that defines the format of buffers for the other member functions of this protocol.
  ///
  EFI_GUID                  Interface;
  EFI_DISK_INFO_INQUIRY     Inquiry;
  EFI_DISK_INFO_IDENTIFY    Identify;
  EFI_DISK_INFO_SENSE_DATA  SenseData;
  EFI_DISK_INFO_WHICH_IDE   WhichIde;
};

extern EFI_GUID gEfiDiskInfoProtocolGuid;

#endif


