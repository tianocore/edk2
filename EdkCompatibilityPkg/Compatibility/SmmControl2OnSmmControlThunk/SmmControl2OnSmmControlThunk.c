/** @file
  SMM Control2 Protocol on SMM Control Protocol Thunk driver.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmmControl2OnSmmControlThunk.h"

EFI_SMM_CONTROL2_PROTOCOL gSmmControl2 = {
  SmmControl2Trigger,
  SmmControl2Clear,
  0
};

EFI_SMM_CONTROL_PROTOCOL  *mSmmControl;
UINT8                      mDataPort;

/**
  Invokes SMI activation from either the preboot or runtime environment.

  This function generates an SMI.

  @param[in]     This                The EFI_SMM_CONTROL2_PROTOCOL instance.
  @param[in, out] CommandPort         The value written to the command port.
  @param[in, out] DataPort            The value written to the data port.
  @param[in]     Periodic            Optional mechanism to engender a periodic stream.
  @param[in]     ActivationInterval  Optional parameter to repeat at this period one
                                     time or, if the Periodic Boolean is set, periodically.

  @retval EFI_SUCCESS            The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR       The timing is unsupported.
  @retval EFI_INVALID_PARAMETER  The activation period is unsupported.
  @retval EFI_INVALID_PARAMETER  The last periodic activation has not been cleared. 
  @retval EFI_NOT_STARTED        The SMM base service has not been initialized.
**/
EFI_STATUS
EFIAPI
SmmControl2Trigger (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN OUT UINT8                        *CommandPort       OPTIONAL,
  IN OUT UINT8                        *DataPort          OPTIONAL,
  IN BOOLEAN                          Periodic           OPTIONAL,
  IN UINTN                            ActivationInterval OPTIONAL
  )
{
  UINTN  ArgumentBufferSize;

  ArgumentBufferSize = 0;
  if (CommandPort != NULL) {
    ArgumentBufferSize = 1;
  }
  if (DataPort != NULL) {
    IoWrite8 (mDataPort, *DataPort);
  }
  return mSmmControl->Trigger (mSmmControl, (INT8 *)CommandPort, &ArgumentBufferSize, Periodic, ActivationInterval);
}

/**
  Clears any system state that was created in response to the Trigger() call.

  This function acknowledges and causes the deassertion of the SMI activation source.

  @param[in] This                The EFI_SMM_CONTROL2_PROTOCOL instance.
  @param[in] Periodic            Optional parameter to repeat at this period one time

  @retval EFI_SUCCESS            The SMI/PMI has been engendered.
  @retval EFI_DEVICE_ERROR       The source could not be cleared.
  @retval EFI_INVALID_PARAMETER  The service did not support the Periodic input argument.
**/
EFI_STATUS
EFIAPI
SmmControl2Clear (
  IN CONST EFI_SMM_CONTROL2_PROTOCOL  *This,
  IN BOOLEAN                          Periodic OPTIONAL
  )
{
  return mSmmControl->Clear (mSmmControl, Periodic);
}

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  This is a notification function registered on EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  It convers pointer to new virtual address.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
SetVirtualAddressNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EfiConvertPointer (0x0, (VOID **)&mSmmControl);
}

/**
  Entry Point for this thunk driver.

  @param[in] ImageHandle  Image handle of this driver.
  @param[in] SystemTable  A Pointer to the EFI System Table.

  @retval EFI_SUCCESS  The entry point is executed successfully.
  @retval other        Some error occurred when executing this entry point.
**/
EFI_STATUS
EFIAPI
SmmControl2ThunkMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  EFI_EVENT                Event;
  EFI_SMM_CONTROL_REGISTER RegisterInfo;

  ///
  /// Locate Framework SMM Control Protocol
  ///
  Status = gBS->LocateProtocol (&gEfiSmmControlProtocolGuid, NULL, (VOID **)&mSmmControl);
  ASSERT_EFI_ERROR (Status);

  gSmmControl2.MinimumTriggerPeriod = mSmmControl->MinimumTriggerPeriod;

  Status = mSmmControl->GetRegisterInfo (mSmmControl, &RegisterInfo);
  ASSERT_EFI_ERROR (Status);
  mDataPort = RegisterInfo.SmiDataRegister;

  ///
  /// Create event on SetVirtualAddressMap() to convert mSmmControl from a physical address to a virtual address
  ///
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SetVirtualAddressNotify,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &Event
                  );
                  
  ASSERT_EFI_ERROR (Status);

  ///
  /// Publish framework SMM Control Protocol
  ///
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSmmControl2ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gSmmControl2
                  );
  return Status;
}

