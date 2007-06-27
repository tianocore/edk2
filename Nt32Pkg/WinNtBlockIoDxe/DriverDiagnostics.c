/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  DriverDiagnostics.c

Abstract:

--*/
#include <Uefi.h>
#include <WinNtDxe.h>
#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>

#include "WinNtBlockIo.h"

//
// EFI Driver Diagnostics Functions
//
EFI_STATUS
EFIAPI
WinNtBlockIoDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL               *This,
  IN  EFI_HANDLE                                    ControllerHandle,
  IN  EFI_HANDLE                                    ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                    DiagnosticType,
  IN  CHAR8                                         *Language,
  OUT EFI_GUID                                      **ErrorType,
  OUT UINTN                                         *BufferSize,
  OUT CHAR16                                        **Buffer
  );

//
// EFI Driver Diagnostics Protocol
//
EFI_DRIVER_DIAGNOSTICS_PROTOCOL gWinNtBlockIoDriverDiagnostics = {
  WinNtBlockIoDriverDiagnosticsRunDiagnostics,
  LANGUAGESUPPORTED
};

EFI_STATUS
EFIAPI
WinNtBlockIoDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL               *This,
  IN  EFI_HANDLE                                    ControllerHandle,
  IN  EFI_HANDLE                                    ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE                    DiagnosticType,
  IN  CHAR8                                         *Language,
  OUT EFI_GUID                                      **ErrorType,
  OUT UINTN                                         *BufferSize,
  OUT CHAR16                                        **Buffer
  )
/*++

  Routine Description:
    Runs diagnostics on a controller.

  Arguments:
    This             - A pointer to the EFI_DRIVER_DIAGNOSTICS_PROTOCOL instance.
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
    Language         - A pointer to a three character ISO 639-2 language
                       identifier.  This is the language in which the optional
                       error message should be returned in Buffer, and it must
                       match one of the languages specified in SupportedLanguages.
                       The number of languages supported by a driver is up to
                       the driver writer.
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
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  CHAR8                 *SupportedLanguage;

  if (Language         == NULL ||
      ErrorType        == NULL ||
      Buffer           == NULL ||
      ControllerHandle == NULL ||
      BufferSize       == NULL) {

    return EFI_INVALID_PARAMETER;
  }

  SupportedLanguage = This->SupportedLanguages;

  Status            = EFI_UNSUPPORTED;
  while (*SupportedLanguage != 0) {
    if (AsciiStrnCmp (Language, SupportedLanguage, 3) == 0) {
      Status = EFI_SUCCESS;
      break;
    }

    SupportedLanguage += 3;
  }

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  *ErrorType  = NULL;
  *BufferSize = 0;
  if (DiagnosticType != EfiDriverDiagnosticTypeStandard) {
    *ErrorType  = &gEfiBlockIoProtocolGuid;
    *BufferSize = 0x60;
    Buffer = AllocatePool ((UINTN) (*BufferSize));
    CopyMem (*Buffer, L"Windows Block I/O Driver Diagnostics Failed\n", *BufferSize);
    return EFI_DEVICE_ERROR;
  }

  //
  // Validate controller handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWinNtIoProtocolGuid,
                  &BlockIo,
                  gWinNtBlockIoDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiWinNtIoProtocolGuid,
          gWinNtBlockIoDriverBinding.DriverBindingHandle,
          ControllerHandle
          );

    return EFI_UNSUPPORTED;
  }

  if (Status == EFI_UNSUPPORTED) {
    return Status;
  } else if (Status != EFI_ALREADY_STARTED) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
