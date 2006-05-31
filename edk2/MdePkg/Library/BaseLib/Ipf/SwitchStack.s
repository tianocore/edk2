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
//  SwitchStack.s
//
// Abstract:
//
//  Contains an implementation of a stack switch for the Itanium-based architecture.
//
//
//
// Revision History:
//
//--

  .file  "SwitchStack.s"

#include  "asm.h"
#include  "ia_64gen.h"

// Define hardware RSE Configuration Register
//
// RS Configuration (RSC) bit field positions

#define RSC_MODE       0
#define RSC_PL         2
#define RSC_BE         4
// RSC bits 5-15 reserved
#define RSC_MBZ0       5
#define RSC_MBZ0_V     0x3ff
#define RSC_LOADRS     16
#define RSC_LOADRS_LEN 14
// RSC bits 30-63 reserved
#define RSC_MBZ1       30
#define RSC_MBZ1_V     0x3ffffffffULL

// RSC modes
// Lazy
#define RSC_MODE_LY (0x0)
// Store intensive
#define RSC_MODE_SI (0x1)
// Load intensive
#define RSC_MODE_LI (0x2)
// Eager
#define RSC_MODE_EA (0x3)

// RSC Endian bit values
#define RSC_BE_LITTLE 0
#define RSC_BE_BIG    1

// RSC while in kernel: enabled, little endian, pl = 0, eager mode
#define RSC_KERNEL ((RSC_MODE_EA<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))
// Lazy RSC in kernel: enabled, little endian, pl = 0, lazy mode
#define RSC_KERNEL_LAZ ((RSC_MODE_LY<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))
// RSE disabled: disabled, pl = 0, little endian, eager mode
#define RSC_KERNEL_DISABLED ((RSC_MODE_LY<<RSC_MODE) | (RSC_BE_LITTLE<<RSC_BE))


//VOID
//_SwitchStack (
//    VOID    *ContinuationFunction,
//    UINTN   Parameter,
//    UINTN   NewTopOfStack,
//    UINTN   NewBSPStore OPTIONAL
//)
///*++
//
//Input Arguments
//
//  ContinuationFunction - This is a pointer to the PLABEL of the function that should  be called once the
//        new stack has been created.  
//  Parameter - The parameter to pass to the continuation function
//  NewTopOfStack - This is the new top of the memory stack for ensuing code.  This is mandatory and
//      should be non-zero
//  NewBSPStore - This is the new BSP store for the ensuing code.  It is optional on IA-32 and mandatory on Itanium-based platform.
//
//--*/

PROCEDURE_ENTRY(_SwitchStack)

        mov        r16 = -0x10;;
        and        r16 = r34, r16;;             // get new stack value in R16, 0 the last nibble.
        mov        r15 = r35;;                  // Get new BspStore into R15
        mov        r13 = r32;;                  // this is a pointer to the PLABEL of the continuation function.  
        mov        r17 = r33;;                  // this is the parameter to pass to the continuation function

        alloc       r11=0,0,0,0                 // Set 0-size frame
        ;;
        flushrs;;

        mov         r21 = RSC_KERNEL_DISABLED   // for rse disable             
        ;;
        mov         ar.rsc = r21                // turn off RSE                

        add         sp = r0, r16;;              // transfer to the EFI stack
        mov         ar.bspstore = r15           // switch to EFI BSP        
        invala                                  // change of ar.bspstore needs invala.     
      
        mov         r18 = RSC_KERNEL_LAZ        // RSC enabled, Lazy mode
        ;;
        mov         ar.rsc = r18                // turn rse on, in kernel mode     
        ;;        
        alloc       r11=0,0,1,0;;               // alloc 0 outs going to ensuing DXE IPL service
        mov         out0 = r17
        ld8         r16 = [r13],8;;             // r16 = address of continuation function from the PLABEL
        ld8         gp = [r13]                  // gp  = gp of continuation function from the PLABEL
        mov         b6 = r16
        ;;
        br.call.sptk.few b0=b6;;                // Call the continuation function
        ;;
PROCEDURE_EXIT(_SwitchStack)


