/** @file
  Provides a service to retrieve a pointer to the start of HOB list.
  Only available to DXE module types.

  This library does not contain any functions or macros.  It simply exports a global
  pointer to the start of HOB list as defined in the Platform Initialization Driver
  Execution Environment Core Interface Specification.  The library constructor must
  initialize this global pointer to the start of HOB list, so it is available at the
  module's entry point.  Since there is overhead in looking up the pointer to the start
  of HOB list, only those modules that actually require access to the HOB list
  should use this library.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef DXE_HOB_LIST_LIB_H_
#define DXE_HOB_LIST_LIB_H_

///
/// Cache copy of the start of HOB list
///
extern VOID  *gHobList;

#endif
