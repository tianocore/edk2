/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ServerProcessorManagementInterfaceTable.h

Abstract:

  ACPI Server Processor Management Interface Table SPMI as described 
  in the IPMI2.0 Specification Revistion 1.5

--*/

#ifndef _SERVER_PROCESSOR_MANAGEMENT_INTERFACE_TABLE_H_
#define _SERVER_PROCESSOR_MANAGEMENT_INTERFACE_TABLE_H_

#include "Acpi2_0.h"

//
// Ensure proper structure formats
//
#pragma pack (1)

//
// Server Processor Management Interface Table definition.
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER            Header;
  UINT8                                  Reserved_36;
  UINT8                                  InterfaceType;
  UINT16                                 SpecificationRevision;
  UINT8                                  InterruptType;
  UINT8                                  GPE;
  UINT8                                  Reserved_42;
  UINT8                                  PCIDeviceFlag;
  UINT32                                 GlobalSystemInterrupt;
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE BaseAddress;
  UINT8                                  PCISegmentGroup_UID1;
  UINT8                                  PCIBusNumber_UID2;
  UINT8                                  PCIDeviceNumber_UID3;
  UINT8                                  PCIFunctionNumber_UID4;
} EFI_ACPI_SERVER_PROCESSOR_MANAGEMENT_INTERFACE_DESCRIPTION_TABLE;

#pragma pack ()

//
// SPMI Revision
//
#define EFI_ACPI_SERVER_PROCESSOR_MANAGEMENT_INTERFACE_TABLE_REVISION  0x05

//
// Interface Type
//
#define EFI_ACPI_SPMI_INTERFACE_TYPE_RESERVED   0
#define EFI_ACPI_SPMI_INTERFACE_TYPE_KCS        1
#define EFI_ACPI_SPMI_INTERFACE_TYPE_SMIC       2
#define EFI_ACPI_SPMI_INTERFACE_TYPE_BT         3
#define EFI_ACPI_SPMI_INTERFACE_TYPE_SSIF       4

//
// SPMI Specfication Revision
//
#define EFI_ACPI_SPMI_SPECIFICATION_REVISION  0x0150

//
// SPMI Interrupt Type
//
#define EFI_ACPI_SPMI_INTERRUPT_TYPE_SCI     0x1
#define EFI_ACPI_SPMI_INTERRUPT_TYPE_IOAPIC  0x2

#endif
