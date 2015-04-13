/** @file
  This file declares the internal Framework Boot Script format used by
  the PI implementation of Script Saver and Executor.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _BOOT_SCRIPT_INTERNAL_FORMAT_H_
#define _BOOT_SCRIPT_INTERNAL_FORMAT_H_

#pragma pack(1)

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
  UINT32                Count;
  UINT64                Address;
  UINT16                Segment;
} EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
} EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE;

typedef struct {
  UINT16                OpCode;
  UINT8                 Length;
  UINT32                Width;
  UINT64                Address;
  UINT16                Segment;
} EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE;

typedef struct {
  UINT16                    OpCode;
  UINT8                     Length;
  UINT64                    SmBusAddress;
  UINT32                    Operation;
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
  UINT32  Width;
  UINT64  Address;
  UINT64  Duration;
  UINT64  LoopTimes;
} EFI_BOOT_SCRIPT_MEM_POLL;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32  InformationLength;
// UINT8   InformationData[InformationLength];
} EFI_BOOT_SCRIPT_INFORMATION;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32  Width;
  UINT64  Address;
  UINT64  Delay;
} EFI_BOOT_SCRIPT_IO_POLL;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32  Width;
  UINT64  Address;
  UINT64  Delay;
} EFI_BOOT_SCRIPT_PCI_CONFIG_POLL;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
  UINT32  Width;
  UINT64  Address;
  UINT16  Segment;  
  UINT64  Delay;
} EFI_BOOT_SCRIPT_PCI_CONFIG2_POLL;

typedef struct {
  UINT16  OpCode;
  UINT8   Length;
} EFI_BOOT_SCRIPT_TERMINATE;


#pragma pack()

#define BOOT_SCRIPT_NODE_MAX_LENGTH   1024

#define BOOT_SCRIPT_TABLE_VERSION     0x0001

#endif
