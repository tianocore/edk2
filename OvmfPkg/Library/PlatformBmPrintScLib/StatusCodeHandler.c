/** @file
  Register a status code handler for printing the Boot Manager's LoadImage()
  and StartImage() preparations, and return codes, to the UEFI console.

  This feature enables users that are not accustomed to analyzing the firmware
  log to glean some information about UEFI boot option processing (loading and
  starting).

  This library instance filters out (ignores) status codes that are not
  reported by the containing firmware module. The intent is to link this
  library instance into BdsDxe via PlatformBootManagerLib (which BdsDxe depends
  upon), then catch only those status codes that BdsDxe reports (which happens
  via UefiBootManagerLib). Status codes reported by other modules (such as
  UiApp), via UefiBootManagerLib or otherwise, are meant to be ignored.

  Copyright (C) 2019, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/ReportStatusCodeHandler.h>

#include <Guid/GlobalVariable.h>
#include <Guid/StatusCodeDataTypeId.h>

#include <Pi/PiStatusCode.h>

//
// Convenience variables for the status codes that are relevant for LoadImage()
// and StartImage() preparations and return codes.
//
STATIC EFI_STATUS_CODE_VALUE  mLoadPrep;
STATIC EFI_STATUS_CODE_VALUE  mLoadFail;
STATIC EFI_STATUS_CODE_VALUE  mStartPrep;
STATIC EFI_STATUS_CODE_VALUE  mStartFail;

/**
  Handle status codes reported through ReportStatusCodeLib /
  EFI_STATUS_CODE_PROTOCOL.ReportStatusCode(). Format matching status codes to
  the system console.

  The highest TPL at which this handler can be registered with
  EFI_RSC_HANDLER_PROTOCOL.Register() is TPL_CALLBACK. That's because
  HandleStatusCode() uses the UEFI variable services.

  The parameter list of this function precisely matches that of
  EFI_STATUS_CODE_PROTOCOL.ReportStatusCode().

  The return status of this function is ignored by the caller, but the function
  still returns sensible codes:

  @retval EFI_SUCCESS               The status code has been processed; either
                                    as a no-op, due to filtering, or by
                                    formatting it to the system console.

  @retval EFI_INVALID_PARAMETER     Unknown or malformed contents have been
                                    detected in Data.

  @retval EFI_INCOMPATIBLE_VERSION  Unexpected UEFI variable behavior has been
                                    encountered.

  @return                           Error codes propagated from underlying
                                    services.
**/
STATIC
EFI_STATUS
EFIAPI
HandleStatusCode (
  IN EFI_STATUS_CODE_TYPE   CodeType,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN EFI_GUID               *CallerId,
  IN EFI_STATUS_CODE_DATA   *Data
  )
{
  UINTN                         VariableSize;
  UINT16                        BootCurrent;
  EFI_STATUS                    Status;
  CHAR16                        BootOptionName[ARRAY_SIZE (L"Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION  BmBootOption;
  BOOLEAN                       DevPathStringIsDynamic;
  CHAR16                        *DevPathString;

  //
  // Ignore all status codes that are irrelevant for LoadImage() and
  // StartImage() preparations and return codes.
  //
  if ((Value != mLoadPrep) && (Value != mLoadFail) &&
      (Value != mStartPrep) && (Value != mStartFail))
  {
    return EFI_SUCCESS;
  }

  //
  // Ignore status codes that are not reported by the same containing module.
  //
  if (!CompareGuid (CallerId, &gEfiCallerIdGuid)) {
    return EFI_SUCCESS;
  }

  //
  // Sanity-check Data in case of failure reports.
  //
  if (((Value == mLoadFail) || (Value == mStartFail)) &&
      ((Data == NULL) ||
       (Data->HeaderSize != sizeof *Data) ||
       (Data->Size != sizeof (EFI_RETURN_STATUS_EXTENDED_DATA) - sizeof *Data) ||
       !CompareGuid (&Data->Type, &gEfiStatusCodeSpecificDataGuid)))
  {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: malformed Data\n",
      gEfiCallerBaseName,
      __FUNCTION__
      ));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Get the number of the Boot#### option that the status code applies to.
  //
  VariableSize = sizeof BootCurrent;
  Status       = gRT->GetVariable (
                        EFI_BOOT_CURRENT_VARIABLE_NAME,
                        &gEfiGlobalVariableGuid,
                        NULL /* Attributes */,
                        &VariableSize,
                        &BootCurrent
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to get %g:\"%s\": %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      &gEfiGlobalVariableGuid,
      EFI_BOOT_CURRENT_VARIABLE_NAME,
      Status
      ));
    return Status;
  }

  if (VariableSize != sizeof BootCurrent) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: got %Lu bytes for %g:\"%s\", expected %Lu\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      (UINT64)VariableSize,
      &gEfiGlobalVariableGuid,
      EFI_BOOT_CURRENT_VARIABLE_NAME,
      (UINT64)sizeof BootCurrent
      ));
    return EFI_INCOMPATIBLE_VERSION;
  }

  //
  // Get the Boot#### option that the status code applies to.
  //
  UnicodeSPrint (
    BootOptionName,
    sizeof BootOptionName,
    L"Boot%04x",
    BootCurrent
    );
  Status = EfiBootManagerVariableToLoadOption (BootOptionName, &BmBootOption);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: EfiBootManagerVariableToLoadOption(\"%s\"): %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      BootOptionName,
      Status
      ));
    return Status;
  }

  //
  // Format the device path.
  //
  DevPathStringIsDynamic = TRUE;
  DevPathString          = ConvertDevicePathToText (
                             BmBootOption.FilePath,
                             FALSE,        // DisplayOnly
                             FALSE         // AllowShortcuts
                             );
  if (DevPathString == NULL) {
    DevPathStringIsDynamic = FALSE;
    DevPathString          = L"<out of memory while formatting device path>";
  }

  //
  // Print the message to the console.
  //
  if ((Value == mLoadPrep) || (Value == mStartPrep)) {
    Print (
      L"%a: %a %s \"%s\" from %s\n",
      gEfiCallerBaseName,
      Value == mLoadPrep ? "loading" : "starting",
      BootOptionName,
      BmBootOption.Description,
      DevPathString
      );
  } else {
    Print (
      L"%a: failed to %a %s \"%s\" from %s: %r\n",
      gEfiCallerBaseName,
      Value == mLoadFail ? "load" : "start",
      BootOptionName,
      BmBootOption.Description,
      DevPathString,
      ((EFI_RETURN_STATUS_EXTENDED_DATA *)Data)->ReturnStatus
      );
  }

  //
  // Done.
  //
  if (DevPathStringIsDynamic) {
    FreePool (DevPathString);
  }

  EfiBootManagerFreeLoadOption (&BmBootOption);
  return EFI_SUCCESS;
}

/**
  Unregister HandleStatusCode() at ExitBootServices().

  (See EFI_RSC_HANDLER_PROTOCOL in Volume 3 of the Platform Init spec.)

  @param[in] Event    Event whose notification function is being invoked.

  @param[in] Context  Pointer to EFI_RSC_HANDLER_PROTOCOL, originally looked up
                      when HandleStatusCode() was registered.
**/
STATIC
VOID
EFIAPI
UnregisterAtExitBootServices (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_RSC_HANDLER_PROTOCOL  *StatusCodeRouter;

  StatusCodeRouter = Context;
  StatusCodeRouter->Unregister (HandleStatusCode);
}

/**
  Register a status code handler for printing the Boot Manager's LoadImage()
  and StartImage() preparations, and return codes, to the UEFI console.

  @retval EFI_SUCCESS  The status code handler has been successfully
                       registered.

  @return              Error codes propagated from boot services and from
                       EFI_RSC_HANDLER_PROTOCOL.
**/
EFI_STATUS
EFIAPI
PlatformBmPrintScRegisterHandler (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_RSC_HANDLER_PROTOCOL  *StatusCodeRouter;
  EFI_EVENT                 ExitBootEvent;

  Status = gBS->LocateProtocol (
                  &gEfiRscHandlerProtocolGuid,
                  NULL /* Registration */,
                  (VOID **)&StatusCodeRouter
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set the EFI_STATUS_CODE_VALUE convenience variables.
  //
  mLoadPrep = PcdGet32 (PcdProgressCodeOsLoaderLoad);
  mLoadFail = (EFI_SOFTWARE_DXE_BS_DRIVER |
               EFI_SW_DXE_BS_EC_BOOT_OPTION_LOAD_ERROR);
  mStartPrep = PcdGet32 (PcdProgressCodeOsLoaderStart);
  mStartFail = (EFI_SOFTWARE_DXE_BS_DRIVER |
                EFI_SW_DXE_BS_EC_BOOT_OPTION_FAILED);

  //
  // Register the handler callback.
  //
  Status = StatusCodeRouter->Register (HandleStatusCode, TPL_CALLBACK);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to register status code handler: %r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    return Status;
  }

  //
  // Status code reporting and routing/handling extend into OS runtime. Since
  // we don't want our handler to survive the BDS phase, we have to unregister
  // the callback at ExitBootServices(). (See EFI_RSC_HANDLER_PROTOCOL in
  // Volume 3 of the Platform Init spec.)
  //
  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES, // Type
                  TPL_CALLBACK,                  // NotifyTpl
                  UnregisterAtExitBootServices,  // NotifyFunction
                  StatusCodeRouter,              // NotifyContext
                  &ExitBootEvent                 // Event
                  );
  if (EFI_ERROR (Status)) {
    //
    // We have to unregister the callback right now, and fail the function.
    //
    DEBUG ((
      DEBUG_ERROR,
      "%a:%a: failed to create ExitBootServices() event: "
      "%r\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    StatusCodeRouter->Unregister (HandleStatusCode);
    return Status;
  }

  return EFI_SUCCESS;
}
