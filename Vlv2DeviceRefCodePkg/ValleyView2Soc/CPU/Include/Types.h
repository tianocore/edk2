/*++

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.



Module Name:

    Types.h

Abstract:

    This file include all the external data types.

--*/

#ifndef _TYPES_H_
#define _TYPES_H_



//
// Modifiers to abstract standard types to aid in debug of problems
//
#define CONST     const
#define STATIC    static
#define VOID      void
#define VOLATILE  volatile

//
// Constants. They may exist in other build structures, so #ifndef them.
//
#ifndef TRUE
#define TRUE  ((BOOLEAN) 1 == 1)
#endif

#ifndef FALSE
#define FALSE ((BOOLEAN) 0 == 1)
#endif

#ifndef NULL
#define NULL  ((VOID *) 0)
#endif

typedef UINT32 STATUS;
#define SUCCESS 0
#define FAILURE 0xFFFFFFFF

#ifndef MRC_DEADLOOP
#define MRC_DEADLOOP()    while (TRUE)
#endif

#endif
