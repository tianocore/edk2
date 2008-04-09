/** @file

  Capsule Architectural Protocol is added to produce UEFI2.0 capsule runtime services.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARCH_PROTOCOL_CAPSULE_ARCH_H__
#define __ARCH_PROTOCOL_CAPSULE_ARCH_H__

//
// Global ID for the Capsule Architectural Protocol
//
#define EFI_CAPSULE_ARCH_PROTOCOL_GUID \
  { 0x5053697e, 0x2ebc, 0x4819, {0x90, 0xd9, 0x05, 0x80, 0xde, 0xee, 0x57, 0x54 }}

extern EFI_GUID gEfiCapsuleArchProtocolGuid;

#endif
