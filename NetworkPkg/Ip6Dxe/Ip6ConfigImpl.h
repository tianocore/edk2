/** @file
  Definitions for EFI IPv6 Configuration Protocol implementation.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IP6_CONFIG_IMPL_H__
#define __IP6_CONFIG_IMPL_H__

#define IP6_CONFIG_INSTANCE_SIGNATURE     SIGNATURE_32 ('I', 'P', '6', 'C')
#define IP6_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('I', 'F', 'C', 'I')
#define IP6_CONFIG_VARIABLE_ATTRIBUTE     (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)

#define IP6_CONFIG_DEFAULT_DAD_XMITS  1

#define DATA_ATTRIB_SIZE_FIXED  0x1
#define DATA_ATTRIB_VOLATILE    0x2

#define DATA_ATTRIB_SET(Attrib, Bits)  (BOOLEAN)((Attrib) & (Bits))
#define SET_DATA_ATTRIB(Attrib, Bits)  ((Attrib) |= (Bits))

typedef struct _IP6_CONFIG_INSTANCE IP6_CONFIG_INSTANCE;

#define IP6_CONFIG_INSTANCE_FROM_PROTOCOL(Proto) \
  CR ((Proto), \
      IP6_CONFIG_INSTANCE, \
      Ip6Config, \
      IP6_CONFIG_INSTANCE_SIGNATURE \
      )

#define IP6_CONFIG_INSTANCE_FROM_FORM_CALLBACK(Callback) \
  CR ((Callback), \
      IP6_CONFIG_INSTANCE, \
      CallbackInfo, \
      IP6_CONFIG_INSTANCE_SIGNATURE \
      )

#define IP6_SERVICE_FROM_IP6_CONFIG_INSTANCE(Instance) \
  CR ((Instance), \
      IP6_SERVICE, \
      Ip6ConfigInstance, \
      IP6_SERVICE_SIGNATURE \
      )

#define IP6_FORM_CALLBACK_INFO_FROM_CONFIG_ACCESS(ConfigAccess) \
  CR ((ConfigAccess), \
      IP6_FORM_CALLBACK_INFO, \
      HiiConfigAccess, \
      IP6_FORM_CALLBACK_INFO_SIGNATURE \
      )

/**
  The prototype of work function for EfiIp6ConfigSetData().

  @param[in]     Instance The pointer to the IP6 config instance data.
  @param[in]     DataSize In bytes, the size of the buffer pointed to by Data.
  @param[in]     Data     The data buffer to set.

  @retval EFI_BAD_BUFFER_SIZE  The DataSize does not match the size of the type,
                               8 bytes.
  @retval EFI_SUCCESS          The specified configuration data for the EFI IPv6
                               network stack was set successfully.

**/
typedef
EFI_STATUS
(*IP6_CONFIG_SET_DATA) (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN UINTN                DataSize,
  IN VOID                 *Data
  );

/**
  The prototype of work function for EfiIp6ConfigGetData().

  @param[in]      Instance The pointer to the IP6 config instance data.
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
(*IP6_CONFIG_GET_DATA) (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN OUT UINTN            *DataSize,
  IN VOID                 *Data      OPTIONAL
  );

typedef union {
  VOID                                        *Ptr;
  EFI_IP6_CONFIG_INTERFACE_INFO               *IfInfo;
  EFI_IP6_CONFIG_INTERFACE_ID                 *AltIfId;
  EFI_IP6_CONFIG_POLICY                       *Policy;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    *DadXmits;
  EFI_IP6_CONFIG_MANUAL_ADDRESS               *ManualAddress;
  EFI_IPv6_ADDRESS                            *Gateway;
  EFI_IPv6_ADDRESS                            *DnsServers;
} IP6_CONFIG_DATA;

typedef struct {
  IP6_CONFIG_SET_DATA    SetData;
  IP6_CONFIG_GET_DATA    GetData;
  EFI_STATUS             Status;
  UINT8                  Attribute;
  NET_MAP                EventMap;
  IP6_CONFIG_DATA        Data;
  UINTN                  DataSize;
} IP6_CONFIG_DATA_ITEM;

typedef struct {
  UINT16                      Offset;
  UINT32                      DataSize;
  EFI_IP6_CONFIG_DATA_TYPE    DataType;
} IP6_CONFIG_DATA_RECORD;

#pragma pack(1)

//
// heap data that contains the data for each data record.
//
//  BOOLEAN                                   IsAltIfIdSet;
//  EFI_IP6_CONFIG_POLICY                     Policy;
//  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS  DadXmits;
//  UINT32                                    ManualaddressCount;
//  UINT32                                    GatewayCount;
//  UINT32                                    DnsServersCount;
//  EFI_IP6_CONFIG_INTERFACE_ID               AltIfId;
//  EFI_IP6_CONFIG_MANUAL_ADDRESS             ManualAddress[];
//  EFI_IPv6_ADDRESS                          Gateway[];
//  EFI_IPv6_ADDRESS                          DnsServers[];
//
typedef struct {
  UINT32                    IaId;
  UINT16                    Checksum;
  UINT16                    DataRecordCount;
  IP6_CONFIG_DATA_RECORD    DataRecord[1];
} IP6_CONFIG_VARIABLE;

#pragma pack()

typedef struct {
  LIST_ENTRY              Link;
  EFI_IP6_ADDRESS_INFO    AddrInfo;
} IP6_ADDRESS_INFO_ENTRY;

typedef struct {
  EFI_IP6_CONFIG_POLICY                       Policy;              ///< manual or automatic
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    DadTransmitCount;    ///< dad transmits count
  EFI_IP6_CONFIG_INTERFACE_ID                 InterfaceId;         ///< alternative interface id
  LIST_ENTRY                                  ManualAddress;       ///< IP addresses
  UINT32                                      ManualAddressCount;  ///< IP addresses count
  LIST_ENTRY                                  GatewayAddress;      ///< Gateway address
  UINT32                                      GatewayAddressCount; ///< Gateway address count
  LIST_ENTRY                                  DnsAddress;          ///< DNS server address
  UINT32                                      DnsAddressCount;     ///< DNS server address count
} IP6_CONFIG_NVDATA;

typedef struct _IP6_FORM_CALLBACK_INFO {
  UINT32                            Signature;
  EFI_HANDLE                        ChildHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    HiiConfigAccess;
  EFI_DEVICE_PATH_PROTOCOL          *HiiVendorDevicePath;
  EFI_HII_HANDLE                    RegisteredHandle;
} IP6_FORM_CALLBACK_INFO;

struct _IP6_CONFIG_INSTANCE {
  UINT32                                      Signature;
  BOOLEAN                                     Configured;
  LIST_ENTRY                                  Link;
  UINT16                                      IfIndex;

  EFI_IP6_CONFIG_INTERFACE_INFO               InterfaceInfo;
  EFI_IP6_CONFIG_INTERFACE_ID                 AltIfId;
  EFI_IP6_CONFIG_POLICY                       Policy;
  EFI_IP6_CONFIG_DUP_ADDR_DETECT_TRANSMITS    DadXmits;

  IP6_CONFIG_DATA_ITEM                        DataItem[Ip6ConfigDataTypeMaximum];
  NET_MAP                                     DadFailedMap;
  NET_MAP                                     DadPassedMap;

  EFI_IP6_CONFIG_PROTOCOL                     Ip6Config;

  EFI_EVENT                                   Dhcp6SbNotifyEvent;
  VOID                                        *Registration;
  EFI_HANDLE                                  Dhcp6Handle;
  EFI_DHCP6_PROTOCOL                          *Dhcp6;
  BOOLEAN                                     OtherInfoOnly;
  UINT32                                      IaId;
  EFI_EVENT                                   Dhcp6Event;
  UINT32                                      FailedIaAddressCount;
  EFI_IPv6_ADDRESS                            *DeclineAddress;
  UINT32                                      DeclineAddressCount;

  IP6_FORM_CALLBACK_INFO                      CallbackInfo;
  IP6_CONFIG_NVDATA                           Ip6NvData;
};

/**
  Read the configuration data from variable storage according to the VarName and
  gEfiIp6ConfigProtocolGuid. It checks the integrity of variable data. If the
  data is corrupted, it clears the variable data to ZERO. Otherwise, it outputs the
  configuration data to IP6_CONFIG_INSTANCE.

  @param[in]      VarName  The pointer to the variable name
  @param[in, out] Instance The pointer to the IP6 config instance data.

  @retval EFI_NOT_FOUND         The variable can not be found or already corrupted.
  @retval EFI_OUT_OF_RESOURCES  Fail to allocate resource to complete the operation.
  @retval EFI_SUCCESS           The configuration data was retrieved successfully.

**/
EFI_STATUS
Ip6ConfigReadConfigData (
  IN     CHAR16               *VarName,
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  );

/**
  The event process routine when the DHCPv6 server is answered with a reply packet
  for an information request.

  @param[in]     This          Points to the EFI_DHCP6_PROTOCOL.
  @param[in]     Context       The pointer to the IP6 configuration instance data.
  @param[in]     Packet        The DHCPv6 reply packet.

  @retval EFI_SUCCESS      The DNS server address was retrieved from the reply packet.
  @retval EFI_NOT_READY    The reply packet does not contain the DNS server option, or
                           the DNS server address is not valid.

**/
EFI_STATUS
EFIAPI
Ip6ConfigOnDhcp6Reply (
  IN EFI_DHCP6_PROTOCOL  *This,
  IN VOID                *Context,
  IN EFI_DHCP6_PACKET    *Packet
  );

/**
  The work function to trigger the DHCPv6 process to perform a stateful autoconfiguration.

  @param[in]     Instance      Pointer to the IP6 config instance data.
  @param[in]     OtherInfoOnly If FALSE, get stateful address and other information
                               via DHCPv6. Otherwise, only get the other information.

  @retval    EFI_SUCCESS       The operation finished successfully.
  @retval    EFI_UNSUPPORTED   The DHCP6 driver is not available.

**/
EFI_STATUS
Ip6ConfigStartStatefulAutoConfig (
  IN IP6_CONFIG_INSTANCE  *Instance,
  IN BOOLEAN              OtherInfoOnly
  );

/**
  Initialize an IP6_CONFIG_INSTANCE.

  @param[out]    Instance       The buffer of IP6_CONFIG_INSTANCE to be initialized.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resources to complete the operation.
  @retval EFI_SUCCESS           The IP6_CONFIG_INSTANCE initialized successfully.

**/
EFI_STATUS
Ip6ConfigInitInstance (
  OUT IP6_CONFIG_INSTANCE  *Instance
  );

/**
  Release an IP6_CONFIG_INSTANCE.

  @param[in, out] Instance    The buffer of IP6_CONFIG_INSTANCE to be freed.

**/
VOID
Ip6ConfigCleanInstance (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  );

/**
  Destroy the Dhcp6 child in IP6_CONFIG_INSTANCE and release the resources.

  @param[in, out] Instance    The buffer of IP6_CONFIG_INSTANCE to be freed.

  @retval EFI_SUCCESS         The child was successfully destroyed.
  @retval Others              Failed to destroy the child.

**/
EFI_STATUS
Ip6ConfigDestroyDhcp6 (
  IN OUT IP6_CONFIG_INSTANCE  *Instance
  );

#endif
