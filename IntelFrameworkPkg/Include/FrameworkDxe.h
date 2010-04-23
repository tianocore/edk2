/** @file
  The root header file that provides Framework extension to UEFI/PI for modules. It can be included by 
  DXE, RUNTIME and SMM type modules that use Framework definitions.


  This header file includes Framework extension definitions common to DXE
  modules.

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef _FRAMEWORK_DXE_H_
#define _FRAMEWORK_DXE_H_

#include <PiDxe.h>

#include <Framework/FrameworkInternalFormRepresentation.h>
#include <Framework/FirmwareVolumeImageFormat.h>
#include <Framework/FirmwareVolumeHeader.h>
#include <Framework/Hob.h>
#include <Framework/BootScript.h>
#include <Framework/StatusCode.h>
#include <Framework/DxeCis.h>

#endif
