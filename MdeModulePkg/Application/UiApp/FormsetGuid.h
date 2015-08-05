/** @file
  FrontPage routines to handle the callbacks and browser calls

Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


//**/
#ifndef _FORMSET_GUID_H_
#define _FORMSET_GUID_H_

#include "BootMaint/FormGuid.h"

#define FRONT_PAGE_FORMSET_GUID  { 0x9e0c30bc, 0x3f06, 0x4ba6, 0x82, 0x88, 0x9, 0x17, 0x9b, 0x85, 0x5d, 0xbe }

// Used by Boot manager form
#define BOOT_MANAGER_FORMSET_GUID  { 0x847bc3fe, 0xb974, 0x446d, 0x94, 0x49, 0x5a, 0xd5, 0x41, 0x2e, 0x99, 0x3b }

#define BOOT_MANAGER_FORM_ID     0x1000


// Used by Device manager form.
#define DEVICE_MANAGER_FORMSET_GUID  { 0x3ebfa8e6, 0x511d, 0x4b5b, 0xa9, 0x5f, 0xfb, 0x38, 0x26, 0xf, 0x1c, 0x27 }

#define DEVICE_MANAGER_FORM_ID             0x2000
#define NETWORK_DEVICE_LIST_FORM_ID        0x2001
#define NETWORK_DEVICE_FORM_ID             0x2002

#define DEVICE_KEY_OFFSET                  0x2000
#define NETWORK_DEVICE_LIST_KEY_OFFSET     0x2100
#define MAX_KEY_SECTION_LEN                0x0100

#define QUESTION_NETWORK_DEVICE_ID         0x2FFF

#define LABEL_DEVICES_LIST                 0x2100
#define LABEL_NETWORK_DEVICE_LIST_ID       0x2101
#define LABEL_NETWORK_DEVICE_ID            0x2102
#define LABEL_END                          0xffff
#define LABEL_FORM_ID_OFFSET               0x0100
#define LABEL_VBIOS                        0x0040


#endif
