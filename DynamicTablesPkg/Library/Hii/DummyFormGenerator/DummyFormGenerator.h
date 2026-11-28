/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <HiiFormsGenerator.h>
#include <Library/HiiFormsLib.h>
#include <Library/HiiStringsLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>

/** Dynamic HII dummy formset GUID.
*/
#define DYNAMIC_HII_DUMMY_FORM_GUID \
  { \
    0xBA7DA9F0, 0x559A, 0x4B64, \
    { 0x94, 0x65, 0xC8, 0x32, 0x70, 0x0D, 0x68, 0x12 } \
  }

/** Dynamic HII dummy form generator signature.
*/
#define DYN_HII_DUMMY_FORM_GEN_SIGNATURE \
          SIGNATURE_64 ('D', 'U', 'M', 'M', 'Y', 'H', 'I', 'I')

/** Varstore ID used by the dummy form generator.
*/
#define DYN_HII_DUMMY_VARSTORE_ID  0x1000

/** Variable name used by the dummy varstore.
*/
#define DYN_HII_DUMMY_VARIABLE_NAME  L"DummyHiiConfig"

/** Question IDs used by the dummy form generator.
*/
#define DYN_HII_DUMMY_ENABLE_QID       0x1000
#define DYN_HII_DUMMY_MODE_QID         0x1001
#define DYN_HII_DUMMY_THRESHOLD_QID    0x1002
#define DYN_HII_DUMMY_ADVANCED_QID     0x1003
#define DYN_HII_DUMMY_REF_QID          0x1004
#define DYN_HII_DUMMY_FORM_ENABLE_QID  0x1005
#define DYN_HII_DUMMY_ADV_LIMIT_QID    0x1006
#define DYN_HII_DUMMY_DIAG_MODE_QID    0x1007

#pragma pack(1)

/** Dummy varstore backing structure.
*/
typedef struct {
  UINT8     EnableFeature;
  UINT8     Mode;
  UINT16    Threshold;
  UINT8     AdvancedEnable;
  UINT8     FormEnable;
  UINT8     AdvancedLimit;
  UINT8     DiagnosticMode;
  UINT8     Reserved;
} DYN_HII_DUMMY_CONFIG;

/** HII vendor device path structure.
*/
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

/** Structure for holding private data for the dummy form generator.
*/
typedef struct {
  /// Form generator signature
  UINT64                  Signature;

  /// Pointer to the formset tree.
  DYN_HII_HANDLE          FormsetHandle;

  /// Varstore backing data.
  DYN_HII_DUMMY_CONFIG    DummyCfg;

  /// String package information for the generator.
  DYN_HII_STR_HANDLE      StrPkg;

  /// Device handle associated with the HII package list.
  EFI_HANDLE              DevHandle;

  /// Config routing protocol instance.
  EFI_HII_CONFIG_ROUTING_PROTOCOL
                          *HiiConfigRouting;

  /// Config access protocol instance.
  EFI_HII_CONFIG_ACCESS_PROTOCOL
                          ConfigAccess;

  /// Primary form ID.
  EFI_FORM_ID             MainFormId;

  /// Advanced form ID.
  EFI_FORM_ID             AdvancedFormId;
} HII_DUMMY_FORM_GEN_PRIV;

#define DYN_HII_DUMMY_FORM_GEN_PRIVATE_FROM_THIS(a)  \
          CR (a, HII_DUMMY_FORM_GEN_PRIV, ConfigAccess, \
              DYN_HII_DUMMY_FORM_GEN_SIGNATURE)

typedef EFI_STATUS (*DUMMY_QUESTION_GEN_TABLE)(
  HII_DUMMY_FORM_GEN_PRIV  *Priv
  );
