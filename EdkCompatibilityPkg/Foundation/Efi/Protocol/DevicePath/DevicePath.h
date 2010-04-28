/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DevicePath.h

Abstract:

  The device path protocol as defined in EFI 1.0.

  The device path represents a programatic path to a device. It's the view
  from a software point of view. It also must persist from boot to boot, so 
  it can not contain things like PCI bus numbers that change from boot to boot.
  

--*/

#ifndef _DEVICE_PATH_H_
#define _DEVICE_PATH_H_

//
// Device Path protocol
//
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
  { \
    0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b} \
  }

#pragma pack(1)

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

#pragma pack()

#define EFI_END_ENTIRE_DEVICE_PATH            0xff
#define EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE    0xff
#define EFI_END_INSTANCE_DEVICE_PATH          0x01
#define EFI_END_DEVICE_PATH_LENGTH            (sizeof (EFI_DEVICE_PATH_PROTOCOL))

#define EfiDevicePathNodeLength(a)            (((a)->Length[0]) | ((a)->Length[1] << 8))
#define EfiNextDevicePathNode(a)              ((EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) (a)) + EfiDevicePathNodeLength (a)))

#define EfiDevicePathType(a)                  (((a)->Type) & 0x7f)
#define EfiIsDevicePathEndType(a)             (EfiDevicePathType (a) == 0x7f)

#define EfiIsDevicePathEndSubType(a)          ((a)->SubType == EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define EfiIsDevicePathEndInstanceSubType(a)  ((a)->SubType == EFI_END_INSTANCE_DEVICE_PATH)

#define EfiIsDevicePathEnd(a)                 (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndSubType (a))
#define EfiIsDevicePathEndInstance(a)         (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndInstanceSubType (a))

extern EFI_GUID gEfiDevicePathProtocolGuid;

#endif
