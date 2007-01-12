//++
// Copyright (c) 2006, Intel Corporation
// All rights reserved. This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//  Module Name:
//
//    IpfCpuCore.s
//
//  Abstract:
//    IPF Specific assembly routines
//
//--

.file  "IpfCpuCore.s"

#include  "IpfMacro.i"
#include  "Ipf/IpfCpuCore.i"

//----------------------------------------------------------------------------------
// This module supports terminating CAR (Cache As RAM) stage. It copies all the
// CAR data into real RAM and then makes a stack switch.

// EFI_STATUS
// SwitchCoreStacks (
//   IN VOID  *EntryPoint,
//   IN UINTN CopySize,
//   IN VOID  *OldBase,
//   IN VOID  *NewBase
//   IN UINTN NewSP, OPTIONAL
//   IN UINTN NewBSP OPTIONAL
//   )
// EFI_STATUS
// SwitchCoreStacks (
//   IN VOID  *EntryPointForContinuationFunction,
//   IN UINTN StartupDescriptor,
//   IN VOID  PEICorePointer,
//   IN UINTN NewSP
//   )
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (SwitchCoreStacks)

        NESTED_SETUP (4,2,0,0)

        // first save all stack registers in GPRs.
        mov     r13 = in0;;             // this is a pointer to the PLABEL of the continuation function.
        ld8     r16 = [r13],8;;         // r16 = address of continuation function from the PLABEL
        ld8     gp = [r13];;            // gp  = gp of continuation function from the PLABEL
        mov     b1 = r16;;

        // save the parameters in r5, r6. these 2 seemed to be preserved across PAL calls
        mov     r5 = in1;;              // this is the parameter1 to pass to the continuation function
        mov     r6 = in2;;              // this is the parameter2 to pass to the continuation function
        dep     r6=0,r6,63,1;;          // zero the bit 63.

        mov     r8 = in3;;              // new stack pointer.

        // r8 has the sp, this is 128K stack size, from this we will reserve 16K for the bspstore
        movl    r15 = PEI_BSP_STORE_SIZE;;
        sub     r8 = r8, r15;;
        add     r15 = (GuardBand),r8;;  // some little buffer, now r15 will be our bspstore

        // save the bspstore value to r4, save sp value to r7
        mov     r4  = r15
        mov     r7  = r8
        mov     r16 = r8;;              // will be the new sp in uncache mode


        alloc   r11=0,0,0,0;;           // Set 0-size frame
        flushrs;;

        mov     r21 = RSC_KERNEL_DISABLED;; // for rse disable
        mov     ar.rsc = r21;;          // turn off RSE

        add     sp = r0, r16            // transfer to the EFI stack
        mov     ar.bspstore = r15       // switch to EFI BSP
        invala                          // change of ar.bspstore needs invala.

        mov     r19 = RSC_KERNEL_LAZ;;  // RSC enabled, Lazy mode
        mov     ar.rsc = r19;;          // turn rse on, in kernel mode

//-----------------------------------------------------------------------------------
// Save here the meaningful stuff for next few lines and then make the PAL call.
// Make PAL call to terminate the CAR status.
        // AVL: do this only for recovery check call...

        mov     r28=ar.k3;;
        dep     r2 = r28,r0,0,8;;       // Extract Function bits from GR20.
        cmp.eq  p6,p7 = RecoveryFn,r2;; // Is it Recovery check
        (p7)  br.sptk.few DoneCARTermination; // if not, don't terminate car..

TerminateCAR::

        mov     r28 = ip;;
        add     r28 = (DoneCARTerminationPALCall - TerminateCAR),r28;;
        mov     b0 = r28

        mov     r8 = ar.k5;;
        mov     b6 = r8
        mov     r28 = 0x208

        mov     r29 = r0
        mov     r30 = r0
        mov     r31 = r0
        mov     r8 = r0;;
        br.sptk.few b6;;                // Call PAL-A call.

DoneCARTerminationPALCall::

// don't check error in soft sdv, it is always returning -1 for this call for some reason
#if SOFT_SDV
#else
ReturnToPEIMain::
        cmp.eq  p6,p7 = r8,r0;;
        //
        // dead loop if the PAL call failed, we have the CAR on but the stack is now pointing to memory
        //
        (p7) br.sptk.few ReturnToPEIMain;;
        //
        // PAL call successed,now the stack are in memory so come into cache mode
        // instead of uncache mode
        //

        alloc   r11=0,0,0,0;;           // Set 0-size frame
        flushrs;;

        mov     r21 = RSC_KERNEL_DISABLED;; // for rse disable
        mov     ar.rsc = r21;;          // turn off RSE

        dep     r6 = 0,r6,63,1          // zero the bit 63
        dep     r7 = 0,r7,63,1          // zero the bit 63
        dep     r4 = 0,r4,63,1;;        // zero the bit 63
        add     sp = r0, r7             // transfer to the EFI stack in cache mode
        mov     ar.bspstore = r4        // switch to EFI BSP
        invala                          // change of ar.bspstore needs invala.

        mov     r19 = RSC_KERNEL_LAZ;;  // RSC enabled, Lazy mode
        mov     ar.rsc = r19;;          // turn rse on, in kernel mode

#endif

DoneCARTermination::

        // allocate a stack frame:
        alloc   r11=0,2,2,0 ;;          // alloc  outs going to ensuing DXE IPL service
                                                                                                // on the new stack
        mov     out0 = r5;;
        mov     out1 = r6;;

        mov     r16 = b1;;
        mov     b6 = r16;;
        br.call.sptk.few b0=b6;;        // Call the continuation function

        NESTED_RETURN

PROCEDURE_EXIT(SwitchCoreStacks)
//-----------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
//++
// GetHandOffStatus
//
// This routine is called by all processors simultaneously, to get some hand-off
// status that has been captured by IPF dispatcher and recorded in kernel registers.
//
// Arguments :
//
// On Entry :  None.
//
// Return Value: Lid, R20Status.
//
//--
//----------------------------------------------------------------------------------
PROCEDURE_ENTRY (GetHandOffStatus)

        NESTED_SETUP (0,2+0,0,0)

        mov     r8 = ar.k6              // Health Status (Self test params)
        mov     r9 = ar.k4              // LID bits
        mov     r10 = ar.k3;;           // SAL_E entry state
        mov     r11 = ar.k7             // Return address to PAL

        NESTED_RETURN
PROCEDURE_EXIT (GetHandOffStatus)
//----------------------------------------------------------------------------------


