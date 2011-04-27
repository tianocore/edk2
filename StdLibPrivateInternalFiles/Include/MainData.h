/** @file
  Global data for the program environment.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/ShellLib.h>

#include  <limits.h>
#include  <signal.h>
#include  <stdlib.h>
#include  <stdio.h>
#include  <time.h>
#include  "Efi/Console.h"

/* ##################  Type Declarations  ################################# */

/** The type of an atexit handler function. **/
typedef void            __xithandler_t(void);

/* ##################  Global Declarations  ############################### */
#ifndef TYPE_BIT
#define TYPE_BIT(type)  (sizeof (type) * CHAR_BIT)
#endif /* !defined TYPE_BIT */

#ifndef TYPE_SIGNED
#define TYPE_SIGNED(type) (((type) -1) < 0)
#endif /* !defined TYPE_SIGNED */

#ifndef INT_STRLEN_MAXIMUM
/*
** 302 / 1000 is log10(2.0) rounded up.
** Subtract one for the sign bit if the type is signed;
** add one for integer division truncation;
** add one more for a minus sign if the type is signed.
*/
#define INT_STRLEN_MAXIMUM(type) \
((TYPE_BIT(type) - TYPE_SIGNED(type)) * 302 / 1000 + 1 + TYPE_SIGNED(type))
#endif /* !defined INT_STRLEN_MAXIMUM */

/*
** Big enough for something such as
** ??? ???-2147483648 -2147483648:-2147483648:-2147483648 -2147483648\n
** (two three-character abbreviations, five strings denoting integers,
** three explicit spaces, two explicit colons, a newline,
** and a trailing ASCII nul).
*/
#define ASCTIME_BUFLEN  (3 * 2 + 5 * INT_STRLEN_MAXIMUM(int) + 3 + 2 + 1 + 1)

struct  __filedes {
  EFI_FILE_HANDLE   FileHandle;
  UINT32            State;        // In use if non-zero
  int               Oflags;       // From the open call
  int               Omode;        // From the open call
  int               RefCount;     // Reference count of opens
  int               SocProc;      // Placeholder: socket owner process or process group.
  UINT16            MyFD;         // Which FD this is.
};

struct  __MainData {
  // File descriptors
  struct __filedes  fdarray[OPEN_MAX];
  // Low-level File abstractions for the stdin, stdout, stderr streams
  ConInstance       StdIo[3];

  // Signal Handlers
  __sighandler_t    *sigarray[SIG_LAST];      // Pointers to signal handlers

  void (*cleanup)(void);   // Cleanup Function Pointer

  __xithandler_t   *atexit_handler[ATEXIT_MAX];  // Array of handlers for atexit.
  clock_t           AppStartTime;                // Set in Main.c and used for time.h
  clock_t           ClocksPerSecond;             // Set in Main.c and used for time.h
  int               num_atexit;                  ///< Number of registered atexit handlers.

  CHAR16            UString[UNICODE_STRING_MAX];
  struct tm         BDTime;                       // Broken-down time structure for localtime.
  EFI_TIME          TimeBuffer;                   // Used by <time.h>mk
  char              ASgetenv[ASCII_STRING_MAX];   // Only modified by getenv
  char              ASasctime[ASCTIME_BUFLEN];    // Only modified by asctime

  BOOLEAN         aborting;                       // Ensures cleanup function only called once when aborting.
};

extern struct  __MainData  *gMD;
