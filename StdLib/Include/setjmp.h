/** @file
    This file defines the macro setjmp, and declares the function longjmp
    and the type jmp_buf, for bypassing the normal function call and return discipline.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _SETJMP_H
#define _SETJMP_H
#include  <Library/BaseLib.h>
#include  <sys/EfiCdefs.h>

/** jmp_buf is an array type suitable for holding the information needed to
    restore a calling environment. The environment of a call to the setjmp
    macro consists of information sufficient for a call to the longjmp function
    to return execution to the correct block and invocation of that block, were
    it called recursively. It does not include the state of the floating-point
    status flags, of open files, or of any other component of the abstract
    machine.
**/
typedef BASE_LIBRARY_JUMP_BUFFER jmp_buf[1];

/** The setjmp macro saves its calling environment in its jmp_buf argument for
    later use by the longjmp function.

    The Standard does not specify whether setjmp is a macro or an identifier
    declared with external linkage. If a macro definition is suppressed in
    order to access an actual function, or a program defines an external
    identifier with the name setjmp, the behavior is undefined by the Standard.

    @param[in,out]  env   A jmp_buf type object into which
                          the current environment is stored.

    @return   If the return is from a direct invocation, the setjmp macro
    returns the value zero. If the return is from a call to the longjmp
    function, the setjmp macro returns a nonzero value based upon the value
    of the second argument to the longjmp function.
**/
#define setjmp(env)   (INTN)SetJump((env))

/** The longjmp function restores the environment saved by the most recent
    invocation of the setjmp macro in the same invocation of the program with
    the corresponding jmp_buf argument. If there has been no such invocation,
    or if the function containing the invocation of the setjmp macro has
    terminated execution in the interim, or if the invocation of the setjmp
    macro was within the scope of an identifier with variably modified type and
    execution has left that scope in the interim, the behavior is undefined.

    @param[in]    env     The jump buffer containing the environment to be returned to.
    @param[in]    val     A non-zero value to be returned from setjmp.

    @return     After longjmp is completed, program execution continues as if the
    corresponding invocation of the setjmp macro had just returned the value
    specified by val. The longjmp function cannot cause the setjmp macro to
    return the value 0; if val is 0, the setjmp macro returns the value 1.
**/
extern void longjmp(jmp_buf env, int val);

#endif  /* _SETJMP_H */
