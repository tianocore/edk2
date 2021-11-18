/******************************************************************************
 * protocols.h
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __XEN_PROTOCOLS_H__
#define __XEN_PROTOCOLS_H__

#define XEN_IO_PROTO_ABI_X86_32  "x86_32-abi"
#define XEN_IO_PROTO_ABI_X86_64  "x86_64-abi"
#define XEN_IO_PROTO_ABI_ARM     "arm-abi"

#if defined (MDE_CPU_IA32)
#define XEN_IO_PROTO_ABI_NATIVE  XEN_IO_PROTO_ABI_X86_32
#elif defined (MDE_CPU_X64)
#define XEN_IO_PROTO_ABI_NATIVE  XEN_IO_PROTO_ABI_X86_64
#elif defined (__arm__) || defined (__aarch64__)
#define XEN_IO_PROTO_ABI_NATIVE  XEN_IO_PROTO_ABI_ARM
#else
  #error arch fixup needed here
#endif

#endif
