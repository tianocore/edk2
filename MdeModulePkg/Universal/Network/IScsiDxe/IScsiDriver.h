/** @file
  The header file of IScsiDriver.c

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiDriver.h

Abstract:
  The header file of IScsiDriver.c

**/

#ifndef _ISCSI_DRIVER_H_
#define _ISCSI_DRIVER_H_

#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/IScsiInitiatorName.h>
#include <Protocol/Ip4Config.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>

#define ISCSI_PRIVATE_GUID \
  { 0xfa3cde4c, 0x87c2, 0x427d, {0xae, 0xde, 0x7d, 0xd0, 0x96, 0xc8, 0x8c, 0x58} }

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
/**
  Test to see if IScsi driver supports the given controller. 

  @param  This[in]                Protocol instance pointer.

  @param  ControllerHandle[in]    Handle of controller to test.

  @param  RemainingDevicePath[in] Optional parameter use to pick a specific child device to start.

  @retval EFI_SUCCES              This driver supports the controller.

  @retval EFI_ALREADY_STARTED     This driver is already running on this device.

  @retval EFI_UNSUPPORTED         This driver doesn't support the controller.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start to manage the controller. 

  @param  This[in]                Protocol instance pointer.

  @param  ControllerHandle[in]    Handle of the controller.

  @param  RemainingDevicePath[in] Optional parameter use to pick a specific child device to start.

  @retval EFI_SUCCES              This driver supports this device.

  @retval EFI_ALREADY_STARTED     This driver is already running on this device.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Release the control of this controller and remove the IScsi functions.

  @param  This[in]              Protocol instance pointer.

  @param  ControllerHandle[in]  Handle of controller to stop.

  @param  NumberOfChildren[in]  Not used.

  @param  ChildHandleBuffer[in] Not used.

  @retval EFI_SUCCES            This driver supports this device.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// EFI Component Name Protocol for IScsi driver.
//
/**
  Retrieves a Unicode string that is the user readable name of the EFI Driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME_PROTOCOL
                                instance.

  @param  Language[in]          A pointer to a three character ISO 639-2 language
                                identifier.
                                This is the language of the driver name that that
                                the caller is requesting, and it must match one of
                                the languages specified in SupportedLanguages.  
                                The number of languages supported by a driver is up
                                to the driver writer.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
IScsiComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by an EFI Driver.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver specified by
                                This is managing.  This handle specifies the controller
                                whose name is to be returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve the name
                                of.  This is an optional parameter that may be NULL.  It
                                will be NULL for device drivers.  It will also be NULL
                                for a bus drivers that wish to retrieve the name of the
                                bus controller.  It will not be NULL for a bus driver
                                that wishes to retrieve the name of a child controller.

  @param  Language[in]          A pointer to a three character ISO 639-2 language 
                                identifier.  This is the language of the controller name
                                that that the caller is requesting, and it must match one
                                of the languages specified in SupportedLanguages.  The
                                number of languages supported by a driver is up to the
                                driver writer.

  @param  ControllerName[out]   A pointer to the Unicode string to return.  This Unicode
                                string is the name of the controller specified by 
                                ControllerHandle and ChildHandle in the language 
                                specified by Language from the point of view of the 
                                driver specified by This. 

  @retval EFI_SUCCESS           The Unicode string for the user readable name in the 
                                language specified by Language for the driver 
                                specified by This was returned in DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently managing
                                the controller specified by ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support the 
                                language specified by Language.

**/
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
// EFI IScsi Initiator Name Protocol for IScsi driver.
//
/**
  Retrieves the current set value of iSCSI Initiator Name. 

  @param  This[in]              Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.

  @param  BufferSize[in][out]   Size of the buffer in bytes pointed to by Buffer / Actual
                                size of the variable data buffer.

  @param  Buffer[out]           Pointer to the buffer for data to be read.

  @retval EFI_SUCCESS           Data was successfully retrieved into the provided 
                                buffer and the BufferSize was sufficient to handle the
                                iSCSI initiator name.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize is too small for the result. BufferSize will
                                be updated with the size required to complete the request.
                                Buffer will not be affected.

  @retval EFI_INVALID_PARAMETER BufferSize is NULL. BufferSize and Buffer will not be
                                affected.

  @retval EFI_INVALID_PARAMETER Buffer is NULL. BufferSize and Buffer will not be
                                affected.

  @retval EFI_DEVICE_ERROR      The iSCSI initiator name could not be retrieved due to
                                a hardware error.

**/
EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

/**
  Sets the iSCSI Initiator Name. 

  @param  This[in]              Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL instance.

  @param  BufferSize[in][out]   Size of the buffer in bytes pointed to by Buffer.

  @param  Buffer[out]           Pointer to the buffer for data to be written.
  
  @retval EFI_SUCCESS           Data was successfully stored by the protocol.

  @retval EFI_UNSUPPORTED       Platform policies do not allow for data to be written.

  @retval EFI_INVALID_PARAMETER BufferSize exceeds the maximum allowed limit.
                                BufferSize will be updated with the maximum size
                                required to complete the request.

  @retval EFI_INVALID_PARAMETER Buffersize is NULL. BufferSize and Buffer will not be
                                affected.

  @retval EFI_INVALID_PARAMETER Buffer is NULL. BufferSize and Buffer will not be affected.

  @retval EFI_DEVICE_ERROR      The data could not be stored due to a hardware error.

  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available to hold the data.

  @retval EFI_PROTOCOL_ERROR    Input iSCSI initiator name does not adhere to RFC 3720.

**/
EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

#endif
