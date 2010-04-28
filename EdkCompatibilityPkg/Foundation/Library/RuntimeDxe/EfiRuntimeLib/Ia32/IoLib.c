/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  IoLib.c

Abstract:

  Light weight lib to support Tiano drivers.

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_PROTOCOL_DEFINITION (CpuIo)

extern EFI_CPU_IO_PROTOCOL  *gCpuIo;

EFI_STATUS
EfiIoRead (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH   Width,
  IN  UINT64                      Address,
  IN  UINTN                       Count,
  IN  OUT VOID                    *Buffer
  )
/*++

Routine Description:
  Perform an IO read into Buffer.

Arguments:
  Width   - Width of read transaction, and repeat operation to use
  Address - IO address to read
  Count   - Number of times to read the IO address.
  Buffer  - Buffer to read data into. size is Width * Count

Returns: 
  BugBug: Check with Mike to see if I can find this #define some ware else

--*/
{
  return gCpuIo->Io.Read (gCpuIo, Width, Address, Count, Buffer);
}

EFI_STATUS
EfiIoWrite (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH   Width,
  IN  UINT64                      Address,
  IN  UINTN                       Count,
  IN  OUT VOID                    *Buffer
  )
/*++

Routine Description:
  Perform an IO write into Buffer.

Arguments:
  Width   - Width of write transaction, and repeat operation to use
  Address - IO address to write
  Count   - Number of times to write the IO address.
  Buffer  - Buffer to write data from. size is Width * Count

Returns: 
  BugBug: Check with Mike to see if I can find this #define some ware else

--*/
{
  return gCpuIo->Io.Write (gCpuIo, Width, Address, Count, Buffer);
}

EFI_STATUS
EfiMemRead (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH   Width,
  IN  UINT64                      Address,
  IN  UINTN                       Count,
  IN  OUT VOID                    *Buffer
  )
/*++

Routine Description:
  Perform a Memory mapped IO read into Buffer.

Arguments:
  Width   - Width of each read transaction.
  Address - Memory mapped IO address to read
  Count   - Number of Width quanta to read
  Buffer  - Buffer to read data into. size is Width * Count

Returns: 
  BugBug: Check with Mike to see if I can find this #define some ware else

--*/
{
  return gCpuIo->Mem.Read (gCpuIo, Width, Address, Count, Buffer);
}

EFI_STATUS
EfiMemWrite (
  IN  EFI_CPU_IO_PROTOCOL_WIDTH    Width,
  IN  UINT64                       Address,
  IN  UINTN                        Count,
  IN  OUT VOID                     *Buffer
  )
/*++

Routine Description:
  Perform a memory mapped IO write into Buffer.

Arguments:
  Width   - Width of write transaction, and repeat operation to use
  Address - IO address to write
  Count   - Number of times to write the IO address.
  Buffer  - Buffer to write data from. size is Width * Count

Returns: 
  BugBug: Check with Mike to see if I can find this #define some ware else

--*/
{
  return gCpuIo->Mem.Write (gCpuIo, Width, Address, Count, Buffer);
}
