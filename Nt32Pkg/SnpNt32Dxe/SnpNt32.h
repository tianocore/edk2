/** @file

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  SnpNt32.h

Abstract:

-**/

#ifndef _SNP_NT32_H_
#define _SNP_NT32_H_

#include <Uefi.h>

#include <Protocol/SimpleNetwork.h>
#include <Protocol/DevicePath.h>
#include <Protocol/WinNtThunk.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/NetLib.h>
#include <Library/MemoryAllocationLib.h>

typedef struct _SNPNT32_GLOBAL_DATA  SNPNT32_GLOBAL_DATA;
typedef struct _SNPNT32_INSTANCE_DATA SNPNT32_INSTANCE_DATA;

#define NETWORK_LIBRARY_NAME_U          L"SnpNt32Io.dll"

#define NETWORK_LIBRARY_INITIALIZE      "SnpInitialize"
#define NETWORK_LIBRARY_FINALIZE        "SnpFinalize"
#define NETWORK_LIBRARY_SET_RCV_FILTER  "SnpSetReceiveFilter"
#define NETWORK_LIBRARY_RECEIVE         "SnpReceive"
#define NETWORK_LIBRARY_TRANSMIT        "SnpTransmit"

#pragma pack(1)
typedef struct _NT_NET_INTERFACE_INFO {
  UINT32          InterfaceIndex;
  EFI_MAC_ADDRESS MacAddr;
} NT_NET_INTERFACE_INFO;
#pragma pack()

#define NET_ETHER_HEADER_SIZE     14

#define MAX_INTERFACE_INFO_NUMBER 16
#define MAX_FILE_NAME_LENGTH      280

//
//  Functions in Net Library
//
typedef
INT32
(*NT_NET_INITIALIZE) (
  IN OUT  UINT32                *InterfaceCount,
  IN OUT  NT_NET_INTERFACE_INFO * InterfaceInfoBuffer
  );

typedef
INT32
(*NT_NET_FINALIZE) (
  VOID
  );

typedef
INT32
(*NT_NET_SET_RECEIVE_FILTER) (
  IN  UINT32                        Index,
  IN  UINT32                        EnableFilter,
  IN  UINT32                        MCastFilterCnt,
  IN  EFI_MAC_ADDRESS               * MCastFilter
  );

typedef
INT32
(*NT_NET_RECEIVE) (
  IN      UINT32                        Index,
  IN OUT  UINT32                        *BufferSize,
  OUT     VOID                          *Buffer
  );

typedef
INT32
(*NT_NET_TRANSMIT) (
  IN  UINT32                        Index,
  IN  UINT32                        HeaderSize,
  IN  UINT32                        BufferSize,
  IN  VOID                          *Buffer,
  IN  EFI_MAC_ADDRESS               * SrcAddr,
  IN  EFI_MAC_ADDRESS               * DestAddr,
  IN  UINT16                        *Protocol
  );

typedef struct _NT_NET_UTILITY_TABLE {
  NT_NET_INITIALIZE         Initialize;
  NT_NET_FINALIZE           Finalize;
  NT_NET_SET_RECEIVE_FILTER SetReceiveFilter;
  NT_NET_RECEIVE            Receive;
  NT_NET_TRANSMIT           Transmit;
} NT_NET_UTILITY_TABLE;

//
//  Private functions
//
typedef
EFI_STATUS
(*SNPNT32_INITIALIZE_GLOBAL_DATA) (
  IN SNPNT32_GLOBAL_DATA * This
  );

typedef
EFI_STATUS
(*SNPNT32_INITIALIZE_INSTANCE_DATA) (
  IN SNPNT32_GLOBAL_DATA    * This,
  IN SNPNT32_INSTANCE_DATA  * Instance
  );

typedef
EFI_STATUS
(*SNPNT32_CLOSE_INSTANCE) (
  IN SNPNT32_GLOBAL_DATA    * This,
  IN SNPNT32_INSTANCE_DATA  * Instance
  );

//
//  Global data for this driver
//
#define SNP_NT32_DRIVER_SIGNATURE SIGNATURE_32 ('W', 'S', 'N', 'P')

struct _SNPNT32_GLOBAL_DATA {
  UINT32                            Signature;

  //
  //  List for all the fake SNP instance
  //
  LIST_ENTRY                        InstanceList;

  EFI_WIN_NT_THUNK_PROTOCOL         *WinNtThunk;
  HMODULE                           NetworkLibraryHandle;

  NT_NET_UTILITY_TABLE              NtNetUtilityTable;

  EFI_LOCK                          Lock;

  //
  //  Private functions
  //
  SNPNT32_INITIALIZE_GLOBAL_DATA    InitializeGlobalData;
  SNPNT32_INITIALIZE_INSTANCE_DATA  InitializeInstanceData;
  SNPNT32_CLOSE_INSTANCE            CloseInstance;
};

//
//  Instance data for each fake SNP instance
//
#define SNP_NT32_INSTANCE_SIGNATURE SIGNATURE_32 ('w', 'S', 'N', 'P')

struct _SNPNT32_INSTANCE_DATA {
  UINT32                      Signature;

  //
  //  List entry use for linking with other instance
  //
  LIST_ENTRY                  Entry;

  SNPNT32_GLOBAL_DATA         *GlobalData;

  EFI_HANDLE                  DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;

  EFI_SIMPLE_NETWORK_PROTOCOL Snp;
  EFI_SIMPLE_NETWORK_MODE     Mode;

  NT_NET_INTERFACE_INFO       InterfaceInfo;

  //
  //  Private functions
  //
};

#define SNP_NT32_INSTANCE_DATA_FROM_SNP_THIS(a) \
  CR ( \
  a, \
  SNPNT32_INSTANCE_DATA, \
  Snp, \
  SNP_NT32_INSTANCE_SIGNATURE \
  )

extern EFI_DRIVER_BINDING_PROTOCOL    gSnpNt32DriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL    gSnpNt32DriverComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   gSnpNt32DriverComponentName2;

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
SnpNt32DriverBindingSupported (
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
SnpNt32DriverBindingStart (
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
SnpNt32DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

/**
  Initialize the driver's global data.

  @param  This                  Pointer to the global context data.

  @retval EFI_SUCCESS           The global data is initialized.
  @retval EFI_NOT_FOUND         The required DLL is not found.
  @retval EFI_DEVICE_ERROR      Error initialize network utility library.
  @retval EFI_OUT_OF_RESOURCES  Out of resource.
  @retval other                 Other errors.

**/
EFI_STATUS
SnpNt32InitializeGlobalData (
  IN OUT SNPNT32_GLOBAL_DATA *This
  );

/**
  Initialize the snpnt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The driver instance is initialized.
  @retval other                 Initialization errors.

**/
EFI_STATUS
SnpNt32InitializeInstanceData (
  IN SNPNT32_GLOBAL_DATA        *This,
  IN OUT SNPNT32_INSTANCE_DATA  *Instance
  );

/**
  Close the SnpNt32 driver instance.

  @param  This                  Pointer to the SnpNt32 global data.
  @param  Instance              Pointer to the instance context data.

  @retval EFI_SUCCESS           The instance is closed.

**/
EFI_STATUS
SnpNt32CloseInstance (
  IN SNPNT32_GLOBAL_DATA        *This,
  IN OUT SNPNT32_INSTANCE_DATA  *Instance
  );

#endif
