/** @file

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

/** Dynamic Platform Config Formset GUID.
*/
#define DYNAMIC_PLATFORM_CFG_GUID \
  { \
    0x1806F55C, 0x0DC5, 0x468B, { 0x80, 0x45, 0xD2, 0x91, 0x6E, 0xF2, 0xF4, 0x55 } \
  }

/** Dynamic Platform Config Form Generator signature.
*/
#define DYN_HII_PLAT_CFG_GEN_SIGNATURE  SIGNATURE_64 ('P', 'L', 'A', 'T', 'C', 'F', 'G', 0)

/** Question ID start value for the Formset
*/
#define DYN_HII_PLAT_CFG_QUESTION_ID_START  0x1000

/** Structure for holding private data for the Form generator.
*/
typedef struct HiiPlatCfgGenPriv {
  /// Form generator signature
  UINT64                  Signature;

  /// Pointer to the Form's Formset
  DYN_HII_FORMSET         *Formset;

  /// Form Package buffer pointer
  DYN_HII_IFR_BUFFER      *IfrBuffer;

  /// String package info pointer
  DYN_HII_STR_PKG_INFO    *StrPkgInfo;

  /// List of HII Handles that are to be added under
  /// this Form
  LIST_ENTRY              *HiiHandleList;
} HII_PLAT_CFG_GEN_PRIV;
