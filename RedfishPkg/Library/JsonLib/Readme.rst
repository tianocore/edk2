=============================================================================
                             Introduction
=============================================================================
  Jansson is a C library for encoding, decoding and manipulating JSON data.
Its main features and design principles are:

  - Simple and intuitive API and data model
  - Comprehensive documentation
  - No dependencies on other libraries
  - Full Unicode support (UTF-8)
  - Extensive test suite

  Jansson is licensed under the MIT license(refer to ReadMe.rst under edk2).
It is used in production and its API is stable. It works on numerous
platforms, including numerous Unix like systems and Windows. It's suitable
for use on any system, including desktop, server, and small embedded systems.

  In UEFI/EDKII environment, Redfish project consumes jansson to achieve JSON
operations.

* Jansson version on edk2: 2.13.1, API reference is on the below URL,
  https://jansson.readthedocs.io/en/2.13/apiref.html

* EDKII jansson library wrapper:
   - JsonLib.h:
     This is the denifitions of EDKII JSON APIs which are mapped to
     jannson funcitons accordingly.

*Known issue:
   Build fail with jansson/src/load.c, add code in load.c to conditionally
   use stdin according to HAVE_UNISTD_H macro. The PR is submitted to
   jansson open source community.
   https://github.com/akheron/jansson/pull/558


