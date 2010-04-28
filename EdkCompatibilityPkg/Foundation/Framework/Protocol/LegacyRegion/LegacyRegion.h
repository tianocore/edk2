/*++

Copyright (c) 1999 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  LegacyRegion.h
    
Abstract:

  This protocol manages the legacy memory regions between 0xc0000 - 0xfffff

Revision History

  The EFI Legacy Region Protocol is compliant with CSM spec 0.96.

--*/

#ifndef _EFI_LEGACY_REGION_H_
#define _EFI_LEGACY_REGION_H_

#define EFI_LEGACY_REGION_PROTOCOL_GUID \
  { \
    0xfc9013a, 0x568, 0x4ba9, {0x9b, 0x7e, 0xc9, 0xc3, 0x90, 0xa6, 0x60, 0x9b} \
  }

EFI_FORWARD_DECLARATION (EFI_LEGACY_REGION_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_DECODE) (
  IN EFI_LEGACY_REGION_PROTOCOL           * This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  IN  BOOLEAN                             *On
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_LOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           * This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_BOOT_LOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           * This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LEGACY_REGION_UNLOCK) (
  IN EFI_LEGACY_REGION_PROTOCOL           * This,
  IN  UINT32                              Start,
  IN  UINT32                              Length,
  OUT UINT32                              *Granularity OPTIONAL
  );

struct _EFI_LEGACY_REGION_PROTOCOL {
  EFI_LEGACY_REGION_DECODE    Decode;
  EFI_LEGACY_REGION_LOCK      Lock;
  EFI_LEGACY_REGION_BOOT_LOCK BootLock;
  EFI_LEGACY_REGION_UNLOCK    UnLock;
};

extern EFI_GUID gEfiLegacyRegionProtocolGuid;

#endif
