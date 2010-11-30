/** @file

 Copyright (c) 2010, Apple, Inc. All rights reserved.

    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnixSnp.h

Abstract:

-**/

#ifndef _UNIX_SNP_H_
#define _UNIX_SNP_H_

#include <Uefi.h>

#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePath.h>
#include <Protocol/UnixIo.h>


#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>

#define NET_ETHER_HEADER_SIZE     14

//
//  Private data for driver.
//
#define UNIX_SNP_PRIVATE_DATA_SIGNATURE SIGNATURE_32( 'U', 'S', 'N', 'P' )

typedef struct
{
	UINT32								Signature;

	EFI_UNIX_THUNK_PROTOCOL*			UnixThunk;

	EFI_HANDLE							DeviceHandle;
	EFI_DEVICE_PATH_PROTOCOL*			DevicePath;

	EFI_MAC_ADDRESS						MacAddress;

	CHAR8*								InterfaceName;
	INTN								ReadBufferSize;
	VOID*								ReadBuffer;
	//
	// Two walking pointers to manage the multiple packets that can be returned
	// in a single read.
	//
	VOID*								CurrentReadPointer;
	VOID*								EndReadPointer;

	INTN								BpfFd;

	EFI_SIMPLE_NETWORK_PROTOCOL			Snp;
	EFI_SIMPLE_NETWORK_MODE				Mode;
} UNIX_SNP_PRIVATE_DATA;

#define UNIX_SNP_PRIVATE_DATA_FROM_SNP_THIS(a) \
			CR( a, UNIX_SNP_PRIVATE_DATA, Snp, UNIX_SNP_PRIVATE_DATA_SIGNATURE )

extern EFI_DRIVER_BINDING_PROTOCOL    gUnixSnpDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL    gUnixSnpDriverComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gUnixSnpDriverComponentName2;

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_UNSUPPORTED     This driver does not support this device

**/
EFI_STATUS
EFIAPI
UnixSnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.
  
  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       Always succeeds.

**/
EFI_STATUS
EFIAPI
UnixSnpDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

#endif	// _UNIX_SNP_H_
