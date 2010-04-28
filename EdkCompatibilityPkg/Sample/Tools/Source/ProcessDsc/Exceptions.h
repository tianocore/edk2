/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  Exceptions.h
  
Abstract:

  Defines and function prototypes for the ProcessDsc utility.
  
--*/

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#define VOID void
#define MAX_EXCEPTION_NESTING 4

//
// Function prototypes
//
int
InitExceptions (
  VOID
  );
int
TryException (
  VOID
  );
char  *
CatchException (
  VOID
  );
int
ExceptionThrown (
  VOID
  );
int
ThrowException (
  char *EMsg
  );

#endif // ifndef _EXCEPTIONS_H_
