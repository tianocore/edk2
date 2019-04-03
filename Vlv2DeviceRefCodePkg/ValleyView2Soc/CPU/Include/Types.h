/*++

Copyright (c) 1999  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent



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
