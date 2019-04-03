/*++

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


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
