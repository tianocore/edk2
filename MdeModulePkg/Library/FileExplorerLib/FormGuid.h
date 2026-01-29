/** @file
Formset guids, form id and VarStore data structure for File explorer library.

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _FILE_EXPLORER_FORM_GUID_H_
#define _FILE_EXPLORER_FORM_GUID_H_

#define EFI_FILE_EXPLORE_FORMSET_GUID \
  { \
  0xfe561596, 0xe6bf, 0x41a6, {0x83, 0x76, 0xc7, 0x2b, 0x71, 0x98, 0x74, 0xd0} \
  }

#define FORM_FILE_EXPLORER_ID                0x1000
#define FORM_ADD_NEW_FILE_ID                 0x2000
#define NEW_FILE_NAME_ID                     0x2001
#define KEY_VALUE_CREATE_FILE_AND_EXIT       0x2002
#define KEY_VALUE_NO_CREATE_FILE_AND_EXIT    0x2003
#define FORM_ADD_NEW_FOLDER_ID               0x3000
#define NEW_FOLDER_NAME_ID                   0x3001
#define KEY_VALUE_CREATE_FOLDER_AND_EXIT     0x3002
#define KEY_VALUE_NO_CREATE_FOLDER_AND_EXIT  0x3003

#define LABEL_END  0xffff

#endif
