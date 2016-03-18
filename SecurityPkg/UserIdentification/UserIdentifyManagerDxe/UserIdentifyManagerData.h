/** @file
  Data structure used by the user identify manager driver.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _USER_IDENTIFY_MANAGER_DATA_H_
#define _USER_IDENTIFY_MANAGER_DATA_H_

#include <Guid/UserIdentifyManagerHii.h>

//
// Forms definition.
//
#define FORMID_USER_FORM      1
#define FORMID_PROVIDER_FORM  2
#define FORMID_INVALID_FORM   0x0FFF

//
// Labels definition.
//
#define LABEL_USER_NAME       0x1000
#define LABEL_PROVIDER_NAME   0x3000
#define LABEL_END             0xffff
#define FORM_OPEN_QUESTION_ID 0xfffe

#endif
