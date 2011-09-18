/** @file
  
  The defintions are required both by Source code and Vfr file.
  The PLAT_OVER_MNGR_DATA structure, form guid and Ifr question ID are defined. 

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLAT_OVER_MNGR_H_
#define _PLAT_OVER_MNGR_H_

#include <Guid/PlatDriOverrideHii.h>

//
// The max number of the supported driver list.
//
#define MAX_CHOICE_NUM    0x00ff
#define UPDATE_DATA_SIZE  0x1000

#define FORM_ID_DEVICE                 0x1100
#define FORM_ID_DRIVER                 0x1200
#define FORM_ID_ORDER                  0x1500

#define KEY_VALUE_DEVICE_OFFSET        0x0100
#define KEY_VALUE_DEVICE_MAX           (KEY_VALUE_DEVICE_OFFSET + MAX_CHOICE_NUM)

#define KEY_VALUE_DEVICE_REFRESH       0x1234
#define KEY_VALUE_DEVICE_FILTER        0x1235
#define KEY_VALUE_DEVICE_CLEAR         0x1236

#define KEY_VALUE_DRIVER_GOTO_PREVIOUS 0x1300
#define KEY_VALUE_DRIVER_GOTO_ORDER    0x1301

#define KEY_VALUE_ORDER_GOTO_PREVIOUS  0x2000
#define KEY_VALUE_ORDER_SAVE_AND_EXIT  0x1800

#define VARSTORE_ID_PLAT_OVER_MNGR     0x1000

#define LABEL_END                      0xffff

typedef struct {
  UINT8   DriSelection[MAX_CHOICE_NUM];
  UINT8   DriOrder[MAX_CHOICE_NUM];
  UINT8   PciDeviceFilter;
} PLAT_OVER_MNGR_DATA;

//
// Field offset of structure PLAT_OVER_MNGR_DATA
//
#define VAR_OFFSET(Field)              ((UINTN) &(((PLAT_OVER_MNGR_DATA *) 0)->Field))
#define DRIVER_SELECTION_VAR_OFFSET     (VAR_OFFSET (DriSelection))
#define DRIVER_ORDER_VAR_OFFSET         (VAR_OFFSET (DriOrder))

//
// Tool automatic generated Question Id start from 1
// In order to avoid to conflict them, the Driver Selection and Order QuestionID offset is defined from 0x0500.
//
#define QUESTION_ID_OFFSET              0x0500
#define DRIVER_SELECTION_QUESTION_ID    (VAR_OFFSET (DriSelection) + QUESTION_ID_OFFSET)
#define DRIVER_ORDER_QUESTION_ID        (VAR_OFFSET (DriOrder) + QUESTION_ID_OFFSET)

#endif
