/** @file
	UEFI Driver Model Library Services

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	UefiDriverModelLib.h

**/

#ifndef __UEFI_DRIVER_MODEL_LIB_H__
#define __UEFI_DRIVER_MODEL_LIB_H__

//
// Declare bitmask values for the protocols that are enabled 
//
#define UEFI_DRIVER_MODEL_LIBRARY_COMPONENT_NAME_PROTOCOL_ENABLED        0x01
#define UEFI_DRIVER_MODEL_LIBRARY_DRIVER_DIAGNOSTICS_PROTOCOL_ENABLED    0x02
#define UEFI_DRIVER_MODEL_LIBRARY_DRIVER_CONFIGURATION_PROTOCOL_ENABLED  0x04

//
// 
//
extern const UINT8                           _gDriverModelProtocolBitmask;

//
//
//
typedef struct {
  const EFI_DRIVER_BINDING_PROTOCOL        *DriverBinding;
  const EFI_COMPONENT_NAME_PROTOCOL        *ComponentName;
  const EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration;
  const EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics;
} EFI_DRIVER_MODEL_PROTOCOL_LIST;

//
//
//
extern const UINTN                           _gDriverModelProtocolListEntries;
extern const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[];

#endif
