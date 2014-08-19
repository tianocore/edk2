/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VIRTUAL_UNCACHED_PAGES_ROTOCOL_H__
#define __VIRTUAL_UNCACHED_PAGES_ROTOCOL_H__

//
// Protocol GUID
//
#define VIRTUAL_UNCACHED_PAGES_PROTOCOL_GUID { 0xAD651C7D, 0x3C22, 0x4DBF, { 0x92, 0xe8, 0x38, 0xa7, 0xcd, 0xae, 0x87, 0xb2 } }



//
// Protocol interface structure
//
typedef struct _VIRTUAL_UNCACHED_PAGES_PROTOCOL  VIRTUAL_UNCACHED_PAGES_PROTOCOL;


typedef
EFI_STATUS
(EFIAPI *CONVERT_PAGES_TO_UNCACHED_VIRTUAL_ADDRESS) (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  OUT UINT64                            *Attributes     OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *FREE_CONVERTED_PAGES) (
  IN  VIRTUAL_UNCACHED_PAGES_PROTOCOL   *This,
  IN  EFI_PHYSICAL_ADDRESS              Address,
  IN  UINTN                             Length,
  IN  EFI_PHYSICAL_ADDRESS              VirtualMask,
  IN  UINT64                            Attributes
  );



struct _VIRTUAL_UNCACHED_PAGES_PROTOCOL {
  CONVERT_PAGES_TO_UNCACHED_VIRTUAL_ADDRESS  ConvertPages;
  FREE_CONVERTED_PAGES                       RevertPages;
};

extern EFI_GUID gVirtualUncachedPagesProtocolGuid;

#endif
