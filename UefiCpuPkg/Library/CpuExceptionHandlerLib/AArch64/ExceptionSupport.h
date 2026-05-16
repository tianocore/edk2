/** @file

 Exception handling helper assembly functions

 Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

VOID
RegisterEl0Stack (
  IN  VOID  *Stack
  );

VOID
ExceptionHandlersStart (
  VOID
  );
