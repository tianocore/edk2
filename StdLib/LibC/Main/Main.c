/** @file
  Establish the program environment and the "main" entry point.

  All of the global data in the gMD structure is initialized to 0, NULL, or
  SIG_DFL; as appropriate.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
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
#include  <string.h>
#include  <MainData.h>

extern int main( int, wchar_t**);
extern int __sse2_available;

struct  __MainData  *gMD;

/* Worker function to keep GCC happy. */
void __main()
{
  ;
}

INTN
EFIAPI
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  INTN   ExitVal;
  INTN   i;
  struct __filedes *mfd;
  FILE  *fp;

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

#ifdef NT32dvm
    gMD->ClocksPerSecond  = 0;  // For NT32 only
    gMD->AppStartTime     = 0;  // For NT32 only
#else
    gMD->ClocksPerSecond = (clock_t)GetPerformanceCounterProperties( NULL, NULL);
    gMD->AppStartTime = (clock_t)GetPerformanceCounter();
#endif  /* NT32 dvm */

    // Initialize file descriptors
    mfd = gMD->fdarray;
    for(i = 0; i < (FOPEN_MAX); ++i) {
      mfd[i].MyFD = (UINT16)i;
    }

    // Open stdin, stdout, stderr
    fp = freopen("stdin:", "r", stdin);
    if(fp != NULL) {
      fp = freopen("stdout:", "w", stdout);
      if(fp != NULL) {
        fp = freopen("stderr:", "w", stderr);
      }
    }
    if(fp == NULL) {
      Print(L"ERROR Initializing Standard IO: %a.\n    %r\n",
            strerror(errno), EFIerrno);
    }

    ExitVal = (INTN)main( (int)Argc, (wchar_t **)Argv);

    if (gMD->cleanup != NULL) {
      gMD->cleanup();
    }
  }
  if(gMD != NULL) {
    FreePool( gMD );
  }
  return ExitVal;
}
