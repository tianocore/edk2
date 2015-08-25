/** @file
  Include file for platform variable cleanup HII.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLAT_VAR_CLEANUP_HII_
#define _PLAT_VAR_CLEANUP_HII_

//
// {24F14D8A-D7A8-4991-91E0-96C3B7DB8456}
//
#define VARIABLE_CLEANUP_HII_GUID \
  { \
    0x24f14d8a, 0xd7a8, 0x4991, { 0x91, 0xe0, 0x96, 0xc3, 0xb7, 0xdb, 0x84, 0x56 } \
  }

#define MAX_USER_VARIABLE_COUNT         0x1000

typedef struct {
  UINT8   SelectAll;
  //
  // FALSE is to not delete, TRUE is to delete.
  //
  UINT8   UserVariable[MAX_USER_VARIABLE_COUNT];
} VARIABLE_CLEANUP_DATA;

#define VARIABLE_CLEANUP_VARSTORE_ID    0x8000

//
// Field offset of structure VARIABLE_CLEANUP_DATA
//
#define VAR_OFFSET(Field)               ((UINTN) &(((VARIABLE_CLEANUP_DATA *) 0)->Field))
#define USER_VARIABLE_VAR_OFFSET        (VAR_OFFSET (UserVariable))

#define FORM_ID_VARIABLE_CLEANUP        0x8000

#define LABEL_START                     0x0000
#define LABEL_END                       0xFFFF

#define SELECT_ALL_QUESTION_ID          0x7FFD
#define SAVE_AND_EXIT_QUESTION_ID       0x7FFE
#define NO_SAVE_AND_EXIT_QUESTION_ID    0x7FFF

//
// Tool automatic generated Question Id start from 1.
// In order to avoid to conflict them, the user variable QuestionID offset is defined from 0x8000.
//
#define USER_VARIABLE_QUESTION_ID       0x8000

#endif
