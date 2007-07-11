//++
// Copyright (c) 2006, Intel Corporation                                                         
// All rights reserved. This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
// Module Name:
//  
//   IpfCpuCore.i
//
// Abstract:
//   IPF CPU definitions
//
//--

#ifndef _IPF_CPU_CORE_
#define _IPF_CPU_CORE_

#define  PEI_BSP_STORE_SIZE                     0x4000
#define  ResetFn                                0x00
#define  MachineCheckFn                         0x01
#define  InitFn                                 0x02
#define  RecoveryFn                             0x03
#define  GuardBand                              0x10 

//
// Define hardware RSE Configuration Register
//

//
// RS Configuration (RSC) bit field positions
//
#define RSC_MODE       0
#define RSC_PL         2
#define RSC_BE         4
//
// RSC bits 5-15 reserved
//
#define RSC_MBZ0       5
#define RSC_MBZ0_V     0x3ff
#define RSC_LOADRS     16
#define RSC_LOADRS_LEN 14
//
// RSC bits 30-63 reserved
//
#define RSC_MBZ1       30
#define RSC_MBZ1_V     0x3ffffffffULL

//
// RSC modes
//

//
// Lazy
//
#define RSC_MODE_LY (0x0)
//
// Store intensive
//
#define RSC_MODE_SI (0x1)
//
// Load intensive
//
#define RSC_MODE_LI (0x2)
//
// Eager
//
#define RSC_MODE_EA (0x3)

//
// RSC Endian bit values
//
#define RSC_BE_LITTLE 0
#define RSC_BE_BIG    1

//
// RSC while in kernel: enabled, little endian, pl = 0, eager mode
//
#define RSC_KERNEL ((RSC_MODE_EA<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))
//
// Lazy RSC in kernel: enabled, little endian, pl = 0, lazy mode
//
#define RSC_KERNEL_LAZ ((RSC_MODE_LY<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))
//
// RSE disabled: disabled, pl = 0, little endian, eager mode
//
#define RSC_KERNEL_DISABLED ((RSC_MODE_LY<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))

#endif
