/** @file
    Python Module configuration.

    Copyright (c) 2011-2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray(void);
extern void init_ast(void);
extern void initbinascii(void);
extern void init_bisect(void);
extern void initcmath(void);
extern void init_codecs(void);
extern void init_collections(void);
extern void initcPickle(void);
extern void initcStringIO(void);
extern void init_csv(void);
extern void init_ctypes(void);
extern void initdatetime(void);
extern void initedk2(void);
extern void initerrno(void);
extern void init_functools(void);
extern void initfuture_builtins(void);
extern void initgc(void);
extern void init_heapq(void);
extern void init_hotshot(void);
extern void initimp(void);
extern void init_io(void);
extern void inititertools(void);
extern void init_json(void);
extern void init_lsprof(void);
extern void initmath(void);
extern void init_md5(void);
extern void initmmap(void);
extern void initoperator(void);
extern void initparser(void);
extern void initpyexpat(void);
extern void init_random(void);
extern void initselect(void);
extern void init_sha(void);
extern void init_sha256(void);
extern void init_sha512(void);
extern void initsignal(void);
extern void init_socket(void);
extern void init_sre(void);
extern void initstrop(void);
extern void init_struct(void);
extern void init_subprocess(void);
extern void init_symtable(void);
extern void initthread(void);
extern void inittime(void);
extern void initunicodedata(void);
extern void init_weakref(void);
extern void init_winreg(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void initzlib(void);

extern void PyMarshal_Init(void);
extern void _PyWarnings_Init(void);

extern void init_multibytecodec(void);
extern void init_codecs_cn(void);
extern void init_codecs_hk(void);
extern void init_codecs_iso2022(void);
extern void init_codecs_jp(void);
extern void init_codecs_kr(void);
extern void init_codecs_tw(void);

struct _inittab _PyImport_Inittab[] = {

    //{"_ast", init_ast},
    //{"_bisect", init_bisect},               /* A fast version of bisect.py */
    //{"_csv", init_csv},
    //{"_heapq", init_heapq},                 /* A fast version of heapq.py */
    //{"_io", init_io},
    //{"_json", init_json},
    //{"_md5", init_md5},
    //{"_sha", init_sha},
    //{"_sha256", init_sha256},
    //{"_sha512", init_sha512},
    //{"_socket", init_socket},
    //{"_symtable", init_symtable},

    //{"array", initarray},
    //{"cmath", initcmath},
    //{"cPickle", initcPickle},
    //{"datetime", initdatetime},
    //{"future_builtins", initfuture_builtins},
    //{"parser", initparser},
    //{"pyexpat", initpyexpat},
    //{"select", initselect},
    //{"signal", initsignal},
    //{"strop", initstrop},                   /* redefines some string operations that are 100-1000 times faster */
    //{"unicodedata", initunicodedata},
    //{"xxsubtype", initxxsubtype},
    //{"zipimport", initzipimport},
    //{"zlib", initzlib},

    /* CJK codecs */
    //{"_multibytecodec", init_multibytecodec},
    //{"_codecs_cn", init_codecs_cn},
    //{"_codecs_hk", init_codecs_hk},
    //{"_codecs_iso2022", init_codecs_iso2022},
    //{"_codecs_jp", init_codecs_jp},
    //{"_codecs_kr", init_codecs_kr},
    //{"_codecs_tw", init_codecs_tw},

#ifdef WITH_THREAD
    {"thread", initthread},
#endif

    /* These modules are required for the full built-in help() facility provided by pydoc. */
    {"_codecs", init_codecs},
    {"_collections", init_collections},
    {"_functools", init_functools},
    {"_random", init_random},
    {"_sre", init_sre},
    {"_struct", init_struct},                   /* Required by the logging package. */
    {"_weakref", init_weakref},
    {"binascii", initbinascii},
    {"cStringIO", initcStringIO},               /* Required by several modules, such as logging. */
    {"gc", initgc},
    {"itertools", inititertools},
    {"math", initmath},
    {"operator", initoperator},
    {"time", inittime},

    /* These four modules should always be built in. */
    {"edk2", initedk2},
    {"errno", initerrno},
    {"imp", initimp},                   /* We get this for free from Python/import.c  */
    {"marshal", PyMarshal_Init},        /* We get this for free from Python/marshal.c */

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"sys", NULL},
    {"exceptions", NULL},
    {"_warnings", _PyWarnings_Init},

    /* Sentinel */
    {0, 0}
};
