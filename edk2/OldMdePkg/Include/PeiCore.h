/** @file

  Root include file for PEI Core.

  The PEI Core has its own module type since its entry point definition is 
  unique. This module type should only be used by the PEI core. The build 
  infrastructure automatically sets EDK_RELEASE_VERSION before including 
  this file. 

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_CORE_H__
#define __PEI_CORE_H__


//
// Check to make sure EDK_RELEASE_VERSION is defined
//
#if !defined(EDK_RELEASE_VERSION)
  #error EDK_RELEASE_VERSION not defined
#elif (EDK_RELEASE_VERSION == 0)
  #error EDK_RELEASE_VERSION can not be zero
#endif



#include <Common/UefiBaseTypes.h>
#include <Peim/PeiCis.h>

#if defined(MDE_CPU_IPF)
#include <SalApi.h>
#include <PalApi.h>
#endif

//
//StatusCodeDataTypeId needs DebugSupport Protocol definition
//
#include <Protocol/DebugSupport.h>
#include <Common/StatusCodeDataTypeId.h>
#include <Guid/StatusCodeDataTypeId.h>

#include <Ppi/Pcd.h>

#endif
