#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation                                                         
# All rights reserved. This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
# Module Name:
#
#   Ia32math.S
# 
# Abstract:
# 
#   Generic math routines for EBC interpreter running on IA32 processor
#
#------------------------------------------------------------------------------
.global _LeftShiftU64
_LeftShiftU64:
        push   %ebp
        mov    %esp,%ebp
        push   %ecx
        cmpl   $0x0,0x14(%ebp)
        jne    _LeftShiftU64+0x12
        mov    0x10(%ebp),%ecx
        cmp    $0x3f,%ecx
        jbe    _LeftShiftU64+0x18
        xor    %eax,%eax
        xor    %edx,%edx
        jmp    _LeftShiftU64+0x2c
        mov    0x8(%ebp),%eax
        mov    0xc(%ebp),%edx
        shld   %cl,%eax,%edx
        shl    %cl,%eax
        cmp    $0x20,%ecx
        jb     _LeftShiftU64+0x2c
        mov    %eax,%edx
        xor    %eax,%eax
        pop    %ecx
        leave  
        ret    

.global _RightShiftU64
_RightShiftU64:
        push   %ebp
        mov    %esp,%ebp
        push   %ecx
        cmpl   $0x0,0x14(%ebp)
        jne    _RightShiftU64+0x12
        mov    0x10(%ebp),%ecx
        cmp    $0x3f,%ecx
        jbe    _RightShiftU64+0x18
        xor    %eax,%eax
        xor    %edx,%edx
        jmp    _RightShiftU64+0x2c
        mov    0x8(%ebp),%eax
        mov    0xc(%ebp),%edx
        shrd   %cl,%edx,%eax
        shr    %cl,%edx
        cmp    $0x20,%ecx
        jb     _RightShiftU64+0x2c
        mov    %edx,%eax
        xor    %edx,%edx
        pop    %ecx
        leave  
        ret    

.global _ARightShift64
_ARightShift64:
        push   %ebp
        mov    %esp,%ebp
        push   %ecx
        cmpl   $0x0,0x14(%ebp)
        jne    _ARightShift64+0x12
        mov    0x10(%ebp),%ecx
        cmp    $0x3f,%ecx
        jbe    _ARightShift64+0x27
        btl    $0x1f,0xc(%ebp)
        jae    _ARightShift64+0x21
        or     $0xffffffff,%eax
        or     $0xffffffff,%edx
        jmp    _ARightShift64+0x3c
        xor    %eax,%eax
        xor    %edx,%edx
        jmp    _ARightShift64+0x3c
        mov    0x8(%ebp),%eax
        mov    0xc(%ebp),%edx
        shrd   %cl,%edx,%eax
        sar    %cl,%edx
        cmp    $0x20,%ecx
        jb     _ARightShift64+0x3c
        mov    %edx,%eax
        sar    $0x1f,%edx
        pop    %ecx
        leave  
        ret    

.global _MulU64x64
_MulU64x64:
        push   %ebp
        mov    %esp,%ebp
        push   %ebx
        push   %ecx
        mov    0x18(%ebp),%ebx
        mov    0x8(%ebp),%eax
        mull   0x10(%ebp)
        push   %eax
        mov    %edx,%ecx
        mov    0xc(%ebp),%eax
        mull   0x10(%ebp)
        add    %eax,%ecx
        adc    $0x0,%edx
        mov    %edx,(%ebx)
        mov    0x8(%ebp),%eax
        mull   0x14(%ebp)
        add    %eax,%ecx
        push   %ecx
        adc    $0x0,%edx
        mov    %edx,%ecx
        mov    0xc(%ebp),%eax
        mull   0x14(%ebp)
        add    %eax,%ecx
        adc    $0x0,%edx
        add    %ecx,(%ebx)
        adc    $0x0,%edx
        mov    %edx,0x4(%ebx)
        pop    %edx
        pop    %eax
        pop    %ecx
        pop    %ebx
        leave  
        ret    

.global _MulS64x64
_MulS64x64:
        push   %ebp
        mov    %esp,%ebp
        push   %ebx
        push   %ecx
        mov    0x18(%ebp),%ebx
        xor    %ecx,%ecx
        mov    0xc(%ebp),%edx
        bt     $0x1f,%edx
        jae    _MulS64x64+0x2a
        mov    0x8(%ebp),%eax
        not    %edx
        not    %eax
        add    $0x1,%eax
        adc    $0x0,%edx
        mov    %eax,0x8(%ebp)
        mov    %edx,0xc(%ebp)
        btc    $0x0,%ecx
        mov    0x14(%ebp),%edx
        bt     $0x1f,%edx
        jae    _MulS64x64+0x4a
        mov    0x10(%ebp),%eax
        not    %edx
        not    %eax
        add    $0x1,%eax
        adc    $0x0,%edx
        mov    %eax,0x10(%ebp)
        mov    %edx,0x14(%ebp)
        btc    $0x0,%ecx
        pushl  0x18(%ebp)
        pushl  0x14(%ebp)
        pushl  0x10(%ebp)
        pushl  0xc(%ebp)
        pushl  0x8(%ebp)
        call   _MulU64x64
        add    $0x14,%esp
        bt     $0x0,%ecx
        jae    _MulS64x64+0x7d
        not    %eax
        not    %edx
        notl   (%ebx)
        notl   0x4(%ebx)
        add    $0x1,%eax
        adc    $0x0,%edx
        adcl   $0x0,(%ebx)
        adcl   $0x0,0x4(%ebx)
        pop    %ecx
        pop    %ebx
        leave  
        ret    

.global _DivU64x64
_DivU64x64:
        push   %ebp
        mov    %esp,%ebp
        push   %ecx
        mov    0x1c(%ebp),%eax
        movl   $0x0,(%eax)
        cmpl   $0x0,0x10(%ebp)
        jne    _DivU64x64+0x3e
        cmpl   $0x0,0x14(%ebp)
        jne    _DivU64x64+0x3e
        movl   $0x1,(%eax)
        cmpl   $0x0,0x18(%ebp)
        je     _DivU64x64+0x35
        mov    0x18(%ebp),%eax
        movl   $0x0,(%eax)
        movl   $0x80000000,0x4(%eax)
        xor    %eax,%eax
        mov    $0x80000000,%edx
        jmp    _DivU64x64+0x7e
        xor    %edx,%edx
        xor    %eax,%eax
        mov    $0x40,%ecx
        shll   0x8(%ebp)
        rcll   0xc(%ebp)
        rcl    %eax
        rcl    %edx
        cmp    0x14(%ebp),%edx
        ja     _DivU64x64+0x5d
        jb     _DivU64x64+0x68
        cmp    0x10(%ebp),%eax
        jb     _DivU64x64+0x68
        btsl   $0x0,0x8(%ebp)
        sub    0x10(%ebp),%eax
        sbb    0x14(%ebp),%edx
        loop   _DivU64x64+0x47
        cmpl   $0x0,0x18(%ebp)
        je     _DivU64x64+0x78
        mov    0x18(%ebp),%ecx
        mov    %eax,(%ecx)
        mov    %edx,0x4(%ecx)
        mov    0x8(%ebp),%eax
        mov    0xc(%ebp),%edx
        pop    %ecx
        leave  
        ret    

.global _DivS64x64
_DivS64x64:
        push   %ebp
        mov    %esp,%ebp
        push   %ecx
        mov    0x1c(%ebp),%eax
        movl   $0x0,(%eax)
        cmpl   $0x0,0x10(%ebp)
        jne    _DivS64x64+0x41
        cmpl   $0x0,0x14(%ebp)
        jne    _DivS64x64+0x41
        movl   $0x1,(%eax)
        cmpl   $0x0,0x18(%ebp)
        je     _DivS64x64+0x35
        mov    0x18(%ebp),%eax
        movl   $0x0,(%eax)
        movl   $0x80000000,0x4(%eax)
        xor    %eax,%eax
        mov    $0x80000000,%edx
        jmp    _DivS64x64+0xc6
        xor    %ecx,%ecx
        mov    0xc(%ebp),%edx
        bt     $0x1f,%edx
        jae    _DivS64x64+0x67
        mov    0x8(%ebp),%eax
        not    %edx
        not    %eax
        add    $0x1,%eax
        adc    $0x0,%edx
        mov    %eax,0x8(%ebp)
        mov    %edx,0xc(%ebp)
        btc    $0x0,%ecx
        btc    $0x1,%ecx
        mov    0x14(%ebp),%edx
        bt     $0x1f,%edx
        jae    _DivS64x64+0x87
        mov    0x10(%ebp),%eax
        not    %edx
        not    %eax
        add    $0x1,%eax
        adc    $0x0,%edx
        mov    %eax,0x10(%ebp)
        mov    %edx,0x14(%ebp)
        btc    $0x0,%ecx
        pushl  0x1c(%ebp)
        pushl  0x18(%ebp)
        pushl  0x14(%ebp)
        pushl  0x10(%ebp)
        pushl  0xc(%ebp)
        pushl  0x8(%ebp)
        call   _DivU64x64
        add    $0x18,%esp
        bt     $0x0,%ecx
        jae    _DivS64x64+0xb1
        not    %eax
        not    %edx
        add    $0x1,%eax
        adc    $0x0,%edx
        bt     $0x1,%ecx
        jae    _DivS64x64+0xc6
        mov    0x18(%ebp),%ecx
        notl   (%ecx)
        notl   0x4(%ecx)
        addl   $0x1,(%ecx)
        adcl   $0x0,0x4(%ecx)
        pop    %ecx
        leave  
        ret    
