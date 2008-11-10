/** @file
  The definition for iSCSI Boot Firmware Table, it's defined in Microsoft's
  iSCSI Boot Firmware Table(iBFT) as Defined in ACPI 3.0b Specification. 
  
  Copyright (c) 2006 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _ISCSI_BOOT_FIRMWARE_TABLE_H_
#define _ISCSI_BOOT_FIRMWARE_TABLE_H_

#include "Acpi30.h"

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_REVISION            0x01
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_ALIGNMENT 8

///
/// Structure Type/ID
///
typedef enum {
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_RESERVED_STRUCTURE_ID = 0,
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_ID,
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_ID,
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_ID,
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_ID,
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_EXTERNSIONS_STRUCTURE_ID
} EFI_ACPI_ISCSI_ID_DEFINITIONS;

///
/// from the definition of IP_PREFIX_ORIGIN Enumeration in MSDN,
/// not defined in Microsoft iBFT document.
///
typedef enum {
  IpPrefixOriginOther = 0,
  IpPrefixOriginManual,
  IpPrefixOriginWellKnown,
  IpPrefixOriginDhcp,
  IpPrefixOriginRouterAdvertisement,
  IpPrefixOriginUnchanged = 16
} IP_PREFIX_VALUE;

///
/// iBF Table Header
///
typedef struct {
  UINT32  Signature;
  UINT32  Length;
  UINT8   Revision;
  UINT8   Checksum;
  UINT8   OemId[6];
  UINT64  OemTableId;
  UINT8   Reserved[24];
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER;

///
/// Common Header of Boot Firmware Table Structure 
///
typedef struct {
  UINT8   StructureId;
  UINT8   Version;
  UINT16  Length;
  UINT8   Index;
  UINT8   Flags;
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_HEADER;

///
/// Control Structure
///
typedef struct {
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_HEADER  Header;
  UINT16                                               Extensions;
  UINT16                                               InitiatorOffset;
  UINT16                                               NIC0Offset;
  UINT16                                               Target0Offset;
  UINT16                                               NIC1Offset;
  UINT16                                               Target1Offset;  
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE;

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_VERSION              0x1

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_FLAG_BOOT_FAILOVER   BIT0

///
/// Initiator Structure
///
typedef struct {
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_HEADER  Header;
  EFI_IPv6_ADDRESS                                     ISnsServer;
  EFI_IPv6_ADDRESS                                     SlpServer;
  EFI_IPv6_ADDRESS                                     PrimaryRadiusServer;
  EFI_IPv6_ADDRESS                                     SecondaryRadiusServer;
  UINT16                                               IScsiNameLength;
  UINT16                                               IScsiNameOffset;
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE;

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_VERSION             0x1

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BLOCK_VALID    BIT0 
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BOOT_SELECTED  BIT1 

///
/// NIC Structure
///
typedef struct {
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_HEADER  Header;
  EFI_IPv6_ADDRESS                                     Ip;
  UINT8                                                SubnetMaskPrefixLength;
  UINT8                                                Origin;
  EFI_IPv6_ADDRESS                                     Gateway;
  EFI_IPv6_ADDRESS                                     PrimaryDns;
  EFI_IPv6_ADDRESS                                     SecondaryDns;
  EFI_IPv6_ADDRESS                                     DhcpServer;
  UINT16                                               VLanTag;
  UINT8                                                Mac[6];
  UINT16                                               PciLocation;
  UINT16                                               HostNameLength;
  UINT16                                               HostNameOffset;
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE;

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_VERSION                 0x1

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BLOCK_VALID        BIT0
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BOOT_SELECTED      BIT1
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_GLOBAL             BIT2

///
/// Target Structure
///
typedef struct {
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_STRUCTURE_HEADER  Header;
  EFI_IPv6_ADDRESS                                     Ip;
  UINT16                                               Port;
  UINT8                                                BootLun[8];
  UINT8                                                CHAPType;
  UINT8                                                NicIndex;
  UINT16                                               IScsiNameLength;
  UINT16                                               IScsiNameOffset;
  UINT16                                               CHAPNameLength;
  UINT16                                               CHAPNameOffset;
  UINT16                                               CHAPSecretLength;
  UINT16                                               CHAPSecretOffset;
  UINT16                                               ReverseCHAPNameLength;
  UINT16                                               ReverseCHAPNameOffset;
  UINT16                                               ReverseCHAPSecretLength;
  UINT16                                               ReverseCHAPSecretOffset;
} EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE;

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_VERSION               0x1

#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BLOCK_VALID      BIT0
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BOOT_SELECTED    BIT1
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_RADIUS_CHAP      BIT2
#define EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_RADIUS_RCHAP     BIT3

#endif

