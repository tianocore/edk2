/******************************************************************************
 * xen.h
 *
 * Guest OS interface to Xen.
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2004, K A Fraser
 */

#ifndef __XEN_PUBLIC_XEN_H__
#define __XEN_PUBLIC_XEN_H__

//
// Xen interface version used by Tianocore
//
#define __XEN_INTERFACE_VERSION__  0x00040400

#include "xen-compat.h"

#if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
  #include "arch-x86/xen.h"
#elif defined (__arm__) || defined (__aarch64__)
  #include "arch-arm/xen.h"
#else
  #error "Unsupported architecture"
#endif

#ifndef __ASSEMBLY__
/* Guest handles for primitive C types. */
DEFINE_XEN_GUEST_HANDLE (CHAR8);
__DEFINE_XEN_GUEST_HANDLE (uchar, UINT8);
DEFINE_XEN_GUEST_HANDLE (INT32);
__DEFINE_XEN_GUEST_HANDLE (uint, UINT32);
  #if __XEN_INTERFACE_VERSION__ < 0x00040300
DEFINE_XEN_GUEST_HANDLE (INTN);
__DEFINE_XEN_GUEST_HANDLE (ulong, UINTN);
  #endif
DEFINE_XEN_GUEST_HANDLE (VOID);

DEFINE_XEN_GUEST_HANDLE (UINT64);
DEFINE_XEN_GUEST_HANDLE (xen_pfn_t);
DEFINE_XEN_GUEST_HANDLE (xen_ulong_t);
#endif

/*
 * HYPERCALLS
 */

/* `incontents 100 hcalls List of hypercalls
 * ` enum hypercall_num { // __HYPERVISOR_* => HYPERVISOR_*()
 */

#define __HYPERVISOR_set_trap_table                 0
#define __HYPERVISOR_mmu_update                     1
#define __HYPERVISOR_set_gdt                        2
#define __HYPERVISOR_stack_switch                   3
#define __HYPERVISOR_set_callbacks                  4
#define __HYPERVISOR_fpu_taskswitch                 5
#define __HYPERVISOR_sched_op_compat                6/* compat since 0x00030101 */
#define __HYPERVISOR_platform_op                    7
#define __HYPERVISOR_set_debugreg                   8
#define __HYPERVISOR_get_debugreg                   9
#define __HYPERVISOR_update_descriptor              10
#define __HYPERVISOR_memory_op                      12
#define __HYPERVISOR_multicall                      13
#define __HYPERVISOR_update_va_mapping              14
#define __HYPERVISOR_set_timer_op                   15
#define __HYPERVISOR_event_channel_op_compat        16/* compat since 0x00030202 */
#define __HYPERVISOR_xen_version                    17
#define __HYPERVISOR_console_io                     18
#define __HYPERVISOR_physdev_op_compat              19/* compat since 0x00030202 */
#define __HYPERVISOR_grant_table_op                 20
#define __HYPERVISOR_vm_assist                      21
#define __HYPERVISOR_update_va_mapping_otherdomain  22
#define __HYPERVISOR_iret                           23/* x86 only */
#define __HYPERVISOR_vcpu_op                        24
#define __HYPERVISOR_set_segment_base               25/* x86/64 only */
#define __HYPERVISOR_mmuext_op                      26
#define __HYPERVISOR_xsm_op                         27
#define __HYPERVISOR_nmi_op                         28
#define __HYPERVISOR_sched_op                       29
#define __HYPERVISOR_callback_op                    30
#define __HYPERVISOR_xenoprof_op                    31
#define __HYPERVISOR_event_channel_op               32
#define __HYPERVISOR_physdev_op                     33
#define __HYPERVISOR_hvm_op                         34
#define __HYPERVISOR_sysctl                         35
#define __HYPERVISOR_domctl                         36
#define __HYPERVISOR_kexec_op                       37
#define __HYPERVISOR_tmem_op                        38
#define __HYPERVISOR_xc_reserved_op                 39/* reserved for XenClient */

/* Architecture-specific hypercall definitions. */
#define __HYPERVISOR_arch_0  48
#define __HYPERVISOR_arch_1  49
#define __HYPERVISOR_arch_2  50
#define __HYPERVISOR_arch_3  51
#define __HYPERVISOR_arch_4  52
#define __HYPERVISOR_arch_5  53
#define __HYPERVISOR_arch_6  54
#define __HYPERVISOR_arch_7  55

/* ` } */

/*
 * HYPERCALL COMPATIBILITY.
 */

/* New sched_op hypercall introduced in 0x00030101. */
#if __XEN_INTERFACE_VERSION__ < 0x00030101
  #undef __HYPERVISOR_sched_op
#define __HYPERVISOR_sched_op  __HYPERVISOR_sched_op_compat
#endif

/* New event-channel and physdev hypercalls introduced in 0x00030202. */
#if __XEN_INTERFACE_VERSION__ < 0x00030202
  #undef __HYPERVISOR_event_channel_op
#define __HYPERVISOR_event_channel_op  __HYPERVISOR_event_channel_op_compat
  #undef __HYPERVISOR_physdev_op
#define __HYPERVISOR_physdev_op  __HYPERVISOR_physdev_op_compat
#endif

/* New platform_op hypercall introduced in 0x00030204. */
#if __XEN_INTERFACE_VERSION__ < 0x00030204
#define __HYPERVISOR_dom0_op  __HYPERVISOR_platform_op
#endif

#ifndef __ASSEMBLY__

typedef UINT16 domid_t;

/* Domain ids >= DOMID_FIRST_RESERVED cannot be used for ordinary domains. */
#define DOMID_FIRST_RESERVED  (0x7FF0U)

/* DOMID_SELF is used in certain contexts to refer to oneself. */
#define DOMID_SELF  (0x7FF0U)

/*
 * DOMID_IO is used to restrict page-table updates to mapping I/O memory.
 * Although no Foreign Domain need be specified to map I/O pages, DOMID_IO
 * is useful to ensure that no mappings to the OS's own heap are accidentally
 * installed. (e.g., in Linux this could cause havoc as reference counts
 * aren't adjusted on the I/O-mapping code path).
 * This only makes sense in MMUEXT_SET_FOREIGNDOM, but in that context can
 * be specified by any calling domain.
 */
#define DOMID_IO  (0x7FF1U)

/*
 * DOMID_XEN is used to allow privileged domains to map restricted parts of
 * Xen's heap space (e.g., the machine_to_phys table).
 * This only makes sense in MMUEXT_SET_FOREIGNDOM, and is only permitted if
 * the caller is privileged.
 */
#define DOMID_XEN  (0x7FF2U)

/*
 * DOMID_COW is used as the owner of sharable pages */
#define DOMID_COW  (0x7FF3U)

/* DOMID_INVALID is used to identify pages with unknown owner. */
#define DOMID_INVALID  (0x7FF4U)

/* Idle domain. */
#define DOMID_IDLE  (0x7FFFU)

  #if __XEN_INTERFACE_VERSION__ < 0x00040400

/*
 * Event channel endpoints per domain (when using the 2-level ABI):
 *  1024 if a INTN is 32 bits; 4096 if a INTN is 64 bits.
 */
#define NR_EVENT_CHANNELS  EVTCHN_2L_NR_CHANNELS
  #endif

struct vcpu_time_info {
  /*
   * Updates to the following values are preceded and followed by an
   * increment of 'version'. The guest can therefore detect updates by
   * looking for changes to 'version'. If the least-significant bit of
   * the version number is set then an update is in progress and the guest
   * must wait to read a consistent set of values.
   * The correct way to interact with the version number is similar to
   * Linux's seqlock: see the implementations of read_seqbegin/read_seqretry.
   */
  UINT32    Version;
  UINT32    pad0;
  UINT64    TscTimestamp;  /* TSC at last update of time vals.  */
  UINT64    SystemTime;    /* Time, in nanosecs, since boot.    */

  /*
   * Current system time:
   *   system_time +
   *   ((((tsc - tsc_timestamp) << tsc_shift) * tsc_to_system_mul) >> 32)
   * CPU frequency (Hz):
   *   ((10^9 << 32) / tsc_to_system_mul) >> tsc_shift
   */
  UINT32    TscToSystemMultiplier;
  INT8      TscShift;
  INT8      pad1[3];
}; /* 32 bytes */

typedef struct vcpu_time_info XEN_VCPU_TIME_INFO;

struct vcpu_info {
  /*
   * 'evtchn_upcall_pending' is written non-zero by Xen to indicate
   * a pending notification for a particular VCPU. It is then cleared
   * by the guest OS /before/ checking for pending work, thus avoiding
   * a set-and-check race. Note that the mask is only accessed by Xen
   * on the CPU that is currently hosting the VCPU. This means that the
   * pending and mask flags can be updated by the guest without special
   * synchronisation (i.e., no need for the x86 LOCK prefix).
   * This may seem suboptimal because if the pending flag is set by
   * a different CPU then an IPI may be scheduled even when the mask
   * is set. However, note:
   *  1. The task of 'interrupt holdoff' is covered by the per-event-
   *     channel mask bits. A 'noisy' event that is continually being
   *     triggered can be masked at source at this very precise
   *     granularity.
   *  2. The main purpose of the per-VCPU mask is therefore to restrict
   *     reentrant execution: whether for concurrency control, or to
   *     prevent unbounded stack usage. Whatever the purpose, we expect
   *     that the mask will be asserted only for short periods at a time,
   *     and so the likelihood of a 'spurious' IPI is suitably small.
   * The mask is read before making an event upcall to the guest: a
   * non-zero mask therefore guarantees that the VCPU will not receive
   * an upcall activation. The mask is cleared when the VCPU requests
   * to block: this avoids wakeup-waiting races.
   */
  UINT8                    evtchn_upcall_pending;
 #ifdef XEN_HAVE_PV_UPCALL_MASK
  UINT8                    evtchn_upcall_mask;
 #else /* XEN_HAVE_PV_UPCALL_MASK */
  UINT8                    pad0;
 #endif /* XEN_HAVE_PV_UPCALL_MASK */
  xen_ulong_t              evtchn_pending_sel;
  struct arch_vcpu_info    arch;
  struct vcpu_time_info    Time;
}; /* 64 bytes (x86) */

  #ifndef __XEN__
typedef struct vcpu_info vcpu_info_t;
  #endif

/*
 * `incontents 200 startofday_shared Start-of-day shared data structure
 * Xen/kernel shared data -- pointer provided in start_info.
 *
 * This structure is defined to be both smaller than a page, and the
 * only data on the shared page, but may vary in actual size even within
 * compatible Xen versions; guests should not rely on the size
 * of this structure remaining constant.
 */
struct shared_info {
  struct vcpu_info    VcpuInfo[XEN_LEGACY_MAX_VCPUS];

  /*
   * A domain can create "event channels" on which it can send and receive
   * asynchronous event notifications. There are three classes of event that
   * are delivered by this mechanism:
   *  1. Bi-directional inter- and intra-domain connections. Domains must
   *     arrange out-of-band to set up a connection (usually by allocating
   *     an unbound 'listener' port and avertising that via a storage service
   *     such as xenstore).
   *  2. Physical interrupts. A domain with suitable hardware-access
   *     privileges can bind an event-channel port to a physical interrupt
   *     source.
   *  3. Virtual interrupts ('events'). A domain can bind an event-channel
   *     port to a virtual interrupt source, such as the virtual-timer
   *     device or the emergency console.
   *
   * Event channels are addressed by a "port index". Each channel is
   * associated with two bits of information:
   *  1. PENDING -- notifies the domain that there is a pending notification
   *     to be processed. This bit is cleared by the guest.
   *  2. MASK -- if this bit is clear then a 0->1 transition of PENDING
   *     will cause an asynchronous upcall to be scheduled. This bit is only
   *     updated by the guest. It is read-only within Xen. If a channel
   *     becomes pending while the channel is masked then the 'edge' is lost
   *     (i.e., when the channel is unmasked, the guest must manually handle
   *     pending notifications as no upcall will be scheduled by Xen).
   *
   * To expedite scanning of pending notifications, any 0->1 pending
   * transition on an unmasked channel causes a corresponding bit in a
   * per-vcpu selector word to be set. Each bit in the selector covers a
   * 'C INTN' in the PENDING bitfield array.
   */
  xen_ulong_t                evtchn_pending[sizeof (xen_ulong_t) * 8];
  xen_ulong_t                evtchn_mask[sizeof (xen_ulong_t) * 8];

  /*
   * Wallclock time: updated only by control software. Guests should base
   * their gettimeofday() syscall on this wallclock-base value.
   */
  UINT32                     wc_version; /* Version counter: see vcpu_time_info_t. */
  UINT32                     wc_sec;     /* Secs  00:00:00 UTC, Jan 1, 1970.  */
  UINT32                     wc_nsec;    /* Nsecs 00:00:00 UTC, Jan 1, 1970.  */

  struct arch_shared_info    arch;
};

  #ifndef __XEN__
typedef struct shared_info shared_info_t;
typedef struct shared_info XEN_SHARED_INFO;
  #endif

/* Turn a plain number into a C UINTN constant. */
#define __mk_unsigned_long(x)  x ## UL
#define mk_unsigned_long(x)    __mk_unsigned_long(x)

__DEFINE_XEN_GUEST_HANDLE (uint8, UINT8);
__DEFINE_XEN_GUEST_HANDLE (uint16, UINT16);
__DEFINE_XEN_GUEST_HANDLE (uint32, UINT32);
__DEFINE_XEN_GUEST_HANDLE (uint64, UINT64);

#else /* __ASSEMBLY__ */

/* In assembly code we cannot use C numeric constant suffixes. */
#define mk_unsigned_long(x)  x

#endif /* !__ASSEMBLY__ */

#endif /* __XEN_PUBLIC_XEN_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
