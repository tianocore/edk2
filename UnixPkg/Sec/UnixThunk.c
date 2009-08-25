/*++

Copyright (c) 2004 - 2009, Intel Corporation                                                         
Portions copyright (c) 2008-2009 Apple Inc. All rights reserved.
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
#include "Uefi.h"
#include "Library/UnixLib.h"

int settimer_initialized;
struct timeval settimer_timeval;
void (*settimer_callback)(UINT64 delta);

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
  if (settimer_callback)
    (*settimer_callback)(delta);
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
  Time->Month = tm->tm_mon + 1;
  Time->Day = tm->tm_mday;
  Time->Hour = tm->tm_hour;
  Time->Minute = tm->tm_min;
  Time->Second = tm->tm_sec;
  Time->Nanosecond = 0;
  Time->TimeZone = timezone;
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

#if __APPLE__
void GasketmsSleep (unsigned long Milliseconds);
void Gasketexit (int status);
void GasketSetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs));
void GasketGetLocalTime (EFI_TIME *Time);
struct tm *Gasketgmtime (const time_t *clock);
long GasketGetTimeZone (void);
int GasketGetDayLight (void);
int Gasketpoll (struct pollfd *pfd, int nfds, int timeout);
int Gasketread (int fd, void *buf, int count);
int Gasketwrite (int fd, const void *buf, int count);
char *Gasketgetenv (const char *name);
int Gasketopen (const char *name, int flags, int mode);
off_t Gasketlseek (int fd, off_t off, int whence);
int Gasketftruncate (int fd, long int len);
int Gasketclose (int fd);
int Gasketmkdir (const char *pathname, mode_t mode);
int Gasketrmdir (const char *pathname);
int Gasketunlink (const char *pathname);
int GasketGetErrno (void);
DIR *Gasketopendir (const char *pathname);
void *Gasketrewinddir (DIR *dir);
struct dirent *Gasketreaddir (DIR *dir);
int Gasketclosedir (DIR *dir);
int Gasketstat (const char *path, struct stat *buf);
int Gasketstatfs (const char *path, struct statfs *buf);
int Gasketrename (const char *oldpath, const char *newpath);
time_t Gasketmktime (struct tm *tm);
int Gasketfsync (int fd);
int Gasketchmod (const char *path, mode_t mode);
int Gasketutime (const char *filename, const struct utimbuf *buf);
int Gaskettcflush (int fildes, int queue_selector);
EFI_STATUS GasketUgaCreate(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title);
void Gasketperror (__const char *__s);

//
// ... is always an int or pointer to device specific data structure
//
int Gasketioctl (int fd, unsigned long int __request, ...);
int Gasketfcntl (int __fd, int __cmd, ...);

int Gasketcfsetispeed (struct termios *__termios_p, speed_t __speed);
int Gasketcfsetospeed (struct termios *__termios_p, speed_t __speed);
int Gaskettcgetattr (int __fd, struct termios *__termios_p); 
int Gaskettcsetattr (int __fd, int __optional_actions, __const struct termios *__termios_p);
int Gasketsigaction (int sig, const struct sigaction *act, struct sigaction *oact);
int Gasketsetcontext (const ucontext_t *ucp);
int Gasketgetcontext (ucontext_t *ucp);
int Gasketsigemptyset (sigset_t *set);
int Gasketsigaltstack (const stack_t *ss, stack_t *oss);

RETURN_STATUS
GasketUnixPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

VOID
GasketUnixPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
GasketPeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

#endif

extern EFI_STATUS
UgaCreate(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title);

EFI_UNIX_THUNK_PROTOCOL mUnixThunkTable = {
  EFI_UNIX_THUNK_PROTOCOL_SIGNATURE,
#ifdef __APPLE__
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
  (UnixPoll)Gasketpoll,
  (UnixRead)Gasketread,
  (UnixWrite)Gasketwrite,
  Gasketgetenv,
  (UnixOpen)Gasketopen,
  (UnixSeek)Gasketlseek,
  (UnixFtruncate)Gasketftruncate,
  Gasketclose,
  Gasketmkdir,
  Gasketrmdir,
  Gasketunlink,
  GasketGetErrno,
  Gasketopendir,
  (UnixRewindDir)Gasketrewinddir,
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
  
  dlopen,  // Update me with a gasket
  dlerror, // Update me with a gasket
  dlsym,   // Update me with a gasket

  SecPeCoffGetEntryPoint,                // Update me with a gasket
  SecPeCoffRelocateImageExtraAction,     // Update me with a gasket
  SecPeCoffLoaderUnloadImageExtraAction  // Update me with a gasket

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
  tcsetattr,
  dlopen,
  dlerror,
  dlsym,
  SecPeCoffGetEntryPoint,
  SecPeCoffRelocateImageExtraAction,
  SecPeCoffLoaderUnloadImageExtraAction
#endif
};


EFI_UNIX_THUNK_PROTOCOL *gUnix = &mUnixThunkTable;
