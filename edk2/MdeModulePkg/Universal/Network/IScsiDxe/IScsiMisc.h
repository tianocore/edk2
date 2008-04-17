/** @file
  Miscellaneous definitions for IScsi driver.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiMisc.h

Abstract:

  Miscellaneous definitions for IScsi driver.

**/

#ifndef _ISCSI_MISC_H_
#define _ISCSI_MISC_H_

#pragma pack(1)
typedef struct _ISCSI_SESSION_CONFIG_NVDATA {
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

  @param  SubnetMask[in] The IPv4 subnet mask.

  @retval UINT8          The prefix length of the subnet mask.

**/
UINT8
IScsiGetSubnetMaskPrefixLength (
  IN EFI_IPv4_ADDRESS  *SubnetMask
  );

/**
  Convert the hexadecimal encoded LUN string into the 64-bit LUN. 

  @param  Str[in]               The hexadecimal encoded LUN string.

  @param  Lun[out]              Storage to return the 64-bit LUN.

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

  @param  Lun[in]  The 64-bit LUN.

  @param  Str[out] The storage to return the hexadecimal encoded LUN string.

  @retval None.

**/
VOID
IScsiLunToUnicodeStr (
  IN UINT8    *Lun,
  OUT CHAR16  *String
  );

/**
  Convert the ASCII string into a UNICODE string.

  @param  Source[out]      The ASCII string.

  @param  Destination[out] The storage to return the UNICODE string.

  @retval CHAR16 *         Pointer to the UNICODE string.

**/
CHAR16 *
IScsiAsciiStrToUnicodeStr (
  IN  CHAR8   *Source,
  OUT CHAR16  *Destination
  );

/**
  Convert the UNICODE string into an ASCII string.

  @param  Source[in]       The UNICODE string.

  @param  Destination[out] The storage to return the ASCII string.

  @retval CHAR8 *          Pointer to the ASCII string.

**/
CHAR8 *
IScsiUnicodeStrToAsciiStr (
  IN  CHAR16  *Source,
  OUT CHAR8   *Destination
  );

/**
  Convert the mac address into a hexadecimal encoded "-" seperated string.

  @param  Mac[in]  The mac address.

  @param  Len[in]  Length in bytes of the mac address.

  @param  Str[out] The storage to return the mac string.

  @retval None.

**/
VOID
IScsiMacAddrToStr (
  IN  EFI_MAC_ADDRESS  *Mac,
  IN  UINT32           Len,
  OUT CHAR16           *Str
  );

/**
  Convert the decimal dotted IPv4 address into the binary IPv4 address.

  @param  Str[in]               The UNICODE string.

  @param  Ip[out]               The storage to return the ASCII string.

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

  @param  BinBuffer[in]        The buffer containing the binary data.

  @param  BinLength[in]        Length of the binary buffer.

  @param  HexStr[in][out]      Pointer to the string.

  @param  HexLength[in][out]   The length of the string.

  @retval EFI_SUCCESS          The binary data is converted to the hexadecimal string
                               and the length of the string is updated.

  @retval EFI_BUFFER_TOO_SMALL The string is too small.

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

  @param  BinBuffer[in][out]   The binary buffer.

  @param  BinLength[in][out]   Length of the binary buffer.

  @param  HexStr[in]           The hexadecimal string.

  @retval EFI_SUCCESS          The hexadecimal string is converted into a binary
                               encoded buffer.

  @retval EFI_BUFFER_TOO_SMALL The binary buffer is too small to hold the converted data.s

**/
EFI_STATUS
IScsiHexToBin (
  IN OUT UINT8  *BinBuffer,
  IN OUT UINT32 *BinLength,
  IN     CHAR8  *HexStr
  );

/**
  Generate random numbers.

  @param  Rand[in][out]  The buffer to contain random numbers.

  @param  RandLength[in] The length of the Rand buffer.

  @retval None.

**/
VOID
IScsiGenRandom (
  IN OUT UINT8  *Rand,
  IN     UINTN  RandLength
  );

/**
  Create the iSCSI driver data..

  @param  Image[in]      The handle of the driver image.

  @param  Controller[in] The handle of the controller.

  @retval The iSCSI driver data created.

**/
ISCSI_DRIVER_DATA *
IScsiCreateDriverData (
  IN EFI_HANDLE  Image,
  IN EFI_HANDLE  Controller
  );

/**
  Clean the iSCSI driver data.

  @param  Private[in] The iSCSI driver data.

  @retval None.

**/
VOID
IScsiCleanDriverData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**

  Get the various configuration data of this iSCSI instance.

  @param  Private[in]   The iSCSI driver data.

  @retval EFI_SUCCESS   The configuration of this instance is got.

  @retval EFI_NOT_FOUND This iSCSI instance is not configured yet.

**/
EFI_STATUS
IScsiGetConfigData (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Get the device path of the iSCSI tcp connection and update it.

  @param  Private[in] The iSCSI driver data.

  @retval The updated device path.

**/
EFI_DEVICE_PATH_PROTOCOL  *
IScsiGetTcpConnDevicePath (
  IN ISCSI_DRIVER_DATA  *Private
  );

/**
  Abort the session when the transition from BS to RT is initiated.

  @param  Event[in]   The event signaled.

  @param  Context[in] The iSCSI driver data.

  @retval None.

**/
VOID
EFIAPI
IScsiOnExitBootService (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

extern CHAR16 NibbleToHexChar(UINT8 Nibble);

#endif
