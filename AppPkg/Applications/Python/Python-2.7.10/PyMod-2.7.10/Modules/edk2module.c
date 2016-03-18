/** @file
    OS-specific module implementation for EDK II and UEFI.
    Derived from posixmodule.c in Python 2.7.2.

    Copyright (c) 2015, Daryl McDaniel. All rights reserved.<BR>
    Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structseq.h"

#include  <stdio.h>
#include  <stdlib.h>
#include  <wchar.h>
#include  <sys/syslimits.h>

#ifdef __cplusplus
extern "C" {
#endif

PyDoc_STRVAR(edk2__doc__,
             "This module provides access to UEFI firmware functionality that is\n\
             standardized by the C Standard and the POSIX standard (a thinly\n\
             disguised Unix interface).  Refer to the library manual and\n\
             corresponding UEFI Specification entries for more information on calls.");

#ifndef Py_USING_UNICODE
  /* This is used in signatures of functions. */
  #define Py_UNICODE void
#endif

#ifdef HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_WAIT_H
  #include <sys/wait.h>           /* For WNOHANG */
#endif

#ifdef HAVE_SIGNAL_H
  #include <signal.h>
#endif

#ifdef HAVE_FCNTL_H
  #include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_GRP_H
  #include <grp.h>
#endif

#ifdef HAVE_SYSEXITS_H
  #include <sysexits.h>
#endif /* HAVE_SYSEXITS_H */

#ifdef HAVE_SYS_LOADAVG_H
  #include <sys/loadavg.h>
#endif

#ifdef HAVE_UTIME_H
  #include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
  #include <sys/utime.h>
  #define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#ifdef HAVE_SYS_TIMES_H
  #include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#ifdef HAVE_SYS_PARAM_H
  #include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_UTSNAME_H
  #include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#ifdef HAVE_DIRENT_H
  #include <dirent.h>
  #define NAMLEN(dirent) wcslen((dirent)->FileName)
#else
  #define dirent direct
  #define NAMLEN(dirent) (dirent)->d_namlen
  #ifdef HAVE_SYS_NDIR_H
    #include <sys/ndir.h>
  #endif
  #ifdef HAVE_SYS_DIR_H
    #include <sys/dir.h>
  #endif
  #ifdef HAVE_NDIR_H
    #include <ndir.h>
  #endif
#endif

#ifndef MAXPATHLEN
  #if defined(PATH_MAX) && PATH_MAX > 1024
    #define MAXPATHLEN PATH_MAX
  #else
    #define MAXPATHLEN 1024
  #endif
#endif /* MAXPATHLEN */

#define WAIT_TYPE int
#define WAIT_STATUS_INT(s) (s)

/* Issue #1983: pid_t can be longer than a C long on some systems */
#if !defined(SIZEOF_PID_T) || SIZEOF_PID_T == SIZEOF_INT
  #define PARSE_PID "i"
  #define PyLong_FromPid PyInt_FromLong
  #define PyLong_AsPid PyInt_AsLong
#elif SIZEOF_PID_T == SIZEOF_LONG
  #define PARSE_PID "l"
  #define PyLong_FromPid PyInt_FromLong
  #define PyLong_AsPid PyInt_AsLong
#elif defined(SIZEOF_LONG_LONG) && SIZEOF_PID_T == SIZEOF_LONG_LONG
  #define PARSE_PID "L"
  #define PyLong_FromPid PyLong_FromLongLong
  #define PyLong_AsPid PyInt_AsLongLong
#else
  #error "sizeof(pid_t) is neither sizeof(int), sizeof(long) or sizeof(long long)"
#endif /* SIZEOF_PID_T */

/* Don't use the "_r" form if we don't need it (also, won't have a
   prototype for it, at least on Solaris -- maybe others as well?). */
#if defined(HAVE_CTERMID_R) && defined(WITH_THREAD)
  #define USE_CTERMID_R
#endif

#if defined(HAVE_TMPNAM_R) && defined(WITH_THREAD)
  #define USE_TMPNAM_R
#endif

/* choose the appropriate stat and fstat functions and return structs */
#undef STAT
#undef FSTAT
#undef STRUCT_STAT
#define STAT stat
#define FSTAT fstat
#define STRUCT_STAT struct stat

/* dummy version. _PyVerify_fd() is already defined in fileobject.h */
#define _PyVerify_fd_dup2(A, B) (1)

#ifndef UEFI_C_SOURCE
/* Return a dictionary corresponding to the POSIX environment table */
extern char **environ;

static PyObject *
convertenviron(void)
{
    PyObject *d;
    char **e;
    d = PyDict_New();
    if (d == NULL)
        return NULL;
    if (environ == NULL)
        return d;
    /* This part ignores errors */
    for (e = environ; *e != NULL; e++) {
        PyObject *k;
        PyObject *v;
        char *p = strchr(*e, '=');
        if (p == NULL)
            continue;
        k = PyString_FromStringAndSize(*e, (int)(p-*e));
        if (k == NULL) {
            PyErr_Clear();
            continue;
        }
        v = PyString_FromString(p+1);
        if (v == NULL) {
            PyErr_Clear();
            Py_DECREF(k);
            continue;
        }
        if (PyDict_GetItem(d, k) == NULL) {
            if (PyDict_SetItem(d, k, v) != 0)
                PyErr_Clear();
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
    return d;
}
#endif  /* UEFI_C_SOURCE */

/* Set a POSIX-specific error from errno, and return NULL */

static PyObject *
edk2_error(void)
{
    return PyErr_SetFromErrno(PyExc_OSError);
}
static PyObject *
edk2_error_with_filename(char* name)
{
    return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
}


static PyObject *
edk2_error_with_allocated_filename(char* name)
{
    PyObject *rc = PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
    PyMem_Free(name);
    return rc;
}

/* POSIX generic methods */

#ifndef UEFI_C_SOURCE
  static PyObject *
  edk2_fildes(PyObject *fdobj, int (*func)(int))
  {
      int fd;
      int res;
      fd = PyObject_AsFileDescriptor(fdobj);
      if (fd < 0)
          return NULL;
      if (!_PyVerify_fd(fd))
          return edk2_error();
      Py_BEGIN_ALLOW_THREADS
      res = (*func)(fd);
      Py_END_ALLOW_THREADS
      if (res < 0)
          return edk2_error();
      Py_INCREF(Py_None);
      return Py_None;
  }
#endif  /* UEFI_C_SOURCE */

static PyObject *
edk2_1str(PyObject *args, char *format, int (*func)(const char*))
{
    char *path1 = NULL;
    int res;
    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path1))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path1);
    PyMem_Free(path1);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
edk2_2str(PyObject *args,
           char *format,
           int (*func)(const char *, const char *))
{
    char *path1 = NULL, *path2 = NULL;
    int res;
    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path1,
                          Py_FileSystemDefaultEncoding, &path2))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1, path2);
    Py_END_ALLOW_THREADS
    PyMem_Free(path1);
    PyMem_Free(path2);
    if (res != 0)
        /* XXX how to report both path1 and path2??? */
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,\n\
or st_flags, they are available as attributes only.\n\
\n\
See os.stat for more information.");

static PyStructSequence_Field stat_result_fields[] = {
    {"st_mode",    "protection bits"},
    //{"st_ino",     "inode"},
    //{"st_dev",     "device"},
    //{"st_nlink",   "number of hard links"},
    //{"st_uid",     "user ID of owner"},
    //{"st_gid",     "group ID of owner"},
    {"st_size",    "total size, in bytes"},
    /* The NULL is replaced with PyStructSequence_UnnamedField later. */
    {NULL,   "integer time of last access"},
    {NULL,   "integer time of last modification"},
    {NULL,   "integer time of last change"},
    {"st_atime",   "time of last access"},
    {"st_mtime",   "time of last modification"},
    {"st_ctime",   "time of last change"},
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    {"st_blksize", "blocksize for filesystem I/O"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    {"st_blocks",  "number of blocks allocated"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    {"st_rdev",    "device type (if inode device)"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    {"st_flags",   "user defined flags for file"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    {"st_gen",    "generation number"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {"st_birthtime",   "time of creation"},
#endif
    {0}
};

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
#define ST_BLKSIZE_IDX 8
#else
#define ST_BLKSIZE_IDX 12
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
#define ST_BLOCKS_IDX (ST_BLKSIZE_IDX+1)
#else
#define ST_BLOCKS_IDX ST_BLKSIZE_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
#define ST_RDEV_IDX (ST_BLOCKS_IDX+1)
#else
#define ST_RDEV_IDX ST_BLOCKS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_FLAGS
#define ST_FLAGS_IDX (ST_RDEV_IDX+1)
#else
#define ST_FLAGS_IDX ST_RDEV_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_GEN
#define ST_GEN_IDX (ST_FLAGS_IDX+1)
#else
#define ST_GEN_IDX ST_FLAGS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
#define ST_BIRTHTIME_IDX (ST_GEN_IDX+1)
#else
#define ST_BIRTHTIME_IDX ST_GEN_IDX
#endif

static PyStructSequence_Desc stat_result_desc = {
    "stat_result", /* name */
    stat_result__doc__, /* doc */
    stat_result_fields,
    10
};

#ifndef UEFI_C_SOURCE   /* Not in UEFI */
PyDoc_STRVAR(statvfs_result__doc__,
"statvfs_result: Result from statvfs or fstatvfs.\n\n\
This object may be accessed either as a tuple of\n\
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),\n\
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.\n\
\n\
See os.statvfs for more information.");

static PyStructSequence_Field statvfs_result_fields[] = {
    {"f_bsize",  },
    {"f_frsize", },
    {"f_blocks", },
    {"f_bfree",  },
    {"f_bavail", },
    {"f_files",  },
    {"f_ffree",  },
    {"f_favail", },
    {"f_flag",   },
    {"f_namemax",},
    {0}
};

static PyStructSequence_Desc statvfs_result_desc = {
    "statvfs_result", /* name */
    statvfs_result__doc__, /* doc */
    statvfs_result_fields,
    10
};

static PyTypeObject StatVFSResultType;
#endif

static int initialized;
static PyTypeObject StatResultType;
static newfunc structseq_new;

static PyObject *
statresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyStructSequence *result;
    int i;

    result = (PyStructSequence*)structseq_new(type, args, kwds);
    if (!result)
        return NULL;
    /* If we have been initialized from a tuple,
       st_?time might be set to None. Initialize it
       from the int slots.  */
    for (i = 7; i <= 9; i++) {
        if (result->ob_item[i+3] == Py_None) {
            Py_DECREF(Py_None);
            Py_INCREF(result->ob_item[i]);
            result->ob_item[i+3] = result->ob_item[i];
        }
    }
    return (PyObject*)result;
}



/* If true, st_?time is float. */
#if defined(UEFI_C_SOURCE)
  static int _stat_float_times = 0;
#else
  static int _stat_float_times = 1;

PyDoc_STRVAR(stat_float_times__doc__,
"stat_float_times([newval]) -> oldval\n\n\
Determine whether os.[lf]stat represents time stamps as float objects.\n\
If newval is True, future calls to stat() return floats, if it is False,\n\
future calls return ints. \n\
If newval is omitted, return the current setting.\n");

static PyObject*
stat_float_times(PyObject* self, PyObject *args)
{
    int newval = -1;

    if (!PyArg_ParseTuple(args, "|i:stat_float_times", &newval))
        return NULL;
    if (newval == -1)
        /* Return old value */
        return PyBool_FromLong(_stat_float_times);
    _stat_float_times = newval;
    Py_INCREF(Py_None);
    return Py_None;
}
#endif  /* UEFI_C_SOURCE */

static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
    PyObject *fval,*ival;
#if SIZEOF_TIME_T > SIZEOF_LONG
    ival = PyLong_FromLongLong((PY_LONG_LONG)sec);
#else
    ival = PyInt_FromLong((long)sec);
#endif
    if (!ival)
        return;
    if (_stat_float_times) {
        fval = PyFloat_FromDouble(sec + 1e-9*nsec);
    } else {
        fval = ival;
        Py_INCREF(fval);
    }
    PyStructSequence_SET_ITEM(v, index, ival);
    PyStructSequence_SET_ITEM(v, index+3, fval);
}

/* pack a system stat C structure into the Python stat tuple
   (used by edk2_stat() and edk2_fstat()) */
static PyObject*
_pystat_fromstructstat(STRUCT_STAT *st)
{
    unsigned long ansec, mnsec, cnsec;
    PyObject *v = PyStructSequence_New(&StatResultType);
    if (v == NULL)
        return NULL;

    PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long)st->st_mode));
    PyStructSequence_SET_ITEM(v, 1,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_size));

    ansec = mnsec = cnsec = 0;
    /* The index used by fill_time is the index of the integer time.
       fill_time will add 3 to the index to get the floating time index.
    */
    fill_time(v, 2, st->st_atime, ansec);
    fill_time(v, 3, st->st_mtime, mnsec);
    fill_time(v, 4, st->st_mtime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
                              PyInt_FromLong((long)st->st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
                              PyInt_FromLong((long)st->st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
                              PyInt_FromLong((long)st->st_rdev));
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    PyStructSequence_SET_ITEM(v, ST_GEN_IDX,
                              PyInt_FromLong((long)st->st_gen));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {
      PyObject *val;
      unsigned long bsec,bnsec;
      bsec = (long)st->st_birthtime;
#ifdef HAVE_STAT_TV_NSEC2
      bnsec = st->st_birthtimespec.tv_nsec;
#else
      bnsec = 0;
#endif
      if (_stat_float_times) {
        val = PyFloat_FromDouble(bsec + 1e-9*bnsec);
      } else {
        val = PyInt_FromLong((long)bsec);
      }
      PyStructSequence_SET_ITEM(v, ST_BIRTHTIME_IDX,
                                val);
    }
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    PyStructSequence_SET_ITEM(v, ST_FLAGS_IDX,
                              PyInt_FromLong((long)st->st_flags));
#endif

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}

static PyObject *
edk2_do_stat(PyObject *self, PyObject *args,
              char *format,
              int (*statfunc)(const char *, STRUCT_STAT *),
              char *wformat,
              int (*wstatfunc)(const Py_UNICODE *, STRUCT_STAT *))
{
    STRUCT_STAT st;
    char *path = NULL;          /* pass this to stat; do not free() it */
    char *pathfree = NULL;  /* this memory must be free'd */
    int res;
    PyObject *result;

    if (!PyArg_ParseTuple(args, format,
                          Py_FileSystemDefaultEncoding, &path))
        return NULL;
    pathfree = path;

    Py_BEGIN_ALLOW_THREADS
    res = (*statfunc)(path, &st);
    Py_END_ALLOW_THREADS

    if (res != 0) {
        result = edk2_error_with_filename(pathfree);
    }
    else
        result = _pystat_fromstructstat(&st);

    PyMem_Free(pathfree);
    return result;
}

/* POSIX methods */

PyDoc_STRVAR(edk2_access__doc__,
"access(path, mode) -> True if granted, False otherwise\n\n\
Use the real uid/gid to test for access to a path.  Note that most\n\
operations will use the effective uid/gid, therefore this routine can\n\
be used in a suid/sgid environment to test if the invoking user has the\n\
specified access to the path.  The mode argument can be F_OK to test\n\
existence, or the inclusive-OR of R_OK, W_OK, and X_OK.");

static PyObject *
edk2_access(PyObject *self, PyObject *args)
{
    char *path;
    int mode;

    int res;
    if (!PyArg_ParseTuple(args, "eti:access",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = access(path, mode);
    Py_END_ALLOW_THREADS
    PyMem_Free(path);
    return PyBool_FromLong(res == 0);
}

#ifndef F_OK
  #define F_OK 0
#endif
#ifndef R_OK
  #define R_OK 4
#endif
#ifndef W_OK
  #define W_OK 2
#endif
#ifndef X_OK
  #define X_OK 1
#endif

PyDoc_STRVAR(edk2_chdir__doc__,
"chdir(path)\n\n\
Change the current working directory to the specified path.");

static PyObject *
edk2_chdir(PyObject *self, PyObject *args)
{
    return edk2_1str(args, "et:chdir", chdir);
}

PyDoc_STRVAR(edk2_chmod__doc__,
"chmod(path, mode)\n\n\
Change the access permissions of a file.");

static PyObject *
edk2_chmod(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int i;
    int res;
    if (!PyArg_ParseTuple(args, "eti:chmod", Py_FileSystemDefaultEncoding,
                          &path, &i))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = chmod(path, i);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef HAVE_FCHMOD
PyDoc_STRVAR(edk2_fchmod__doc__,
"fchmod(fd, mode)\n\n\
Change the access permissions of the file given by file\n\
descriptor fd.");

static PyObject *
edk2_fchmod(PyObject *self, PyObject *args)
{
    int fd, mode, res;
    if (!PyArg_ParseTuple(args, "ii:fchmod", &fd, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fchmod(fd, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHMOD */

#ifdef HAVE_LCHMOD
PyDoc_STRVAR(edk2_lchmod__doc__,
"lchmod(path, mode)\n\n\
Change the access permissions of a file. If path is a symlink, this\n\
affects the link itself rather than the target.");

static PyObject *
edk2_lchmod(PyObject *self, PyObject *args)
{
    char *path = NULL;
    int i;
    int res;
    if (!PyArg_ParseTuple(args, "eti:lchmod", Py_FileSystemDefaultEncoding,
                          &path, &i))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = lchmod(path, i);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_RETURN_NONE;
}
#endif /* HAVE_LCHMOD */


#ifdef HAVE_CHFLAGS
PyDoc_STRVAR(edk2_chflags__doc__,
"chflags(path, flags)\n\n\
Set file flags.");

static PyObject *
edk2_chflags(PyObject *self, PyObject *args)
{
    char *path;
    unsigned long flags;
    int res;
    if (!PyArg_ParseTuple(args, "etk:chflags",
                          Py_FileSystemDefaultEncoding, &path, &flags))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = chflags(path, flags);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_CHFLAGS */

#ifdef HAVE_LCHFLAGS
PyDoc_STRVAR(edk2_lchflags__doc__,
"lchflags(path, flags)\n\n\
Set file flags.\n\
This function will not follow symbolic links.");

static PyObject *
edk2_lchflags(PyObject *self, PyObject *args)
{
    char *path;
    unsigned long flags;
    int res;
    if (!PyArg_ParseTuple(args, "etk:lchflags",
                          Py_FileSystemDefaultEncoding, &path, &flags))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = lchflags(path, flags);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_LCHFLAGS */

#ifdef HAVE_CHROOT
PyDoc_STRVAR(edk2_chroot__doc__,
"chroot(path)\n\n\
Change root directory to path.");

static PyObject *
edk2_chroot(PyObject *self, PyObject *args)
{
    return edk2_1str(args, "et:chroot", chroot);
}
#endif

#ifdef HAVE_FSYNC
PyDoc_STRVAR(edk2_fsync__doc__,
"fsync(fildes)\n\n\
force write of file with filedescriptor to disk.");

static PyObject *
edk2_fsync(PyObject *self, PyObject *fdobj)
{
    return edk2_fildes(fdobj, fsync);
}
#endif /* HAVE_FSYNC */

#ifdef HAVE_FDATASYNC

#ifdef __hpux
extern int fdatasync(int); /* On HP-UX, in libc but not in unistd.h */
#endif

PyDoc_STRVAR(edk2_fdatasync__doc__,
"fdatasync(fildes)\n\n\
force write of file with filedescriptor to disk.\n\
 does not force update of metadata.");

static PyObject *
edk2_fdatasync(PyObject *self, PyObject *fdobj)
{
    return edk2_fildes(fdobj, fdatasync);
}
#endif /* HAVE_FDATASYNC */


#ifdef HAVE_CHOWN
PyDoc_STRVAR(edk2_chown__doc__,
"chown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.");

static PyObject *
edk2_chown(PyObject *self, PyObject *args)
{
    char *path = NULL;
    long uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "etll:chown",
                          Py_FileSystemDefaultEncoding, &path,
                          &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = chown(path, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_CHOWN */

#ifdef HAVE_FCHOWN
PyDoc_STRVAR(edk2_fchown__doc__,
"fchown(fd, uid, gid)\n\n\
Change the owner and group id of the file given by file descriptor\n\
fd to the numeric uid and gid.");

static PyObject *
edk2_fchown(PyObject *self, PyObject *args)
{
    int fd;
    long uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "ill:chown", &fd, &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fchown(fd, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHOWN */

#ifdef HAVE_LCHOWN
PyDoc_STRVAR(edk2_lchown__doc__,
"lchown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
This function will not follow symbolic links.");

static PyObject *
edk2_lchown(PyObject *self, PyObject *args)
{
    char *path = NULL;
    long uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "etll:lchown",
                          Py_FileSystemDefaultEncoding, &path,
                          &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = lchown(path, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_LCHOWN */


#ifdef HAVE_GETCWD
PyDoc_STRVAR(edk2_getcwd__doc__,
"getcwd() -> path\n\n\
Return a string representing the current working directory.");

static PyObject *
edk2_getcwd(PyObject *self, PyObject *noargs)
{
    int bufsize_incr = 1024;
    int bufsize = 0;
    char *tmpbuf = NULL;
    char *res = NULL;
    PyObject *dynamic_return;

    Py_BEGIN_ALLOW_THREADS
    do {
        bufsize = bufsize + bufsize_incr;
        tmpbuf = malloc(bufsize);
        if (tmpbuf == NULL) {
            break;
        }
        res = getcwd(tmpbuf, bufsize);
        if (res == NULL) {
            free(tmpbuf);
        }
    } while ((res == NULL) && (errno == ERANGE));
    Py_END_ALLOW_THREADS

    if (res == NULL)
        return edk2_error();

    dynamic_return = PyString_FromString(tmpbuf);
    free(tmpbuf);

    return dynamic_return;
}

#ifdef Py_USING_UNICODE
PyDoc_STRVAR(edk2_getcwdu__doc__,
"getcwdu() -> path\n\n\
Return a unicode string representing the current working directory.");

static PyObject *
edk2_getcwdu(PyObject *self, PyObject *noargs)
{
    char buf[1026];
    char *res;

    Py_BEGIN_ALLOW_THREADS
    res = getcwd(buf, sizeof buf);
    Py_END_ALLOW_THREADS
    if (res == NULL)
        return edk2_error();
    return PyUnicode_Decode(buf, strlen(buf), Py_FileSystemDefaultEncoding,"strict");
}
#endif /* Py_USING_UNICODE */
#endif /* HAVE_GETCWD */


PyDoc_STRVAR(edk2_listdir__doc__,
"listdir(path) -> list_of_strings\n\n\
Return a list containing the names of the entries in the directory.\n\
\n\
    path: path of directory to list\n\
\n\
The list is in arbitrary order.  It does not include the special\n\
entries '.' and '..' even if they are present in the directory.");

static PyObject *
edk2_listdir(PyObject *self, PyObject *args)
{
    /* XXX Should redo this putting the (now four) versions of opendir
       in separate files instead of having them all here... */

    char           *name            = NULL;
    char           *MBname;
    PyObject       *d, *v;
    DIR            *dirp;
    struct dirent  *ep;
    int             arg_is_unicode  = 1;

    errno = 0;
    if (!PyArg_ParseTuple(args, "U:listdir", &v)) {
        arg_is_unicode = 0;
        PyErr_Clear();
    }
    if (!PyArg_ParseTuple(args, "et:listdir", Py_FileSystemDefaultEncoding, &name))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    dirp = opendir(name);
    Py_END_ALLOW_THREADS
    if (dirp == NULL) {
        return edk2_error_with_allocated_filename(name);
    }
    if ((d = PyList_New(0)) == NULL) {
        Py_BEGIN_ALLOW_THREADS
        closedir(dirp);
        Py_END_ALLOW_THREADS
        PyMem_Free(name);
        return NULL;
    }
    if((MBname = malloc(NAME_MAX)) == NULL) {
      Py_BEGIN_ALLOW_THREADS
      closedir(dirp);
      Py_END_ALLOW_THREADS
      Py_DECREF(d);
      PyMem_Free(name);
      return NULL;
    }
    for (;;) {
        errno = 0;
        Py_BEGIN_ALLOW_THREADS
        ep = readdir(dirp);
        Py_END_ALLOW_THREADS
        if (ep == NULL) {
            if ((errno == 0) || (errno == EISDIR)) {
                break;
            } else {
                Py_BEGIN_ALLOW_THREADS
                closedir(dirp);
                Py_END_ALLOW_THREADS
                Py_DECREF(d);
                return edk2_error_with_allocated_filename(name);
            }
        }
        if (ep->FileName[0] == L'.' &&
            (NAMLEN(ep) == 1 ||
             (ep->FileName[1] == L'.' && NAMLEN(ep) == 2)))
            continue;
        if(wcstombs(MBname, ep->FileName, NAME_MAX) == -1) {
          free(MBname);
          Py_BEGIN_ALLOW_THREADS
          closedir(dirp);
          Py_END_ALLOW_THREADS
          Py_DECREF(d);
          PyMem_Free(name);
          return NULL;
        }
        v = PyString_FromStringAndSize(MBname, strlen(MBname));
        if (v == NULL) {
            Py_DECREF(d);
            d = NULL;
            break;
        }
#ifdef Py_USING_UNICODE
        if (arg_is_unicode) {
            PyObject *w;

            w = PyUnicode_FromEncodedObject(v,
                                            Py_FileSystemDefaultEncoding,
                                            "strict");
            if (w != NULL) {
                Py_DECREF(v);
                v = w;
            }
            else {
                /* fall back to the original byte string, as
                   discussed in patch #683592 */
                PyErr_Clear();
            }
        }
#endif
        if (PyList_Append(d, v) != 0) {
            Py_DECREF(v);
            Py_DECREF(d);
            d = NULL;
            break;
        }
        Py_DECREF(v);
    }
    Py_BEGIN_ALLOW_THREADS
    closedir(dirp);
    Py_END_ALLOW_THREADS
    PyMem_Free(name);
    if(MBname != NULL) {
      free(MBname);
    }

    return d;

}  /* end of edk2_listdir */

PyDoc_STRVAR(edk2_mkdir__doc__,
"mkdir(path [, mode=0777])\n\n\
Create a directory.");

static PyObject *
edk2_mkdir(PyObject *self, PyObject *args)
{
    int res;
    char *path = NULL;
    int mode = 0777;

    if (!PyArg_ParseTuple(args, "et|i:mkdir",
                          Py_FileSystemDefaultEncoding, &path, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = mkdir(path, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error_with_allocated_filename(path);
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
}


/* sys/resource.h is needed for at least: wait3(), wait4(), broken nice. */
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif


#ifdef HAVE_NICE
PyDoc_STRVAR(edk2_nice__doc__,
"nice(inc) -> new_priority\n\n\
Decrease the priority of process by inc and return the new priority.");

static PyObject *
edk2_nice(PyObject *self, PyObject *args)
{
    int increment, value;

    if (!PyArg_ParseTuple(args, "i:nice", &increment))
        return NULL;

    /* There are two flavours of 'nice': one that returns the new
       priority (as required by almost all standards out there) and the
       Linux/FreeBSD/BSDI one, which returns '0' on success and advices
       the use of getpriority() to get the new priority.

       If we are of the nice family that returns the new priority, we
       need to clear errno before the call, and check if errno is filled
       before calling edk2_error() on a returnvalue of -1, because the
       -1 may be the actual new priority! */

    errno = 0;
    value = nice(increment);
#if defined(HAVE_BROKEN_NICE) && defined(HAVE_GETPRIORITY)
    if (value == 0)
        value = getpriority(PRIO_PROCESS, 0);
#endif
    if (value == -1 && errno != 0)
        /* either nice() or getpriority() returned an error */
        return edk2_error();
    return PyInt_FromLong((long) value);
}
#endif /* HAVE_NICE */

PyDoc_STRVAR(edk2_rename__doc__,
"rename(old, new)\n\n\
Rename a file or directory.");

static PyObject *
edk2_rename(PyObject *self, PyObject *args)
{
    return edk2_2str(args, "etet:rename", rename);
}


PyDoc_STRVAR(edk2_rmdir__doc__,
"rmdir(path)\n\n\
Remove a directory.");

static PyObject *
edk2_rmdir(PyObject *self, PyObject *args)
{
    return edk2_1str(args, "et:rmdir", rmdir);
}


PyDoc_STRVAR(edk2_stat__doc__,
"stat(path) -> stat result\n\n\
Perform a stat system call on the given path.");

static PyObject *
edk2_stat(PyObject *self, PyObject *args)
{
    return edk2_do_stat(self, args, "et:stat", STAT, NULL, NULL);
}


#ifdef HAVE_SYSTEM
PyDoc_STRVAR(edk2_system__doc__,
"system(command) -> exit_status\n\n\
Execute the command (a string) in a subshell.");

static PyObject *
edk2_system(PyObject *self, PyObject *args)
{
    char *command;
    long sts;
    if (!PyArg_ParseTuple(args, "s:system", &command))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    sts = system(command);
    Py_END_ALLOW_THREADS
    return PyInt_FromLong(sts);
}
#endif


PyDoc_STRVAR(edk2_umask__doc__,
"umask(new_mask) -> old_mask\n\n\
Set the current numeric umask and return the previous umask.");

static PyObject *
edk2_umask(PyObject *self, PyObject *args)
{
    int i;
    if (!PyArg_ParseTuple(args, "i:umask", &i))
        return NULL;
    i = (int)umask(i);
    if (i < 0)
        return edk2_error();
    return PyInt_FromLong((long)i);
}


PyDoc_STRVAR(edk2_unlink__doc__,
"unlink(path)\n\n\
Remove a file (same as remove(path)).");

PyDoc_STRVAR(edk2_remove__doc__,
"remove(path)\n\n\
Remove a file (same as unlink(path)).");

static PyObject *
edk2_unlink(PyObject *self, PyObject *args)
{
    return edk2_1str(args, "et:remove", unlink);
}


static int
extract_time(PyObject *t, time_t* sec, long* usec)
{
    time_t intval;
    if (PyFloat_Check(t)) {
        double tval = PyFloat_AsDouble(t);
        PyObject *intobj = PyNumber_Long(t);
        if (!intobj)
            return -1;
#if SIZEOF_TIME_T > SIZEOF_LONG
        intval = PyInt_AsUnsignedLongLongMask(intobj);
#else
        intval = PyInt_AsLong(intobj);
#endif
        Py_DECREF(intobj);
        if (intval == -1 && PyErr_Occurred())
            return -1;
        *sec = intval;
        *usec = (long)((tval - intval) * 1e6); /* can't exceed 1000000 */
        if (*usec < 0)
            /* If rounding gave us a negative number,
               truncate.  */
            *usec = 0;
        return 0;
    }
#if SIZEOF_TIME_T > SIZEOF_LONG
    intval = PyInt_AsUnsignedLongLongMask(t);
#else
    intval = PyInt_AsLong(t);
#endif
    if (intval == -1 && PyErr_Occurred())
        return -1;
    *sec = intval;
    *usec = 0;
    return 0;
}

PyDoc_STRVAR(edk2_utime__doc__,
"utime(path, (atime, mtime))\n\
utime(path, None)\n\n\
Set the access and modified time of the file to the given values.  If the\n\
second form is used, set the access and modified times to the current time.");

static PyObject *
edk2_utime(PyObject *self, PyObject *args)
{
    char *path = NULL;
    time_t atime, mtime;
    long ausec, musec;
    int res;
    PyObject* arg;

#if defined(HAVE_UTIMES)
    struct timeval buf[2];
#define ATIME buf[0].tv_sec
#define MTIME buf[1].tv_sec
#elif defined(HAVE_UTIME_H)
/* XXX should define struct utimbuf instead, above */
    struct utimbuf buf;
#define ATIME buf.actime
#define MTIME buf.modtime
#define UTIME_ARG &buf
#else /* HAVE_UTIMES */
    time_t buf[2];
#define ATIME buf[0]
#define MTIME buf[1]
#define UTIME_ARG buf
#endif /* HAVE_UTIMES */


    if (!PyArg_ParseTuple(args, "etO:utime",
                          Py_FileSystemDefaultEncoding, &path, &arg))
        return NULL;
    if (arg == Py_None) {
        /* optional time values not given */
        Py_BEGIN_ALLOW_THREADS
        res = utime(path, NULL);
        Py_END_ALLOW_THREADS
    }
    else if (!PyTuple_Check(arg) || PyTuple_Size(arg) != 2) {
        PyErr_SetString(PyExc_TypeError,
                        "utime() arg 2 must be a tuple (atime, mtime)");
        PyMem_Free(path);
        return NULL;
    }
    else {
        if (extract_time(PyTuple_GET_ITEM(arg, 0),
                         &atime, &ausec) == -1) {
            PyMem_Free(path);
            return NULL;
        }
        if (extract_time(PyTuple_GET_ITEM(arg, 1),
                         &mtime, &musec) == -1) {
            PyMem_Free(path);
            return NULL;
        }
        ATIME = atime;
        MTIME = mtime;
#ifdef HAVE_UTIMES
        buf[0].tv_usec = ausec;
        buf[1].tv_usec = musec;
        Py_BEGIN_ALLOW_THREADS
        res = utimes(path, buf);
        Py_END_ALLOW_THREADS
#else
        Py_BEGIN_ALLOW_THREADS
        res = utime(path, UTIME_ARG);
        Py_END_ALLOW_THREADS
#endif /* HAVE_UTIMES */
    }
    if (res < 0) {
        return edk2_error_with_allocated_filename(path);
    }
    PyMem_Free(path);
    Py_INCREF(Py_None);
    return Py_None;
#undef UTIME_ARG
#undef ATIME
#undef MTIME
}


/* Process operations */

PyDoc_STRVAR(edk2__exit__doc__,
"_exit(status)\n\n\
Exit to the system with specified status, without normal exit processing.");

static PyObject *
edk2__exit(PyObject *self, PyObject *args)
{
    int sts;
    if (!PyArg_ParseTuple(args, "i:_exit", &sts))
        return NULL;
    _Exit(sts);
    return NULL; /* Make gcc -Wall happy */
}

#if defined(HAVE_EXECV) || defined(HAVE_SPAWNV)
static void
free_string_array(char **array, Py_ssize_t count)
{
    Py_ssize_t i;
    for (i = 0; i < count; i++)
        PyMem_Free(array[i]);
    PyMem_DEL(array);
}
#endif


#ifdef HAVE_EXECV
PyDoc_STRVAR(edk2_execv__doc__,
"execv(path, args)\n\n\
Execute an executable path with arguments, replacing current process.\n\
\n\
    path: path of executable file\n\
    args: tuple or list of strings");

static PyObject *
edk2_execv(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv;
    char **argvlist;
    Py_ssize_t i, argc;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* execv has two arguments: (path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "etO:execv",
                          Py_FileSystemDefaultEncoding,
                          &path, &argv))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "execv() arg 2 must be a tuple or list");
        PyMem_Free(path);
        return NULL;
    }
    if (argc < 1) {
        PyErr_SetString(PyExc_ValueError, "execv() arg 2 must not be empty");
        PyMem_Free(path);
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyMem_Free(path);
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i), "et",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(PyExc_TypeError,
                            "execv() arg 2 must contain only strings");
            PyMem_Free(path);
            return NULL;

        }
    }
    argvlist[argc] = NULL;

    execv(path, argvlist);

    /* If we get here it's definitely an error */

    free_string_array(argvlist, argc);
    PyMem_Free(path);
    return edk2_error();
}


PyDoc_STRVAR(edk2_execve__doc__,
"execve(path, args, env)\n\n\
Execute a path with arguments and environment, replacing current process.\n\
\n\
    path: path of executable file\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings");

static PyObject *
edk2_execve(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv, *env;
    char **argvlist;
    char **envlist;
    PyObject *key, *val, *keys=NULL, *vals=NULL;
    Py_ssize_t i, pos, argc, envc;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    Py_ssize_t lastarg = 0;

    /* execve has three arguments: (path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    if (!PyArg_ParseTuple(args, "etOO:execve",
                          Py_FileSystemDefaultEncoding,
                          &path, &argv, &env))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "execve() arg 2 must be a tuple or list");
        goto fail_0;
    }
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "execve() arg 3 must be a mapping object");
        goto fail_0;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        goto fail_0;
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i),
                         "et;execve() arg 2 must contain only strings",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i]))
        {
            lastarg = i;
            goto fail_1;
        }
    }
    lastarg = argc;
    argvlist[argc] = NULL;

    i = PyMapping_Size(env);
    if (i < 0)
        goto fail_1;
    envlist = PyMem_NEW(char *, i + 1);
    if (envlist == NULL) {
        PyErr_NoMemory();
        goto fail_1;
    }
    envc = 0;
    keys = PyMapping_Keys(env);
    vals = PyMapping_Values(env);
    if (!keys || !vals)
        goto fail_2;
    if (!PyList_Check(keys) || !PyList_Check(vals)) {
        PyErr_SetString(PyExc_TypeError,
                        "execve(): env.keys() or env.values() is not a list");
        goto fail_2;
    }

    for (pos = 0; pos < i; pos++) {
        char *p, *k, *v;
        size_t len;

        key = PyList_GetItem(keys, pos);
        val = PyList_GetItem(vals, pos);
        if (!key || !val)
            goto fail_2;

        if (!PyArg_Parse(
                    key,
                    "s;execve() arg 3 contains a non-string key",
                    &k) ||
            !PyArg_Parse(
                val,
                "s;execve() arg 3 contains a non-string value",
                &v))
        {
            goto fail_2;
        }

#if defined(PYOS_OS2)
        /* Omit Pseudo-Env Vars that Would Confuse Programs if Passed On */
        if (stricmp(k, "BEGINLIBPATH") != 0 && stricmp(k, "ENDLIBPATH") != 0) {
#endif
        len = PyString_Size(key) + PyString_Size(val) + 2;
        p = PyMem_NEW(char, len);
        if (p == NULL) {
            PyErr_NoMemory();
            goto fail_2;
        }
        PyOS_snprintf(p, len, "%s=%s", k, v);
        envlist[envc++] = p;
#if defined(PYOS_OS2)
        }
#endif
    }
    envlist[envc] = 0;

    execve(path, argvlist, envlist);

    /* If we get here it's definitely an error */

    (void) edk2_error();

  fail_2:
    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
    Py_XDECREF(vals);
    Py_XDECREF(keys);
  fail_0:
    PyMem_Free(path);
    return NULL;
}
#endif /* HAVE_EXECV */


#ifdef HAVE_SPAWNV
PyDoc_STRVAR(edk2_spawnv__doc__,
"spawnv(mode, path, args)\n\n\
Execute the program 'path' in a new process.\n\
\n\
    mode: mode of process creation\n\
    path: path of executable file\n\
    args: tuple or list of strings");

static PyObject *
edk2_spawnv(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv;
    char **argvlist;
    int mode, i;
    Py_ssize_t argc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* spawnv has three arguments: (mode, path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "ietO:spawnv", &mode,
                          Py_FileSystemDefaultEncoding,
                          &path, &argv))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnv() arg 2 must be a tuple or list");
        PyMem_Free(path);
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyMem_Free(path);
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i), "et",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(
                PyExc_TypeError,
                "spawnv() arg 2 must contain only strings");
            PyMem_Free(path);
            return NULL;
        }
    }
    argvlist[argc] = NULL;

#if defined(PYOS_OS2) && defined(PYCC_GCC)
    Py_BEGIN_ALLOW_THREADS
    spawnval = spawnv(mode, path, argvlist);
    Py_END_ALLOW_THREADS
#else
    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnv(mode, path, argvlist);
    Py_END_ALLOW_THREADS
#endif

    free_string_array(argvlist, argc);
    PyMem_Free(path);

    if (spawnval == -1)
        return edk2_error();
    else
#if SIZEOF_LONG == SIZEOF_VOID_P
        return Py_BuildValue("l", (long) spawnval);
#else
        return Py_BuildValue("L", (PY_LONG_LONG) spawnval);
#endif
}


PyDoc_STRVAR(edk2_spawnve__doc__,
"spawnve(mode, path, args, env)\n\n\
Execute the program 'path' in a new process.\n\
\n\
    mode: mode of process creation\n\
    path: path of executable file\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings");

static PyObject *
edk2_spawnve(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv, *env;
    char **argvlist;
    char **envlist;
    PyObject *key, *val, *keys=NULL, *vals=NULL, *res=NULL;
    int mode, pos, envc;
    Py_ssize_t argc, i;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    Py_ssize_t lastarg = 0;

    /* spawnve has four arguments: (mode, path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    if (!PyArg_ParseTuple(args, "ietOO:spawnve", &mode,
                          Py_FileSystemDefaultEncoding,
                          &path, &argv, &env))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnve() arg 2 must be a tuple or list");
        goto fail_0;
    }
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnve() arg 3 must be a mapping object");
        goto fail_0;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        goto fail_0;
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i),
                     "et;spawnve() arg 2 must contain only strings",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i]))
        {
            lastarg = i;
            goto fail_1;
        }
    }
    lastarg = argc;
    argvlist[argc] = NULL;

    i = PyMapping_Size(env);
    if (i < 0)
        goto fail_1;
    envlist = PyMem_NEW(char *, i + 1);
    if (envlist == NULL) {
        PyErr_NoMemory();
        goto fail_1;
    }
    envc = 0;
    keys = PyMapping_Keys(env);
    vals = PyMapping_Values(env);
    if (!keys || !vals)
        goto fail_2;
    if (!PyList_Check(keys) || !PyList_Check(vals)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnve(): env.keys() or env.values() is not a list");
        goto fail_2;
    }

    for (pos = 0; pos < i; pos++) {
        char *p, *k, *v;
        size_t len;

        key = PyList_GetItem(keys, pos);
        val = PyList_GetItem(vals, pos);
        if (!key || !val)
            goto fail_2;

        if (!PyArg_Parse(
                    key,
                    "s;spawnve() arg 3 contains a non-string key",
                    &k) ||
            !PyArg_Parse(
                val,
                "s;spawnve() arg 3 contains a non-string value",
                &v))
        {
            goto fail_2;
        }
        len = PyString_Size(key) + PyString_Size(val) + 2;
        p = PyMem_NEW(char, len);
        if (p == NULL) {
            PyErr_NoMemory();
            goto fail_2;
        }
        PyOS_snprintf(p, len, "%s=%s", k, v);
        envlist[envc++] = p;
    }
    envlist[envc] = 0;

#if defined(PYOS_OS2) && defined(PYCC_GCC)
    Py_BEGIN_ALLOW_THREADS
    spawnval = spawnve(mode, path, argvlist, envlist);
    Py_END_ALLOW_THREADS
#else
    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnve(mode, path, argvlist, envlist);
    Py_END_ALLOW_THREADS
#endif

    if (spawnval == -1)
        (void) edk2_error();
    else
#if SIZEOF_LONG == SIZEOF_VOID_P
        res = Py_BuildValue("l", (long) spawnval);
#else
        res = Py_BuildValue("L", (PY_LONG_LONG) spawnval);
#endif

  fail_2:
    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
    Py_XDECREF(vals);
    Py_XDECREF(keys);
  fail_0:
    PyMem_Free(path);
    return res;
}

/* OS/2 supports spawnvp & spawnvpe natively */
#if defined(PYOS_OS2)
PyDoc_STRVAR(edk2_spawnvp__doc__,
"spawnvp(mode, file, args)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
    mode: mode of process creation\n\
    file: executable file name\n\
    args: tuple or list of strings");

static PyObject *
edk2_spawnvp(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv;
    char **argvlist;
    int mode, i, argc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* spawnvp has three arguments: (mode, path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "ietO:spawnvp", &mode,
                          Py_FileSystemDefaultEncoding,
                          &path, &argv))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvp() arg 2 must be a tuple or list");
        PyMem_Free(path);
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyMem_Free(path);
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i), "et",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(
                PyExc_TypeError,
                "spawnvp() arg 2 must contain only strings");
            PyMem_Free(path);
            return NULL;
        }
    }
    argvlist[argc] = NULL;

    Py_BEGIN_ALLOW_THREADS
#if defined(PYCC_GCC)
    spawnval = spawnvp(mode, path, argvlist);
#else
    spawnval = _spawnvp(mode, path, argvlist);
#endif
    Py_END_ALLOW_THREADS

    free_string_array(argvlist, argc);
    PyMem_Free(path);

    if (spawnval == -1)
        return edk2_error();
    else
        return Py_BuildValue("l", (long) spawnval);
}


PyDoc_STRVAR(edk2_spawnvpe__doc__,
"spawnvpe(mode, file, args, env)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
    mode: mode of process creation\n\
    file: executable file name\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings");

static PyObject *
edk2_spawnvpe(PyObject *self, PyObject *args)
{
    char *path;
    PyObject *argv, *env;
    char **argvlist;
    char **envlist;
    PyObject *key, *val, *keys=NULL, *vals=NULL, *res=NULL;
    int mode, i, pos, argc, envc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    int lastarg = 0;

    /* spawnvpe has four arguments: (mode, path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    if (!PyArg_ParseTuple(args, "ietOO:spawnvpe", &mode,
                          Py_FileSystemDefaultEncoding,
                          &path, &argv, &env))
        return NULL;
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvpe() arg 2 must be a tuple or list");
        goto fail_0;
    }
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvpe() arg 3 must be a mapping object");
        goto fail_0;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        goto fail_0;
    }
    for (i = 0; i < argc; i++) {
        if (!PyArg_Parse((*getitem)(argv, i),
                     "et;spawnvpe() arg 2 must contain only strings",
                         Py_FileSystemDefaultEncoding,
                         &argvlist[i]))
        {
            lastarg = i;
            goto fail_1;
        }
    }
    lastarg = argc;
    argvlist[argc] = NULL;

    i = PyMapping_Size(env);
    if (i < 0)
        goto fail_1;
    envlist = PyMem_NEW(char *, i + 1);
    if (envlist == NULL) {
        PyErr_NoMemory();
        goto fail_1;
    }
    envc = 0;
    keys = PyMapping_Keys(env);
    vals = PyMapping_Values(env);
    if (!keys || !vals)
        goto fail_2;
    if (!PyList_Check(keys) || !PyList_Check(vals)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvpe(): env.keys() or env.values() is not a list");
        goto fail_2;
    }

    for (pos = 0; pos < i; pos++) {
        char *p, *k, *v;
        size_t len;

        key = PyList_GetItem(keys, pos);
        val = PyList_GetItem(vals, pos);
        if (!key || !val)
            goto fail_2;

        if (!PyArg_Parse(
                    key,
                    "s;spawnvpe() arg 3 contains a non-string key",
                    &k) ||
            !PyArg_Parse(
                val,
                "s;spawnvpe() arg 3 contains a non-string value",
                &v))
        {
            goto fail_2;
        }
        len = PyString_Size(key) + PyString_Size(val) + 2;
        p = PyMem_NEW(char, len);
        if (p == NULL) {
            PyErr_NoMemory();
            goto fail_2;
        }
        PyOS_snprintf(p, len, "%s=%s", k, v);
        envlist[envc++] = p;
    }
    envlist[envc] = 0;

    Py_BEGIN_ALLOW_THREADS
#if defined(PYCC_GCC)
    spawnval = spawnvpe(mode, path, argvlist, envlist);
#else
    spawnval = _spawnvpe(mode, path, argvlist, envlist);
#endif
    Py_END_ALLOW_THREADS

    if (spawnval == -1)
        (void) edk2_error();
    else
        res = Py_BuildValue("l", (long) spawnval);

  fail_2:
    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
    Py_XDECREF(vals);
    Py_XDECREF(keys);
  fail_0:
    PyMem_Free(path);
    return res;
}
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */


#ifdef HAVE_FORK1
PyDoc_STRVAR(edk2_fork1__doc__,
"fork1() -> pid\n\n\
Fork a child process with a single multiplexed (i.e., not bound) thread.\n\
\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
edk2_fork1(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    int result = 0;
    _PyImport_AcquireLock();
    pid = fork1();
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return edk2_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return PyLong_FromPid(pid);
}
#endif


#ifdef HAVE_FORK
PyDoc_STRVAR(edk2_fork__doc__,
"fork() -> pid\n\n\
Fork a child process.\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
edk2_fork(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    int result = 0;
    _PyImport_AcquireLock();
    pid = fork();
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return edk2_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return PyLong_FromPid(pid);
}
#endif

/* AIX uses /dev/ptc but is otherwise the same as /dev/ptmx */
/* IRIX has both /dev/ptc and /dev/ptmx, use ptmx */
#if defined(HAVE_DEV_PTC) && !defined(HAVE_DEV_PTMX)
#define DEV_PTY_FILE "/dev/ptc"
#define HAVE_DEV_PTMX
#else
#define DEV_PTY_FILE "/dev/ptmx"
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX)
#ifdef HAVE_PTY_H
#include <pty.h>
#else
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#else
#ifdef HAVE_UTIL_H
#include <util.h>
#endif /* HAVE_UTIL_H */
#endif /* HAVE_LIBUTIL_H */
#endif /* HAVE_PTY_H */
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#endif /* defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX */

#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
PyDoc_STRVAR(edk2_openpty__doc__,
"openpty() -> (master_fd, slave_fd)\n\n\
Open a pseudo-terminal, returning open fd's for both master and slave end.\n");

static PyObject *
edk2_openpty(PyObject *self, PyObject *noargs)
{
    int master_fd, slave_fd;
#ifndef HAVE_OPENPTY
    char * slave_name;
#endif
#if defined(HAVE_DEV_PTMX) && !defined(HAVE_OPENPTY) && !defined(HAVE__GETPTY)
    PyOS_sighandler_t sig_saved;
#ifdef sun
    extern char *ptsname(int fildes);
#endif
#endif

#ifdef HAVE_OPENPTY
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) != 0)
        return edk2_error();
#elif defined(HAVE__GETPTY)
    slave_name = _getpty(&master_fd, O_RDWR, 0666, 0);
    if (slave_name == NULL)
        return edk2_error();

    slave_fd = open(slave_name, O_RDWR);
    if (slave_fd < 0)
        return edk2_error();
#else
    master_fd = open(DEV_PTY_FILE, O_RDWR | O_NOCTTY); /* open master */
    if (master_fd < 0)
        return edk2_error();
    sig_saved = PyOS_setsig(SIGCHLD, SIG_DFL);
    /* change permission of slave */
    if (grantpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        return edk2_error();
    }
    /* unlock slave */
    if (unlockpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        return edk2_error();
    }
    PyOS_setsig(SIGCHLD, sig_saved);
    slave_name = ptsname(master_fd); /* get name of slave */
    if (slave_name == NULL)
        return edk2_error();
    slave_fd = open(slave_name, O_RDWR | O_NOCTTY); /* open slave */
    if (slave_fd < 0)
        return edk2_error();
#if !defined(__CYGWIN__) && !defined(HAVE_DEV_PTC)
    ioctl(slave_fd, I_PUSH, "ptem"); /* push ptem */
    ioctl(slave_fd, I_PUSH, "ldterm"); /* push ldterm */
#ifndef __hpux
    ioctl(slave_fd, I_PUSH, "ttcompat"); /* push ttcompat */
#endif /* __hpux */
#endif /* HAVE_CYGWIN */
#endif /* HAVE_OPENPTY */

    return Py_BuildValue("(ii)", master_fd, slave_fd);

}
#endif /* defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX) */

#ifdef HAVE_FORKPTY
PyDoc_STRVAR(edk2_forkpty__doc__,
"forkpty() -> (pid, master_fd)\n\n\
Fork a new process with a new pseudo-terminal as controlling tty.\n\n\
Like fork(), return 0 as pid to child process, and PID of child to parent.\n\
To both, return fd of newly opened pseudo-terminal.\n");

static PyObject *
edk2_forkpty(PyObject *self, PyObject *noargs)
{
    int master_fd = -1, result = 0;
    pid_t pid;

    _PyImport_AcquireLock();
    pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return edk2_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return Py_BuildValue("(Ni)", PyLong_FromPid(pid), master_fd);
}
#endif

PyDoc_STRVAR(edk2_getpid__doc__,
"getpid() -> pid\n\n\
Return the current process id");

static PyObject *
edk2_getpid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromPid(getpid());
}


#ifdef HAVE_GETLOGIN
PyDoc_STRVAR(edk2_getlogin__doc__,
"getlogin() -> string\n\n\
Return the actual login name.");

static PyObject *
edk2_getlogin(PyObject *self, PyObject *noargs)
{
    PyObject *result = NULL;
    char *name;
    int old_errno = errno;

    errno = 0;
    name = getlogin();
    if (name == NULL) {
        if (errno)
        edk2_error();
        else
        PyErr_SetString(PyExc_OSError,
                        "unable to determine login name");
    }
    else
        result = PyString_FromString(name);
    errno = old_errno;

    return result;
}
#endif

#ifdef HAVE_KILL
PyDoc_STRVAR(edk2_kill__doc__,
"kill(pid, sig)\n\n\
Kill a process with a signal.");

static PyObject *
edk2_kill(PyObject *self, PyObject *args)
{
    pid_t pid;
    int sig;
    if (!PyArg_ParseTuple(args, PARSE_PID "i:kill", &pid, &sig))
        return NULL;
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
    if (sig == XCPT_SIGNAL_INTR || sig == XCPT_SIGNAL_BREAK) {
        APIRET rc;
        if ((rc = DosSendSignalException(pid, sig)) != NO_ERROR)
            return os2_error(rc);

    } else if (sig == XCPT_SIGNAL_KILLPROC) {
        APIRET rc;
        if ((rc = DosKillProcess(DKP_PROCESS, pid)) != NO_ERROR)
            return os2_error(rc);

    } else
        return NULL; /* Unrecognized Signal Requested */
#else
    if (kill(pid, sig) == -1)
        return edk2_error();
#endif
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_PLOCK

#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

PyDoc_STRVAR(edk2_plock__doc__,
"plock(op)\n\n\
Lock program segments into memory.");

static PyObject *
edk2_plock(PyObject *self, PyObject *args)
{
    int op;
    if (!PyArg_ParseTuple(args, "i:plock", &op))
        return NULL;
    if (plock(op) == -1)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif


#ifdef HAVE_POPEN
PyDoc_STRVAR(edk2_popen__doc__,
"popen(command [, mode='r' [, bufsize]]) -> pipe\n\n\
Open a pipe to/from a command returning a file object.");

static PyObject *
edk2_popen(PyObject *self, PyObject *args)
{
    char *name;
    char *mode = "r";
    int bufsize = -1;
    FILE *fp;
    PyObject *f;
    if (!PyArg_ParseTuple(args, "s|si:popen", &name, &mode, &bufsize))
        return NULL;
    /* Strip mode of binary or text modifiers */
    if (strcmp(mode, "rb") == 0 || strcmp(mode, "rt") == 0)
        mode = "r";
    else if (strcmp(mode, "wb") == 0 || strcmp(mode, "wt") == 0)
        mode = "w";
    Py_BEGIN_ALLOW_THREADS
    fp = popen(name, mode);
    Py_END_ALLOW_THREADS
    if (fp == NULL)
        return edk2_error();
    f = PyFile_FromFile(fp, name, mode, pclose);
    if (f != NULL)
        PyFile_SetBufSize(f, bufsize);
    return f;
}

#endif /* HAVE_POPEN */


#if defined(HAVE_WAIT3) || defined(HAVE_WAIT4)
static PyObject *
wait_helper(pid_t pid, int status, struct rusage *ru)
{
    PyObject *result;
    static PyObject *struct_rusage;

    if (pid == -1)
        return edk2_error();

    if (struct_rusage == NULL) {
        PyObject *m = PyImport_ImportModuleNoBlock("resource");
        if (m == NULL)
            return NULL;
        struct_rusage = PyObject_GetAttrString(m, "struct_rusage");
        Py_DECREF(m);
        if (struct_rusage == NULL)
            return NULL;
    }

    /* XXX(nnorwitz): Copied (w/mods) from resource.c, there should be only one. */
    result = PyStructSequence_New((PyTypeObject*) struct_rusage);
    if (!result)
        return NULL;

#ifndef doubletime
#define doubletime(TV) ((double)(TV).tv_sec + (TV).tv_usec * 0.000001)
#endif

    PyStructSequence_SET_ITEM(result, 0,
                              PyFloat_FromDouble(doubletime(ru->ru_utime)));
    PyStructSequence_SET_ITEM(result, 1,
                              PyFloat_FromDouble(doubletime(ru->ru_stime)));
#define SET_INT(result, index, value)\
        PyStructSequence_SET_ITEM(result, index, PyInt_FromLong(value))
    SET_INT(result, 2, ru->ru_maxrss);
    SET_INT(result, 3, ru->ru_ixrss);
    SET_INT(result, 4, ru->ru_idrss);
    SET_INT(result, 5, ru->ru_isrss);
    SET_INT(result, 6, ru->ru_minflt);
    SET_INT(result, 7, ru->ru_majflt);
    SET_INT(result, 8, ru->ru_nswap);
    SET_INT(result, 9, ru->ru_inblock);
    SET_INT(result, 10, ru->ru_oublock);
    SET_INT(result, 11, ru->ru_msgsnd);
    SET_INT(result, 12, ru->ru_msgrcv);
    SET_INT(result, 13, ru->ru_nsignals);
    SET_INT(result, 14, ru->ru_nvcsw);
    SET_INT(result, 15, ru->ru_nivcsw);
#undef SET_INT

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return Py_BuildValue("NiN", PyLong_FromPid(pid), status, result);
}
#endif /* HAVE_WAIT3 || HAVE_WAIT4 */

#ifdef HAVE_WAIT3
PyDoc_STRVAR(edk2_wait3__doc__,
"wait3(options) -> (pid, status, rusage)\n\n\
Wait for completion of a child process.");

static PyObject *
edk2_wait3(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:wait3", &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    pid = wait3(&status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT3 */

#ifdef HAVE_WAIT4
PyDoc_STRVAR(edk2_wait4__doc__,
"wait4(pid, options) -> (pid, status, rusage)\n\n\
Wait for completion of a given child process.");

static PyObject *
edk2_wait4(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, PARSE_PID "i:wait4", &pid, &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    pid = wait4(pid, &status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT4 */

#ifdef HAVE_WAITPID
PyDoc_STRVAR(edk2_waitpid__doc__,
"waitpid(pid, options) -> (pid, status)\n\n\
Wait for completion of a given child process.");

static PyObject *
edk2_waitpid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, PARSE_PID "i:waitpid", &pid, &options))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    pid = waitpid(pid, &status, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return edk2_error();

    return Py_BuildValue("Ni", PyLong_FromPid(pid), WAIT_STATUS_INT(status));
}

#elif defined(HAVE_CWAIT)

/* MS C has a variant of waitpid() that's usable for most purposes. */
PyDoc_STRVAR(edk2_waitpid__doc__,
"waitpid(pid, options) -> (pid, status << 8)\n\n"
"Wait for completion of a given process.  options is ignored on Windows.");

static PyObject *
edk2_waitpid(PyObject *self, PyObject *args)
{
    Py_intptr_t pid;
    int status, options;

    if (!PyArg_ParseTuple(args, PARSE_PID "i:waitpid", &pid, &options))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    pid = _cwait(&status, pid, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return edk2_error();

    /* shift the status left a byte so this is more like the POSIX waitpid */
    return Py_BuildValue("Ni", PyLong_FromPid(pid), status << 8);
}
#endif /* HAVE_WAITPID || HAVE_CWAIT */

#ifdef HAVE_WAIT
PyDoc_STRVAR(edk2_wait__doc__,
"wait() -> (pid, status)\n\n\
Wait for completion of a child process.");

static PyObject *
edk2_wait(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    Py_BEGIN_ALLOW_THREADS
    pid = wait(&status);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return edk2_error();

    return Py_BuildValue("Ni", PyLong_FromPid(pid), WAIT_STATUS_INT(status));
}
#endif


PyDoc_STRVAR(edk2_lstat__doc__,
"lstat(path) -> stat result\n\n\
Like stat(path), but do not follow symbolic links.");

static PyObject *
edk2_lstat(PyObject *self, PyObject *args)
{
#ifdef HAVE_LSTAT
    return edk2_do_stat(self, args, "et:lstat", lstat, NULL, NULL);
#else /* !HAVE_LSTAT */
    return edk2_do_stat(self, args, "et:lstat", STAT, NULL, NULL);
#endif /* !HAVE_LSTAT */
}


#ifdef HAVE_READLINK
PyDoc_STRVAR(edk2_readlink__doc__,
"readlink(path) -> path\n\n\
Return a string representing the path to which the symbolic link points.");

static PyObject *
edk2_readlink(PyObject *self, PyObject *args)
{
    PyObject* v;
    char buf[MAXPATHLEN];
    char *path;
    int n;
#ifdef Py_USING_UNICODE
    int arg_is_unicode = 0;
#endif

    if (!PyArg_ParseTuple(args, "et:readlink",
                          Py_FileSystemDefaultEncoding, &path))
        return NULL;
#ifdef Py_USING_UNICODE
    v = PySequence_GetItem(args, 0);
    if (v == NULL) {
        PyMem_Free(path);
        return NULL;
    }

    if (PyUnicode_Check(v)) {
        arg_is_unicode = 1;
    }
    Py_DECREF(v);
#endif

    Py_BEGIN_ALLOW_THREADS
    n = readlink(path, buf, (int) sizeof buf);
    Py_END_ALLOW_THREADS
    if (n < 0)
        return edk2_error_with_allocated_filename(path);

    PyMem_Free(path);
    v = PyString_FromStringAndSize(buf, n);
#ifdef Py_USING_UNICODE
    if (arg_is_unicode) {
        PyObject *w;

        w = PyUnicode_FromEncodedObject(v,
                                        Py_FileSystemDefaultEncoding,
                                        "strict");
        if (w != NULL) {
            Py_DECREF(v);
            v = w;
        }
        else {
            /* fall back to the original byte string, as
               discussed in patch #683592 */
            PyErr_Clear();
        }
    }
#endif
    return v;
}
#endif /* HAVE_READLINK */


#ifdef HAVE_SYMLINK
PyDoc_STRVAR(edk2_symlink__doc__,
"symlink(src, dst)\n\n\
Create a symbolic link pointing to src named dst.");

static PyObject *
edk2_symlink(PyObject *self, PyObject *args)
{
    return edk2_2str(args, "etet:symlink", symlink);
}
#endif /* HAVE_SYMLINK */


#ifdef HAVE_TIMES
#define NEED_TICKS_PER_SECOND
static long ticks_per_second = -1;
static PyObject *
edk2_times(PyObject *self, PyObject *noargs)
{
    struct tms t;
    clock_t c;
    errno = 0;
    c = times(&t);
    if (c == (clock_t) -1)
        return edk2_error();
    return Py_BuildValue("ddddd",
                         (double)t.tms_utime / ticks_per_second,
                         (double)t.tms_stime / ticks_per_second,
                         (double)t.tms_cutime / ticks_per_second,
                         (double)t.tms_cstime / ticks_per_second,
                         (double)c / ticks_per_second);
}
#endif /* HAVE_TIMES */


#ifdef HAVE_TIMES
PyDoc_STRVAR(edk2_times__doc__,
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\n\
Return a tuple of floating point numbers indicating process times.");
#endif


#ifdef HAVE_GETSID
PyDoc_STRVAR(edk2_getsid__doc__,
"getsid(pid) -> sid\n\n\
Call the system call getsid().");

static PyObject *
edk2_getsid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int sid;
    if (!PyArg_ParseTuple(args, PARSE_PID ":getsid", &pid))
        return NULL;
    sid = getsid(pid);
    if (sid < 0)
        return edk2_error();
    return PyInt_FromLong((long)sid);
}
#endif /* HAVE_GETSID */


#ifdef HAVE_SETSID
PyDoc_STRVAR(edk2_setsid__doc__,
"setsid()\n\n\
Call the system call setsid().");

static PyObject *
edk2_setsid(PyObject *self, PyObject *noargs)
{
    if (setsid() < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETSID */

#ifdef HAVE_SETPGID
PyDoc_STRVAR(edk2_setpgid__doc__,
"setpgid(pid, pgrp)\n\n\
Call the system call setpgid().");

static PyObject *
edk2_setpgid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int pgrp;
    if (!PyArg_ParseTuple(args, PARSE_PID "i:setpgid", &pid, &pgrp))
        return NULL;
    if (setpgid(pid, pgrp) < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETPGID */


#ifdef HAVE_TCGETPGRP
PyDoc_STRVAR(edk2_tcgetpgrp__doc__,
"tcgetpgrp(fd) -> pgid\n\n\
Return the process group associated with the terminal given by a fd.");

static PyObject *
edk2_tcgetpgrp(PyObject *self, PyObject *args)
{
    int fd;
    pid_t pgid;
    if (!PyArg_ParseTuple(args, "i:tcgetpgrp", &fd))
        return NULL;
    pgid = tcgetpgrp(fd);
    if (pgid < 0)
        return edk2_error();
    return PyLong_FromPid(pgid);
}
#endif /* HAVE_TCGETPGRP */


#ifdef HAVE_TCSETPGRP
PyDoc_STRVAR(edk2_tcsetpgrp__doc__,
"tcsetpgrp(fd, pgid)\n\n\
Set the process group associated with the terminal given by a fd.");

static PyObject *
edk2_tcsetpgrp(PyObject *self, PyObject *args)
{
    int fd;
    pid_t pgid;
    if (!PyArg_ParseTuple(args, "i" PARSE_PID ":tcsetpgrp", &fd, &pgid))
        return NULL;
    if (tcsetpgrp(fd, pgid) < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

PyDoc_STRVAR(edk2_open__doc__,
"open(filename, flag [, mode=0777]) -> fd\n\n\
Open a file (for low level IO).");

static PyObject *
edk2_open(PyObject *self, PyObject *args)
{
    char *file = NULL;
    int flag;
    int mode = 0777;
    int fd;

    if (!PyArg_ParseTuple(args, "eti|i",
                          Py_FileSystemDefaultEncoding, &file,
                          &flag, &mode))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    fd = open(file, flag, mode);
    Py_END_ALLOW_THREADS
    if (fd < 0)
        return edk2_error_with_allocated_filename(file);
    PyMem_Free(file);
    return PyInt_FromLong((long)fd);
}


PyDoc_STRVAR(edk2_close__doc__,
"close(fd)\n\n\
Close a file descriptor (for low level IO).");

static PyObject *
edk2_close(PyObject *self, PyObject *args)
{
    int fd, res;
    if (!PyArg_ParseTuple(args, "i:close", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
    res = close(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(edk2_closerange__doc__,
"closerange(fd_low, fd_high)\n\n\
Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

static PyObject *
edk2_closerange(PyObject *self, PyObject *args)
{
    int fd_from, fd_to, i;
    if (!PyArg_ParseTuple(args, "ii:closerange", &fd_from, &fd_to))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    for (i = fd_from; i < fd_to; i++)
        if (_PyVerify_fd(i))
            close(i);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


PyDoc_STRVAR(edk2_dup__doc__,
"dup(fd) -> fd2\n\n\
Return a duplicate of a file descriptor.");

static PyObject *
edk2_dup(PyObject *self, PyObject *args)
{
    int fd;
    if (!PyArg_ParseTuple(args, "i:dup", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
    fd = dup(fd);
    Py_END_ALLOW_THREADS
    if (fd < 0)
        return edk2_error();
    return PyInt_FromLong((long)fd);
}


PyDoc_STRVAR(edk2_dup2__doc__,
"dup2(old_fd, new_fd)\n\n\
Duplicate file descriptor.");

static PyObject *
edk2_dup2(PyObject *self, PyObject *args)
{
    int fd, fd2, res;
    if (!PyArg_ParseTuple(args, "ii:dup2", &fd, &fd2))
        return NULL;
    if (!_PyVerify_fd_dup2(fd, fd2))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
    res = dup2(fd, fd2);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(edk2_lseek__doc__,
"lseek(fd, pos, how) -> newpos\n\n\
Set the current position of a file descriptor.");

static PyObject *
edk2_lseek(PyObject *self, PyObject *args)
{
    int fd, how;
    off_t pos, res;
    PyObject *posobj;
    if (!PyArg_ParseTuple(args, "iOi:lseek", &fd, &posobj, &how))
        return NULL;
#ifdef SEEK_SET
    /* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
    switch (how) {
    case 0: how = SEEK_SET; break;
    case 1: how = SEEK_CUR; break;
    case 2: how = SEEK_END; break;
    }
#endif /* SEEK_END */

#if !defined(HAVE_LARGEFILE_SUPPORT)
    pos = PyInt_AsLong(posobj);
#else
    pos = PyLong_Check(posobj) ?
        PyLong_AsLongLong(posobj) : PyInt_AsLong(posobj);
#endif
    if (PyErr_Occurred())
        return NULL;

    if (!_PyVerify_fd(fd))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
    res = lseek(fd, pos, how);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();

#if !defined(HAVE_LARGEFILE_SUPPORT)
    return PyInt_FromLong(res);
#else
    return PyLong_FromLongLong(res);
#endif
}


PyDoc_STRVAR(edk2_read__doc__,
"read(fd, buffersize) -> string\n\n\
Read a file descriptor.");

static PyObject *
edk2_read(PyObject *self, PyObject *args)
{
    int fd, size, n;
    PyObject *buffer;
    if (!PyArg_ParseTuple(args, "ii:read", &fd, &size))
        return NULL;
    if (size < 0) {
        errno = EINVAL;
        return edk2_error();
    }
    buffer = PyString_FromStringAndSize((char *)NULL, size);
    if (buffer == NULL)
        return NULL;
    if (!_PyVerify_fd(fd)) {
        Py_DECREF(buffer);
        return edk2_error();
    }
    Py_BEGIN_ALLOW_THREADS
    n = read(fd, PyString_AsString(buffer), size);
    Py_END_ALLOW_THREADS
    if (n < 0) {
        Py_DECREF(buffer);
        return edk2_error();
    }
    if (n != size)
        _PyString_Resize(&buffer, n);
    return buffer;
}


PyDoc_STRVAR(edk2_write__doc__,
"write(fd, string) -> byteswritten\n\n\
Write a string to a file descriptor.");

static PyObject *
edk2_write(PyObject *self, PyObject *args)
{
    Py_buffer pbuf;
    int fd;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "is*:write", &fd, &pbuf))
        return NULL;
    if (!_PyVerify_fd(fd)) {
        PyBuffer_Release(&pbuf);
        return edk2_error();
    }
    Py_BEGIN_ALLOW_THREADS
    size = write(fd, pbuf.buf, (size_t)pbuf.len);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&pbuf);
    if (size < 0)
        return edk2_error();
    return PyInt_FromSsize_t(size);
}


PyDoc_STRVAR(edk2_fstat__doc__,
"fstat(fd) -> stat result\n\n\
Like stat(), but for an open file descriptor.");

static PyObject *
edk2_fstat(PyObject *self, PyObject *args)
{
    int fd;
    STRUCT_STAT st;
    int res;
    if (!PyArg_ParseTuple(args, "i:fstat", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
    res = FSTAT(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0) {
      return edk2_error();
    }

    return _pystat_fromstructstat(&st);
}


PyDoc_STRVAR(edk2_fdopen__doc__,
"fdopen(fd [, mode='r' [, bufsize]]) -> file_object\n\n\
Return an open file object connected to a file descriptor.");

static PyObject *
edk2_fdopen(PyObject *self, PyObject *args)
{
    int fd;
    char *orgmode = "r";
    int bufsize = -1;
    FILE *fp;
    PyObject *f;
    char *mode;
    if (!PyArg_ParseTuple(args, "i|si", &fd, &orgmode, &bufsize))
        return NULL;

    /* Sanitize mode.  See fileobject.c */
    mode = PyMem_MALLOC(strlen(orgmode)+3);
    if (!mode) {
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(mode, orgmode);
    if (_PyFile_SanitizeMode(mode)) {
        PyMem_FREE(mode);
        return NULL;
    }
    if (!_PyVerify_fd(fd))
        return edk2_error();
    Py_BEGIN_ALLOW_THREADS
#if defined(HAVE_FCNTL_H)
    if (mode[0] == 'a') {
        /* try to make sure the O_APPEND flag is set */
        int flags;
        flags = fcntl(fd, F_GETFL);
        if (flags != -1)
            fcntl(fd, F_SETFL, flags | O_APPEND);
        fp = fdopen(fd, mode);
        if (fp == NULL && flags != -1)
            /* restore old mode if fdopen failed */
            fcntl(fd, F_SETFL, flags);
    } else {
        fp = fdopen(fd, mode);
    }
#else
    fp = fdopen(fd, mode);
#endif
    Py_END_ALLOW_THREADS
    PyMem_FREE(mode);
    if (fp == NULL)
        return edk2_error();
    f = PyFile_FromFile(fp, "<fdopen>", orgmode, fclose);
    if (f != NULL)
        PyFile_SetBufSize(f, bufsize);
    return f;
}

PyDoc_STRVAR(edk2_isatty__doc__,
"isatty(fd) -> bool\n\n\
Return True if the file descriptor 'fd' is an open file descriptor\n\
connected to the slave end of a terminal.");

static PyObject *
edk2_isatty(PyObject *self, PyObject *args)
{
    int fd;
    if (!PyArg_ParseTuple(args, "i:isatty", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return PyBool_FromLong(0);
    return PyBool_FromLong(isatty(fd));
}

#ifdef HAVE_PIPE
PyDoc_STRVAR(edk2_pipe__doc__,
"pipe() -> (read_end, write_end)\n\n\
Create a pipe.");

static PyObject *
edk2_pipe(PyObject *self, PyObject *noargs)
{
    int fds[2];
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = pipe(fds);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return edk2_error();
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}
#endif  /* HAVE_PIPE */


#ifdef HAVE_MKFIFO
PyDoc_STRVAR(edk2_mkfifo__doc__,
"mkfifo(filename [, mode=0666])\n\n\
Create a FIFO (a POSIX named pipe).");

static PyObject *
edk2_mkfifo(PyObject *self, PyObject *args)
{
    char *filename;
    int mode = 0666;
    int res;
    if (!PyArg_ParseTuple(args, "s|i:mkfifo", &filename, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = mkfifo(filename, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif


#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
PyDoc_STRVAR(edk2_mknod__doc__,
"mknod(filename [, mode=0600, device])\n\n\
Create a filesystem node (file, device special file or named pipe)\n\
named filename. mode specifies both the permissions to use and the\n\
type of node to be created, being combined (bitwise OR) with one of\n\
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO. For S_IFCHR and S_IFBLK,\n\
device defines the newly created device special file (probably using\n\
os.makedev()), otherwise it is ignored.");


static PyObject *
edk2_mknod(PyObject *self, PyObject *args)
{
    char *filename;
    int mode = 0600;
    int device = 0;
    int res;
    if (!PyArg_ParseTuple(args, "s|ii:mknod", &filename, &mode, &device))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = mknod(filename, mode, device);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_DEVICE_MACROS
PyDoc_STRVAR(edk2_major__doc__,
"major(device) -> major number\n\
Extracts a device major number from a raw device number.");

static PyObject *
edk2_major(PyObject *self, PyObject *args)
{
    int device;
    if (!PyArg_ParseTuple(args, "i:major", &device))
        return NULL;
    return PyInt_FromLong((long)major(device));
}

PyDoc_STRVAR(edk2_minor__doc__,
"minor(device) -> minor number\n\
Extracts a device minor number from a raw device number.");

static PyObject *
edk2_minor(PyObject *self, PyObject *args)
{
    int device;
    if (!PyArg_ParseTuple(args, "i:minor", &device))
        return NULL;
    return PyInt_FromLong((long)minor(device));
}

PyDoc_STRVAR(edk2_makedev__doc__,
"makedev(major, minor) -> device number\n\
Composes a raw device number from the major and minor device numbers.");

static PyObject *
edk2_makedev(PyObject *self, PyObject *args)
{
    int major, minor;
    if (!PyArg_ParseTuple(args, "ii:makedev", &major, &minor))
        return NULL;
    return PyInt_FromLong((long)makedev(major, minor));
}
#endif /* device macros */


#ifdef HAVE_FTRUNCATE
PyDoc_STRVAR(edk2_ftruncate__doc__,
"ftruncate(fd, length)\n\n\
Truncate a file to a specified length.");

static PyObject *
edk2_ftruncate(PyObject *self, PyObject *args)
{
    int fd;
    off_t length;
    int res;
    PyObject *lenobj;

    if (!PyArg_ParseTuple(args, "iO:ftruncate", &fd, &lenobj))
        return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
    length = PyInt_AsLong(lenobj);
#else
    length = PyLong_Check(lenobj) ?
        PyLong_AsLongLong(lenobj) : PyInt_AsLong(lenobj);
#endif
    if (PyErr_Occurred())
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = ftruncate(fd, length);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return edk2_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_PUTENV
PyDoc_STRVAR(edk2_putenv__doc__,
"putenv(key, value)\n\n\
Change or add an environment variable.");

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *edk2_putenv_garbage;

static PyObject *
edk2_putenv(PyObject *self, PyObject *args)
{
    char *s1, *s2;
    char *newenv;
    PyObject *newstr;
    size_t len;

    if (!PyArg_ParseTuple(args, "ss:putenv", &s1, &s2))
        return NULL;

    /* XXX This can leak memory -- not easy to fix :-( */
    len = strlen(s1) + strlen(s2) + 2;
    /* len includes space for a trailing \0; the size arg to
       PyString_FromStringAndSize does not count that */
    newstr = PyString_FromStringAndSize(NULL, (int)len - 1);
    if (newstr == NULL)
        return PyErr_NoMemory();
    newenv = PyString_AS_STRING(newstr);
    PyOS_snprintf(newenv, len, "%s=%s", s1, s2);
    if (putenv(newenv)) {
        Py_DECREF(newstr);
        edk2_error();
        return NULL;
    }
    /* Install the first arg and newstr in edk2_putenv_garbage;
     * this will cause previous value to be collected.  This has to
     * happen after the real putenv() call because the old value
     * was still accessible until then. */
    if (PyDict_SetItem(edk2_putenv_garbage,
                       PyTuple_GET_ITEM(args, 0), newstr)) {
        /* really not much we can do; just leak */
        PyErr_Clear();
    }
    else {
        Py_DECREF(newstr);
    }

    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* putenv */

#ifdef HAVE_UNSETENV
PyDoc_STRVAR(edk2_unsetenv__doc__,
"unsetenv(key)\n\n\
Delete an environment variable.");

static PyObject *
edk2_unsetenv(PyObject *self, PyObject *args)
{
    char *s1;

    if (!PyArg_ParseTuple(args, "s:unsetenv", &s1))
        return NULL;

    unsetenv(s1);

    /* Remove the key from edk2_putenv_garbage;
     * this will cause it to be collected.  This has to
     * happen after the real unsetenv() call because the
     * old value was still accessible until then.
     */
    if (PyDict_DelItem(edk2_putenv_garbage,
                       PyTuple_GET_ITEM(args, 0))) {
        /* really not much we can do; just leak */
        PyErr_Clear();
    }

    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* unsetenv */

PyDoc_STRVAR(edk2_strerror__doc__,
"strerror(code) -> string\n\n\
Translate an error code to a message string.");

static PyObject *
edk2_strerror(PyObject *self, PyObject *args)
{
    int code;
    char *message;
    if (!PyArg_ParseTuple(args, "i:strerror", &code))
        return NULL;
    message = strerror(code);
    if (message == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "strerror() argument out of range");
        return NULL;
    }
    return PyString_FromString(message);
}


#ifdef HAVE_SYS_WAIT_H

#ifdef WCOREDUMP
PyDoc_STRVAR(edk2_WCOREDUMP__doc__,
"WCOREDUMP(status) -> bool\n\n\
Return True if the process returning 'status' was dumped to a core file.");

static PyObject *
edk2_WCOREDUMP(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WCOREDUMP", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WCOREDUMP(status));
}
#endif /* WCOREDUMP */

#ifdef WIFCONTINUED
PyDoc_STRVAR(edk2_WIFCONTINUED__doc__,
"WIFCONTINUED(status) -> bool\n\n\
Return True if the process returning 'status' was continued from a\n\
job control stop.");

static PyObject *
edk2_WIFCONTINUED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WCONTINUED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFCONTINUED(status));
}
#endif /* WIFCONTINUED */

#ifdef WIFSTOPPED
PyDoc_STRVAR(edk2_WIFSTOPPED__doc__,
"WIFSTOPPED(status) -> bool\n\n\
Return True if the process returning 'status' was stopped.");

static PyObject *
edk2_WIFSTOPPED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFSTOPPED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFSTOPPED(status));
}
#endif /* WIFSTOPPED */

#ifdef WIFSIGNALED
PyDoc_STRVAR(edk2_WIFSIGNALED__doc__,
"WIFSIGNALED(status) -> bool\n\n\
Return True if the process returning 'status' was terminated by a signal.");

static PyObject *
edk2_WIFSIGNALED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFSIGNALED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFSIGNALED(status));
}
#endif /* WIFSIGNALED */

#ifdef WIFEXITED
PyDoc_STRVAR(edk2_WIFEXITED__doc__,
"WIFEXITED(status) -> bool\n\n\
Return true if the process returning 'status' exited using the exit()\n\
system call.");

static PyObject *
edk2_WIFEXITED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFEXITED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFEXITED(status));
}
#endif /* WIFEXITED */

#ifdef WEXITSTATUS
PyDoc_STRVAR(edk2_WEXITSTATUS__doc__,
"WEXITSTATUS(status) -> integer\n\n\
Return the process return code from 'status'.");

static PyObject *
edk2_WEXITSTATUS(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WEXITSTATUS", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WEXITSTATUS(status));
}
#endif /* WEXITSTATUS */

#ifdef WTERMSIG
PyDoc_STRVAR(edk2_WTERMSIG__doc__,
"WTERMSIG(status) -> integer\n\n\
Return the signal that terminated the process that provided the 'status'\n\
value.");

static PyObject *
edk2_WTERMSIG(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WTERMSIG", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WTERMSIG(status));
}
#endif /* WTERMSIG */

#ifdef WSTOPSIG
PyDoc_STRVAR(edk2_WSTOPSIG__doc__,
"WSTOPSIG(status) -> integer\n\n\
Return the signal that stopped the process that provided\n\
the 'status' value.");

static PyObject *
edk2_WSTOPSIG(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WSTOPSIG", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WSTOPSIG(status));
}
#endif /* WSTOPSIG */

#endif /* HAVE_SYS_WAIT_H */


#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>

static PyObject*
_pystatvfs_fromstructstatvfs(struct statvfs st) {
    PyObject *v = PyStructSequence_New(&StatVFSResultType);
    if (v == NULL)
        return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
    PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
    PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
    PyStructSequence_SET_ITEM(v, 2, PyInt_FromLong((long) st.f_blocks));
    PyStructSequence_SET_ITEM(v, 3, PyInt_FromLong((long) st.f_bfree));
    PyStructSequence_SET_ITEM(v, 4, PyInt_FromLong((long) st.f_bavail));
    PyStructSequence_SET_ITEM(v, 5, PyInt_FromLong((long) st.f_files));
    PyStructSequence_SET_ITEM(v, 6, PyInt_FromLong((long) st.f_ffree));
    PyStructSequence_SET_ITEM(v, 7, PyInt_FromLong((long) st.f_favail));
    PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
    PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#else
    PyStructSequence_SET_ITEM(v, 0, PyInt_FromLong((long) st.f_bsize));
    PyStructSequence_SET_ITEM(v, 1, PyInt_FromLong((long) st.f_frsize));
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_blocks));
    PyStructSequence_SET_ITEM(v, 3,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_bfree));
    PyStructSequence_SET_ITEM(v, 4,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_bavail));
    PyStructSequence_SET_ITEM(v, 5,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_files));
    PyStructSequence_SET_ITEM(v, 6,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_ffree));
    PyStructSequence_SET_ITEM(v, 7,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_favail));
    PyStructSequence_SET_ITEM(v, 8, PyInt_FromLong((long) st.f_flag));
    PyStructSequence_SET_ITEM(v, 9, PyInt_FromLong((long) st.f_namemax));
#endif

    return v;
}

PyDoc_STRVAR(edk2_fstatvfs__doc__,
"fstatvfs(fd) -> statvfs result\n\n\
Perform an fstatvfs system call on the given fd.");

static PyObject *
edk2_fstatvfs(PyObject *self, PyObject *args)
{
    int fd, res;
    struct statvfs st;

    if (!PyArg_ParseTuple(args, "i:fstatvfs", &fd))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fstatvfs(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return edk2_error();

    return _pystatvfs_fromstructstatvfs(st);
}
#endif /* HAVE_FSTATVFS && HAVE_SYS_STATVFS_H */


#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>

PyDoc_STRVAR(edk2_statvfs__doc__,
"statvfs(path) -> statvfs result\n\n\
Perform a statvfs system call on the given path.");

static PyObject *
edk2_statvfs(PyObject *self, PyObject *args)
{
    char *path;
    int res;
    struct statvfs st;
    if (!PyArg_ParseTuple(args, "s:statvfs", &path))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = statvfs(path, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return edk2_error_with_filename(path);

    return _pystatvfs_fromstructstatvfs(st);
}
#endif /* HAVE_STATVFS */


#ifdef HAVE_TEMPNAM
PyDoc_STRVAR(edk2_tempnam__doc__,
"tempnam([dir[, prefix]]) -> string\n\n\
Return a unique name for a temporary file.\n\
The directory and a prefix may be specified as strings; they may be omitted\n\
or None if not needed.");

static PyObject *
edk2_tempnam(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    char *dir = NULL;
    char *pfx = NULL;
    char *name;

    if (!PyArg_ParseTuple(args, "|zz:tempnam", &dir, &pfx))
    return NULL;

    if (PyErr_Warn(PyExc_RuntimeWarning,
                   "tempnam is a potential security risk to your program") < 0)
        return NULL;

    if (PyErr_WarnPy3k("tempnam has been removed in 3.x; "
                       "use the tempfile module", 1) < 0)
        return NULL;

    name = tempnam(dir, pfx);
    if (name == NULL)
        return PyErr_NoMemory();
    result = PyString_FromString(name);
    free(name);
    return result;
}
#endif


#ifdef HAVE_TMPFILE
PyDoc_STRVAR(edk2_tmpfile__doc__,
"tmpfile() -> file object\n\n\
Create a temporary file with no directory entries.");

static PyObject *
edk2_tmpfile(PyObject *self, PyObject *noargs)
{
    FILE *fp;

    if (PyErr_WarnPy3k("tmpfile has been removed in 3.x; "
                       "use the tempfile module", 1) < 0)
        return NULL;

    fp = tmpfile();
    if (fp == NULL)
        return edk2_error();
    return PyFile_FromFile(fp, "<tmpfile>", "w+b", fclose);
}
#endif


#ifdef HAVE_TMPNAM
PyDoc_STRVAR(edk2_tmpnam__doc__,
"tmpnam() -> string\n\n\
Return a unique name for a temporary file.");

static PyObject *
edk2_tmpnam(PyObject *self, PyObject *noargs)
{
    char buffer[L_tmpnam];
    char *name;

    if (PyErr_Warn(PyExc_RuntimeWarning,
                   "tmpnam is a potential security risk to your program") < 0)
        return NULL;

    if (PyErr_WarnPy3k("tmpnam has been removed in 3.x; "
                       "use the tempfile module", 1) < 0)
        return NULL;

#ifdef USE_TMPNAM_R
    name = tmpnam_r(buffer);
#else
    name = tmpnam(buffer);
#endif
    if (name == NULL) {
        PyObject *err = Py_BuildValue("is", 0,
#ifdef USE_TMPNAM_R
                                      "unexpected NULL from tmpnam_r"
#else
                                      "unexpected NULL from tmpnam"
#endif
                                      );
        PyErr_SetObject(PyExc_OSError, err);
        Py_XDECREF(err);
        return NULL;
    }
    return PyString_FromString(buffer);
}
#endif

PyDoc_STRVAR(edk2_abort__doc__,
"abort() -> does not return!\n\n\
Abort the interpreter immediately.  This 'dumps core' or otherwise fails\n\
in the hardest way possible on the hosting operating system.");

static PyObject *
edk2_abort(PyObject *self, PyObject *noargs)
{
    abort();
    /*NOTREACHED*/
    Py_FatalError("abort() called from Python code didn't abort!");
    return NULL;
}

static PyMethodDef edk2_methods[] = {
    {"access",          edk2_access,     METH_VARARGS, edk2_access__doc__},
#ifdef HAVE_TTYNAME
    {"ttyname",         edk2_ttyname, METH_VARARGS, edk2_ttyname__doc__},
#endif
    {"chdir",           edk2_chdir,      METH_VARARGS, edk2_chdir__doc__},
#ifdef HAVE_CHFLAGS
    {"chflags",         edk2_chflags, METH_VARARGS, edk2_chflags__doc__},
#endif /* HAVE_CHFLAGS */
    {"chmod",           edk2_chmod,      METH_VARARGS, edk2_chmod__doc__},
#ifdef HAVE_FCHMOD
    {"fchmod",          edk2_fchmod, METH_VARARGS, edk2_fchmod__doc__},
#endif /* HAVE_FCHMOD */
#ifdef HAVE_CHOWN
    {"chown",           edk2_chown, METH_VARARGS, edk2_chown__doc__},
#endif /* HAVE_CHOWN */
#ifdef HAVE_LCHMOD
    {"lchmod",          edk2_lchmod, METH_VARARGS, edk2_lchmod__doc__},
#endif /* HAVE_LCHMOD */
#ifdef HAVE_FCHOWN
    {"fchown",          edk2_fchown, METH_VARARGS, edk2_fchown__doc__},
#endif /* HAVE_FCHOWN */
#ifdef HAVE_LCHFLAGS
    {"lchflags",        edk2_lchflags, METH_VARARGS, edk2_lchflags__doc__},
#endif /* HAVE_LCHFLAGS */
#ifdef HAVE_LCHOWN
    {"lchown",          edk2_lchown, METH_VARARGS, edk2_lchown__doc__},
#endif /* HAVE_LCHOWN */
#ifdef HAVE_CHROOT
    {"chroot",          edk2_chroot, METH_VARARGS, edk2_chroot__doc__},
#endif
#ifdef HAVE_CTERMID
    {"ctermid",         edk2_ctermid, METH_NOARGS, edk2_ctermid__doc__},
#endif
#ifdef HAVE_GETCWD
    {"getcwd",          edk2_getcwd,     METH_NOARGS,  edk2_getcwd__doc__},
#ifdef Py_USING_UNICODE
    {"getcwdu",         edk2_getcwdu,    METH_NOARGS,  edk2_getcwdu__doc__},
#endif
#endif
#ifdef HAVE_LINK
    {"link",            edk2_link, METH_VARARGS, edk2_link__doc__},
#endif /* HAVE_LINK */
    {"listdir",         edk2_listdir,    METH_VARARGS, edk2_listdir__doc__},
    {"lstat",           edk2_lstat,      METH_VARARGS, edk2_lstat__doc__},
    {"mkdir",           edk2_mkdir,      METH_VARARGS, edk2_mkdir__doc__},
#ifdef HAVE_NICE
    {"nice",            edk2_nice, METH_VARARGS, edk2_nice__doc__},
#endif /* HAVE_NICE */
#ifdef HAVE_READLINK
    {"readlink",        edk2_readlink, METH_VARARGS, edk2_readlink__doc__},
#endif /* HAVE_READLINK */
    {"rename",          edk2_rename,     METH_VARARGS, edk2_rename__doc__},
    {"rmdir",           edk2_rmdir,      METH_VARARGS, edk2_rmdir__doc__},
    {"stat",            edk2_stat,       METH_VARARGS, edk2_stat__doc__},
    //{"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
#ifdef HAVE_SYMLINK
    {"symlink",         edk2_symlink, METH_VARARGS, edk2_symlink__doc__},
#endif /* HAVE_SYMLINK */
#ifdef HAVE_SYSTEM
    {"system",          edk2_system, METH_VARARGS, edk2_system__doc__},
#endif
    {"umask",           edk2_umask,      METH_VARARGS, edk2_umask__doc__},
#ifdef HAVE_UNAME
    {"uname",           edk2_uname, METH_NOARGS, edk2_uname__doc__},
#endif /* HAVE_UNAME */
    {"unlink",          edk2_unlink,     METH_VARARGS, edk2_unlink__doc__},
    {"remove",          edk2_unlink,     METH_VARARGS, edk2_remove__doc__},
    {"utime",           edk2_utime,      METH_VARARGS, edk2_utime__doc__},
#ifdef HAVE_TIMES
    {"times",           edk2_times, METH_NOARGS, edk2_times__doc__},
#endif /* HAVE_TIMES */
    {"_exit",           edk2__exit,      METH_VARARGS, edk2__exit__doc__},
#ifdef HAVE_EXECV
    {"execv",           edk2_execv, METH_VARARGS, edk2_execv__doc__},
    {"execve",          edk2_execve, METH_VARARGS, edk2_execve__doc__},
#endif /* HAVE_EXECV */
#ifdef HAVE_SPAWNV
    {"spawnv",          edk2_spawnv, METH_VARARGS, edk2_spawnv__doc__},
    {"spawnve",         edk2_spawnve, METH_VARARGS, edk2_spawnve__doc__},
#if defined(PYOS_OS2)
    {"spawnvp",         edk2_spawnvp, METH_VARARGS, edk2_spawnvp__doc__},
    {"spawnvpe",        edk2_spawnvpe, METH_VARARGS, edk2_spawnvpe__doc__},
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */
#ifdef HAVE_FORK1
    {"fork1",       edk2_fork1, METH_NOARGS, edk2_fork1__doc__},
#endif /* HAVE_FORK1 */
#ifdef HAVE_FORK
    {"fork",            edk2_fork, METH_NOARGS, edk2_fork__doc__},
#endif /* HAVE_FORK */
#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
    {"openpty",         edk2_openpty, METH_NOARGS, edk2_openpty__doc__},
#endif /* HAVE_OPENPTY || HAVE__GETPTY || HAVE_DEV_PTMX */
#ifdef HAVE_FORKPTY
    {"forkpty",         edk2_forkpty, METH_NOARGS, edk2_forkpty__doc__},
#endif /* HAVE_FORKPTY */
    {"getpid",          edk2_getpid,     METH_NOARGS,  edk2_getpid__doc__},
#ifdef HAVE_GETPGRP
    {"getpgrp",         edk2_getpgrp, METH_NOARGS, edk2_getpgrp__doc__},
#endif /* HAVE_GETPGRP */
#ifdef HAVE_GETPPID
    {"getppid",         edk2_getppid, METH_NOARGS, edk2_getppid__doc__},
#endif /* HAVE_GETPPID */
#ifdef HAVE_GETLOGIN
    {"getlogin",        edk2_getlogin, METH_NOARGS, edk2_getlogin__doc__},
#endif
#ifdef HAVE_KILL
    {"kill",            edk2_kill, METH_VARARGS, edk2_kill__doc__},
#endif /* HAVE_KILL */
#ifdef HAVE_KILLPG
    {"killpg",          edk2_killpg, METH_VARARGS, edk2_killpg__doc__},
#endif /* HAVE_KILLPG */
#ifdef HAVE_PLOCK
    {"plock",           edk2_plock, METH_VARARGS, edk2_plock__doc__},
#endif /* HAVE_PLOCK */
#ifdef HAVE_POPEN
    {"popen",           edk2_popen, METH_VARARGS, edk2_popen__doc__},
#endif /* HAVE_POPEN */
#ifdef HAVE_SETGROUPS
    {"setgroups",       edk2_setgroups, METH_O, edk2_setgroups__doc__},
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_INITGROUPS
    {"initgroups",      edk2_initgroups, METH_VARARGS, edk2_initgroups__doc__},
#endif /* HAVE_INITGROUPS */
#ifdef HAVE_GETPGID
    {"getpgid",         edk2_getpgid, METH_VARARGS, edk2_getpgid__doc__},
#endif /* HAVE_GETPGID */
#ifdef HAVE_SETPGRP
    {"setpgrp",         edk2_setpgrp, METH_NOARGS, edk2_setpgrp__doc__},
#endif /* HAVE_SETPGRP */
#ifdef HAVE_WAIT
    {"wait",            edk2_wait, METH_NOARGS, edk2_wait__doc__},
#endif /* HAVE_WAIT */
#ifdef HAVE_WAIT3
    {"wait3",           edk2_wait3, METH_VARARGS, edk2_wait3__doc__},
#endif /* HAVE_WAIT3 */
#ifdef HAVE_WAIT4
    {"wait4",           edk2_wait4, METH_VARARGS, edk2_wait4__doc__},
#endif /* HAVE_WAIT4 */
#if defined(HAVE_WAITPID) || defined(HAVE_CWAIT)
    {"waitpid",         edk2_waitpid, METH_VARARGS, edk2_waitpid__doc__},
#endif /* HAVE_WAITPID */
#ifdef HAVE_GETSID
    {"getsid",          edk2_getsid, METH_VARARGS, edk2_getsid__doc__},
#endif /* HAVE_GETSID */
#ifdef HAVE_SETSID
    {"setsid",          edk2_setsid, METH_NOARGS, edk2_setsid__doc__},
#endif /* HAVE_SETSID */
#ifdef HAVE_SETPGID
    {"setpgid",         edk2_setpgid, METH_VARARGS, edk2_setpgid__doc__},
#endif /* HAVE_SETPGID */
#ifdef HAVE_TCGETPGRP
    {"tcgetpgrp",       edk2_tcgetpgrp, METH_VARARGS, edk2_tcgetpgrp__doc__},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
    {"tcsetpgrp",       edk2_tcsetpgrp, METH_VARARGS, edk2_tcsetpgrp__doc__},
#endif /* HAVE_TCSETPGRP */
    {"open",            edk2_open,       METH_VARARGS, edk2_open__doc__},
    {"close",           edk2_close,      METH_VARARGS, edk2_close__doc__},
    {"closerange",      edk2_closerange, METH_VARARGS, edk2_closerange__doc__},
    {"dup",             edk2_dup,        METH_VARARGS, edk2_dup__doc__},
    {"dup2",            edk2_dup2,       METH_VARARGS, edk2_dup2__doc__},
    {"lseek",           edk2_lseek,      METH_VARARGS, edk2_lseek__doc__},
    {"read",            edk2_read,       METH_VARARGS, edk2_read__doc__},
    {"write",           edk2_write,      METH_VARARGS, edk2_write__doc__},
    {"fstat",           edk2_fstat,      METH_VARARGS, edk2_fstat__doc__},
    {"fdopen",          edk2_fdopen,     METH_VARARGS, edk2_fdopen__doc__},
    {"isatty",          edk2_isatty,     METH_VARARGS, edk2_isatty__doc__},
#ifdef HAVE_PIPE
    {"pipe",            edk2_pipe, METH_NOARGS, edk2_pipe__doc__},
#endif
#ifdef HAVE_MKFIFO
    {"mkfifo",          edk2_mkfifo, METH_VARARGS, edk2_mkfifo__doc__},
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
    {"mknod",           edk2_mknod, METH_VARARGS, edk2_mknod__doc__},
#endif
#ifdef HAVE_DEVICE_MACROS
    {"major",           edk2_major, METH_VARARGS, edk2_major__doc__},
    {"minor",           edk2_minor, METH_VARARGS, edk2_minor__doc__},
    {"makedev",         edk2_makedev, METH_VARARGS, edk2_makedev__doc__},
#endif
#ifdef HAVE_FTRUNCATE
    {"ftruncate",       edk2_ftruncate, METH_VARARGS, edk2_ftruncate__doc__},
#endif
#ifdef HAVE_PUTENV
    {"putenv",          edk2_putenv, METH_VARARGS, edk2_putenv__doc__},
#endif
#ifdef HAVE_UNSETENV
    {"unsetenv",        edk2_unsetenv, METH_VARARGS, edk2_unsetenv__doc__},
#endif
    {"strerror",        edk2_strerror,   METH_VARARGS, edk2_strerror__doc__},
#ifdef HAVE_FCHDIR
    {"fchdir",          edk2_fchdir, METH_O, edk2_fchdir__doc__},
#endif
#ifdef HAVE_FSYNC
    {"fsync",       edk2_fsync, METH_O, edk2_fsync__doc__},
#endif
#ifdef HAVE_FDATASYNC
    {"fdatasync",   edk2_fdatasync,  METH_O, edk2_fdatasync__doc__},
#endif
#ifdef HAVE_SYS_WAIT_H
#ifdef WCOREDUMP
    {"WCOREDUMP",       edk2_WCOREDUMP, METH_VARARGS, edk2_WCOREDUMP__doc__},
#endif /* WCOREDUMP */
#ifdef WIFCONTINUED
    {"WIFCONTINUED",edk2_WIFCONTINUED, METH_VARARGS, edk2_WIFCONTINUED__doc__},
#endif /* WIFCONTINUED */
#ifdef WIFSTOPPED
    {"WIFSTOPPED",      edk2_WIFSTOPPED, METH_VARARGS, edk2_WIFSTOPPED__doc__},
#endif /* WIFSTOPPED */
#ifdef WIFSIGNALED
    {"WIFSIGNALED",     edk2_WIFSIGNALED, METH_VARARGS, edk2_WIFSIGNALED__doc__},
#endif /* WIFSIGNALED */
#ifdef WIFEXITED
    {"WIFEXITED",       edk2_WIFEXITED, METH_VARARGS, edk2_WIFEXITED__doc__},
#endif /* WIFEXITED */
#ifdef WEXITSTATUS
    {"WEXITSTATUS",     edk2_WEXITSTATUS, METH_VARARGS, edk2_WEXITSTATUS__doc__},
#endif /* WEXITSTATUS */
#ifdef WTERMSIG
    {"WTERMSIG",        edk2_WTERMSIG, METH_VARARGS, edk2_WTERMSIG__doc__},
#endif /* WTERMSIG */
#ifdef WSTOPSIG
    {"WSTOPSIG",        edk2_WSTOPSIG, METH_VARARGS, edk2_WSTOPSIG__doc__},
#endif /* WSTOPSIG */
#endif /* HAVE_SYS_WAIT_H */
#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
    {"fstatvfs",        edk2_fstatvfs, METH_VARARGS, edk2_fstatvfs__doc__},
#endif
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
    {"statvfs",         edk2_statvfs, METH_VARARGS, edk2_statvfs__doc__},
#endif
#ifdef HAVE_TMPFILE
    {"tmpfile",         edk2_tmpfile,    METH_NOARGS,  edk2_tmpfile__doc__},
#endif
#ifdef HAVE_TEMPNAM
    {"tempnam",         edk2_tempnam,    METH_VARARGS, edk2_tempnam__doc__},
#endif
#ifdef HAVE_TMPNAM
    {"tmpnam",          edk2_tmpnam,     METH_NOARGS,  edk2_tmpnam__doc__},
#endif
#ifdef HAVE_CONFSTR
    {"confstr",         edk2_confstr, METH_VARARGS, edk2_confstr__doc__},
#endif
#ifdef HAVE_SYSCONF
    {"sysconf",         edk2_sysconf, METH_VARARGS, edk2_sysconf__doc__},
#endif
#ifdef HAVE_FPATHCONF
    {"fpathconf",       edk2_fpathconf, METH_VARARGS, edk2_fpathconf__doc__},
#endif
#ifdef HAVE_PATHCONF
    {"pathconf",        edk2_pathconf, METH_VARARGS, edk2_pathconf__doc__},
#endif
    {"abort",           edk2_abort,      METH_NOARGS,  edk2_abort__doc__},

    {NULL,              NULL}            /* Sentinel */
};


static int
ins(PyObject *module, char *symbol, long value)
{
    return PyModule_AddIntConstant(module, symbol, value);
}

static int
all_ins(PyObject *d)
{
#ifdef F_OK
    if (ins(d, "F_OK", (long)F_OK)) return -1;
#endif
#ifdef R_OK
    if (ins(d, "R_OK", (long)R_OK)) return -1;
#endif
#ifdef W_OK
    if (ins(d, "W_OK", (long)W_OK)) return -1;
#endif
#ifdef X_OK
    if (ins(d, "X_OK", (long)X_OK)) return -1;
#endif
#ifdef NGROUPS_MAX
    if (ins(d, "NGROUPS_MAX", (long)NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
    if (ins(d, "TMP_MAX", (long)TMP_MAX)) return -1;
#endif
#ifdef WCONTINUED
    if (ins(d, "WCONTINUED", (long)WCONTINUED)) return -1;
#endif
#ifdef WNOHANG
    if (ins(d, "WNOHANG", (long)WNOHANG)) return -1;
#endif
#ifdef WUNTRACED
    if (ins(d, "WUNTRACED", (long)WUNTRACED)) return -1;
#endif
#ifdef O_RDONLY
    if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
    if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
    if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
    if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
    if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
    if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
    if (ins(d, "O_DSYNC", (long)O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
    if (ins(d, "O_RSYNC", (long)O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
    if (ins(d, "O_SYNC", (long)O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
    if (ins(d, "O_NOCTTY", (long)O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
    if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
#endif
#ifdef O_EXCL
    if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
    if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;
#endif
#ifdef O_BINARY
    if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
    if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif
#ifdef O_LARGEFILE
    if (ins(d, "O_LARGEFILE", (long)O_LARGEFILE)) return -1;
#endif
#ifdef O_SHLOCK
    if (ins(d, "O_SHLOCK", (long)O_SHLOCK)) return -1;
#endif
#ifdef O_EXLOCK
    if (ins(d, "O_EXLOCK", (long)O_EXLOCK)) return -1;
#endif

/* MS Windows */
#ifdef O_NOINHERIT
    /* Don't inherit in child processes. */
    if (ins(d, "O_NOINHERIT", (long)O_NOINHERIT)) return -1;
#endif
#ifdef _O_SHORT_LIVED
    /* Optimize for short life (keep in memory). */
    /* MS forgot to define this one with a non-underscore form too. */
    if (ins(d, "O_SHORT_LIVED", (long)_O_SHORT_LIVED)) return -1;
#endif
#ifdef O_TEMPORARY
    /* Automatically delete when last handle is closed. */
    if (ins(d, "O_TEMPORARY", (long)O_TEMPORARY)) return -1;
#endif
#ifdef O_RANDOM
    /* Optimize for random access. */
    if (ins(d, "O_RANDOM", (long)O_RANDOM)) return -1;
#endif
#ifdef O_SEQUENTIAL
    /* Optimize for sequential access. */
    if (ins(d, "O_SEQUENTIAL", (long)O_SEQUENTIAL)) return -1;
#endif

/* GNU extensions. */
#ifdef O_ASYNC
    /* Send a SIGIO signal whenever input or output
       becomes available on file descriptor */
    if (ins(d, "O_ASYNC", (long)O_ASYNC)) return -1;
#endif
#ifdef O_DIRECT
    /* Direct disk access. */
    if (ins(d, "O_DIRECT", (long)O_DIRECT)) return -1;
#endif
#ifdef O_DIRECTORY
    /* Must be a directory.      */
    if (ins(d, "O_DIRECTORY", (long)O_DIRECTORY)) return -1;
#endif
#ifdef O_NOFOLLOW
    /* Do not follow links.      */
    if (ins(d, "O_NOFOLLOW", (long)O_NOFOLLOW)) return -1;
#endif
#ifdef O_NOATIME
    /* Do not update the access time. */
    if (ins(d, "O_NOATIME", (long)O_NOATIME)) return -1;
#endif

    /* These come from sysexits.h */
#ifdef EX_OK
    if (ins(d, "EX_OK", (long)EX_OK)) return -1;
#endif /* EX_OK */
#ifdef EX_USAGE
    if (ins(d, "EX_USAGE", (long)EX_USAGE)) return -1;
#endif /* EX_USAGE */
#ifdef EX_DATAERR
    if (ins(d, "EX_DATAERR", (long)EX_DATAERR)) return -1;
#endif /* EX_DATAERR */
#ifdef EX_NOINPUT
    if (ins(d, "EX_NOINPUT", (long)EX_NOINPUT)) return -1;
#endif /* EX_NOINPUT */
#ifdef EX_NOUSER
    if (ins(d, "EX_NOUSER", (long)EX_NOUSER)) return -1;
#endif /* EX_NOUSER */
#ifdef EX_NOHOST
    if (ins(d, "EX_NOHOST", (long)EX_NOHOST)) return -1;
#endif /* EX_NOHOST */
#ifdef EX_UNAVAILABLE
    if (ins(d, "EX_UNAVAILABLE", (long)EX_UNAVAILABLE)) return -1;
#endif /* EX_UNAVAILABLE */
#ifdef EX_SOFTWARE
    if (ins(d, "EX_SOFTWARE", (long)EX_SOFTWARE)) return -1;
#endif /* EX_SOFTWARE */
#ifdef EX_OSERR
    if (ins(d, "EX_OSERR", (long)EX_OSERR)) return -1;
#endif /* EX_OSERR */
#ifdef EX_OSFILE
    if (ins(d, "EX_OSFILE", (long)EX_OSFILE)) return -1;
#endif /* EX_OSFILE */
#ifdef EX_CANTCREAT
    if (ins(d, "EX_CANTCREAT", (long)EX_CANTCREAT)) return -1;
#endif /* EX_CANTCREAT */
#ifdef EX_IOERR
    if (ins(d, "EX_IOERR", (long)EX_IOERR)) return -1;
#endif /* EX_IOERR */
#ifdef EX_TEMPFAIL
    if (ins(d, "EX_TEMPFAIL", (long)EX_TEMPFAIL)) return -1;
#endif /* EX_TEMPFAIL */
#ifdef EX_PROTOCOL
    if (ins(d, "EX_PROTOCOL", (long)EX_PROTOCOL)) return -1;
#endif /* EX_PROTOCOL */
#ifdef EX_NOPERM
    if (ins(d, "EX_NOPERM", (long)EX_NOPERM)) return -1;
#endif /* EX_NOPERM */
#ifdef EX_CONFIG
    if (ins(d, "EX_CONFIG", (long)EX_CONFIG)) return -1;
#endif /* EX_CONFIG */
#ifdef EX_NOTFOUND
    if (ins(d, "EX_NOTFOUND", (long)EX_NOTFOUND)) return -1;
#endif /* EX_NOTFOUND */

#ifdef HAVE_SPAWNV
    if (ins(d, "P_WAIT", (long)_P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)_P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)_OLD_P_OVERLAY)) return -1;
    if (ins(d, "P_NOWAITO", (long)_P_NOWAITO)) return -1;
    if (ins(d, "P_DETACH", (long)_P_DETACH)) return -1;
#endif
  return 0;
}

#define INITFUNC initedk2
#define MODNAME "edk2"

PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m;

#ifndef UEFI_C_SOURCE
  PyObject *v;
#endif

    m = Py_InitModule3(MODNAME,
                       edk2_methods,
                       edk2__doc__);
    if (m == NULL)
        return;

#ifndef UEFI_C_SOURCE
    /* Initialize environ dictionary */
    v = convertenviron();
    Py_XINCREF(v);
    if (v == NULL || PyModule_AddObject(m, "environ", v) != 0)
        return;
    Py_DECREF(v);
#endif  /* UEFI_C_SOURCE */

    if (all_ins(m))
        return;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

#ifdef HAVE_PUTENV
    if (edk2_putenv_garbage == NULL)
        edk2_putenv_garbage = PyDict_New();
#endif

    if (!initialized) {
        stat_result_desc.name = MODNAME ".stat_result";
        stat_result_desc.fields[2].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[3].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[4].name = PyStructSequence_UnnamedField;
        PyStructSequence_InitType(&StatResultType, &stat_result_desc);
        structseq_new = StatResultType.tp_new;
        StatResultType.tp_new = statresult_new;

        //statvfs_result_desc.name = MODNAME ".statvfs_result";
        //PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);
#ifdef NEED_TICKS_PER_SECOND
#  if defined(HAVE_SYSCONF) && defined(_SC_CLK_TCK)
        ticks_per_second = sysconf(_SC_CLK_TCK);
#  elif defined(HZ)
        ticks_per_second = HZ;
#  else
        ticks_per_second = 60; /* magic fallback value; may be bogus */
#  endif
#endif
    }
    Py_INCREF((PyObject*) &StatResultType);
    PyModule_AddObject(m, "stat_result", (PyObject*) &StatResultType);
    //Py_INCREF((PyObject*) &StatVFSResultType);
    //PyModule_AddObject(m, "statvfs_result",
    //                   (PyObject*) &StatVFSResultType);
    initialized = 1;

}

#ifdef __cplusplus
}
#endif


