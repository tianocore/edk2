/*++

Copyright (c)  2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

Module Name:

  IScsiDriver.h

Abstract:

--*/

#ifndef _ISCSI_DRIVER_H_
#define _ISCSI_DRIVER_H_

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <protocol/DriverBinding.h>
#include <protocol/ScsiPassThruExt.h>
#include <protocol/IScsiInitiatorName.h>
#include <protocol/Ip4Config.h>
#include <protocol/ComponentName.h>
#include <protocol/ComponentName2.h>

#define ISCSI_PRIVATE_GUID \
  { \
    0xfa3cde4c, 0x87c2, 0x427d, 0xae, 0xde, 0x7d, 0xd0, 0x96, 0xc8, 0x8c, 0x58 \
  }

#define ISCSI_INITIATOR_NAME_VAR_NAME L"I_NAME"

extern EFI_COMPONENT_NAME2_PROTOCOL       gIScsiComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL        gIScsiComponentName;

extern EFI_ISCSI_INITIATOR_NAME_PROTOCOL  gIScsiInitiatorName;

extern EFI_GUID                           mIScsiPrivateGuid;

typedef struct _ISCSI_PRIVATE_PROTOCOL {
  UINT32  Reserved;
} ISCSI_PRIVATE_PROTOCOL;

//
// EFI Driver Binding Protocol for iSCSI driver.
//
EFI_STATUS
EFIAPI
IScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
IScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
IScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// EFI Component Name Protocol for iSCSI driver.
//
EFI_STATUS
EFIAPI
IScsiComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  );

EFI_STATUS
EFIAPI
IScsiComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  );

//
// EFI iSCSI Initiator Name Protocol for iSCSI driver.
//
EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

#endif
