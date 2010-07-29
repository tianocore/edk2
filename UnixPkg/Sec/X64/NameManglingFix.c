/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SecMain.h"

//
// OS X Posix does some strange name mangling on these names in C.
// If you call from assembler you get the wrong version of the function
// So these globals get you the correct name mangled functions that can
// be accessed from assembly
//
UnixRmDir     gUnixRmDir     = rmdir;
UnixOpenDir   gUnixOpenDir   = opendir;
UnixStat      gUnixStat      = (UnixStat)stat;
UnixStatFs    gUnixStatFs    = statfs;
UnixReadDir   gUnixReaddir   = readdir;
UnixRewindDir gUnixRewinddir = rewinddir;

int
UnixIoCtl1 (
  int               fd, 
  unsigned long int __request, 
  UINTN             Arg
  ) 
{
  return ioctl (fd, __request, Arg);
}

int
UnixFcntl1 (int __fd, int __cmd, UINTN Arg)
{
  return fcntl (__fd, __cmd, Arg);
}