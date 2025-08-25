/******************************************************************************
 * arch-x86/xen.h
 *
 * Guest OS interface to x86 Xen.
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2004-2006, K A Fraser
 */

#include "../xen.h"

#ifndef __XEN_PUBLIC_ARCH_X86_XEN_H__
#define __XEN_PUBLIC_ARCH_X86_XEN_H__

/* Structural guest handles introduced in 0x00030201. */
  #if __XEN_INTERFACE_VERSION__ >= 0x00030201
#define ___DEFINE_XEN_GUEST_HANDLE(name, type) \
    typedef struct { type *p; } __guest_handle_ ## name
  #else
#define ___DEFINE_XEN_GUEST_HANDLE(name, type) \
    typedef type * __guest_handle_ ## name
  #endif

/*
 * XEN_GUEST_HANDLE represents a guest pointer, when passed as a field
 * in a struct in memory.
 * XEN_GUEST_HANDLE_PARAM represent a guest pointer, when passed as an
 * hypercall argument.
 * XEN_GUEST_HANDLE_PARAM and XEN_GUEST_HANDLE are the same on X86 but
 * they might not be on other architectures.
 */
#define __DEFINE_XEN_GUEST_HANDLE(name, type) \
    ___DEFINE_XEN_GUEST_HANDLE(name, type);   \
    ___DEFINE_XEN_GUEST_HANDLE(const_##name, const type)
#define DEFINE_XEN_GUEST_HANDLE(name)       __DEFINE_XEN_GUEST_HANDLE(name, name)
#define __XEN_GUEST_HANDLE(name)            __guest_handle_ ## name
#define XEN_GUEST_HANDLE(name)              __XEN_GUEST_HANDLE(name)
#define XEN_GUEST_HANDLE_PARAM(name)        XEN_GUEST_HANDLE(name)
#define set_xen_guest_handle_raw(hnd, val)  do { (hnd).p = val; } while (0)
  #ifdef __XEN_TOOLS__
#define get_xen_guest_handle(val, hnd)  do { val = (hnd).p; } while (0)
  #endif
#define set_xen_guest_handle(hnd, val)  set_xen_guest_handle_raw(hnd, val)

  #if defined (MDE_CPU_X64)
    #include "xen-x86_64.h"
  #endif

  #ifndef __ASSEMBLY__
typedef UINTN xen_pfn_t;
#define PRI_xen_pfn  "lx"
  #endif

#define XEN_HAVE_PV_UPCALL_MASK  1

/* Maximum number of virtual CPUs in legacy multi-processor guests. */
#define XEN_LEGACY_MAX_VCPUS  32

  #ifndef __ASSEMBLY__

typedef UINTN xen_ulong_t;
#define PRI_xen_ulong  "lx"

typedef UINT64 tsc_timestamp_t; /* RDTSC timestamp */

struct arch_shared_info {
  UINTN        max_pfn;             /* max pfn that appears in table */
  /* Frame containing list of mfns containing list of mfns containing p2m. */
  xen_pfn_t    pfn_to_mfn_frame_list_list;
  UINTN        nmi_reason;
  UINT64       pad[32];
};

typedef struct arch_shared_info arch_shared_info_t;
    #ifdef MDE_CPU_IA32
      #pragma pack()
    #endif

  #endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_ARCH_X86_XEN_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
