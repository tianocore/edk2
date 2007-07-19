/**@file
  ACPI Serial Port Console Redirection Table as defined by Microsoft in
  http://www.microsoft.com/whdc/system/platform/server/spcr.mspx
    
  Copyright (c) 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
**/

#ifndef _SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_H_
#define _SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_H_


#include <IndustryStandard/Acpi.h>

//
// Ensure proper structure formats
//
#ifdef _MSC_EXTENSIONS
#pragma pack(1)
#endif

//
// SPCR Revision (defined in spec)
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION 0x01

//
// SPCR Structure Definition
//
typedef struct {
  EFI_ACPI_DESCRIPTION_HEADER             Header;
  UINT8                                   InterfaceType;
  UINT8                                   Reserved1[3];
  EFI_ACPI_2_0_GENERIC_ADDRESS_STRUCTURE  BaseAddress;
  UINT8                                   InterruptType;
  UINT8                                   Irq;
  UINT32                                  GlobalSystemInterrupt;
  UINT8                                   BaudRate;
  UINT8                                   Parity;
  UINT8                                   StopBits;
  UINT8                                   FlowControl;
  UINT8                                   TerminalType;
  UINT8                                   Language;
  UINT16                                  PciDeviceId;
  UINT16                                  PciVendorId;
  UINT8                                   PciBusNumber;
  UINT8                                   PciDeviceNumber;
  UINT8                                   PciFunctionNumber;
  UINT32                                  PciFlags;
  UINT8                                   PciSegment;
  UINT32                                  Reserved2;
} EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE;

#ifdef _MSC_EXTENSIONS
#pragma pack()
#endif

//
// SPCR Definitions
//

//
// Interface Type
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERFACE_TYPE_16550   0
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERFACE_TYPE_16450   1

//
// Interrupt Type
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259    0x1
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_APIC    0x2
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_SAPIC   0x4

//
// Baud Rate
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_9600         3
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_19200        4
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_57600        5
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_115200       6

//
// Parity
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_PARITY_NO_PARITY       0

//
// Stop Bits
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_STOP_BITS_1            1

//
// Flow Control
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_FLOW_CONTROL_DCD       0x1
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_FLOW_CONTROL_RTS_CTS   0x2
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_FLOW_CONTROL_XON_XOFF  0x4

//
// Terminal Type
//
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT100      0
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT100_PLUS 1
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT_UTF8    2
#define EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_ANSI       3

#endif
