/** @file
  EFI Network Interface Identifier Protocol

  Copyright (c) 2006 - 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


**/

#ifndef __EFI_NETWORK_INTERFACE_IDENTIFER_H__
#define __EFI_NETWORK_INTERFACE_IDENTIFER_H__


#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID \
  { \
    0xE18541CD, 0xF755, 0x4f73, {0x92, 0x8D, 0x64, 0x3C, 0x8A, 0x79, 0xB2, 0x29 } \
  }

#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION    0x00010000

///
/// Revision defined in EFI1.1.
/// 
#define EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE_REVISION   EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION

///
/// Forward reference for pure ANSI compatability
///
typedef struct _EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL  EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL;

///
/// Protocol defined in EFI1.1.
/// 
typedef EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL   EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE;

///
/// An optional protocol that is used to describe details about the software 
/// layer that is used to produce the Simple Network Protocol. 
///
struct _EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL {
  UINT64    Revision;   ///< The revision of the EFI_NETWORK_INTERFACE_IDENTIFIER protocol.
  UINT64    Id;         ///< Address of the first byte of the identifying structure for this network 
                        ///< interface. This is only valid when the network interface is started 
                        ///< (see Start()). When the network interface is not started, this field is set to zero.
  UINT64    ImageAddr;  ///< Address of the first byte of the identifying structure for this
                        ///< network interface.  This is set to zero if there is no structure.
  UINT32    ImageSize;  ///< Size of unrelocated network interface image.
  CHAR8     StringId[4];///< A four-character ASCII string that is sent in the class identifier field of
                        ///< option 60 in DHCP. For a Type of EfiNetworkInterfaceUndi, this field is UNDI.
  UINT8     Type;       ///< Network interface type. This will be set to one of the values 
                        ///< in EFI_NETWORK_INTERFACE_TYPE.
  UINT8     MajorVer;   ///< Major version number.
  UINT8     MinorVer;   ///< Minor version number.
  BOOLEAN   Ipv6Supported; ///< TRUE if the network interface supports IPv6; otherwise FALSE.
  UINT8     IfNum;      ///< The network interface number that is being identified by this Network 
                        ///< Interface Identifier Protocol. This field must be less than or equal 
                        ///< to the IFcnt field in the !PXE structure.

};

///
///*******************************************************
/// EFI_NETWORK_INTERFACE_TYPE
///*******************************************************
///
typedef enum {
  EfiNetworkInterfaceUndi = 1
} EFI_NETWORK_INTERFACE_TYPE;

extern EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid;
extern EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31;

#endif
