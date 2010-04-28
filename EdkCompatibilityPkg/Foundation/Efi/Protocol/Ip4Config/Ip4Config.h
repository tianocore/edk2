/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED. 

Module Name:

  Ip4Config.h

Abstract:

--*/

#ifndef _IP4CONFIG_H_
#define _IP4CONFIG_H_

#include EFI_PROTOCOL_DEFINITION (Ip4)

#define EFI_IP4_CONFIG_PROTOCOL_GUID \
  { 0x3b95aa31, 0x3793, 0x434b, {0x86, 0x67, 0xc8, 0x07, 0x08, 0x92, 0xe0, 0x5e} }

EFI_FORWARD_DECLARATION (EFI_IP4_CONFIG_PROTOCOL);

#define IP4_CONFIG_VARIABLE_ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

typedef struct {
  EFI_IPv4_ADDRESS       StationAddress;
  EFI_IPv4_ADDRESS       SubnetMask;
  UINT32                 RouteTableSize;
  EFI_IP4_ROUTE_TABLE    *RouteTable;
} EFI_IP4_IPCONFIG_DATA;


typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_START) (
  IN EFI_IP4_CONFIG_PROTOCOL   *This,
  IN EFI_EVENT                 DoneEvent,
  IN EFI_EVENT                 ReconfigEvent 
  );

typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_STOP) (
  IN EFI_IP4_CONFIG_PROTOCOL  *This 
  );

typedef
EFI_STATUS
(EFIAPI *EFI_IP4_CONFIG_GET_DATA) (
  IN EFI_IP4_CONFIG_PROTOCOL *This,
  IN OUT UINTN               *ConfigDataSize,
  OUT EFI_IP4_IPCONFIG_DATA  *ConfigData    OPTIONAL 
  );


struct _EFI_IP4_CONFIG_PROTOCOL {
  EFI_IP4_CONFIG_START     Start;
  EFI_IP4_CONFIG_STOP      Stop;
  EFI_IP4_CONFIG_GET_DATA  GetData;
};


extern EFI_GUID gEfiIp4ConfigProtocolGuid;

#endif
