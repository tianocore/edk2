/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    DriverDiagnostics2.h

Abstract:

    EFI Driver Diagnostics2 Protocol

Revision History

--*/

#ifndef _EFI_DRIVER_DIAGNOSTICS2_H_
#define _EFI_DRIVER_DIAGNOSTICS2_H_

#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics)

//
// Global ID for the Driver Diagnostics Protocol
//
#define EFI_DRIVER_DIAGNOSTICS2_PROTOCOL_GUID \
  { \
    0x4d330321, 0x025f, 0x4aac, {0x90, 0xd8, 0x5e, 0xd9, 0x0, 0x17, 0x3b, 0x63} \
  }

EFI_FORWARD_DECLARATION (EFI_DRIVER_DIAGNOSTICS2_PROTOCOL);

typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS) (
  IN EFI_DRIVER_DIAGNOSTICS2_PROTOCOL                       * This,
  IN  EFI_HANDLE                                            ControllerHandle,
  IN  EFI_HANDLE                                            ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                            DiagnosticType,
  IN  CHAR8                                                 *Language,
  OUT EFI_GUID                                              **ErrorType,
  OUT UINTN                                                 *BufferSize,
  OUT CHAR16                                                **Buffer
  );

/*++

  Routine Description:
    Runs diagnostics on a controller.

  Arguments:
    This             - A pointer to the EFI_DRIVER_DIAGNOSTICS2_PROTOCOL instance.
    ControllerHandle - The handle of the controller to run diagnostics on.
    ChildHandle      - The handle of the child controller to run diagnostics on
                       This is an optional parameter that may be NULL.  It will
                       be NULL for device drivers.  It will also be NULL for a
                       bus drivers that wish to run diagnostics on the bus
                       controller.  It will not be NULL for a bus driver that
                       wishes to run diagnostics on one of its child controllers.
    DiagnosticType   - Indicates type of diagnostics to perform on the controller
                       specified by ControllerHandle and ChildHandle.   See
                       "Related Definitions" for the list of supported types.
    Language         - A pointer to a NULL-terminated ASCII string array indicating
                       the language. This is the language in which the optional
                       error message should be returned in Buffer, and it must
                       match one of the languages specified in SupportedLanguages.
                       The number of languages supported by a driver is up to
                       the driver writer. Language is specified in RFC 3066
                       language code format.
    ErrorType        - A GUID that defines the format of the data returned in
                       Buffer.
    BufferSize       - The size, in bytes, of the data returned in Buffer.
    Buffer           - A buffer that contains a Null-terminated Unicode string
                       plus some additional data whose format is defined by
                       ErrorType.  Buffer is allocated by this function with
                       AllocatePool(), and it is the caller's responsibility
                       to free it with a call to FreePool().

  Returns:
    EFI_SUCCESS           - The controller specified by ControllerHandle and
                            ChildHandle passed the diagnostic.
    EFI_INVALID_PARAMETER - ControllerHandle is not a valid EFI_HANDLE.
    EFI_INVALID_PARAMETER - ChildHandle is not NULL and it is not a valid
                            EFI_HANDLE.
    EFI_INVALID_PARAMETER - Language is NULL.
    EFI_INVALID_PARAMETER - ErrorType is NULL.
    EFI_INVALID_PARAMETER - BufferType is NULL.
    EFI_INVALID_PARAMETER - Buffer is NULL.
    EFI_UNSUPPORTED       - The driver specified by This does not support
                            running diagnostics for the controller specified
                            by ControllerHandle and ChildHandle.
    EFI_UNSUPPORTED       - The driver specified by This does not support the
                            type of diagnostic specified by DiagnosticType.
    EFI_UNSUPPORTED       - The driver specified by This does not support the
                            language specified by Language.
    EFI_OUT_OF_RESOURCES  - There are not enough resources available to complete
                            the diagnostics.
    EFI_OUT_OF_RESOURCES  - There are not enough resources available to return
                            the status information in ErrorType, BufferSize,
                            and Buffer.
    EFI_DEVICE_ERROR      - The controller specified by ControllerHandle and
                            ChildHandle did not pass the diagnostic.

--*/

//
// Interface structure for the Driver Diagnostics Protocol
//
struct _EFI_DRIVER_DIAGNOSTICS2_PROTOCOL {
  EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS  RunDiagnostics;
  CHAR8                                    *SupportedLanguages;
};

/*++

  Protocol Description:
    Used to perform diagnostics on a controller that an EFI Driver is managing.

  Parameters:
    RunDiagnostics     - Runs diagnostics on a controller.
    SupportedLanguages - A Null-terminated ASCII string that contains one or more
                         RFC 3066 language codes.  This is the list of language
                         codes that this protocol supports.

--*/

extern EFI_GUID gEfiDriverDiagnostics2ProtocolGuid;

#endif
