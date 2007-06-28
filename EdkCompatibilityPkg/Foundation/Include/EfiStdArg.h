/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#define _EFI_INT_SIZE_OF(n) ((sizeof (n) + sizeof (UINTN) - 1) &~(sizeof (UINTN) - 1))

//
// Also support coding convention rules for var arg macros
//
#ifndef VA_START

typedef CHAR8 *VA_LIST;
#define VA_START(ap, v) (ap = (VA_LIST) & (v) + _EFI_INT_SIZE_OF (v))
#define VA_ARG(ap, t)   (*(t *) ((ap += _EFI_INT_SIZE_OF (t)) - _EFI_INT_SIZE_OF (t)))
#define VA_END(ap)      (ap = (VA_LIST) 0)

#endif

#endif
