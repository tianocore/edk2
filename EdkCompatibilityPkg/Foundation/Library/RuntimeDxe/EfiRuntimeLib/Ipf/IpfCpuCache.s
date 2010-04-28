//++
// Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
// This program and the accompanying materials                          
// are licensed and made available under the terms and conditions of the BSD License         
// which accompanies this distribution.  The full text of the license may be found at        
// http://opensource.org/licenses/bsd-license.php                                            
//                                                                                           
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
// 
//  Module Name:
//
//    IpfCpuCache.s
//
//  Abstract:
//
//    Contains Misc assembly procedures to support IPF CPU AP.
//
// Revision History:
//
//--

.file  "IpfCpuCache.s"

#include  "IpfMacro.i"
#include  "IpfDefines.h"

//-----------------------------------------------------------------------------
//++
// Flush Cache
//
// Arguments : 

// Input = in0 = Starting Address to Flush.
// Input = in1 = Length in bytes.
// Input = b0 = return branch register.
// On Entry :
//
// Return Value: 
//
//  VOID
//  SalFlushCache (
//    IN UINT64   BaseToFlush,
//    IN UINT64   LengthToFlush
//    );
//
//--
//---------------------------------------------------------------------------
PROCEDURE_ENTRY (SalFlushCache)

      NESTED_SETUP (5,8,0,0)
            
      mov         loc2 = ar.lc
      
      mov         loc3 = in0                  // Start address.
      mov         loc4 = in1;;                // Length in bytes.
      
      cmp.eq  p6,p7 = loc4, r0;;               // If Length is zero then don't flush any cache
      (p6)  br.spnt.many DoneFlushingC;;         
      
      add         loc4 = loc4,loc3 
      mov         loc5 = 1;;
      sub         loc4 = loc4, loc5 ;; // the End address to flush
                                         
      dep         loc3 = r0,loc3,0,5          
      dep         loc4 = r0,loc4,0,5;;         
      shr         loc3 = loc3,5             
      shr         loc4 = loc4,5;;    // 32 byte cache line
      
      sub         loc4 = loc4,loc3;; // total flush count, It should be add 1 but 
                                     // the br.cloop will first execute one time 
      mov         loc3 = in0                  
      mov         loc5 = 32      
      mov         ar.lc = loc4;;

StillFlushingC:
      fc          loc3;; 
      sync.i;;
      srlz.i;;
      add         loc3 = loc5,loc3;;
      br.cloop.sptk.few StillFlushingC;;

DoneFlushingC:      
      mov         ar.lc = loc2     
      NESTED_RETURN

PROCEDURE_EXIT (SalFlushCache)

