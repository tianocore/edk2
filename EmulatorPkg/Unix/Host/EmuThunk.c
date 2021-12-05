/*++ @file
  Since the SEC is the only program in our emulation we
  must use a UEFI/PI mechanism to export APIs to other modules.
  This is the role of the EFI_EMU_THUNK_PROTOCOL.

  The mUnixThunkTable exists so that a change to EFI_EMU_THUNK_PROTOCOL
  will cause an error in initializing the array if all the member functions
  are not added. It looks like adding a element to end and not initializing
  it may cause the table to be initialized with the members at the end being
  set to zero. This is bad as jumping to zero will crash.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2011, Apple Inc. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Host.h"

#ifdef __APPLE__
#define DebugAssert  _Mangle__DebugAssert

  #include <assert.h>
  #include <CoreServices/CoreServices.h>
  #include <mach/mach.h>
  #include <mach/mach_time.h>

  #undef DebugAssert
#endif

int             settimer_initialized;
struct timeval  settimer_timeval;
UINTN           settimer_callback = 0;

BOOLEAN  gEmulatorInterruptEnabled = FALSE;

UINTN
SecWriteStdErr (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  ssize_t  Return;

  Return = write (STDERR_FILENO, (const void *)Buffer, (size_t)NumberOfBytes);

  return (Return == -1) ? 0 : Return;
}

EFI_STATUS
SecConfigStdIn (
  VOID
  )
{
  struct termios  tty;

  //
  // Need to turn off line buffering, ECHO, and make it unbuffered.
  //
  tcgetattr (STDIN_FILENO, &tty);
  tty.c_lflag &= ~(ICANON | ECHO);
  tcsetattr (STDIN_FILENO, TCSANOW, &tty);

  //  setvbuf (STDIN_FILENO, NULL, _IONBF, 0);

  // now ioctl FIONREAD will do what we need
  return EFI_SUCCESS;
}

UINTN
SecWriteStdOut (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  ssize_t  Return;

  Return = write (STDOUT_FILENO, (const void *)Buffer, (size_t)NumberOfBytes);

  return (Return == -1) ? 0 : Return;
}

UINTN
SecReadStdIn (
  IN UINT8  *Buffer,
  IN UINTN  NumberOfBytes
  )
{
  ssize_t  Return;

  Return = read (STDIN_FILENO, Buffer, (size_t)NumberOfBytes);

  return (Return == -1) ? 0 : Return;
}

BOOLEAN
SecPollStdIn (
  VOID
  )
{
  int  Result;
  int  Bytes;

  Result = ioctl (STDIN_FILENO, FIONREAD, &Bytes);
  if (Result == -1) {
    return FALSE;
  }

  return (BOOLEAN)(Bytes > 0);
}

VOID *
SecMalloc (
  IN  UINTN  Size
  )
{
  return malloc ((size_t)Size);
}

VOID *
SecValloc (
  IN  UINTN  Size
  )
{
  return valloc ((size_t)Size);
}

BOOLEAN
SecFree (
  IN  VOID  *Ptr
  )
{
  if (EfiSystemMemoryRange (Ptr)) {
    // If an address range is in the EFI memory map it was alloced via EFI.
    // So don't free those ranges and let the caller know.
    return FALSE;
  }

  free (Ptr);
  return TRUE;
}

void
settimer_handler (
  int  sig
  )
{
  struct timeval  timeval;
  UINT64          delta;

  gettimeofday (&timeval, NULL);
  delta = ((UINT64)timeval.tv_sec * 1000) + (timeval.tv_usec / 1000)
          - ((UINT64)settimer_timeval.tv_sec * 1000)
          - (settimer_timeval.tv_usec / 1000);
  settimer_timeval = timeval;

  if (settimer_callback) {
    ReverseGasketUint64 (settimer_callback, delta);
  }
}

VOID
SecSetTimer (
  IN  UINT64                  PeriodMs,
  IN  EMU_SET_TIMER_CALLBACK  CallBack
  )
{
  struct itimerval  timerval;
  UINT32            remainder;

  if (!settimer_initialized) {
    struct sigaction  act;

    settimer_initialized = 1;
    act.sa_handler       = settimer_handler;
    act.sa_flags         = 0;
    sigemptyset (&act.sa_mask);
    gEmulatorInterruptEnabled = TRUE;
    if (sigaction (SIGALRM, &act, NULL) != 0) {
      printf ("SetTimer: sigaction error %s\n", strerror (errno));
    }

    if (gettimeofday (&settimer_timeval, NULL) != 0) {
      printf ("SetTimer: gettimeofday error %s\n", strerror (errno));
    }
  }

  timerval.it_value.tv_sec = DivU64x32 (PeriodMs, 1000);
  DivU64x32Remainder (PeriodMs, 1000, &remainder);
  timerval.it_value.tv_usec = remainder * 1000;
  timerval.it_value.tv_sec  = DivU64x32 (PeriodMs, 1000);
  timerval.it_interval      = timerval.it_value;

  if (setitimer (ITIMER_REAL, &timerval, NULL) != 0) {
    printf ("SetTimer: setitimer error %s\n", strerror (errno));
  }

  settimer_callback = (UINTN)CallBack;
}

VOID
SecEnableInterrupt (
  VOID
  )
{
  sigset_t  sigset;

  gEmulatorInterruptEnabled = TRUE;
  // Since SetTimer() uses SIGALRM we emulate turning on and off interrupts
  // by enabling/disabling SIGALRM.
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGALRM);
  pthread_sigmask (SIG_UNBLOCK, &sigset, NULL);
}

VOID
SecDisableInterrupt (
  VOID
  )
{
  sigset_t  sigset;

  // Since SetTimer() uses SIGALRM we emulate turning on and off interrupts
  // by enabling/disabling SIGALRM.
  sigemptyset (&sigset);
  sigaddset (&sigset, SIGALRM);
  pthread_sigmask (SIG_BLOCK, &sigset, NULL);
  gEmulatorInterruptEnabled = FALSE;
}

BOOLEAN
SecInterruptEanbled (
  void
  )
{
  return gEmulatorInterruptEnabled;
}

UINT64
QueryPerformanceFrequency (
  VOID
  )
{
  // Hard code to nanoseconds
  return 1000000000ULL;
}

UINT64
QueryPerformanceCounter (
  VOID
  )
{
 #if __APPLE__
  UINT64                            Start;
  static mach_timebase_info_data_t  sTimebaseInfo;

  Start = mach_absolute_time ();

  // Convert to nanoseconds.

  // If this is the first time we've run, get the timebase.
  // We can use denom == 0 to indicate that sTimebaseInfo is
  // uninitialised because it makes no sense to have a zero
  // denominator is a fraction.

  if ( sTimebaseInfo.denom == 0 ) {
    (void)mach_timebase_info (&sTimebaseInfo);
  }

  // Do the maths. We hope that the multiplication doesn't
  // overflow; the price you pay for working in fixed point.

  return (Start * sTimebaseInfo.numer) / sTimebaseInfo.denom;
 #else
  // Need to figure out what to do for Linux?
  return 0;
 #endif
}

VOID
SecSleep (
  IN  UINT64  Nanoseconds
  )
{
  struct timespec  rq, rm;
  struct timeval   start, end;
  unsigned long    MicroSec;

  rq.tv_sec  = DivU64x32 (Nanoseconds, 1000000000);
  rq.tv_nsec = ModU64x32 (Nanoseconds, 1000000000);

  //
  // nanosleep gets interrupted by our timer tic.
  // we need to track wall clock time or we will stall for way too long
  //
  gettimeofday (&start, NULL);
  end.tv_sec  = start.tv_sec + rq.tv_sec;
  MicroSec    = (start.tv_usec + rq.tv_nsec/1000);
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
    }

    if ((start.tv_sec == end.tv_sec) && (start.tv_usec > end.tv_usec)) {
      break;
    }

    rq = rm;
  }
}

VOID
SecCpuSleep (
  VOID
  )
{
  struct timespec  rq, rm;

  // nanosleep gets interrupted by the timer tic
  rq.tv_sec  = 1;
  rq.tv_nsec = 0;

  nanosleep (&rq, &rm);
}

VOID
SecExit (
  UINTN  Status
  )
{
  exit (Status);
}

VOID
SecGetTime (
  OUT  EFI_TIME              *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL
  )
{
  struct tm  *tm;
  time_t     t;

  t  = time (NULL);
  tm = localtime (&t);

  Time->Year       = 1900 + tm->tm_year;
  Time->Month      = tm->tm_mon + 1;
  Time->Day        = tm->tm_mday;
  Time->Hour       = tm->tm_hour;
  Time->Minute     = tm->tm_min;
  Time->Second     = tm->tm_sec;
  Time->Nanosecond = 0;
  Time->TimeZone   = timezone / 60;
  Time->Daylight   = (daylight ? EFI_TIME_ADJUST_DAYLIGHT : 0)
                     | (tm->tm_isdst > 0 ? EFI_TIME_IN_DAYLIGHT : 0);

  if (Capabilities != NULL) {
    Capabilities->Resolution = 1;
    Capabilities->Accuracy   = 50000000;
    Capabilities->SetsToZero = FALSE;
  }
}

VOID
SecSetTime (
  IN  EFI_TIME  *Time
  )
{
  // Don't change the time on the system
  // We could save delta to localtime() and have SecGetTime adjust return values?
  return;
}

EFI_STATUS
SecGetNextProtocol (
  IN  BOOLEAN                EmuBusDriver,
  OUT EMU_IO_THUNK_PROTOCOL  **Instance   OPTIONAL
  )
{
  return GetNextThunkProtocol (EmuBusDriver, Instance);
}

EMU_THUNK_PROTOCOL  gEmuThunkProtocol = {
  GasketSecWriteStdErr,
  GasketSecConfigStdIn,
  GasketSecWriteStdOut,
  GasketSecReadStdIn,
  GasketSecPollStdIn,
  GasketSecMalloc,
  GasketSecValloc,
  GasketSecFree,
  GasketSecPeCoffGetEntryPoint,
  GasketSecPeCoffRelocateImageExtraAction,
  GasketSecPeCoffUnloadImageExtraAction,
  GasketSecEnableInterrupt,
  GasketSecDisableInterrupt,
  GasketQueryPerformanceFrequency,
  GasketQueryPerformanceCounter,
  GasketSecSleep,
  GasketSecCpuSleep,
  GasketSecExit,
  GasketSecGetTime,
  GasketSecSetTime,
  GasketSecSetTimer,
  GasketSecGetNextProtocol
};

VOID
SecInitThunkProtocol (
  VOID
  )
{
  // timezone and daylight lib globals depend on tzset be called 1st.
  tzset ();
}
