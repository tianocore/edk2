#/*++
#
#Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
#This program and the accompanying materials                          
#are licensed and made available under the terms and conditions of the BSD License         
#which accompanies this distribution.  The full text of the license may be found at        
#http://opensource.org/licenses/bsd-license.php                                            
#                                                                                          
#THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#
#Module Name:
#
#  EfiCopyMem.c
#
#Abstract:
#
#  This is the code that supports IA32-optimized CopyMem service
#
#--*/
#include "EfiBind.h"
#---------------------------------------------------------------------------
    .686: 
    #.MODEL flat,C
    .mmx: 
    .code: 

#---------------------------------------------------------------------------

.globl ASM_PFX(EfiCommonLibCopyMem)

#VOID
#EfiCommonLibCopyMem (
#  IN VOID   *Destination,
#  IN VOID   *Source,
#  IN UINTN  Count
#  )
#/*++
#
#Routine Description:
#
#  Copy Length bytes from Source to Destination.
#
#Arguments:
#
#  Destination - Target of copy
#
#  Source      - Place to copy from
#
#  Length      - Number of bytes to copy
#
#Returns:
#
#  None
#
#--*/
ASM_PFX(EfiCommonLibCopyMem):

  pushl  %ebp
  movl   %esp, %ebp
  pushl  %ecx # reserve space for Scratch Local variable UINT64 MmxSave
  pushl  %ecx
  pushl  %esi
  pushl  %edi

  movl  0x10(%ebp), %ecx  # Count
  movl  0xC(%ebp), %esi  # Source
  movl  8(%ebp), %edi  # Destination

##First off, make sure we have no overlap. That is to say,
##   if (Source == Destination)           => do nothing
##   if (Source + Count <= Destination)   => regular copy
##   if (Destination + Count <= Source)   => regular copy
##   otherwise, do a reverse copy
  movl  %esi, %eax
  addl  %ecx, %eax                    # Source + Count
  cmpl  %edi, %eax
  jbe   _StartByteCopy

  movl  %edi, %eax
  addl  %ecx, %eax                    # Dest + Count
  cmpl  %esi, %eax
  jbe   _StartByteCopy

  cmpl  %edi, %esi
  je    _CopyMemDone
  jb    _CopyOverlapped               # too bad -- overlaps

  # Pick up misaligned start bytes to get destination pointer 4-byte aligned
_StartByteCopy: 
  cmpl  $0, %ecx
  je    _CopyMemDone                # Count == 0, all done
  movl  %edi, %edx
  andb  $3, %dl                     # check lower 2 bits of address
  testb %dl, %dl
  je    _CopyBlocks                 # already aligned?

  # Copy a byte
  movb  (%esi), %al                 # get byte from Source
  movb  %al, (%edi)                 # write byte to Destination
  decl   %ecx
  incl  %edi
  incl  %esi
  jmp   _StartByteCopy               # back to top of loop

_CopyBlocks: 
  # Compute how many 64-byte blocks we can clear 
  movl  %ecx, %eax                  # get Count in eax
  shrl  $6, %eax                    # convert to 64-byte count
  shll  $6, %eax                    # convert back to bytes
  subl  %eax, %ecx                  # subtract from the original count
  shrl  $6, %eax                    # and this is how many 64-byte blocks

  # If no 64-byte blocks, then skip 
  cmpl  $0, %eax
  je    _CopyRemainingDWords

  # Save mm0 to UINT64 MmxSave
  movq  %mm0, -8(%ebp)

copymmx: 

  movq  %ds:(%esi), %mm0
  movq  %mm0, %ds:(%edi)
  movq  %ds:8(%esi), %mm0
  movq  %mm0, %ds:8(%edi)
  movq  %ds:16(%esi), %mm0
  movq  %mm0, %ds:16(%edi)
  movq  %ds:24(%esi), %mm0
  movq  %mm0, %ds:24(%edi)
  movq  %ds:32(%esi), %mm0
  movq  %mm0, %ds:32(%edi)
  movq  %ds:40(%esi), %mm0
  movq  %mm0, %ds:40(%edi)
  movq  %ds:48(%esi), %mm0
  movq  %mm0, %ds:48(%edi)
  movq  %ds:56(%esi), %mm0
  movq  %mm0, %ds:56(%edi)

  addl  $64, %edi
  addl  $64, %esi
  decl  %eax
  jnz   copymmx

# Restore mm0 from MmxSave
  movq  -8(%ebp), %mm0
  emms                                 # Exit MMX Instruction

  # Copy as many DWORDS as possible
_CopyRemainingDWords: 
  cmpl  $4, %ecx
  jb    _CopyRemainingBytes

  movl  (%esi), %eax                # get data from Source
  movl  %eax, (%edi)                # write byte to Destination
  subl  $4, %ecx                    # decrement Count
  addl  $4, %esi                    # advance Source pointer
  addl  $4, %edi                    # advance Destination pointer
  jmp   _CopyRemainingDWords        # back to top

_CopyRemainingBytes: 
  cmpl  $0, %ecx
  je    _CopyMemDone
  movb  (%esi), %al                 # get byte from Source
  movb  %al, (%edi)                 # write byte to Destination
  decl   %ecx
  incl   %esi
  incl  %edi                     # advance Destination pointer
  jmp   _CopyRemainingBytes         # back to top of loop

  #
  # We do this block if the source and destination buffers overlap. To
  # handle it, copy starting at the end of the source buffer and work
  # your way back. Since this is the atypical case, this code has not
  # been optimized, and thus simply copies bytes.
  #
_CopyOverlapped: 

  # Move the source and destination pointers to the end of the range
  addl  %ecx, %esi                    # Source + Count
  decl   %esi
  addl  %ecx, %edi                    # Dest + Count
  decl   %edi

_CopyOverlappedLoop: 
  cmpl  $0, %ecx
  je    _CopyMemDone
  movb  (%esi), %al                 # get byte from Source
  movb  %al, (%edi)                 # write byte to Destination
  decl   %ecx
  decl   %esi
  decl  %edi
  jmp   _CopyOverlappedLoop         # back to top of loop

_CopyMemDone: 

  popl  %edi
  popl  %esi
  leave
  ret
#EfiCommonLibCopyMem ENDP

