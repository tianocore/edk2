/** @file
  The device path protocol as defined in EFI 1.0.

  The device path represents a programatic path to a device. It's the view
  from a software point of view. It also must persist from boot to boot, so 
  it can not contain things like PCI bus numbers that change from boot to boot.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  DevicePath.h

**/

#ifndef __EFI_DEVICE_PATH_PROTOCOL_H__
#define __EFI_DEVICE_PATH_PROTOCOL_H__

//
// Device Path protocol
//
#define EFI_DEVICE_PATH_PROTOCOL_GUID \
  { \
    0x9576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#pragma pack(1)

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

#pragma pack()
                                             
#define EFI_DP_TYPE_MASK                     0x7F
#define EFI_DP_TYPE_UNPACKED                 0x80
#define END_DEVICE_PATH_TYPE                 0x7f
                                             
#define EFI_END_ENTIRE_DEVICE_PATH           0xff
#define EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE   0xff
#define EFI_END_INSTANCE_DEVICE_PATH         0x01
#define END_ENTIRE_DEVICE_PATH_SUBTYPE       EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE
#define END_INSTANCE_DEVICE_PATH_SUBTYPE     EFI_END_INSTANCE_DEVICE_PATH
                                             
#define EFI_END_DEVICE_PATH_LENGTH           (sizeof (EFI_DEVICE_PATH_PROTOCOL))
#define END_DEVICE_PATH_LENGTH               EFI_END_DEVICE_PATH_LENGTH
                                             
#define DP_IS_END_TYPE(a)                    
#define DP_IS_END_SUBTYPE(a)                 (((a)->SubType == END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define DevicePathSubType(a)                 ((a)->SubType)
#define IsDevicePathUnpacked(a)              ((a)->Type & EFI_DP_TYPE_UNPACKED)
                                             
#define EfiDevicePathNodeLength(a)           (((a)->Length[0]) | ((a)->Length[1] << 8))
#define DevicePathNodeLength(a)              (EfiDevicePathNodeLength(a))
#define EfiNextDevicePathNode(a)             ((EFI_DEVICE_PATH_PROTOCOL *) (((UINT8 *) (a)) + EfiDevicePathNodeLength (a)))
#define NextDevicePathNode(a)                (EfiNextDevicePathNode(a)) 
                                             
#define EfiDevicePathType(a)                 (((a)->Type) & EFI_DP_TYPE_MASK)
#define DevicePathType(a)                    (EfiDevicePathType(a))
#define EfiIsDevicePathEndType(a)            (EfiDevicePathType (a) == END_DEVICE_PATH_TYPE)
#define IsDevicePathEndType(a)               (EfiIsDevicePathEndType(a)) 
                                             
                                             
#define EfiIsDevicePathEndSubType(a)         ((a)->SubType == EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE)
#define IsDevicePathEndSubType(a)            (EfiIsDevicePathEndSubType(a))
#define EfiIsDevicePathEndInstanceSubType(a) ((a)->SubType == EFI_END_INSTANCE_DEVICE_PATH)
                                             
#define EfiIsDevicePathEnd(a)                (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndSubType (a))
#define IsDevicePathEnd(a)                   (EfiIsDevicePathEnd(a))
#define EfiIsDevicePathEndInstance(a)        (EfiIsDevicePathEndType (a) && EfiIsDevicePathEndInstanceSubType (a))


#define SetDevicePathNodeLength(a,l) {                           \
          (a)->Length[0] = (UINT8) (l);                          \
          (a)->Length[1] = (UINT8) ((l) >> 8);                   \
          }

#define SetDevicePathEndNode(a)  {                               \
          (a)->Type = END_DEVICE_PATH_TYPE;                      \
          (a)->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;         \
          (a)->Length[0] = sizeof(EFI_DEVICE_PATH_PROTOCOL);     \
          (a)->Length[1] = 0;                                    \
          }

extern EFI_GUID gEfiDevicePathProtocolGuid;

#endif
