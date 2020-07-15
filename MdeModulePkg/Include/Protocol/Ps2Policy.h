/** @file
  PS/2 policy protocol abstracts the specific platform initialization and settings.

Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _PS2_POLICY_PROTOCOL_H_
#define _PS2_POLICY_PROTOCOL_H_

#define EFI_PS2_POLICY_PROTOCOL_GUID \
  { \
    0x4df19259, 0xdc71, 0x4d46, {0xbe, 0xf1, 0x35, 0x7b, 0xb5, 0x78, 0xc4, 0x18 } \
  }

#define EFI_KEYBOARD_CAPSLOCK   0x0004
#define EFI_KEYBOARD_NUMLOCK    0x0002
#define EFI_KEYBOARD_SCROLLLOCK 0x0001

typedef
EFI_STATUS
(EFIAPI *EFI_PS2_INIT_HARDWARE) (
  IN  EFI_HANDLE              Handle
  );

typedef struct {
  UINT8                 KeyboardLight;
  EFI_PS2_INIT_HARDWARE Ps2InitHardware;
} EFI_PS2_POLICY_PROTOCOL;

extern EFI_GUID gEfiPs2PolicyProtocolGuid;

#endif
