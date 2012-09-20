/** @file
  The EFI kernel's interpretation of a "file".

  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * Copyright (c) 1982, 1986, 1989, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  file.h  8.3 (Berkeley) 1/9/95
    NetBSD: file.h,v 1.56 2006/05/14 21:38:18 elad Exp
**/
#ifndef _PIF_KFILE_H_
#define _PIF_KFILE_H_

#include  <Uefi.h>
#include  <Protocol/SimpleTextOut.h>
#include  <Protocol/SimpleFileSystem.h>

#include  <wchar.h>
#include  <stdarg.h>
#include  <sys/fcntl.h>
#include  <sys/unistd.h>

struct stat;
struct fileops;
struct _Device_Node;

/* The number of "special" character stream devices.
   These include:
    stdin, stdout, stderr
*/
#define NUM_SPECIAL   3

/* Organization of the f_iflags member of the __filedes structure. */
#define DTYPE_MASK      0x00000007    ///< Device Type
#define DTYPE_VNODE             1     /* file */
#define DTYPE_SOCKET            2     /* communications endpoint */
#define DTYPE_PIPE              3     /* pipe */
#define DTYPE_KQUEUE            4     /* event queue */
#define DTYPE_MISC              5     /* misc file descriptor type */
#define DTYPE_CRYPTO            6     /* crypto */
#define DTYPE_NAMES   "0", "file", "socket", "pipe", "kqueue", "misc", "crypto"

#define FIF_WANTCLOSE   0x10000000  /* a close is waiting for usecount */
#define FIF_DELCLOSE    0x20000000  /* Delete on close. */
#define FIF_LARVAL      0x80000000  /* not fully constructed; don't use */

/*
    This structure must be a multiple of 8 bytes in length.
*/
struct __filedes {
  off_t                   f_offset;     /* current position in file */
  const struct fileops   *f_ops;

  /*  The devdata member has different meanings depending upon whether
      a block oriented or character oriented device is being accessed.
      For block devices, devdata holds an EFI handle to the open file or directory.
      For character devices, devdata points to the device's IIO structure,
      if it has one.  It may be NULL indicating a non-interactive character
      device.
  */
  void                   *devdata;      /* Device-specific data */
  int                     Oflags;       // From the open call, see fcntl.h
  int                     Omode;        // From the open call
  int                     RefCount;     // Reference count of opens
  UINT32                  f_flag;       /* see fcntl.h */
  UINT32                  f_iflags;     // In use if non-zero
  UINT16                  MyFD;         // Which FD this is.
  UINT16                  Reserved_1;   // Force this structure to be a multiple of 8-bytes in length
};

struct fileops {
  /* These functions must always be implemented. */
  int     (EFIAPI *fo_close)    (struct __filedes *filp);
  ssize_t (EFIAPI *fo_read)     (struct __filedes *filp, off_t *Offset, size_t Len, void *Buf);
  ssize_t (EFIAPI *fo_write)    (struct __filedes *filp, off_t *Offset, size_t Len, const void *Buf);

  /* Call the fnullop_* version of these functions if not implemented by the device. */
  int     (EFIAPI *fo_fcntl)    (struct __filedes *filp, UINT32 Cmd, void *p3, void *p4);
  short   (EFIAPI *fo_poll)     (struct __filedes *filp, short Events);
  int     (EFIAPI *fo_flush)    (struct __filedes *filp);

  /* Call the fbadop_* version of these functions if not implemented by the device. */
  int     (EFIAPI *fo_stat)     (struct __filedes *filp, struct stat *StatBuf, void *Buf);
  int     (EFIAPI *fo_ioctl)    (struct __filedes *filp, ULONGN Cmd, va_list argp);
  int     (EFIAPI *fo_delete)   (struct __filedes *filp);
  int     (EFIAPI *fo_rmdir)    (struct __filedes *filp);
  int     (EFIAPI *fo_mkdir)    (const char *path, __mode_t perms);
  int     (EFIAPI *fo_rename)   (const char *from, const char *to);

  /* Use a NULL if this function has not been implemented by the device. */
  off_t   (EFIAPI *fo_lseek)    (struct __filedes *filp, off_t, int);
};

/*  A generic instance structure which is valid for
    for all device instance structures.

    All device instance structures MUST be a multiple of 8-bytes in length.
*/
typedef struct {
  UINT32                      Cookie;       ///< Special value identifying this as a valid Instance
  UINT32                      InstanceNum;  ///< Which instance is this?  Zero-based.
  EFI_HANDLE                  Dev;          ///< Pointer to either Input or Output Protocol.
  struct _Device_Node        *Parent;       ///< Points to the parent Device Node.
  struct fileops              Abstraction;  ///< Pointers to functions implementing this device's abstraction.
  UINTN                       Reserved_1;   // Force this to always be a multiple of 8-bytes in length
} GenericInstance;

/* Type of all Device-specific handler's open routines. */
typedef
  int     (EFIAPI *FO_OPEN)    (struct _Device_Node *This, struct __filedes *FD,
                                int Instance, wchar_t *Path, wchar_t *MPath);

#define FILE_IS_USABLE(fp)  (((fp)->f_iflags &      \
          (FIF_WANTCLOSE|FIF_LARVAL)) == 0)

#define FILE_SET_MATURE(fp)       \
do {                              \
  (fp)->f_iflags &= ~FIF_LARVAL;  \
} while (/*CONSTCOND*/0)

/*
 * Flags for fo_read and fo_write.
 */
#define FOF_UPDATE_OFFSET 0x01      /* update the file offset */

__BEGIN_DECLS

int   fdcreate    (CHAR16 *, UINT32, UINT32, BOOLEAN, VOID *, const struct fileops *);

/* Commonly used fileops
      fnullop_*   Does nothing and returns success.
      fbadop_*    Does nothing and returns EPERM
*/
int     EFIAPI fnullop_fcntl (struct __filedes *filp, UINT32 Cmd, void *p3, void *p4);
short   EFIAPI fnullop_poll  (struct __filedes *filp, short Events);
int     EFIAPI fnullop_flush (struct __filedes *filp);

int     EFIAPI fbadop_stat   (struct __filedes *filp, struct stat *StatBuf, void *Buf);
int     EFIAPI fbadop_ioctl  (struct __filedes *filp, ULONGN Cmd, va_list argp);
int     EFIAPI fbadop_delete (struct __filedes *filp);
int     EFIAPI fbadop_rmdir  (struct __filedes *filp);
int     EFIAPI fbadop_mkdir  (const char *path, __mode_t perms);
int     EFIAPI fbadop_rename (const char *from, const char *to);

__END_DECLS

/* From the original file... */
#if 0

//struct proc;
//struct lwp;
//struct uio;
//struct iovec;
//struct knote;

//LIST_HEAD(filelist, file);
//extern struct filelist  filehead;   /* head of list of open files */
//extern int              maxfiles;   /* kernel limit on # of open files */
//extern int              nfiles;     /* actual number of open files */

//extern const struct fileops vnops;  /* vnode operations for files */

struct fileops {
  int (*fo_read)      (struct file *, off_t *, struct uio *, kauth_cred_t, int);
  int (*fo_write)     (struct file *, off_t *, struct uio *, kauth_cred_t, int);
  int (*fo_ioctl)     (struct file *, u_long, void *, struct lwp *);
  int (*fo_fcntl)     (struct file *, u_int, void *, struct lwp *);
  int (*fo_poll)      (struct file *, int, struct lwp *);
  int (*fo_stat)      (struct file *, struct stat *, struct lwp *);
  int (*fo_close)     (struct file *, struct lwp *);
};

/*
 * Kernel descriptor table.
 * One entry for each open kernel vnode and socket.
 */
struct file {
  LIST_ENTRY(file)        f_list;     /* list of active files */
  void                   *f_data;     /* descriptor data, e.g. vnode/socket */
  const struct fileops   *f_ops;
  void                   *f_DevDesc;  /* Device Descriptor pointer */
  EFI_FILE_HANDLE         FileHandle;
  EFI_HANDLE              Handle;
  off_t                   f_offset;   /* current position in file */
  int                     f_flag;     /* see fcntl.h */
  UINT32                  f_iflags;   /* internal flags; FIF_* */
  int                     f_advice;   /* access pattern hint; UVM_ADV_* */
  int                     f_type;     /* descriptor type */
  int                     f_usecount; /* number active users */
  u_int                   f_count;    /* reference count */
  u_int                   f_msgcount; /* references from message queue */
//  kauth_cred_t            f_cred;     /* creds associated with descriptor */
  struct simplelock       f_slock;
  UINT16                  MyFD;       /* Which FD this is. */
};

#ifdef DIAGNOSTIC
#define FILE_USE_CHECK(fp, str)   \
  do {                              \
  if ((fp)->f_usecount < 0)       \
    panic(str);                   \
} while (/* CONSTCOND */ 0)
#else
#define FILE_USE_CHECK(fp, str)   /* nothing */
#endif

  /*
   * FILE_USE() must be called with the file lock held.
   * (Typical usage is: `fp = fd_getfile(..); FILE_USE(fp);'
   * and fd_getfile() returns the file locked)
   *
   * fp is a pointer to a __filedes structure.
   */
#define FILE_USE(fp)                                \
    do {                                                \
    (fp)->f_usecount++;                               \
    FILE_USE_CHECK((fp), "f_usecount overflow");      \
    simple_unlock(&(fp)->f_slock);                    \
  } while (/* CONSTCOND */ 0)

#define FILE_UNUSE_WLOCK(fp, l, havelock)           \
      do {                                                \
      if (!(havelock))                                  \
        simple_lock(&(fp)->f_slock);                    \
        if ((fp)->f_iflags & FIF_WANTCLOSE) {             \
        simple_unlock(&(fp)->f_slock);                  \
        /* Will drop usecount */                        \
        (void) closef((fp), (l));                       \
        break;                                          \
        } else {                                          \
        (fp)->f_usecount--;                             \
        FILE_USE_CHECK((fp), "f_usecount underflow");   \
      }                                                 \
      simple_unlock(&(fp)->f_slock);                    \
    } while (/* CONSTCOND */ 0)

#define FILE_UNUSE(fp, l)           FILE_UNUSE_WLOCK(fp, l, 0)
#define FILE_UNUSE_HAVELOCK(fp, l)  FILE_UNUSE_WLOCK(fp, l, 1)

__BEGIN_DECLS
//int   dofileread  (struct lwp *, int, struct file *, void *, size_t, off_t *, int, register_t *);
//int   dofilewrite (struct lwp *, int, struct file *, const void *, size_t, off_t *, int, register_t *);

//int   dofilereadv (struct lwp *, int, struct file *, const struct iovec *, int, off_t *, int, register_t *);
//int   dofilewritev(struct lwp *, int, struct file *, const struct iovec *, int, off_t *, int, register_t *);

//int   fsetown     (struct proc *, pid_t *, int, const void *);
//int   fgetown     (struct proc *, pid_t, int, void *);
//void  fownsignal  (pid_t, int, int, int, void *);

//int   fdclone     (struct lwp *, struct file *, int, int, const struct fileops *, void *);

/* Commonly used fileops
      fnullop_*   Does nothing and returns success.
      fbadop_*    Does nothing and returns EPERM
*/
//int   fnullop_fcntl   (struct file *, u_int, void *, struct lwp *);
//int   fnullop_poll    (struct file *, int, struct lwp *);
//int   fnullop_kqfilter(struct file *, struct knote *);
//int   fbadop_stat     (struct file *, struct stat *, struct lwp *);
//int   fbadop_ioctl    (struct file *, u_long, void *, struct lwp *);
__END_DECLS

#endif

#endif /* _PIF_KFILE_H_ */
