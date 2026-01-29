/** @file
  Definition shared by VFR file and C file.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DRIVER_HEALTH_VFR_H_
#define _DRIVER_HEALTH_VFR_H_
#include <Guid/HiiPlatformSetupFormset.h>

#define DRIVER_HEALTH_MANAGER_FORMSET_GUID    { 0xcfb3b000, 0x0b63, 0x444b, { 0xb1, 0xd1, 0x12, 0xd5, 0xd9, 0x5d, 0xc4, 0xfc } }
#define DRIVER_HEALTH_CONFIGURE_FORMSET_GUID  { 0x4296d9f4, 0xf6fc, 0x4dde, { 0x86, 0x85, 0x8c, 0xe2, 0xd7, 0x9d, 0x90, 0xf0 } }

#define LABEL_BEGIN  0x2000
#define LABEL_END    0x2001

#define DRIVER_HEALTH_FORM_ID  0x1001

#define QUESTION_ID_REFRESH_MANAGER    0x0001
#define QUESTION_ID_REFRESH_CONFIGURE  0x0002

#define QUESTION_ID_DRIVER_HEALTH_BASE  0x0003

#endif
