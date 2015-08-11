/** @file
  Miscellaneous definitions for iSCSI driver.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_MISC_H_
#define _ISCSI_MISC_H_

#include <Library/BaseLib.h>

typedef struct _ISCSI_SESSION_CONFIG_DATA ISCSI_SESSION_CONFIG_DATA;

///
/// IPv4 Device Path Node Length
///
#define IP4_NODE_LEN_NEW_VERSIONS    27

#pragma pack(1)
typedef struct {
  BOOLEAN           Enabled;

  BOOLEAN           InitiatorInfoFromDhcp;
  EFI_IPv4_ADDRESS  LocalIp;
  EFI_IPv4_ADDRESS  SubnetMask;
  EFI_IPv4_ADDRESS  Gateway;

  BOOLEAN           TargetInfoFromDhcp;
  CHAR8             TargetName[ISCSI_NAME_MAX_SIZE];
  EFI_IPv4_ADDRESS  TargetIp;
  UINT16            TargetPort;
  UINT8             BootLun[8];

  UINT8             IsId[6];
} ISCSI_SESSION_CONFIG_NVDATA;
#pragma pack()

struct _ISCSI_SESSION_CONFIG_DATA {
  ISCSI_SESSION_CONFIG_NVDATA NvData;

  EFI_IPv4_ADDRESS            PrimaryDns;
  EFI_IPv4_ADDRESS            SecondaryDns;
  EFI_IPv4_ADDRESS            DhcpServer;
};

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

  @param[in]   Lun The 64-bit LUN.
  @param[out]  Str The storage to return the hexadecimal encoded LUN string.
**/
VOID
IScsiLunToUnicodeStr (
  IN UINT8    *Lun,
  OUT CHAR16  *Str
  );

/**
  Convert the ASCII string into a UNICODE string.

  @param[in]   Source      The ASCII string.
  @param[out]  Destination The storage to return the UNICODE string.

  @return CHAR16 *         Pointer to the UNICODE string.
**/
CHAR16 *
IScsiAsciiStrToUnicodeStr (
  IN  CHAR8   *Source,
  OUT CHAR16  *Destination
  );

/**
  Convert the UNICODE string into an ASCII string.

  @param[in]  Source       The UNICODE string.
  @param[out] Destination  The storage to return the ASCII string.

  @return CHAR8 *          Pointer to the ASCII string.
**/
CHAR8 *
IScsiUnicodeStrToAsciiStr (
  IN  CHAR16  *Source,
  OUT CHAR8   *Destination
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
  Convert the decimal dotted IPv4 address into the binary IPv4 address.

  @param[in]   Str             The UNICODE string.
  @param[out]  Ip              The storage to return the ASCII string.

  @retval EFI_SUCCESS           The binary IP address is returned in Ip.
  @retval EFI_INVALID_PARAMETER The IP string is malformatted.
**/
EFI_STATUS
IScsiAsciiStrToIp (
  IN  CHAR8             *Str,
  OUT EFI_IPv4_ADDRESS  *Ip
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
  Create the iSCSI driver data..

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

  @param[in]  Private The iSCSI driver data.
**/
VOID
IScsiCleanDriverData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Check wheather the Controller is configured to use DHCP protocol.

  @param[in]  Controller           The handle of the controller.
  
  @retval TRUE                     The handle of the controller need the Dhcp protocol.
  @retval FALSE                    The handle of the controller does not need the Dhcp protocol.
  
**/
BOOLEAN
IScsiDhcpIsConfigured (
  IN EFI_HANDLE  Controller
  );

/**
  Get the various configuration data of this iSCSI instance.

  @param[in]  Private   The iSCSI driver data.

  @retval EFI_SUCCESS   The configuration of this instance is got.
  @retval EFI_ABORTED   The operation was aborted.
  @retval Others        Other errors as indicated.
**/
EFI_STATUS
IScsiGetConfigData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Get the device path of the iSCSI tcp connection and update it.

  @param[in]  Private The iSCSI driver data.

  @return The updated device path.
  @retval NULL Other errors as indicated.
**/
EFI_DEVICE_PATH_PROTOCOL *
IScsiGetTcpConnDevicePath (
  IN ISCSI_DRIVER_DATA  *Private
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
