/** @file
  This library is mainly used by the Standalone MM Core to start performance logging to ensure that
  Standalone MM Performance and PerformanceEx Protocol are installed as early as possible in the MM phase.

  Caution: This module requires additional review when modified.
    - This driver will have external input - performance data and communicate buffer in MM mode.
    - This external input must be validated carefully to avoid security issue like
      buffer overflow, integer overflow.

  SmmPerformanceHandlerEx(), SmmPerformanceHandler() will receive untrusted input and do basic validation.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmmCorePerformanceLibInternal.h"

#include <Guid/EventGroup.h>
#include <Library/StandaloneMmMemLib.h>

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
  return MmIsBufferOutsideMmValid (Buffer, Length);
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
  return TRUE;
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
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;

  if (Handle == NULL) {
    return NULL;
  }

  LoadedImage = NULL;
  gMmst->MmHandleProtocol (
           Handle,
           &gEfiLoadedImageProtocolGuid,
           (VOID **)&LoadedImage
           );
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
  return EFI_NOT_FOUND;
}

/**
  The constructor function initializes the Standalone MM Core performance library.

  It will ASSERT() if one of these operations fails and it will always return EFI_SUCCESS.

  @param[in]  ImageHandle     The firmware allocated handle for the image.
  @param[in]  MmSystemTable   A pointer to the MM System Table.

  @retval EFI_SUCCESS     The constructor successfully gets HobList.
  @retval Other value     The constructor can't get HobList.

**/
EFI_STATUS
EFIAPI
StandaloneMmCorePerformanceLibConstructor (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  EFI_STATUS  Status;

  mPerformanceMeasurementEnabled =  (BOOLEAN)((FixedPcdGet8 (PcdPerformanceLibraryPropertyMask) & PERFORMANCE_LIBRARY_PROPERTY_MEASUREMENT_ENABLED) != 0);

  if (!PerformanceMeasurementEnabled ()) {
    DEBUG ((DEBUG_WARN, "[%a] - Performance library is linked but performance tracing is not enabled.\n", __func__));
    //
    // Do not initialize performance infrastructure if not required.
    //
    return EFI_SUCCESS;
  }

  Status = InitializeMmCorePerformanceLibCommon (&gEfiEventExitBootServicesGuid);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
