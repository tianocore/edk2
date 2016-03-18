/** @file
    This header, <stdarg.h>, declares type va_list and defines macros: va_start, va_arg, va_end;
    for advancing through a list of arguments whose number and types are not known to the
    called function when it is translated.

    A function may be called with a variable number of arguments of varying types.
    The rightmost argument plays a special role in the access mechanism, and will
    be designated paramN in this and subsequent descriptions.

    The type va_list is a type suitable for holding information needed by the
    macros va_start, va_arg, and va_end.  If access to the varying arguments
    is desired, the called function shall declare an object (referred to as ap
    in these descriptions) having type va_list.  The object ap may be passed as
    an argument to another function; if the receiving function invokes the va_arg macro
    with parameter ap, the value of ap in the calling function becomes indeterminate
    and must be passed to the va_end macro prior to any further reference to ap.

    The va_start and va_arg macros must be implemented as macros, not as actual
    functions.  The va_start and va_end macros must be invoked in the
    function accepting a varying number of arguments, if access to the varying
    arguments is desired.

    Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _STDARG_H
#define _STDARG_H
#include  <sys/EfiCdefs.h>

/** @{
    The type va_list is a type suitable for holding information needed by the
    macros va_start, va_arg, and va_end.

    Depending upon compiler or CPU architecture, different definitions are required.
**/
#if defined(__GNUC__)
typedef __builtin_va_list   va_list;
#else
#define va_list   VA_LIST
#endif
/*@}*/

/** @{
    The va_start macro must be invoked before any access to the unnamed arguments.
    The va_start macro initializes ap for subsequent use by va_arg and va_end.

    Synopsys: void va_start(va_list ap, paramN);

    @param  ap      An object of type va_list that is to be initialized such
                    that subsequent successive invocations of va_arg will
                    return the values of the parameters following paramN.

    @param  paramN  The parameter paramN is the identifier of the rightmost
                    parameter in the variable parameter list in the function
                    definition (the one just before the ,...).  If the
                    parameter parmN is declared with the register storage
                    class, with a function of array type, or with a type that
                    is not compatible with the type that results after
                    application of the default argument promotions, the
                    behavior is undefined.
**/
#if defined(__GNUC__)
#define va_start    __builtin_va_start
#else
#define va_start    VA_START
#endif
/*@}*/

/** @{
    The va_arg macro expands to an expression that has the type and value of
    the next argument in the call.  The parameter ap shall be the same as the
    va_list ap initialized by va_start.  Each invocation of va_arg modifies ap
    so that the values of successive arguments are returned in turn.  The
    parameter type is a type name specified such that the type of a pointer to
    an object that has the specified type can be obtained simply by postfixing
    a * to type.  If there is no actual next argument, or if type is not
    compatible with the type of the actual next argument (as promoted
    according to the default argument promotions), the behavior is undefined.

    Synopsys: type va_arg(va_list ap, type);

    @param  ap    An object of type va_list that was initialized by a prior
                  invocation of va_start.

    @param  type  A type name specifying the type of the parameter to be retrieved.

    @return       The first invocation of the va_arg macro after that of the
                  va_start macro returns the value of the argument after that
                  specified by paramN.  Successive invocations return the values
                  of the remaining arguments in succession.
**/
#if defined(__GNUC__)
#define va_arg        __builtin_va_arg
#else
#define va_arg        VA_ARG
#endif
/*@}*/

/** @{
    The va_end macro facillitates a normal return from the function whose
    variable argument list was referred to by the expansion of va_start that
    initialized the va_list ap.

    Synopsys: void va_end(va_list ap);

    The va_end macro may modify ap so that it is no longer usable (without an
    intervening invocation of va_start).  If there is no corresponding
    invocation of the va_start macro, or if the va_end macro is not invoked
    before the return, the behavior is undefined.

    @param  ap    An object of type va_list, initialized by a prior
                  invocation of va_start, that will no longer be referenced.
**/
#if defined(__GNUC__)
#define va_end              __builtin_va_end
#else
#define va_end              VA_END
#endif
/*@}*/

/** @{
    For BSD compatibility.
**/
#if defined(__GNUC__)
#define va_copy         __builtin_va_copy
#else
#define va_copy(s,d)      (s) = (d)
#endif
/*@}*/

/** Provide a generic version of the compiler-dependent __va_copy macro.
    Some software, such as Python 2.7.2, relies upon the existence of __va_copy.
    If this macro is not defined, it just assigns one va_list item to another.
    This breaks for compiler environments that define va_list as an array or structure.
**/
#ifndef __va_copy
  #define __va_copy       va_copy
#endif

#endif  /* _STDARG_H */
