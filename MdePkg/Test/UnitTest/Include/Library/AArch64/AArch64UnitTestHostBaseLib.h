/** @file
  Unit Test Host BaseLib hooks.

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Prototype of service with no parameters and no return value.
**/
typedef
VOID
(EFIAPI *UNIT_TEST_HOST_BASE_LIB_VOID)(
  VOID
  );

/**
  Prototype of service that reads and returns a BOOLEAN value.

  @return The value read.
**/
typedef
BOOLEAN
(EFIAPI *UNIT_TEST_HOST_BASE_LIB_READ_BOOLEAN)(
  VOID
  );

///
/// Common services
///
typedef struct {
  UNIT_TEST_HOST_BASE_LIB_VOID            EnableInterrupts;
  UNIT_TEST_HOST_BASE_LIB_VOID            DisableInterrupts;
  UNIT_TEST_HOST_BASE_LIB_VOID            EnableDisableInterrupts;
  UNIT_TEST_HOST_BASE_LIB_READ_BOOLEAN    GetInterruptState;
} UNIT_TEST_HOST_BASE_LIB_COMMON;

///
/// Data structure that contains pointers structures of common services and CPU
/// architctuire specific services.  Support for additional CPU architectures
/// can be added to the end of this structure.
///
typedef struct {
  UNIT_TEST_HOST_BASE_LIB_COMMON    *Common;
} UNIT_TEST_HOST_BASE_LIB;

extern UNIT_TEST_HOST_BASE_LIB  gUnitTestHostBaseLib;
