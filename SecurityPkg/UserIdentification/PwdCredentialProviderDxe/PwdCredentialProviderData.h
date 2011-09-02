/** @file
  Data structure used by the Password Credential Provider driver.
    
Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PWD_CREDENTIAL_PROVIDER_DATA_H_
#define _PWD_CREDENTIAL_PROVIDER_DATA_H_

#define PWD_CREDENTIAL_PROVIDER_GUID \
  { \
    0x78b9ec8b, 0xc000, 0x46c5, { 0xac, 0x93, 0x24, 0xa0, 0xc1, 0xbb, 0x0, 0xce } \
  }

//
// Forms definition
//
#define FORMID_GET_PASSWORD_FORM      1

//
// Key defination
// 
#define KEY_GET_PASSWORD              0x1000

#endif
