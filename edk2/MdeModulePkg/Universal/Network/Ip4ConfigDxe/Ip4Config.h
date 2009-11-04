/** @file
  Header file for IP4Config driver.

Copyright (c) 2006 - 2009, Intel Corporation.<BR>
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
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>

#include <Guid/MdeModuleHii.h>

#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/NetLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/PrintLib.h>
#include <Library/DpcLib.h>

#include "NicIp4Variable.h"

typedef struct _IP4_CONFIG_INSTANCE IP4_CONFIG_INSTANCE;

//
// Global variables
//
extern EFI_DRIVER_BINDING_PROTOCOL     gIp4ConfigDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL     gIp4ConfigComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gIp4ConfigComponentName2;
                                      
extern IP4_CONFIG_INSTANCE             *mIp4ConfigNicList[MAX_IP4_CONFIG_IN_VARIABLE];
extern EFI_IP4_CONFIG_PROTOCOL         mIp4ConfigProtocolTemplate;

#define IP4_PROTO_ICMP                 0x01
#define IP4_CONFIG_INSTANCE_SIGNATURE  SIGNATURE_32 ('I', 'P', '4', 'C')

#define IP4_CONFIG_STATE_IDLE          0
#define IP4_CONFIG_STATE_STARTED       1
#define IP4_CONFIG_STATE_CONFIGURED    2

#define DHCP_TAG_PARA_LIST             55
#define DHCP_TAG_NETMASK               1
#define DHCP_TAG_ROUTER                3


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


typedef struct {
  UINTN             DeviceNum;
  BOOLEAN           Enabled;
  EFI_IPv4_ADDRESS  LocalIp;
  EFI_IPv4_ADDRESS  SubnetMask;
  EFI_IPv4_ADDRESS  Gateway;
} IP4_CONFIG_SESSION_DATA;

typedef struct _IP4_CONFIG_FORM_ENTRY {
  LIST_ENTRY                    Link;
  IP4_CONFIG_INSTANCE           *Ip4ConfigInstance;
  EFI_HANDLE                    Controller;
  CHAR16                        MacString[95];
  EFI_STRING_ID                 PortTitleToken;
  EFI_STRING_ID                 PortTitleHelpToken;
  IP4_CONFIG_SESSION_DATA       SessionConfigData;
} IP4CONFIG_FORM_ENTRY;

#define IP4CONFIG_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('I', 'P', '4', 'C')

typedef struct _IP4_FORM_CALLBACK_INFO_INSTANCE {
  UINTN                            Signature;
  EFI_HANDLE                       DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   ConfigAccess;
  EFI_HII_DATABASE_PROTOCOL        *HiiDatabase;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *ConfigRouting;
  EFI_HII_HANDLE                   RegisteredHandle;
  IP4CONFIG_FORM_ENTRY             *Current;
} IP4_FORM_CALLBACK_INFO;

#define IP4CONFIG_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK(Callback) \
  CR ( \
  Callback, \
  IP4_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  IP4CONFIG_FORM_CALLBACK_INFO_SIGNATURE \
  )

struct _IP4_CONFIG_INSTANCE {
  UINT32                        Signature;
  EFI_HANDLE                    Controller;
  EFI_HANDLE                    Image;

  EFI_IP4_CONFIG_PROTOCOL       Ip4ConfigProtocol;

  IP4_FORM_CALLBACK_INFO        Ip4FormCallbackInfo;

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

#define IP4_CONFIG_INSTANCE_FROM_IP4FORM_CALLBACK_INFO(this) \
  CR (this, IP4_CONFIG_INSTANCE, Ip4FormCallbackInfo, IP4_CONFIG_INSTANCE_SIGNATURE)


/**
  Set the IP configure parameters for this NIC. 

  If Reconfig is TRUE, the IP driver will be informed to discard current 
  auto configure parameter and restart the auto configuration process. 
  If current there is a pending auto configuration, EFI_ALREADY_STARTED is
  returned. You can only change the configure setting when either
  the configure has finished or not started yet. If NicConfig, the
  NIC's configure parameter is removed from the variable.

  @param  Instance               The IP4 CONFIG instance.
  @param  NicConfig              The new NIC IP4 configure parameter
  @param  Reconfig               Inform the IP4 driver to restart the auto
                                 configuration
                                 
  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 set successfully .
  @retval EFI_INVALID_PARAMETER  This is NULL or the configure parameter is
                                 invalid.
  @retval EFI_ALREADY_STARTED    There is a pending auto configuration.
  @retval EFI_NOT_FOUND          No auto configure parameter is found

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigSetInfo (
  IN IP4_CONFIG_INSTANCE          *Instance,
  IN NIC_IP4_CONFIG_INFO          *NicConfig     OPTIONAL,
  IN BOOLEAN                      Reconfig
  );

/**
  Get the configure parameter for this NIC.

  @param  Instance               The IP4 CONFIG Instance.
  @param  ConfigLen              The length of the NicConfig buffer.
  @param  NicConfig              The buffer to receive the NIC's configure
                                 parameter.

  @retval EFI_SUCCESS            The configure parameter for this NIC was 
                                 obtained successfully .
  @retval EFI_INVALID_PARAMETER  This or ConfigLen is NULL.
  @retval EFI_NOT_FOUND          There is no configure parameter for the NIC in
                                 NVRam.
  @retval EFI_BUFFER_TOO_SMALL   The ConfigLen is too small or the NicConfig is 
                                 NULL.

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigGetInfo (
  IN  IP4_CONFIG_INSTANCE         *Instance,
  IN OUT  UINTN                   *ConfigLen,
  OUT NIC_IP4_CONFIG_INFO         *NicConfig
  );

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
