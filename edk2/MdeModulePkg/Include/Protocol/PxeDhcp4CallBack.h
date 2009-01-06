/** @file
  EFI PXE DHCP4 Callback protocol definition.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PXE_DHCP4CALLBACK_H__
#define __PXE_DHCP4CALLBACK_H__

#include <Protocol/PxeDhcp4.h>

#define EFI_PXE_DHCP4_CALLBACK_PROTOCOL_GUID \
{ 0xc1544c01, 0x92a4, 0x4198, {0x8a, 0x84, 0x77, 0x85, 0x83, 0xc2, 0x36, 0x21 } }

#define EFI_PXE_DHCP4_CALLBACK_INTERFACE_REVISION   0x00010000

typedef struct _EFI_PXE_DHCP4_CALLBACK_PROTOCOL EFI_PXE_DHCP4_CALLBACK_PROTOCOL;

typedef enum {
  EFI_PXE_DHCP4_FUNCTION_FIRST,
  EFI_PXE_DHCP4_FUNCTION_INIT,
  EFI_PXE_DHCP4_FUNCTION_SELECT,
  EFI_PXE_DHCP4_FUNCTION_RENEW,
  EFI_PXE_DHCP4_FUNCTION_REBIND,
  EFI_PXE_DHCP4_FUNCTION_LAST
} EFI_PXE_DHCP4_FUNCTION;

typedef enum {
  EFI_PXE_DHCP4_CALLBACK_STATUS_FIRST,
  EFI_PXE_DHCP4_CALLBACK_STATUS_ABORT,
  EFI_PXE_DHCP4_CALLBACK_STATUS_IGNORE_ABORT,
  EFI_PXE_DHCP4_CALLBACK_STATUS_KEEP_ABORT,
  EFI_PXE_DHCP4_CALLBACK_STATUS_CONTINUE,
  EFI_PXE_DHCP4_CALLBACK_STATUS_IGNORE_CONTINUE,
  EFI_PXE_DHCP4_CALLBACK_STATUS_KEEP_CONTINUE,
  EFI_PXE_DHCP4_CALLBACK_STATUS_LAST
} EFI_PXE_DHCP4_CALLBACK_STATUS;

typedef
EFI_PXE_DHCP4_CALLBACK_STATUS
(EFIAPI *EFI_PXE_DHCP4_CALLBACK)(
  IN EFI_PXE_DHCP4_PROTOCOL *This,
  IN EFI_PXE_DHCP4_FUNCTION Function,
  IN UINT32                 PacketLen,
  IN DHCP4_PACKET           *Packet OPTIONAL
  );

struct _EFI_PXE_DHCP4_CALLBACK_PROTOCOL {
  UINT64                      Revision;
  EFI_PXE_DHCP4_CALLBACK      Callback;
};

extern EFI_GUID gEfiPxeDhcp4CallbackProtocolGuid;

#endif

