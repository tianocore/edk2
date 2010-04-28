/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BusSpecificDriverOverride.h

Abstract:

  Bus Specific Driver Override protocol as defined in the EFI 1.1 specification.

--*/

#ifndef _EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_H_
#define _EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_H_

//
// Global ID for the Bus Specific Driver Override Protocol
//
#define EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID \
  { \
    0x3bc1b285, 0x8a15, 0x4a82, {0xaa, 0xbf, 0x4d, 0x7d, 0x13, 0xfb, 0x32, 0x65} \
  }

EFI_FORWARD_DECLARATION (EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL);

//
// Prototypes for the Bus Specific Driver Override Protocol
//
typedef
EFI_STATUS
(EFIAPI *EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_GET_DRIVER) (
  IN EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL              * This,
  IN OUT EFI_HANDLE                                         * DriverImageHandle
  );

//
// Interface structure for the Bus Specific Driver Override Protocol
//
struct _EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL {
  EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_GET_DRIVER GetDriver;
};

extern EFI_GUID gEfiBusSpecificDriverOverrideProtocolGuid;

#endif
