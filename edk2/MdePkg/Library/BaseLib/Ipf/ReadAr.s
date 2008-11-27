/// @file
///  IPF specific application register reading functions
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
.type   AsmReadApplicationRegisterK0, @function
.proc   AsmReadApplicationRegisterK0
//
// Reads appplication register K0.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K0
//
// @return The 64-bit application register K0.
//
AsmReadApplicationRegisterK0::
  mov            r8 = ar.k0;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK0



.text
.type   AsmReadApplicationRegisterK1, @function
.proc   AsmReadApplicationRegisterK1
//
// Reads appplication register K1.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K1
//
// @return The 64-bit application register K1.
//
AsmReadApplicationRegisterK1::
  mov            r8 = ar.k1;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK1



.text
.type   AsmReadApplicationRegisterK2, @function
.proc   AsmReadApplicationRegisterK2
//
// Reads appplication register K2.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K2
//
// @return The 64-bit application register K2.
//
AsmReadApplicationRegisterK2::
  mov            r8 = ar.k2;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK2



.text
.type   AsmReadApplicationRegisterK3, @function
.proc   AsmReadApplicationRegisterK3
//
// Reads appplication register K3.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K3
//
// @return The 64-bit application register K3.
//
AsmReadApplicationRegisterK3::
  mov            r8 = ar.k3;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK3



.text
.type   AsmReadApplicationRegisterK4, @function
.proc   AsmReadApplicationRegisterK4
//
// Reads appplication register K4.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K4
//
// @return The 64-bit application register K4.
//
AsmReadApplicationRegisterK4::
  mov            r8 = ar.k4;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK4



.text
.type   AsmReadApplicationRegisterK5, @function
.proc   AsmReadApplicationRegisterK5
//
// Reads appplication register K5.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K5
//
// @return The 64-bit application register K5.
//
AsmReadApplicationRegisterK5::
  mov            r8 = ar.k5;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK5



.text
.type   AsmReadApplicationRegisterK6, @function
.proc   AsmReadApplicationRegisterK6
//
// Reads appplication register K6.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K6
//
// @return The 64-bit application register K6.
//
AsmReadApplicationRegisterK6::
  mov            r8 = ar.k6;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK6



.text
.type   AsmReadApplicationRegisterK7, @function
.proc   AsmReadApplicationRegisterK7
//
// Reads appplication register K7.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_K7
//
// @return The 64-bit application register K7.
//
AsmReadApplicationRegisterK7::
  mov            r8 = ar.k7;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterK7



.text
.type   AsmReadApplicationRegisterRsc, @function
.proc   AsmReadApplicationRegisterRsc
//
// Reads appplication register RSC.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_RSC
//
// @return The 64-bit application register RSC.
//
AsmReadApplicationRegisterRsc::
  mov            r8 = ar.rsc;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterRsc



.text
.type   AsmReadApplicationRegisterBsp, @function
.proc   AsmReadApplicationRegisterBsp
//
// Reads appplication register BSP.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_BSP
//
// @return The 64-bit application register BSP.
//
AsmReadApplicationRegisterBsp::
  mov            r8 = ar.bsp;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterBsp



.text
.type   AsmReadApplicationRegisterBspstore, @function
.proc   AsmReadApplicationRegisterBspstore
//
// Reads appplication register BSPSTORE.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_BSPSTORE
//
// @return The 64-bit application register BSPSTORE.
//
AsmReadApplicationRegisterBspstore::
  mov            r8 = ar.bspstore;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterBspstore



.text
.type   AsmReadApplicationRegisterRnat, @function
.proc   AsmReadApplicationRegisterRnat
//
// Reads appplication register RNAT.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_RNAT
//
// @return The 64-bit application register RNAT.
//
AsmReadApplicationRegisterRnat::
  mov            r8 = ar.rnat;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterRnat



.text
.type   AsmReadApplicationRegisterFcr, @function
.proc   AsmReadApplicationRegisterFcr
//
// Reads appplication register FCR.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_FCR
//
// @return The 64-bit application register FCR.
//
AsmReadApplicationRegisterFcr::
  mov            r8 = ar.fcr;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterFcr



.text
.type   AsmReadApplicationRegisterEflag, @function
.proc   AsmReadApplicationRegisterEflag
//
// Reads appplication register EFLAG.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_EFLAG
//
// @return The 64-bit application register EFLAG.
//
AsmReadApplicationRegisterEflag::
  mov            r8 = ar.eflag;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterEflag



.text
.type   AsmReadApplicationRegisterCsd, @function
.proc   AsmReadApplicationRegisterCsd
//
// Reads appplication register CSD.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_CSD
//
// @return The 64-bit application register CSD.
//
AsmReadApplicationRegisterCsd::
  mov            r8 = ar.csd;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterCsd



.text
.type   AsmReadApplicationRegisterSsd, @function
.proc   AsmReadApplicationRegisterSsd
//
// Reads appplication register SSD.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_SSD
//
// @return The 64-bit application register SSD.
//
AsmReadApplicationRegisterSsd::
  mov            r8 = ar.ssd;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterSsd



.text
.type   AsmReadApplicationRegisterCflg, @function
.proc   AsmReadApplicationRegisterCflg
//
// Reads appplication register CFLG.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_CFLG
//
// @return The 64-bit application register CFLG.
//
AsmReadApplicationRegisterCflg::
  mov            r8 = ar.cflg;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterCflg



.text
.type   AsmReadApplicationRegisterFsr, @function
.proc   AsmReadApplicationRegisterFsr
//
// Reads appplication register FSR.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_FSR
//
// @return The 64-bit application register FSR.
//
AsmReadApplicationRegisterFsr::
  mov            r8 = ar.fsr;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterFsr



.text
.type   AsmReadApplicationRegisterFir, @function
.proc   AsmReadApplicationRegisterFir
//
// Reads appplication register FIR.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_FIR
//
// @return The 64-bit application register FIR.
//
AsmReadApplicationRegisterFir::
  mov            r8 = ar.fir;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterFir



.text
.type   AsmReadApplicationRegisterFdr, @function
.proc   AsmReadApplicationRegisterFdr
//
// Reads appplication register FDR.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_FDR
//
// @return The 64-bit application register FDR.
//
AsmReadApplicationRegisterFdr::
  mov            r8 = ar.fdr;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterFdr



.text
.type   AsmReadApplicationRegisterCcv, @function
.proc   AsmReadApplicationRegisterCcv
//
// Reads appplication register CCV.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_CCV
//
// @return The 64-bit application register CCV.
//
AsmReadApplicationRegisterCcv::
  mov            r8 = ar.ccv;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterCcv



.text
.type   AsmReadApplicationRegisterUnat, @function
.proc   AsmReadApplicationRegisterUnat
//
// Reads appplication register UNAT.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_UNAT
//
// @return The 64-bit application register UNAT.
//
AsmReadApplicationRegisterUnat::
  mov            r8 = ar.unat;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterUnat



.text
.type   AsmReadApplicationRegisterFpsr, @function
.proc   AsmReadApplicationRegisterFpsr
//
// Reads appplication register FPSR.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_FPSR
//
// @return The 64-bit application register FPSR.
//
AsmReadApplicationRegisterFpsr::
  mov            r8 = ar.fpsr;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterFpsr



.text
.type   AsmReadApplicationRegisterItc, @function
.proc   AsmReadApplicationRegisterItc
//
// Reads appplication register ITC.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_ITC
//
// @return The 64-bit application register ITC.
//
AsmReadApplicationRegisterItc::
  mov            r8 = ar.itc;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterItc



.text
.type   AsmReadApplicationRegisterPfs, @function
.proc   AsmReadApplicationRegisterPfs
//
// Reads appplication register PFS.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_PFS
//
// @return The 64-bit application register PFS.
//
AsmReadApplicationRegisterPfs::
  mov            r8 = ar.pfs;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterPfs



.text
.type   AsmReadApplicationRegisterLc, @function
.proc   AsmReadApplicationRegisterLc
//
// Reads appplication register LC.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_LC
//
// @return The 64-bit application register LC.
//
AsmReadApplicationRegisterLc::
  mov            r8 = ar.lc;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterLc



.text
.type   AsmReadApplicationRegisterEc, @function
.proc   AsmReadApplicationRegisterEc
//
// Reads appplication register EC.
//
// This is a worker function for AsmReadApplicationRegister()
// when its parameter Index is IPF_APPLICATION_REGISTER_EC
//
// @return The 64-bit application register EC.
//
AsmReadApplicationRegisterEc::
  mov            r8 = ar.ec;;
  br.ret.dpnt    b0;;
.endp   AsmReadApplicationRegisterEc

