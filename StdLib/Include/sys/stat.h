/** @file

    Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made
    available under  the terms and conditions of the BSD License that
    accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Copyright (c) 1982, 1986, 1989, 1993
     The Regents of the University of California.  All rights reserved.
    (c) UNIX System Laboratories, Inc.
    All or some portions of this file are derived from material licensed
    to the University of California by American Telephone and Telegraph
    Co. or Unix System Laboratories, Inc. and are reproduced herein with
    the permission of UNIX System Laboratories, Inc.

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

    stat.h  8.12 (Berkeley) 8/17/94
    NetBSD: stat.h,v 1.54 2006/02/24 22:01:30 thorpej Exp
 */
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#include  <sys/featuretest.h>
#include  <sys/types.h>
#include  <sys/time.h>

struct stat {
  off_t           st_size;          ///< file size, in bytes
  off_t           st_physsize;      ///< physical space the file consumes
  off_t           st_curpos;        ///< current position within the file, or XY coord. for Console
  dtime_t         st_birthtime;     ///< time of creation
  dtime_t         st_atime;         ///< time of last access
  dtime_t         st_mtime;         ///< time of last data modification
  mode_t          st_mode;          ///< file attributes

  blksize_t       st_blksize;       ///< optimal blocksize for I/O
  uint32_t        st_spare[1];
};

#define S_ISUID       0004000     ///< set user id on execution
#define S_ISGID       0002000     ///< set group id on execution
#define S_ISTXT       0001000     ///< sticky bit

#define S_IRWXU       0000700     ///< RWX mask for owner
#define S_IRUSR       0000400     ///< R for owner
#define S_IWUSR       0000200     ///< W for owner
#define S_IXUSR       0000100     ///< X for owner

#define S_IREAD       S_IRUSR
#define S_IWRITE      S_IWUSR
#define S_IEXEC       S_IXUSR

#define S_IRWXG       0000070     ///< RWX mask for group
#define S_IRGRP       0000040     ///< R for group
#define S_IWGRP       0000020     ///< W for group
#define S_IXGRP       0000010     ///< X for group

#define S_IRWXO       0000007     ///< RWX mask for other
#define S_IROTH       0000004     ///< R for other
#define S_IWOTH       0000002     ///< W for other
#define S_IXOTH       0000001     ///< X for other

/*  The Octal access modes, above, fall into the Hex mask 0x00000FFF.
    Traditionally, the remainder of the flags are specified in Octal
    but they are expressed in Hex here for modern clarity.

    The basic file types, specified within 0x0000F000, are mutually exclusive.
*/
#define _S_IFMT       0x000FF000   ///< type-of-file mask
#define _S_IFIFO      0x00001000   ///< named pipe (fifo)
#define _S_IFCHR      0x00002000   ///< character special device
#define _S_IFDIR      0x00004000   ///< directory
#define _S_IFBLK      0x00006000   ///< block special device
#define _S_IFREG      0x00008000   ///< regular
#define _S_IFSOCK     0x0000C000   ///< socket
#define _S_ITTY       0x00010000   ///< File connects to a TTY device
#define _S_IWTTY      0x00020000   ///< TTY sends and receives Wide characters
#define _S_ICONSOLE   0x00030000   ///< UEFI Console Device

/*  UEFI specific (FAT file system) File attributes.
    Specified in Hexadecimal instead of Octal.
    These bits correspond to the xx portion of _S_IFMT
*/
#define S_IREADONLY   0x00100000    // Read Only File
#define S_IHIDDEN     0x00200000    // Hidden File
#define S_ISYSTEM     0x00400000    // System File
#define S_IDIRECTORY  0x01000000    // Directory
#define S_IARCHIVE    0x02000000    // Archive Bit
#define S_IROFS       0x08000000   ///< Read Only File System

#define S_EFIONLY     0xFFF00000  ///< Flags only used by the EFI system calls.

#define S_EFISHIFT    20            // LS bit of the UEFI attributes

#define S_IFMT      _S_IFMT
#define S_IFIFO     _S_IFIFO
#define S_IFCHR     _S_IFCHR
#define S_IFDIR     _S_IFDIR
#define S_IFBLK     _S_IFBLK
#define S_IFREG     _S_IFREG
#define S_IFSOCK    _S_IFSOCK
#define S_ITTY      _S_ITTY
#define S_IWTTY     _S_IWTTY
#define S_ICONSOLE  _S_ICONSOLE

#define S_ISFIFO(m) ((m & _S_IFMT) == _S_IFIFO)   ///< fifo
#define S_ISCHR(m)  ((m & _S_IFMT) == _S_IFCHR)   ///< char special
#define S_ISDIR(m)  ((m & _S_IFMT) == _S_IFDIR)   ///< directory
#define S_ISBLK(m)  ((m & _S_IFMT) == _S_IFBLK)   ///< block special
#define S_ISREG(m)  ((m & _S_IFMT) == _S_IFREG)   ///< regular file
#define S_ISSOCK(m) ((m & _S_IFMT) == _S_IFSOCK)  ///< socket


/*  The following three macros have been changed to reflect
    access permissions that better reflect the UEFI FAT file system.
    UEFI only supports Read or Read+Write instead of the *nix
    rwx paradigm.  Thus, using 0777 is the closest analog.
*/
#define ACCESSPERMS (S_IRWXU|S_IRWXG|S_IRWXO)     ///< 0777
#define ALLPERMS  (S_IRWXU|S_IRWXG|S_IRWXO)       ///< 0777
#define DEFFILEMODE (S_IRWXU|S_IRWXG|S_IRWXO)     ///< 0777

#define READ_PERMS  (S_IRUSR | S_IRGRP | S_IROTH)   ///< 0444
#define WRITE_PERMS (S_IWUSR | S_IWGRP | S_IWOTH)   ///< 0222
#define EXEC_PERMS  (S_IXUSR | S_IXGRP | S_IXOTH)   ///< 0111

#define S_BLKSIZE 512   ///< block size used in the stat struct

/*
 * Definitions of flags stored in file flags word.
 *
 * Super-user and owner changeable flags.
 */
#define UF_SETTABLE   0x0000ffff  ///< mask of owner changeable flags
#define UF_NODUMP     0x00000001  ///< do not dump file
#define UF_IMMUTABLE  0x00000002  ///< file may not be changed
#define UF_APPEND     0x00000004  ///< writes to file may only append
/*  UF_NOUNLINK 0x00000010     [NOT IMPLEMENTED] */
/*
 * Super-user changeable flags.
 */
#define SF_SETTABLE   0xffff0000  ///< mask of superuser changeable flags
#define SF_ARCHIVED   0x00010000  ///< file is archived
#define SF_IMMUTABLE  0x00020000  ///< file may not be changed
#define SF_APPEND     0x00040000  ///< writes to file may only append
/*  SF_NOUNLINK 0x00100000     [NOT IMPLEMENTED] */

#include  <sys/EfiCdefs.h>

__BEGIN_DECLS
#ifndef __STAT_SYSCALLS_DECLARED
  #define __STAT_SYSCALLS_DECLARED

  /**
  **/
  mode_t  umask (mode_t);

  /**
  **/
  int     mkdir (const char *, mode_t);

  /**
  **/
  int     fstat (int, struct stat *);

  /**
  **/
  int     lstat (const char *, struct stat *);

/** Obtains information about the file pointed to by path.

    Opens the file pointed to by path, calls _EFI_FileInfo with the file's handle,
    then closes the file.

    @param[in]    path      Path to the file to obtain information about.
    @param[out]   statbuf   Buffer in which the file status is put.

    @retval    0  Successful Completion.
    @retval   -1  An error has occurred and errno has been set to
                  identify the error.
**/
  int     stat  (const char *, struct stat *);

  /**
  **/
  int     chmod (const char *, mode_t);
#endif  // __STAT_SYSCALLS_DECLARED
__END_DECLS

#endif /* !_SYS_STAT_H_ */
