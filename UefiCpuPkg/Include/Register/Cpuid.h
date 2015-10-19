/** @file
CPUID Definitions.

CPUID definitions based on contents of the Intel(R) 64 and IA-32 Architectures
Software Developer's Manual, Volume 2A, CPUID instruction.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __CPUID_H__
#define __CPUID_H__

//
// Definitions for CPUID instruction
//
#define CPUID_SIGNATURE                         0x0

#define CPUID_VERSION_INFO                      0x1

#define CPUID_CACHE_INFO                        0x2

#define CPUID_SERIAL_NUMBER                     0x3

#define CPUID_CACHE_PARAMS                      0x4

#define CPUID_EXTENDED_TOPOLOGY                 0xB
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID  0x0
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT      0x1
#define   CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE     0x2

#define CPUID_EXTENDED_FUNCTION                 0x80000000

#define CPUID_EXTENDED_CPU_SIG                  0x80000001

#define CPUID_BRAND_STRING1                     0x80000002

#define CPUID_BRAND_STRING2                     0x80000003

#define CPUID_BRAND_STRING3                     0x80000004

#define CPUID_VIR_PHY_ADDRESS_SIZE              0x80000008

#endif
