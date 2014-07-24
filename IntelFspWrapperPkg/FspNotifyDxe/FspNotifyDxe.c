/** @file
  This driver will register two callbacks to call fsp's notifies.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Protocol/PciEnumerationComplete.h>

#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>
#include <Library/FspApiLib.h>

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
  FSP_STATUS          FspStatus;
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
  FspStatus = CallFspNotifyPhase (mFspHeader, &NotifyPhaseParams);
  if (FspStatus != FSP_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP NotifyPhase AfterPciEnumeration failed, status: 0x%x\n", FspStatus));
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
  FSP_STATUS          FspStatus;

  gBS->CloseEvent (Event);

  NotifyPhaseParams.Phase = EnumInitPhaseReadyToBoot;
  FspStatus = CallFspNotifyPhase (mFspHeader, &NotifyPhaseParams);
  if (FspStatus != FSP_SUCCESS) {
    DEBUG((DEBUG_ERROR, "FSP NotifyPhase ReadyToBoot failed, status: 0x%x\n", FspStatus));
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

  mFspHeader = FspFindFspHeader (PcdGet32 (PcdFlashFvFspBase));
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

