/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  LpcWpce791Policy.h

Abstract:

  Protocol used for WPCE791 Policy definition.

**/

#ifndef _WPCE791_POLICY_PROTOCOL_H_
#define _WPCE791_POLICY_PROTOCOL_H_


#define EFI_WPCE791_POLICY_PROTOCOL_GUID \
  { \
    0xab2bee2f, 0xc1a6, 0x4399, 0x85, 0x3d, 0xc0, 0x7c, 0x77, 0x4f, 0xfd, 0xd \
  }

#define EFI_WPCE791_PS2_KEYBOARD_ENABLE       0x01
#define EFI_WPCE791_PS2_KEYBOARD_DISABLE      0x00

#define EFI_WPCE791_PS2_MOUSE_ENABLE       0x01
#define EFI_WPCE791_PS2_MOUSE_DISABLE      0x00

typedef struct {
  UINT16  Com1               :1;             // 0 = Disable, 1 = Enable
  UINT16  Lpt1               :1;             // 0 = Disable, 1 = Enable
  UINT16  Floppy             :1;             // 0 = Disable, 1 = Enable
  UINT16  FloppyWriteProtect :1;             // 0 = Write Protect, 1 = Write Enable
  UINT16  Port80             :1;             // 0 = Disable, 1 = Enable
  UINT16  CIR                :1;             // CIR enable or disable
  UINT16  Ps2Keyboard        :1;             // 0 = Disable, 1 = Enable
  UINT16  Ps2Mouse           :1;             // 0 = Disable, 1 = Enable
  UINT16  Com2               :1;             // 0 = Disable, 1 = Enable
  UINT16  Dac                :1;             // 0 = Disable, 1 = Enable
  UINT16  Rsvd               :6;
} EFI_WPCE791_DEVICE_ENABLES;

typedef struct _EFI_WPCE791_POLICY_PROTOCOL {
  EFI_WPCE791_DEVICE_ENABLES DeviceEnables;
} EFI_WPCE791_POLICY_PROTOCOL;

extern EFI_GUID gEfiLpcWpce791PolicyProtocolGuid;

#endif
