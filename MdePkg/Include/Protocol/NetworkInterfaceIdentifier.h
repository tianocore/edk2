/** @file
  EFI Network Interface Identifier Protocol

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
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

typedef enum {
  EfiNetworkInterfaceUndi = 1
} EFI_NETWORK_PROTOCOL_TYPE;

/**  
  @par Protocol Description:
  An optional protocol that is used to describe details about the software 
  layer that is used to produce the Simple Network Protocol. 

  @param Revision
  The revision of the EFI_NETWORK_INTERFACE_IDENTIFIER protocol.

  @param Id
  Address of the first byte of the identifying structure for this network 
  interface. This is only valid when the network interface is started 
  (see Start()). When the network interface is not started, this field is set to zero.

  @param ImageAddr
  Address of the unrelocated network interface image.

  @param ImageSize
  Size of unrelocated network interface image.

  @param StringId
  A four-character ASCII string that is sent in the class identifier field of
  option 60 in DHCP. For a Type of EfiNetworkInterfaceUndi, this field is UNDI.

  @param Type
  Network interface type. This will be set to one of the values 
  in EFI_NETWORK_INTERFACE_TYPE.

  @param MajorVer
  Major version number.

  @param MinorVer
  Minor version number.

  @param Ipv6Supported
  TRUE if the network interface supports IPv6; otherwise FALSE.

  @param IfNum
  The network interface number that is being identified by this Network 
  Interface Identifier Protocol. This field must be less than or equal 
  to the IFcnt field in the !PXE structure.

**/
struct _EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL {

  UINT64  Revision;
  //
  // Revision of the network interface identifier protocol interface.
  //
  UINT64  ID;
  //
  // Address of the first byte of the identifying structure for this
  // network interface.  This is set to zero if there is no structure.
  //
  // For PXE/UNDI this is the first byte of the !PXE structure.
  //
  UINT64  ImageAddr;
  //
  // Address of the UNrelocated driver/ROM image.  This is set
  // to zero if there is no driver/ROM image.
  //
  // For 16-bit UNDI, this is the first byte of the option ROM in
  // upper memory.
  //
  // For 32/64-bit S/W UNDI, this is the first byte of the EFI ROM
  // image.
  //
  // For H/W UNDI, this is set to zero.
  //
  UINT32  ImageSize;
  //
  // Size of the UNrelocated driver/ROM image of this network interface.
  // This is set to zero if there is no driver/ROM image.
  //
  CHAR8   StringId[4];
  //
  // 4 char ASCII string to go in class identifier (option 60) in DHCP
  // and Boot Server discover packets.
  // For EfiNetworkInterfaceUndi this field is "UNDI".
  // For EfiNetworkInterfaceSnp this field is "SNPN".
  //
  UINT8   Type;
  UINT8   MajorVer;
  UINT8   MinorVer;
  //
  // Information to be placed into the PXE DHCP and Discover packets.
  // This is the network interface type and version number that will
  // be placed into DHCP option 94 (client network interface identifier).
  //
  BOOLEAN Ipv6Supported;
  UINT8   IfNum;  // interface number to be used with pxeid structure
};

extern EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid;
extern EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31;

#endif // _EFI_NII_H
