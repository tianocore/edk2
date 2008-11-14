/** @file
  Provides services to print debug and assert messages to a debug output device.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __DEBUG_LIB_H__
#define __DEBUG_LIB_H__

//
// Declare bits for PcdDebugPropertyMask
//
#define DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED       0x01
#define DEBUG_PROPERTY_DEBUG_PRINT_ENABLED        0x02
#define DEBUG_PROPERTY_DEBUG_CODE_ENABLED         0x04
#define DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED       0x08
#define DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED  0x10
#define DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED    0x20

//
// Declare bits for PcdDebugPrintErrorLevel and the ErrorLevel parameter of DebugPrint()
//
#define DEBUG_INIT      0x00000001  // Initialization
#define DEBUG_WARN      0x00000002  // Warnings
#define DEBUG_LOAD      0x00000004  // Load events
#define DEBUG_FS        0x00000008  // EFI File system
#define DEBUG_POOL      0x00000010  // Alloc & Free's
#define DEBUG_PAGE      0x00000020  // Alloc & Free's
#define DEBUG_INFO      0x00000040  // Verbose
#define DEBUG_DISPATCH  0x00000080  // PEI/DXE Dispatchers
#define DEBUG_VARIABLE  0x00000100  // Variable
#define DEBUG_BM        0x00000400  // Boot Manager
#define DEBUG_BLKIO     0x00001000  // BlkIo Driver
#define DEBUG_NET       0x00004000  // SNI Driver
#define DEBUG_UNDI      0x00010000  // UNDI Driver
#define DEBUG_LOADFILE  0x00020000  // UNDI Driver
#define DEBUG_EVENT     0x00080000  // Event messages
#define DEBUG_ERROR     0x80000000  // Error

//
// Aliases of debug message mask bits
//
#define EFI_D_INIT      DEBUG_INIT
#define EFI_D_WARN      DEBUG_WARN
#define EFI_D_LOAD      DEBUG_LOAD
#define EFI_D_FS        DEBUG_FS
#define EFI_D_POOL      DEBUG_POOL
#define EFI_D_PAGE      DEBUG_PAGE
#define EFI_D_INFO      DEBUG_INFO
#define EFI_D_DISPATCH  DEBUG_DISPATCH
#define EFI_D_VARIABLE  DEBUG_VARIABLE
#define EFI_D_BM        DEBUG_BM
#define EFI_D_BLKIO     DEBUG_BLKIO
#define EFI_D_NET       DEBUG_NET
#define EFI_D_UNDI      DEBUG_UNDI
#define EFI_D_LOADFILE  DEBUG_LOADFILE
#define EFI_D_EVENT     DEBUG_EVENT
#define EFI_D_ERROR     DEBUG_ERROR

/**

  Prints a debug message to the debug output device if the specified error level is enabled.

  If any bit in ErrorLevel is also set in PcdDebugPrintErrorLevel, then print 
  the message specified by Format and the associated variable argument list to 
  the debug output device.

  If Format is NULL, then ASSERT().

  @param  ErrorLevel  The error level of the debug message.
  @param  Format      Format string for the debug message to print.
  @param  ...         The variable argument list.

**/
VOID
EFIAPI
DebugPrint (
  IN  UINTN        ErrorLevel,
  IN  CONST CHAR8  *Format,
  ...
  );


/**

  Prints an assert message containing a filename, line number, and description.  
  This may be followed by a breakpoint or a dead loop.

  Print a message of the form "ASSERT <FileName>(<LineNumber>): <Description>\n"
  to the debug output device.  If DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED bit of 
  PcdDebugProperyMask is set then CpuBreakpoint() is called. Otherwise, if 
  DEBUG_PROPERTY_ASSERT_DEADLOOP_ENABLED bit of PcdDebugProperyMask is set then 
  CpuDeadLoop() is called.  If neither of these bits are set, then this function 
  returns immediately after the message is printed to the debug output device.
  DebugAssert() must actively prevent recursion.  If DebugAssert() is called while
  processing another DebugAssert(), then DebugAssert() must return immediately.

  If FileName is NULL, then a <FileName> string of "(NULL) Filename" is printed.

  If Description is NULL, then a <Description> string of "(NULL) Description" is printed.

  @param  FileName     Pointer to the name of the source file that generated the assert condition.
  @param  LineNumber   The line number in the source file that generated the assert condition
  @param  Description  Pointer to the description of the assert condition.

**/
VOID
EFIAPI
DebugAssert (
  IN CONST CHAR8  *FileName,
  IN UINTN        LineNumber,
  IN CONST CHAR8  *Description
  );


/**

  Fills a target buffer with PcdDebugClearMemoryValue, and returns the target buffer.

  This function fills Length bytes of Buffer with the value specified by 
  PcdDebugClearMemoryValue, and returns Buffer.

  If Buffer is NULL, then ASSERT().

  If Length is greater than (MAX_ADDRESS -Buffer + 1), then ASSERT(). 

  @param   Buffer  Pointer to the target buffer to be filled with PcdDebugClearMemoryValue.
  @param   Length  Number of bytes in Buffer to fill with zeros PcdDebugClearMemoryValue. 

  @return  Buffer  Pointer to the target buffer filled with PcdDebugClearMemoryValue.

**/
VOID *
EFIAPI
DebugClearMemory (
  OUT VOID  *Buffer,
  IN UINTN  Length
  );


/**
  
  Returns TRUE if ASSERT() macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugAssertEnabled (
  VOID
  );


/**
  
  Returns TRUE if DEBUG()macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugPrintEnabled (
  VOID
  );


/**
  
  Returns TRUE if DEBUG_CODE()macros are enabled.

  This function returns TRUE if the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugCodeEnabled (
  VOID
  );


/**
  
  Returns TRUE if DEBUG_CLEAR_MEMORY()macro is enabled.

  This function returns TRUE if the DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of 
  PcdDebugProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE    The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is set.
  @retval  FALSE   The DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is clear.

**/
BOOLEAN
EFIAPI
DebugClearMemoryEnabled (
  VOID
  );


/**
  
  Internal worker macro that calls DebugAssert().

  This macro calls DebugAssert() passing in the filename, line number, and 
  expression that evailated to FALSE.

  @param  Expression  Boolean expression that evailated to FALSE

**/
#define _ASSERT(Expression)  DebugAssert (__FILE__, __LINE__, #Expression)


/**
  
  Internal worker macro that calls DebugPrint().

  This macro calls DebugPrint() passing in the debug error level, a format 
  string, and a variable argument list.

  @param  Expression  Expression containing an error level, a format string, 
                      and a variable argument list based on the format string.

**/
#define _DEBUG(Expression)   DebugPrint Expression


/**
  
  Macro that calls DebugAssert() if a expression evaluates to FALSE.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro evaluates the Boolean expression specified by Expression.  If 
  Expression evaluates to FALSE, then DebugAssert() is called passing in the 
  source filename, source line number, and Expression.

  @param  Expression  Boolean expression

**/
#define ASSERT(Expression)        \
  do {                            \
    if (DebugAssertEnabled ()) {  \
      if (!(Expression)) {        \
        _ASSERT (Expression);     \
      }                           \
    }                             \
  } while (FALSE)


/**
  
  Macro that calls DebugPrint().

  If the DEBUG_PROPERTY_DEBUG_PRINT_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro passes Expression to DebugPrint().

  @param  Expression  Expression containing an error level, a format string, 
                      and a variable argument list based on the format string.
  

**/
#define DEBUG(Expression)        \
  do {                           \
    if (DebugPrintEnabled ()) {  \
      _DEBUG (Expression);       \
    }                            \
  } while (FALSE)


/**
  
  Macro that calls DebugAssert() if an EFI_STATUS evaluates to an error code.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro evaluates the EFI_STATUS value specified by StatusParameter.  
  If StatusParameter is an error code, then DebugAssert() is called passing in 
  the source filename, source line number, and StatusParameter.

  @param  StatusParameter  EFI_STATUS value to evaluate.

**/
#define ASSERT_EFI_ERROR(StatusParameter)                                              \
  do {                                                                                 \
    if (DebugAssertEnabled ()) {                                                       \
      if (EFI_ERROR (StatusParameter)) {                                               \
        DEBUG ((EFI_D_ERROR, "\nASSERT_EFI_ERROR (Status = %r)\n", StatusParameter));  \
        _ASSERT (!EFI_ERROR (StatusParameter));                                        \
      }                                                                                \
    }                                                                                  \
  } while (FALSE)


/**
  
  Macro that calls DebugAssert() if a protocol is already installed in the 
  handle database.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear, 
  then return.

  If Handle is NULL, then a check is made to see if the protocol specified by Guid 
  is present on any handle in the handle database.  If Handle is not NULL, then 
  a check is made to see if the protocol specified by Guid is present on the 
  handle specified by Handle.  If the check finds the protocol, then DebugAssert() 
  is called passing in the source filename, source line number, and Guid.

  If Guid is NULL, then ASSERT().

  @param  Handle  The handle to check for the protocol.  This is an optional 
                  parameter that may be NULL.  If it is NULL, then the entire 
                  handle database is searched.

  @param  Guid    Pointer to a protocol GUID.

**/
#define ASSERT_PROTOCOL_ALREADY_INSTALLED(Handle, Guid)                               \
  do {                                                                                \
    if (DebugAssertEnabled ()) {                                                      \
      VOID  *Instance;                                                                \
      ASSERT (Guid != NULL);                                                          \
      if (Handle == NULL) {                                                           \
        if (!EFI_ERROR (gBS->LocateProtocol ((EFI_GUID *)Guid, NULL, &Instance))) {   \
          _ASSERT (Guid already installed in database);                               \
        }                                                                             \
      } else {                                                                        \
        if (!EFI_ERROR (gBS->HandleProtocol (Handle, (EFI_GUID *)Guid, &Instance))) { \
          _ASSERT (Guid already installed on Handle);                                 \
        }                                                                             \
      }                                                                               \
    }                                                                                 \
  } while (FALSE)


/**
  Macro that marks the beginning of debug source code.

  If the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro marks the beginning of source code that is included in a module.
  Otherwise, the source lines between DEBUG_CODE_BEGIN() and DEBUG_CODE_END() 
  are not included in a module.

**/
#define DEBUG_CODE_BEGIN()  do { if (DebugCodeEnabled ()) { UINT8  __DebugCodeLocal


/**
  
  Macro that marks the end of debug source code.

  If the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro marks the end of source code that is included in a module.  
  Otherwise, the source lines between DEBUG_CODE_BEGIN() and DEBUG_CODE_END() 
  are not included in a module.

**/
#define DEBUG_CODE_END()    __DebugCodeLocal = 0; __DebugCodeLocal++; } } while (FALSE)


/**
  
  Macro that declares a section of debug source code.

  If the DEBUG_PROPERTY_DEBUG_CODE_ENABLED bit of PcdDebugProperyMask is set, 
  then the source code specified by Expression is included in a module.  
  Otherwise, the source specified by Expression is not included in a module.

**/
#define DEBUG_CODE(Expression)  \
  DEBUG_CODE_BEGIN ();          \
  Expression                    \
  DEBUG_CODE_END ()


/**
  
  Macro that calls DebugClearMemory() to clear a buffer to a default value.

  If the DEBUG_PROPERTY_CLEAR_MEMORY_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro calls DebugClearMemory() passing in Address and Length.

  @param  Address  Pointer to a buffer.
  @param  Length   The number of bytes in the buffer to set.

**/
#define DEBUG_CLEAR_MEMORY(Address, Length)  \
  do {                                       \
    if (DebugClearMemoryEnabled ()) {        \
      DebugClearMemory (Address, Length);    \
    }                                        \
  } while (FALSE)


/**

  Macro that calls DebugAssert() if the containing record does not have a 
  matching signature.  If the signatures matches, then a pointer to the data 
  structure that contains a specified field of that data structure is returned.  
  This is a light weight method hide information by placing a public data 
  structure inside a larger private data structure and using a pointer to the 
  public data structure to retrieve a pointer to the private data structure.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is clear, 
  then this macro computes the offset, in bytes, of field specified by Field 
  from the beginning of the  data structure specified by TYPE.  This offset is 
  subtracted from Record, and is used to return a pointer to a data structure 
  of the type specified by TYPE.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set,  
  then this macro computes the offset, in bytes, of field specified by Field from 
  the beginning of the data structure specified by TYPE.  This offset is 
  subtracted from Record, and is used to compute a pointer to a data structure of 
  the type specified by TYPE.  The Signature field of the data structure specified 
  by TYPE is compared to TestSignature.  If the signatures match, then a pointer 
  to the pointer to a data structure of the type specified by TYPE is returned.  
  If the signatures do not match, then DebugAssert() is called with a description 
  of "CR has a bad signature" and Record is returned.  

  If the data type specified by TYPE does not contain the field specified by Field, 
  then the module will not compile.

  If TYPE does not contain a field called Signature, then the module will not 
  compile.

  @param  Record         Pointer to the field specified by Field within a data 
                         structure of type TYPE.

  @param  TYPE           The name of the data structure type to return  This 
                         data structure must contain the field specified by Field. 

  @param  Field          The name of the field in the data structure specified 
                         by TYPE to which Record points.

  @param  TestSignature  The 32-bit signature value to match.

**/
#define CR(Record, TYPE, Field, TestSignature)                                          \
  (DebugAssertEnabled () && (_CR (Record, TYPE, Field)->Signature != TestSignature)) ?  \
  (TYPE *) (_ASSERT (CR has Bad Signature), Record) :                                   \
  _CR (Record, TYPE, Field)
    
#endif
