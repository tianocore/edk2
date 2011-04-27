/** @file
    This header defines the human readable descriptions of the errors declared
    in errno.h.

    The string literals defined in this file must be kept in sync with the
    error numbers declared in <errno.h>.  This is because the error numbers are
    used to index into the sys_errlist array to retrieve its associated
    string literal.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

/* Describe the error numbers, sequentially, beginning at 0. */
const char *const
sys_errlist[] = {
  "No Error Detected",                            /*  0   errno reset or no error yet detected */
  "Operation not permitted",                      /*  1   EPERM */
  "No such file or directory",                    /*  2   ENOENT */
  "No such process",                              /*  3   ESRCH */
  "Interrupted system call",                      /*  4   EINTR */
  "Input/output error",                           /*  5   EIO */
  "Device not configured",                        /*  6   ENXIO */
  "Argument list too long",                       /*  7   E2BIG */
  "Exec format error",                            /*  8   ENOEXEC */
  "Bad file descriptor",                          /*  9   EBADF */
  "No child processes",                           /* 10   ECHILD */
  "Resource deadlock avoided",                    /* 11   EDEADLK */
  "Cannot allocate memory",                       /* 12   ENOMEM */
  "Permission denied",                            /* 13   EACCES */
  "Bad address",                                  /* 14   EFAULT */
  "Block device required",                        /* 15   ENOTBLK */
  "Device busy",                                  /* 16   EBUSY */
  "File exists",                                  /* 17   EEXIST */
  "Cross-device link",                            /* 18   EXDEV */
  "Operation not supported by device",            /* 19   ENODEV */
  "Not a directory",                              /* 20   ENOTDIR */
  "Is a directory",                               /* 21   EISDIR */
  "Invalid argument",                             /* 22   EINVAL */
  "Too many open files in system",                /* 23   ENFILE */
  "Too many open files",                          /* 24   EMFILE */
  "Inappropriate ioctl for device",               /* 25   ENOTTY */
  "Text file busy",                               /* 26   ETXTBSY */
  "File too large",                               /* 27   EFBIG */
  "No space left on device",                      /* 28   ENOSPC */
  "Illegal seek",                                 /* 29   ESPIPE */
  "Read-only filesystem",                         /* 30   EROFS */
  "Too many links",                               /* 31   EMLINK */
  "Broken pipe",                                  /* 32   EPIPE */

  /* math software -- these are the only two values required by the C Standard */
  "Numerical argument out of domain",             /* 33   EDOM */
  "Result too large",                             /* 34   ERANGE */

  /* non-blocking and interrupt i/o */
  "Resource temporarily unavailable",             /* 35   EAGAIN or EWOULDBLOCK */
  "Operation now in progress",                    /* 36   EINPROGRESS */
  "Operation already in progress",                /* 37   EALREADY */

  /* ipc/network software -- argument errors */
  "Socket operation on non-socket",                   /* 38   ENOTSOCK */
  "Destination address required",                     /* 39   EDESTADDRREQ */
  "Message too long",                                 /* 40   EMSGSIZE */
  "Protocol wrong type for socket",                   /* 41   EPROTOTYPE */
  "Protocol not available",                           /* 42   ENOPROTOOPT */
  "Protocol not supported",                           /* 43   EPROTONOSUPPORT */
  "Socket type not supported",                        /* 44   ESOCKTNOSUPPORT */
  "Operation not supported",                          /* 45   EOPNOTSUPP or ENOTSUP */
  "Protocol family not supported",                    /* 46   EPFNOSUPPORT */
  "Address family not supported by protocol family",  /* 47   EAFNOSUPPORT */
  "Address already in use",                           /* 48   EADDRINUSE */
  "Can't assign requested address",                   /* 49   EADDRNOTAVAIL */

  /* ipc/network software -- operational errors */
  "Network is down",                              /* 50   ENETDOWN */
  "Network is unreachable",                       /* 51   ENETUNREACH */
  "Network dropped connection on reset",          /* 52   ENETRESET */
  "Software caused connection abort",             /* 53   ECONNABORTED */
  "Connection reset by peer",                     /* 54   ECONNRESET */
  "No buffer space available",                    /* 55   ENOBUFS */
  "Socket is already connected",                  /* 56   EISCONN */
  "Socket is not connected",                      /* 57   ENOTCONN */
  "Can't send after socket shutdown",             /* 58   ESHUTDOWN */
  "Too many references: can't splice",            /* 59   ETOOMANYREFS */
  "Operation timed out",                          /* 60   ETIMEDOUT */
  "Connection refused",                           /* 61   ECONNREFUSED */
  "Too many levels of symbolic links",            /* 62   ELOOP */
  "File name too long",                           /* 63   ENAMETOOLONG */
  "Host is down",                                 /* 64   EHOSTDOWN */
  "No route to host",                             /* 65   EHOSTUNREACH */
  "Directory not empty",                          /* 66   ENOTEMPTY */

  /* quotas, etc. */
  "Too many processes",                           /* 67   EPROCLIM */
  "Too many users",                               /* 68   EUSERS */
  "Disc quota exceeded",                          /* 69   EDQUOT */

  /* Network File System */
  "Stale NFS file handle",                        /* 70   ESTALE */
  "Too many levels of remote in path",            /* 71   EREMOTE */
  "RPC struct is bad",                            /* 72   EBADRPC */
  "RPC version wrong",                            /* 73   ERPCMISMATCH */
  "RPC prog. not avail",                          /* 74   EPROGUNAVAIL */
  "Program version wrong",                        /* 75   EPROGMISMATCH */
  "Bad procedure for program",                    /* 76   EPROCUNAVAIL */
  "No locks available",                           /* 77   ENOLCK */
  "Function not implemented",                     /* 78   ENOSYS */
  "Inappropriate file type or format",            /* 79   EFTYPE */
  "Authentication error",                         /* 80   EAUTH */
  "Need authenticator",                           /* 81   ENEEDAUTH */
  "Identifier removed",                           /* 82   EIDRM */
  "No message of desired type",                   /* 83   ENOMSG */
  "Value too large to be stored in data type",    /* 84   EOVERFLOW */
  "Illegal byte sequence",                        /* 85   EILSEQ */
  "Bad errno 86",                                 /* 86   ENOTHING_1 */
  "Operation canceled",                           /* 87   ECANCELED */

  "Bad message",                                  /* 88   EBADMSG */
  "No message available",                         /* 89   ENODATA */
  "No STREAM resources",                          /* 90   ENOSR  */
  "Not a STREAM",                                 /* 91   ENOSTR */
  "STREAM ioctl timeout",                         /* 92   ETIME  */

  "Attribute not found",                          /* 93   ENOATTR */

  "Programming error",                            /* 94   EDOOFUS */

  "Multihop attempted",                           /* 95   EMULTIHOP */
  "Link has been severed",                        /* 96   ENOLINK */
  "Protocol error",                               /* 97   EPROTO */

  "Buffer too small to hold result",              /* 98   EBUFSIZE */

  "System Error list and errno.h are out-of-sync" /* EMAXERRORVAL - Should always be last. */
};
