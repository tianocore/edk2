/** @file
  The header <stdarg.h> declares a type and defines three macros, for advancing
  through a list of arguments whose number and types are not known to the
  called function when it is translated.

  A function may be called with a variable number of arguments of varying types.
  Its parameter list contains one or more parameters.  The rightmost parameter
  plays a special role in the access mechanism, and will be designated paramN
  in this description.

  The type va_list is a type suitable for holding information needed by the
  macros va_start, va_arg, and va_end.  If access to the varying arguments
  is desired, the called function shall declare an object (referred to as ap
  in these descriptions) having type va_list.  The object ap may be passed as
  an argument to another function; if that function invokes the va_arg macro
  with parameter ap, the value of ap in the calling function is indeterminate
  and shall be passed to the va_end macro prior to any further reference to ap.

  The va_start and va_arg macros shall be implemented as macros, not as actual
  functions.  It is unspecified, by the C library standards, whether va_end
  is a macro or an identifier declared with external linkage.  If a macro
  definition is suppressed in order to access an actual function, or a
  program defines an external identifier with the name va_end, the behavior
  is undefined.  The va_start and va_end macros shall be invoked in the
  function accepting a varying number of arguments, if access to the varying
  arguments is desired.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _STDARG_H
#define _STDARG_H
#include  <sys/EfiCdefs.h>

/** The type va_list is a type suitable for holding information needed by the
    macros va_start, va_arg, and va_end.

    This implementation aliases va_list to VA_LIST, declared in MdePkg/Base.h.
**/
#define va_list   VA_LIST

/** The va_start macro shall be invoked before any access to the unnamed arguments.
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

    This implementation aliases va_start to VA_START, declared in MdePkg/Base.h.
**/
//#define va_start(ap, ParamN)    VA_START(ap, ParamN)
#define va_start    VA_START

/** The va_arg macro expands to an expression that has the type and value of
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

    This implementation aliases va_arg to VA_ARG, declared in MdePkg/Base.h.
**/
//#define va_arg(ap, type)        VA_ARG(ap, type)
#define va_arg        VA_ARG

/** The va_end macro facillitates a normal return from the function whose
    variable argument list was referred to by the expansion of va_start that
    initialized the va_list ap.

    Synopsys: void va_end(va_list ap);

    The va_end macro may modify ap so that it is no longer usable (without an
    intervening invocation of va_start).  If there is no corresponding
    invocation of the va_start macro, or if the va_end macro is not invoked
    before the return, the behavior is undefined.

    @param  ap    An object of type va_list, initialized by a prior
                  invocation of va_start, that will no longer be referenced.

    This implementation aliases va_end to VA_END, declared in MdePkg/Base.h.
**/
//#define va_end(ap)              VA_END(ap)
#define va_end              VA_END

/** For BSD compatibility. **/
#define va_copy(s,d)      (s) = (d)

#endif  /* _STDARG_H */
