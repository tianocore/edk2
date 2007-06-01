/** @file
  This file declares the related BootScript definitions.

  Copyright (c) 2006 - 2007, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  BootScript.h

  @par Revision Reference:
  These definitions are defined in BootScript Spec 0.91.

**/

#ifndef _EFI_BOOT_SCRIPT_H_
#define _EFI_BOOT_SCRIPT_H_

#define EFI_ACPI_S3_RESUME_SCRIPT_TABLE               0x00

//
// Boot Script Opcode Definitions
//

#define EFI_BOOT_SCRIPT_IO_WRITE_OPCODE               0x00
#define EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE          0x01
#define EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE              0x02
#define EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE         0x03
#define EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE       0x04
#define EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE  0x05
#define EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE          0x06
#define EFI_BOOT_SCRIPT_STALL_OPCODE                  0x07
#define EFI_BOOT_SCRIPT_DISPATCH_OPCODE               0x08
#define EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE             0x09
#define EFI_BOOT_SCRIPT_INFORMATION_OPCODE            0x0A

#define EFI_BOOT_SCRIPT_TABLE_OPCODE                  0xAA
#define EFI_BOOT_SCRIPT_TERMINATE_OPCODE              0xFF

//
// EFI Boot Script Width
//
typedef enum {
  EfiBootScriptWidthUint8,
  EfiBootScriptWidthUint16,
  EfiBootScriptWidthUint32,
  EfiBootScriptWidthUint64,
  EfiBootScriptWidthFifoUint8,
  EfiBootScriptWidthFifoUint16,
  EfiBootScriptWidthFifoUint32,
  EfiBootScriptWidthFifoUint64,
  EfiBootScriptWidthFillUint8,
  EfiBootScriptWidthFillUint16,
  EfiBootScriptWidthFillUint32,
  EfiBootScriptWidthFillUint64,
  EfiBootScriptWidthMaximum
} EFI_BOOT_SCRIPT_WIDTH;

#endif
