/** @file
  Header file for IP4Config driver.

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
#include <Protocol/ServiceBinding.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/NicIp4ConfigNvData.h>

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
#include <Library/UefiHiiServicesLib.h>


//
// Global variables
//
extern EFI_DRIVER_BINDING_PROTOCOL     gIp4ConfigDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL     gIp4ConfigComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gIp4ConfigComponentName2;

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

typedef struct _IP4CONFIG_CALLBACK_INFO {
  BOOLEAN                          Configured;
  BOOLEAN                          DhcpEnabled;
  EFI_IPv4_ADDRESS                 LocalIp;
  EFI_IPv4_ADDRESS                 SubnetMask;
  EFI_IPv4_ADDRESS                 Gateway;
} IP4_SETTING_INFO;

typedef struct _IP4_CONFIG_INSTANCE {
  UINT32                          Signature;
  EFI_HANDLE                      Controller;
  EFI_HANDLE                      Image;
  EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;

  EFI_IP4_CONFIG_PROTOCOL         Ip4ConfigProtocol;

  EFI_HII_CONFIG_ACCESS_PROTOCOL  HiiConfigAccessProtocol;
  EFI_HANDLE                      ChildHandle;
  EFI_DEVICE_PATH_PROTOCOL        *HiiVendorDevicePath;
  EFI_HII_HANDLE                  RegisteredHandle;
  IP4_SETTING_INFO                Ip4ConfigCallbackInfo;

  //
  // NicConfig's state, such as IP4_CONFIG_STATE_IDLE
  //
  INTN                            State;

  //
  // Mnp child to keep the connection with MNP.
  //
  EFI_MANAGED_NETWORK_PROTOCOL    *Mnp;
  EFI_HANDLE                      MnpHandle;

  //
  // User's requests data
  //
  EFI_EVENT                       DoneEvent;
  EFI_EVENT                       ReconfigEvent;
  EFI_STATUS                      Result;

  //
  // Identity of this interface and some configuration info.
  //
  NIC_ADDR                        NicAddr;
  CHAR16                          *MacString;
  NIC_IP4_CONFIG_INFO             *NicConfig;

  //
  // DHCP handles to access DHCP
  //
  EFI_DHCP4_PROTOCOL              *Dhcp4;
  EFI_HANDLE                      Dhcp4Handle;
  EFI_EVENT                       Dhcp4Event;

  //
  // A dedicated timer is used to poll underlying media status
  //
  EFI_EVENT                       Timer;

  //
  // Underlying media present status. 
  //
  BOOLEAN                         MediaPresent;

  //
  // A flag to indicate EfiIp4ConfigStart should not run
  //
  BOOLEAN                         DoNotStart;
} IP4_CONFIG_INSTANCE;

#define IP4_CONFIG_INSTANCE_FROM_IP4CONFIG(this) \
  CR (this, IP4_CONFIG_INSTANCE, Ip4ConfigProtocol, IP4_CONFIG_INSTANCE_SIGNATURE)

#define IP4_CONFIG_INSTANCE_FROM_CONFIG_ACCESS(this) \
  CR (this, IP4_CONFIG_INSTANCE, HiiConfigAccessProtocol, IP4_CONFIG_INSTANCE_SIGNATURE)


/**
  Set the IP configure parameters for this NIC.

  If Reconfig is TRUE, the IP driver will be informed to discard current
  auto configure parameter and restart the auto configuration process.
  If current there is a pending auto configuration, EFI_ALREADY_STARTED is
  returned. You can only change the configure setting when either
  the configure has finished or not started yet. If NicConfig, the
  NIC's configure parameter is removed from the variable.

  @param  Instance               The IP4 CONFIG instance.
  @param  NicConfig              The new NIC IP4 configure parameter.
  @param  Reconfig               Inform the IP4 driver to restart the auto
                                 configuration.

  @retval EFI_SUCCESS            The configure parameter for this NIC was
                                 set successfully.
  @retval EFI_INVALID_PARAMETER  This is NULL or the configure parameter is
                                 invalid.
  @retval EFI_ALREADY_STARTED    There is a pending auto configuration.
  @retval EFI_NOT_FOUND          No auto configure parameter is found.

**/
EFI_STATUS
EFIAPI
EfiNicIp4ConfigSetInfo (
  IN IP4_CONFIG_INSTANCE          *Instance,
  IN NIC_IP4_CONFIG_INFO          *NicConfig     OPTIONAL,
  IN BOOLEAN                      Reconfig
  );

/**
  Get the NIC's configure information from the IP4 configure variable.
  It will remove the invalid variable.

  @param  Instance               The IP4 CONFIG instance.

  @return NULL if no configure for the NIC in the variable, or it is invalid.
          Otherwise the pointer to the NIC's IP configure parameter will be returned.

**/
NIC_IP4_CONFIG_INFO *
EfiNicIp4ConfigGetInfo (
  IN  IP4_CONFIG_INSTANCE   *Instance
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
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
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

/**
  Starts running the configuration policy for the EFI IPv4 Protocol driver.

  The Start() function is called to determine and to begin the platform
  configuration policy by the EFI IPv4 Protocol driver. This determination may
  be as simple as returning EFI_UNSUPPORTED if there is no EFI IPv4 Protocol
  driver configuration policy. It may be as involved as loading some defaults
  from nonvolatile storage, downloading dynamic data from a DHCP server, and
  checking permissions with a site policy server.
  Starting the configuration policy is just the beginning. It may finish almost
  instantly or it may take several minutes before it fails to retrieve configuration
  information from one or more servers. Once the policy is started, drivers
  should use the DoneEvent parameter to determine when the configuration policy
  has completed. EFI_IP4_CONFIG_PROTOCOL.GetData() must then be called to
  determine if the configuration succeeded or failed.
  Until the configuration completes successfully, EFI IPv4 Protocol driver instances
  that are attempting to use default configurations must return EFI_NO_MAPPING.
  Once the configuration is complete, the EFI IPv4 Configuration Protocol driver
  signals DoneEvent. The configuration may need to be updated in the future,
  however; in this case, the EFI IPv4 Configuration Protocol driver must signal
  ReconfigEvent, and all EFI IPv4 Protocol driver instances that are using default
  configurations must return EFI_NO_MAPPING until the configuration policy has
  been rerun.

  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  DoneEvent              Event that will be signaled when the EFI IPv4
                                 Protocol driver configuration policy completes
                                 execution. This event must be of type EVT_NOTIFY_SIGNAL.
  @param  ReconfigEvent          Event that will be signaled when the EFI IPv4
                                 Protocol driver configuration needs to be updated.
                                 This event must be of type EVT_NOTIFY_SIGNAL.

  @retval EFI_SUCCESS            The configuration policy for the EFI IPv4 Protocol
                                 driver is now running.
  @retval EFI_INVALID_PARAMETER  One or more of the following parameters is NULL:
                                  This
                                  DoneEvent
                                  ReconfigEvent
  @retval EFI_OUT_OF_RESOURCES   Required system resources could not be allocated.
  @retval EFI_ALREADY_STARTED    The configuration policy for the EFI IPv4 Protocol
                                 driver was already started.
  @retval EFI_DEVICE_ERROR       An unexpected system error or network error occurred.
  @retval EFI_UNSUPPORTED        This interface does not support the EFI IPv4 Protocol
                                 driver configuration.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigStart (
  IN EFI_IP4_CONFIG_PROTOCOL  *This,
  IN EFI_EVENT                DoneEvent,
  IN EFI_EVENT                ReconfigEvent
  );

/**
  Stops running the configuration policy for the EFI IPv4 Protocol driver.

  The Stop() function stops the configuration policy for the EFI IPv4 Protocol driver.
  All configuration data will be lost after calling Stop().

  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.

  @retval EFI_SUCCESS            The configuration policy for the EFI IPv4 Protocol
                                 driver has been stopped.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        The configuration policy for the EFI IPv4 Protocol
                                 driver was not started.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigStop (
  IN EFI_IP4_CONFIG_PROTOCOL  *This
  );

/**
  Returns the default configuration data (if any) for the EFI IPv4 Protocol driver.

  The GetData() function returns the current configuration data for the EFI IPv4
  Protocol driver after the configuration policy has completed.

  @param  This                   Pointer to the EFI_IP4_CONFIG_PROTOCOL instance.
  @param  ConfigDataSize         On input, the size of the ConfigData buffer.
                                 On output, the count of bytes that were written
                                 into the ConfigData buffer.
  @param  ConfigData             Pointer to the EFI IPv4 Configuration Protocol
                                 driver configuration data structure.
                                 Type EFI_IP4_IPCONFIG_DATA is defined in
                                 "Related Definitions" below.

  @retval EFI_SUCCESS            The EFI IPv4 Protocol driver configuration has been returned.
  @retval EFI_INVALID_PARAMETER  This is NULL.
  @retval EFI_NOT_STARTED        The configuration policy for the EFI IPv4 Protocol
                                 driver is not running.
  @retval EFI_NOT_READY          EFI IPv4 Protocol driver configuration is still running.
  @retval EFI_ABORTED            EFI IPv4 Protocol driver configuration could not complete.
                                 Currently not implemented.
  @retval EFI_BUFFER_TOO_SMALL   *ConfigDataSize is smaller than the configuration
                                 data buffer or ConfigData is NULL.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigGetData (
  IN  EFI_IP4_CONFIG_PROTOCOL *This,
  IN  OUT  UINTN              *ConfigDataSize,
  OUT EFI_IP4_IPCONFIG_DATA   *ConfigData           OPTIONAL
  );

/**
  A dedicated timer is used to poll underlying media status. In case of
  cable swap, a new round auto configuration will be initiated. The timer 
  will signal the IP4 to run the auto configuration again. IP4 driver will free
  old IP address related resource, such as route table and Interface, then
  initiate a DHCP round by IP4Config->Start to acquire new IP, eventually
  create route table for new IP address.

  @param[in]  Event                  The IP4 service instance's heart beat timer.
  @param[in]  Context                The IP4 service instance.

**/
VOID
EFIAPI
MediaChangeDetect (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

#endif
