/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  SimpleCpuIoLib.h

Abstract:

  Light weight monolithic Cpu Io Lib to support PEI Modules.

--*/

#ifndef _PEI_SIMPLE_CPU_IO_LIB_H_
#define _PEI_SIMPLE_CPU_IO_LIB_H_

//
//  Base IO Class Functions
//
UINT8
IoRead8 (
  IN  UINT64  Address
  );

UINT16
IoRead16 (
  IN  UINT64  Address
  );

UINT32
IoRead32 (
  IN  UINT64  Address
  );

VOID
IoWrite8 (
  IN  UINT64  Address,
  IN  UINT8   Data
  );

VOID
IoWrite16 (
  IN  UINT64  Address,
  IN  UINT16  Data
  );

VOID
IoWrite32 (
  IN  UINT64  Address,
  IN  UINT32  Data
  );

UINT32
MemRead32 (
  IN  UINT64  Address
  );

UINT64
MemRead64 (
  IN  UINT64  Address
  );

VOID
MemWrite32 (
  IN  UINT64  Address,
  IN  UINT32  Data
  );

VOID
MemWrite64 (
  IN  UINT64  Address,
  IN  UINT64  Data
  );

#endif
