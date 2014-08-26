/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EMBEDDED_EXTERNAL_DEVICE_H__
#define __EMBEDDED_EXTERNAL_DEVICE_H__

//
// Protocol GUID
//
#define EMBEDDED_EXTERNAL_DEVICE_PROTOCOL_GUID { 0x735F8C64, 0xD696, 0x44D0, { 0xBD, 0xF2, 0x44, 0x7F, 0xD0, 0x5A, 0x54, 0x06 }}

//
// Protocol interface structure
//
typedef struct _EMBEDDED_EXTERNAL_DEVICE EMBEDDED_EXTERNAL_DEVICE;

//
// Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_EXTERNAL_DEVICE_READ) (
    IN  EMBEDDED_EXTERNAL_DEVICE  *This,
    IN  UINTN                       Register,
    IN  UINTN                       Length,
    OUT VOID                        *Buffer
    )
/*++

Routine Description:

  Read a set of contiguous external device registers.

Arguments:

  This        - pointer to protocol
  Offset      - starting register number
  Length      - number of bytes to read
  Buffer      - destination buffer

Returns:

  EFI_SUCCESS - registers read successfully

--*/
;

typedef
EFI_STATUS
(EFIAPI *EMBEDDED_EXTERNAL_DEVICE_WRITE) (
    IN EMBEDDED_EXTERNAL_DEVICE *This,
    IN UINTN                      Register,
    IN UINTN                      Length,
    IN VOID                       *Buffer
    )
/*++

Routine Description:

  Write to a set of contiguous external device registers.

Arguments:

  This        - pointer to protocol
  Offset      - starting register number
  Length      - number of bytes to write
  Buffer      - source buffer

Returns:

  EFI_SUCCESS - registers written successfully

--*/
;

struct _EMBEDDED_EXTERNAL_DEVICE {
  EMBEDDED_EXTERNAL_DEVICE_READ      Read;
  EMBEDDED_EXTERNAL_DEVICE_WRITE     Write;
};

extern EFI_GUID gEmbeddedExternalDeviceProtocolGuid;

#endif  // __EMBEDDED_EXTERNAL_DEVICE_H__
