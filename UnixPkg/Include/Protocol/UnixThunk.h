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

  UnixThunk.h

Abstract:

  This protocol allows an EFI driver in the Unix emulation environment
  to make Posix calls.

  NEVER make an Unix call directly, always make the call via this protocol.

  There are no This pointers on the protocol member functions as they map
  exactly into Unix system calls.

--*/

#ifndef _UNIX_THUNK_H_
#define _UNIX_THUNK_H_

#include <Common/UnixInclude.h>

#include <Base.h>
#include <Library/PeCoffLib.h>



#define EFI_UNIX_THUNK_PROTOCOL_GUID \
  { \
    0xf2e98868, 0x8985, 0x11db, {0x9a, 0x59, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
VOID
(EFIAPI *UnixSleep) (
  unsigned long Milliseconds
  );

typedef
VOID
(EFIAPI *UnixExit) (
  int status  // exit code for all threads
  );

typedef
VOID
(EFIAPI *UnixSetTimer) (
  UINT64 PeriodMs,
  VOID (EFIAPI *CallBack)(UINT64 DeltaMs)
  );

typedef
VOID
(EFIAPI *UnixGetLocalTime) (
  EFI_TIME *Time
  );

typedef
struct tm *
(EFIAPI *UnixGmTime)(
  const time_t *timep
  );

typedef
long
(EFIAPI *UnixGetTimeZone)(
  void
  );

typedef
int
(EFIAPI *UnixGetDayLight)(
  void
  );

typedef
int
(EFIAPI *UnixPoll)(
  struct pollfd *pfd,
  unsigned int  nfds,
  int           timeout
  );

typedef
long
(EFIAPI *UnixRead) (
  int  fd,
  void *buf,
  int  count
  );

typedef
long
(EFIAPI *UnixWrite) (
  int         fd,
  const void  *buf,
  int         count
  );

typedef
char *
(EFIAPI *UnixGetenv) (const char *var);

typedef
int
(EFIAPI *UnixOpen) (const char *name, int flags, int mode);

typedef
off_t
(EFIAPI *UnixSeek) (int fd, off_t off, int whence);

typedef
int
(EFIAPI *UnixFtruncate) (int fd, long int len);

typedef
int
(EFIAPI *UnixClose) (int fd);

typedef
int
(EFIAPI *UnixMkdir)(const char *pathname, mode_t mode);

typedef
int
(EFIAPI *UnixRmDir)(const char *pathname);

typedef
int
(EFIAPI *UnixUnLink)(const char *pathname);

typedef
int
(EFIAPI *UnixGetErrno)(VOID);

typedef
DIR *
(EFIAPI *UnixOpenDir)(const char *pathname);

typedef
void
(EFIAPI *UnixRewindDir)(DIR *dir);

typedef
struct dirent *
(EFIAPI *UnixReadDir)(DIR *dir);

typedef
int
(EFIAPI *UnixCloseDir)(DIR *dir);

typedef
int
(EFIAPI *UnixStat)(const char *path, STAT_FIX *buf);

typedef
int
(EFIAPI *UnixStatFs)(const char *path, struct statfs *buf);

typedef
int
(EFIAPI *UnixRename)(const char *oldpath, const char *newpath);

typedef
time_t
(EFIAPI *UnixMkTime)(struct tm *tm);

typedef
int
(EFIAPI *UnixFSync)(int fd);

typedef
int
(EFIAPI *UnixChmod)(const char *path, mode_t mode);

typedef
int
(EFIAPI *UnixUTime)(const char *filename, const struct utimbuf *buf);

struct _EFI_UNIX_UGA_IO_PROTOCOL;
typedef
EFI_STATUS
(EFIAPI *UnixUgaCreate)(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo,
     CONST CHAR16 *Title);

typedef
int
(EFIAPI *UnixTcflush) (int fildes, int queue_selector);

typedef
void
(EFIAPI *UnixPerror) (__const char *__s);

typedef
int
#if __CYGWIN__
(EFIAPI *UnixIoCtl) (int fd, int __request, UINTN Arg);
#else
(EFIAPI *UnixIoCtl) (int fd, unsigned long int __request, void *Arg);
#endif

typedef
int
(EFIAPI *UnixFcntl) (int __fd, int __cmd, void *Arg);

typedef
int
(EFIAPI *UnixCfsetispeed) (struct termios *__termios_p, speed_t __speed);

typedef
int
(EFIAPI *UnixCfsetospeed) (struct termios *__termios_p, speed_t __speed);

typedef
int
(EFIAPI *UnixTcgetattr) (int __fd, struct termios *__termios_p);

typedef
int
(EFIAPI *UnixTcsetattr) (int __fd, int __optional_actions,
          __const struct termios *__termios_p);


//
// Worker functions to enable source level debug in the emulator
//

typedef
RETURN_STATUS
(EFIAPI *UnixPeCoffGetEntryPoint) (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

typedef
VOID
(EFIAPI *UnixPeCoffRelocateImageExtraAction) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

typedef
VOID
(EFIAPI *UnixPeCoffLoaderUnloadImageExtraAction) (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

typedef
int
(EFIAPI *UnixGetIfAddrs) (
  struct ifaddrs **ifap
  );

typedef
void
(EFIAPI *UnixFreeIfAddrs) (
  struct ifaddrs *ifap
  );

typedef
int
(EFIAPI *UnixSocket) (
  int domain, 
  int type, 
  int protocol
  );

typedef 
void
(EFIAPI *UnixDisableInterruptEmulation) (void);

typedef 
void
(EFIAPI *UnixEnableInterruptEmulation) (void);




#define EFI_UNIX_THUNK_PROTOCOL_SIGNATURE SIGNATURE_32 ('L', 'N', 'X', 'T')

typedef struct _EFI_UNIX_THUNK_PROTOCOL {
  UINT64                              Signature;

  UnixSleep                           Sleep;
  UnixExit                            Exit;
  UnixSetTimer                        SetTimer;
  UnixGetLocalTime                    GetLocalTime;
  UnixGmTime                          GmTime;
  UnixGetTimeZone                     GetTimeZone;
  UnixGetDayLight                     GetDayLight;
  UnixPoll                            Poll;
  UnixRead                            Read;
  UnixWrite                           Write;
  UnixGetenv                          Getenv;
  UnixOpen                            Open;
  UnixSeek                            Lseek;
  UnixFtruncate                       FTruncate;
  UnixClose                           Close;
  UnixMkdir                           MkDir;
  UnixRmDir                           RmDir;
  UnixUnLink                          UnLink;
  UnixGetErrno                        GetErrno;
  UnixOpenDir                         OpenDir;
  UnixRewindDir                       RewindDir;
  UnixReadDir                         ReadDir;
  UnixCloseDir                        CloseDir;
  UnixStat                            Stat;
  UnixStatFs                          StatFs;
  UnixRename                          Rename;
  UnixMkTime                          MkTime;
  UnixFSync                           FSync;
  UnixChmod                           Chmod;
  UnixUTime                           UTime;
  UnixTcflush                         Tcflush;
  UnixUgaCreate                        UgaCreate;
  UnixPerror                          Perror;
  UnixIoCtl                           IoCtl;
  UnixFcntl                           Fcntl;
  UnixCfsetispeed                     Cfsetispeed;
  UnixCfsetospeed                     Cfsetospeed;
  UnixTcgetattr                       Tcgetattr;
  UnixTcsetattr                       Tcsetattr;
  UnixPeCoffGetEntryPoint                 PeCoffGetEntryPoint;
  UnixPeCoffRelocateImageExtraAction      PeCoffRelocateImageExtraAction;
  UnixPeCoffLoaderUnloadImageExtraAction  PeCoffUnloadImageExtraAction;
  UnixEnableInterruptEmulation        EnableInterrupt;
  UnixDisableInterruptEmulation       DisableInterrupt;

  UnixGetIfAddrs                      GetIfAddrs;
  UnixFreeIfAddrs                     FreeIfAddrs;
  UnixSocket                          Socket;
} EFI_UNIX_THUNK_PROTOCOL;

extern EFI_GUID gEfiUnixThunkProtocolGuid;

#endif
