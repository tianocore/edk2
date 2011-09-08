/** @file
  Declarations pertaining to directory entries under the UEFI environment.

  The information is based upon the EFI_FILE_INFO structure
  in MdePkg/Include/Guid/FileInfo.h.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Copyright (c) 1989, 1993
   The Regents of the University of California.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the University nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  SUCH DAMAGE.

   @(#)dirent.h  8.3 (Berkeley) 8/10/94
   NetBSD: dirent.h,v 1.23 2005/12/26 18:41:36 perry Exp
**/

#ifndef _SYS_DIRENT_H_
#define _SYS_DIRENT_H_

#include  <sys/featuretest.h>
#include  <time.h>
#include  <sys/time.h>

#define MAXNAMLEN 511

/*
 * The dirent structure defines the format of directory entries returned by
 * read(fd, ...) when fd refers to a directory.
 *
 * All names are wide characters and are guaranteed to be null terminated.
 * The maximum length of a name in a directory is MAXNAMLEN.
 */
struct dirent {
  UINT64    Size;               // Size of this dirent structure instance,
                                  // including the Null-terminated FileName string.
  UINT64    FileSize;           // The size of the file in bytes.
  UINT64    PhysicalSize;       // The amount of physical space the file consumes
                                  // on the file system volume.
  UINT64    Attribute;          // The time the file was created.
  struct timespec  CreateTime;         // The time when the file was last accessed.
  struct timespec  LastAccessTime;     // The time when the file's contents were last modified.
  struct timespec  ModificationTime;   // The attribute bits for the file. See below.
  CHAR16    FileName[1];         // The Null-terminated name of the file.
};

/*
 * File Attributes
 */
#define DT_UNKNOWN    0
#define DT_READ_ONLY  0x0000000000000001
#define DT_HIDDEN     0x0000000000000002
#define DT_SYSTEM     0x0000000000000004
#define DT_RESERVED   0x0000000000000008
#define DT_DIRECTORY  0x0000000000000010
#define DT_ARCHIVE    0x0000000000000020
#define DT_CHR        0x0000000000010000  // File attaches to a character device
#define DT_BLK        0x0000000000020000  // File attaches to a block device
#define DT_SOCKET     0x0000000000030000  // File attaches to a socket
#define DT_VALID_ATTR 0x0000000000030037  // Mask for valid attribute bits

#endif  /* !_SYS_DIRENT_H_ */
