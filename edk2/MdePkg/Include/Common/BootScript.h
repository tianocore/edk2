/** @file
  This file declares the related BootScript definitions and some SMBus definitions.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  BootScript.h

  @par Revision Reference:
  These definitions are defined in BootScript Spec 0.91 and SmBus PPI spec 0.9.

**/

#ifndef _EFI_SCRIPT_H_
#define _EFI_SCRIPT_H_

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

#ifndef __GNUC__
#pragma pack(1)
#endif

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

//
// EFI Smbus Device Address, Smbus Device Command, Smbus Operation
//
typedef struct {
  UINTN SmbusDeviceAddress : 7;
} EFI_SMBUS_DEVICE_ADDRESS;

typedef UINTN EFI_SMBUS_DEVICE_COMMAND;

typedef enum _EFI_SMBUS_OPERATION
{
  EfiSmbusQuickRead,
  EfiSmbusQuickWrite,
  EfiSmbusReceiveByte,
  EfiSmbusSendByte,
  EfiSmbusReadByte,
  EfiSmbusWriteByte,
  EfiSmbusReadWord,
  EfiSmbusWriteWord,
  EfiSmbusReadBlock,
  EfiSmbusWriteBlock,
  EfiSmbusProcessCall,
  EfiSmbusBWBRProcessCall
} EFI_SMBUS_OPERATION;

//
// Boot Script Opcode Header Structure Definitions
//

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
} EFI_BOOT_SCRIPT_GENERIC_HEADER;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT16  Version;
  UINT32  TableLength;
  UINT16  Reserved[2];
} EFI_BOOT_SCRIPT_TABLE_HEADER;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
} EFI_BOOT_SCRIPT_COMMON_HEADER;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_IO_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_IO_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_MEM_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_MEM_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT32                Count;
  UINT64                Address;
} EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE;

typedef struct {
  UINT16                    OpCode;
  UINT8                     Length;
  UINT64                    SlaveAddress;
  UINT64                    Command;
  UINT32                    Operation;
  BOOLEAN                   PecCheck;
  UINT32                    DataSize;
} EFI_BOOT_SCRIPT_SMBUS_EXECUTE;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT64  Duration;
} EFI_BOOT_SCRIPT_STALL;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  EFI_PHYSICAL_ADDRESS  EntryPoint;
} EFI_BOOT_SCRIPT_DISPATCH;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  EFI_PHYSICAL_ADDRESS  EntryPoint;
  EFI_PHYSICAL_ADDRESS  Context;
} EFI_BOOT_SCRIPT_DISPATCH_2;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32                InformationLength;  
  EFI_PHYSICAL_ADDRESS  Information;
} EFI_BOOT_SCRIPT_INFORMATION;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
} EFI_BOOT_SCRIPT_TERMINATE;

typedef union {
  EFI_BOOT_SCRIPT_GENERIC_HEADER        *Header;
  EFI_BOOT_SCRIPT_TABLE_HEADER          *TableInfo;
  EFI_BOOT_SCRIPT_IO_WRITE              *IoWrite;
  EFI_BOOT_SCRIPT_IO_READ_WRITE         *IoReadWrite;
  EFI_BOOT_SCRIPT_MEM_WRITE             *MemWrite;
  EFI_BOOT_SCRIPT_MEM_READ_WRITE        *MemReadWrite;
  EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE      *PciWrite;
  EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE *PciReadWrite;
  EFI_BOOT_SCRIPT_SMBUS_EXECUTE         *SmbusExecute;
  EFI_BOOT_SCRIPT_STALL                 *Stall;
  EFI_BOOT_SCRIPT_DISPATCH              *Dispatch;
  EFI_BOOT_SCRIPT_DISPATCH_2            *Dispatch2;  
  EFI_BOOT_SCRIPT_INFORMATION           *Information;
  EFI_BOOT_SCRIPT_TERMINATE             *Terminate;
  EFI_BOOT_SCRIPT_COMMON_HEADER         *CommonHeader;
  UINT8                                 *Raw;
} BOOT_SCRIPT_POINTERS;

#ifndef __GNUC__
#pragma pack()
#endif

#endif
