/*++

Copyright (c) 2008 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    PlatformToDriverConfiguration.h

Abstract:

    UEFI Authentication Info Protocol.

Revision History:

--*/

#ifndef _EFI_AUTHENTICATION_INFO_H_
#define _EFI_AUTHENTICATION_INFO_H_

//
// Global ID for the Authentication Info Protocol
//
#define EFI_AUTHENTICATION_INFO_PROTOCOL_GUID \
  { \
    0x7671d9d0, 0x53db, 0x4173, {0xaa, 0x69, 0x23, 0x27, 0xf2, 0x1f, 0x0b, 0xc7} \
  }

EFI_FORWARD_DECLARATION (EFI_AUTHENTICATION_INFO_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_AUTHENTICATION_INFO_PROTOCOL_GET) (
  IN  EFI_AUTHENTICATION_INFO_PROTOCOL    *This,
  IN  EFI_HANDLE                          ControllerHandle,
  OUT VOID                                **Buffer
  );
/*++

  Routine Description:
    Retrieves the Authentication information associated with a particular
    controller handle.

  Arguments:
    This               - Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL instance.
    ControllerHandle   - Handle to the Controller.
    Buffer             - Pointer to the authentication information. This function
                         is responsible for allocating the buffer and it is the
                         caller's responsibility to free buffer when the caller
                         is finished with buffer.

  Returns:
    EFI_SUCCESS        - Successfully retrieved Authentication information
                         for the given ControllerHandle.
    EFI_NOT_FOUND      - No matching Authentication information found for the
                         given ControllerHandle.
    EFI_DEVICE_ERROR   - The Authentication information could not be retrieved
                         due to a hardware error.

--*/

typedef
EFI_STATUS
(EFIAPI *EFI_AUTHENTICATION_INFO_PROTOCOL_SET) (
  IN  EFI_AUTHENTICATION_INFO_PROTOCOL    *This,
  IN  EFI_HANDLE                          ControllerHandle,
  IN  VOID                                *Buffer
  );
/*++

  Routine Description:
    Set the Authentication information for a given controller handle.

  Arguments:
    This               - Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL instance.
    ControllerHandle   - Handle to the Controller.
    Buffer             - Pointer to the authentication information.

  Returns:
    EFI_SUCCESS          - Successfully set the Authentication node information
                           for the given ControllerHandle.
    EFI_UNSUPPORTED      - If the platform policies do not allow setting of the
                           Authentication information.
    EFI_DEVICE_ERROR     - The authentication node information could not be configured
                           due to a hardware error.
    EFI_OUT_OF_RESOURCES - Not enough storage is available to hold the data.

--*/

//
// Interface structure for the Authentication Info Protocol
//
struct _EFI_AUTHENTICATION_INFO_PROTOCOL {
  EFI_AUTHENTICATION_INFO_PROTOCOL_GET   Get;
  EFI_AUTHENTICATION_INFO_PROTOCOL_SET   Set;
};

extern EFI_GUID gEfiAuthenticationInfoProtocolGuid;

#endif
