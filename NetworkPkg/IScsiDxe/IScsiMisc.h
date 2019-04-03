/** @file
  Miscellaneous definitions for iSCSI driver.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_MISC_H_
#define _ISCSI_MISC_H_

typedef struct _ISCSI_DRIVER_DATA ISCSI_DRIVER_DATA;

///
/// IPv4 Device Path Node Length
///
#define IP4_NODE_LEN_NEW_VERSIONS    27

///
/// IPv6 Device Path Node Length
///
#define IP6_NODE_LEN_OLD_VERSIONS    43
#define IP6_NODE_LEN_NEW_VERSIONS    60

///
/// The ignored field StaticIpAddress's offset in old IPv6 Device Path
///
#define IP6_OLD_IPADDRESS_OFFSET      42


#pragma pack(1)
typedef struct _ISCSI_SESSION_CONFIG_NVDATA {
  UINT16            TargetPort;
  UINT8             Enabled;
  UINT8             IpMode;

  EFI_IP_ADDRESS    LocalIp;
  EFI_IPv4_ADDRESS  SubnetMask;
  EFI_IP_ADDRESS    Gateway;

  BOOLEAN           InitiatorInfoFromDhcp;
  BOOLEAN           TargetInfoFromDhcp;

  CHAR8             TargetName[ISCSI_NAME_MAX_SIZE];
  EFI_IP_ADDRESS    TargetIp;
  UINT8             PrefixLength;
  UINT8             BootLun[8];

  UINT16            ConnectTimeout; ///< timout value in milliseconds.
  UINT8             ConnectRetryCount;
  UINT8             IsId[6];

  BOOLEAN           RedirectFlag;
  UINT16            OriginalTargetPort;     // The port of proxy/virtual target.
  EFI_IP_ADDRESS    OriginalTargetIp;       // The address of proxy/virtual target.

  BOOLEAN           DnsMode;  // Flag indicate whether the Target address is expressed as URL format.
  CHAR8             TargetUrl[ISCSI_TARGET_URI_MAX_SIZE];

} ISCSI_SESSION_CONFIG_NVDATA;
#pragma pack()

/**
  Calculate the prefix length of the IPv4 subnet mask.

  @param[in]  SubnetMask The IPv4 subnet mask.

  @return The prefix length of the subnet mask.
  @retval 0 Other errors as indicated.

**/
UINT8
IScsiGetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  );

/**
  Convert the hexadecimal encoded LUN string into the 64-bit LUN.

  @param[in]   Str             The hexadecimal encoded LUN string.
  @param[out]  Lun             Storage to return the 64-bit LUN.

  @retval EFI_SUCCESS           The 64-bit LUN is stored in Lun.
  @retval EFI_INVALID_PARAMETER The string is malformatted.

**/
EFI_STATUS
IScsiAsciiStrToLun (
  IN  CHAR8  *Str,
  OUT UINT8  *Lun
  );

/**
  Convert the 64-bit LUN into the hexadecimal encoded LUN string.

  @param[in]   Lun    The 64-bit LUN.
  @param[out]  String The storage to return the hexadecimal encoded LUN string.

**/
VOID
IScsiLunToUnicodeStr (
  IN UINT8    *Lun,
  OUT CHAR16  *String
  );

/**
  Convert the mac address into a hexadecimal encoded "-" seperated string.

  @param[in]  Mac     The mac address.
  @param[in]  Len     Length in bytes of the mac address.
  @param[in]  VlanId  VLAN ID of the network device.
  @param[out] Str     The storage to return the mac string.

**/
VOID
IScsiMacAddrToStr (
  IN  EFI_MAC_ADDRESS  *Mac,
  IN  UINT32           Len,
  IN  UINT16           VlanId,
  OUT CHAR16           *Str
  );

/**
  Convert the formatted IP address into the binary IP address.

  @param[in]   Str               The UNICODE string.
  @param[in]   IpMode            Indicates whether the IP address is v4 or v6.
  @param[out]  Ip                The storage to return the ASCII string.

  @retval EFI_SUCCESS            The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER  The IP string is malformatted or IpMode is
                                 invalid.

**/
EFI_STATUS
IScsiAsciiStrToIp (
  IN  CHAR8             *Str,
  IN  UINT8             IpMode,
  OUT EFI_IP_ADDRESS    *Ip
  );

/**
  Convert the binary encoded buffer into a hexadecimal encoded string.

  @param[in]       BinBuffer   The buffer containing the binary data.
  @param[in]       BinLength   Length of the binary buffer.
  @param[in, out]  HexStr      Pointer to the string.
  @param[in, out]  HexLength   The length of the string.

  @retval EFI_SUCCESS          The binary data is converted to the hexadecimal string
                               and the length of the string is updated.
  @retval EFI_BUFFER_TOO_SMALL The string is too small.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.

**/
EFI_STATUS
IScsiBinToHex (
  IN     UINT8  *BinBuffer,
  IN     UINT32 BinLength,
  IN OUT CHAR8  *HexStr,
  IN OUT UINT32 *HexLength
  );

/**
  Convert the hexadecimal string into a binary encoded buffer.

  @param[in, out]  BinBuffer   The binary buffer.
  @param[in, out]  BinLength   Length of the binary buffer.
  @param[in]       HexStr      The hexadecimal string.

  @retval EFI_SUCCESS          The hexadecimal string is converted into a binary
                               encoded buffer.
  @retval EFI_BUFFER_TOO_SMALL The binary buffer is too small to hold the converted data.

**/
EFI_STATUS
IScsiHexToBin (
  IN OUT UINT8  *BinBuffer,
  IN OUT UINT32 *BinLength,
  IN     CHAR8  *HexStr
  );


/**
  Convert the decimal-constant string or hex-constant string into a numerical value.

  @param[in] Str                    String in decimal or hex.

  @return The numerical value.

**/
UINTN
IScsiNetNtoi (
  IN     CHAR8  *Str
  );

/**
  Generate random numbers.

  @param[in, out]  Rand       The buffer to contain random numbers.
  @param[in]       RandLength The length of the Rand buffer.

**/
VOID
IScsiGenRandom (
  IN OUT UINT8  *Rand,
  IN     UINTN  RandLength
  );

/**
  Record the NIC information in a global structure.

  @param[in]  Controller         The handle of the controller.
  @param[in]  Image              Handle of the image.

  @retval EFI_SUCCESS            The operation is completed.
  @retval EFI_OUT_OF_RESOURCES   Do not have sufficient resource to finish this
                                 operation.

**/
EFI_STATUS
IScsiAddNic (
  IN EFI_HANDLE  Controller,
  IN EFI_HANDLE  Image
  );

/**
  Delete the recorded NIC information from a global structure. Also delete corresponding
  attempts.

  @param[in]  Controller         The handle of the controller.

  @retval EFI_SUCCESS            The operation completed.
  @retval EFI_NOT_FOUND          The NIC information to be deleted is not recorded.

**/
EFI_STATUS
IScsiRemoveNic (
  IN EFI_HANDLE  Controller
  );

/**
  Create and initialize the Attempts.

  @param[in]  AttemptNum          The number of Attempts will be created.

  @retval EFI_SUCCESS             The Attempts have been created successfully.
  @retval Others                  Failed to create the Attempt.

**/
EFI_STATUS
IScsiCreateAttempts (
  IN UINTN            AttemptNum
  );

/**
  Create the iSCSI configuration Keywords for each attempt.

  @param[in]  KeywordNum          The number Sets of Keywords will be created.

  @retval EFI_SUCCESS             The operation is completed.
  @retval Others                  Failed to create the Keywords.

**/
EFI_STATUS
IScsiCreateKeywords (
  IN UINTN            KeywordNum
  );

/**

  Free the attempt configure data variable.

**/
VOID
IScsiCleanAttemptVariable (
  IN   VOID
  );

/**
  Get the recorded NIC information from a global structure by the Index.

  @param[in]  NicIndex          The index indicates the position of NIC info.

  @return Pointer to the NIC info or NULL if not found.

**/
ISCSI_NIC_INFO *
IScsiGetNicInfoByIndex (
  IN UINT8      NicIndex
  );


/**
  Get the NIC's PCI location and return it according to the composited
  format defined in iSCSI Boot Firmware Table.

  @param[in]   Controller        The handle of the controller.
  @param[out]  Bus               The bus number.
  @param[out]  Device            The device number.
  @param[out]  Function          The function number.

  @return      The composited representation of the NIC PCI location.

**/
UINT16
IScsiGetNICPciLocation (
  IN EFI_HANDLE  Controller,
  OUT UINTN      *Bus,
  OUT UINTN      *Device,
  OUT UINTN      *Function
  );

/**
  Read the EFI variable (VendorGuid/Name) and return a dynamically allocated
  buffer, and the size of the buffer. If failure, return NULL.

  @param[in]   Name                   String part of EFI variable name.
  @param[in]   VendorGuid             GUID part of EFI variable name.
  @param[out]  VariableSize           Returns the size of the EFI variable that was read.

  @return Dynamically allocated memory that contains a copy of the EFI variable.
  @return Caller is responsible freeing the buffer.
  @retval NULL                   Variable was not read.

**/
VOID *
IScsiGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

/**
  Create the iSCSI driver data.

  @param[in] Image      The handle of the driver image.
  @param[in] Controller The handle of the controller.

  @return The iSCSI driver data created.
  @retval NULL Other errors as indicated.

**/
ISCSI_DRIVER_DATA *
IScsiCreateDriverData (
  IN EFI_HANDLE  Image,
  IN EFI_HANDLE  Controller
  );

/**
  Clean the iSCSI driver data.

  @param[in]              Private The iSCSI driver data.

  @retval EFI_SUCCES      The clean operation is successful.
  @retval Others          Other errors as indicated.

**/
EFI_STATUS
IScsiCleanDriverData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Check wheather the Controller handle is configured to use DHCP protocol.

  @param[in]  Controller           The handle of the controller.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval TRUE                     The handle of the controller need the Dhcp protocol.
  @retval FALSE                    The handle of the controller does not need the Dhcp protocol.

**/
BOOLEAN
IScsiDhcpIsConfigured (
  IN EFI_HANDLE  Controller,
  IN UINT8       IpVersion
  );

/**
  Check wheather the Controller handle is configured to use DNS protocol.

  @param[in]  Controller           The handle of the controller.

  @retval TRUE                     The handle of the controller need the DNS protocol.
  @retval FALSE                    The handle of the controller does not need the DNS protocol.

**/
BOOLEAN
IScsiDnsIsConfigured (
  IN EFI_HANDLE  Controller
  );

/**
  Get the various configuration data of this iSCSI instance.

  @param[in]  Private   The iSCSI driver data.

  @retval EFI_SUCCESS   Obtained the configuration of this instance.
  @retval EFI_ABORTED   The operation was aborted.
  @retval Others        Other errors as indicated.

**/
EFI_STATUS
IScsiGetConfigData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Get the device path of the iSCSI tcp connection and update it.

  @param[in]  Session The iSCSI session data.

  @return The updated device path.
  @retval NULL Other errors as indicated.

**/
EFI_DEVICE_PATH_PROTOCOL *
IScsiGetTcpConnDevicePath (
  IN ISCSI_SESSION      *Session
  );

/**
  Abort the session when the transition from BS to RT is initiated.

  @param[in]   Event  The event signaled.
  @param[in]  Context The iSCSI driver data.

**/
VOID
EFIAPI
IScsiOnExitBootService (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Tests whether a controller handle is being managed by IScsi driver.

  This function tests whether the driver specified by DriverBindingHandle is
  currently managing the controller specified by ControllerHandle.  This test
  is performed by evaluating if the the protocol specified by ProtocolGuid is
  present on ControllerHandle and is was opened by DriverBindingHandle and Nic
  Device handle with an attribute of EFI_OPEN_PROTOCOL_BY_DRIVER.
  If ProtocolGuid is NULL, then ASSERT().

  @param  ControllerHandle     A handle for a controller to test.
  @param  DriverBindingHandle  Specifies the driver binding handle for the
                               driver.
  @param  ProtocolGuid         Specifies the protocol that the driver specified
                               by DriverBindingHandle opens in its Start()
                               function.

  @retval EFI_SUCCESS          ControllerHandle is managed by the driver
                               specified by DriverBindingHandle.
  @retval EFI_UNSUPPORTED      ControllerHandle is not managed by the driver
                               specified by DriverBindingHandle.

**/
EFI_STATUS
EFIAPI
IScsiTestManagedDevice (
  IN  EFI_HANDLE       ControllerHandle,
  IN  EFI_HANDLE       DriverBindingHandle,
  IN  EFI_GUID         *ProtocolGuid
  );
#endif
