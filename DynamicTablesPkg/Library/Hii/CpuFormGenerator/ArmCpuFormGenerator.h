/** @file

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/DevicePath.h>

/// Formset GUID
#define DYNAMIC_PLATFORM_CFG_ARM_CPU_GUID \
  { \
    0xe1ab22e8, 0xeabe, 0x4bcb, {0xb0, 0xef, 0x9d, 0x5a, 0x5c, 0xa5, 0x5d, 0x9f} \
  }

/// Varstore GUID
#define DYN_PLATFORM_CPU_VARSTORE_GUID \
  { \
    0xBC65E462, 0x138D, 0x499B, { 0x9A, 0x97, 0xAF, 0xD1, 0xA9, 0x18, 0x17, 0x9E } \
  }

/// Varstore ID for the Formset
#define DYN_HII_ARM_CPU_VARSTORE_ID  0x1234

/// Question ID start value for the Formset
#define DYN_HII_ARM_CPU_QUESTION_ID_START  0x2000

#define DYN_HII_ARM_CPU_GEN_SIGNATURE  SIGNATURE_64 ('A', 'R', 'M', 'C', 'P', 'U', 0, 0)

/** This structure is used for holding the formset specific
    private data.
**/
typedef struct HiiArmCpuGenPriv {
  /// Formset Signature
  UINT64                             Signature;

  /// Pointer to all Formset Questions
  DYN_HII_QUESTION_DATA              *HiiArmCpuFormQuestion;

  /// Question count
  UINT32                             QuestionCount;

  /// Pointer to Formset varstores
  DYN_HII_VARSTORE_BUFFER_DATA       *HiiVarstore;

  /// Varstore count
  UINT32                             VarstoreCount;

  /// Pointer to varstore backend
  UINT32                             *ArmCpuVar;

  /// Size of the varstore backend
  UINT32                             ArmCpuVarSize;

  /// Pointer to the generated Formset
  DYN_HII_FORMSET                    *Formset;

  /// Pointer to the form package holding structure
  DYN_HII_IFR_BUFFER                 *IfrBuffer;

  /// Pointer to string package holding structure
  DYN_HII_STR_PKG_INFO               *StrPkgInfo;

  /// Device handle
  EFI_HANDLE                         DevHandle;

  /// Config routing protocol instance for the form
  /// generator
  EFI_HII_CONFIG_ROUTING_PROTOCOL    *HiiConfigRouting;

  /// Config access protocol instance for the form
  /// generator
  EFI_HII_CONFIG_ACCESS_PROTOCOL     ConfigAccess;
} HII_ARM_CPU_GEN_PRIV;

#define DYN_HII_ARM_CPU_GEN_PRIVATE_FROM_THIS(a)  CR (a, HII_ARM_CPU_GEN_PRIV, ConfigAccess, DYN_HII_ARM_CPU_GEN_SIGNATURE)

#pragma pack(1)

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()
