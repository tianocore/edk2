/** @file
  Data structure used by the user identify manager driver.
    
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _USER_IDENTIFY_MANAGER_DATA_H_
#define _USER_IDENTIFY_MANAGER_DATA_H_

#include "UserIdentifyManagerStrDefs.h"

//
// Guid used in user profile saving and in form browser.
//
#define USER_IDENTIFY_MANAGER_GUID \
  { \
    0x3ccd3dd8, 0x8d45, 0x4fed, { 0x96, 0x2d, 0x2b, 0x38, 0xcd, 0x82, 0xb3, 0xc4 } \
  }

//
// Forms definition.
//
#define FORMID_USER_FORM      1
#define FORMID_PROVIDER_FORM  2

//
// Labels definition.
//
#define LABEL_USER_NAME       0x1000
#define LABEL_PROVIDER_NAME   0x3000
#define LABEL_END             0xffff
#define FORM_OPEN_QUESTION_ID 0xfffe

#endif
