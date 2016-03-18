/** @file
    Return the initial module search path.

    Search in specified locations for the associated Python libraries.

    Py_GetPath returns module_search_path.
    Py_GetPrefix returns PREFIX
    Py_GetExec_Prefix returns PREFIX
    Py_GetProgramFullPath returns the full path to the python executable.

    These are built dynamically so that the proper volume name can be prefixed
    to the paths.

    For the EDK II, UEFI, implementation of Python, PREFIX and EXEC_PREFIX
    are set as follows:
      PREFIX      = /Efi/StdLib
      EXEC_PREFIX = PREFIX

    The following final paths are assumed:
      /Efi/Tools/Python.efi                     The Python executable.
      /Efi/StdLib/lib/python.VERSION            The platform independent Python modules.
      /Efi/StdLib/lib/python.VERSION/dynalib    Dynamically loadable Python extension modules.

    Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <Python.h>
#include <osdefs.h>
#include  <ctype.h>

#ifdef __cplusplus
 extern "C" {
#endif

/* VERSION must be at least two characters long. */
#ifndef VERSION
  #define VERSION     "27"
#endif

#ifndef VPATH
  #define VPATH       "."
#endif

/* Search path entry delimiter */
#ifdef DELIM
  #define sDELIM        ";"
#endif

#ifndef PREFIX
  #define PREFIX      "/Efi/StdLib"
#endif

#ifndef EXEC_PREFIX
  #define EXEC_PREFIX PREFIX
#endif

#ifndef   LIBPYTHON
  #define   LIBPYTHON     "lib/python." VERSION
#endif

#ifndef PYTHONPATH
  #ifdef HAVE_ENVIRONMENT_OPS
    #define PYTHONPATH  PREFIX LIBPYTHON sDELIM \
                        EXEC_PREFIX LIBPYTHON "/lib-dynload"
  #else
    #define PYTHONPATH  LIBPYTHON
  #endif
#endif

#ifndef LANDMARK
#define LANDMARK    "os.py"
#endif

static char   prefix[MAXPATHLEN+1];
static char   exec_prefix[MAXPATHLEN+1];
static char   progpath[MAXPATHLEN+1];
static char  *module_search_path          = NULL;
static char   lib_python[]                = LIBPYTHON;
static char   volume_name[32]             = { 0 };

/** Determine if "ch" is a separator character.

    @param[in]  ch      The character to test.

    @retval     TRUE    ch is a separator character.
    @retval     FALSE   ch is NOT a separator character.
**/
static int
is_sep(char ch)
{
#ifdef ALTSEP
  return ch == SEP || ch == ALTSEP;
#else
  return ch == SEP;
#endif
}

/** Reduce a path by its last element.

    The last element (everything to the right of the last separator character)
    in the path, dir, is removed from the path.  Parameter dir is modified in place.

    @param[in,out]    dir   Pointer to the path to modify.
**/
static void
reduce(char *dir)
{
    size_t i = strlen(dir);
    while (i > 0 && !is_sep(dir[i]))
        --i;
    dir[i] = '\0';
}

#ifndef UEFI_C_SOURCE
/** Does filename point to a file and not directory?

    @param[in]    filename    The fully qualified path to the object to test.

    @retval       0     Filename was not found, or is a directory.
    @retval       1     Filename refers to a regular file.
**/
static int
isfile(char *filename)
{
    struct stat buf;
    if (stat(filename, &buf) != 0) {
      return 0;
    }
    //if (!S_ISREG(buf.st_mode))
    if (S_ISDIR(buf.st_mode)) {
      return 0;
    }
    return 1;
}

/** Determine if filename refers to a Python module.

    A Python module is indicated if the file exists, or if the file with
    'o' or 'c' appended exists.

    @param[in]    filename    The fully qualified path to the object to test.

    @retval       0
**/
static int
ismodule(char *filename)
{
  if (isfile(filename)) {
    //if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: file = \"%s\"\n", __func__, __LINE__, filename);
    return 1;
  }

    /* Check for the compiled version of prefix. */
    if (strlen(filename) < MAXPATHLEN) {
        strcat(filename, Py_OptimizeFlag ? "o" : "c");
        if (isfile(filename)) {
          return 1;
        }
    }
    return 0;
}

/** Does filename point to a directory?

    @param[in]    filename    The fully qualified path to the object to test.

    @retval       0     Filename was not found, or is not a regular file.
    @retval       1     Filename refers to a directory.
**/
static int
isdir(char *filename)
{
    struct stat buf;

    if (stat(filename, &buf) != 0)
        return 0;

    if (!S_ISDIR(buf.st_mode))
        return 0;

    return 1;
}
#endif  /* UEFI_C_SOURCE */

/** Determine if a path is absolute, or not.
    An absolute path consists of a volume name, "VOL:", followed by a rooted path,
    "/path/elements".  If both of these components are present, the path is absolute.

    Let P be a pointer to the path to test.
    Let A be a pointer to the first ':' in P.
    Let B be a pointer to the first '/' or '\\' in P.

    If A and B are not NULL
      If (A-P+1) == (B-P) then the path is absolute.
    Otherwise, the path is NOT absolute.

    @param[in]  path    The path to test.

    @retval     -1      Path is absolute but lacking volume name.
    @retval      0      Path is NOT absolute.
    @retval      1      Path is absolute.
*/
static int
is_absolute(char *path)
{
  char  *A;
  char  *B;

  A = strchr(path, ':');
  B = strpbrk(path, "/\\");

  if(B != NULL) {
    if(A == NULL) {
      if(B == path) {
        return -1;
      }
    }
    else {
      if(((A - path) + 1) == (B - path)) {
        return 1;
      }
    }
  }
  return 0;
}


/** Add a path component, by appending stuff to buffer.
    buffer must have at least MAXPATHLEN + 1 bytes allocated, and contain a
    NUL-terminated string with no more than MAXPATHLEN characters (not counting
    the trailing NUL).  It's a fatal error if it contains a string longer than
    that (callers must be careful!).  If these requirements are met, it's
    guaranteed that buffer will still be a NUL-terminated string with no more
    than MAXPATHLEN characters at exit.  If stuff is too long, only as much of
    stuff as fits will be appended.

    @param[in,out]    buffer    The path to be extended.
    @param[in]        stuff     The stuff to join onto the path.
*/
static void
joinpath(char *buffer, char *stuff)
{
  size_t n, k;

  k = 0;
  if (is_absolute(stuff) == 1) {
    n = 0;
  }
  else {
    n = strlen(buffer);
    if(n == 0) {
      strncpy(buffer, volume_name, MAXPATHLEN);
      n = strlen(buffer);
    }
    /* We must not use an else clause here because we want to test n again.
        volume_name may have been empty.
    */
    if (n > 0 && n < MAXPATHLEN) {
      if(!is_sep(buffer[n-1])) {
        buffer[n++] = SEP;
      }
      if(is_sep(stuff[0]))   ++stuff;
    }
  }
  if (n > MAXPATHLEN)
    Py_FatalError("buffer overflow in getpath.c's joinpath()");
  k = strlen(stuff);
  if (n + k > MAXPATHLEN)
    k = MAXPATHLEN - n;
  strncpy(buffer+n, stuff, k);
  buffer[n+k] = '\0';
}

/** Is filename an executable file?

    An executable file:
      1) exists
      2) is a file, not a directory
      3) has a name ending with ".efi"
      4) Only has a single '.' in the name.

    If basename(filename) does not contain a '.', append ".efi" to filename
    If filename ends in ".efi", it is executable, else it isn't.

    This routine is used to when searching for the file named by argv[0].
    As such, there is no need to search for extensions other than ".efi".

    @param[in]    filename      The name of the file to test.  It may, or may not, have an extension.

    @retval       0     filename already has a path other than ".efi", or it doesn't exist, or is a directory.
    @retval       1     filename refers to an executable file.
**/
static int
isxfile(char *filename)
{
    struct stat  buf;
    char        *bn;
    char        *newbn;
    int          bnlen;

    bn = basename(filename);            // Separate off the file name component
    reduce(filename);                   // and isolate the path component
    bnlen = strlen(bn);
    newbn = strrchr(bn, '.');           // Does basename contain a period?
    if(newbn == NULL) {                   // Does NOT contain a period.
      newbn = &bn[bnlen];
      strncpyX(newbn, ".efi", MAXPATHLEN - bnlen);    // append ".efi" to basename
      bnlen += 4;
    }
    else if(strcmp(newbn, ".efi") != 0) {
      return 0;                         // File can not be executable.
    }
    joinpath(filename, bn);             // Stitch path and file name back together

    if (stat(filename, &buf) != 0) {    // Now, verify that file exists
      return 0;
    }
    if(S_ISDIR(buf.st_mode)) {          // And it is not a directory.
      return 0;
    }

    return 1;
}

/** Copy p into path, ensuring that the result is an absolute path.

    copy_absolute requires that path be allocated at least
    MAXPATHLEN + 1 bytes and that p be no more than MAXPATHLEN bytes.

    @param[out]     path    Destination to receive the absolute path.
    @param[in]      p       Path to be tested and possibly converted.
**/
static void
copy_absolute(char *path, char *p)
{
  if (is_absolute(p) == 1)
        strcpy(path, p);
  else {
    if (!getcwd(path, MAXPATHLEN)) {
      /* unable to get the current directory */
      if(volume_name[0] != 0) {
        strcpy(path, volume_name);
        joinpath(path, p);
      }
      else
        strcpy(path, p);
      return;
    }
    if (p[0] == '.' && is_sep(p[1]))
        p += 2;
    joinpath(path, p);
  }
}

/** Modify path so that the result is an absolute path.
    absolutize() requires that path be allocated at least MAXPATHLEN+1 bytes.

    @param[in,out]    path    The path to be made absolute.
*/
static void
absolutize(char *path)
{
    char buffer[MAXPATHLEN + 1];

    if (is_absolute(path) == 1)
        return;
    copy_absolute(buffer, path);
    strcpy(path, buffer);
}

/** Extract the volume name from a path.

    @param[out]   Dest    Pointer to location in which to store the extracted volume name.
    @param[in]    path    Pointer to the path to extract the volume name from.
**/
static void
set_volume(char *Dest, char *path)
{
  size_t    VolLen;

  if(is_absolute(path)) {
    VolLen = strcspn(path, "/\\:");
    if((VolLen != 0) && (path[VolLen] == ':')) {
      (void) strncpyX(Dest, path, VolLen + 1);
    }
  }
}


/** Determine paths.

    Two directories must be found, the platform independent directory
    (prefix), containing the common .py and .pyc files, and the platform
    dependent directory (exec_prefix), containing the shared library
    modules.  Note that prefix and exec_prefix are the same directory
    for UEFI installations.

    Separate searches are carried out for prefix and exec_prefix.
    Each search tries a number of different locations until a ``landmark''
    file or directory is found.  If no prefix or exec_prefix is found, a
    warning message is issued and the preprocessor defined PREFIX and
    EXEC_PREFIX are used (even though they may not work); python carries on
    as best as is possible, but some imports may fail.

    Before any searches are done, the location of the executable is
    determined.  If argv[0] has one or more slashes in it, it is used
    unchanged.  Otherwise, it must have been invoked from the shell's path,
    so we search %PATH% for the named executable and use that.  If the
    executable was not found on %PATH% (or there was no %PATH% environment
    variable), the original argv[0] string is used.

    Finally, argv0_path is set to the directory containing the executable
    (i.e. the last component is stripped).

    With argv0_path in hand, we perform a number of steps.  The same steps
    are performed for prefix and for exec_prefix, but with a different
    landmark.

    The prefix landmark will always be lib/python.VERSION/os.py and the
    exec_prefix will always be lib/python.VERSION/dynaload, where VERSION
    is Python's version number as defined at the beginning of this file.

    First. See if the %PYTHONHOME% environment variable points to the
    installed location of the Python libraries.  If %PYTHONHOME% is set, then
    it points to prefix and exec_prefix.  %PYTHONHOME% can be a single
    directory, which is used for both, or the prefix and exec_prefix
    directories separated by the DELIM character.

    Next. Search the directories pointed to by the preprocessor variables
    PREFIX and EXEC_PREFIX.  These paths are prefixed with the volume name
    extracted from argv0_path.  The volume names correspond to the UEFI
    shell "map" names.

    That's it!

    Well, almost.  Once we have determined prefix and exec_prefix, the
    preprocessor variable PYTHONPATH is used to construct a path.  Each
    relative path on PYTHONPATH is prefixed with prefix.  Then the directory
    containing the shared library modules is appended.  The environment
    variable $PYTHONPATH is inserted in front of it all.  Finally, the
    prefix and exec_prefix globals are tweaked so they reflect the values
    expected by other code, by stripping the "lib/python$VERSION/..." stuff
    off.  This seems to make more sense given that currently the only
    known use of sys.prefix and sys.exec_prefix is for the ILU installation
    process to find the installed Python tree.

    The final, fully resolved, paths should look something like:
      fs0:/Efi/Tools/python.efi
      fs0:/Efi/StdLib/lib/python27
      fs0:/Efi/StdLib/lib/python27/dynaload

**/
static void
calculate_path(void)
{
    extern char *Py_GetProgramName(void);

    static char delimiter[2] = {DELIM, '\0'};
    static char separator[2] = {SEP, '\0'};
    char *pythonpath = PYTHONPATH;
    char *rtpypath = Py_GETENV("PYTHONPATH");
    //char *home = Py_GetPythonHome();
    char *path = getenv("PATH");
    char *prog = Py_GetProgramName();
    char argv0_path[MAXPATHLEN+1];
    char zip_path[MAXPATHLEN+1];
    char *buf;
    size_t bufsz;
    size_t prefixsz;
    char *defpath;


/* ###########################################################################
      Determine path to the Python.efi binary.
      Produces progpath, argv0_path, and volume_name.
########################################################################### */

    /* If there is no slash in the argv0 path, then we have to
     * assume python is on the user's $PATH, since there's no
     * other way to find a directory to start the search from.  If
     * $PATH isn't exported, you lose.
     */
    if (strchr(prog, SEP))
            strncpy(progpath, prog, MAXPATHLEN);
    else if (path) {
      while (1) {
        char *delim = strchr(path, DELIM);

        if (delim) {
                size_t len = delim - path;
                if (len > MAXPATHLEN)
                        len = MAXPATHLEN;
                strncpy(progpath, path, len);
                *(progpath + len) = '\0';
        }
        else
                strncpy(progpath, path, MAXPATHLEN);

        joinpath(progpath, prog);
        if (isxfile(progpath))
                break;

        if (!delim) {
                progpath[0] = '\0';
                break;
        }
        path = delim + 1;
      }
    }
    else
            progpath[0] = '\0';
    if ( (!is_absolute(progpath)) && (progpath[0] != '\0') )
            absolutize(progpath);
    strncpy(argv0_path, progpath, MAXPATHLEN);
    argv0_path[MAXPATHLEN] = '\0';
    set_volume(volume_name, argv0_path);

    reduce(argv0_path);
    /* At this point, argv0_path is guaranteed to be less than
       MAXPATHLEN bytes long.
    */

/* ###########################################################################
      Build the FULL prefix string, including volume name.
      This is the full path to the platform independent libraries.
########################################################################### */

        strncpy(prefix, volume_name, MAXPATHLEN);
        joinpath(prefix, PREFIX);
        joinpath(prefix, lib_python);

/* ###########################################################################
      Build the FULL path to the zipped-up Python library.
########################################################################### */

    strncpy(zip_path, prefix, MAXPATHLEN);
    zip_path[MAXPATHLEN] = '\0';
    reduce(zip_path);
    joinpath(zip_path, "python00.zip");
    bufsz = strlen(zip_path);   /* Replace "00" with version */
    zip_path[bufsz - 6] = VERSION[0];
    zip_path[bufsz - 5] = VERSION[1];

/* ###########################################################################
      Build the FULL path to dynamically loadable libraries.
########################################################################### */

        strncpy(exec_prefix, volume_name, MAXPATHLEN);
        joinpath(exec_prefix, EXEC_PREFIX);
        joinpath(exec_prefix, lib_python);
        joinpath(exec_prefix, "lib-dynload");

/* ###########################################################################
      Build the module search path.
########################################################################### */

    /* Reduce prefix and exec_prefix to their essence,
     * e.g. /usr/local/lib/python1.5 is reduced to /usr/local.
     * If we're loading relative to the build directory,
     * return the compiled-in defaults instead.
     */
    reduce(prefix);
    reduce(prefix);
    /* The prefix is the root directory, but reduce() chopped
     * off the "/". */
    if (!prefix[0]) {
      strcpy(prefix, volume_name);
    }
    bufsz = strlen(prefix);
    if(prefix[bufsz-1] == ':') {
      prefix[bufsz] = SEP;
      prefix[bufsz+1] = 0;
    }

    /* Calculate size of return buffer.
     */
    defpath = pythonpath;
    bufsz = 0;

    if (rtpypath)
        bufsz += strlen(rtpypath) + 1;

    prefixsz = strlen(prefix) + 1;

    while (1) {
        char *delim = strchr(defpath, DELIM);

        if (is_absolute(defpath) == 0)
            /* Paths are relative to prefix */
            bufsz += prefixsz;

        if (delim)
            bufsz += delim - defpath + 1;
        else {
            bufsz += strlen(defpath) + 1;
            break;
        }
        defpath = delim + 1;
    }

    bufsz += strlen(zip_path) + 1;
    bufsz += strlen(exec_prefix) + 1;

    /* This is the only malloc call in this file */
    buf = (char *)PyMem_Malloc(bufsz);

    if (buf == NULL) {
        /* We can't exit, so print a warning and limp along */
        fprintf(stderr, "Not enough memory for dynamic PYTHONPATH.\n");
        fprintf(stderr, "Using default static PYTHONPATH.\n");
        module_search_path = PYTHONPATH;
    }
    else {
        /* Run-time value of $PYTHONPATH goes first */
        if (rtpypath) {
            strcpy(buf, rtpypath);
            strcat(buf, delimiter);
        }
        else
            buf[0] = '\0';

        /* Next is the default zip path */
        strcat(buf, zip_path);
        strcat(buf, delimiter);

        /* Next goes merge of compile-time $PYTHONPATH with
         * dynamically located prefix.
         */
        defpath = pythonpath;
        while (1) {
            char *delim = strchr(defpath, DELIM);

            if (is_absolute(defpath) != 1) {
                strcat(buf, prefix);
                strcat(buf, separator);
            }

            if (delim) {
                size_t len = delim - defpath + 1;
                size_t end = strlen(buf) + len;
                strncat(buf, defpath, len);
                *(buf + end) = '\0';
            }
            else {
                strcat(buf, defpath);
                break;
            }
            defpath = delim + 1;
        }
        strcat(buf, delimiter);

        /* Finally, on goes the directory for dynamic-load modules */
        strcat(buf, exec_prefix);

        /* And publish the results */
        module_search_path = buf;
    }
        /*  At this point, exec_prefix is set to VOL:/Efi/StdLib/lib/python.27/dynalib.
            We want to get back to the root value, so we have to remove the final three
            segments to get VOL:/Efi/StdLib.  Because we don't know what VOL is, and
            EXEC_PREFIX is also indeterminate, we just remove the three final segments.
        */
        reduce(exec_prefix);
        reduce(exec_prefix);
        reduce(exec_prefix);
        if (!exec_prefix[0]) {
          strcpy(exec_prefix, volume_name);
        }
        bufsz = strlen(exec_prefix);
        if(exec_prefix[bufsz-1] == ':') {
          exec_prefix[bufsz] = SEP;
          exec_prefix[bufsz+1] = 0;
        }
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: module_search_path = \"%s\"\n", __func__, __LINE__, module_search_path);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: prefix             = \"%s\"\n", __func__, __LINE__, prefix);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: exec_prefix        = \"%s\"\n", __func__, __LINE__, exec_prefix);
    if (Py_VerboseFlag) PySys_WriteStderr("%s[%d]: progpath           = \"%s\"\n", __func__, __LINE__, progpath);
}


/* External interface */

char *
Py_GetPath(void)
{
    if (!module_search_path)
        calculate_path();
    return module_search_path;
}

char *
Py_GetPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return prefix;
}

char *
Py_GetExecPrefix(void)
{
    if (!module_search_path)
        calculate_path();
    return exec_prefix;
}

char *
Py_GetProgramFullPath(void)
{
    if (!module_search_path)
        calculate_path();
    return progpath;
}


#ifdef __cplusplus
}
#endif

