/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Tiano.h

Abstract:

  Tiano master include file.

  This is the main include file for Tiano components. 

  Don't add include files to the list for convenience, only add things
  that are architectural. Don't add Protocols or GUID include files here

--*/

#ifndef _TIANO_H_
#define _TIANO_H_

//
// Check to make sure EFI_SPECIFICATION_VERSION and TIANO_RELEASE_VERSION are defined.
//
#if !defined(EFI_SPECIFICATION_VERSION)
  #error EFI_SPECIFICATION_VERSION not defined
#elif !defined(TIANO_RELEASE_VERSION)
  #error TIANO_RELEASE_VERSION not defined
#elif (TIANO_RELEASE_VERSION == 0)
  #error TIANO_RELEASE_VERSION can not be zero
#elif (EFI_SPECIFICATION_VERSION <= 0x00020000)
  #define TIANO_EXTENSION_FLAG
#endif

#include "TianoCommon.h"
#include "TianoApi.h"
#include "EfiDebug.h"
#include "TianoDevicePath.h"
#include "EfiSpec.h"

//
// EFI Revision information
//
#define EFI_FIRMWARE_MAJOR_REVISION 0x1000
#define EFI_FIRMWARE_MINOR_REVISION 1
#define EFI_FIRMWARE_REVISION       ((EFI_FIRMWARE_MAJOR_REVISION << 16) | (EFI_FIRMWARE_MINOR_REVISION))

#endif
