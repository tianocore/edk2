///** @file
//  
//  Contains low level routines for the Virtual Machine implementation
//  on an Itanium-based platform.
//
//  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//  
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//  
//**/

.file  "EbcLowLevel.s"

#define PROCEDURE_ENTRY(name)   .##text;            \
                                .##type name, @function;    \
                                .##proc name;           \
name::

#define PROCEDURE_EXIT(name)    .##endp name

// Note: use of NESTED_SETUP requires number of locals (l) >= 3

#define NESTED_SETUP(i,l,o,r) \
         alloc loc1=ar##.##pfs,i,l,o,r ;\
         mov loc0=b0

#define NESTED_RETURN \
         mov b0=loc0 ;\
         mov ar##.##pfs=loc1 ;;\
         br##.##ret##.##dpnt  b0;;

.type CopyMem, @function;

//-----------------------------------------------------------------------------
//++
// EbcAsmLLCALLEX
//
//  Implements the low level EBC CALLEX instruction. Sets up the
//  stack pointer, does the spill of function arguments, and
//  calls the native function. On return it restores the original
//  stack pointer and returns to the caller.
//
// Arguments :
//
// On Entry :
//    in0 = Address of native code to call
//    in1 = New stack pointer
//
// Return Value:
//
// As per static calling conventions.
//
//--
//---------------------------------------------------------------------------
;// void EbcAsmLLCALLEX (UINTN FunctionAddr, UINTN EbcStackPointer)
PROCEDURE_ENTRY(EbcAsmLLCALLEX)
  NESTED_SETUP (2,6,8,0)

  // NESTED_SETUP uses loc0 and loc1 for context save

  //
  // Save a copy of the EBC VM stack pointer
  //
  mov r8 = in1;;

  //
  // Copy stack arguments from EBC stack into registers.
  // Assume worst case and copy 8.
  //
  ld8   out0 = [r8], 8;;
  ld8   out1 = [r8], 8;;
  ld8   out2 = [r8], 8;;
  ld8   out3 = [r8], 8;;
  ld8   out4 = [r8], 8;;
  ld8   out5 = [r8], 8;;
  ld8   out6 = [r8], 8;;
  ld8   out7 = [r8], 8;;

  //
  // Save the original stack pointer
  //
  mov   loc2 = r12;

  //
  // Save the gp
  //
  or    loc3 = r1, r0

  //
  // Set the new aligned stack pointer. Reserve space for the required
  // 16-bytes of scratch area as well.
  //
  add  r12 = 48, in1

  //
  // Now call the function. Load up the function address from the descriptor
  // pointed to by in0. Then get the gp from the descriptor at the following
  // address in the descriptor.
  //
  ld8   r31 = [in0], 8;;
  ld8   r30 = [in0];;
  mov   b1 = r31
  mov   r1 = r30
  (p0) br.call.dptk.many b0 = b1;;

  //
  // Restore the original stack pointer and gp
  //
  mov   r12 = loc2
  or    r1 = loc3, r0

  //
  // Now return
  //
  NESTED_RETURN

PROCEDURE_EXIT(EbcAsmLLCALLEX)

//-----------------------------------------------------------------------------
//++
// EbcLLCALLEXNative
//
//  This function is called to execute an EBC CALLEX instruction.
//  This instruction requires that we thunk out to external native
//  code. On return, we restore the stack pointer to its original location.
//  Destroys no working registers.  For IPF, at least 8 register slots
//  must be allocated on the stack frame to support any number of 
//  arguments beiung passed to the external native function.  The
//  size of the stack frame is FramePtr - EbcSp.  If this size is less
//  than 64-bytes, the amount of stack frame allocated is rounded up
//  to 64-bytes 
//
// Arguments On Entry :
//    in0 = CallAddr     The function address.
//    in1 = EbcSp        The new EBC stack pointer.
//    in2 = FramePtr     The frame pointer.
//
// Return Value:
//    None
//
// C Function Prototype:
//    VOID
//    EFIAPI
//    EbcLLCALLEXNative (
//      IN UINTN        CallAddr,
//      IN UINTN        EbcSp,
//      IN VOID         *FramePtr
//      );
//--
//---------------------------------------------------------------------------

PROCEDURE_ENTRY(EbcLLCALLEXNative)
  NESTED_SETUP (3,6,3,0)

  mov   loc2 = in2;;              // loc2 = in2 = FramePtr
  mov   loc3 = in1;;              // loc3 = in1 = EbcSp
  sub   loc2 = loc2, loc3;;       // loc2 = loc2 - loc3 = FramePtr - EbcSp
  mov   out2 = loc2;;             // out2 = loc2 = FramePtr - EbcSp
  mov   loc4 = 0x40;;             // loc4 = 0x40
  cmp.leu p6  = out2, loc4;;      // IF out2 < loc4 THEN P6=1 ELSE P6=0; IF (FramePtr - EbcSp) < 0x40 THEN P6 = 1 ELSE P6=0
  (p6) mov   loc2 = loc4;;        // IF P6==1 THEN loc2 = loc4 = 0x40
  mov   loc4 = r12;;              // save sp
  or    loc5 = r1, r0             // save gp

  sub   r12 = r12, loc2;;         // sp = sp - loc2 = sp - MAX (0x40, FramePtr - EbcSp)

  and   r12 = -0x10, r12          // Round sp down to the nearest 16-byte boundary
  mov   out1 = in1;;              // out1 = EbcSp
  mov   out0 = r12;;              // out0 = sp
  adds  r12 = -0x8, r12           
  (p0) br.call.dptk.many b0 = CopyMem;;      // CopyMem (sp, EbcSp, (FramePtr - EbcSp))
  adds  r12 = 0x8, r12            

  mov   out0 = in0;;              // out0 = CallAddr
  mov   out1 = r12;;              // out1 = sp
  (p0) br.call.dptk.many b0 = EbcAsmLLCALLEX;;    // EbcAsmLLCALLEX (CallAddr, sp)
  mov   r12 = loc4;;              // restore sp
  or    r1 = loc5, r0             // restore gp

  NESTED_RETURN
PROCEDURE_EXIT(EbcLLCALLEXNative)


//
// UINTN EbcLLGetEbcEntryPoint(VOID)
//
// Description:
//    Simply return, so that the caller retrieves the return register
//    contents (R8). That's where the thunk-to-ebc code stuffed the
//    EBC entry point.
//
PROCEDURE_ENTRY(EbcLLGetEbcEntryPoint)
    br.ret.sptk  b0 ;;
PROCEDURE_EXIT(EbcLLGetEbcEntryPoint)







