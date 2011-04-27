/**
Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _MACHINE_SIGNAL_H
#define _MACHINE_SIGNAL_H
#include <sys/EfiCdefs.h>

/** The type sig_atomic_t is the (possibly volatile-qualified) integer type of
    an object that can be accessed as an atomic entity, even in the presence
    of asynchronous interrupts.
**/
typedef INTN sig_atomic_t;

#endif    /* _MACHINE_SIGNAL_H */
