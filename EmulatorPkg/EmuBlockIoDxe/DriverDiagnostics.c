/**@file

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  DriverDiagnostics.c

Abstract:

**/

#include "EmuBlockIo.h"

//
// EFI Driver Diagnostics Functions
//
EFI_STATUS
EFIAPI
EmuBlockIoDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL  *This,
  IN  EFI_HANDLE                       ControllerHandle,
  IN  EFI_HANDLE                       ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE       DiagnosticType,
  IN  CHAR8                            *Language,
  OUT EFI_GUID                         **ErrorType,
  OUT UINTN                            *BufferSize,
  OUT CHAR16                           **Buffer
  );

//
// EFI Driver Diagnostics Protocol
//
EFI_DRIVER_DIAGNOSTICS_PROTOCOL  gEmuBlockIoDriverDiagnostics = {
  EmuBlockIoDriverDiagnosticsRunDiagnostics,
  "eng"
};

//
// EFI Driver Diagnostics 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_DRIVER_DIAGNOSTICS2_PROTOCOL  gEmuBlockIoDriverDiagnostics2 = {
  (EFI_DRIVER_DIAGNOSTICS2_RUN_DIAGNOSTICS)EmuBlockIoDriverDiagnosticsRunDiagnostics,
  "en"
};

EFI_STATUS
EFIAPI
EmuBlockIoDriverDiagnosticsRunDiagnostics (
  IN  EFI_DRIVER_DIAGNOSTICS_PROTOCOL  *This,
  IN  EFI_HANDLE                       ControllerHandle,
  IN  EFI_HANDLE                       ChildHandle  OPTIONAL,
  IN  EFI_DRIVER_DIAGNOSTIC_TYPE       DiagnosticType,
  IN  CHAR8                            *Language,
  OUT EFI_GUID                         **ErrorType,
  OUT UINTN                            *BufferSize,
  OUT CHAR16                           **Buffer
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
                       identifier or a Null-terminated ASCII string array indicating
                       the language.  This is the language in which the optional
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
  EFI_STATUS             Status;
  EFI_BLOCK_IO_PROTOCOL  *BlockIo;
  CHAR8                  *SupportedLanguages;
  BOOLEAN                Iso639Language;
  BOOLEAN                Found;
  UINTN                  Index;

  if ((Language         == NULL) ||
      (ErrorType        == NULL) ||
      (Buffer           == NULL) ||
      (ControllerHandle == NULL) ||
      (BufferSize       == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SupportedLanguages = This->SupportedLanguages;
  Iso639Language     = (BOOLEAN)(This == &gEmuBlockIoDriverDiagnostics);
  //
  // Make sure Language is in the set of Supported Languages
  //
  Found = FALSE;
  while (*SupportedLanguages != 0) {
    if (Iso639Language) {
      if (CompareMem (Language, SupportedLanguages, 3) == 0) {
        Found = TRUE;
        break;
      }

      SupportedLanguages += 3;
    } else {
      for (Index = 0; SupportedLanguages[Index] != 0 && SupportedLanguages[Index] != ';'; Index++) {
      }

      if ((AsciiStrnCmp (SupportedLanguages, Language, Index) == 0) && (Language[Index] == 0)) {
        Found = TRUE;
        break;
      }

      SupportedLanguages += Index;
      for ( ; *SupportedLanguages != 0 && *SupportedLanguages == ';'; SupportedLanguages++) {
      }
    }
  }

  //
  // If Language is not a member of SupportedLanguages, then return EFI_UNSUPPORTED
  //
  if (!Found) {
    return EFI_UNSUPPORTED;
  }

  *ErrorType  = NULL;
  *BufferSize = 0;
  if (DiagnosticType != EfiDriverDiagnosticTypeStandard) {
    *ErrorType  = &gEfiBlockIoProtocolGuid;
    *BufferSize = 0x60;
    Buffer      = AllocatePool ((UINTN)(*BufferSize));
    CopyMem (*Buffer, L"Windows Block I/O Driver Diagnostics Failed\n", *BufferSize);
    return EFI_DEVICE_ERROR;
  }

  //
  // This is a device driver, so ChildHandle must be NULL.
  //
  if (ChildHandle != NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Validate controller handle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEmuIoThunkProtocolGuid,
                  (VOID **)&BlockIo,
                  gEmuBlockIoDriverBinding.DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEmuIoThunkProtocolGuid,
           gEmuBlockIoDriverBinding.DriverBindingHandle,
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
