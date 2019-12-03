/** @file
  Definitions for EFI IPv4 Configuration II Protocol implementation.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IP4_CONFIG2_IMPL_H__
#define __IP4_CONFIG2_IMPL_H__

#define IP4_CONFIG2_INSTANCE_SIGNATURE    SIGNATURE_32 ('I', 'P', 'C', '2')
#define IP4_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('I', 'F', 'C', 'I')

#define IP4_CONFIG2_VARIABLE_ATTRIBUTE    (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)

#define DATA_ATTRIB_SIZE_FIXED              0x1
#define DATA_ATTRIB_VOLATILE                0x2

#define DATA_ATTRIB_SET(Attrib, Bits)       (BOOLEAN)((Attrib) & (Bits))
#define SET_DATA_ATTRIB(Attrib, Bits)       ((Attrib) |= (Bits))
#define REMOVE_DATA_ATTRIB(Attrib, Bits)    ((Attrib) &= (~Bits))

typedef struct _IP4_CONFIG2_INSTANCE IP4_CONFIG2_INSTANCE;

#define IP4_CONFIG2_INSTANCE_FROM_PROTOCOL(Proto) \
  CR ((Proto), \
      IP4_CONFIG2_INSTANCE, \
      Ip4Config2, \
      IP4_CONFIG2_INSTANCE_SIGNATURE \
      )

#define IP4_SERVICE_FROM_IP4_CONFIG2_INSTANCE(Instance) \
  CR ((Instance), \
      IP4_SERVICE, \
      Ip4Config2Instance, \
      IP4_SERVICE_SIGNATURE \
      )

#define IP4_CONFIG2_INSTANCE_FROM_FORM_CALLBACK(Callback) \
  CR ((Callback), \
      IP4_CONFIG2_INSTANCE, \
      CallbackInfo, \
      IP4_CONFIG2_INSTANCE_SIGNATURE \
      )

#define IP4_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(ConfigAccess) \
  CR ((ConfigAccess), \
      IP4_FORM_CALLBACK_INFO, \
      HiiConfigAccessProtocol, \
      IP4_FORM_CALLBACK_INFO_SIGNATURE \
      )

/**
  The prototype of work function for EfiIp4Config2SetData().

  @param[in]     Instance The pointer to the IP4 config2 instance data.
  @param[in]     DataSize In bytes, the size of the buffer pointed to by Data.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE  The DataSize does not match the size of the type,
                               8 bytes.
  @retval EFI_SUCCESS          The specified configuration data for the EFI IPv4
                               network stack was set successfully.

**/
typedef
EFI_STATUS
(*IP4_CONFIG2_SET_DATA) (
  IN IP4_CONFIG2_INSTANCE *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  );

/**
  The prototype of work function for EfiIp4Config2GetData().

  @param[in]      Instance The pointer to the IP4 config2 instance data.
  @param[in, out] DataSize On input, in bytes, the size of Data. On output, in
                           bytes, the size of buffer required to store the specified
                           configuration data.
  @param[in]      Data     The data buffer in which the configuration data is returned.
                           Ignored if DataSize is ZERO.

  @retval EFI_BUFFER_TOO_SMALL The size of Data is too small for the specified
                               configuration data, and the required size is
                               returned in DataSize.
  @retval EFI_SUCCESS          The specified configuration data was obtained successfully.

**/
typedef
EFI_STATUS
(*IP4_CONFIG2_GET_DATA) (
  IN IP4_CONFIG2_INSTANCE *Instance,
  IN OUT UINTN            *DataSize,
  IN VOID                 *Data      OPTIONAL
  );

typedef union {
  VOID                                      *Ptr;
  EFI_IP4_CONFIG2_INTERFACE_INFO            *IfInfo;
  EFI_IP4_CONFIG2_POLICY                    *Policy;
  EFI_IP4_CONFIG2_MANUAL_ADDRESS            *ManualAddress;
  EFI_IPv4_ADDRESS                          *Gateway;
  EFI_IPv4_ADDRESS                          *DnsServers;
} IP4_CONFIG2_DATA;

typedef struct {
  IP4_CONFIG2_SET_DATA SetData;
  IP4_CONFIG2_GET_DATA GetData;
  EFI_STATUS           Status;
  UINT8                Attribute;
  NET_MAP              EventMap;
  IP4_CONFIG2_DATA     Data;
  UINTN                DataSize;
} IP4_CONFIG2_DATA_ITEM;

typedef struct {
  UINT16                    Offset;
  UINT32                    DataSize;
  EFI_IP4_CONFIG2_DATA_TYPE DataType;
} IP4_CONFIG2_DATA_RECORD;

#pragma pack(1)

//
// heap data that contains the data for each data record.
//
//  EFI_IP4_CONFIG2_POLICY                    Policy;
//  UINT32                                    ManualaddressCount;
//  UINT32                                    GatewayCount;
//  UINT32                                    DnsServersCount;
//  EFI_IP4_CONFIG2_MANUAL_ADDRESS            ManualAddress[];
//  EFI_IPv4_ADDRESS                          Gateway[];
//  EFI_IPv4_ADDRESS                          DnsServers[];
//
typedef struct {
  UINT16                  Checksum;
  UINT16                  DataRecordCount;
  IP4_CONFIG2_DATA_RECORD DataRecord[1];
} IP4_CONFIG2_VARIABLE;

#pragma pack()

typedef struct {
  EFI_IP4_CONFIG2_POLICY                   Policy;               ///< manual or automatic
  EFI_IP4_CONFIG2_MANUAL_ADDRESS           *ManualAddress;       ///< IP addresses
  UINT32                                   ManualAddressCount;   ///< IP addresses count
  EFI_IPv4_ADDRESS                         *GatewayAddress;      ///< Gateway address
  UINT32                                   GatewayAddressCount;  ///< Gateway address count
  EFI_IPv4_ADDRESS                         *DnsAddress;          ///< DNS server address
  UINT32                                   DnsAddressCount;      ///< DNS server address count
} IP4_CONFIG2_NVDATA;

typedef struct _IP4_FORM_CALLBACK_INFO {
  UINT32                           Signature;
  EFI_HANDLE                       ChildHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL   HiiConfigAccessProtocol;
  EFI_DEVICE_PATH_PROTOCOL         *HiiVendorDevicePath;
  EFI_HII_HANDLE                   RegisteredHandle;
} IP4_FORM_CALLBACK_INFO;

struct _IP4_CONFIG2_INSTANCE {
  UINT32                                    Signature;
  BOOLEAN                                   Configured;
  LIST_ENTRY                                Link;
  UINT16                                    IfIndex;

  EFI_IP4_CONFIG2_PROTOCOL                  Ip4Config2;

  EFI_IP4_CONFIG2_INTERFACE_INFO            InterfaceInfo;
  EFI_IP4_CONFIG2_POLICY                    Policy;
  IP4_CONFIG2_DATA_ITEM                     DataItem[Ip4Config2DataTypeMaximum];

  EFI_EVENT                                 Dhcp4SbNotifyEvent;
  VOID                                      *Registration;
  EFI_HANDLE                                Dhcp4Handle;
  EFI_DHCP4_PROTOCOL                        *Dhcp4;
  BOOLEAN                                   DhcpSuccess;
  BOOLEAN                                   OtherInfoOnly;
  EFI_EVENT                                 Dhcp4Event;
  UINT32                                    FailedIaAddressCount;
  EFI_IPv4_ADDRESS                          *DeclineAddress;
  UINT32                                    DeclineAddressCount;

  IP4_FORM_CALLBACK_INFO                    CallbackInfo;

  IP4_CONFIG2_NVDATA                        Ip4NvData;
};

//
// Configure the DHCP to request the routers and netmask
// from server. The DHCP4_TAG_NETMASK is included in Head.
//
#pragma pack(1)
typedef struct {
  EFI_DHCP4_PACKET_OPTION Head;
  UINT8                   Route;
  UINT8                   Dns;
} IP4_CONFIG2_DHCP4_OPTION;
#pragma pack()

/**
  Read the configuration data from variable storage according to the VarName and
  gEfiIp4Config2ProtocolGuid. It checks the integrity of variable data. If the
  data is corrupted, it clears the variable data to ZERO. Othewise, it outputs the
  configuration data to IP4_CONFIG2_INSTANCE.

  @param[in]      VarName       The pointer to the variable name
  @param[in, out] Instance      The pointer to the IP4 config2 instance data.

  @retval EFI_NOT_FOUND         The variable can not be found or already corrupted.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data was retrieved successfully.

**/
EFI_STATUS
Ip4Config2ReadConfigData (
  IN     CHAR16               *VarName,
  IN OUT IP4_CONFIG2_INSTANCE *Instance
  );

/**
  Start the DHCP configuration for this IP service instance.
  It will locates the EFI_IP4_CONFIG2_PROTOCOL, then start the
  DHCP configuration.

  @param[in]  Instance           The IP4 config2 instance to configure.

  @retval EFI_SUCCESS            The auto configuration is successfully started.
  @retval Others                 Failed to start auto configuration.

**/
EFI_STATUS
Ip4StartAutoConfig (
  IN IP4_CONFIG2_INSTANCE   *Instance
  );

/**
  Initialize an IP4_CONFIG2_INSTANCE.

  @param[out]    Instance       The buffer of IP4_CONFIG2_INSTANCE to be initialized.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_SUCCESS           The IP4_CONFIG2_INSTANCE initialized successfully.

**/
EFI_STATUS
Ip4Config2InitInstance (
  OUT IP4_CONFIG2_INSTANCE  *Instance
  );

/**
  Release an IP4_CONFIG2_INSTANCE.

  @param[in, out] Instance    The buffer of IP4_CONFIG2_INSTANCE to be freed.

**/
VOID
Ip4Config2CleanInstance (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  );

/**
  Request Ip4AutoReconfigCallBackDpc as a DPC at TPL_CALLBACK.

  @param Event     The event that is signalled.
  @param Context   The IP4 service binding instance.

**/
VOID
EFIAPI
Ip4AutoReconfigCallBack (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

/**
  Destroy the Dhcp4 child in IP4_CONFIG2_INSTANCE and release the resources.

  @param[in, out] Instance    The buffer of IP4 config2 instance to be freed.

  @retval EFI_SUCCESS         The child was successfully destroyed.
  @retval Others              Failed to destroy the child.

**/
EFI_STATUS
Ip4Config2DestroyDhcp4 (
  IN OUT IP4_CONFIG2_INSTANCE  *Instance
  );

#endif
