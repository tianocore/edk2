//// @file
//
// Copyright (c) 1999 - 2008, Intel Corporation. All rights reserved.<BR>
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions
// of the BSD License which accompanies this distribution.  The
// full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
////

.file  "IpfThunk.s"

#include  "IpfMacro.i"
#include  "Ipf/IpfThunk.i"

.align	0x10
//-----------------------------------------------------------------------------
//++
// EfiIaEntryPoint
//
// Register physical address of Esal Data Area
//
// On Entry :
//  in1 = ptr to legacy bios reg
//  in2 = ptr to Call Stack
//  in3 = Call Stack Size
//
// Return Value:
//  r8 = SAL_SUCCESS
//
// As per static calling conventions.
//
//--
//---------------------------------------------------------------------------
PROCEDURE_ENTRY(EfiIaEntryPoint)

	        alloc	      loc0 = 8,10,8,0;;

	        mov	        out0 = r0;;
          mov	        out1 = r0;;
          mov	        out2 = r0;;
          mov	        out3 = r0;;
	        mov	        out4 = r0;;
          mov	        out5 = r0;;
          mov	        out6 = r0;;
          mov	        out7 = r0;;

	        mov	        loc1 = b0;;						        // save efi (b0)
	        mov	        loc2 = psr;;					        // save efi (PSR)
	        mov	        loc3 = gp;;						        // save efi (GP)
	        mov	        loc4 = pr;;						        // save efi (PR)
	        mov	        loc5 = sp;;						        // save efi (SP)
	        mov	        loc6 = r13;;					        // save efi (TP)
	        mov	        loc7 = ar.lc;;				        // save efi (LC)
	        mov	        loc8 = ar.fpsr;;			        // save efi (FPSR)

	        mov	        r8   = r0;; 						      // return status
	        mov	        r9   = r0;; 						      // return value
	        mov	        r10  = r0;;						        // return value
	        mov	        r11  = r0;;						        // return value

bios_int_func::
	        rsm		      0x4000;;					            // i(14)=0, disable interrupt
	        srlz.d;;
	        srlz.i;;

//---------------------//
// save fp registers   //
//---------------------//

	        dep		      sp = 0,sp,0,4;;					      // align 16
	        add		      sp = -16,sp;;						      // post decrement

int_ip_1x::
	        mov		      r2 = ip;;
	        add		      r2 = (int_ip_1y - int_ip_1x),r2;;
	        mov		      b7 = r2;;
	        br		      save_fp_registers;;

int_ip_1y::
	        add		      sp    = 16,sp;;               // adjust (SP)
	        mov		      loc9  = sp;;						      // save (SP)
	        adds	      sp    = 0x10,in1;;				    // in1 + 0x10 = SP
	        ld4		      sp    = [sp];;						    // SP
	        adds	      r17   = 0x32,in1;;			      // in1 + 0x32 = SS
	        ld2		      r17   = [r17];;					      // SS
	        movl	      r2    = 0xffffffff;;			    // if no SS:SP, then define new SS:SP
	        cmp.ne	    p6,p0 = sp,r2;;
	        movl	      r2    = 0xffff;;
	        cmp.ne.or   p6,p0 = r17,r2;;
     (p6) br.sptk	bif_1;;

	        mov		      sp    = in3;;						      // 16-bit stack pointer
	        mov		      r2    = psr;;
	        tbit.z	    p6,p7 = r2,17;;			          // psr.dt (Physical OR Virtual)

bif_ip1x::
	        mov		      r2    = in2;;						      // ia32 callback stack top
	        mov		      r3    = in3;;						      // 16-bit stack pointer
	        sub		      r2    = r2,r3;;
	        shr.u	      r17   = r2,4;;					      // 16-bit stack segment

bif_1::
	        extr.u	    sp    = sp,0,16;;			        // SP (16-bit sp for legacy code)
	        dep		      sp    = 0,sp,0,3;;		        // align 8
	        cmp.eq	    p6,p0 = 0,sp;;                // if SP=0000 then wrap to 0x10000
    (p6)	dep	        sp    = -1,sp,16,1;;
	        shladd	    r2    = r17,4,sp;;			      // ESP = SS<<4+SP
	        add		      r2    = -8,r2;;					      // post decrement 64 bit pointer
	        add		      sp    = -8,sp;;					      // post decrement SP

sale_ip1x::
	        mov		      r18   = ip;;
	        adds	      r18   = (sale_ip1y - sale_ip1x),r18;;
	        sub		      r18   = r18,r2;;				      // return address - CS base
	        add		      r18   = r18,sp;;				      // adjustment for stack
	        shl		      r18   = r18,32;;
	        movl	      r19   = 0xb80f66fa;;		      // CLI, JMPE xxxxxxxx
	        or		      r18   = r18,r19;;
	        st8		      [r2]  = r18;;					        // (FA,66,0F,B8,xx,xx,xx,xx)

	        cmp.eq	    p6,p0 = 0,sp;;			          // if SP=0000 then wrap to 0x10000
    (p6)	dep	        sp    = -1,sp,16,1;;
	        shladd	    r2    = r17,4,sp;;			      // ESP=SS<<4+SP
	        add		      r2    = -2,r2;;					      // post decrement 64 bit pointer
	        add		      sp    = -2,sp;;					      // post decrement SP

	        movl	      r18   = 0x8000000000000100;;	// CALL FAR function
	        cmp.eq	    p6,p7 = in0,r18;;
    (p6)	add	        r19   = 0x28,in1;;			      // in1 + 0x28 = CS
    (p6)	ld2	        r18   = [r19],-4;;			      // CS
    (p6)	st2	        [r2]  = r18,-2;;				      // in1 + 0x24 = EIP
    (p6)	ld2	        r18   = [r19];;					      // EIP
    (p6)	st2	        [r2]  = r18,-2;;				      //
    (p6)	movl	      r18   = 0x9a90;;			        // nop, CALLFAR xxxx:yyyy

    (p7)	movl	      r18   = 0xcd;;				        // INT xx
    (p7)	dep	        r18   = in0,r18,8,8;;
    	    st2	        [r2]  = r18;;							    // (CD,xx)

	        mov	        r18   = r2;;	                // EIP for legacy execution

//------------------------------//
// flush 32 bytes legacy code	  //
//------------------------------//

	        dep		      r2    = 0,r2,0,5;;            // align to 32
	        fc		      r2;;
	        sync.i;;
	        srlz.i;;
	        srlz.d;;

//------------------------------//
// load legacy registers		    //
//------------------------------//
	        mov		      r2    = in1;;						      // IA32 BIOS register state
	        ld4		      r8    = [r2],4;;						  // in1 + 0 = EAX
	        ld4		      r9    = [r2],4;;						  // in1 + 4 = ECX
	        ld4		      r10   = [r2],4;;					    // in1 + 8 = EDX
	        ld4		      r11   = [r2],4;;					    // in1 + 12 = EBX

	        add		      r2    = 4,r2;;						    // in1 + 16 = ESP (skip)

	        ld4		      r13   = [r2],4;;					    // in1 + 20 = EBP
	        ld4		      r14   = [r2],4;;					    // in1 + 24 = ESI
	        ld4		      r15   = [r2],4;;					    // in1 + 28 = EDI
	        ld4		      r3    = [r2],4;;						  // in1 + 32 = EFLAGS
	        mov		      ar.eflag = r3;;

	        add		      r2    = 4,r2;;						    // in1 + 36 = EIP (skip)
	        add		      r2    = 2,r2;;						    // in1 + 40 = CS (skip)

	        ld2		      r16   = [r2],2;;					    // in1 + 42 = DS, (r16 = GS,FS,ES,DS)
	        movl	      r27   = 0xc93fffff00000000;;
	        dep		      r27   = r16,r27,4,16;;				// r27 = DSD

	        ld2		      r19   = [r2],2;;					    // in1 + 44 = ES
	        dep		      r16   = r19,r16,16,16;;
	        movl	      r24   = 0xc93fffff00000000;;
	        dep		      r24   = r19,r24,4,16;;				// r24 = ESD

	        ld2		      r19   = [r2],2;;					    // in1 + 46 = FS
	        dep		      r16   = r19,r16,32,16;;
	        movl	      r28   = 0xc93fffff00000000;;
	        dep		      r28   = r19,r28,4,16;;				// r28 = FSD

	        ld2		      r19   = [r2],2;;					    // in1 + 48 = GS
	        dep		      r16   = r19,r16,48,16;;
	        movl	      r29   = 0xc93fffff00000000;;
	        dep		      r29   = r19,r29,4,16;;				// r29 = GSD

	        mov		      r30   = r0;;						      // r30 = LDTD, clear NaT
	        mov		      r31   = r0;;						      // r31 = GDTD, clear NaT

	        dep		      r17   = r17,r17,16,16;;			  // CS = SS, (r17 = TSS,LDT,SS,CS)

	        movl	      r3    = 0x0930ffff00000000;;
	        dep		      r3    = r17,r3,4,16;;
	        mov		      ar.csd = r3;;						      // ar25 = CSD
	        mov		      ar.ssd = r3;;						      // ar26 = SSD

//------------------------------//
// give control to INT function //
//------------------------------//

	        br.call.sptk  b0  = execute_int_function;;

//------------------------------//
// store legacy registers		    //
//------------------------------//

	        mov		      r2    = in1;;
	        st4		      [r2]  = r8,4;;						    // EAX
	        st4		      [r2]  = r9,4;;						    // ECX
	        st4		      [r2]  = r10,4;;					      // EDX
	        st4		      [r2]  = r11,4;;					      // EBX

	        add		      r2    = 4,r2;;						    // ESP (skip)

	        st4		      [r2]  = r13,4;;					      // EBP
	        st4		      [r2]  = r14,4;;					      // ESI
	        st4		      [r2]  = r15,4;;					      // EDI

	        mov		      r3    = ar.eflag;;
	        st4		      [r2]  = r3,4;;						    // EFLAGS

	        add		      r2    = 4,r2;;						    // EIP (skip)
	        add		      r2    = 2,r2;;						    // CS (skip)

	        st2		      [r2]  = r16,2;;					      // DS, (r16 = GS,FS,ES,DS)

	        extr.u	    r3    = r16,16,16;;
	        st2		      [r2]  = r3,2;;						    // ES

	        extr.u	    r3    = r16,32,16;;
	        st2		      [r2]  = r3,2;;						    // FS

	        extr.u	    r3    = r16,48,16;;
	        st2		      [r2]  = r3,2;;						    // GS

//------------------------------//
// restore fp registers 		    //
//------------------------------//
        	mov		      sp    = loc9;;						    // restore (SP)
int_ip_2x::
	        mov		      r2    = ip;;
	        add		      r2    = (int_ip_2y - int_ip_2x),r2;;
	        mov		      b7    = r2;;
	        br		      restore_fp_registers;;

int_ip_2y::
	        mov		      r8    = r0;; 						      // return status
	        mov		      r9    = r0;; 						      // return value
	        mov		      r10   = r0;;						      // return value
	        mov		      r11   = r0;;						      // return value

	        mov		      ar.fpsr = loc8;;			        // restore efi (FPSR)
	        mov		      ar.lc = loc7;;				        // restore efi (LC)
	        mov		      r13   = loc6;;					      // restore efi (TP)
	        mov		      sp    = loc5;;						    // restore efi (SP)
	        mov		      pr    = loc4;;						    // restore efi (PR)
	        mov		      gp    = loc3;;						    // restore efi (GP)
	        mov		      psr.l = loc2;;				        // restore efi (PSR)
	        srlz.d;;
	        srlz.i;;
	        mov		      b0    = loc1;;						    // restore efi (b0)
	        mov		      ar.pfs = loc0;;
	br.ret.sptk	b0;;					                        // return to efi

PROCEDURE_EXIT (EfiIaEntryPoint)

//==============================//
//	EXECUTE_INT_FUNCTION		    //
//==============================//
// switch to virtual address	  //
//------------------------------//

execute_int_function::

	        alloc	      r2 = 0,0,0,0;;				        // cfm.sof=0
	        flushrs;;

	        rsm		      0x2000;;						          // ic(13)=0 for control register programming
	        srlz.d;;
	        srlz.i;;

	        mov		      r2  = psr;;
	        dep		      r2  = -1,r2,34,1;;			      // set is(34)
	        dep		      r2  = -1,r2,44,1;;			      // set bn(44)
	        dep		      r2  = -1,r2,36,1;;			      // set it(36)
	        dep		      r2  = -1,r2,27,1;;			      // set rt(27)
	        dep		      r2  = -1,r2,17,1;;			      // set dt(17)
	        dep		      r2  = 0,r2,3,1;;				      // reset ac(3)
	        dep		      r2  = -1,r2,13,1;;			      // set ic(13)

	        mov		      cr.ipsr = r2;;
	        mov		      cr.ifs  = r0;;					      // clear interruption function state register
	        mov		      cr.iip  = r18;;

	        rfi;;									                    // go to legacy code execution

//------------------------------//
// back from legacy code		    //
//------------------------------//
// switch to physical address	  //
//------------------------------//

sale_ip1y::
	        rsm		    0x6000;;						            // i(14)=0,ic(13)=0 for control reg programming
	        srlz.d;;
	        srlz.i;;

	        mov		    r2  = psr;;
	        dep		    r2  = -1,r2,44,1;;					      // set bn(44)
	        dep		    r2  = 0,r2,36,1;;					      // reset it(36)
	        dep		    r2  = 0,r2,27,1;;					      // reset rt(27)
	        dep		    r2  = 0,r2,17,1;;					      // reset dt(17)
	        dep		    r2  = -1,r2,13,1;;					      // set ic(13)
	        mov		    cr.ipsr = r2;;

sale_ip2x::
	        mov		    r2  = ip;;
	        add		    r2  = (sale_ip2y - sale_ip2x),r2;;
	        mov		    cr.ifs = r0;;						        // clear interruption function state register
	        mov		    cr.iip = r2;;
	        rfi;;

sale_ip2y::
	        br.ret.sptk	b0;;				                  // return to SAL

//------------------------------//
// store fp registers		        //
//------------------------------//
save_fp_registers::
	stf.spill [sp]=f2,-16;;  stf.spill [sp]=f3,-16;;
	stf.spill [sp]=f4,-16;;  stf.spill [sp]=f5,-16;;  stf.spill [sp]=f6,-16;;  stf.spill [sp]=f7,-16;;
	stf.spill [sp]=f8,-16;;  stf.spill [sp]=f9,-16;;  stf.spill [sp]=f10,-16;; stf.spill [sp]=f11,-16;;
	stf.spill [sp]=f12,-16;; stf.spill [sp]=f13,-16;; stf.spill [sp]=f14,-16;; stf.spill [sp]=f15,-16;;
	stf.spill [sp]=f16,-16;; stf.spill [sp]=f17,-16;; stf.spill [sp]=f18,-16;; stf.spill [sp]=f19,-16;;
	stf.spill [sp]=f20,-16;; stf.spill [sp]=f21,-16;; stf.spill [sp]=f22,-16;; stf.spill [sp]=f23,-16;;
	stf.spill [sp]=f24,-16;; stf.spill [sp]=f25,-16;; stf.spill [sp]=f26,-16;; stf.spill [sp]=f27,-16;;
	stf.spill [sp]=f28,-16;; stf.spill [sp]=f29,-16;; stf.spill [sp]=f30,-16;; stf.spill [sp]=f31,-16;;
	stf.spill [sp]=f32,-16;; stf.spill [sp]=f33,-16;; stf.spill [sp]=f34,-16;; stf.spill [sp]=f35,-16;;
	stf.spill [sp]=f36,-16;; stf.spill [sp]=f37,-16;; stf.spill [sp]=f38,-16;; stf.spill [sp]=f39,-16;;
	stf.spill [sp]=f40,-16;; stf.spill [sp]=f41,-16;; stf.spill [sp]=f42,-16;; stf.spill [sp]=f43,-16;;
	stf.spill [sp]=f44,-16;; stf.spill [sp]=f45,-16;; stf.spill [sp]=f46,-16;; stf.spill [sp]=f47,-16;;
	stf.spill [sp]=f48,-16;; stf.spill [sp]=f49,-16;; stf.spill [sp]=f50,-16;; stf.spill [sp]=f51,-16;;
	stf.spill [sp]=f52,-16;; stf.spill [sp]=f53,-16;; stf.spill [sp]=f54,-16;; stf.spill [sp]=f55,-16;;
	stf.spill [sp]=f56,-16;; stf.spill [sp]=f57,-16;; stf.spill [sp]=f58,-16;; stf.spill [sp]=f59,-16;;
	stf.spill [sp]=f60,-16;; stf.spill [sp]=f61,-16;; stf.spill [sp]=f62,-16;; stf.spill [sp]=f63,-16;;
	stf.spill [sp]=f64,-16;; stf.spill [sp]=f65,-16;; stf.spill [sp]=f66,-16;; stf.spill [sp]=f67,-16;;
	stf.spill [sp]=f68,-16;; stf.spill [sp]=f69,-16;; stf.spill [sp]=f70,-16;; stf.spill [sp]=f71,-16;;
	stf.spill [sp]=f72,-16;; stf.spill [sp]=f73,-16;; stf.spill [sp]=f74,-16;; stf.spill [sp]=f75,-16;;
	stf.spill [sp]=f76,-16;; stf.spill [sp]=f77,-16;; stf.spill [sp]=f78,-16;; stf.spill [sp]=f79,-16;;
	stf.spill [sp]=f80,-16;; stf.spill [sp]=f81,-16;; stf.spill [sp]=f82,-16;; stf.spill [sp]=f83,-16;;
	stf.spill [sp]=f84,-16;; stf.spill [sp]=f85,-16;; stf.spill [sp]=f86,-16;; stf.spill [sp]=f87,-16;;
	stf.spill [sp]=f88,-16;; stf.spill [sp]=f89,-16;; stf.spill [sp]=f90,-16;; stf.spill [sp]=f91,-16;;
	stf.spill [sp]=f92,-16;; stf.spill [sp]=f93,-16;; stf.spill [sp]=f94,-16;; stf.spill [sp]=f95,-16;;
	stf.spill [sp]=f96,-16;; stf.spill [sp]=f97,-16;; stf.spill [sp]=f98,-16;; stf.spill [sp]=f99,-16;;
	stf.spill [sp]=f100,-16;;stf.spill [sp]=f101,-16;;stf.spill [sp]=f102,-16;;stf.spill [sp]=f103,-16;;
	stf.spill [sp]=f104,-16;;stf.spill [sp]=f105,-16;;stf.spill [sp]=f106,-16;;stf.spill [sp]=f107,-16;;
	stf.spill [sp]=f108,-16;;stf.spill [sp]=f109,-16;;stf.spill [sp]=f110,-16;;stf.spill [sp]=f111,-16;;
	stf.spill [sp]=f112,-16;;stf.spill [sp]=f113,-16;;stf.spill [sp]=f114,-16;;stf.spill [sp]=f115,-16;;
	stf.spill [sp]=f116,-16;;stf.spill [sp]=f117,-16;;stf.spill [sp]=f118,-16;;stf.spill [sp]=f119,-16;;
	stf.spill [sp]=f120,-16;;stf.spill [sp]=f121,-16;;stf.spill [sp]=f122,-16;;stf.spill [sp]=f123,-16;;
	stf.spill [sp]=f124,-16;;stf.spill [sp]=f125,-16;;stf.spill [sp]=f126,-16;;stf.spill [sp]=f127,-16;;
	invala;;
	br	b7;;

//------------------------------//
// restore fp registers	        //
//------------------------------//
restore_fp_registers::
	ldf.fill f127=[sp],16;;ldf.fill f126=[sp],16;;ldf.fill f125=[sp],16;;ldf.fill f124=[sp],16;;
	ldf.fill f123=[sp],16;;ldf.fill f122=[sp],16;;ldf.fill f121=[sp],16;;ldf.fill f120=[sp],16;;
	ldf.fill f119=[sp],16;;ldf.fill f118=[sp],16;;ldf.fill f117=[sp],16;;ldf.fill f116=[sp],16;;
	ldf.fill f115=[sp],16;;ldf.fill f114=[sp],16;;ldf.fill f113=[sp],16;;ldf.fill f112=[sp],16;;
	ldf.fill f111=[sp],16;;ldf.fill f110=[sp],16;;ldf.fill f109=[sp],16;;ldf.fill f108=[sp],16;;
	ldf.fill f107=[sp],16;;ldf.fill f106=[sp],16;;ldf.fill f105=[sp],16;;ldf.fill f104=[sp],16;;
	ldf.fill f103=[sp],16;;ldf.fill f102=[sp],16;;ldf.fill f101=[sp],16;;ldf.fill f100=[sp],16;;
	ldf.fill f99=[sp],16;; ldf.fill f98=[sp],16;; ldf.fill f97=[sp],16;; ldf.fill f96=[sp],16;;
	ldf.fill f95=[sp],16;; ldf.fill f94=[sp],16;; ldf.fill f93=[sp],16;; ldf.fill f92=[sp],16;;
	ldf.fill f91=[sp],16;; ldf.fill f90=[sp],16;; ldf.fill f89=[sp],16;; ldf.fill f88=[sp],16;;
	ldf.fill f87=[sp],16;; ldf.fill f86=[sp],16;; ldf.fill f85=[sp],16;; ldf.fill f84=[sp],16;;
	ldf.fill f83=[sp],16;; ldf.fill f82=[sp],16;; ldf.fill f81=[sp],16;; ldf.fill f80=[sp],16;;
	ldf.fill f79=[sp],16;; ldf.fill f78=[sp],16;; ldf.fill f77=[sp],16;; ldf.fill f76=[sp],16;;
	ldf.fill f75=[sp],16;; ldf.fill f74=[sp],16;; ldf.fill f73=[sp],16;; ldf.fill f72=[sp],16;;
	ldf.fill f71=[sp],16;; ldf.fill f70=[sp],16;; ldf.fill f69=[sp],16;; ldf.fill f68=[sp],16;;
	ldf.fill f67=[sp],16;; ldf.fill f66=[sp],16;; ldf.fill f65=[sp],16;; ldf.fill f64=[sp],16;;
	ldf.fill f63=[sp],16;; ldf.fill f62=[sp],16;; ldf.fill f61=[sp],16;; ldf.fill f60=[sp],16;;
	ldf.fill f59=[sp],16;; ldf.fill f58=[sp],16;; ldf.fill f57=[sp],16;; ldf.fill f56=[sp],16;;
	ldf.fill f55=[sp],16;; ldf.fill f54=[sp],16;; ldf.fill f53=[sp],16;; ldf.fill f52=[sp],16;;
	ldf.fill f51=[sp],16;; ldf.fill f50=[sp],16;; ldf.fill f49=[sp],16;; ldf.fill f48=[sp],16;;
	ldf.fill f47=[sp],16;; ldf.fill f46=[sp],16;; ldf.fill f45=[sp],16;; ldf.fill f44=[sp],16;;
	ldf.fill f43=[sp],16;; ldf.fill f42=[sp],16;; ldf.fill f41=[sp],16;; ldf.fill f40=[sp],16;;
	ldf.fill f39=[sp],16;; ldf.fill f38=[sp],16;; ldf.fill f37=[sp],16;; ldf.fill f36=[sp],16;;
	ldf.fill f35=[sp],16;; ldf.fill f34=[sp],16;; ldf.fill f33=[sp],16;; ldf.fill f32=[sp],16;;
	ldf.fill f31=[sp],16;; ldf.fill f30=[sp],16;; ldf.fill f29=[sp],16;; ldf.fill f28=[sp],16;;
	ldf.fill f27=[sp],16;; ldf.fill f26=[sp],16;; ldf.fill f25=[sp],16;; ldf.fill f24=[sp],16;;
	ldf.fill f23=[sp],16;; ldf.fill f22=[sp],16;; ldf.fill f21=[sp],16;; ldf.fill f20=[sp],16;;
	ldf.fill f19=[sp],16;; ldf.fill f18=[sp],16;; ldf.fill f17=[sp],16;; ldf.fill f16=[sp],16;;
	ldf.fill f15=[sp],16;; ldf.fill f14=[sp],16;; ldf.fill f13=[sp],16;; ldf.fill f12=[sp],16;;
	ldf.fill f11=[sp],16;; ldf.fill f10=[sp],16;; ldf.fill f9=[sp],16;;  ldf.fill f8=[sp],16;;
	ldf.fill f7=[sp],16;;  ldf.fill f6=[sp],16;;  ldf.fill f5=[sp],16;;  ldf.fill f4=[sp],16;;
	ldf.fill f3=[sp],16;;  ldf.fill f2=[sp],16;;
	invala;;
	br	b7;;

//-----------------------------------------------------------------------------
//++
// EsalSetSalDataArea
//
// Register physical address of Esal Data Area
//
// On Entry :
//  in0 = Reverse Thunk Address
//  in1 = IntThunk Address
//
// Return Value:
//  r8 = SAL_SUCCESS
//
// As per static calling conventions.
//
//--
//---------------------------------------------------------------------------

PROCEDURE_ENTRY (EsalSetSalDataArea)

      NESTED_SETUP (4,8,0,0)

EsalCalcStart1_3::
      mov   r8   = ip;;
      add   r8   = (ReverseThunkAddress - EsalCalcStart1_3), r8;;
      st8   [r8] = in0;;

EsalCalcStart1_4::
      mov   r8   = ip;;
      add   r8   = (IntThunkAddress - EsalCalcStart1_4), r8;;
      st8   [r8] = in1;;

      mov   r8   = r0;;

      NESTED_RETURN

PROCEDURE_EXIT (EsalSetSalDataArea)

//-----------------------------------------------------------------------------
//++
// EsagGetReverseThunkAddress
//
// Register physical address of Esal Data Area
//
// On Entry :
//  out0 = CodeStart
//  out1 = CodeEnd
//  out1 = ReverseThunkCode
//
// Return Value:
//  r8 = SAL_SUCCESS
//
// As per static calling conventions.
//
//--
//---------------------------------------------------------------------------

PROCEDURE_ENTRY (EsalGetReverseThunkAddress)

          NESTED_SETUP (4,8,0,0)

EsalCalcStart1_31::
          mov       r8 = ip;;
          add       r8 = (Ia32CodeStart - EsalCalcStart1_31), r8;;
          mov       r9 = r8;;

EsalCalcStart1_41::
          mov       r8  = ip;;
          add       r8  = (Ia32CodeEnd - EsalCalcStart1_41), r8;;
          mov       r10 = r8;;

EsalCalcStart1_51::
          mov       r8  = ip;;
          add       r8  = (ReverseThunkAddress - EsalCalcStart1_51), r8;;
          mov       r11 = r8;;
          mov       r8  = r0;;

          NESTED_RETURN

PROCEDURE_EXIT (EsalGetReverseThunkAddress)


.align 16
PROCEDURE_ENTRY (InterruptRedirectionTemplate)
          data8	    0x90CFCD08
          data8	    0x90CFCD09
          data8	    0x90CFCD0A
          data8	    0x90CFCD0B
          data8	    0x90CFCD0C
          data8	    0x90CFCD0D
          data8	    0x90CFCD0E
          data8	    0x90CFCD0F
PROCEDURE_EXIT (InterruptRedirectionTemplate)

//------------------------------//
// Reverse Thunk Code           //
//------------------------------//

Ia32CodeStart::
          br.sptk.few Ia32CodeStart;;             // IPF CSM integration -Bug (Write This Code)
ReverseThunkCode::
	        data8	      0xb80f66fa			            // CLI, JMPE xxxx
ReverseThunkAddress::
	        data8	      0					                  // Return Address
IntThunkAddress::
	        data8	      0					                  // IntThunk Address
Ia32CodeEnd::




