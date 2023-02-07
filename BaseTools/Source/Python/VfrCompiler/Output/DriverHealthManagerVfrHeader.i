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
/** @file
  GUID indicates that the form set contains forms designed to be used
  for platform configuration and this form set will be displayed.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  GUID defined in UEFI 2.1.

**/

#ifndef __HII_PLATFORM_SETUP_FORMSET_GUID_H__
#define __HII_PLATFORM_SETUP_FORMSET_GUID_H__

#define EFI_HII_PLATFORM_SETUP_FORMSET_GUID \
  { 0x93039971, 0x8545, 0x4b04, { 0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe } }

#define EFI_HII_DRIVER_HEALTH_FORMSET_GUID \
  { 0xf22fc20c, 0x8cf4, 0x45eb, { 0x8e, 0x6, 0xad, 0x4e, 0x50, 0xb9, 0x5d, 0xd3 } }

#define EFI_HII_USER_CREDENTIAL_FORMSET_GUID \
  { 0x337f4407, 0x5aee, 0x4b83, { 0xb2, 0xa7, 0x4e, 0xad, 0xca, 0x30, 0x88, 0xcd } }

#define EFI_HII_REST_STYLE_FORMSET_GUID \
  { 0x790217bd, 0xbecf, 0x485b, { 0x91, 0x70, 0x5f, 0xf7, 0x11, 0x31, 0x8b, 0x27 } }

extern EFI_GUID  gEfiHiiPlatformSetupFormsetGuid;
extern EFI_GUID  gEfiHiiDriverHealthFormsetGuid;
extern EFI_GUID  gEfiHiiUserCredentialFormsetGuid;
extern EFI_GUID  gEfiHiiRestStyleFormsetGuid;

#endif
