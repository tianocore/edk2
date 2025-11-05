/******************************************************************************
 * xen-x86_64.h
 *
 * Guest OS interface to x86 64-bit Xen.
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2004-2006, K A Fraser
 */

#ifndef __XEN_PUBLIC_ARCH_X86_XEN_X86_64_H__
#define __XEN_PUBLIC_ARCH_X86_XEN_X86_64_H__

/*
 * Hypercall interface:
 *  Input:  %rdi, %rsi, %rdx, %r10, %r8, %r9 (arguments 1-6)
 *  Output: %rax
 * Access is via hypercall page (set up by guest loader or via a Xen MSR):
 *  call hypercall_page + hypercall-number * 32
 * Clobbered: argument registers (e.g., 2-arg hypercall clobbers %rdi,%rsi)
 */

#ifndef __ASSEMBLY__

struct arch_vcpu_info {
  UINTN    cr2;
  UINTN    pad; /* sizeof(vcpu_info_t) == 64 */
};

typedef struct arch_vcpu_info arch_vcpu_info_t;

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_X86_XEN_X86_64_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
