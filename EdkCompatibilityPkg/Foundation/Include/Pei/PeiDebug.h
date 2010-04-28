/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PeiDebug.h

Abstract:

  PEI Debug macros. The work needs to be done in library. The Debug
  macros them selves are standard for all files, including the core.
  
  There needs to be code linked in that produces the following macros:
  
  PeiDebugAssert(file, linenumber, assertion string) - worker function for 
      ASSERT. filename and line number of where this ASSERT() is located
      is passed in along with the stringized version of the assertion.
  
  PeiDebugPrint - Worker function for debug print

  _DEBUG_SET_MEM(address, length, value) - Set memory at address to value
    for legnth bytes. This macro is used to initialzed uninitialized memory
    or memory that is free'ed, so it will not be used by mistake. 

--*/

#ifndef _PEIDEBUG_H_
#define _PEIDEBUG_H_

#ifdef EFI_DEBUG

  VOID
  PeiDebugAssert (
    IN CONST EFI_PEI_SERVICES   **PeiServices,
    IN CHAR8              *FileName,
    IN INTN               LineNumber,
    IN CHAR8              *Description
    );

  VOID
  PeiDebugPrint (
    IN CONST EFI_PEI_SERVICES   **PeiServices,
    IN UINTN              ErrorLevel,
    IN CHAR8              *Format,
    ...
    );

  #define _PEI_DEBUG_ASSERT(PeiST, assertion)  \
            PeiDebugAssert (PeiST, __FILE__, __LINE__, #assertion)

  #define _PEI_DEBUG(PeiST, arg) PeiDebugPrint (PeiST, arg)

  //
  // Define ASSERT() macro, if assertion is FALSE trigger the ASSERT
  //
  #define PEI_ASSERT(PeiST, assertion)   if(!(assertion))  \
                                            _PEI_DEBUG_ASSERT(PeiST, assertion)
    
  #define PEI_ASSERT_LOCKED(PeiST, l)    if(!(l)->Lock) _PEI_DEBUG_ASSERT(PeiST, l not locked)

  //
  // DEBUG((DebugLevel, "format string", ...)) - if DebugLevel is active do 
  //   the a debug print.
  //
  
  #define PEI_DEBUG(arg)        PeiDebugPrint arg

  #define PEI_DEBUG_CODE(code)  code

  #define PEI_CR(Record, TYPE, Field, Signature)   \
            _CR(Record, TYPE, Field)                           


  #define _PEI_DEBUG_SET_MEM(address, length, data) SetMem(address, length, data)

#else
  #define PEI_ASSERT(PeiST, a)               
  #define PEI_ASSERT_LOCKED(PeiST, l)    
  #define PEI_DEBUG(arg) 
  #define PEI_DEBUG_CODE(code)  
  #define PEI_CR(Record, TYPE, Field, Signature)   \
            _CR(Record, TYPE, Field)                           
  #define _PEI_DEBUG_SET_MEM(address, length, data) 
#endif

#define ASSERT_PEI_ERROR(PeiST, status)  PEI_ASSERT(PeiST, !EFI_ERROR(status))

#ifdef EFI_DEBUG_CLEAR_MEMORY
  #define PEI_DEBUG_SET_MEMORY(address,length)  \
            _PEI_DEBUG_SET_MEM(address, length, EFI_BAD_POINTER_AS_BYTE)
#else
  #define PEI_DEBUG_SET_MEMORY(address,length)
#endif


#endif
