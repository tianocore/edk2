/** @file
    Machine dependent type definitions.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1990, 1993
    The Regents of the University of California.  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      - Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      - Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      - Neither the name of the University nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    NetBSD: types.h,v 1.14 2006/09/03 20:42:14 perry Exp
    types.h 7.5 (Berkeley) 3/9/91
**/
#ifndef _MACHTYPES_H_
#define _MACHTYPES_H_

#include  <sys/EfiCdefs.h>
#include  <machine/int_types.h>

typedef PHYSICAL_ADDRESS  paddr_t;
typedef UINT64            psize_t;
typedef PHYSICAL_ADDRESS  vaddr_t;
typedef UINT64            vsize_t;

typedef INTN    register_t;
typedef INT32   register32_t;

typedef volatile INT32    __cpu_simple_lock_t;

#define __SIMPLELOCK_LOCKED   1
#define __SIMPLELOCK_UNLOCKED 0

/* The amd64 does not have strict alignment requirements. */
#define __NO_STRICT_ALIGNMENT

#define __HAVE_DEVICE_REGISTER
#define __HAVE_CPU_COUNTER
#define __HAVE_SYSCALL_INTERN
#define __HAVE_MINIMAL_EMUL
#define __HAVE_GENERIC_SOFT_INTERRUPTS
#define __HAVE_CPU_MAXPROC
#define __HAVE_TIMECOUNTER
#define __HAVE_GENERIC_TODR

#endif  /* _MACHTYPES_H_ */
