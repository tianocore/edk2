/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _GASKET_H_
#define _GASKET_H_

#include <Library/PeCoffLib.h>

#include <Protocol/UgaDraw.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/UnixUgaIo.h>


//
// Gasket functions for EFI_UNIX_THUNK_PROTOCOL
//

void
EFIAPI
GasketmsSleep (unsigned long Milliseconds);

void
EFIAPI
Gasketexit (
  int status
  );

void
EFIAPI
GasketSetTimer (
  UINT64 PeriodMs,
  VOID (*CallBack)(UINT64 DeltaMs)
  );

void
EFIAPI
GasketGetLocalTime (
  EFI_TIME *Time
  );

struct tm *
EFIAPI
Gasketgmtime (
  const time_t *clock
  );

long
EFIAPI
GasketGetTimeZone (
  void
  );

int
EFIAPI
GasketGetDayLight (
  void
  );


int
EFIAPI
Gasketpoll (
  struct pollfd *pfd,
  unsigned int nfds,
  int timeout
  );

long
EFIAPI
Gasketread (
  int fd,
  void *buf,
  int count);

long
EFIAPI
Gasketwrite (
  int fd,
  const void *buf,
  int count
  );

char *
EFIAPI
Gasketgetenv (
  const char *name
  );

int
EFIAPI
Gasketopen (
  const char *name,
  int flags,
  int mode
  );

off_t
EFIAPI
Gasketlseek (
  int fd,
  off_t off,
  int whence
  );

int
EFIAPI
Gasketftruncate (
  int fd,
  long int len
  );

int
EFIAPI
Gasketclose (
  int fd
  );

int
EFIAPI
Gasketmkdir (
  const char *pathname,
  mode_t mode
  );

int
EFIAPI
Gasketrmdir (
  const char *pathname
  );

int
EFIAPI
Gasketunlink (
  const char *pathname
  );

int
EFIAPI
GasketGetErrno (
  void
  );

DIR *
EFIAPI
Gasketopendir (
  const char *pathname
  );

void
EFIAPI
Gasketrewinddir (
  DIR *dir
  );

struct dirent *
EFIAPI
Gasketreaddir (
  DIR *dir
  );

int
EFIAPI
Gasketclosedir (
  DIR *dir
  );

int
EFIAPI
Gasketstat (const char *path, STAT_FIX *buf);

int
EFIAPI
Gasketstatfs (const char *path, struct statfs *buf);

int
EFIAPI
Gasketrename (
  const char *oldpath,
  const char *newpath
  );

time_t
EFIAPI
Gasketmktime (
  struct tm *tm
  );

int
EFIAPI
Gasketfsync (
  int fd
  );

int
EFIAPI
Gasketchmod (
  const char *path,
  mode_t mode
  );

int
EFIAPI
Gasketutime (
  const char *filename,
  const struct utimbuf *buf
  );

int
EFIAPI
Gaskettcflush (
  int fildes,
  int queue_selector
  );

EFI_STATUS
EFIAPI
GasketUgaCreate (
  struct _EFI_UNIX_UGA_IO_PROTOCOL **UgaIo,
  CONST CHAR16 *Title
  );

void
EFIAPI
Gasketperror (
  __const char *__s
  );

//
// ... is always an int or pointer to device specific data structure
//

int
EFIAPI
Gasketioctl (
  int fd,
  unsigned long int __request,
  void *Arg
  );

int
EFIAPI
Gasketfcntl (
  int __fd,
  int __cmd,
  void *Arg
  );

int
EFIAPI
Gasketcfsetispeed (
  struct termios *__termios_p,
  speed_t __speed
  );

int
EFIAPI
Gasketcfsetospeed (
  struct termios *__termios_p,
  speed_t __speed
  );

int
EFIAPI
Gaskettcgetattr (
  int __fd,
  struct termios *__termios_p
  );

int
EFIAPI
Gaskettcsetattr (
  int __fd,
  int __optional_actions,
  __const struct termios *__termios_p
  );

int
EFIAPI
Gasketsigaction (
  int sig,
  const struct sigaction *act,
  struct sigaction *oact
  );

RETURN_STATUS
EFIAPI
GasketUnixPeCoffGetEntryPoint (
  IN     VOID  *Pe32Data,
  IN OUT VOID  **EntryPoint
  );

VOID
EFIAPI
GasketUnixPeCoffRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );

VOID
EFIAPI
GasketUnixPeCoffUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  );



UINTN
EFIAPI
GasketVoid (
  void *api
  );

UINTN
EFIAPI
GasketUintn (
  void *api,
  UINTN a
  );

UINTN
EFIAPI
GasketUintnUintn (
  void *api,
  UINTN a,
  UINTN b
  );

UINTN
EFIAPI
GasketUintnUintnUintn (
  void *api,
  UINTN a,
  UINTN b,
  UINTN c
  );

UINTN
EFIAPI
GasketUintnUintnUintnUintn (
  void *api,
  UINTN a,
  UINTN b,
  UINTN c,
  UINTN d
  );

UINTN
EFIAPI
GasketUintn10Args (
  void *api,
  UINTN a,
  UINTN b,
  UINTN c,
  UINTN d,
  UINTN e,
  UINTN f,
  UINTN g,
  UINTN h,
  UINTN i,
  UINTN j
  );

UINTN
EFIAPI
GasketUint64Uintn (
  void *api,
  UINT64 a,
  UINTN b);

UINT64
EFIAPI
GasketUintnUint64Uintn (
  void *api,
  UINTN a,
  UINT64 b,
  UINTN c
  );

UINTN
EFIAPI
GasketUintnUint16 (
  void *api,
  UINTN a,
  UINT16 b
  );

typedef
void
(*CALL_BACK) (
  UINT64 Delta
  );

UINTN
ReverseGasketUint64 (
  CALL_BACK CallBack,
  UINT64 a
  );

//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//


EFI_STATUS
EFIAPI
GasketUgaClose (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo
  );

EFI_STATUS
EFIAPI
GasketUgaSize (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
  UINT32 Width,
  UINT32 Height
  );

EFI_STATUS
EFIAPI
GasketUgaCheckKey (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo
  );

EFI_STATUS
EFIAPI
GasketUgaGetKey (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
  EFI_INPUT_KEY *key
  );

EFI_STATUS
EFIAPI
GasketUgaBlt (
   EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
   IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
   IN  EFI_UGA_BLT_OPERATION                   BltOperation,
   IN  UINTN                                   SourceX,
   IN  UINTN                                   SourceY,
   IN  UINTN                                   DestinationX,
   IN  UINTN                                   DestinationY,
   IN  UINTN                                   Width,
   IN  UINTN                                   Height,
   IN  UINTN                                   Delta OPTIONAL
   );

EFI_STATUS
EFIAPI
UgaCreate (
  EFI_UNIX_UGA_IO_PROTOCOL **Uga,
  CONST CHAR16 *Title
  );


//
// Gasket functions for EFI_UNIX_UGA_IO_PROTOCOL
//
EFI_STATUS
EFIAPI
UgaClose (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo
  );

EFI_STATUS
EFIAPI
UgaSize(
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
  UINT32 Width,
  UINT32 Height
  );

EFI_STATUS
EFIAPI
UgaCheckKey(
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo
  );

EFI_STATUS
EFIAPI
UgaGetKey (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
  EFI_INPUT_KEY *key
  );

EFI_STATUS
EFIAPI
UgaBlt (
  EFI_UNIX_UGA_IO_PROTOCOL *UgaIo,
  IN  EFI_UGA_PIXEL                           *BltBuffer OPTIONAL,
  IN  EFI_UGA_BLT_OPERATION                   BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta OPTIONAL
  );



#endif


