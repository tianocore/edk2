/** @file
  The header <sys/errno.h> defines several values, all relating to the reporting of
  error conditions.

  The enum members expand to integral constant expressions
  with distinct nonzero values, suitable for use in #if preprocessing
  directives.  These default values are specified as an enum in order to ease
  the maintenance of the values.

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#ifdef  _ERRNO_H          // May only be included from <errno.h>
#ifndef _SYS_ERRNO_H
#define _SYS_ERRNO_H

/* Define the error numbers, sequentially, beginning at 1. */
enum {
  __ESUCCESS      = 0,              /*  0   For those rare times one needs to say all is OK */
  __EMINERRORVAL  = 1,              /* The lowest valid error value */
  __EPERM         = __EMINERRORVAL, /*  1   Operation not permitted */
  __ENOENT,                         /*  2   No such file or directory */
  __ESRCH,                          /*  3   No such process */
  __EINTR,                          /*  4   Interrupted system call */
  __EIO,                            /*  5   Input/output error */
  __ENXIO,                          /*  6   Device not configured */
  __E2BIG,                          /*  7   Argument list too long */
  __ENOEXEC,                        /*  8   Exec format error */
  __EBADF,                          /*  9   Bad file descriptor */
  __ECHILD,                         /* 10   No child processes */
  __EDEADLK,                        /* 11   Resource deadlock avoided */
  __ENOMEM,                         /* 12   Cannot allocate memory */
  __EACCES,                         /* 13   Permission denied */
  __EFAULT,                         /* 14   Bad address */
  __ENOTBLK,                        /* 15   Block device required */
  __EBUSY,                          /* 16   Device busy */
  __EEXIST,                         /* 17   File exists */
  __EXDEV,                          /* 18   Cross-device link */
  __ENODEV,                         /* 19   Operation not supported by device */
  __ENOTDIR,                        /* 20   Not a directory */
  __EISDIR,                         /* 21   Is a directory */
  __EINVAL,                         /* 22   Invalid argument */
  __ENFILE,                         /* 23   Too many open files in system */
  __EMFILE,                         /* 24   Too many open file descriptors */
  __ENOTTY,                         /* 25   Inappropriate ioctl for device */
  __ETXTBSY,                        /* 26   Text file busy */
  __EFBIG,                          /* 27   File too large */
  __ENOSPC,                         /* 28   No space left on device */
  __ESPIPE,                         /* 29   Illegal seek */
  __EROFS,                          /* 30   Read-only filesystem */
  __EMLINK,                         /* 31   Too many links */
  __EPIPE,                          /* 32   Broken pipe */

  /* math software -- these are the only two values required by the C Standard */
  __EDOM,                           /* 33   Numerical argument out of domain */
  __ERANGE,                         /* 34   Result too large */

  /* non-blocking and interrupt i/o */
  __EAGAIN,                         /* 35   Resource temporarily unavailable */
  __EWOULDBLOCK     = __EAGAIN,     /* 35   Operation would block */
  __EINPROGRESS,                    /* 36   Operation now in progress */
  __EALREADY,                       /* 37   Operation already in progress */

  /* ipc/network software -- argument errors */
  __ENOTSOCK,                       /* 38   Socket operation on non-socket */
  __EDESTADDRREQ,                   /* 39   Destination address required */
  __EMSGSIZE,                       /* 40   Message too long */
  __EPROTOTYPE,                     /* 41   Protocol wrong type for socket */
  __ENOPROTOOPT,                    /* 42   Protocol not available */
  __EPROTONOSUPPORT,                /* 43   Protocol not supported */
  __ESOCKTNOSUPPORT,                /* 44   Socket type not supported */
  __EOPNOTSUPP,                     /* 45   Operation not supported */
  __ENOTSUP         = __EOPNOTSUPP, /* 45   Operation not supported */
  __EPFNOSUPPORT,                   /* 46   Protocol family not supported */
  __EAFNOSUPPORT,                   /* 47   Address family not supported by protocol family */
  __EADDRINUSE,                     /* 48   Address already in use */
  __EADDRNOTAVAIL,                  /* 49   Can't assign requested address */

  /* ipc/network software -- operational errors */
  __ENETDOWN,                       /* 50   Network is down */
  __ENETUNREACH,                    /* 51   Network is unreachable */
  __ENETRESET,                      /* 52   Network dropped connection on reset */
  __ECONNABORTED,                   /* 53   Software caused connection abort */
  __ECONNRESET,                     /* 54   Connection reset by peer */
  __ENOBUFS,                        /* 55   No buffer space available */
  __EISCONN,                        /* 56   Socket is already connected */
  __ENOTCONN,                       /* 57   Socket is not connected */
  __ESHUTDOWN,                      /* 58   Can't send after socket shutdown */
  __ETOOMANYREFS,                   /* 59   Too many references: can't splice */
  __ETIMEDOUT,                      /* 60   Operation timed out */
  __ECONNREFUSED,                   /* 61   Connection refused */
  __ELOOP,                          /* 62   Too many levels of symbolic links */
  __ENAMETOOLONG,                   /* 63   File name too long */
  __EHOSTDOWN,                      /* 64   Host is down */
  __EHOSTUNREACH,                   /* 65   No route to host */

  __ENOTEMPTY,                      /* 66   Directory not empty */

  /* quotas, etc. */
  __EPROCLIM,                       /* 67   Too many processes */
  __EUSERS,                         /* 68   Too many users */
  __EDQUOT,                         /* 69   Disc quota exceeded */

  /* Network File System */
  __ESTALE,                         /* 70   Stale NFS file handle */
  __EREMOTE,                        /* 71   Too many levels of remote in path */
  __EBADRPC,                        /* 72   RPC struct is bad */
  __ERPCMISMATCH,                   /* 73   RPC version wrong */
  __EPROGUNAVAIL,                   /* 74   RPC prog. not avail */
  __EPROGMISMATCH,                  /* 75   Program version wrong */
  __EPROCUNAVAIL,                   /* 76   Bad procedure for program */
  __ENOLCK,                         /* 77   No locks available */
  __ENOSYS,                         /* 78   Function not implemented */
  __EFTYPE,                         /* 79   Inappropriate file type or format */
  __EAUTH,                          /* 80   Authentication error */
  __ENEEDAUTH,                      /* 81   Need authenticator */
  __EIDRM,                          /* 82   Identifier removed */
  __ENOMSG,                         /* 83   No message of desired type */
  __EOVERFLOW,                      /* 84   Value too large to be stored in data type */
  __EILSEQ,                         /* 85   Illegal byte sequence */
  __ENOTHING_1,                     /* 86   Place Holder */
  __ECANCELED,                      /* 87   Operation canceled */

  __EBADMSG,                        /* 88   Bad message */
  __ENODATA,                        /* 89   No message available */
  __ENOSR,                          /* 90   No STREAM resources */
  __ENOSTR,                         /* 91   Not a STREAM */
  __ETIME,                          /* 92   STREAM ioctl timeout */

  __ENOATTR,                        /* 93   Attribute not found */

  __EDOOFUS,                        /* 94   Programming error */

  __EMULTIHOP,                      /* 95   Multihop attempted */
  __ENOLINK,                        /* 96   Link has been severed */
  __EPROTO,                         /* 97   Protocol error */

  __EBUFSIZE,                       /* 98   Buffer too small to hold result */

  __EMAXERRORVAL                    /* One more than the highest defined error value. */
};

#endif  /* _SYS_ERRNO_H */
#else   /* not defined _ERRNO_H */
#error  <sys/errno.h> must only be included by <errno.h>.
#endif  /* _ERRNO_H */
