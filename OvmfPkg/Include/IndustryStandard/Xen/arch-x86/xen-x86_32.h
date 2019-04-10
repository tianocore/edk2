/******************************************************************************
 * xen-x86_32.h
 * 
 * Guest OS interface to x86 32-bit Xen.
 * 
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2004-2007, K A Fraser
 */

#ifndef __XEN_PUBLIC_ARCH_X86_XEN_X86_32_H__
#define __XEN_PUBLIC_ARCH_X86_XEN_X86_32_H__

/*
 * Hypercall interface:
 *  Input:  %ebx, %ecx, %edx, %esi, %edi, %ebp (arguments 1-6)
 *  Output: %eax
 * Access is via hypercall page (set up by guest loader or via a Xen MSR):
 *  call hypercall_page + hypercall-number * 32
 * Clobbered: Argument registers (e.g., 2-arg hypercall clobbers %ebx,%ecx)
 */

#ifndef __ASSEMBLY__

struct arch_vcpu_info {
    UINTN cr2;
    UINTN pad[5]; /* sizeof(vcpu_info_t) == 64 */
};
typedef struct arch_vcpu_info arch_vcpu_info_t;

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_X86_XEN_X86_32_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
