/** @file
  The assistant function declaration for IpSecConfig application.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HELPER_H_
#define _HELPER_H_

#define  FORMAT_NUMBER 0x1
#define  FORMAT_STRING 0x2

/**
  Helper function called to change input parameter in string format to number.

  @param[in]      FlagStr         The pointer to the flag string.
  @param[in]      Maximum         most value number.
  @param[in, out] ValuePtr        The pointer to the input parameter in string format.
  @param[in]      ByteCount       The valid byte count
  @param[in]      Map             The pointer to the STR2INT table.
  @param[in]      ParamPackage    The pointer to the ParamPackage list.
  @param[in]      FormatMask      The bit mask.
                                  BIT 0 set indicates the value of flag might be number.
                                  BIT 1 set indicates the value of flag might be a string that needs to be looked up.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_NOT_FOUND            The input parameter can't be found.
  @retval EFI_INVALID_PARAMETER    The input parameter is an invalid input.
**/
EFI_STATUS
GetNumber (
  IN     CHAR16        *FlagStr,
  IN     UINT64        Maximum,
  IN OUT VOID          *ValuePtr,
  IN     UINTN         ByteCount,
  IN     STR2INT       *Map,
  IN     LIST_ENTRY    *ParamPackage,
  IN     UINT32        FormatMask
  );

/**
  Helper function called to convert a string containing an (Ipv4) Internet Protocol dotted address
  into a proper address for the EFI_IP_ADDRESS structure.

  @param[in]  Ptr    The pointer to the string containing an (Ipv4) Internet Protocol dotted address.
  @param[out] Ip     The pointer to the Ip address structure to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
**/
EFI_STATUS
EfiInetAddr2 (
  IN  CHAR16            *Ptr,
  OUT EFI_IP_ADDRESS    *Ip
  );

/**
  Helper function called to calculate the prefix length associated with the string
  containing an Ipv4 or Ipv6 Internet Protocol address.

  @param[in]  Ptr     The pointer to the string containing an Ipv4 or Ipv6 Internet Protocol address.
  @param[out] Addr    The pointer to the EFI_IP_ADDRESS_INFO structure to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval Others                   Other mistake case.
**/
EFI_STATUS
EfiInetAddrRange (
  IN  CHAR16                 *Ptr,
  OUT EFI_IP_ADDRESS_INFO    *Addr
  );

/**
  Helper function called to calculate the port range associated with the string.

  @param[in]  Ptr          The pointer to the string containing a port and range.
  @param[out] Port         The pointer to the Port to contain the result.
  @param[out] PortRange    The pointer to the PortRange to contain the result.

  @retval EFI_SUCCESS              The operation completed successfully.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
  @retval Others                   Other mistake case.
**/
EFI_STATUS
EfiInetPortRange (
  IN  CHAR16    *Ptr,
  OUT UINT16    *Port,
  OUT UINT16    *PortRange
  );

/**
  Helper function called to transfer a string to an unsigned integer.

  @param[in]  Str       The pointer to the string.
  @param[out] Status    The operation status.

  @return The integer value of a converted str.
**/
UINT64
StrToUInteger (
  IN  CONST CHAR16    *Str,
  OUT EFI_STATUS      *Status
  );

/**
  Helper function called to transfer a string to an unsigned integer according to the map table.

  @param[in] Str    The pointer to the string.
  @param[in] Map    The pointer to the map table.

  @return The integer value of converted str. If not found, then return -1.
**/
UINT32
MapStringToInteger (
  IN CONST CHAR16    *Str,
  IN STR2INT         *Map
  );

/**
  Helper function called to transfer an unsigned integer to a string according to the map table.

  @param[in] Integer    The pointer to the string.
  @param[in] Map        The pointer to the map table.

  @return The converted str. If not found, then return NULL.
**/
CHAR16 *
MapIntegerToString (
  IN UINT32     Integer,
  IN STR2INT    *Map
  );

#endif
