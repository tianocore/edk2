/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiStdArg.h

Abstract:

  Support for variable length argument lists using the ANSI standard.
  
  Since we are using the ANSI standard we used the standard nameing and
  did not folow the coding convention

  VA_LIST  - typedef for argument list.
  VA_START (VA_LIST Marker, argument before the ...) - Init Marker for use.
  VA_END (VA_LIST Marker) - Clear Marker
  VA_ARG (VA_LIST Marker, var arg size) - Use Marker to get an argumnet from
    the ... list. You must know the size and pass it in this macro.

  example:

  UINTN
  ExampleVarArg (
    IN UINTN  NumberOfArgs,
    ...
    )
  {
    VA_LIST Marker;
    UINTN   Index;
    UINTN   Result;

    //
    // Initialize the Marker
    //
    VA_START (Marker, NumberOfArgs);
    for (Index = 0, Result = 0; Index < NumberOfArgs; Index++) {
      //
      // The ... list is a series of UINTN values, so average them up.
      //
      Result += VA_ARG (Marker, UINTN);
    }

    VA_END (Marker);
    return Result
  }

--*/

#ifndef _EFISTDARG_H_
#define _EFISTDARG_H_

/**
  Return the size of argument that has been aligned to sizeof (UINTN).

  @param  n    The parameter size to be aligned.

  @return The aligned size.
**/
#define _INT_SIZE_OF(n) ((sizeof (n) + sizeof (UINTN) - 1) &~(sizeof (UINTN) - 1))

#if defined(__CC_ARM)
//
// RVCT ARM variable argument list support.
//

///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
#ifdef __APCS_ADSABI
  typedef int         *va_list[1];
  #define VA_LIST     va_list
#else
  typedef struct __va_list { void *__ap; } va_list;
  #define VA_LIST                          va_list
#endif

#define VA_START(Marker, Parameter)   __va_start(Marker, Parameter)

#define VA_ARG(Marker, TYPE)          __va_arg(Marker, TYPE)

#define VA_END(Marker)                ((void)0)

#define VA_COPY(Dest, Start)          __va_copy (Dest, Start)

#elif defined(__GNUC__) && !defined(NO_BUILTIN_VA_FUNCS)
//
// Use GCC built-in macros for variable argument lists.
//

///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
typedef __builtin_va_list VA_LIST;

#define VA_START(Marker, Parameter)  __builtin_va_start (Marker, Parameter)

#define VA_ARG(Marker, TYPE)         ((sizeof (TYPE) < sizeof (UINTN)) ? (TYPE)(__builtin_va_arg (Marker, UINTN)) : (TYPE)(__builtin_va_arg (Marker, TYPE)))

#define VA_END(Marker)               __builtin_va_end (Marker)

#define VA_COPY(Dest, Start)         __builtin_va_copy (Dest, Start)

#else

#ifndef VA_START

///
/// Variable used to traverse the list of arguments. This type can vary by 
/// implementation and could be an array or structure. 
///
typedef CHAR8 *VA_LIST;

/**
  Retrieves a pointer to the beginning of a variable argument list, based on 
  the name of the parameter that immediately precedes the variable argument list. 

  This function initializes Marker to point to the beginning of the variable  
  argument list that immediately follows Parameter.  The method for computing the 
  pointer to the next argument in the argument list is CPU-specific following the 
  EFIAPI ABI.

  @param   Marker       The VA_LIST used to traverse the list of arguments.
  @param   Parameter    The name of the parameter that immediately precedes 
                        the variable argument list.
  
  @return  A pointer to the beginning of a variable argument list.

**/
#define VA_START(Marker, Parameter) (Marker = (VA_LIST) ((UINTN) & (Parameter) + _INT_SIZE_OF (Parameter)))

/**
  Returns an argument of a specified type from a variable argument list and updates 
  the pointer to the variable argument list to point to the next argument. 

  This function returns an argument of the type specified by TYPE from the beginning 
  of the variable argument list specified by Marker.  Marker is then updated to point 
  to the next argument in the variable argument list.  The method for computing the 
  pointer to the next argument in the argument list is CPU-specific following the EFIAPI ABI.

  @param   Marker   VA_LIST used to traverse the list of arguments.
  @param   TYPE     The type of argument to retrieve from the beginning 
                    of the variable argument list.
  
  @return  An argument of the type specified by TYPE.

**/
#define VA_ARG(Marker, TYPE)   (*(TYPE *) ((Marker += _INT_SIZE_OF (TYPE)) - _INT_SIZE_OF (TYPE)))

/**
  Terminates the use of a variable argument list.

  This function initializes Marker so it can no longer be used with VA_ARG().  
  After this macro is used, the only way to access the variable argument list is 
  by using VA_START() again.

  @param   Marker   VA_LIST used to traverse the list of arguments.
  
**/
#define VA_END(Marker)      (Marker = (VA_LIST) 0)

#define VA_COPY(dest, src) ((void)((dest) = (src)))

#endif

#endif

#endif
