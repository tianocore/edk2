/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    DriverSupportedEfiVersion.h

Abstract:

    Driver Supported EFI Version Protocol

Revision History:

--*/

#ifndef _EFI_DRIVER_SUPPORTED_EFI_VERSION_H_
#define _EFI_DRIVER_SUPPORTED_EFI_VERSION_H_

//
// Global ID for the Driver Supported EFI Version Protocol
//
#define EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL_GUID \
  { \
    0x5c198761, 0x16a8, 0x4e69, {0x97, 0x2c, 0x89, 0xd6, 0x79, 0x54, 0xf8, 0x1d} \
  }

EFI_FORWARD_DECLARATION (EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL);

//
// Interface structure for the Driver Supported EFI Version Protocol
//
struct _EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL {
  UINT32    Length;
  UINT32    FirmwareVersion;
};

extern EFI_GUID gEfiDriverSupportedEfiVersionProtocolGuid;

#endif
