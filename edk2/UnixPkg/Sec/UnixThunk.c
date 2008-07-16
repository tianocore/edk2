/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixThunk.c

Abstract:

  Since the SEC is the only program in our emulation we 
  must use a Tiano mechanism to export APIs to other modules.
  This is the role of the EFI_UNIX_THUNK_PROTOCOL.

  The mUnixThunkTable exists so that a change to EFI_UNIX_THUNK_PROTOCOL
  will cause an error in initializing the array if all the member functions
  are not added. It looks like adding a element to end and not initializing
  it may cause the table to be initaliized with the members at the end being
  set to zero. This is bad as jumping to zero will crash.
  

  gUnix is a a public exported global that contains the initialized
  data.

--*/

#include "SecMain.h"
#include "Library/UnixLib.h"

static int settimer_initialized;
static struct timeval settimer_timeval;
static void (*settimer_callback)(UINT64 delta);

static void
settimer_handler (int sig)
{
  struct timeval timeval;
  UINT64 delta;

  gettimeofday (&timeval, NULL);
  delta = ((UINT64)timeval.tv_sec * 1000) + (timeval.tv_usec / 1000)
    - ((UINT64)settimer_timeval.tv_sec * 1000) 
    - (settimer_timeval.tv_usec / 1000);
  settimer_timeval = timeval;
  if (settimer_callback)
    (*settimer_callback)(delta);
}

static
VOID
SetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs))
{
  struct itimerval timerval;
  UINT32 remainder;

  if (!settimer_initialized) {
    struct sigaction act;

    settimer_initialized = 1;
    act.sa_handler = settimer_handler;
    act.sa_flags = 0;
    sigemptyset (&act.sa_mask);
    if (sigaction (SIGALRM, &act, NULL) != 0) {
      printf ("SetTimer: sigaction error %s\n", strerror (errno));
    }
    if (gettimeofday (&settimer_timeval, NULL) != 0) {
      printf ("SetTimer: gettimeofday error %s\n", strerror (errno));
    }
  }
  timerval.it_value.tv_sec = DivU64x32(PeriodMs, 1000);
  DivU64x32Remainder(PeriodMs, 1000, &remainder);
  timerval.it_value.tv_usec = remainder * 1000;
  timerval.it_value.tv_sec = DivU64x32(PeriodMs, 1000);
  timerval.it_interval = timerval.it_value;
  
  if (setitimer (ITIMER_REAL, &timerval, NULL) != 0) {
    printf ("SetTimer: setitimer error %s\n", strerror (errno));
  }
  settimer_callback = CallBack;
}

void
msSleep (unsigned long Milliseconds)
{
  struct timespec ts;

  ts.tv_sec = Milliseconds / 1000;
  ts.tv_nsec = (Milliseconds % 1000) * 1000000;

  while (nanosleep (&ts, &ts) != 0 && errno == EINTR)
    ;
}

void
GetLocalTime (EFI_TIME *Time)
{
  struct tm *tm;
  time_t t;

  t = time (NULL);
  tm = localtime (&t);

  Time->Year = 1900 + tm->tm_year;
  Time->Month = tm->tm_mon;
  Time->Day = tm->tm_mday;
  Time->Hour = tm->tm_hour;
  Time->Minute = tm->tm_min;
  Time->Second = tm->tm_sec;
  Time->Nanosecond = 0;
  Time->TimeZone = timezone;
  Time->Daylight = (daylight ? EFI_TIME_ADJUST_DAYLIGHT : 0)
    | (tm->tm_isdst > 0 ? EFI_TIME_IN_DAYLIGHT : 0);
}

static void
TzSet (void)
{
  static int done = 0;
  if (!done) {
    tzset ();
    done = 1;
  }
}

long
GetTimeZone(void)
{
  TzSet ();
  return timezone;
}

int
GetDayLight(void)
{
  TzSet ();
  return daylight;
}

int
GetErrno(void)
{
  return errno;
}

extern EFI_STATUS
UgaCreate(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title);

EFI_UNIX_THUNK_PROTOCOL mUnixThunkTable = {
  EFI_UNIX_THUNK_PROTOCOL_SIGNATURE,
  msSleep, /* Sleep */
  exit, /* Exit */
  SetTimer,
  GetLocalTime,
  gmtime,
  GetTimeZone,
  GetDayLight,
  (UnixPoll)poll,
  (UnixRead)read,
  (UnixWrite)write,
  getenv,
  (UnixOpen)open,
  lseek,
  ftruncate,
  close,
  mkdir,
  rmdir,
  unlink,
  GetErrno,
  opendir,
  rewinddir,
  readdir,
  closedir,
  stat,
  statfs,
  rename,
  mktime,
  fsync,
  chmod,
  utime,
  tcflush,
  UgaCreate,
  perror,
  ioctl,
  fcntl,
  cfsetispeed,
  cfsetospeed,
  tcgetattr,
  tcsetattr
};


EFI_UNIX_THUNK_PROTOCOL *gUnix = &mUnixThunkTable;
