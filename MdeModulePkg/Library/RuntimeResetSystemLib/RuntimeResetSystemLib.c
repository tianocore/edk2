/** @file
  DXE Reset System Library instance that calls gRT->ResetSystem().

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_EVENT                    mRuntimeResetSystemLibVirtualAddressChangeEvent;
static EFI_RUNTIME_SERVICES  *mInternalRT;

/**
  This function causes a system-wide reset (cold reset), in which
  all circuitry within the system returns to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  If this function returns, it means that the system does not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  mInternalRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes a system-wide initialization (warm reset), in which all processors
  are set to their initial state. Pending cycles are not corrupted.

  If this function returns, it means that the system does not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  mInternalRT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  If this function returns, it means that the system does not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  mInternalRT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL);
}

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN  DataSize,
  IN VOID   *ResetData
  )
{
  mInternalRT->ResetSystem (EfiResetPlatformSpecific, EFI_SUCCESS, DataSize, ResetData);
}

/**
  The ResetSystem function resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.
**/
VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  mInternalRT->ResetSystem (ResetType, ResetStatus, DataSize, ResetData);
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
RuntimeResetSystemLibVirtualAddressChange (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mInternalRT->ConvertPointer (0, (VOID **)&mInternalRT);
}

/**
  The constructor function of Runtime Reset System Lib.

  This function allocates memory for extended status code data, caches
  the report status code service, and registers events.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
RuntimeResetSystemLibConstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Library should not use the gRT directly, for it may be converted by other library instance.
  //
  mInternalRT = gRT;

  //
  // Register notify function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  RuntimeResetSystemLibVirtualAddressChange,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mRuntimeResetSystemLibVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The Deconstructor function of Runtime Reset System Lib.

  The destructor function frees memory allocated by constructor, and closes related events.
  It will ASSERT() if that related operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
RuntimeResetSystemLibDeconstruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ASSERT (gBS != NULL);
  Status = gBS->CloseEvent (mRuntimeResetSystemLibVirtualAddressChangeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
