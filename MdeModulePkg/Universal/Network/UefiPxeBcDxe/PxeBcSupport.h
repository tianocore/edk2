/** @file
  Support routines for PxeBc.
Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_PXEBC_SUPPORT_H__
#define __EFI_PXEBC_SUPPORT_H__


/**
  The common notify function associated with various PxeBc events. 

  @param  Event     The event signaled.
  @param  Context   The context.

**/
VOID
EFIAPI
PxeBcCommonNotify (
  IN EFI_EVENT           Event,
  IN VOID                *Context
  );


/**
  This function initialize(or configure) the Udp4Write instance.
  
  @param  Udp4       Pointer to the EFI_UDP4_PROTOCOL instance.
  @param  StationIp  Pointer to the station ip address.
  @param  SubnetMask Pointer to the subnetmask of the station ip address.
  @param  Gateway    Pointer to the gateway ip address.
  @param  SrcPort    Pointer to the srouce port of the station.
  
  @retval EFI_SUCCESS           The configuration settings were set, changed, or reset successfully.
  @retval EFI_NO_MAPPING        When using a default address, configuration (DHCP, BOOTP,
                                RARP, etc.) is not finished yet.
  @retval EFI_INVALID_PARAMETER One or more following conditions are TRUE:
  @retval EFI_ALREADY_STARTED   The EFI UDPv4 Protocol instance is already started/configured
                                and must be stopped/reset before it can be reconfigured.
  @retval EFI_ACCESS_DENIED     UdpConfigData. AllowDuplicatePort is FALSE
                                and UdpConfigData.StationPort is already used by
                                other instance.
  @retval EFI_OUT_OF_RESOURCES  The EFI UDPv4 Protocol driver cannot allocate memory for this
                                EFI UDPv4 Protocol instance.
  @retval EFI_DEVICE_ERROR      An unexpected network or system error occurred and this instance
                                was not opened.
  @retval Others                Please examine the function Udp4->Routes(Udp4, FALSE, &mZeroIp4Addr, &mZeroIp4Addr, Gateway) returns.
  
**/
EFI_STATUS
PxeBcConfigureUdpWriteInstance (
  IN EFI_UDP4_PROTOCOL  *Udp4,
  IN EFI_IPv4_ADDRESS   *StationIp,
  IN EFI_IPv4_ADDRESS   *SubnetMask,
  IN EFI_IPv4_ADDRESS   *Gateway,
  IN OUT UINT16         *SrcPort
  );
/**
  Convert number to ASCII value.

  @param  Number              Numeric value to convert to decimal ASCII value.
  @param  Buffer              Buffer to place ASCII version of the Number.
  @param  Length              Length of Buffer.

**/
VOID
CvtNum (
  IN UINTN  Number,
  IN UINT8  *Buffer,
  IN UINTN   Length
  );


/**
  Convert unsigned int number to decimal number.

  @param      Number         The unsigned int number will be converted.
  @param      Buffer         Pointer to the buffer to store the decimal number after transform.
  @param[in]  BufferSize     The maxsize of the buffer.
  
  @return the length of the number after transform.

**/
UINTN
UtoA10 (
  IN UINTN Number,
  IN CHAR8 *Buffer,
  IN UINTN BufferSize
  
  );


/**
  Convert ASCII numeric string to a UINTN value.

  @param  Buffer  Pointer to the 8-byte unsigned int value.

  @return UINTN value of the ASCII string.

**/
UINT64
AtoU64 (
  IN UINT8 *Buffer
  );


#endif

