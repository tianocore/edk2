/// @file
///  Low level IPF routines used by the debug support driver
///
/// Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
/// This program and the accompanying materials
/// are licensed and made available under the terms and conditions of the BSD License
/// which accompanies this distribution.  The full text of the license may be found at
/// http://opensource.org/licenses/bsd-license.php
///
/// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
/// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
///
///


#include "Common.i"
#include "Ds64Macros.i"

ASM_GLOBAL PatchSaveBuffer
ASM_GLOBAL IpfContextBuf
ASM_GLOBAL CommonHandler
ASM_GLOBAL ExternalInterruptCount


/////////////////////////////////////////////
//
//  Name:
//      InstructionCacheFlush
//
//  Description:
//      Flushes instruction cache for specified number of bytes
//
        ASM_GLOBAL InstructionCacheFlush
        .proc   InstructionCacheFlush
        .align 32
InstructionCacheFlush::
 {      .mii
        alloc   r3=2, 0, 0, 0
        cmp4.leu p0,p6=32, r33;;
        (p6)    mov r33=32;;
 }
 {      .mii
        nop.m    0
        zxt4    r29=r33;;
        dep.z   r30=r29, 0, 5;;
 }
 {      .mii
        cmp4.eq p0,p7=r0, r30
        shr.u   r28=r29, 5;;
        (p7)    adds    r28=1, r28;;
 }
 {      .mii
        nop.m    0
        shl r27=r28, 5;;
        zxt4    r26=r27;;
 }
 {      .mfb
        add r31=r26, r32
        nop.f    0
        nop.b    0
 }
LoopBack:   // $L143:
 {      .mii
        fc   r32
        adds    r32=32, r32;;
        cmp.ltu p14,p15=r32, r31
 }
 {      .mfb
        nop.m    0
        nop.f    0
        //(p14) br.cond.dptk.few $L143#;;
        (p14)   br.cond.dptk.few LoopBack;;
 }
 {      .mmi
        sync.i;;
        srlz.i
        nop.i   0;;
 }
 {      .mfb
        nop.m    0
        nop.f    0
        br.ret.sptk.few b0;;
 }
        .endp   InstructionCacheFlush


/////////////////////////////////////////////
//
//  Name:
//      ChainHandler
//
//  Description:
//      Chains an interrupt handler
//
//      The purpose of this function is to enable chaining of the external interrupt.
//      Since there's no clean SAL abstraction for doing this, we must do it
//      surreptitiously.
//
//      The reserved IVT entry at offset 0x3400 is coopted for use by this handler.
//      According to Itanium architecture, it is reserved.  Strictly speaking, this is
//      not safe, as we're cheating and violating the Itanium architecture.  However,
//      as long as we're the only ones cheating, we should be OK.  Without hooks in
//      the SAL to enable IVT management, there aren't many good options.
//
//      The strategy is to replace the first bundle of the external interrupt handler
//      with our own that will branch into a piece of code we've supplied and located
//      in the reserved IVT entry.  Only the first bundle of the external interrupt
//      IVT entry is modified.
//
//      The original bundle is moved and relocated to space
//      allocated within the reserved IVT entry.  The next bundle following is
//      is generated to go a hard coded branch back to the second bundle of the
//      external interrupt IVT entry just in case the first bundle had no branch.
//
//      Our new code will execute our handler, and then fall through to the
//      original bundle after restoring all context appropriately.
//
//      The following is a representation of what the IVT memory map looks like with
//      our chained handler installed:
//
//
//
//
//
//      This IVT entry is      Failsafe bundle
//      reserved by the
//      Itanium architecture   Original bundle 0
//      and is used for
//      for locating our
//      handler and the
//      original bundle        Patch code...
//      zero of the ext
//      interrupt handler
//
//      RSVD    (3400)         Unused
//
//
//
//
//
//
//
//
//
//
//
//
//      EXT_INT (3000)         Bundle 0               Bundle zero - This one is
//                                modified, all other bundles
//                                                       in the EXT_INT entry are
//                                                       untouched.
//
//
//       Arguments:
//
//       Returns:
//
//       Notes:
//
//
        ASM_GLOBAL ChainHandler
        .proc ChainHandler
ChainHandler:

        NESTED_SETUP( 0,2+3,3,0 )

        mov         r8=1                           // r8 = success
        mov         r2=cr.iva;;
//
// NOTE: There's a potential hazard here in that we're simply stealing a bunch of
// bundles (memory) from the IVT and assuming there's no catastrophic side effect.
//
// First, save IVT area we're taking over with the patch so we can restore it later
//
        addl        out0=PATCH_ENTRY_OFFSET, r2    // out0 = source buffer
        movl        out1=PatchSaveBuffer           // out1 = destination buffer
        mov         out2=0x40;;                    // out2 = number of bundles to copy... save entire IDT entry
        br.call.sptk.few    b0 = CopyBundles

// Next, copy the patch code into the IVT
        movl        out0=PatchCode                 // out0 = source buffer of patch code
        addl        out1=PATCH_OFFSET, r2          // out1 = destination buffer - in IVT
        mov         out2=PATCH_CODE_SIZE;;       
        shr         out2=out2, 4;;                 // out2 = number of bundles to copy
        br.call.sptk.few    b0 = CopyBundles


// copy original bundle 0 from the external interrupt handler to the
// appropriate place in the reserved IVT interrupt slot
        addl        out0=EXT_INT_ENTRY_OFFSET, r2  // out0 = source buffer
        addl        out1=RELOCATED_EXT_INT, r2     // out1 = destination buffer - in reserved IVT
        mov         out2=1;;                       // out2 = copy 1 bundle
        br.call.sptk.few    b0 = CopyBundles

// Now relocate it there because it very likely had a branch instruction that
// that must now be fixed up.
        addl        out0=RELOCATED_EXT_INT, r2     // out0 = new runtime address of bundle - in reserved IVT
        addl        out1=EXT_INT_ENTRY_OFFSET, r2;;// out1 = IP address of previous location
        mov         out2=out0;;                    // out2 = IP address of new location
        br.call.sptk.few    b0 = RelocateBundle

// Now copy into the failsafe branch into the next bundle just in case
// the original ext int bundle 0 bundle did not contain a branch instruction
        movl        out0=FailsafeBranch            // out0 = source buffer
        addl        out1=FAILSAFE_BRANCH_OFFSET, r2  // out1 = destination buffer - in reserved IVT
        mov         out2=1;;                       // out2 = copy 1 bundle
        br.call.sptk.few    b0 = CopyBundles

// Last, copy in our replacement for the external interrupt IVT entry bundle 0
        movl        out0=PatchCodeNewBun0          // out0 = source buffer - our replacement bundle 0
        addl        out1=EXT_INT_ENTRY_OFFSET, r2  // out1 = destination buffer - bundle 0 of External interrupt entry
        mov         out2=1;;                       // out2 = copy 1 bundle
        br.call.sptk.few    b0 = CopyBundles

ChainHandlerDone:
        NESTED_RETURN

        .endp ChainHandler


/////////////////////////////////////////////
//
//  Name:
//      UnchainHandler
//
//  Description:
//      Unchains an interrupt handler
//
//  Arguments:
//
//  Returns:
//
//  Notes:
//
//
        ASM_GLOBAL UnchainHandler
        .proc UnchainHandler

UnchainHandler:

        NESTED_SETUP( 0,2+3,3,0 )

        mov         r8=1                        // r8 = success
        mov         r2=cr.iva;;                 // r2 = interrupt vector address

// First copy original Ext Int bundle 0 back to it's proper home...
        addl        out0=RELOCATED_EXT_INT, r2     // out0 = source - in reserved IVT
        addl        out1=EXT_INT_ENTRY_OFFSET, r2  // out1 = destination buffer - first bundle of Ext Int entry
        mov         out2=1;;                       // out2 = copy 1 bundle
        br.call.sptk.few    b0 = CopyBundles

// Now, relocate it again...
        addl        out0=EXT_INT_ENTRY_OFFSET, r2  // out1 = New runtime address
        addl        out1=RELOCATED_EXT_INT, r2;;   // out0 = IP address of previous location
        mov         out2=out0;;                    // out2 = IP address of new location
        br.call.sptk.few    b0 = RelocateBundle

// Last, restore the patch area
        movl        out0=PatchSaveBuffer           // out0 = source buffer
        addl        out1=PATCH_ENTRY_OFFSET, r2    // out1 = destination buffer
        mov         out2=0x40;;                    // out2 = number of bundles to copy... save entire IDT entry
        br.call.sptk.few    b0 = CopyBundles

UnchainHandlerDone:
        NESTED_RETURN

        .endp UnchainHandler


/////////////////////////////////////////////
//
//  Name:
//      CopyBundles
//
//  Description:
//      Copies instruction bundles - flushes icache as necessary
//
//  Arguments:
//      in0 - Bundle source
//      in1 - Bundle destination
//      in2 - Bundle count
//
//  Returns:
//
//  Notes:
//      This procedure is a leaf routine
//
        .proc   CopyBundles

CopyBundles:

        NESTED_SETUP(3,2+1,0,0)

        shl         in2=in2, 1;;                // in2 = count of 8 byte blocks to copy

CopyBundlesLoop:

        cmp.eq      p14, p15 = 0, in2;;         // Check if done
(p14)   br.sptk.few CopyBundlesDone;;

        ld8         loc2=[in0], 0x8;;           // loc2 = source bytes
        st8         [in1]=loc2;;                // [in1] = destination bytes
        fc          in1;;                       // Flush instruction cache
        sync.i;;                                // Ensure local and remote data/inst caches in sync
        srlz.i;;                                // Ensure sync has been observed
        add         in1=0x8, in1;;              // in1 = next destination
        add         in2=-1, in2;;               // in2 = decrement 8 bytes blocks to copy
        br.sptk.few CopyBundlesLoop;;

CopyBundlesDone:
        NESTED_RETURN

        .endp   CopyBundles


/////////////////////////////////////////////
//
//  Name:
//      RelocateBundle
//
//  Description:
//      Relocates an instruction bundle by updating any ip-relative branch instructions.
//
//  Arguments:
//      in0 - Runtime address of bundle
//      in1 - IP address of previous location of bundle
//      in2 - IP address of new location of bundle
//
//  Returns:
//      in0 - 1 if successful or 0 if unsuccessful
//
//  Notes:
//      This routine examines all slots in the given bundle that are destined for the
//      branch execution unit.  If any of these slots contain an IP-relative branch
//      namely instructions B1, B2, B3, or B6, the slot is fixed-up with a new relative
//      address.  Errors can occur if a branch cannot be reached.
//
        .proc   RelocateBundle

RelocateBundle:

        NESTED_SETUP(3,2+4,3,0)

        mov         loc2=SLOT0                  // loc2 = slot index
        mov         loc5=in0;;                  // loc5 = runtime address of bundle
        mov         in0=1;;                     // in0 = success

RelocateBundleNextSlot:

        cmp.ge      p14, p15 = SLOT2, loc2;;    // Check if maximum slot
(p15)   br.sptk.few RelocateBundleDone

        mov         out0=loc5;;                 // out0 = runtime address of bundle
        br.call.sptk.few    b0 = GetTemplate
        mov         loc3=out0;;                 // loc3 = instruction template
        mov         out0=loc5                   // out0 = runtime address of bundle
        mov         out1=loc2;;                 // out1 = instruction slot number
        br.call.sptk.few    b0 = GetSlot
        mov         loc4=out0;;                 // loc4 = instruction encoding
        mov         out0=loc4                   // out0 = instuction encoding
        mov         out1=loc2                   // out1 = instruction slot number
        mov         out2=loc3;;                 // out2 = instruction template
        br.call.sptk.few    b0 = IsSlotBranch
        cmp.eq      p14, p15 = 1, out0;;        // Check if branch slot
(p15)   add         loc2=1,loc2                 // Increment slot
(p15)   br.sptk.few RelocateBundleNextSlot
        mov         out0=loc4                   // out0 = instuction encoding
        mov         out1=in1                    // out1 = IP address of previous location
        mov         out2=in2;;                  // out2 = IP address of new location
        br.call.sptk.few    b0 = RelocateSlot
        cmp.eq      p14, p15 = 1, out1;;        // Check if relocated slot
(p15)   mov         in0=0                       // in0 = failure
(p15)   br.sptk.few RelocateBundleDone
        mov         out2=out0;;                 // out2 = instruction encoding
        mov         out0=loc5                   // out0 = runtime address of bundle
        mov         out1=loc2;;                 // out1 = instruction slot number
        br.call.sptk.few    b0 = SetSlot
        add         loc2=1,loc2;;               // Increment slot
        br.sptk.few RelocateBundleNextSlot

RelocateBundleDone:
        NESTED_RETURN

        .endp   RelocateBundle


/////////////////////////////////////////////
//
//  Name:
//      RelocateSlot
//
//  Description:
//      Relocates an instruction bundle by updating any ip-relative branch instructions.
//
//  Arguments:
//      in0 - Instruction encoding (41-bits, right justified)
//      in1 - IP address of previous location of bundle
//      in2 - IP address of new location of bundle
//
//  Returns:
//      in0 - Instruction encoding (41-bits, right justified)
//      in1 - 1 if successful otherwise 0
//
//  Notes:
//      This procedure is a leaf routine
//
        .proc   RelocateSlot

RelocateSlot:
        NESTED_SETUP(3,2+5,0,0)
        extr.u      loc2=in0, 37, 4;;           // loc2 = instruction opcode
        cmp.eq      p14, p15 = 4, loc2;;        // IP-relative branch (B1) or
                                                // IP-relative counted branch (B2)
(p15)   cmp.eq      p14, p15 = 5, loc2;;        // IP-relative call (B3)
(p15)   cmp.eq      p14, p15 = 7, loc2;;        // IP-relative predict (B6)
(p15)   mov         in1=1                       // Instruction did not need to be reencoded
(p15)   br.sptk.few RelocateSlotDone
        tbit.nz     p14, p15 = in0, 36;;        // put relative offset sign bit in p14
        extr.u      loc2=in0, 13, 20;;          // loc2 = relative offset in instruction
(p14)   movl        loc3=0xfffffffffff00000;;   // extend sign
(p14)   or          loc2=loc2, loc3;;
        shl         loc2=loc2,4;;               // convert to byte offset instead of bundle offset
        add         loc3=loc2, in1;;            // loc3 = physical address of branch target
(p14)   sub         loc2=r0,loc2;;              // flip sign in loc2 if offset is negative
        sub         loc4=loc3,in2;;             // loc4 = relative offset from new ip to branch target
        cmp.lt      p15, p14 = 0, loc4;;        // get new sign bit
(p14)   sub         loc5=r0,loc4                // get absolute value of offset
(p15)   mov         loc5=loc4;;
        movl        loc6=0x0FFFFFF;;            // maximum offset in bytes for ip-rel branch
        cmp.gt      p14, p15 = loc5, loc6;;     // check to see we're not out of range for an ip-relative branch
(p14)   br.sptk.few RelocateSlotError
        cmp.lt      p15, p14 = 0, loc4;;        // store sign in p14 again
(p14)   dep         in0=-1,in0,36,1              // store sign bit in instruction
(p15)   dep         in0=0,in0,36,1
        shr         loc4=loc4, 4;;              // convert back to bundle offset
        dep         in0=loc4,in0,13,16;;        // put first 16 bits of new offset into instruction
        shr         loc4=loc4,16;;
        dep         in0=loc4,in0,13+16,4        // put last 4 bits of new offset into instruction
        mov         in1=1;;                     // in1 = success
        br.sptk.few RelocateSlotDone;;

RelocateSlotError:
        mov         in1=0;;                     // in1 = failure

RelocateSlotDone:
        NESTED_RETURN

        .endp   RelocateSlot


/////////////////////////////////////////////
//
//  Name:
//      IsSlotBranch
//
//  Description:
//      Determines if the given instruction is a branch instruction.
//
//  Arguments:
//      in0 - Instruction encoding (41-bits, right justified)
//      in1 - Instruction slot number
//      in2 - Bundle template
//
//  Returns:
//      in0 - 1 if branch or 0 if not branch
//
//  Notes:
//      This procedure is a leaf routine
//
//      IsSlotBranch recognizes all branch instructions by looking at the provided template.
//      The instruction encoding is only passed to this routine for future expansion.
//
        .proc   IsSlotBranch

IsSlotBranch:

        NESTED_SETUP (3,2+0,0,0)

        mov         in0=1;;                     // in0 = 1 which destroys the instruction
        andcm       in2=in2,in0;;               // in2 = even template to reduce compares
        mov         in0=0;;                     // in0 = not a branch
        cmp.eq      p14, p15 = 0x16, in2;;      // Template 0x16 is BBB
(p14)   br.sptk.few IsSlotBranchTrue
        cmp.eq      p14, p15 = SLOT0, in1;;     // Slot 0 has no other possiblities
(p14)   br.sptk.few IsSlotBranchDone
        cmp.eq      p14, p15 = 0x12, in2;;      // Template 0x12 is MBB
(p14)   br.sptk.few IsSlotBranchTrue
        cmp.eq      p14, p15 = SLOT1, in1;;     // Slot 1 has no other possiblities
(p14)   br.sptk.few IsSlotBranchDone
        cmp.eq      p14, p15 = 0x10, in2;;      // Template 0x10 is MIB
(p14)   br.sptk.few IsSlotBranchTrue
        cmp.eq      p14, p15 = 0x18, in2;;      // Template 0x18 is MMB
(p14)   br.sptk.few IsSlotBranchTrue
        cmp.eq      p14, p15 = 0x1C, in2;;      // Template 0x1C is MFB
(p14)   br.sptk.few IsSlotBranchTrue
        br.sptk.few IsSlotBranchDone

IsSlotBranchTrue:
        mov         in0=1;;                     // in0 = branch

IsSlotBranchDone:
        NESTED_RETURN

        .endp   IsSlotBranch


/////////////////////////////////////////////
//
//  Name:
//      GetTemplate
//
//  Description:
//      Retrieves the instruction template for an instruction bundle
//
//  Arguments:
//      in0 - Runtime address of bundle
//
//  Returns:
//      in0 - Instruction template (5-bits, right-justified)
//
//  Notes:
//      This procedure is a leaf routine
//
        .proc   GetTemplate

GetTemplate:

        NESTED_SETUP (1,2+2,0,0)

        ld8     loc2=[in0], 0x8             // loc2 = first 8 bytes of branch bundle
        movl    loc3=MASK_0_4;;             // loc3 = template mask
        and     loc2=loc2,loc3;;            // loc2 = template, right justified
        mov     in0=loc2;;                  // in0 = template, right justified

        NESTED_RETURN

        .endp   GetTemplate


/////////////////////////////////////////////
//
//  Name:
//      GetSlot
//
//  Description:
//      Gets the instruction encoding for an instruction slot and bundle
//
//  Arguments:
//      in0 - Runtime address of bundle
//      in1 - Instruction slot (either 0, 1, or 2)
//
//  Returns:
//      in0 - Instruction encoding (41-bits, right justified)
//
//  Notes:
//      This procedure is a leaf routine
//
//      Slot0 - [in0 + 0x8] Bits 45-5
//      Slot1 - [in0 + 0x8] Bits 63-46 and [in0] Bits 22-0
//      Slot2 - [in0] Bits 63-23
//
        .proc   GetSlot

GetSlot:
        NESTED_SETUP (2,2+3,0,0)

        ld8     loc2=[in0], 0x8;;           // loc2 = first 8 bytes of branch bundle
        ld8     loc3=[in0];;                // loc3 = second 8 bytes of branch bundle
        cmp.eq  p14, p15 = 2, in1;;         // check if slot 2 specified
 (p14)  br.cond.sptk.few    GetSlot2;;      // get slot 2
        cmp.eq  p14, p15 = 1, in1;;         // check if slot 1 specified
 (p14)  br.cond.sptk.few    GetSlot1;;      // get slot 1

GetSlot0:
        extr.u  in0=loc2, 5, 45             // in0 = extracted slot 0
        br.sptk.few GetSlotDone;;

GetSlot1:
        extr.u  in0=loc2, 46, 18            // in0 = bits 63-46 of loc2 right-justified
        extr.u  loc4=loc3, 0, 23;;          // loc4 = bits 22-0 of loc3 right-justified
        dep     in0=loc4, in0, 18, 15;;
        shr.u   loc4=loc4,15;;
        dep     in0=loc4, in0, 33, 8;;      // in0 = extracted slot 1
        br.sptk.few GetSlotDone;;

GetSlot2:
        extr.u  in0=loc3, 23, 41;;          // in0 = extracted slot 2

GetSlotDone:
        NESTED_RETURN

        .endp   GetSlot


/////////////////////////////////////////////
//
//  Name:
//      SetSlot
//
//  Description:
//      Sets the instruction encoding for an instruction slot and bundle
//
//  Arguments:
//      in0 - Runtime address of bundle
//      in1 - Instruction slot (either 0, 1, or 2)
//      in2 - Instruction encoding (41-bits, right justified)
//
//  Returns:
//
//  Notes:
//      This procedure is a leaf routine
//
        .proc       SetSlot

SetSlot:
        NESTED_SETUP (3,2+3,0,0)

        ld8     loc2=[in0], 0x8;;           // loc2 = first 8 bytes of bundle
        ld8     loc3=[in0];;                // loc3 = second 8 bytes of bundle
        cmp.eq  p14, p15 = 2, in1;;         // check if slot 2 specified
 (p14)  br.cond.sptk.few    SetSlot2;;      // set slot 2
        cmp.eq  p14, p15 = 1, in1;;         // check if slot 1 specified
 (p14)  br.cond.sptk.few    SetSlot1;;      // set slot 1

SetSlot0:
        dep     loc2=0, loc2, 5, 41;;       // remove old instruction from slot 0
        shl     loc4=in2, 5;;               // loc4 = new instruction ready to be inserted
        or      loc2=loc2, loc4;;           // loc2 = updated first 8 bytes of bundle
        add     loc4=0x8,in0;;              // loc4 = address to store first 8 bytes of bundle
        st8     [loc4]=loc2                 // [loc4] = updated bundle
        br.sptk.few SetSlotDone;;
        ;;

SetSlot1:
        dep     loc2=0, loc2, 46, 18        // remove old instruction from slot 1
        dep     loc3=0, loc3, 0, 23;;
        shl     loc4=in2, 46;;              // loc4 = partial instruction ready to be inserted
        or      loc2=loc2, loc4;;           // loc2 = updated first 8 bytes of bundle
        add     loc4=0x8,in0;;              // loc4 = address to store first 8 bytes of bundle
        st8     [loc4]=loc2;;               // [loc4] = updated bundle
        shr.u   loc4=in2, 18;;              // loc4 = partial instruction ready to be inserted
        or      loc3=loc3, loc4;;           // loc3 = updated second 8 bytes of bundle
        st8     [in0]=loc3;;                // [in0] = updated bundle
        br.sptk.few SetSlotDone;;

SetSlot2:
        dep     loc3=0, loc3, 23, 41;;      // remove old instruction from slot 2
        shl     loc4=in2, 23;;              // loc4 = instruction ready to be inserted
        or      loc3=loc3, loc4;;           // loc3 = updated second 8 bytes of bundle
        st8     [in0]=loc3;;                // [in0] = updated bundle

SetSlotDone:

        NESTED_RETURN
        .endp       SetSlot


/////////////////////////////////////////////
//
//  Name:
//      GetIva
//
//  Description:
//      C callable function to obtain the current value of IVA
//
//  Returns:
//      Current value if IVA

        ASM_GLOBAL     GetIva
        .proc       GetIva
GetIva:
        mov         r8=cr2;;
        br.ret.sptk.many    b0

        .endp       GetIva


/////////////////////////////////////////////
//
//  Name:
//      ProgramInterruptFlags
//
//  Description:
//      C callable function to enable/disable interrupts
//
//  Returns:
//      Previous state of psr.ic
//
        ASM_GLOBAL     ProgramInterruptFlags
        .proc       ProgramInterruptFlags
ProgramInterruptFlags:
        alloc       loc0=1,2,0,0;;
        mov         loc0=psr
        mov         loc1=0x6000;;
        and         r8=loc0, loc1           // obtain current psr.ic and psr.i state
        and         in0=in0, loc1           // insure no extra bits set in input
        andcm       loc0=loc0,loc1;;        // clear original psr.i and psr.ic
        or          loc0=loc0,in0;;         // OR in new psr.ic value
        mov         psr.l=loc0;;            // write new psr
        srlz.d
        br.ret.sptk.many    b0              // return

        .endp       ProgramInterruptFlags


/////////////////////////////////////////////
//
//  Name:
//      SpillContext
//
//  Description:
//      Saves system context to context record.
//
//  Arguments:
//          in0 = 512 byte aligned context record address
//          in1 = original B0
//          in2 = original ar.bsp
//          in3 = original ar.bspstore
//          in4 = original ar.rnat
//          in5 = original ar.pfs
//
//  Notes:
//      loc0 - scratch
//      loc1 - scratch
//      loc2 - temporary application unat storage
//      loc3 - temporary exception handler unat storage

        .proc       SpillContext

SpillContext:
        alloc       loc0=6,4,0,0;;          // alloc 6 input, 4 locals, 0 outs
        mov         loc2=ar.unat;;          // save application context unat (spilled later)
        mov         ar.unat=r0;;            // set UNAT=0
        st8.spill   [in0]=r0,8;;
        st8.spill   [in0]=r1,8;;            // save R1 - R31
        st8.spill   [in0]=r2,8;;
        st8.spill   [in0]=r3,8;;
        st8.spill   [in0]=r4,8;;
        st8.spill   [in0]=r5,8;;
        st8.spill   [in0]=r6,8;;
        st8.spill   [in0]=r7,8;;
        st8.spill   [in0]=r8,8;;
        st8.spill   [in0]=r9,8;;
        st8.spill   [in0]=r10,8;;
        st8.spill   [in0]=r11,8;;
        st8.spill   [in0]=r12,8;;
        st8.spill   [in0]=r13,8;;
        st8.spill   [in0]=r14,8;;
        st8.spill   [in0]=r15,8;;
        st8.spill   [in0]=r16,8;;
        st8.spill   [in0]=r17,8;;
        st8.spill   [in0]=r18,8;;
        st8.spill   [in0]=r19,8;;
        st8.spill   [in0]=r20,8;;
        st8.spill   [in0]=r21,8;;
        st8.spill   [in0]=r22,8;;
        st8.spill   [in0]=r23,8;;
        st8.spill   [in0]=r24,8;;
        st8.spill   [in0]=r25,8;;
        st8.spill   [in0]=r26,8;;
        st8.spill   [in0]=r27,8;;
        st8.spill   [in0]=r28,8;;
        st8.spill   [in0]=r29,8;;
        st8.spill   [in0]=r30,8;;
        st8.spill   [in0]=r31,8;;
        mov         loc3=ar.unat;;          // save debugger context unat (spilled later)
        stf.spill   [in0]=f2,16;;           // save f2 - f31
        stf.spill   [in0]=f3,16;;
        stf.spill   [in0]=f4,16;;
        stf.spill   [in0]=f5,16;;
        stf.spill   [in0]=f6,16;;
        stf.spill   [in0]=f7,16;;
        stf.spill   [in0]=f8,16;;
        stf.spill   [in0]=f9,16;;
        stf.spill   [in0]=f10,16;;
        stf.spill   [in0]=f11,16;;
        stf.spill   [in0]=f12,16;;
        stf.spill   [in0]=f13,16;;
        stf.spill   [in0]=f14,16;;
        stf.spill   [in0]=f15,16;;
        stf.spill   [in0]=f16,16;;
        stf.spill   [in0]=f17,16;;
        stf.spill   [in0]=f18,16;;
        stf.spill   [in0]=f19,16;;
        stf.spill   [in0]=f20,16;;
        stf.spill   [in0]=f21,16;;
        stf.spill   [in0]=f22,16;;
        stf.spill   [in0]=f23,16;;
        stf.spill   [in0]=f24,16;;
        stf.spill   [in0]=f25,16;;
        stf.spill   [in0]=f26,16;;
        stf.spill   [in0]=f27,16;;
        stf.spill   [in0]=f28,16;;
        stf.spill   [in0]=f29,16;;
        stf.spill   [in0]=f30,16;;
        stf.spill   [in0]=f31,16;;
        mov         loc0=pr;;               // save predicates
        st8.spill   [in0]=loc0,8;;
        st8.spill   [in0]=in1,8;;           // save b0 - b7... in1 already equals saved b0
        mov         loc0=b1;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b2;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b3;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b4;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b5;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b6;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=b7;;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.rsc;;           // save ar.rsc
        st8.spill   [in0]=loc0,8;;
        st8.spill   [in0]=in2,8;;           // save ar.bsp (in2)
        st8.spill   [in0]=in3,8;;           // save ar.bspstore (in3)
        st8.spill   [in0]=in4,8;;           // save ar.rnat (in4)
        mov         loc0=ar.fcr;;           // save ar.fcr (ar21 - IA32 floating-point control register)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.eflag;;         // save ar.eflag (ar24)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.csd;;           // save ar.csd (ar25 - ia32 CS descriptor)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.ssd;;           // save ar.ssd (ar26 - ia32 ss descriptor)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.cflg;;          // save ar.cflg (ar27 - ia32 cr0 and cr4)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.fsr;;           // save ar.fsr (ar28 - ia32 floating-point status register)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.fir;;           // save ar.fir (ar29 - ia32 floating-point instruction register)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.fdr;;           // save ar.fdr (ar30 - ia32 floating-point data register)
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.ccv;;           // save ar.ccv
        st8.spill   [in0]=loc0,8;;
        st8.spill   [in0]=loc2,8;;          // save ar.unat (saved to loc2 earlier)
        mov         loc0=ar.fpsr;;          // save floating point status register
        st8.spill   [in0]=loc0,8;;
        st8.spill   [in0]=in5,8;;           // save ar.pfs
        mov         loc0=ar.lc;;            // save ar.lc
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ar.ec;;            // save ar.ec
        st8.spill   [in0]=loc0,8;;

        // save control registers
        mov         loc0=cr.dcr;;           // save dcr
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.itm;;           // save itm
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.iva;;           // save iva
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.pta;;           // save pta
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.ipsr;;          // save ipsr
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.isr;;           // save isr
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.iip;;           // save iip
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.ifa;;           // save ifa
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.itir;;          // save itir
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.iipa;;          // save iipa
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.ifs;;           // save ifs
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.iim;;           // save iim
        st8.spill   [in0]=loc0,8;;
        mov         loc0=cr.iha;;           // save iha
        st8.spill   [in0]=loc0,8;;

        // save debug registers
        mov         loc0=dbr[r0];;          // save dbr0 - dbr7
        st8.spill   [in0]=loc0,8;;
        movl        loc1=1;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=2;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=3;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=4;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=5;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=6;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=7;;
        mov         loc0=dbr[loc1];;
        st8.spill   [in0]=loc0,8;;
        mov         loc0=ibr[r0];;          // save ibr0 - ibr7
        st8.spill   [in0]=loc0,8;;
        movl        loc1=1;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=2;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=3;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=4;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=5;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=6;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        movl        loc1=7;;
        mov         loc0=ibr[loc1];;
        st8.spill   [in0]=loc0,8;;
        st8.spill   [in0]=loc3;;

        br.ret.sptk.few     b0

        .endp       SpillContext


/////////////////////////////////////////////
//
//  Name:
//      FillContext
//
//  Description:
//      Restores register context from context record.
//
//  Arguments:
//          in0 = address of last element 512 byte aligned context record address
//          in1 = modified B0
//          in2 = modified ar.bsp
//          in3 = modified ar.bspstore
//          in4 = modified ar.rnat
//          in5 = modified ar.pfs
//
//  Notes:
//      loc0 - scratch
//      loc1 - scratch
//      loc2 - temporary application unat storage
//      loc3 - temporary exception handler unat storage

        .proc       FillContext
FillContext:
        alloc       loc0=6,4,0,0;;          // alloc 6 inputs, 4 locals, 0 outs
        ld8.fill    loc3=[in0],-8;;         // int_nat (nat bits for R1-31)
        movl        loc1=7;;                // ibr7
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=6;;                // ibr6
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=5;;                // ibr5
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=4;;                // ibr4
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=3;;                // ibr3
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=2;;                // ibr2
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        movl        loc1=1;;                // ibr1
        ld8.fill    loc0=[in0],-8;;
        mov         ibr[loc1]=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ibr0
        mov         ibr[r0]=loc0;;
        movl        loc1=7;;                // dbr7
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=6;;                // dbr6
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=5;;                // dbr5
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=4;;                // dbr4
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=3;;                // dbr3
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=2;;                // dbr2
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        movl        loc1=1;;                // dbr1
        ld8.fill    loc0=[in0],-8;;
        mov         dbr[loc1]=loc0;;
        ld8.fill    loc0=[in0],-8;;         // dbr0
        mov         dbr[r0]=loc0;;
        ld8.fill    loc0=[in0],-8;;         // iha
        mov         cr.iha=loc0;;
        ld8.fill    loc0=[in0],-8;;         // iim
        mov         cr.iim=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ifs
        mov         cr.ifs=loc0;;
        ld8.fill    loc0=[in0],-8;;         // iipa
        mov         cr.iipa=loc0;;
        ld8.fill    loc0=[in0],-8;;         // itir
        mov         cr.itir=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ifa
        mov         cr.ifa=loc0;;
        ld8.fill    loc0=[in0],-8;;         // iip
        mov         cr.iip=loc0;;
        ld8.fill    loc0=[in0],-8;;         // isr
        mov         cr.isr=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ipsr
        mov         cr.ipsr=loc0;;
        ld8.fill    loc0=[in0],-8;;         // pta
        mov         cr.pta=loc0;;
        ld8.fill    loc0=[in0],-8;;         // iva
        mov         cr.iva=loc0;;
        ld8.fill    loc0=[in0],-8;;         // itm
        mov         cr.itm=loc0;;
        ld8.fill    loc0=[in0],-8;;         // dcr
        mov         cr.dcr=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ec
        mov         ar.ec=loc0;;
        ld8.fill    loc0=[in0],-8;;         // lc
        mov         ar.lc=loc0;;
        ld8.fill    in5=[in0],-8;;          // ar.pfs
        ld8.fill    loc0=[in0],-8;;         // ar.fpsr
        mov         ar.fpsr=loc0;;
        ld8.fill    loc2=[in0],-8;;         // ar.unat - restored later...
        ld8.fill    loc0=[in0],-8;;         // ar.ccv
        mov         ar.ccv=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.fdr
        mov         ar.fdr=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.fir
        mov         ar.fir=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.fsr
        mov         ar.fsr=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.cflg
        mov         ar.cflg=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.ssd
        mov         ar.ssd=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.csd
        mov         ar.csd=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.eflag
        mov         ar.eflag=loc0;;
        ld8.fill    loc0=[in0],-8;;         // ar.fcr
        mov         ar.fcr=loc0;;
        ld8.fill    in4=[in0],-8;;          // ar.rnat
        ld8.fill    in3=[in0],-8;;          // bspstore
        ld8.fill    in2=[in0],-8;;          // bsp
        ld8.fill    loc0=[in0],-8;;         // ar.rsc
        mov         ar.rsc=loc0;;
        ld8.fill    loc0=[in0],-8;;         // B7 - B0
        mov         b7=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b6=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b5=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b4=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b3=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b2=loc0;;
        ld8.fill    loc0=[in0],-8;;
        mov         b1=loc0;;
        ld8.fill    in1=[in0],-8;;          // b0 is temporarily stored in in1
        ld8.fill    loc0=[in0],-16;;        // predicates
        mov         pr=loc0;;
        ldf.fill    f31=[in0],-16;;
        ldf.fill    f30=[in0],-16;;
        ldf.fill    f29=[in0],-16;;
        ldf.fill    f28=[in0],-16;;
        ldf.fill    f27=[in0],-16;;
        ldf.fill    f26=[in0],-16;;
        ldf.fill    f25=[in0],-16;;
        ldf.fill    f24=[in0],-16;;
        ldf.fill    f23=[in0],-16;;
        ldf.fill    f22=[in0],-16;;
        ldf.fill    f21=[in0],-16;;
        ldf.fill    f20=[in0],-16;;
        ldf.fill    f19=[in0],-16;;
        ldf.fill    f18=[in0],-16;;
        ldf.fill    f17=[in0],-16;;
        ldf.fill    f16=[in0],-16;;
        ldf.fill    f15=[in0],-16;;
        ldf.fill    f14=[in0],-16;;
        ldf.fill    f13=[in0],-16;;
        ldf.fill    f12=[in0],-16;;
        ldf.fill    f11=[in0],-16;;
        ldf.fill    f10=[in0],-16;;
        ldf.fill    f9=[in0],-16;;
        ldf.fill    f8=[in0],-16;;
        ldf.fill    f7=[in0],-16;;
        ldf.fill    f6=[in0],-16;;
        ldf.fill    f5=[in0],-16;;
        ldf.fill    f4=[in0],-16;;
        ldf.fill    f3=[in0],-16;;
        ldf.fill    f2=[in0],-8;;
        mov         ar.unat=loc3;;          // restore unat (int_nat) before fill of general registers
        ld8.fill    r31=[in0],-8;;
        ld8.fill    r30=[in0],-8;;
        ld8.fill    r29=[in0],-8;;
        ld8.fill    r28=[in0],-8;;
        ld8.fill    r27=[in0],-8;;
        ld8.fill    r26=[in0],-8;;
        ld8.fill    r25=[in0],-8;;
        ld8.fill    r24=[in0],-8;;
        ld8.fill    r23=[in0],-8;;
        ld8.fill    r22=[in0],-8;;
        ld8.fill    r21=[in0],-8;;
        ld8.fill    r20=[in0],-8;;
        ld8.fill    r19=[in0],-8;;
        ld8.fill    r18=[in0],-8;;
        ld8.fill    r17=[in0],-8;;
        ld8.fill    r16=[in0],-8;;
        ld8.fill    r15=[in0],-8;;
        ld8.fill    r14=[in0],-8;;
        ld8.fill    r13=[in0],-8;;
        ld8.fill    r12=[in0],-8;;
        ld8.fill    r11=[in0],-8;;
        ld8.fill    r10=[in0],-8;;
        ld8.fill    r9=[in0],-8;;
        ld8.fill    r8=[in0],-8;;
        ld8.fill    r7=[in0],-8;;
        ld8.fill    r6=[in0],-8;;
        ld8.fill    r5=[in0],-8;;
        ld8.fill    r4=[in0],-8;;
        ld8.fill    r3=[in0],-8;;
        ld8.fill    r2=[in0],-8;;
        ld8.fill    r1=[in0],-8;;
        mov         ar.unat=loc2;;          // restore application context unat

        br.ret.sptk.many    b0

        .endp       FillContext


/////////////////////////////////////////////
//
//  Name:
//      HookHandler
//
//  Description:
//      Common branch target from hooked IVT entries.  Runs in interrupt context.
//      Responsible for saving and restoring context and calling common C
//      handler.  Banked registers running on bank 0 at entry.
//
//  Arguments:
//      All arguments are passed in banked registers:
//          B0_REG = Original B0
//          SCRATCH_REG1 = IVT entry index
//
//  Returns:
//      Returns via rfi
//
//  Notes:
//      loc0 - scratch
//      loc1 - scratch
//      loc2 - vector number / mask
//      loc3 - 16 byte aligned context record address
//      loc4 - temporary storage of last address in context record

HookHandler:
        flushrs;;                               // Synch RSE with backing store
        mov         SCRATCH_REG2=ar.bsp         // save interrupted context bsp
        mov         SCRATCH_REG3=ar.bspstore    // save interrupted context bspstore
        mov         SCRATCH_REG4=ar.rnat        // save interrupted context rnat
        mov         SCRATCH_REG6=cr.ifs;;       // save IFS in case we need to chain...
        cover;;                                 // creates new frame, moves old
                                                //   CFM to IFS.
        alloc       SCRATCH_REG5=0,5,6,0        // alloc 5 locals, 6 outs
        ;;
        // save banked registers to locals
        mov         out1=B0_REG                 // out1 = Original B0
        mov         out2=SCRATCH_REG2           // out2 = original ar.bsp
        mov         out3=SCRATCH_REG3           // out3 = original ar.bspstore
        mov         out4=SCRATCH_REG4           // out4 = original ar.rnat
        mov         out5=SCRATCH_REG5           // out5 = original ar.pfs
        mov         loc2=SCRATCH_REG1;;         // loc2 = vector number + chain flag
        bsw.1;;                                 // switch banked registers to bank 1
        srlz.d                                  // explicit serialize required
                                                // now fill in context record structure
        movl        loc3=IpfContextBuf          // Insure context record is aligned
        add         loc0=-0x200,r0;;            // mask the lower 9 bits (align on 512 byte boundary)
        and         loc3=loc3,loc0;;
        add         loc3=0x200,loc3;;           // move to next 512 byte boundary
                                                // loc3 now contains the 512 byte aligned context record
                                                // spill register context into context record
        mov         out0=loc3;;                 // Context record base in out0
                                                // original B0 in out1 already
                                                // original ar.bsp in out2 already
                                                // original ar.bspstore in out3 already
        br.call.sptk.few b0=SpillContext;;      // spill context
        mov         loc4=out0                   // save modified address

    // At this point, the context has been saved to the context record and we're
    // ready to call the C part of the handler...

        movl        loc0=CommonHandler;;        // obtain address of plabel
        ld8         loc1=[loc0];;               // get entry point of CommonHandler
        mov         b6=loc1;;                   // put it in a branch register
        adds        loc1= 8, loc0;;             // index to GP in plabel
        ld8         r1=[loc1];;                 // set up gp for C call
        mov         loc1=0xfffff;;              // mask off so only vector bits are present
        and         out0=loc2,loc1;;            // pass vector number (exception type)
        mov         out1=loc3;;                 // pass context record address
        br.call.sptk.few b0=b6;;                // call C handler

    // We've returned from the C call, so restore the context and either rfi
    // back to interrupted thread, or chain into the SAL if this was an external interrupt
        mov         out0=loc4;;                 // pass address of last element in context record
        br.call.sptk.few b0=FillContext;;       // Fill context
        mov         b0=out1                     // fill in b0
        mov         ar.rnat=out4
        mov         ar.pfs=out5

  // Loadrs is necessary because the debugger may have changed some values in
  // the backing store.  The processor, however may not be aware that the
  // stacked registers need to be reloaded from the backing store.  Therefore,
  // we explicitly cause the RSE to refresh the stacked register's contents
  // from the backing store.
        mov         loc0=ar.rsc                 // get RSC value
        mov         loc1=ar.rsc                 // save it so we can restore it
        movl        loc3=0xffffffffc000ffff;;   // create mask for clearing RSC.loadrs
        and         loc0=loc0,loc3;;            // create value for RSC with RSC.loadrs==0
        mov         ar.rsc=loc0;;               // modify RSC
        loadrs;;                                // invalidate register stack
        mov         ar.rsc=loc1;;               // restore original RSC

        bsw.0;;                                 // switch banked registers back to bank 0
        srlz.d;;                                // explicit serialize required
        mov         PR_REG=pr                   // save predicates - to be restored after chaining decision
        mov         B0_REG=b0                   // save b0 - required by chain code
        mov         loc2=EXCPT_EXTERNAL_INTERRUPT;;
        cmp.eq      p7,p0=SCRATCH_REG1,loc2;;   // check to see if this is the timer tick
  (p7)  br.cond.dpnt.few    DO_CHAIN;;

NO_CHAIN:
        mov         pr=PR_REG;;
        rfi;;                                   // we're outa here.

DO_CHAIN:
        mov         pr=PR_REG
        mov         SCRATCH_REG1=cr.iva
        mov         SCRATCH_REG2=PATCH_RETURN_OFFSET;;
        add         SCRATCH_REG1=SCRATCH_REG1, SCRATCH_REG2;;
        mov         b0=SCRATCH_REG1;;
        br.cond.sptk.few  b0;;

EndHookHandler:


/////////////////////////////////////////////
//
//  Name:
//      HookStub
//
//  Description:
//      HookStub will be copied from it's loaded location into the IVT when
//      an IVT entry is hooked.  The IVT entry does an indirect jump via B0 to
//      HookHandler, which in turn calls into the default C handler, which calls
//      the user-installed C handler.  The calls return and HookHandler executes
//      an rfi.
//
//  Notes:
//      Saves B0 to B0_REG
//      Saves IVT index to SCRATCH_REG1 (immediate value is fixed up when code is copied
//          to the IVT entry.

        ASM_GLOBAL HookStub
        .proc   HookStub
HookStub:

        mov         B0_REG=b0
        movl        SCRATCH_REG1=HookHandler;;
        mov         b0=SCRATCH_REG1;;
        mov         SCRATCH_REG1=0;;// immediate value is fixed up during install of handler to be the vector number
        br.cond.sptk.few b0

        .endp       HookStub


/////////////////////////////////////////////
// The following code is moved into IVT entry 14 (offset 3400) which is reserved
// in the Itanium architecture.  The patch code is located at the end of the
// IVT entry.

PatchCode:
        mov       SCRATCH_REG0=psr
        mov       SCRATCH_REG6=cr.ipsr
        mov       PR_REG=pr
        mov       B0_REG=b0;;

        // turn off any virtual translations
        movl      SCRATCH_REG1 = ~( MASK(PSR_DT,1) | MASK(PSR_RT,1));;
        and       SCRATCH_REG1 = SCRATCH_REG0, SCRATCH_REG1;;
        mov       psr.l = SCRATCH_REG1;;
        srlz.d
        tbit.z    p14, p15 = SCRATCH_REG6, PSR_IS;;   // Check to see if we were
                                                      // interrupted from IA32
                                                      // context.  If so, bail out
                                                      // and chain to SAL immediately
 (p15)  br.cond.sptk.few Stub_IVT_Passthru;;
        // we only want to take 1 out of 32 external interrupts to minimize the
        // impact to system performance.  Check our interrupt count and bail
        // out if we're not up to 32
        movl      SCRATCH_REG1=ExternalInterruptCount;;
        ld8       SCRATCH_REG2=[SCRATCH_REG1];;       // ExternalInterruptCount
        tbit.z    p14, p15 = SCRATCH_REG2, 5;;        // bit 5 set?
 (p14)  add       SCRATCH_REG2=1, SCRATCH_REG2;;      // No?  Then increment
                                                      // ExternalInterruptCount
                                                      // and Chain to SAL
                                                      // immediately
 (p14)  st8       [SCRATCH_REG1]=SCRATCH_REG2;;
 (p14)  br.cond.sptk.few Stub_IVT_Passthru;;
 (p15)  mov       SCRATCH_REG2=0;;                    // Yes?  Then reset
                                                        // ExternalInterruptCount
                                                        // and branch to
                                                        // HookHandler
 (p15)  st8       [SCRATCH_REG1]=SCRATCH_REG2;;
        mov       pr=PR_REG
        movl      SCRATCH_REG1=HookHandler;;          // SCRATCH_REG1 = entrypoint of HookHandler
        mov       b0=SCRATCH_REG1;;                   // b0 = entrypoint of HookHandler
        mov       SCRATCH_REG1=EXCPT_EXTERNAL_INTERRUPT;;
        br.sptk.few b0;;                                // branch to HookHandler

PatchCodeRet:
        // fake-up an rfi to get RSE back to being coherent and insure psr has
        // original contents when interrupt occured, then exit to SAL
        // at this point:
        //      cr.ifs has been modified by previous "cover"
        //      SCRATCH_REG6 has original cr.ifs

        mov       SCRATCH_REG5=cr.ipsr
        mov       SCRATCH_REG4=cr.iip;;
        mov       cr.ipsr=SCRATCH_REG0
        mov       SCRATCH_REG1=ip;;
        add       SCRATCH_REG1=0x30, SCRATCH_REG1;;
        mov       cr.iip=SCRATCH_REG1;;
        rfi;;                       // rfi to next instruction

Stub_RfiTarget:
        mov       cr.ifs=SCRATCH_REG6
        mov       cr.ipsr=SCRATCH_REG5
        mov       cr.iip=SCRATCH_REG4;;

Stub_IVT_Passthru:
        mov       pr=PR_REG                         // pr = saved predicate registers
        mov       b0=B0_REG;;                       // b0 = saved b0
EndPatchCode:


/////////////////////////////////////////////
// The following bundle is moved into IVT entry 14 (offset 0x3400) which is reserved
// in the Itanium architecture.  This bundle will be the last bundle and will
// be located at offset 0x37F0 in the IVT.

FailsafeBranch:
{
        .mib
        nop.m     0
        nop.i     0
        br.sptk.few -(FAILSAFE_BRANCH_OFFSET - EXT_INT_ENTRY_OFFSET - 0x10)
}


/////////////////////////////////////////////
// The following bundle is moved into IVT entry 13 (offset 0x3000) which is the
// external interrupt.  It branches to the patch code.

PatchCodeNewBun0:
{
        .mib
        nop.m     0
        nop.i     0
        br.cond.sptk.few PATCH_BRANCH
}
