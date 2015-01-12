/*++

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  PlatformDefinitions.h

Abstract:

  This header file provides platform specific definitions used by other modules
  for platform specific initialization.

  THIS FILE SHOULD ONLY CONTAIN #defines BECAUSE IT IS CONSUMED BY NON-C MODULES
  (ASL and VFR)

  This file should not contain addition or other operations that an ASL compiler or
  VFR compiler does not understand.

--*/

#ifndef _PLATFORM_DEFINITIONS_H_
#define _PLATFORM_DEFINITIONS_H_


//
// Platform Base Address definitions
//
#define PCIEX_BASE_ADDRESS          EDKII_GLUE_PciExpressBaseAddress // Pci Express Configuration Space Base Address

#define PCIEX_LENGTH                PLATFORM_PCIEXPRESS_LENGTH

#define THERMAL_BASE_ADDRESS        0xFED08000

#ifndef MCH_BASE_ADDRESS
#define MCH_BASE_ADDRESS            0xFED10000  // MCH  Register Base Address
#endif
#endif
