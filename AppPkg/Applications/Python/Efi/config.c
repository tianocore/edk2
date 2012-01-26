/** @file
    Python Module configuration.

    Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
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
#ifndef MS_WINI64
extern void initaudioop(void);
#endif
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initfuture_builtins(void);
extern void initgc(void);
#ifndef MS_WINI64
extern void initimageop(void);
#endif
extern void initmath(void);
extern void init_md5(void);
extern void initedk2(void);
extern void initoperator(void);
extern void initsignal(void);
extern void init_sha(void);
extern void init_sha256(void);
extern void init_sha512(void);
extern void initstrop(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
extern void initcPickle(void);
#ifdef WIN32
extern void initmsvcrt(void);
extern void init_locale(void);
#endif
extern void init_codecs(void);
extern void init_weakref(void);
extern void init_hotshot(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_random(void);
extern void inititertools(void);
extern void init_collections(void);
extern void init_heapq(void);
extern void init_bisect(void);
extern void init_symtable(void);
extern void initmmap(void);
extern void init_csv(void);
extern void init_sre(void);
extern void initparser(void);
extern void init_winreg(void);
extern void init_struct(void);
extern void initdatetime(void);
extern void init_functools(void);
extern void init_json(void);
extern void initzlib(void);

extern void init_multibytecodec(void);
extern void init_codecs_cn(void);
extern void init_codecs_hk(void);
extern void init_codecs_iso2022(void);
extern void init_codecs_jp(void);
extern void init_codecs_kr(void);
extern void init_codecs_tw(void);
extern void init_subprocess(void);
extern void init_lsprof(void);
extern void init_ast(void);
extern void init_io(void);
extern void _PyWarnings_Init(void);

extern void init_socket(void);
extern void initselect(void);

/* tools/freeze/makeconfig.py marker for additional "extern" */
/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {

    {"array", initarray},
    {"_ast", init_ast},
    {"binascii", initbinascii},
    {"errno", initerrno},
    {"future_builtins", initfuture_builtins},
    {"gc", initgc},
    {"signal", initsignal},
    {"edk2", initedk2},
    {"operator", initoperator},
    {"_weakref", init_weakref},
    {"math", initmath},
    {"time", inittime},
    {"datetime", initdatetime},
    {"cStringIO", initcStringIO},
    {"_codecs", init_codecs},

    /* CJK codecs */
    {"_multibytecodec", init_multibytecodec},
    {"_codecs_cn", init_codecs_cn},
    {"_codecs_hk", init_codecs_hk},
    {"_codecs_iso2022", init_codecs_iso2022},
    {"_codecs_jp", init_codecs_jp},
    {"_codecs_kr", init_codecs_kr},
    {"_codecs_tw", init_codecs_tw},

    {"_bisect", init_bisect},
    {"_md5", init_md5},
    {"_sha", init_sha},
    {"_sha256", init_sha256},
    {"_sha512", init_sha512},
    {"_random", init_random},
    {"_heapq", init_heapq},
    {"itertools", inititertools},
    {"_collections", init_collections},
    {"_sre", init_sre},
    {"parser", initparser},
    {"_struct", init_struct},
    {"cPickle", initcPickle},

    {"strop", initstrop},
    {"_functools", init_functools},
    {"cmath", initcmath},
    {"_json", init_json},

    {"_socket", init_socket},
    {"select", initselect},

    {"xxsubtype", initxxsubtype},

#if 0
#ifndef MS_WINI64
    {"imageop", initimageop},
#endif
#ifdef WITH_THREAD
    {"thread", initthread},
#endif
#ifdef WIN32
    {"msvcrt", initmsvcrt},
    {"_locale", init_locale},
#endif
    /* XXX Should _subprocess go in a WIN32 block?  not WIN64? */
    //{"_subprocess", init_subprocess},

    //{"_hotshot", init_hotshot},
    //{"_lsprof", init_lsprof},
    //{"mmap", initmmap},
    //{"_winreg", init_winreg},
    {"_symtable", init_symtable},
    {"_csv", init_csv},
    {"zipimport", initzipimport},
    {"zlib", initzlib},
#endif

/* tools/freeze/makeconfig.py marker for additional "_inittab" entries */
/* -- ADDMODULE MARKER 2 -- */

    /* This module "lives in" with marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in with import.c */
    {"imp", initimp},

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"sys", NULL},
    {"exceptions", NULL},
    {"_warnings", _PyWarnings_Init},

    {"_io", init_io},

    /* Sentinel */
    {0, 0}
};
