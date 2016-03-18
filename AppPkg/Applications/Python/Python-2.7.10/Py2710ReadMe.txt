                                EDK II Python
                                   ReadMe
                                Version 2.7.10
                                 Release 1.00
                                  3 Nov. 2015


1. OVERVIEW
===========
This document is devoted to general information on building and setup of the
Python environment for UEFI, the invocation of the interpreter, and things
that make working with Python easier.

It is assumed that you already have UDK2010 or later, or a current snapshot of
the EDK II sources from www.tianocore.org, and that you can successfully build
packages within that distribution.

2. Release Notes
================
  1)  All C extension modules must be statically linked (built in)
  2)  The site and os modules must exist as discrete files in ...\lib\python27.10
  3)  User-specific configurations are not supported.
  4)  Environment variables are not supported.

3. Getting and Building Python
======================================================
  3.1 Getting Python
  ==================
  This file describes the UEFI port of version 2.7.10 of the CPython distribution.
  For development ease, a subset of the Python 2.7.10 distribution has been
  included as part of the AppPkg/Applications/Python/Python-2.7.10 source tree.
  If this is sufficient, you may skip to section 3.2, Building Python.

  If a full distribution is desired, it can be merged into the Python-2.7.10
  source tree.  Directory AppPkg/Applications/Python/Python-2.7.10 corresponds
  to the root directory of the CPython 2.7.10 distribution.  The full
  CPython 2.7.10 source code may be downloaded from
  http://www.python.org/ftp/python/2.7.10/.

  A.  Within your EDK II development tree, extract the Python distribution into
    AppPkg/Applications/Python/Python-2.7.10.  This should merge the additional
    files into the source tree.  It will also create the following directories:
        Demo      Doc         Grammar     Mac       Misc
        PC        PCbuild     RISCOS      Tools

    The greatest change will be within the Python-2.7.10/Lib directory where
    many more packages and modules will be added.  These additional components
    may not have been ported to EDK II yet.

  3.2 Building Python
  ===================
  A.  From the AppPkg/Applications/Python/Python-2.7.10 directory, execute the
    srcprep.bat (srcprep.sh) script to copy the header files from within the
    PyMod-2.7.10 sub-tree into their corresponding directories within the
    distribution.  This step only needs to be performed prior to the first
    build of Python, or if one of the header files within the PyMod tree has been
    modified.

  B.  Edit PyMod-2.7.10\Modules\config.c to enable the built-in modules you need.
    By default, it is configured for the minimally required set of modules.
      Mandatory Built-in Modules:
        edk2      errno       imp         marshal

      Additional built-in modules which are required to use the help()
      functionality provided by PyDoc, are:
        _codecs     _collections    _functools    _random
        _sre        _struct         _weakref      binascii
        cStringIO   gc              itertools     math
        operator    time

  C.  Edit AppPkg/AppPkg.dsc to enable (uncomment) the Python2710.inf line
    within the [Components] section.

  D.  Build AppPkg using the standard "build" command:
    For example, to build Python for an X64 CPU architecture:
                    build -a X64 -p AppPkg\AppPkg.dsc

4. Python-related paths and files
=================================
Python depends upon the existence of several directories and files on the
target system.

  \EFI                              Root of the UEFI system area.
   |- \Tools                        Location of the Python.efi executable.
   |- \Boot                         UEFI specified Boot directory.
   |- \StdLib                       Root of the Standard Libraries sub-tree.
       |- \etc                      Configuration files used by libraries.
       |- \tmp                      Temporary files created by tmpfile(), etc.
       |- \lib                      Root of the libraries tree.
           |- \python27.10          Directory containing the Python library
               |                    modules.
               |- \lib-dynload      Dynamically loadable Python extensions.
               |- \site-packages    Site-specific packages and modules.

  NOTE: The name of the directory containing the Python library modules has
        changed in order to distinguish it from the library modules for
        version 2.7.2.

5. Installing Python
====================
These directories, on the target system, are populated from the development
system as follows:

  * \Efi\Tools receives a copy of Build/AppPkg/DEBUG_VS2015/X64/Python2710.efi.
                                               ^^^^^^^^^^^^^^^^
    Modify the host path to match your build type and compiler.

  * The \Efi\StdLib\etc directory is populated from the StdLib/Efi/StdLib/etc
    source directory.

  * Directory \Efi\StdLib\lib\python27.10 is populated with packages and modules
    from the AppPkg/Applications/Python/Python-2.7.10/Lib directory.
    The recommended minimum set of modules (.py, .pyc, and/or .pyo):
        os      stat      ntpath      warnings      traceback
        site    types     copy_reg    linecache     genericpath

  * Python C Extension Modules built as dynamically loadable extensions go into
    the \Efi\StdLib\lib\python.27\lib-dynload directory.  This functionality is not
    yet implemented.

  A script, libprep.bat (libprep.sh), is provided which facilitates the population
  of the target Lib directory.  Execute this script from within the
  AppPkg/Applications/Python/Python-2.7.10 directory, providing a single argument
  which is the path to the destination directory.  The appropriate contents of the
  AppPkg/Applications/Python/Python-2.7.10/Lib and
  AppPkg/Applications/Python/Python-2.7.10/PyMod-2.7.10/Lib directories will be
  recursively copied into the specified destination directory.

6. Example: Enabling socket support
===================================
  1.  enable {"_socket", init_socket}, in Efi\config.c
  2.  enable LibraryClasses BsdSocketLib and EfiSocketLib in PythonCore.inf.
  3.  Build Python2710
          build -a X64 -p AppPkg\AppPkg.dsc
  6.  copy Build\AppPkg\DEBUG_VS2005\X64\Python2710.efi to \Efi\Tools on your
      target system. Replace "DEBUG_VS2005\X64", in the source path, with
      values appropriate for your tool chain and processor architecture.

7. Running Python
=================
  Python must currently be run from an EFI FAT-32 partition, or volume, under
  the UEFI Shell.  At the Shell prompt enter the desired volume name, followed
  by a colon ':', then press Enter.  Python can then be executed by typing its
  name, followed by any desired options and arguments.

  EXAMPLE:
      2.0 Shell> fs0:
      2.0 FS0:\> python2710
      Python 2.7.10 (default, Oct 13 2015, 16:21:53) [C] on uefi
      Type "help", "copyright", "credits" or "license" for more information.
      >>> exit()
      2.0 FS0:\>

  NOTE:
      Python, as distributed, sends its interactive prompts to stderr.  If
      STDERR isn't enabled in UEFI Setup so that it's output goes to the
      console, it may appear that Python hangs on startup.  If this happens,
      one may be able to rectify the condition by typing "exit()" followed
      by <enter> to exit out of Python.  Then, type "exit" at the Shell prompt
      which should enter Setup where you can use the Boot Maintenance
      Manager to modify your Console settings.

  NOTE:
      Some platforms don't include the Setup utility, or don't allow STDERR to
      be modified.  In these cases, Python may be started with the '-#' option
      which will cause stderr to be the same as stdout and should allow
      Python to be used interactively on those platforms.

      Depending upon the version of Shell you are using, it may be necessary
      to escape the '#' character so that the Shell doesn't interpret it as
      the start of a comment.  The escape character is '^'.
      Example:
          python -^# -V

8. Supported C Modules
======================
    Module Name               C File(s)
  ===============       =============================================
  _ast                  Python/Python-ast.c
  _bisect               Modules/_bisectmodule.c
  _codecs               Modules/_codecsmodule.c
  _codecs_cn            Modules/cjkcodecs/_codecs_cn.c
  _codecs_hk            Modules/cjkcodecs/_codecs_hk.c
  _codecs_iso2022       Modules/cjkcodecs/_codecs_iso2022.c
  _codecs_jp            Modules/cjkcodecs/_codecs_jp
  _codecs_kr            Modules/cjkcodecs/_codecs_kr
  _codecs_tw            Modules/cjkcodecs/_codecs_tw
  _collections          Modules/_collectionsmodule.c
  _csv                  Modules/_csv.c
  _functools            Modules/_functoolsmodule.c
  _heapq                Modules/_heapqmodule.c
  _io                   Modules/_io/_iomodule.c       Modules/_io/*
  _json                 Modules/_json.c
  _md5                  Modules/md5module.c           Modules/md5.c
  _multibytecodec       Modules/cjkcodecs/_multibytecodec.c
  _random               Modules/_randommodule.c
  _sha                  Modules/shamodule.c
  _sha256               Modules/sha256module.c
  _sha512               Modules/sha512module.c
  _socket               Modules/socketmodule.c
  _sre                  Modules/_sre.c
  _struct               Modules/_struct.c
  _symtable             Modules/symtablemodule.c
  _weakref              Modules/_weakref.c
  array                 Modules/arraymodule.c
  binascii              Modules/binascii.c
  cmath                 Modules/cmathmodule.c
  cPickle               Modules/cPickle.c
  cStringIO             Modules/cStringIO.c
  datetime              Modules/datetimemodule.c
  edk2                  Modules/Efi/edk2module.c
  errno                 Modules/errnomodule.c
  future_builtins       Modules/future_builtins.c
  gc                    Modules/gcmodule.c
  imp                   Python/import.c
  itertools             Modules/itertoolsmodule.c
  marshal               Python/marshal.c
  math                  Modules/mathmodule.c          Modules/_math.c
  operator              Modules/operator.c
  parser                Modules/parsermodule.c
  select                Modules/selectmodule.c
  signal                Modules/signalmodule.c
  strop                 Modules/stropmodule.c
  time                  Modules/timemodule.c
  xxsubtype             Modules/xxsubtype.c
  zipimport             Modules/zipimport.c
  zlib                  Modules/zlibmodule.c          Modules/zlib/*


9. Tested Python Library Modules
================================
This is a partial list of the packages and modules of the Python Standard
Library that have been tested or used in some manner.

  encodings               genericpath.py            sha.py
  importlib               getopt.py                 SimpleHTTPServer.py
  json                    hashlib.py                site.py
  pydoc_data              heapq.py                  socket.py
  xml                     HTMLParser.py             SocketServer.py
  abc.py                  inspect.py                sre.py
  argparse.py             io.py                     sre_compile.py
  ast.py                  keyword.py                sre_constants.py
  atexit.py               linecache.py              sre_parse.py
  BaseHTTPServer.py       locale.py                 stat.py
  binhex.py               md5.py                    string.py
  bisect.py               modulefinder.py           StringIO.py
  calendar.py             ntpath.py                 struct.py
  cmd.py                  numbers.py                textwrap.py
  codecs.py               optparse.py               token.py
  collections.py          os.py                     tokenize.py
  copy.py                 platform.py               traceback.py
  copy_reg.py             posixpath.py              types.py
  csv.py                  pydoc.py                  warnings.py
  dummy_thread.py         random.py                 weakref.py
  fileinput.py            re.py                     xmllib.py
  formatter.py            repr.py                   zipfile.py
  functools.py            runpy.py                  expat

# # #
