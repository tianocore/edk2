/** @file
  This code abstracts the CPU IO Protocol

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  CpuIO.h

  @par Revision Reference:
  CPU IO Protocol is defined in Framework of EFI CPU IO Protocol Spec
  Version 0.9

**/

#ifndef _CPUIO_H_
#define _CPUIO_H_

#define EFI_CPU_IO_PROTOCOL_GUID \
  { \
    0xB0732526, 0x38C8, 0x4b40, {0x88, 0x77, 0x61, 0xC7, 0xB0, 0x6A, 0xAC, 0x45 } \
  }

typedef struct _EFI_CPU_IO_PROTOCOL EFI_CPU_IO_PROTOCOL;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_WIDTH
// *******************************************************
//
typedef enum {
  EfiCpuIoWidthUint8,
  EfiCpuIoWidthUint16,
  EfiCpuIoWidthUint32,
  EfiCpuIoWidthUint64,
  EfiCpuIoWidthFifoUint8,
  EfiCpuIoWidthFifoUint16,
  EfiCpuIoWidthFifoUint32,
  EfiCpuIoWidthFifoUint64,
  EfiCpuIoWidthFillUint8,
  EfiCpuIoWidthFillUint16,
  EfiCpuIoWidthFillUint32,
  EfiCpuIoWidthFillUint64,
  EfiCpuIoWidthMaximum
} EFI_CPU_IO_PROTOCOL_WIDTH;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_IO_MEM
// *******************************************************
//
/**
  Enables a driver to access memory-mapped registers in the EFI system memory space.
  Or, Enables a driver to access registers in the EFI CPU I/O space.

  @param  This A pointer to the EFI_CPU_IO_PROTOCOL instance.
  
  @param  Width Signifies the width of the I/O or Memory operation. 
  
  @param  Address The base address of the I/O or Memoryoperation. 
  
  @param  Count The number of I/O or Memory operations to perform.
  The number of bytes moved is Width size * Count, starting at Address.
  
  @param  Buffer For read operations, the destination buffer to store the results.
  For write operations, the source buffer from which to write data.

  @retval EFI_SUCCESS The data was read from or written to the EFI system.
  
  @retval EFI_INVALID_PARAMETER Width is invalid for this EFI system.Or Buffer is NULL.
  
  @retval EFI_UNSUPPORTED The Buffer is not aligned for the given Width. 
  Or,The address range specified by Address, Width, and Count is not valid for this EFI system.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_CPU_IO_PROTOCOL_IO_MEM) (
  IN EFI_CPU_IO_PROTOCOL                *This,
  IN  EFI_CPU_IO_PROTOCOL_WIDTH         Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL_ACCESS
// *******************************************************
//
typedef struct {
  EFI_CPU_IO_PROTOCOL_IO_MEM  Read;
  EFI_CPU_IO_PROTOCOL_IO_MEM  Write;
} EFI_CPU_IO_PROTOCOL_ACCESS;

//
// *******************************************************
// EFI_CPU_IO_PROTOCOL
// *******************************************************
//
/**
  @par Protocol Description:
  Provides the basic memory and I/O interfaces that are used to abstract 
  accesses to devices in a system.

  @param Mem.Read
  Allows reads from memory-mapped I/O space.

  @param Mem.Write
  Allows writes to memory-mapped I/O space.

  @param Io.Read
  Allows reads from I/O space.

  @param Io.Write
  Allows writes to I/O space.

**/
struct _EFI_CPU_IO_PROTOCOL {
  EFI_CPU_IO_PROTOCOL_ACCESS  Mem;
  EFI_CPU_IO_PROTOCOL_ACCESS  Io;
};

extern EFI_GUID gEfiCpuIoProtocolGuid;

#endif
