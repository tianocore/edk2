/*++

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiDebug.h

Abstract:

  EFI Debug macros. The work needs tobe done in library. The Debug
  macros them selves are standard for all files, including the core.
  
  There needs to be code linked in that produces the following macros:
  
  EfiDebugAssert(file, linenumber, assertion string) - worker function for 
      ASSERT. filename and line number of where this ASSERT() is located
      is passed in along with the stringized version of the assertion.
  
  EfiDebugPrint - Worker function for debug print

  _DEBUG_SET_MEM(address, length, value) - Set memory at address to value
    for legnth bytes. This macro is used to initialzed uninitialized memory
    or memory that is free'ed, so it will not be used by mistake. 

--*/

#ifndef _EFI_DEBUG_H_
#define _EFI_DEBUG_H_

#ifdef EFI_DEBUG

  VOID
  EfiDebugAssert (
    IN CHAR8    *FileName,
    IN INTN     LineNumber,
    IN CHAR8    *Description
    );

  VOID
  EfiDebugPrint (
    IN  UINTN ErrorLevel,
    IN  CHAR8 *Format,
    ...
    );

  VOID
  EfiDebugVPrint (
    IN  UINTN   ErrorLevel,
    IN  CHAR8   *Format,
    IN  VA_LIST Marker
    );

  //
  // Define macros for the above functions so we can make them go away
  // in non-debug builds.
  //
  #define EFI_DEBUG_VPRINT(ErrorLevel, Format, Marker) \
                      EfiDebugVPrint(ErrorLevel, Format, Marker) 

  #define EFI_DEBUG_ASSERT(FileName, LineNumber, Description)  \
                      EfiDebugAssert (FileName, LineNumber, Description)

  #define _DEBUG_ASSERT(assertion)  \
            EfiDebugAssert (__FILE__, __LINE__, #assertion)

  #define _DEBUG(arg) DebugPrint arg

  //
  // Define ASSERT() macro, if assertion is FALSE trigger the ASSERT
  //
  #define ASSERT(assertion)   if(!(assertion))  \
                                _DEBUG_ASSERT(assertion)
    
  #define ASSERT_LOCKED(l)    if(!(l)->Lock) _DEBUG_ASSERT(l not locked)

  //
  // DEBUG((DebugLevel, "format string", ...)) - if DebugLevel is active do 
  //   the a debug print.
  //
  #define DEBUG(arg)        EfiDebugPrint arg

  #define DEBUG_CODE(code)  code

  #define CR(record, TYPE, field, signature)                    \
            _CR(record, TYPE, field)->Signature != signature ?  \
            (TYPE *) (_DEBUG_ASSERT("CR has Bad Signature"), record) :                        \
            _CR(record, TYPE, field)

  #define _DEBUG_SET_MEM(address, length, data) EfiCommonLibSetMem(address, length, data)

  //
  // Generate an ASSERT if the protocol specified by GUID is already installed on Handle.
  // If Handle is NULL, then an ASSERT is generated if the protocol specified by GUID 
  // is present anywhere in the handle database
  //
  #define ASSERT_PROTOCOL_ALREADY_INSTALLED(Handle, Guid)               \
    DEBUG_CODE ( {                                                      \
    VOID  *Instance;                                                    \
    if (Handle == NULL) {                                               \
      ASSERT(EFI_ERROR(gBS->LocateProtocol (Guid, NULL, &Instance)));   \
    } else {                                                            \
      ASSERT(EFI_ERROR(gBS->HandleProtocol (Handle, Guid, &Instance))); \
    } } ) 

#else
  #define ASSERT(a)               
  #define ASSERT_LOCKED(l)    
  #define DEBUG(arg) 
  #define DEBUG_CODE(code)  
  #define CR(Record, TYPE, Field, Signature)   \
            _CR(Record, TYPE, Field)                           
  #define _DEBUG_SET_MEM(address, length, data) 
  #define EFI_DEBUG_VPRINT(ErrorLevel, Format, Marker) 
  #define EFI_DEBUG_ASSERT(FileName, LineNumber, Description)  
  #define ASSERT_PROTOCOL_ALREADY_INSTALLED(Handle, Guid)
#endif

//
// Generate an ASSERT if Status is an error code
//
//#define ASSERT_EFI_ERROR(status)  ASSERT(!EFI_ERROR(status))
#define ASSERT_EFI_ERROR(status)  if (EFI_ERROR(status))                \
    DEBUG_CODE ( {                                                      \
      DEBUG((EFI_D_ERROR, "\nASSERT!Status = 0x%x Info :",status));     \
      ASSERT(!EFI_ERROR(status));                                \
    } ) 
    
#ifdef EFI_DEBUG_CLEAR_MEMORY
  #define DEBUG_SET_MEMORY(address,length)  \
            _DEBUG_SET_MEM(address, length, EFI_BAD_POINTER_AS_BYTE)
#else
  #define DEBUG_SET_MEMORY(address,length)
#endif

#define EFI_D_INIT        0x00000001          // Initialization style messages
#define EFI_D_WARN        0x00000002          // Warnings
#define EFI_D_LOAD        0x00000004          // Load events
#define EFI_D_FS          0x00000008          // EFI File system
#define EFI_D_POOL        0x00000010          // Alloc & Free's
#define EFI_D_PAGE        0x00000020          // Alloc & Free's
#define EFI_D_INFO        0x00000040          // Informational debug messages
#define EFI_D_VARIABLE    0x00000100          // Variable
#define EFI_D_BM          0x00000400          // Boot Manager (BDS)
#define EFI_D_BLKIO       0x00001000          // BlkIo Driver
#define EFI_D_NET         0x00004000          // SNI Driver
#define EFI_D_UNDI        0x00010000          // UNDI Driver
#define EFI_D_LOADFILE    0x00020000          // UNDI Driver
#define EFI_D_EVENT       0x00080000          // Event messages
#define EFI_D_VERBOSE     0x00400000          // Detailed debug messages that may significantly impact boot performance
#define EFI_D_ERROR       0x80000000          // Error

#define EFI_D_GENERIC (EFI_D_ERROR | EFI_D_INIT | EFI_D_WARN | EFI_D_INFO | \
                        EFI_D_BLKIO | EFI_D_NET | EFI_D_UNDI | EFI_D_VERBOSE)       

#define EFI_D_INTRINSIC ( EFI_D_EVENT | EFI_D_POOL | EFI_D_PAGE | \
                          EFI_D_BM | EFI_D_LOAD | EFI_D_VARIABLE )        

#define EFI_D_RESERVED  (EFI_D_GENERIC | EFI_D_INTRINSIC)       

#define EFI_DBUG_MASK   (EFI_D_ERROR | EFI_D_INFO | EFI_D_WARN)

#endif
