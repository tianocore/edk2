/** @file
  Opal Password common header file.

Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Protocol/DevicePath.h>
#include <Guid/OpalDeviceLockBox.h>

#define OPAL_DEVICE_TYPE_UNKNOWN  0x0
#define OPAL_DEVICE_TYPE_ATA      0x1
#define OPAL_DEVICE_TYPE_NVME     0x2
