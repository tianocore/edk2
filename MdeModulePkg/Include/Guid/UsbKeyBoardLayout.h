/** @file
  USB KeyBoard Layout GUIDs

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __USB_KEYBOARD_LAYOUT_GUID_H__
#define __USB_KEYBOARD_LAYOUT_GUID_H__

//
// GUID for USB keyboard HII package list.
//
#define USB_KEYBOARD_LAYOUT_PACKAGE_GUID \
  { \
    0xc0f3b43, 0x44de, 0x4907, { 0xb4, 0x78, 0x22, 0x5f, 0x6f, 0x62, 0x89, 0xdc } \
  }

//
// GUID for USB keyboard layout
//
#define USB_KEYBOARD_LAYOUT_KEY_GUID \
  { \
    0x3a4d7a7c, 0x18a, 0x4b42, { 0x81, 0xb3, 0xdc, 0x10, 0xe3, 0xb5, 0x91, 0xbd } \
  }

extern EFI_GUID  gUsbKeyboardLayoutPackageGuid;
extern EFI_GUID  gUsbKeyboardLayoutKeyGuid;

#endif
