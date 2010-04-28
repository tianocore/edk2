/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Exceptions.c

Abstract:

  Exception logging routines.

--*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset()
#include "Exceptions.h"

//
// Max length of a saved exception message
//
#define MAX_EXCEPTION_MSG 200

//
// We use this structure to track exceptions thrown. We nest deeper on
// TryException() calls, and come back out on CatchException() calls.
// We save off the first exception message for a given exception level,
// but we save the count of how many were thrown.
//
typedef struct {
  int   ExceptionCount;
  char  ExceptionMsg[MAX_EXCEPTION_MSG];
} EXCEPTION_LOG;

static EXCEPTION_LOG  ExceptionLog[MAX_EXCEPTION_NESTING + 1];
static int            ExceptionLevel;

//
// Initialize our data and structures for tracking exceptions.
//
int
InitExceptions (
  VOID
  )
{
  ExceptionLevel = -1;
  memset ((char *) &ExceptionLog, 0, sizeof (ExceptionLog));
  return 0;
}
//
// This function replaces the _try() exception macro. It sets the
// nesting level.
//
int
TryException (
  VOID
  )
{
  //
  // Boost our exception level if we would not go out of range
  //
  ExceptionLevel++;
  if (ExceptionLevel >= MAX_EXCEPTION_NESTING) {
    fprintf (stderr, "ERROR: Max exception nesting level exceeded\n");
    ExceptionLevel--;
    return 1;
  }

  return 0;
}
//
// This function replaces the _catch() exception macro. It's used to decrement
// the nesting level and return any exeption error messages that were
// thrown at the current nesting level.
//
char *
CatchException (
  VOID
  )
{
  //
  // Return a pointer to exception message. NULL if no exceptions at this level
  //
  if (ExceptionLevel >= 0) {
    ExceptionLevel--;
    if (ExceptionLog[ExceptionLevel + 1].ExceptionMsg[0]) {
      return ExceptionLog[ExceptionLevel + 1].ExceptionMsg;
    } else {
      return NULL;
    }
  } else {
    fprintf (stderr, "ERROR: Invalid nesting level call to CatchException()\n");
    return NULL;
  }
}
//
// This function can be used to test for exceptions between the TryException()
// and CatchException() calls in a given function.
//
int
ExceptionThrown (
  VOID
  )
{
  return ExceptionLog[ExceptionLevel].ExceptionCount;
}
//
// This function replaces the _throw() exception macro. It saves off the
// given error message at the current exeption level nesting.
//
int
ThrowException (
  char *Msg
  )
{
  if (ExceptionLevel < 0) {
    //
    // fprintf (stderr, "ERROR: Exception thrown out of scope");
    // Haven't yet enabled handling of exceptions, so just emit the message.
    //
    fprintf (stderr, Msg);
    return 1;
  }
  //
  // Only log the first
  //
  if (ExceptionLog[ExceptionLevel].ExceptionMsg[0] == 0) {
    strncpy (ExceptionLog[ExceptionLevel].ExceptionMsg, Msg, MAX_EXCEPTION_MSG);
  }

  ExceptionLog[ExceptionLevel].ExceptionCount++;
  return 0;
}
