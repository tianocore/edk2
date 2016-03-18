/** @file
  This file defines a configuration Table Guid for Load module at fixed address.
   
  This configuration table is to hold  the top address below which the Dxe runtime code and 
  boot time code will be loaded and Tseg base. When this feature is enabled, Build tools will assigned 
  module loading address relative to these two addresses.
  

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LOAD_MODULE_AT_FIX_ADDRESS_GUID_H__
#define __LOAD_MODULE_AT_FIX_ADDRESS_GUID_H__

#define EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE_GUID \
  { 0x2CA88B53,0xD296,0x4080, { 0xA4,0xA5,0xCA,0xD9,0xBA,0xE2,0x4B,0x9} }


extern EFI_GUID gLoadFixedAddressConfigurationTableGuid;

typedef struct {
  EFI_PHYSICAL_ADDRESS    DxeCodeTopAddress;   ///< The top address below which the Dxe runtime code and below which the Dxe runtime/boot code and PEI code.
  EFI_PHYSICAL_ADDRESS    SmramBase;           ///< SMRAM base address. The build tool assigns an offset relative to the SMRAM base for a SMM driver.
} EFI_LOAD_FIXED_ADDRESS_CONFIGURATION_TABLE;

#endif
