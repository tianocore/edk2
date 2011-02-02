/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifdef __APPLE__

#include "SecMain.h"
#include "Gasket.h"

//
// Gasket functions for EFI_UNIX_THUNK_PROTOCOL
//

void 
GasketmsSleep (unsigned long Milliseconds)
{ 
  GasketUintn (msSleep, Milliseconds);
  return;
}

void 
Gasketexit (int status)
{
 GasketUintn (exit, status);
  return;
}


void 
GasketSetTimer (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs))
{
 GasketUint64Uintn (SetTimer, PeriodMs, (UINTN)CallBack);
  return;
}


void 
GasketGetLocalTime (EFI_TIME *Time)
{
  GasketUintn (GetLocalTime, (UINTN)Time);
  return;
}


struct tm *
Gasketgmtime (const time_t *clock)
{
  return (struct tm *)(UINTN)GasketUintn (localtime, (UINTN)clock);
}


long 
GasketGetTimeZone (void)
{
  return  GasketVoid (GetTimeZone);
}


int 
GasketGetDayLight (void)
{
  return  GasketVoid (GetDayLight);
}


int 
Gasketpoll (struct pollfd *pfd, unsigned int nfds, int timeout)
{
  return GasketUintnUintnUintn (poll, (UINTN)pfd, nfds, timeout);
}


long
Gasketread (int fd, void *buf, int count)
{
  return  GasketUintnUintnUintn (read, fd, (UINTN)buf, count);
}


long
Gasketwrite (int fd, const void *buf, int count)
{
  return  GasketUintnUintnUintn (write, fd, (UINTN)buf, count);
}


char *
Gasketgetenv (const char *name)
{
  return (char *)(UINTN)GasketUintn (getenv, (UINTN)name);
}


int 
Gasketopen (const char *name, int flags, int mode)
{
  return  GasketUintnUintnUintn (open, (UINTN)name, flags, mode);
}


off_t 
Gasketlseek (int fd, off_t off, int whence)
{
  if (sizeof off == 8) {
    return GasketUintnUint64Uintn (lseek, fd, off, whence);
  } else if (sizeof off == 4) {
    return GasketUintnUintnUintn (lseek, fd, off, whence);
  }
}


int 
Gasketftruncate (int fd, long int len)
{
  return GasketUintnUintn (ftruncate, fd, len);
}


int 
Gasketclose (int fd)
{
  return GasketUintn (close, fd);
}


int 
Gasketmkdir (const char *pathname, mode_t mode)
{
  return GasketUintnUint16 (mkdir, (UINTN)pathname, mode);
}


int 
Gasketrmdir (const char *pathname)
{
  return GasketUintn (rmdir, (UINTN)pathname);
}


int 
Gasketunlink (const char *pathname)
{
  return GasketUintn (unlink, (UINTN)pathname);
}


int 
GasketGetErrno (void)
{
  return  GasketVoid (GetErrno);
}


DIR *
Gasketopendir (const char *pathname)
{
  return (DIR *)(UINTN)GasketUintn (opendir, (UINTN)pathname);
}


void 
Gasketrewinddir (DIR *dir)
{
  GasketUintn (rewinddir, (UINTN)dir);
  return;
}


struct dirent *
Gasketreaddir (DIR *dir)
{
  return (struct dirent *)(UINTN)GasketUintn (readdir, (UINTN)dir);
}


int 
Gasketclosedir (DIR *dir)
{
  return GasketUintn (closedir,  (UINTN)dir);
}


int 
Gasketstat (const char *path, STAT_FIX *buf)
{
  return GasketUintnUintn (stat, (UINTN)path, (UINTN)buf);
}


int 
Gasketstatfs (const char *path, struct statfs *buf)
{
  return GasketUintnUintn (statfs, (UINTN)path, (UINTN)buf);
}


int 
Gasketrename (const char *oldpath, const char *newpath)
{
  return GasketUintnUintn (rename, (UINTN)oldpath, (UINTN)newpath);
}


time_t 
Gasketmktime (struct tm *tm)
{
  return GasketUintn (mktime, (UINTN)tm);
}


int 
Gasketfsync (int fd)
{
  return GasketUintn (fsync, fd);
}


int 
Gasketchmod (const char *path, mode_t mode)
{
  return GasketUintnUint16 (chmod, (UINTN)path, mode);
}


int 
Gasketutime (const char *filename, const struct utimbuf *buf)
{
  return GasketUintnUintn (utime, (UINTN)filename, (UINTN)buf);
}


int 
Gaskettcflush (int fildes, int queue_selector)
{
  return GasketUintnUintn (tcflush, fildes, queue_selector);
}


EFI_STATUS 
GasketUgaCreate (struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo, CONST CHAR16 *Title)
{
  return GasketUintnUintn (UgaCreate, (UINTN)UgaIo, (UINTN)Title);
}


void 
Gasketperror (__const char *__s)
{
  GasketUintn (perror, (UINTN)__s);
  return;
}



//
// ... is always an int or pointer to device specific data structure
//
int 
Gasketioctl (int fd, unsigned long int __request, void *Arg)
{
  return GasketUintnUintnUintn (ioctl, fd, __request, (UINTN)Arg);
}


int 
Gasketfcntl (int __fd, int __cmd, void  *Arg)
{
  return GasketUintnUintnUintn (fcntl, __fd, __cmd, (UINTN)Arg);
}



int 
Gasketcfsetispeed (struct termios *__termios_p, speed_t __speed)
{
  return GasketUintnUintn (cfsetispeed, (UINTN)__termios_p, __speed);
}


int 
Gasketcfsetospeed (struct termios *__termios_p, speed_t __speed)
{
  return GasketUintnUintn (cfsetospeed, (UINTN)__termios_p, __speed);
}


int 
Gaskettcgetattr (int __fd, struct termios *__termios_p)
{
  return GasketUintnUintn (tcgetattr, __fd, (UINTN)__termios_p);
}

 
int 
Gaskettcsetattr (int __fd, int __optional_actions, __const struct termios *__termios_p)
{
  return GasketUintnUintnUintn (tcsetattr, __fd, __optional_actions, (UINTN)__termios_p);
}




RETURN_STATUS
GasketUnixPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  )
{
  return GasketUintnUintn (SecPeCoffGetEntryPoint, (UINTN)Pe32Data, (UINTN)EntryPoint);
}



VOID
GasketUnixPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  GasketUintn (SecPeCoffRelocateImageExtraAction, (UINTN)ImageContext);
  return;
}



VOID
GasketUnixPeCoffUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  GasketUintn (SecPeCoffLoaderUnloadImageExtraAction, (UINTN)ImageContext);
  return;
}


//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//

EFI_STATUS 
EFIAPI 
GasketUgaClose (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  return GasketUintn (UgaClose, (UINTN)UgaIo);
}

EFI_STATUS 
EFIAPI 
GasketUgaSize (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, UINT32 Width, UINT32 Height)
{
  return GasketUintnUintnUintn (UgaSize, (UINTN)UgaIo, Width, Height);
}

EFI_STATUS 
EFIAPI 
GasketUgaCheckKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  return GasketUintn (UgaCheckKey, (UINTN)UgaIo);
}

EFI_STATUS 
EFIAPI 
GasketUgaKeySetState (EFI_UNIX_UGA_IO_PROTOCOL   *UgaIo, EFI_KEY_TOGGLE_STATE *KeyToggleState)
{
  return GasketUintnUintn (UgaGetKey, (UINTN)UgaIo, (UINTN)KeyToggleState);
}

EFI_STATUS 
EFIAPI 
GasketUgaGetKey (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_KEY_DATA *key)
{
  return GasketUintnUintn (UgaGetKey, (UINTN)UgaIo, (UINTN)key);
}

EFI_STATUS 
EFIAPI 
GasketUgaRegisterKeyNotify (
  IN EFI_UNIX_UGA_IO_PROTOCOL           *UgaIo, 
  IN UGA_REGISTER_KEY_NOTIFY_CALLBACK   CallBack,
  IN VOID                               *Context
  )
{
  return GasketUintnUintnUintn (UgaRegisterKeyNotify, (UINTN)UgaIo, (UINTN)CallBack, (UINTN)Context);  
}

EFI_STATUS 
EFIAPI 
GasketUgaBlt (
   EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
   IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
   IN  EFI_UGA_BLT_OPERATION                   BltOperation,
   IN  UGA_BLT_ARGS                            *Args
   )
{
  return GasketUintnUintnUintnUintn (UgaBlt, (UINTN)UgaIo, (UINTN)BltBuffer, (UINTN)BltOperation, (UINTN)Args);
}

EFI_STATUS 
EFIAPI 
GasketUgaCheckPointer (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo)
{
  return GasketUintn (UgaCheckPointer, (UINTN)UgaIo);
}

EFI_STATUS 
EFIAPI 
GasketUgaGetPointerState (EFI_UNIX_UGA_IO_PROTOCOL *UgaIo, EFI_SIMPLE_POINTER_STATE *state)
{
  return GasketUintnUintn (UgaGetPointerState, (UINTN)UgaIo, (UINTN)state);
}

void
GasketUnixEnableInterrupt (void)
{
  GasketVoid (UnixEnableInterrupt);
}

void
GasketUnixDisableInterrupt (void)
{
  GasketVoid (UnixDisableInterrupt);
}


int
Gasketgetifaddrs (struct ifaddrs **ifap)
{
  return( GasketUintn( getifaddrs, ( UINTN ) ifap ) );
}


void
Gasketfreeifaddrs (struct ifaddrs *ifap)
{
  GasketUintn( freeifaddrs, ( UINTN ) ifap );
}


int
Gasketsocket (int domain, int type, int protocol )
{
  return( GasketUintnUintnUintn( socket, domain, type, protocol ) );
}


#endif

