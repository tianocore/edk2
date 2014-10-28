/** @file
  Declarations and macros for the console abstraction.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Depends on <kfile.h>, <Device/Device.h>, <Protocol/SimpleTextIn.h>, <Uefi/UefiBaseType.h>
**/
#ifndef _DEVICE_UEFI_CONSOLE_H
#define _DEVICE_UEFI_CONSOLE_H

#include  <kfile.h>
#include  <Device/Device.h>

/*  The members Cookie through Abstraction, inclusive, are the same type and order
    for all instance structures.

    All instance structures must be a multiple of sizeof(UINTN) bytes long
*/
typedef struct {
  UINT32                      Cookie;       ///< Special value identifying this as a valid ConInstance
  UINT32                      InstanceNum;  ///< Which instance is this?  Zero-based.
  EFI_HANDLE                  Dev;          ///< Pointer to either Input or Output Protocol.
  DeviceNode                 *Parent;       ///< Points to the parent Device Node.
  struct fileops              Abstraction;  ///< Pointers to functions implementing this device's abstraction.
  UINTN                       Reserved_1;   // Ensure that next member starts on an 8-byte boundary
  UINT64                      NumRead;      ///< Number of characters Read.
  UINT64                      NumWritten;   ///< Number of characters Written.
  __mbstate_t                 CharState;    ///< Character state for the byte stream passing through this device
  CHAR16                      UnGetKey;     ///< One-key pushback, for poll().
} ConInstance;

__BEGIN_DECLS

int
EFIAPI
da_ConOpen(
  IN  DeviceNode         *DevNode,
  IN  struct __filedes   *filp,
  IN  int                 DevInstance,
  IN  CHAR16             *Path,
  IN  CHAR16             *MPath
);

__END_DECLS

#endif  /* _DEVICE_UEFI_CONSOLE_H */
