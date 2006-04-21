/** @file

  Root include file for Mde Package DXE modules

  DXE modules follow the public Framework specifications and the UEFI 
  specifiations. The build infrastructure must set 
  EFI_SPECIFICATION_VERSION  before including  this file. To support 
  R9/UEFI2.0 set EFI_SPECIFIATION_VERSION to 0x00020000. To support 
  R8.5/EFI 1.10 set EFI_SPECIFIATION_VERSION to 0x00010010. 
  EDK_RELEASE_VERSION must be set to a non zero value.
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

#ifndef __DXE_H__
#define __DXE_H__

//
// Check to make sure EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION are defined.
//
#if !defined(EFI_SPECIFICATION_VERSION)
  #error EFI_SPECIFICATION_VERSION not defined
#elif !defined(EDK_RELEASE_VERSION)
  #error EDK_RELEASE_VERSION not defined
#elif (EDK_RELEASE_VERSION == 0)
  #error EDK_RELEASE_VERSION can not be zero
#endif


#include <Common/UefiBaseTypes.h>
#include <Dxe/DxeCis.h>
#include <Dxe/SmmCis.h>

#include <Common/DataHubRecords.h>
#include <Guid/DataHubRecords.h>

#include <Protocol/Pcd.h>
#include <Common/PcdTemp.h> //This will be removed when PCD PEIM is completed!

#endif
