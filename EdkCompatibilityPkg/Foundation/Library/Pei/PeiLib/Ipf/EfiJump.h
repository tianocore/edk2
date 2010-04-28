/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiJump.h

Abstract:

  This is the Setjump/Longjump pair for an IA32 processor.

--*/

#ifndef _EFI_JUMP_H_
#define _EFI_JUMP_H_

#include EFI_GUID_DEFINITION (PeiTransferControl)

//
//  NOTE:Set/LongJump needs to have this buffer start
//  at 16 byte boundary. Either fix the structure
//  which call this buffer or fix inside SetJump/LongJump
//  Choosing 1K buffer storage for now
//
typedef struct {
  CHAR8 Buffer[1024];
} EFI_JUMP_BUFFER;

EFI_STATUS
SetJump (
  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
  IN EFI_JUMP_BUFFER                    *Jump
  )
/*++

Routine Description:

  SetJump stores the current register set in the area pointed to
by "save".  It returns zero.  Subsequent calls to "LongJump" will
restore the registers and return non-zero to the same location.
  On entry, r32 contains the pointer to the jmp_buffer
  
Arguments:
  
  This  - Calling context
  Jump  - Jump buffer

Returns:

  Status code

--*/
;

EFI_STATUS
LongJump (
  IN EFI_PEI_TRANSFER_CONTROL_PROTOCOL  *This,
  IN EFI_JUMP_BUFFER                    *Jump
  )
/*++

Routine Description:

  LongJump initializes the register set to the values saved by a
previous 'SetJump' and jumps to the return location saved by that
'SetJump'.  This has the effect of unwinding the stack and returning
for a second time to the 'SetJump'.

Arguments:

  This  - Calling context
  Jump  - Jump buffer

Returns:

  Status code

--*/
;

VOID
RtPioICacheFlush (
  IN  VOID    *StartAddress,
  IN  UINTN   SizeInBytes
  )
/*++

Routine Description:

  Flushing the CPU instruction cache.

Arguments:

  StartAddress  - Start address to flush
  SizeInBytes   - Length in bytes to flush

Returns:

  None

--*/
;

#endif
