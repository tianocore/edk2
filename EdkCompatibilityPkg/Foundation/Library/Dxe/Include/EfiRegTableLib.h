/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiRegTableLib.h
  
Abstract: 

  Definitions and macros for building register tables for chipset 
  initialization..
  
  Components linking this lib must include CpuIo, PciRootBridgeIo, and 
  BootScriptSave protocols in their DPX.
  
Revision History:

--*/

#ifndef EFI_REG_TABLE_H
#define EFI_REG_TABLE_H

#include "Tiano.h"
#include "EfiScriptLib.h"
#include EFI_PROTOCOL_CONSUMER (CpuIo)
#include EFI_PROTOCOL_CONSUMER (PciRootBridgeIo)

//
// RegTable OpCodes are encoded as follows:
//
//  |31----------------------------16|15---------8|7-------0|
//                                 \             \         \
//                                 \             \         \
//  31:16 defined by Base OpCode---+             \         \
//                                Opcode Flags---+         \
//                                           Base OpCode---+
//
#define OPCODE_BASE(OpCode)       ((UINT8)((OpCode) & 0xFF))
#define OPCODE_FLAGS(OpCode)      ((UINT8)(((OpCode) >> 8) & 0xFF)) 
#define OPCODE_EXTRA_DATA(OpCode) ((UINT16)((OpCode) >> 16)) 

//
// RegTable Base OpCodes
//
#define OP_TERMINATE_TABLE                0
#define OP_MEM_WRITE                      1
#define OP_MEM_READ_MODIFY_WRITE          2
#define OP_IO_WRITE                       3
#define OP_IO_READ_MODIFY_WRITE           4
#define OP_PCI_WRITE                      5
#define OP_PCI_READ_MODIFY_WRITE          6
#define OP_STALL                          7

//
// RegTable OpCode Flags
//
#define OPCODE_FLAG_S3SAVE                1


#define TERMINATE_TABLE { (UINT32) OP_TERMINATE_TABLE, (UINT32) 0, (UINT32) 0 }


//
// REG_TABLE_ENTRY_PCI_WRITE encodes the width in the upper bits of the OpCode
// as one of the EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_WIDTH values
//
typedef struct {
  UINT32                                OpCode;
  UINT32                                PciAddress;
  UINT32                                Data;
} EFI_REG_TABLE_PCI_WRITE;

#define PCI_WRITE(Bus, Dev, Fnc, Reg, Width, Data, S3Flag)                    \
  {                                                                           \
    (UINT32) (OP_PCI_WRITE | ((S3Flag) << 8) | ((Width) << 16)),              \
    (UINT32) (EFI_PCI_ADDRESS ((Bus), (Dev), (Fnc), (Reg))),                  \
    (UINT32) (Data),                                                          \
    (UINT32) (0)                                                              \
  }

typedef struct {
  UINT32                                OpCode;
  UINT32                                PciAddress;
  UINT32                                OrMask;
  UINT32                                AndMask;
} EFI_REG_TABLE_PCI_READ_MODIFY_WRITE;

#define PCI_READ_MODIFY_WRITE(Bus, Dev, Fnc, Reg, Width, OrMask, AndMask, S3Flag)  \
  {                                                                           \
    (UINT32) (OP_PCI_READ_MODIFY_WRITE | ((S3Flag) << 8) | ((Width) << 16)),  \
    (UINT32) (EFI_PCI_ADDRESS ((Bus), (Dev), (Fnc), (Reg))),                  \
    (UINT32) (OrMask),                                                        \
    (UINT32) (AndMask)                                                        \
  }

typedef struct {
  UINT32                                OpCode;
  UINT32                                MemAddress;
  UINT32                                OrMask;
  UINT32                                AndMask;
} EFI_REG_TABLE_MEM_READ_MODIFY_WRITE;

#define MEM_READ_MODIFY_WRITE(Address, Width, OrMask, AndMask, S3Flag)  \
  {                                                                           \
    (UINT32) (OP_MEM_READ_MODIFY_WRITE | ((S3Flag) << 8) | ((Width) << 16)),  \
    (UINT32) (Address),                  \
    (UINT32) (OrMask),                                                        \
    (UINT32) (AndMask)                                                        \
  }
  
typedef struct {
  UINT32                                OpCode;
  UINT32                                Field2;
  UINT32                                Field3;
  UINT32                                Field4;
} EFI_REG_TABLE_GENERIC;

typedef union {
  EFI_REG_TABLE_GENERIC                 Generic;
  EFI_REG_TABLE_PCI_WRITE               PciWrite;
  EFI_REG_TABLE_PCI_READ_MODIFY_WRITE   PciReadModifyWrite;
  EFI_REG_TABLE_MEM_READ_MODIFY_WRITE   MemReadModifyWrite;
} EFI_REG_TABLE;

VOID
ProcessRegTablePci (
  EFI_REG_TABLE                   * RegTableEntry,
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL * PciRootBridgeIo,
  EFI_CPU_IO_PROTOCOL             * CpuIo
  )
/*++

Routine Description:
  Processes register table assuming which may contain PCI, IO, MEM, and STALL
  entries.
  
  No parameter checking is done so the caller must be careful about omitting
  values for PciRootBridgeIo or CpuIo parameters.  If the regtable does
  not contain any PCI accesses, it is safe to omit the PciRootBridgeIo (supply
  NULL).  If the regtable does not contain any IO or Mem entries, it is safe to
  omit the CpuIo (supply NULL).
  
  The RegTableEntry parameter is not checked, but is required.
  
  gBS is assumed to have been defined and is used when processing stalls.
  
  The function processes each entry sequentially until an OP_TERMINATE_TABLE
  entry is encountered.

Arguments:
  RegTableEntry - A pointer to the register table to process
  
  PciRootBridgeIo - A pointer to the instance of PciRootBridgeIo that is used
                  when processing PCI table entries
                  
  CpuIo - A pointer to the instance of CpuIo that is used when processing IO and
                  MEM table entries

Returns:
  Nothing.  
  
--*/
;

VOID
ProcessRegTableCpu (
  EFI_REG_TABLE                   * RegTableEntry,
  EFI_CPU_IO_PROTOCOL             * CpuIo
  )
/*++

Routine Description:
  Processes register table assuming which may contain IO, MEM, and STALL
  entries, but must NOT contain any PCI entries.  Any PCI entries cause an
  ASSERT in a DEBUG build and are skipped in a free build.
  
  No parameter checking is done.  Both RegTableEntry and CpuIo parameters are
  required.
  
  gBS is assumed to have been defined and is used when processing stalls.

  The function processes each entry sequentially until an OP_TERMINATE_TABLE
  entry is encountered.

Arguments:
  RegTableEntry - A pointer to the register table to process
  
  CpuIo - A pointer to the instance of CpuIo that is used when processing IO and
                  MEM table entries

Returns:
  Nothing.  
  
--*/
;

#endif
