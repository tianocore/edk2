/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecMain.h"
#include "Gasket.h"

//
// OS X Posix does some strange name mangling on these names in C.
// If you call from assembler you get the wrong version of the function
// So these globals get you the correct name mangled functions that can
// be accessed from assembly
//  
extern UnixRmDir   gUnixRmDir;
extern UnixOpenDir gUnixOpenDir;
extern UnixStat    gUnixStat;
extern UnixStatFs  gUnixStatFs;

//
// Gasket functions for EFI_UNIX_THUNK_PROTOCOL
//

int 
Gasketrmdir (const char *pathname)
{
  return gUnixRmDir (pathname);
}


DIR *
Gasketopendir (const char *pathname)
{
  return gUnixOpenDir (pathname);
}


int 
Gasketstat (const char *path, STAT_FIX *buf)
{
  return gUnixStat (path, buf);
}


int 
Gasketstatfs (const char *path, struct statfs *buf)
{
  return gUnixStatFs (path, buf);
}

/////


void 
GasketmsSleep (unsigned long Milliseconds)
{ 
  msSleep (Milliseconds);
  return;
}

void 
Gasketexit (int status)
{
  exit (status);
  return;
}


void 
GasketSetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs))
{
  SetTimer (PeriodMs, CallBack);
  return;
}


void 
GasketGetLocalTime (EFI_TIME *Time)
{
  GetLocalTime (Time);
  return;
}


struct tm *
Gasketgmtime (const time_t *clock)
{
  return localtime (clock);
}


long 
GasketGetTimeZone (void)
{
  return  GetTimeZone ();
}


int 
GasketGetDayLight (void)
{
  return  GetDayLight ();
}


int 
Gasketpoll (struct pollfd *pfd, unsigned int nfds, int timeout)
{
  return poll (pfd, nfds, timeout);
}


long
Gasketread (int fd, void *buf, int count)
{
  return  read (fd, buf, count);
}


long
Gasketwrite (int fd, const void *buf, int count)
{
  return  write (fd, buf, count);
}


char *
Gasketgetenv (const char *name)
{
  return getenv (name);
}


int 
Gasketopen (const char *name, int flags, int mode)
{
  return open (name, flags, mode);
}


off_t 
Gasketlseek (int fd, off_t off, int whence)
{
  return lseek (fd, off, whence);
}


int 
Gasketftruncate (int fd, long int len)
{
  return ftruncate (fd, len);
}


int 
Gasketclose (int fd)
{
  return close (fd);
}


int 
Gasketmkdir (const char *pathname, mode_t mode)
{
  return mkdir (pathname, mode);
}


int 
Gasketunlink (const char *pathname)
{
  return unlink (pathname);
}


int 
GasketGetErrno (void)
{
  return GetErrno ();
}


void 
Gasketrewinddir (DIR *dir)
{
 rewinddir (dir);
  return;
}


struct dirent *
Gasketreaddir (DIR *dir)
{
  return readdir (dir);
}


int 
Gasketclosedir (DIR *dir)
{
  return closedir (dir);
}


int 
Gasketrename (const char *oldpath, const char *newpath)
{
  return rename (oldpath, newpath);
}


time_t 
Gasketmktime (struct tm *tm)
{
  return mktime (tm);
}


int 
Gasketfsync (int fd)
{
  return fsync (fd);
}


int 
Gasketchmod (const char *path, mode_t mode)
{
  return chmod (path, mode);
}


int 
Gasketutime (const char *filename, const struct utimbuf *buf)
{
  return utime (filename, buf);
}


int 
Gaskettcflush (int fildes, int queue_selector)
{
  return tcflush (fildes, queue_selector);
}


EFI_STATUS 
GasketUgaCreate (struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title)
{
  return UgaCreate (UgaIo, Title);
}


void 
Gasketperror (__const char *__s)
{
  perror (__s);
  return;
}



//
// ... is always an int or pointer to device specific data structure
//
int 
Gasketioctl (int fd, unsigned long int __request, ...)
{
  VA_LIST Marker;
  
  VA_START (Marker, __request);
  return ioctl (fd, __request, VA_ARG (Marker, UINTN));
}


int 
Gasketfcntl (int __fd, int __cmd, ...)
{
  VA_LIST Marker;
  
  VA_START (Marker, __cmd);
  return fcntl (__fd, __cmd, VA_ARG (Marker, UINTN));
}



int 
Gasketcfsetispeed (struct termios *__termios_p, speed_t __speed)
{
  return cfsetispeed (__termios_p, __speed);
}


int 
Gasketcfsetospeed (struct termios *__termios_p, speed_t __speed)
{
  return cfsetospeed (__termios_p, __speed);
}


int 
Gaskettcgetattr (int __fd, struct termios *__termios_p)
{
  return tcgetattr (__fd, __termios_p);
}

 
int 
Gaskettcsetattr (int __fd, int __optional_actions, __const struct termios *__termios_p)
{
  return tcsetattr (__fd, __optional_actions, __termios_p);
}




RETURN_STATUS
GasketUnixPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
{
  return SecPeCoffGetEntryPoint (Pe32Data, EntryPoint);
}



VOID
GasketUnixPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  SecPeCoffRelocateImageExtraAction (ImageContext);
  return;
}



VOID
GasketUnixPeCoffUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  SecPeCoffLoaderUnloadImageExtraAction (ImageContext);
  return;
}


//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//

EFI_STATUS 
EFIAPI 
GasketUgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  return UgaClose (UgaIo);
}

EFI_STATUS 
EFIAPI 
GasketUgaSize (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height)
{
  return UgaSize (UgaIo, Width, Height);
}

EFI_STATUS 
EFIAPI 
GasketUgaCheckKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  return UgaCheckKey (UgaIo);
}

EFI_STATUS 
EFIAPI 
GasketUgaGetKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_INPUT_KEY *key)
{
  return UgaGetKey (UgaIo, key);
}

EFI_STATUS 
EFIAPI 
GasketUgaBlt (
   EFI_UNIX_UGA_IO_PROTOCOL                    *UgaIo,
   IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
   IN  EFI_UGA_BLT_OPERATION                   BltOperation,
	 IN  UGA_BLT_ARGS                            *Args
   )
{
  return UgaBlt (UgaIo, BltBuffer, BltOperation, Args);
}

typedef void (*SET_TIMER_CALLBACK)(UINT64 delta);


UINTN 
ReverseGasketUint64 (SET_TIMER_CALLBACK settimer_callback, UINT64 a)
{
  (*settimer_callback)(a);
  return 0;
}

