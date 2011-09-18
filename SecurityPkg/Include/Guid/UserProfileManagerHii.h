/** @file
  GUID used as HII FormSet and HII Package list GUID in UserProfileManagerDxe driver.

Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __USER_PROFILE_MANAGER_HII_H__
#define __USER_PROFILE_MANAGER_HII_H__

#define USER_PROFILE_MANAGER_GUID \
  { \
    0xc35f272c, 0x97c2, 0x465a, { 0xa2, 0x16, 0x69, 0x6b, 0x66, 0x8a, 0x8c, 0xfe } \
  }

extern EFI_GUID gUserProfileManagerGuid;

#endif