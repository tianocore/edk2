/** @file
  Establish the program environment and the "main" entry point.

  All of the global data in the gMD structure is initialized to 0, NULL, or
  SIG_DFL; as appropriate.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/DebugLib.h>

#include  <Library/ShellCEntryLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/TimerLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <time.h>
#include  <MainData.h>
#include  <unistd.h>

extern int main( int, char**);
extern int __sse2_available;

struct  __MainData  *gMD;

/* Worker function to keep GCC happy. */
void __main()
{
  ;
}

/** Clean up data as required by the exit() function.

**/
void
exitCleanup(INTN ExitVal)
{
  void (*CleanUp)(void);   // Pointer to Cleanup Function
  int i;

  if(gMD != NULL) {
    gMD->ExitValue = (int)ExitVal;
    CleanUp = gMD->cleanup; // Preserve the pointer to the Cleanup Function

    // Call all registered atexit functions in reverse order
    i = gMD->num_atexit;
    if( i > 0) {
      do {
        (gMD->atexit_handler[--i])();
      } while( i > 0);
  }

    if (CleanUp != NULL) {
      CleanUp();
    }
  }
}

/* Create mbcs versions of the Argv strings. */
static
char **
ArgvConvert(UINTN Argc, CHAR16 **Argv)
{
  ssize_t  AVsz;       /* Size of a single nArgv string, or -1 */
  UINTN   count;
  char  **nArgv;
  char   *string;
  INTN    nArgvSize;  /* Cumulative size of narrow Argv[i] */

DEBUG_CODE_BEGIN();
  DEBUG((DEBUG_INIT, "ArgvConvert called with %d arguments.\n", Argc));
  for(count = 0; count < ((Argc > 5)? 5: Argc); ++count) {
    DEBUG((DEBUG_INIT, "Argument[%d] = \"%s\".\n", count, Argv[count]));
  }
DEBUG_CODE_END();

  nArgvSize = Argc;
  /* Determine space needed for narrow Argv strings. */
  for(count = 0; count < Argc; ++count) {
    AVsz = (ssize_t)wcstombs(NULL, Argv[count], ARG_MAX);
    if(AVsz < 0) {
      DEBUG((DEBUG_ERROR, "ABORTING: Argv[%d] contains an unconvertable character.\n", count));
      exit(EXIT_FAILURE);
      /* Not Reached */
    }
    nArgvSize += AVsz;
  }

  /* Reserve space for the converted strings. */
  gMD->NCmdLine = (char *)AllocateZeroPool(nArgvSize+1);
  if(gMD->NCmdLine == NULL) {
    DEBUG((DEBUG_ERROR, "ABORTING: Insufficient memory.\n"));
    exit(EXIT_FAILURE);
    /* Not Reached */
  }

  /* Convert Argument Strings. */
  nArgv   = gMD->NArgV;
  string  = gMD->NCmdLine;
  for(count = 0; count < Argc; ++count) {
    nArgv[count] = string;
    AVsz = wcstombs(string, Argv[count], nArgvSize) + 1;
    DEBUG((DEBUG_INFO, "Cvt[%d] %d \"%s\" --> \"%a\"\n", (INT32)count, (INT32)AVsz, Argv[count], nArgv[count]));
    string += AVsz;
    nArgvSize -= AVsz;
    if(nArgvSize < 0) {
      DEBUG((DEBUG_ERROR, "ABORTING: Internal Argv[%d] conversion error.\n", count));
      exit(EXIT_FAILURE);
      /* Not Reached */
    }
  }
  return gMD->NArgV;
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  struct __filedes   *mfd;
  char              **nArgv;
  INTN   ExitVal;
  int                 i;

  ExitVal = (INTN)RETURN_SUCCESS;
  gMD = AllocateZeroPool(sizeof(struct __MainData));
  if( gMD == NULL ) {
    ExitVal = (INTN)RETURN_OUT_OF_RESOURCES;
  }
  else {
    /* Initialize data */
    __sse2_available      = 0;
    _fltused              = 1;
    errno                 = 0;
    EFIerrno              = 0;

    gMD->ClocksPerSecond  = 1;
    gMD->AppStartTime     = (clock_t)((UINT32)time(NULL));

    // Initialize file descriptors
    mfd = gMD->fdarray;
    for(i = 0; i < (FOPEN_MAX); ++i) {
      mfd[i].MyFD = (UINT16)i;
    }

    DEBUG((DEBUG_INIT, "StdLib: Open Standard IO.\n"));
    i = open("stdin:", (O_RDONLY | O_TTY_INIT), 0444);
    if(i == 0) {
      i = open("stdout:", (O_WRONLY | O_TTY_INIT), 0222);
      if(i == 1) {
        i = open("stderr:", O_WRONLY, 0222);
      }
    }
    if(i != 2) {
      Print(L"ERROR Initializing Standard IO: %a.\n    %r\n",
            strerror(errno), EFIerrno);
    }

    /* Create mbcs versions of the Argv strings. */
    nArgv = ArgvConvert(Argc, Argv);
    if(nArgv == NULL) {
      ExitVal = (INTN)RETURN_INVALID_PARAMETER;
    }
    else {
      if( setjmp(gMD->MainExit) == 0) {
        errno   = 0;    // Clean up any "scratch" values from startup.
        ExitVal = (INTN)main( (int)Argc, gMD->NArgV);
        exitCleanup(ExitVal);
      }
      /* You reach here if:
          * normal return from main()
          * call to _Exit(), either directly or through exit().
      */
      ExitVal = (INTN)gMD->ExitValue;
    }

    if( ExitVal == EXIT_FAILURE) {
      ExitVal = RETURN_ABORTED;
    }

    /* Close any open files */
    for(i = OPEN_MAX - 1; i >= 0; --i) {
      (void)close(i);   // Close properly handles closing a closed file.
    }

    /* Free the global MainData structure */
    if(gMD != NULL) {
      if(gMD->NCmdLine != NULL) {
        FreePool( gMD->NCmdLine );
      }
      FreePool( gMD );
  }
  }
  return ExitVal;
}
