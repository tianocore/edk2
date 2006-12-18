/** @file

  Root include file for Mde Package UEFI modules.

  UEFI modules follow the public EFI 1.10 or UEFI 2.0 specifications and
  also contains the infrastructure required to build modules. The build 
  infrastructure must set EFI_SPECIFICATION_VERSION  before including  this 
  file. To support EDK II/UEFI2.0 set EFI_SPECIFIATION_VERSION to 0x00020000. To 
  support EDK/EFI 1.10 set EFI_SPECIFIATION_VERSION to 0x00010010. 
  Seting EDK_RELEASE_VERSION to zero implies no Tiano extensions and a
  non zero value implies Tiano extensions are availible. 
  EFI_SPECIFIATION_VERSION and EDK_RELEASE_VERSION are set automatically
  by the build infrastructure for every module.
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __UEFI_H__
#define __UEFI_H__


//
// Check to make sure EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION are defined.
//  also check for legal combinations
//
#if !defined(EFI_SPECIFICATION_VERSION)
  #error EFI_SPECIFICATION_VERSION not defined
#elif !defined(EDK_RELEASE_VERSION)
  #error EDK_RELEASE_VERSION not defined
#elif EDK_RELEASE_VERSION == 0x00000000
//
// UEFI mode with no Tiano extensions is legal
//
#elif (EDK_RELEASE_VERSION < 0x00020000) && (EFI_SPECIFICATION_VERSION >= 0x00020000)
  #error Illegal combination of EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION versions
#endif



#include <Common/UefiBaseTypes.h>
#include <Uefi/UefiSpec.h>

#if defined(MDE_CPU_IPF)
#include <SalApi.h>
#endif

#endif
