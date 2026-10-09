/** @file
  The root header file that provides Framework extension to UEFI/PI for modules. It can be included by
  DXE, RUNTIME and SMM type modules that use Framework definitions.


  This header file includes Framework extension definitions common to DXE
  modules.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _FRAMEWORK_DXE_H_
#define _FRAMEWORK_DXE_H_

#include <PiDxe.h>

#include <Framework/FrameworkInternalFormRepresentation.h>
#include <Framework/FirmwareVolumeImageFormat.h>
#include <Framework/FirmwareVolumeHeader.h>
#include <Framework/Hob.h>
#include <Framework/BootScript.h>
#include <Framework/StatusCode.h>
#include <Framework/DxeCis.h>

#endif
