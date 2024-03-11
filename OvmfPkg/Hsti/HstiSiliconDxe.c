/** @file
  This file contains DXE driver for testing and publishing HSTI

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HstiSiliconDxe.h"

ADAPTER_INFO_PLATFORM_SECURITY_STRUCT mHstiStruct = {
    PLATFORM_SECURITY_VERSION_VNEXTCS,
    PLATFORM_SECURITY_ROLE_PLATFORM_REFERENCE,
    {HSTI_PLATFORM_NAME},
    HSTI_SECURITY_FEATURE_SIZE,
    {0},
    {0},
    {0},
    0,
};

/**
  Handler to gather and publish HSTI results on ReadyToBootEvent

  @param[in]  Event     Event whose notification function is being invoked
  @param[in]  Context   Pointer to the notification function's context
**/
VOID
    EFIAPI
    OnReadyToBoot(
        EFI_EVENT Event,
        VOID *Context)
{
  EFI_HANDLE Handle;
  EFI_STATUS Status;

  Status = HstiLibSetTable(
      &mHstiStruct,
      sizeof(mHstiStruct));
  if (EFI_ERROR(Status))
  {
    if (Status != EFI_ALREADY_STARTED)
    {
      ASSERT_EFI_ERROR(Status);
    }
  }

  Handle = NULL;
  Status = gBS->InstallProtocolInterface(
      &Handle,
      &gHstiPublishCompleteProtocolGuid,
      EFI_NATIVE_INTERFACE,
      NULL);
  ASSERT_EFI_ERROR(Status);

  if (Event != NULL)
  {
    gBS->CloseEvent(Event);
  }
}

/**
  The driver's entry point.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
HstiSiliconDxeEntrypoint(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_EVENT Event;

  Status = gBS->InstallProtocolInterface(
      &gImageHandle,
      &gHstiProtocolGuid,
      EFI_NATIVE_INTERFACE,
      NULL);
  ASSERT_EFI_ERROR(Status);

  EfiCreateEventReadyToBootEx(
      TPL_NOTIFY,
      OnReadyToBoot,
      NULL,
      &Event);

  return EFI_SUCCESS;
}
