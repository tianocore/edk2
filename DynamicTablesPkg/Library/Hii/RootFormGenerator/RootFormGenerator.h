/** @file

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

/** Dynamic HII root formset GUID.
*/
#define DYNAMIC_HII_ROOT_FORM_GUID \
  { \
    0x1806F55C, 0x0DC5, 0x468B, { 0x80, 0x45, 0xD2, 0x91, 0x6E, 0xF2, 0xF4, 0x55 } \
  }

/** Dynamic HII root form generator signature.
*/
#define DYN_HII_ROOT_FORM_GEN_SIGNATURE  SIGNATURE_64 ('R', 'O', 'O', 'T', 'F', 'O', 'R', 'M')

/** Question ID start value for the formset
*/
#define DYN_HII_ROOT_FORM_QUESTION_ID_START  0x1000

/** Structure for holding private data for the Form generator.
*/
typedef struct HiiPlatCfgGenPriv {
  /// Form generator signature
  UINT64                Signature;

  /// Pointer to the Form's formset
  DYN_HII_HANDLE        FormsetHandle;

  /// Handle to the IFR buffer structure
  DYN_HII_HANDLE        IfrBufHandle;

  /// Form package buffer pointer
  UINT8                 *IfrBuffer;

  /// String package information for the
  /// generator
  DYN_HII_STR_HANDLE    StrPkg;

  /// String package data buffer pointer
  UINT8                 *StrPkgBuf;

  /// List of HII Handles that are to be added under
  /// this Form
  LIST_ENTRY            *HiiHandleList;
} HII_ROOT_FORM_GEN_PRIV;
