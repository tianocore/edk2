/** @file
  This driver will register two callbacks to call fsp's notifies.

  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/PciEnumerationComplete.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/FspApiLib.h>

/**
  Relocate this image under 4G memory.

  @param  ImageHandle  Handle of driver image.
  @param  SystemTable  Pointer to system table.

  @retval EFI_SUCCESS  Image successfully relocated.
  @retval EFI_ABORTED  Failed to relocate image.

**/
EFI_STATUS
RelocateImageUnder4GIfNeeded (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

FSP_INFO_HEADER *mFspHeader = NULL;

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_PHASE_PARAMS NotifyPhaseParams;
  EFI_STATUS          Status;
  VOID                *Interface;

  //
  // Try to locate it because gEfiPciEnumerationCompleteProtocolGuid will trigger it once when registration.
  // Just return if it is not found.
  //
  Status = gBS->LocateProtocol (
                  &gEfiPciEnumerationCompleteProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }

  NotifyPhaseParams.Phase = EnumInitPhaseAfterPciEnumeration;
  Status = CallFspNotifyPhase (mFspHeader, &NotifyPhaseParams);
  if (Status != EFI_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP NotifyPhase AfterPciEnumeration failed, status: 0x%x\n", Status));
  } else {
    DEBUG((DEBUG_INFO, "FSP NotifyPhase AfterPciEnumeration Success.\n"));
  }
}

/**
  Notification function of EVT_GROUP_READY_TO_BOOT event group.

  This is a notification function registered on EVT_GROUP_READY_TO_BOOT event group.
  When the Boot Manager is about to load and execute a boot option, it reclaims variable
  storage if free size is below the threshold.

  @param[in] Event        Event whose notification function is being invoked.
  @param[in] Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
OnReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_PHASE_PARAMS NotifyPhaseParams;
  EFI_STATUS          Status;

  gBS->CloseEvent (Event);

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  Status = CallFspNotifyPhase (mFspHeader, &NotifyPhaseParams);
  if (Status != EFI_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP NotifyPhase ReadyToBoot failed, status: 0x%x\n", Status));
  } else {
    DEBUG((DEBUG_INFO, "FSP NotifyPhase ReadyToBoot Success.\n"));
  }
}

/**
  Main entry for the FSP DXE module.

  This routine registers two callbacks to call fsp's notifies.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FspDxeEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_EVENT  ReadyToBootEvent;
  VOID       *Registration;
  EFI_EVENT  ProtocolNotifyEvent;

  //
  // Load this driver's image to memory
  //
  Status = RelocateImageUnder4GIfNeeded (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  if (PcdGet32 (PcdFlashFvSecondFspBase) == 0) {
    mFspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
  } else {
    mFspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvSecondFspBase));
  }
  DEBUG ((DEBUG_INFO, "FspHeader - 0x%x\n", mFspHeader));
  if (mFspHeader == NULL) {
    return EFI_DEVICE_ERROR;
  }

  ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiPciEnumerationCompleteProtocolGuid,
                          TPL_CALLBACK,
                          OnPciEnumerationComplete,
                          NULL,
                          &Registration
                          );
  ASSERT (ProtocolNotifyEvent != NULL);

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             OnReadyToBoot,
             NULL,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

