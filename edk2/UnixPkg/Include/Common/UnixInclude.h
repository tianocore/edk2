/*++

Copyright (c) 2006 - 2009, Intel Corporation
Portions copyright (c) 2008-2009 Apple Inc.
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  UnixInclude.h

Abstract:
  Public include file for the Unix Library

--*/

#ifndef __UNIX_INCLUDE_H__
#define __UNIX_INCLUDE_H__
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __APPLE__
#include <sys/param.h>
#include <sys/mount.h>
#else
#include <sys/vfs.h>
#endif 

#include <sys/poll.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#endif
