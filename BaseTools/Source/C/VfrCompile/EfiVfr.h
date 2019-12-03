/** @file
Defines and prototypes for the UEFI VFR compiler internal use.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFIVFR_H_
#define _EFIVFR_H_

#include "Common/UefiBaseTypes.h"
#include "Common/UefiInternalFormRepresentation.h"
#include "Common/MdeModuleHii.h"

#define MAX_VFR_LINE_LEN         4096

#define EFI_IFR_MAX_LENGTH       0xFF
#define MAX_IFR_EXPRESSION_DEPTH 0x9

#define EFI_VARSTORE_ID_INVALID  0
#define EFI_VAROFFSET_INVALID    0xFFFF
#define EFI_VARSTORE_ID_START    0x20
#define EFI_STRING_ID_INVALID    0x0
#define EFI_IMAGE_ID_INVALID     0xFFFF

#define EFI_IFR_MAX_DEFAULT_TYPE 0x10

typedef enum {
  QUESTION_NORMAL,
  QUESTION_DATE,
  QUESTION_TIME,
  QUESTION_REF,
} EFI_QUESION_TYPE;

typedef enum {
  EQUAL,
  LESS_EQUAL,
  LESS_THAN,
  GREATER_THAN,
  GREATER_EQUAL
} EFI_COMPARE_TYPE;

#endif
