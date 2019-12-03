/** @file
  This driver effectuates OVMF's platform configuration settings and exposes
  them via HII.

  Copyright (C) 2014, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

//
// Macro and type definitions that connect the form with the HII driver code.
//
#define FORMSTATEID_MAIN_FORM  1
#define FORMID_MAIN_FORM       1

#define QUESTION_RES_CUR       1
#define MAXSIZE_RES_CUR       16

#define LABEL_RES_NEXT         1
#define QUESTION_RES_NEXT      2

#define QUESTION_SAVE_EXIT     3
#define QUESTION_DISCARD_EXIT  4

//
// This structure describes the form state. Its fields relate strictly to the
// visual widgets on the form.
//
typedef struct {
  UINT16 CurrentPreferredResolution[MAXSIZE_RES_CUR];
  UINT32 NextPreferredResolution;
} MAIN_FORM_STATE;

#endif // _PLATFORM_H_
