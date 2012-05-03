/** @file
  GUID used as HII FormSet and HII Package list GUID in UserIdentifyManagerDxe driver.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __USER_IDENTIFY_MANAGER_HII_H__
#define __USER_IDENTIFY_MANAGER_HII_H__

#define USER_IDENTIFY_MANAGER_GUID \
  { \
    0x3ccd3dd8, 0x8d45, 0x4fed, { 0x96, 0x2d, 0x2b, 0x38, 0xcd, 0x82, 0xb3, 0xc4 } \
  }

extern EFI_GUID gUserIdentifyManagerGuid;

#endif
