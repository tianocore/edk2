/** @file
  Header file for IP4Config driver.

Copyright (c) 2006 - 2008, Intel Corporation.<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_IP4CONFIG_H_
#define _EFI_IP4CONFIG_H_

#include <Uefi.h>

#include <Protocol/Dhcp4.h>
#include <Protocol/Ip4Config.h>
#include <Protocol/ManagedNetwork.h>

#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#include "NicIp4Variable.h"

typedef struct _IP4_CONFIG_INSTANCE IP4_CONFIG_INSTANCE;

//
// Global variables 
//
extern EFI_DRIVER_BINDING_PROTOCOL   gIp4ConfigDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gIp4ConfigComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gIp4ConfigComponentName2;

extern IP4_CONFIG_INSTANCE           *mIp4ConfigNicList[MAX_IP4_CONFIG_IN_VARIABLE];
extern EFI_IP4_CONFIG_PROTOCOL       mIp4ConfigProtocolTemplate;
extern EFI_NIC_IP4_CONFIG_PROTOCOL   mNicIp4ConfigProtocolTemplate;

#define IP4_PROTO_ICMP                 0x01
#define IP4_CONFIG_INSTANCE_SIGNATURE  SIGNATURE_32 ('I', 'P', '4', 'C')

typedef enum {
  IP4_CONFIG_STATE_IDLE         = 0,
  IP4_CONFIG_STATE_STARTED,
  IP4_CONFIG_STATE_CONFIGURED
} IP4_CONFIG_STATE;

typedef enum {
  DHCP_TAG_PARA_LIST            = 55,
  DHCP_TAG_NETMASK              = 1,
  DHCP_TAG_ROUTER               = 3
} DHCP_TAGS;

//
// Configure the DHCP to request the routers and netmask
// from server. The DHCP_TAG_NETMASK is included in Head.
//
#pragma pack(1)
typedef struct {
  EFI_DHCP4_PACKET_OPTION Head;
  UINT8                   Route;
} IP4_CONFIG_DHCP4_OPTION;
#pragma pack()

struct _IP4_CONFIG_INSTANCE {
  UINT32                        Signature;
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  EFI_IP4_CONFIG_PROTOCOL       Ip4ConfigProtocol;
  EFI_NIC_IP4_CONFIG_PROTOCOL   NicIp4Protocol;

  //
  // NicConfig's state, such as IP4_CONFIG_STATE_IDLE
  //
  INTN                          State;

  //
  // Mnp child to keep the connection with MNP.
  //
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  EFI_HANDLE                    MnpHandle;

  //
  // User's requests data
  //
  EFI_EVENT                     DoneEvent;
  EFI_EVENT                     ReconfigEvent;
  EFI_STATUS                    Result;

  //
  // Identity of this interface and some configuration info.
  //
  NIC_ADDR                      NicAddr;
  UINT16                        NicName[IP4_NIC_NAME_LENGTH];
  UINT32                        NicIndex;
  NIC_IP4_CONFIG_INFO           *NicConfig;

  //
  // DHCP handles to access DHCP
  //
  EFI_DHCP4_PROTOCOL            *Dhcp4;
  EFI_HANDLE                    Dhcp4Handle;
  EFI_EVENT                     Dhcp4Event;
};

#define IP4_CONFIG_INSTANCE_FROM_IP4CONFIG(this) \
  CR (this, IP4_CONFIG_INSTANCE, Ip4ConfigProtocol, IP4_CONFIG_INSTANCE_SIGNATURE)

#define IP4_CONFIG_INSTANCE_FROM_NIC_IP4CONFIG(this) \
  CR (this, IP4_CONFIG_INSTANCE, NicIp4Protocol, IP4_CONFIG_INSTANCE_SIGNATURE)

/**
  Release all the DHCP related resources.

  @param  This                   The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanDhcp4 (
  IN IP4_CONFIG_INSTANCE    *This
  );

/**
  Clean up all the configuration parameters.

  @param  Instance               The IP4 configure instance

  @return None

**/
VOID
Ip4ConfigCleanConfig (
  IN IP4_CONFIG_INSTANCE        *Instance
  );
  
//
// EFI Component Name Functions
//

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 3066 or ISO 639-2 language code format.                                
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
Ip4ConfigComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 3066 or ISO 639-2 language code format.
  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.
                                
  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
Ip4ConfigComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );
  
/**
  Test to see if this driver supports ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to test
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCES           This driver supports this device
  @retval EFI_ALREADY_STARTED  This driver is already running on this device
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  );
  
/**
  Start this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCES           This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );
  
/**
  Stop this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to stop driver on
  @param  NumberOfChildren     Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param  ChildHandleBuffer    List of Child Handles to Stop.

  @retval EFI_SUCCES           This driver is removed ControllerHandle
  @retval other                This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );
  
#endif
