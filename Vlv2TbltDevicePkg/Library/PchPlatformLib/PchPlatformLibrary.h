/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

  @file
  PchPlatformLibrary.h

  @brief
  Header file for PCH Platform Lib implementation.

**/

#ifndef _PCH_PLATFORM_LIBRARY_IMPLEMENTATION_H_
#define _PCH_PLATFORM_LIBRARY_IMPLEMENTATION_H_

#include "PchAccess.h"
#ifdef ECP_FLAG
#include "EdkIIGlueBase.h"
#include "Library/EdkIIGlueMemoryAllocationLib.h"
#else
#include "Library/PciLib.h"
#include "Library/IoLib.h"
#include "Library/DebugLib.h"
#include "Library/PcdLib.h"

#include "PchCommonDefinitions.h"
#endif

#endif
