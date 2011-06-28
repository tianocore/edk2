/** @file
  Establish the program environment and the "main" entry point.

  All of the global data in the gMD structure is initialized to 0, NULL, or
  SIG_DFL; as appropriate.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/UefiLib.h>

#include  <Library/ShellCEntryLib.h>
#include  <Library/MemoryAllocationLib.h>
#include  <Library/TimerLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <MainData.h>
#include  <sys/EfiSysCall.h>

extern int main( int, char**);
extern int __sse2_available;

struct  __MainData  *gMD;

/* Worker function to keep GCC happy. */
void __main()
{
  ;
}

static
void
FinalCleanup( void )
{
  int i;

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

/* Create mbcs versions of the Argv strings. */
static
char **
ArgvConvert(UINTN Argc, CHAR16 **Argv)
{
  size_t  AVsz;       /* Size of a single nArgv string */
  UINTN   count;
  char  **nArgv;
  char   *string;
  INTN    nArgvSize;  /* Cumulative size of narrow Argv[i] */

  nArgvSize = Argc;
  /* Determine space needed for narrow Argv strings. */
  for(count = 0; count < Argc; ++count) {
    AVsz = wcstombs(NULL, Argv[count], ARG_MAX);
    if(AVsz < 0) {
      Print(L"ABORTING: Argv[%d] contains an unconvertable character.\n", count);
      exit(EXIT_FAILURE);
      /* Not Reached */
    }
    nArgvSize += AVsz;
  }

  /* Reserve space for the converted strings. */
  gMD->NCmdLine = (char *)AllocateZeroPool(nArgvSize+1);
  if(gMD->NCmdLine == NULL) {
    Print(L"ABORTING: Insufficient memory.\n");
    exit(EXIT_FAILURE);
    /* Not Reached */
  }

  /* Convert Argument Strings. */
  nArgv   = gMD->NArgV;
  string  = gMD->NCmdLine;
  for(count = 0; count < Argc; ++count) {
    nArgv[count] = string;
    AVsz = wcstombs(string, Argv[count], nArgvSize);
    string[AVsz] = 0;   /* NULL terminate the argument */
    string += AVsz + 1;
    nArgvSize -= AVsz + 1;
    if(nArgvSize < 0) {
      Print(L"ABORTING: Internal Argv[%d] conversion error.\n", count);
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
  INTN   i;

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
    gMD->FinalCleanup     = &FinalCleanup;

#ifdef NT32dvm
    gMD->ClocksPerSecond  = 1;  // For NT32 only
    gMD->AppStartTime     = 1;  // For NT32 only
#else
    gMD->ClocksPerSecond = (clock_t)GetPerformanceCounterProperties( NULL, NULL);
    gMD->AppStartTime = (clock_t)GetPerformanceCounter();
#endif  /* NT32 dvm */

    // Initialize file descriptors
    mfd = gMD->fdarray;
    for(i = 0; i < (FOPEN_MAX); ++i) {
      mfd[i].MyFD = (UINT16)i;
    }

    i = open("stdin:", O_RDONLY, 0444);
    if(i == 0) {
      i = open("stdout:", O_WRONLY, 0222);
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
      ExitVal = (INTN)main( (int)Argc, nArgv);
  }
  }
  exit((int)ExitVal);
  /* Not Reached */
  return ExitVal;
}
