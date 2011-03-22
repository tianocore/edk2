/*++

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials                          
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
#include "Uefi.h"
#include "Library/UnixLib.h"

#if defined(__APPLE__) || defined(MDE_CPU_X64)
#include "Gasket.h"
#endif

int settimer_initialized;
struct timeval settimer_timeval;
void (*settimer_callback)(UINT64 delta);

BOOLEAN gEmulatorInterruptEnabled = FALSE;


void
settimer_handler (int sig)
{
  struct timeval timeval;
  UINT64 delta;

  gettimeofday (&timeval, NULL);
  delta = ((UINT64)timeval.tv_sec * 1000) + (timeval.tv_usec / 1000)
    - ((UINT64)settimer_timeval.tv_sec * 1000) 
    - (settimer_timeval.tv_usec / 1000);
  settimer_timeval = timeval;
  
  if (settimer_callback) {
#if defined(__APPLE__) || defined(MDE_CPU_X64)
   ReverseGasketUint64 (settimer_callback, delta);
#else
    (*settimer_callback)(delta);
#endif
  }
}

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
    gEmulatorInterruptEnabled = TRUE;
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
UnixEnableInterrupt (void)
{
  sigset_t  sigset;

  gEmulatorInterruptEnabled = TRUE;
  // Since SetTimer() uses SIGALRM we emulate turning on and off interrupts 
  // by enabling/disabling SIGALRM.
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGALRM);
  sigprocmask (SIG_UNBLOCK, &sigset, NULL);
}


void
UnixDisableInterrupt (void)
{
  sigset_t  sigset;

  // Since SetTimer() uses SIGALRM we emulate turning on and off interrupts 
  // by enabling/disabling SIGALRM.
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGALRM);
  sigprocmask (SIG_BLOCK, &sigset, NULL);
  gEmulatorInterruptEnabled = FALSE;
}


BOOLEAN
UnixInterruptEanbled (void)
{
  return gEmulatorInterruptEnabled;
}



void
msSleep (unsigned long Milliseconds)
{
  struct timespec rq, rm;
  struct timeval  start, end;
  unsigned long  MicroSec;
  
  rq.tv_sec = Milliseconds / 1000;
  rq.tv_nsec = (Milliseconds % 1000) * 1000000;

  //
  // nanosleep gets interrupted by our timer tic. 
  // we need to track wall clock time or we will stall for way too long
  //
  gettimeofday (&start, NULL);
  end.tv_sec  = start.tv_sec + rq.tv_sec;
  MicroSec = (start.tv_usec + rq.tv_nsec/1000);
  end.tv_usec = MicroSec % 1000000;
  if (MicroSec > 1000000) {
    end.tv_sec++;
  }

  while (nanosleep (&rq, &rm) == -1) {
    if (errno != EINTR) {
      break;
    }
    gettimeofday (&start, NULL);
    if (start.tv_sec > end.tv_sec) {
      break;
    } if ((start.tv_sec == end.tv_sec) && (start.tv_usec > end.tv_usec)) {
      break;
    }
    rq = rm;
  } 
}

void
GetLocalTime (EFI_TIME *Time)
{
  struct tm *tm;
  time_t t;

  t = time (NULL);
  tm = localtime (&t);

  Time->Year = 1900 + tm->tm_year;
  Time->Month = tm->tm_mon + 1;
  Time->Day = tm->tm_mday;
  Time->Hour = tm->tm_hour;
  Time->Minute = tm->tm_min;
  Time->Second = tm->tm_sec;
  Time->Nanosecond = 0;
  Time->TimeZone = GetTimeZone ();
  Time->Daylight = (daylight ? EFI_TIME_ADJUST_DAYLIGHT : 0)
    | (tm->tm_isdst > 0 ? EFI_TIME_IN_DAYLIGHT : 0);
}

void
TzSet (void)
{
  STATIC int done = 0;
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
#if defined(__APPLE__) || defined(MDE_CPU_X64)
//
// Mac OS X requires the stack to be 16-byte aligned for IA-32. So on an OS X build
// we add an assembly wrapper that makes sure the stack ges aligned. 
// This has the nice benfit of being able to run EFI ABI code, like the EFI shell
// that is checked in to source control in the OS X version of the emulator
//
  GasketmsSleep, /* Sleep */
  Gasketexit, /* Exit */
  GasketSetTimer,
  GasketGetLocalTime,
  Gasketgmtime,
  GasketGetTimeZone,
  GasketGetDayLight,
  Gasketpoll,
  Gasketread,
  Gasketwrite,
  Gasketgetenv,
  Gasketopen,
  Gasketlseek,
  Gasketftruncate,
  Gasketclose,
  Gasketmkdir,
  Gasketrmdir,
  Gasketunlink,
  GasketGetErrno,
  Gasketopendir,
  Gasketrewinddir,
  Gasketreaddir,
  Gasketclosedir,
  Gasketstat,
  Gasketstatfs,
  Gasketrename,
  Gasketmktime,
  Gasketfsync,
  Gasketchmod,
  Gasketutime,
  Gaskettcflush,
  GasketUgaCreate,
  Gasketperror,
  Gasketioctl,
  Gasketfcntl,
  Gasketcfsetispeed,
  Gasketcfsetospeed,
  Gaskettcgetattr,
  Gaskettcsetattr,
  GasketUnixPeCoffGetEntryPoint,                
  GasketUnixPeCoffRelocateImageExtraAction,     
  GasketUnixPeCoffUnloadImageExtraAction,  
  
  GasketUnixEnableInterrupt,
  GasketUnixDisableInterrupt,

  Gasketgetifaddrs,
  Gasketfreeifaddrs,
  Gasketsocket,

#else
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
  (UnixSeek)lseek,
  (UnixFtruncate)ftruncate,
  close,
  mkdir,
  rmdir,
  unlink,
  GetErrno,
  opendir,
  rewinddir,
  readdir,
  closedir,
  (UnixStat)stat,
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
  tcsetattr,
  SecPeCoffGetEntryPoint,
  SecPeCoffRelocateImageExtraAction,
  SecPeCoffLoaderUnloadImageExtraAction,
  UnixEnableInterrupt,
  UnixDisableInterrupt,
  getifaddrs,
  freeifaddrs,
  socket
#endif
};


EFI_UNIX_THUNK_PROTOCOL *gUnix = &mUnixThunkTable;
