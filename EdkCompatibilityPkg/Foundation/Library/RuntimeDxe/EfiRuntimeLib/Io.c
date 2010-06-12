/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Io.c

Abstract:

  Light weight lib functions that wrape IoRead (), IoWrite, MemRead (), 
  and MemWrite ().

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"

UINT8
IoRead8 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a one byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
{
  UINT8 Buffer;

  Buffer = 0;
  EfiIoRead (EfiCpuIoWidthUint8, Address, 1, &Buffer);
  return Buffer;
}

UINT16
IoRead16 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a two byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
{
  UINT16  Buffer;

  Buffer = 0;
  EfiIoRead (EfiCpuIoWidthUint16, Address, 1, &Buffer);
  return Buffer;
}

UINT32
IoRead32 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a four byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
{
  UINT32  Buffer;

  Buffer = 0;
  EfiIoRead (EfiCpuIoWidthUint32, Address, 1, &Buffer);
  return Buffer;
}

VOID
IoWrite8 (
  IN  UINT64    Address,
  IN  UINT8     Data
  )
/*++

Routine Description:
  Do a one byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiIoWrite (EfiCpuIoWidthUint8, Address, 1, &Data);
}

VOID
IoWrite16 (
  IN  UINT64    Address,
  IN  UINT16    Data
  )
/*++

Routine Description:
  Do a two byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiIoWrite (EfiCpuIoWidthUint16, Address, 1, &Data);
}

VOID
IoWrite32 (
  IN  UINT64    Address,
  IN  UINT32    Data
  )
/*++

Routine Description:
  Do a four byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiIoWrite (EfiCpuIoWidthUint32, Address, 1, &Data);
}

UINT8
MemRead8 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a one byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
{
  UINT8   Buffer;

  Buffer = 0;
  EfiMemRead (EfiCpuIoWidthUint8, Address, 1, &Buffer);
  return Buffer;
}

UINT16
MemRead16 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a two byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
{
  UINT16  Buffer;

  Buffer = 0;
  EfiMemRead (EfiCpuIoWidthUint16, Address, 1, &Buffer);
  return Buffer;
}

UINT32
MemRead32 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a four byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
{
  UINT32  Buffer;

  Buffer = 0;
  EfiMemRead (EfiCpuIoWidthUint32, Address, 1, &Buffer);
  return Buffer;
}

UINT64
MemRead64 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a eight byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
{
  UINT64  Buffer;

  Buffer = 0;
  EfiMemRead (EfiCpuIoWidthUint64, Address, 1, &Buffer);
  return Buffer;
}

VOID
MemWrite8 (
  IN  UINT64    Address,
  IN  UINT8     Data
  )
/*++

Routine Description:
  Do a one byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiMemWrite (EfiCpuIoWidthUint8, Address, 1, &Data);
}

VOID
MemWrite16 (
  IN  UINT64    Address,
  IN  UINT16    Data
  )
/*++

Routine Description:
  Do a two byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiMemWrite (EfiCpuIoWidthUint16, Address, 1, &Data);
}

VOID
MemWrite32 (
  IN  UINT64    Address,
  IN  UINT32    Data
  )
/*++

Routine Description:
  Do a four byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiMemWrite (EfiCpuIoWidthUint32, Address, 1, &Data);
}

VOID
MemWrite64 (
  IN  UINT64    Address,
  IN  UINT64    Data
  )
/*++

Routine Description:
  Do a eight byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
{
  EfiMemWrite (EfiCpuIoWidthUint64, Address, 1, &Data);
}
