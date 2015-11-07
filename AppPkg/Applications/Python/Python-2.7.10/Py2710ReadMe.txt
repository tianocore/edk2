                                EDK II Python
                                    ReadMe
                                 Release 1.02
                                 18 Jan. 2013


1. OVERVIEW
===========
This document is devoted to general information on building and setup of the
Python environment for UEFI 2.3, the invocation of the interpreter, and things
that make working with Python easier.

It is assumed that you already have either UDK2010 or a current snapshot of
the EDK II sources from www.tianocore.org, and that you can successfully build
packages within that distribution.

2. Release Notes
================
  1)  All C extension modules must be statically linked (built in)
  2)  The site and os modules must exist as discrete files in ...\lib\python.27
  3)  User-specific configurations are not supported.
  4)  Environment variables are not supported.

3. Getting and Building Python
======================================================
  3.1 Getting Python
  ==================
  Currently only version 2.7.2 of the CPython distribution is supported.  For development
  ease, a subset of the Python 2.7.2 distribution has been included in the AppPkg source
  tree.  If a full distribution is desired, the Python-2.7.2 directory can be removed or
  renamed and the full source code downloaded from http://www.python.org/ftp/python/2.7.2/.

  A.  Within your EDK II development tree, extract the Python distribution into
    AppPkg/Applications/Python.  This should create the
    AppPkg/Applications/Python/Python-2.7.2 directory.

  B.  Copy the files from PyMod-2.7.2 into the corresponding directories within
    the Python-2.7.2 tree.  This will overwrite existing files with files
    modified for UEFI usage.

  3.2 Building Python
  ===================
  A.  Edit Efi/config.c to enable the built-in modules you need.
        Mandatory Built-in Modules:
        edk2      errno       imp         marshal

      Additional built-in modules which are required to use the help()
      functionality provided by PyDoc, are:
        _codecs     _collections    _functools    _random
        _sre        _struct         _weakref      binascii
        cStringIO   gc              itertools     math
        operator    time

  B.  Edit AppPkg/AppPkg.dsc to enable (uncomment) the PythonCore.inf line
    within the [Components] section.

  C.  Build AppPkg, which includes Python, using the standard "build" command:
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
           |- \python.27            Directory containing the Python library modules.
               |- \lib-dynload      Dynamically loadable Python extensions.
               |- \site-packages    Site-specific packages and modules.


5. Installing Python
====================
These directories, on the target system, are populated from the development
system as follows:

  * \Efi\Tools receives a copy of Build/AppPkg/DEBUG_VS2005/X64/Python.efi.
                                               ^^^^^ ^^^^^^
    Modify the host path to match the your build type and compiler.

  * The \Efi\StdLib\etc directory is populated from the StdLib/Efi/StdLib/etc
    source directory.

  * Directory \Efi\StdLib\lib\python.27 is populated with packages and modules
    from the AppPkg/Applications/Python/Python-2.7.2/Lib directory.
    The recommended minimum set of modules (.py, .pyc, and/or .pyo):
        os      stat      ntpath      warnings      traceback
        site    types     copy_reg    linecache     genericpath

  * Python C Extension Modules built as dynamically loadable extensions go into
    the \Efi\StdLib\lib\python.27\lib-dynload directory.  This functionality is not
    yet implemented.


6. Example: Enabling socket support
===================================
  1.  enable {"_socket", init_socket}, in Efi\config.c
  2.  enable Python-2.7.2/Modules/socketmodule.c in PythonCore.inf.
  3.  copy socket.py over to /Efi/StdLib/lib/python.27 on your target system.
  4.  Make sure dependent modules are present(.py) or built in(.c):
        functools, types, os, sys, warnings, cStringIO, StringIO, errno

  5.  build -a X64 -p AppPkg\AppPkg.dsc
  6.  copy Build\AppPkg\DEBUG_VS2005\X64\Python.efi to \Efi\Tools on your target system.
                                ^^^^ Modify as needed


7. Supported C Modules
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


8. Tested Python Library Modules
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
