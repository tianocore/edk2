/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#include <sys/termios.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/dir.h>
#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termio.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#include <utime.h>

#define EFI_UNIX_THUNK_PROTOCOL_GUID \
  { \
    0xf2e98868, 0x8985, 0x11db, {0x9a, 0x59, 0x00, 0x40, 0xd0, 0x2b, 0x18, 0x35 } \
  }

typedef
VOID
(*UnixSleep) (
  unsigned long Milliseconds
  );

typedef
VOID
(*UnixExit) (
  int status  // exit code for all threads
  );

typedef
VOID
(*UnixSetTimer) (UINT64 PeriodMs, VOID (*CallBack)(UINT64 DeltaMs));
typedef
VOID
(*UnixGetLocalTime) (EFI_TIME *Time);
typedef
struct tm *
(*UnixGmTime)(const time_t *timep);
typedef
long
(*UnixGetTimeZone)(void);
typedef
int
(*UnixGetDayLight)(void);
typedef
int
(*UnixPoll)(struct pollfd *pfd, int nfds, int timeout);
typedef
int
(*UnixRead) (int fd, void *buf, int count);
typedef
int
(*UnixWrite) (int fd, const void *buf, int count);
typedef
char *
(*UnixGetenv) (const char *var);
typedef
int
(*UnixOpen) (const char *name, int flags, int mode);
typedef
long int
(*UnixSeek) (int fd, long int off, int whence);
typedef
int
(*UnixFtruncate) (int fd, long int len);
typedef
int
(*UnixClose) (int fd);

typedef
int
(*UnixMkdir)(const char *pathname, mode_t mode);
typedef
int
(*UnixRmDir)(const char *pathname);
typedef
int
(*UnixUnLink)(const char *pathname);
typedef
int
(*UnixGetErrno)(VOID);
typedef
DIR *
(*UnixOpenDir)(const char *pathname);
typedef
void
(*UnixRewindDir)(DIR *dir);
typedef
struct dirent *
(*UnixReadDir)(DIR *dir);
typedef
int
(*UnixCloseDir)(DIR *dir);
typedef
int
(*UnixStat)(const char *path, struct stat *buf);
typedef
int
(*UnixStatFs)(const char *path, struct statfs *buf);
typedef
int
(*UnixRename)(const char *oldpath, const char *newpath);
typedef
time_t
(*UnixMkTime)(struct tm *tm);
typedef
int
(*UnixFSync)(int fd);
typedef
int
(*UnixChmod)(const char *path, mode_t mode);
typedef
int
(*UnixUTime)(const char *filename, const struct utimbuf *buf);

struct _EFI_UNIX_UGA_IO_PROTOCOL;
typedef
EFI_STATUS
(*UnixUgaCreate)(struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo,
		 CONST CHAR16 *Title);

typedef
int
(*UnixTcflush) (int fildes, int queue_selector);

typedef
void
(*UnixPerror) (__const char *__s);

typedef 
int 
(*UnixIoCtl) (int fd, unsigned long int __request, ...);

typedef 
int 
(*UnixFcntl) (int __fd, int __cmd, ...);

typedef
int 
(*UnixCfsetispeed) (struct termios *__termios_p, speed_t __speed);

typedef 
int 
(*UnixCfsetospeed) (struct termios *__termios_p, speed_t __speed);

typedef
int 
(*UnixTcgetattr) (int __fd, struct termios *__termios_p);

typedef 
int 
(*UnixTcsetattr) (int __fd, int __optional_actions,
		      __const struct termios *__termios_p);

//
//
//

#define EFI_UNIX_THUNK_PROTOCOL_SIGNATURE EFI_SIGNATURE_32 ('L', 'N', 'X', 'T')

typedef struct _EFI_UNIX_THUNK_PROTOCOL {
  UINT64                              Signature;

  UnixSleep                           Sleep;
  UnixExit                    	      Exit;
  UnixSetTimer                        SetTimer;
  UnixGetLocalTime		                GetLocalTime;
  UnixGmTime                          GmTime;
  UnixGetTimeZone                     GetTimeZone;
  UnixGetDayLight                     GetDayLight;
  UnixPoll	                          Poll;
  UnixRead                           Read;
  UnixWrite                          Write;
  UnixGetenv                         Getenv;
  UnixOpen                           Open;
  UnixSeek                           Lseek;
  UnixFtruncate                      FTruncate;
  UnixClose                          Close;
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
  UnixUgaCreate			                  UgaCreate;
  UnixPerror                          Perror;
  UnixIoCtl                           IoCtl;
  UnixFcntl                           Fcntl;
  UnixCfsetispeed                     Cfsetispeed;
  UnixCfsetospeed                     Cfsetospeed;
  UnixTcgetattr                       Tcgetattr;
  UnixTcsetattr                       Tcsetattr;
} EFI_UNIX_THUNK_PROTOCOL;

extern EFI_GUID gEfiUnixThunkProtocolGuid;

#endif
