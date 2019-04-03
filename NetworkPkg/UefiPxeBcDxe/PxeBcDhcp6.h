/** @file
  Functions declaration related with DHCPv6 for UefiPxeBc Driver.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_PXEBC_DHCP6_H__
#define __EFI_PXEBC_DHCP6_H__

#define PXEBC_DHCP6_OPTION_MAX_NUM        16
#define PXEBC_DHCP6_OPTION_MAX_SIZE       312
#define PXEBC_DHCP6_PACKET_MAX_SIZE       (sizeof (EFI_PXE_BASE_CODE_PACKET))
#define PXEBC_IP6_POLICY_MAX              0xff
#define PXEBC_IP6_ROUTE_TABLE_TIMEOUT     10

#define PXEBC_DHCP6_S_PORT                547
#define PXEBC_DHCP6_C_PORT                546

#define PXEBC_DHCP6_ENTERPRISE_NUM        343   // TODO: IANA TBD: temporarily using Intel's
#define PXEBC_DHCP6_MAX_BOOT_FILE_SIZE    65535 //   It's a limitation of bit length, 65535*512 bytes.


#define PXEBC_DHCP6_IDX_IA_NA             0
#define PXEBC_DHCP6_IDX_BOOT_FILE_URL     1
#define PXEBC_DHCP6_IDX_BOOT_FILE_PARAM   2
#define PXEBC_DHCP6_IDX_VENDOR_CLASS      3
#define PXEBC_DHCP6_IDX_DNS_SERVER        4
#define PXEBC_DHCP6_IDX_MAX               5

#define PXEBC_DHCP6_BOOT_FILE_URL_PREFIX  "tftp://"
#define PXEBC_TFTP_URL_SEPARATOR          '/'
#define PXEBC_ADDR_START_DELIMITER        '['
#define PXEBC_ADDR_END_DELIMITER          ']'

#define GET_NEXT_DHCP6_OPTION(Opt) \
  (EFI_DHCP6_PACKET_OPTION *) ((UINT8 *) (Opt) + \
  sizeof (EFI_DHCP6_PACKET_OPTION) + (NTOHS ((Opt)->OpLen)) - 1)

#define GET_DHCP6_OPTION_SIZE(Pkt)  \
  ((Pkt)->Length - sizeof (EFI_DHCP6_HEADER))

#define IS_PROXY_OFFER(Type) \
  ((Type) == PxeOfferTypeProxyBinl || \
   (Type) == PxeOfferTypeProxyPxe10 || \
   (Type) == PxeOfferTypeProxyWfm11a)


#pragma pack(1)
typedef struct {
  UINT16 OpCode[256];
} PXEBC_DHCP6_OPTION_ORO;

typedef struct {
  UINT8 Type;
  UINT8 MajorVer;
  UINT8 MinorVer;
} PXEBC_DHCP6_OPTION_UNDI;

typedef struct {
  UINT16 Type;
} PXEBC_DHCP6_OPTION_ARCH;

typedef struct {
  UINT8 ClassIdentifier[10];
  UINT8 ArchitecturePrefix[5];
  UINT8 ArchitectureType[5];
  UINT8 Lit3[1];
  UINT8 InterfaceName[4];
  UINT8 Lit4[1];
  UINT8 UndiMajor[3];
  UINT8 UndiMinor[3];
} PXEBC_CLASS_ID;

typedef struct {
  UINT32         Vendor;
  UINT16         ClassLen;
  PXEBC_CLASS_ID ClassId;
} PXEBC_DHCP6_OPTION_VENDOR_CLASS;

#pragma pack()

typedef union {
  PXEBC_DHCP6_OPTION_ORO            *Oro;
  PXEBC_DHCP6_OPTION_UNDI           *Undi;
  PXEBC_DHCP6_OPTION_ARCH           *Arch;
  PXEBC_DHCP6_OPTION_VENDOR_CLASS   *VendorClass;
} PXEBC_DHCP6_OPTION_ENTRY;

typedef struct {
  LIST_ENTRY              Link;
  EFI_DHCP6_PACKET_OPTION *Option;
  UINT8                   Precedence;
} PXEBC_DHCP6_OPTION_NODE;

#define PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE  (OFFSET_OF (EFI_DHCP6_PACKET, Dhcp6) + PXEBC_DHCP6_PACKET_MAX_SIZE)

typedef union {
  EFI_DHCP6_PACKET        Offer;
  EFI_DHCP6_PACKET        Ack;
  UINT8                   Buffer[PXEBC_CACHED_DHCP6_PACKET_MAX_SIZE];
} PXEBC_DHCP6_PACKET;

typedef struct {
  PXEBC_DHCP6_PACKET      Packet;
  PXEBC_OFFER_TYPE        OfferType;
  EFI_DHCP6_PACKET_OPTION *OptList[PXEBC_DHCP6_IDX_MAX];
} PXEBC_DHCP6_PACKET_CACHE;




/**
  Parse the Boot File URL option.

  @param[in]      Private             Pointer to PxeBc private data.
  @param[out]     FileName     The pointer to the boot file name.
  @param[in, out] SrvAddr      The pointer to the boot server address.
  @param[in]      BootFile     The pointer to the boot file URL option data.
  @param[in]      Length       Length of the boot file URL option data.

  @retval EFI_ABORTED     User canceled the operation.
  @retval EFI_SUCCESS     Selected the boot menu successfully.
  @retval EFI_NOT_READY   Read the input key from the keybroad has not finish.

**/
EFI_STATUS
PxeBcExtractBootFileUrl (
  IN PXEBC_PRIVATE_DATA      *Private,
     OUT UINT8               **FileName,
  IN OUT EFI_IPv6_ADDRESS    *SrvAddr,
  IN     CHAR8               *BootFile,
  IN     UINT16              Length
  );


/**
  Parse the Boot File Parameter option.

  @param[in]  BootFilePara      The pointer to the boot file parameter option data.
  @param[out] BootFileSize      The pointer to the parsed boot file size.

  @retval EFI_SUCCESS     Successfully obtained the boot file size from parameter option.
  @retval EFI_NOT_FOUND   Failed to extract the boot file size from parameter option.

**/
EFI_STATUS
PxeBcExtractBootFileParam (
  IN  CHAR8                  *BootFilePara,
  OUT UINT16                 *BootFileSize
  );


/**
  Parse the cached DHCPv6 packet, including all the options.

  @param[in]  Cache6           The pointer to a cached DHCPv6 packet.

  @retval     EFI_SUCCESS      Parsed the DHCPv6 packet successfully.
  @retval     EFI_DEVICE_ERROR Failed to parse and invalid packet.

**/
EFI_STATUS
PxeBcParseDhcp6Packet (
  IN PXEBC_DHCP6_PACKET_CACHE  *Cache6
  );


/**
  Register the ready address by Ip6Config protocol.

  @param[in]  Private             The pointer to the PxeBc private data.
  @param[in]  Address             The pointer to the ready address.

  @retval     EFI_SUCCESS         Registered the address succesfully.
  @retval     Others              Failed to register the address.

**/
EFI_STATUS
PxeBcRegisterIp6Address (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_IPv6_ADDRESS              *Address
  );


/**
  Unregister the address by Ip6Config protocol.

  @param[in]  Private             The pointer to the PxeBc private data.

**/
VOID
PxeBcUnregisterIp6Address (
  IN PXEBC_PRIVATE_DATA            *Private
  );


/**
  Build and send out the request packet for the bootfile, and parse the reply.

  @param[in]  Private               The pointer to the PxeBc private data.
  @param[in]  Type                  PxeBc option boot item type.
  @param[in]  Layer                 The pointer to the option boot item layer.
  @param[in]  UseBis                Use BIS or not.
  @param[in]  DestIp                The pointer to the server address.

  @retval     EFI_SUCCESS           Successfully discovered theboot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resource.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover boot file.

**/
EFI_STATUS
PxeBcDhcp6Discover (
  IN  PXEBC_PRIVATE_DATA            *Private,
  IN  UINT16                        Type,
  IN  UINT16                        *Layer,
  IN  BOOLEAN                       UseBis,
  IN  EFI_IP_ADDRESS                *DestIp
  );

/**
  Set the IP6 policy to Automatic.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS         Switch the IP policy succesfully.
  @retval     Others              Unexpect error happened.

**/
EFI_STATUS
PxeBcSetIp6Policy (
  IN PXEBC_PRIVATE_DATA            *Private
  );

/**
  This function will register the station IP address and flush IP instance to start using the new IP address.

  @param[in]  Private             The pointer to PXEBC_PRIVATE_DATA.

  @retval     EFI_SUCCESS         The new IP address has been configured successfully.
  @retval     Others              Failed to configure the address.

**/
EFI_STATUS
PxeBcSetIp6Address (
  IN  PXEBC_PRIVATE_DATA              *Private
  );

/**
  Start the DHCPv6 S.A.R.R. process to acquire the IPv6 address and other PXE boot information.

  @param[in]  Private           The pointer to the PxeBc private data.
  @param[in]  Dhcp6             The pointer to EFI_DHCP6_PROTOCOL.

  @retval EFI_SUCCESS           The S.A.R.R. process successfully finished.
  @retval Others                Failed to finish the S.A.R.R. process.

**/
EFI_STATUS
PxeBcDhcp6Sarr (
  IN PXEBC_PRIVATE_DATA            *Private,
  IN EFI_DHCP6_PROTOCOL            *Dhcp6
  );

#endif

