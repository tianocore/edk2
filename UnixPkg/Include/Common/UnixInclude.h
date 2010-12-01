/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
This program and the accompanying materials
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

// #include <sys/poll.h>
// #include <dirent.h>

//
// Name mangle to prevent build errors. I.e conflicts between EFI and OS
//
#define NTOHL   _UNIX_EFI_NAME_MANGLE_NTOHL_
#define HTONL   _UNIX_EFI_NAME_MANGLE_HTONL_
#define NTOHS   _UNIX_EFI_NAME_MANGLE_NTOHS_
#define HTONS   _UNIX_EFI_NAME_MANGLE_HTOHS_
#define B0      _UNIX_EFI_NAME_MANGLE_B0_


#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <sys/time.h>

#if __CYGWIN__
#include <sys/dirent.h>
#else
#include <sys/dir.h>
#endif

#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/bpf.h>

#ifdef __APPLE__
#include <sys/param.h>
#include <sys/mount.h>
#define _XOPEN_SOURCE
#ifndef _Bool
  #define _Bool char // for clang debug
#endif
#else
#include <termio.h>
#include <sys/vfs.h>
#endif 

#include <utime.h>

#if __APPLE__
//
// EFI packing is not compatible witht he default OS packing for struct stat.
// st_size is 64-bit but starts on a 32-bit offset in the structure. The compiler
// flags used to produce compatible EFI images, break struct stat
//
#ifdef MDE_CPU_IA32
#pragma pack(4)
#endif

#if defined(__DARWIN_64_BIT_INO_T)


typedef struct {
  UINTN	tv_sec;		/* seconds */
	UINTN	tv_nsec;	/* and nanoseconds */
} EFI_timespec;



typedef struct stat_fix { \
	dev_t		st_dev;			/* [XSI] ID of device containing file */ 
	mode_t		st_mode;		/* [XSI] Mode of file (see below) */ 
	nlink_t		st_nlink;		/* [XSI] Number of hard links */ 
	__darwin_ino64_t st_ino;		/* [XSI] File serial number */ 
	uid_t		st_uid;			/* [XSI] User ID of the file */ 
	gid_t		st_gid;			/* [XSI] Group ID of the file */ 
	dev_t		st_rdev;		/* [XSI] Device ID */ 

  // clang for X64 ABI follows Windows and a long is 32-bits
  // this breaks system inlcude files so that is why we need
  // to redefine timespec as EFI_timespec 
  EFI_timespec  st_atimespec;
  EFI_timespec  st_mtimespec;
  EFI_timespec  st_ctimespec;
  EFI_timespec  st_birthtimespec;

	off_t		st_size;		/* [XSI] file size, in bytes */ 
	blkcnt_t	st_blocks;		/* [XSI] blocks allocated for file */ 
	blksize_t	st_blksize;		/* [XSI] optimal blocksize for I/O */ 
	__uint32_t	st_flags;		/* user defined flags for file */ 
	__uint32_t	st_gen;			/* file generation number */ 
	__int32_t	st_lspare;		/* RESERVED: DO NOT USE! */ 
	__int64_t	st_qspare[2];		/* RESERVED: DO NOT USE! */ 
} STAT_FIX;

#else /* !__DARWIN_64_BIT_INO_T */

typedef struct stat_fix {
	dev_t	 	st_dev;		/* [XSI] ID of device containing file */
	ino_t	  	st_ino;		/* [XSI] File serial number */
	mode_t	 	st_mode;	/* [XSI] Mode of file (see below) */
	nlink_t		st_nlink;	/* [XSI] Number of hard links */
	uid_t		st_uid;		/* [XSI] User ID of the file */
	gid_t		st_gid;		/* [XSI] Group ID of the file */
	dev_t		st_rdev;	/* [XSI] Device ID */
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
	struct	timespec st_atimespec;	/* time of last access */
	struct	timespec st_mtimespec;	/* time of last data modification */
	struct	timespec st_ctimespec;	/* time of last status change */
#else
	time_t		st_atime;	/* [XSI] Time of last access */
	long		st_atimensec;	/* nsec of last access */
	time_t		st_mtime;	/* [XSI] Last data modification time */
	long		st_mtimensec;	/* last data modification nsec */
	time_t		st_ctime;	/* [XSI] Time of last status change */
	long		st_ctimensec;	/* nsec of last status change */
#endif
	off_t		st_size;	/* [XSI] file size, in bytes */
	blkcnt_t	st_blocks;	/* [XSI] blocks allocated for file */
	blksize_t	st_blksize;	/* [XSI] optimal blocksize for I/O */
	__uint32_t	st_flags;	/* user defined flags for file */
	__uint32_t	st_gen;		/* file generation number */
	__int32_t	st_lspare;	/* RESERVED: DO NOT USE! */
	__int64_t	st_qspare[2];	/* RESERVED: DO NOT USE! */
} STAT_FIX;

#endif

#ifdef MDE_CPU_IA32
#pragma pack(4)
#endif

#else 

  typedef struct stat STAT_FIX;

#endif

//
// Undo name mangling
//
#undef NTOHL
#undef HTONL
#undef NTOHS
#undef HTONS
#undef B0


#endif

