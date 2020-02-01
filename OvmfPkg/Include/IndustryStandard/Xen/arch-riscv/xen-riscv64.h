/******************************************************************************
 * xen-riscv64.h
 * 
 * Guest OS interface to RISC-V 64-bit Xen.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR> 
 * Copyright (c) 2004-2006, K A Fraser
 */

#ifndef __XEN_PUBLIC_ARCH_RISCV_XEN_RISCV64_H__
#define __XEN_PUBLIC_ARCH_RISCV_XEN_RISCV64_H__

/*
 * Hypercall interface:
 *  Input:  TBD
 *  Output: TBD
 */

#ifndef __ASSEMBLY__

struct arch_vcpu_info {
    UINTN pad [8]; /* sizeof(vcpu_info_t) == 64 */
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_RISCV_XEN_RISCV64_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
