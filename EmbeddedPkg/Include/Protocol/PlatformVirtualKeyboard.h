/** @file

  Copyright (c) 2018, Linaro. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PLATFORM_VIRTUAL_KEYBOARD_H__
#define __PLATFORM_VIRTUAL_KEYBOARD_H__

//
// Protocol interface structure
//
typedef struct _PLATFORM_VIRTUAL_KBD_PROTOCOL PLATFORM_VIRTUAL_KBD_PROTOCOL;

typedef struct _VIRTUAL_KBD_KEY VIRTUAL_KBD_KEY;

#define VIRTUAL_KEYBOARD_KEY_SIGNATURE  SIGNATURE_32 ('v', 'k', 'b', 'd')

struct _VIRTUAL_KBD_KEY {
  UINTN            Signature;
  EFI_INPUT_KEY    Key;
};

typedef
EFI_STATUS
(EFIAPI *PLATFORM_VIRTUAL_KBD_REGISTER)(
  IN VOID
  );

typedef
EFI_STATUS
(EFIAPI *PLATFORM_VIRTUAL_KBD_RESET)(
  IN VOID
  );

typedef
BOOLEAN
(EFIAPI *PLATFORM_VIRTUAL_KBD_QUERY)(
  IN VIRTUAL_KBD_KEY                           *VirtualKey
  );

typedef
EFI_STATUS
(EFIAPI *PLATFORM_VIRTUAL_KBD_CLEAR)(
  IN VIRTUAL_KBD_KEY                           *VirtualKey
  );

struct _PLATFORM_VIRTUAL_KBD_PROTOCOL {
  PLATFORM_VIRTUAL_KBD_REGISTER    Register;
  PLATFORM_VIRTUAL_KBD_RESET       Reset;
  PLATFORM_VIRTUAL_KBD_QUERY       Query;
  PLATFORM_VIRTUAL_KBD_CLEAR       Clear;
};

extern EFI_GUID  gPlatformVirtualKeyboardProtocolGuid;

#endif /* __PLATFORM_VIRTUAL_KEYBOARD_H__ */
