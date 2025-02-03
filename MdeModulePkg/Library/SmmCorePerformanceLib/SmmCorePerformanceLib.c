/** @file
  Performance library instance used by SMM Core.

  This library provides the performance measurement interfaces and initializes performance
  logging for the SMM phase.
  It initializes SMM phase performance logging by publishing the SMM Performance and PerformanceEx Protocol,
  which is consumed by SmmPerformanceLib to logging performance data in SMM phase.

  This library is mainly used by SMM Core to start performance logging to ensure that
  SMM Performance and PerformanceEx Protocol are installed at the very beginning of SMM phase.

 Caution: This module requires additional review when modified.
 This driver will have external input - performance data and communicate buffer in SMM mode.
 This external input must be validated carefully to avoid security issue like
 buffer overflow, integer overflow.

 SmmPerformanceHandlerEx(), SmmPerformanceHandler() will receive untrusted input and do basic validation.

Copyright (c) 2011 - 2023, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmCorePerformanceLibInternal.h"

#include <Library/DxeServicesLib.h>
#include <Library/SmmMemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SmmBase2.h>
#include <Protocol/SmmExitBootServices.h>

PERFORMANCE_PROPERTY  mPerformanceProperty;

/**
  A library internal MM-instance specific implementation to check if a buffer outside MM is valid.

  This function is provided so Standalone MM and Traditional MM may use a different implementation
  of data buffer check logic.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This buffer is valid per processor architecture.
  @retval FALSE This buffer is not valid per processor architecture.
**/
BOOLEAN
MmCorePerformanceIsNonPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return SmmIsBufferOutsideSmmValid (Buffer, Length);
}

/**
  A library internal MM-instance specific implementation to check if a comm buffer is valid.

  This function is provided so Standalone MM and Traditional MM may use a different implementation
  of comm buffer check logic.

  @param[in] Buffer  The buffer start address to be checked.
  @param[in] Length  The buffer length to be checked.

  @retval TRUE  This communicate buffer is valid per processor architecture.
  @retval FALSE This communicate buffer is not valid per processor architecture.
**/
BOOLEAN
MmCorePerformanceIsPrimaryBufferValid (
  IN EFI_PHYSICAL_ADDRESS  Buffer,
  IN UINT64                Length
  )
{
  return SmmIsBufferOutsideSmmValid (Buffer, Length);
}

/**
  Return a pointer to the loaded image protocol for the given handle.

  @param[in]  Handle      A handle to query for the loaded image protocol.

  @return A pointer to a loaded image protocol instance or null if the handle does not support load image protocol.
**/
EFI_LOADED_IMAGE_PROTOCOL *
GetLoadedImageProtocol (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                   Status;
  EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding;
  EFI_LOADED_IMAGE_PROTOCOL    *LoadedImage;

  if (Handle == NULL) {
    return NULL;
  }

  LoadedImage = NULL;

  //
  // Try Handle as ImageHandle.
  //
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage
                  );
  if (EFI_ERROR (Status)) {
    //
    // Try Handle as Controller Handle
    //
    Status = gBS->OpenProtocol (
                    Handle,
                    &gEfiDriverBindingProtocolGuid,
                    (VOID **)&DriverBinding,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      //
      // Get Image protocol from ImageHandle
      //
      Status = gBS->HandleProtocol (
                      DriverBinding->ImageHandle,
                      &gEfiLoadedImageProtocolGuid,
                      (VOID **)&LoadedImage
                      );
    }
  }

  return LoadedImage;
}

/**
  Get the module name from the user interface section.

  @param[in]  ModuleGuid    The GUID of the module.
  @param[out] NameString    The buffer to store the name string.
  @param[in]  BufferSize    The size of the buffer in bytes.

  @retval EFI_SUCCESS      The name string is successfully retrieved.
  @retval EFI_NOT_FOUND    The module name was not found.

**/
EFI_STATUS
GetNameFromUiSection (
  IN EFI_GUID  *ModuleGuid,
  OUT CHAR8    *NameString,
  IN UINTN     BufferSize
  )
{
  EFI_STATUS  Status;
  CHAR16      *StringPtr;
  UINTN       Index;
  UINTN       StringSize;

  StringPtr  = NULL;
  StringSize = 0;
  Status     = GetSectionFromAnyFv (
                 ModuleGuid,
                 EFI_SECTION_USER_INTERFACE,
                 0,
                 (VOID **)&StringPtr,
                 &StringSize
                 );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < BufferSize - 1 && StringPtr[Index] != 0; Index++) {
      NameString[Index] = (CHAR8)StringPtr[Index];
    }

    NameString[Index] = 0;
    FreePool (StringPtr);
  }

  return Status;
}

/**
  Initializes the MM Core Performance library after MM services are available.

  @param[in]  Event    The event of notify protocol.
  @param[in]  Context  Notify event context.

**/
VOID
EFIAPI
InitializeSmmCorePerformanceLib (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS            Status;
  PERFORMANCE_PROPERTY  *PerformanceProperty;

  Status = InitializeMmCorePerformanceLibCommon (&gEdkiiSmmExitBootServicesProtocolGuid);
  ASSERT_EFI_ERROR (Status);

  Status = EfiGetSystemConfigurationTable (&gPerformanceProtocolGuid, (VOID **)&PerformanceProperty);
  if (EFI_ERROR (Status)) {
    //
    // Install configuration table for performance property.
    //
    mPerformanceProperty.Revision  = PERFORMANCE_PROPERTY_REVISION;
    mPerformanceProperty.Reserved  = 0;
    mPerformanceProperty.Frequency = GetPerformanceCounterProperties (
                                       &mPerformanceProperty.TimerStartValue,
                                       &mPerformanceProperty.TimerEndValue
                                       );
    Status = gBS->InstallConfigurationTable (&gPerformanceProtocolGuid, &mPerformanceProperty);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  The constructor function initializes the Performance Measurement Enable flag and
  registers SmmBase2 protocol notify callback.
  It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
SmmCorePerformanceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;
  VOID        *Registration;

  mPerformanceMeasurementEnabled =  (BOOLEAN)((PcdGet8 (PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);

  if (!PerformanceMeasurementEnabled ()) {
    DEBUG ((DEBUG_WARN, "[%a] - Performance library is linked but performance tracing is not enabled.\n", __func__));
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  //
  // Create the events to do the library init.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InitializeSmmCorePerformanceLib,
                  NULL,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register for protocol notifications on this event
  //
  Status = gBS->RegisterProtocolNotify (
                  &gEfiSmmBase2ProtocolGuid,
                  Event,
                  &Registration
                  );

  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
