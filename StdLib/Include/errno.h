/** @file
  The header <errno.h> defines several macros, all relating to the reporting of
  error conditions.

  The ISO/IEC 9899 specification requires that these be macros.

  The macros expand to integral constant expressions
  with distinct nonzero values, suitable for use in #if preprocessing
  directives; the variable errno which expands to a modifiable lvalue that has type int,
  the value of which is set to a positive error number by several library
  functions; and the variable EFIerrno which is an extension allowing the return status
  of the underlying UEFI functions to be returned.

  The value of errno and EFIerrno is zero at program startup.  On program startup, errno
  is initialized to zero but is never set to zero by
  any library function.  The value of errno may be set to a non-zero value by
  a library function call whether or not there is an error, provided the use
  of errno is not documented in the description of the function in
  the governing standard: ISO/IEC 9899:1990 with Amendment 1 or ISO/IEC 9899:199409.

  EFIerrno, like errno, should only be checked if it is known that the preceeding function call
  called a UEFI function.  Functions in which UEFI functions are called dependent upon context
  or parameter values should guarantee that EFIerrno is set to zero by default, or to the status
  value returned by any UEFI functions which are called.

  All macro definitions in this list must begin with the letter 'E'
  and be followed by a digit or an uppercase letter.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifndef _ERRNO_H
#define _ERRNO_H
#include  <sys/EfiCdefs.h>
#include  <sys/errno.h>

extern  int             errno;
extern  RETURN_STATUS   EFIerrno;

// Define error number in terms of the ENUM in <sys/errno.h>

#define ERESTART          -1                      /* restart syscall */
#define ESUCCESS          __ESUCCESS              /* No Problems */

#define EMINERRORVAL      __EMINERRORVAL          /*  1   The lowest valid error value */

#define EPERM             __EPERM                 /*  1   Operation not permitted */
#define ENOENT            __ENOENT                /*  2   No such file or directory */
#define ESRCH             __ESRCH                 /*  3   No such process */
#define EINTR             __EINTR                 /*  4   Interrupted system call */
#define EIO               __EIO                   /*  5   Input/output error */
#define ENXIO             __ENXIO                 /*  6   Device not configured */
#define E2BIG             __E2BIG                 /*  7   Argument list too long */
#define ENOEXEC           __ENOEXEC               /*  8   Exec format error */
#define EBADF             __EBADF                 /*  9   Bad file descriptor */
#define ECHILD            __ECHILD                /* 10   No child processes */
#define EDEADLK           __EDEADLK               /* 11   Resource deadlock avoided */
#define ENOMEM            __ENOMEM                /* 12   Cannot allocate memory */
#define EACCES            __EACCES                /* 13   Permission denied */
#define EFAULT            __EFAULT                /* 14   Bad address */
#define ENOTBLK           __ENOTBLK               /* 15   Block device required */
#define EBUSY             __EBUSY                 /* 16   Device busy */
#define EEXIST            __EEXIST                /* 17   File exists */
#define EXDEV             __EXDEV                 /* 18   Cross-device link */
#define ENODEV            __ENODEV                /* 19   Operation not supported by device */
#define ENOTDIR           __ENOTDIR               /* 20   Not a directory */
#define EISDIR            __EISDIR                /* 21   Is a directory */
#define EINVAL            __EINVAL                /* 22   Invalid argument */
#define ENFILE            __ENFILE                /* 23   Too many open files in system */
#define EMFILE            __EMFILE                /* 24   Too many open file descriptors */
#define ENOTTY            __ENOTTY                /* 25   Inappropriate ioctl for device */
#define ETXTBSY           __ETXTBSY               /* 26   Text file busy */
#define EFBIG             __EFBIG                 /* 27   File too large */
#define ENOSPC            __ENOSPC                /* 28   No space left on device */
#define ESPIPE            __ESPIPE                /* 29   Illegal seek */
#define EROFS             __EROFS                 /* 30   Read-only filesystem */
#define EMLINK            __EMLINK                /* 31   Too many links */
#define EPIPE             __EPIPE                 /* 32   Broken pipe */

/* math software -- these are the only two values required by the C Standard */
#define EDOM              __EDOM                  /* 33   Numerical argument out of domain */
#define ERANGE            __ERANGE                /* 34   Result too large */

/* non-blocking and interrupt i/o */
#define EAGAIN            __EAGAIN                /* 35   Resource temporarily unavailable */
#define EWOULDBLOCK       __EWOULDBLOCK           /* 35   Operation would block */
#define EINPROGRESS       __EINPROGRESS           /* 36   Operation now in progress */
#define EALREADY          __EALREADY              /* 37   Operation already in progress */

/* ipc/network software -- argument errors */
#define ENOTSOCK          __ENOTSOCK              /* 38   Socket operation on non-socket */
#define EDESTADDRREQ      __EDESTADDRREQ          /* 39   Destination address required */
#define EMSGSIZE          __EMSGSIZE              /* 40   Message too long */
#define EPROTOTYPE        __EPROTOTYPE            /* 41   Protocol wrong type for socket */
#define ENOPROTOOPT       __ENOPROTOOPT           /* 42   Protocol not available */
#define EPROTONOSUPPORT   __EPROTONOSUPPORT       /* 43   Protocol not supported */
#define ESOCKTNOSUPPORT   __ESOCKTNOSUPPORT       /* 44   Socket type not supported */
#define EOPNOTSUPP        __EOPNOTSUPP            /* 45   Operation not supported */
#define ENOTSUP           __ENOTSUP               /* 45   Operation not supported */
#define EPFNOSUPPORT      __EPFNOSUPPORT          /* 46   Protocol family not supported */
#define EAFNOSUPPORT      __EAFNOSUPPORT          /* 47   Address family not supported by protocol family */
#define EADDRINUSE        __EADDRINUSE            /* 48   Address already in use */
#define EADDRNOTAVAIL     __EADDRNOTAVAIL         /* 49   Can't assign requested address */

/* ipc/network software -- operational errors */
#define ENETDOWN          __ENETDOWN              /* 50   Network is down */
#define ENETUNREACH       __ENETUNREACH           /* 51   Network is unreachable */
#define ENETRESET         __ENETRESET             /* 52   Network dropped connection on reset */
#define ECONNABORTED      __ECONNABORTED          /* 53   Software caused connection abort */
#define ECONNRESET        __ECONNRESET            /* 54   Connection reset by peer */
#define ENOBUFS           __ENOBUFS               /* 55   No buffer space available */
#define EISCONN           __EISCONN               /* 56   Socket is already connected */
#define ENOTCONN          __ENOTCONN              /* 57   Socket is not connected */
#define ESHUTDOWN         __ESHUTDOWN             /* 58   Can't send after socket shutdown */
#define ETOOMANYREFS      __ETOOMANYREFS          /* 59   Too many references: can't splice */
#define ETIMEDOUT         __ETIMEDOUT             /* 60   Operation timed out */
#define ECONNREFUSED      __ECONNREFUSED          /* 61   Connection refused */
#define ELOOP             __ELOOP                 /* 62   Too many levels of symbolic links */
#define ENAMETOOLONG      __ENAMETOOLONG          /* 63   File name too long */
#define EHOSTDOWN         __EHOSTDOWN             /* 64   Host is down */
#define EHOSTUNREACH      __EHOSTUNREACH          /* 65   No route to host */

#define ENOTEMPTY         __ENOTEMPTY             /* 66   Directory not empty */

/* quotas, etc. */
#define EPROCLIM          __EPROCLIM              /* 67   Too many processes */
#define EUSERS            __EUSERS                /* 68   Too many users */
#define EDQUOT            __EDQUOT                /* 69   Disc quota exceeded */

/* Network File System */
#define ESTALE            __ESTALE                /* 70   Stale NFS file handle */
#define EREMOTE           __EREMOTE               /* 71   Too many levels of remote in path */
#define EBADRPC           __EBADRPC               /* 72   RPC struct is bad */
#define ERPCMISMATCH      __ERPCMISMATCH          /* 73   RPC version wrong */
#define EPROGUNAVAIL      __EPROGUNAVAIL          /* 74   RPC prog. not avail */
#define EPROGMISMATCH     __EPROGMISMATCH         /* 75   Program version wrong */
#define EPROCUNAVAIL      __EPROCUNAVAIL          /* 76   Bad procedure for program */
#define ENOLCK            __ENOLCK                /* 77   No locks available */
#define ENOSYS            __ENOSYS                /* 78   Function not implemented */
#define EFTYPE            __EFTYPE                /* 79   Inappropriate file type or format */
#define EAUTH             __EAUTH                 /* 80   Authentication error */
#define ENEEDAUTH         __ENEEDAUTH             /* 81   Need authenticator */
#define EIDRM             __EIDRM                 /* 82   Identifier removed */
#define ENOMSG            __ENOMSG                /* 83   No message of desired type */
#define EOVERFLOW         __EOVERFLOW             /* 84   Value too large to be stored in data type */
#define EILSEQ            __EILSEQ                /* 85   Illegal byte sequence */
#define ENOTHING_1        __ENOTHING_1            /* 86   Place Holder */
#define ECANCELED         __ECANCELED             /* 87   Operation canceled */

#define EBADMSG           __EBADMSG               /* 88   Bad message */
#define ENODATA           __ENODATA               /* 89   No message available */
#define ENOSR             __ENOSR                 /* 90   No STREAM resources */
#define ENOSTR            __ENOSTR                /* 91   Not a STREAM */
#define ETIME             __ETIME                 /* 92   STREAM ioctl timeout */

#define ENOATTR           __ENOATTR               /* 93   Attribute not found */

#define EDOOFUS           __EDOOFUS               /* 94   Programming error */

#define EMULTIHOP         __EMULTIHOP             /* 95   Multihop attempted */
#define ENOLINK           __ENOLINK               /* 96   Link has been severed */
#define EPROTO            __EPROTO                /* 97   Protocol error */

#define EBUFSIZE          __EBUFSIZE              /* 98   Buffer too small to hold result */

#define EMAXERRORVAL      __EMAXERRORVAL          /* One more than the highest defined error value. */

#endif  /* _ERRNO_H */
