#****************************************************************************
#*                                                                         
#*  Copyright (c) 2006, Intel Corporation                                                         
#*  All rights reserved. This program and the accompanying materials                          
#*  are licensed and made available under the terms and conditions of the BSD License         
#*  which accompanies this distribution.  The full text of the license may be found at        
#*  http://opensource.org/licenses/bsd-license.php                                            
#*                                                                                            
#*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#*                                                                          
#****************************************************************************
.globl _EbcLLCALLEXNative
_EbcLLCALLEXNative:
        push   %ebp
        mov    %esp,%ebp
        mov    0x8(%esp),%ecx
        mov    0xc(%esp),%eax
        mov    %eax,%esp
        call   *%ecx
        mov    %ebp,%esp
        mov    %ebp,%esp
        pop    %ebp
        ret    

.globl _EbcLLGetEbcEntryPoint
_EbcLLGetEbcEntryPoint:
        ret    

.globl _EbcLLGetStackPointer
_EbcLLGetStackPointer:
        mov    %esp,%eax
        add    $0x4,%eax
        ret    

.globl _EbcLLGetReturnValue
_EbcLLGetReturnValue:
        ret    
