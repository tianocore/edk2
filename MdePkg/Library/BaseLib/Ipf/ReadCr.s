/// @file
///  IPF specific control register reading functions
///
/// Copyright (c) 2008, Intel Corporation
/// All rights reserved. This program and the accompanying materials
/// are licensed and made available under the terms and conditions of the BSD License
/// which accompanies this distribution.  The full text of the license may be found at
/// http://opensource.org/licenses/bsd-license.php
///
/// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
/// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
///
///
///


.text
.type   AsmReadControlRegisterDcr, @function
.proc   AsmReadControlRegisterDcr
//
// Reads control register DCR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_DCR
//
// @return The 64-bit control register DCR.
//
AsmReadControlRegisterDcr::
  mov            r8 = cr.dcr;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterDcr



.text
.type   AsmReadControlRegisterItm, @function
.proc   AsmReadControlRegisterItm
//
// Reads control register ITM.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_ITM
//
// @return The 64-bit control register ITM.
//
AsmReadControlRegisterItm::
  mov            r8 = cr.itm;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterItm



.text
.type   AsmReadControlRegisterIva, @function
.proc   AsmReadControlRegisterIva
//
// Reads control register IVA.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IVA
//
// @return The 64-bit control register IVA.
//
AsmReadControlRegisterIva::
  mov            r8 = cr.iva;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIva



.text
.type   AsmReadControlRegisterPta, @function
.proc   AsmReadControlRegisterPta
//
// Reads control register PTA.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_PTA
//
// @return The 64-bit control register PTA.
//
AsmReadControlRegisterPta::
  mov            r8 = cr.pta;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterPta



.text
.type   AsmReadControlRegisterIpsr, @function
.proc   AsmReadControlRegisterIpsr
//
// Reads control register IPSR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IPSR
//
// @return The 64-bit control register IPSR.
//
AsmReadControlRegisterIpsr::
  mov            r8 = cr.ipsr;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIpsr



.text
.type   AsmReadControlRegisterIsr, @function
.proc   AsmReadControlRegisterIsr
//
// Reads control register ISR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_ISR
//
// @return The 64-bit control register ISR.
//
AsmReadControlRegisterIsr::
  mov            r8 = cr.isr;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIsr



.text
.type   AsmReadControlRegisterIip, @function
.proc   AsmReadControlRegisterIip
//
// Reads control register IIP.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IIP
//
// @return The 64-bit control register IIP.
//
AsmReadControlRegisterIip::
  mov            r8 = cr.iip;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIip



.text
.type   AsmReadControlRegisterIfa, @function
.proc   AsmReadControlRegisterIfa
//
// Reads control register IFA.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IFA
//
// @return The 64-bit control register IFA.
//
AsmReadControlRegisterIfa::
  mov            r8 = cr.ifa;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIfa



.text
.type   AsmReadControlRegisterItir, @function
.proc   AsmReadControlRegisterItir
//
// Reads control register ITIR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_ITIR
//
// @return The 64-bit control register ITIR.
//
AsmReadControlRegisterItir::
  mov            r8 = cr.itir;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterItir



.text
.type   AsmReadControlRegisterIipa, @function
.proc   AsmReadControlRegisterIipa
//
// Reads control register IIPA.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IIPA
//
// @return The 64-bit control register IIPA.
//
AsmReadControlRegisterIipa::
  mov            r8 = cr.iipa;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIipa



.text
.type   AsmReadControlRegisterIfs, @function
.proc   AsmReadControlRegisterIfs
//
// Reads control register IFS.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IFS
//
// @return The 64-bit control register IFS.
//
AsmReadControlRegisterIfs::
  mov            r8 = cr.ifs;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIfs



.text
.type   AsmReadControlRegisterIim, @function
.proc   AsmReadControlRegisterIim
//
// Reads control register IIM.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IIM
//
// @return The 64-bit control register IIM.
//
AsmReadControlRegisterIim::
  mov            r8 = cr.iim;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIim



.text
.type   AsmReadControlRegisterIha, @function
.proc   AsmReadControlRegisterIha
//
// Reads control register IHA.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IHA
//
// @return The 64-bit control register IHA.
//
AsmReadControlRegisterIha::
  mov            r8 = cr.iha;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIha



.text
.type   AsmReadControlRegisterLid, @function
.proc   AsmReadControlRegisterLid
//
// Reads control register LID.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_LID
//
// @return The 64-bit control register LID.
//
AsmReadControlRegisterLid::
  mov            r8 = cr.lid;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterLid



.text
.type   AsmReadControlRegisterIvr, @function
.proc   AsmReadControlRegisterIvr
//
// Reads control register IVR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IVR
//
// @return The 64-bit control register IVR.
//
AsmReadControlRegisterIvr::
  mov            r8 = cr.ivr;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIvr



.text
.type   AsmReadControlRegisterTpr, @function
.proc   AsmReadControlRegisterTpr
//
// Reads control register TPR.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_TPR
//
// @return The 64-bit control register TPR.
//
AsmReadControlRegisterTpr::
  mov            r8 = cr.tpr;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterTpr



.text
.type   AsmReadControlRegisterEoi, @function
.proc   AsmReadControlRegisterEoi
//
// Reads control register EOI.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_EOI
//
// @return The 64-bit control register EOI.
//
AsmReadControlRegisterEoi::
  mov            r8 = cr.eoi;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterEoi



.text
.type   AsmReadControlRegisterIrr0, @function
.proc   AsmReadControlRegisterIrr0
//
// Reads control register IRR0.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IRR0
//
// @return The 64-bit control register IRR0.
//
AsmReadControlRegisterIrr0::
  mov            r8 = cr.irr0;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIrr0



.text
.type   AsmReadControlRegisterIrr1, @function
.proc   AsmReadControlRegisterIrr1
//
// Reads control register IRR1.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IRR1
//
// @return The 64-bit control register IRR1.
//
AsmReadControlRegisterIrr1::
  mov            r8 = cr.irr1;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIrr1



.text
.type   AsmReadControlRegisterIrr2, @function
.proc   AsmReadControlRegisterIrr2
//
// Reads control register IRR2.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IRR2
//
// @return The 64-bit control register IRR2.
//
AsmReadControlRegisterIrr2::
  mov            r8 = cr.irr2;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIrr2



.text
.type   AsmReadControlRegisterIrr3, @function
.proc   AsmReadControlRegisterIrr3
//
// Reads control register IRR3.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_IRR3
//
// @return The 64-bit control register IRR3.
//
AsmReadControlRegisterIrr3::
  mov            r8 = cr.irr3;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterIrr3



.text
.type   AsmReadControlRegisterItv, @function
.proc   AsmReadControlRegisterItv
//
// Reads control register ITV.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_ITV
//
// @return The 64-bit control register ITV.
//
AsmReadControlRegisterItv::
  mov            r8 = cr.itv;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterItv



.text
.type   AsmReadControlRegisterPmv, @function
.proc   AsmReadControlRegisterPmv
//
// Reads control register PMV.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_PMV
//
// @return The 64-bit control register PMV.
//
AsmReadControlRegisterPmv::
  mov            r8 = cr.pmv;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterPmv



.text
.type   AsmReadControlRegisterCmcv, @function
.proc   AsmReadControlRegisterCmcv
//
// Reads control register CMCV.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_CMCV
//
// @return The 64-bit control register CMCV.
//
AsmReadControlRegisterCmcv::
  mov            r8 = cr.cmcv;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterCmcv



.text
.type   AsmReadControlRegisterLrr0, @function
.proc   AsmReadControlRegisterLrr0
//
// Reads control register LRR0.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_LRR0
//
// @return The 64-bit control register LRR0.
//
AsmReadControlRegisterLrr0::
  mov            r8 = cr.lrr0;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterLrr0



.text
.type   AsmReadControlRegisterLrr1, @function
.proc   AsmReadControlRegisterLrr1
//
// Reads control register LRR1.
//
// This is a worker function for AsmReadControlRegister()
// when its parameter Index is IPF_CONTROL_REGISTER_LRR1
//
// @return The 64-bit control register LRR1.
//
AsmReadControlRegisterLrr1::
  mov            r8 = cr.lrr1;;
  br.ret.dpnt    b0;;
.endp   AsmReadControlRegisterLrr1

