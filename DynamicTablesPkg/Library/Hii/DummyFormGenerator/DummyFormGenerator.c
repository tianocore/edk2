/** @file
  Dynamic HII dummy form generator.

  This generator demonstrates how conditional expressions can be attached to
  forms, questions, OneOf options, defaults, and nested condition nodes.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

// Module specific include files.
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <HiiFormsGenerator.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiFormsLib.h>
#include <Library/HiiLib.h>
#include <Library/HiiStringsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "DummyFormGenerator.h"

extern UINT8  DummyFormGeneratorStrings[];

static
CONST
HII_VENDOR_DEVICE_PATH  mDummyHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DYNAMIC_HII_DUMMY_FORM_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8)(END_DEVICE_PATH_LENGTH),
      (UINT8)((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};

/** Extract the current dummy form configuration.

  @param [in]  This      Pointer to the config access protocol.
  @param [in]  Request   Configuration request string.
  @param [out] Progress  Progress pointer for the request.
  @param [out] Results   Allocated configuration response string.

  @retval EFI_SUCCESS           The configuration was extracted.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The request does not target this varstore.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
static
EFI_STATUS
EFIAPI
DummyExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  BOOLEAN                          AllocatedRequest;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STATUS                       Status;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  HII_DUMMY_FORM_GEN_PRIV          *Priv;
  CHAR16                           *StrPointer;
  UINTN                            BufferSize;
  UINTN                            Size;
  CONST EFI_GUID                   VarGuid        = DYNAMIC_HII_DUMMY_FORM_GUID;
  CONST CHAR16                     VariableName[] =
    DYN_HII_DUMMY_VARIABLE_NAME;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Priv             = DYN_HII_DUMMY_FORM_GEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = Priv->HiiConfigRouting;
  AllocatedRequest = FALSE;
  BufferSize       = sizeof (DYN_HII_DUMMY_CONFIG);

  if (Request == NULL) {
    ConfigRequestHdr = HiiConstructConfigHdr (
                         (EFI_GUID *)&VarGuid,
                         (CHAR16 *)VariableName,
                         Priv->DevHandle
                         );
    if (ConfigRequestHdr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (
      ConfigRequest,
      Size,
      L"%s&OFFSET=0&WIDTH=%016LX",
      ConfigRequestHdr,
      (UINT64)BufferSize
      );
    FreePool (ConfigRequestHdr);
  } else {
    if (!HiiIsConfigHdrMatch (Request, (EFI_GUID *)&VarGuid, NULL)) {
      *Progress = Request;
      return EFI_NOT_FOUND;
    }

    StrPointer = StrStr (Request, L"PATH");
    if (StrPointer == NULL) {
      *Progress = Request;
      return EFI_INVALID_PARAMETER;
    }

    if (StrStr (StrPointer, L"&") == NULL) {
      Size          = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      if (ConfigRequest == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      AllocatedRequest = TRUE;
      UnicodeSPrint (
        ConfigRequest,
        Size,
        L"%s&OFFSET=0&WIDTH=%016LX",
        Request,
        (UINT64)BufferSize
        );
    } else {
      ConfigRequest = Request;
    }
  }

  if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
    if (AllocatedRequest) {
      FreePool (ConfigRequest);
    }

    *Progress = Request;
    return EFI_INVALID_PARAMETER;
  }

  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *)&Priv->DummyCfg,
                               BufferSize,
                               Results,
                               Progress
                               );

  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return EFI_SUCCESS;
}

/** Route an updated dummy form configuration to the backing variable.

  @param [in]  This           Pointer to the config access protocol.
  @param [in]  Configuration  Configuration response string.
  @param [out] Progress       Progress pointer for the response.

  @retval EFI_SUCCESS           The configuration was routed.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The response does not target this varstore.
**/
static
EFI_STATUS
EFIAPI
DummyRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STATUS                       Status;
  HII_DUMMY_FORM_GEN_PRIV          *Priv;
  UINTN                            BufferSize;
  CONST EFI_GUID                   VarGuid        = DYNAMIC_HII_DUMMY_FORM_GUID;
  CONST CHAR16                     VariableName[] =
    DYN_HII_DUMMY_VARIABLE_NAME;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Priv             = DYN_HII_DUMMY_FORM_GEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = Priv->HiiConfigRouting;

  if (!HiiIsConfigHdrMatch (Configuration, (EFI_GUID *)&VarGuid, NULL)) {
    *Progress = Configuration;
    return EFI_NOT_FOUND;
  }

  BufferSize = sizeof (DYN_HII_DUMMY_CONFIG);
  Status     = gRT->GetVariable (
                      (CHAR16 *)VariableName,
                      (EFI_GUID *)&VarGuid,
                      NULL,
                      &BufferSize,
                      &Priv->DummyCfg
                      );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    return Status;
  }

  BufferSize = sizeof (DYN_HII_DUMMY_CONFIG);
  Status     = HiiConfigRouting->ConfigToBlock (
                                   HiiConfigRouting,
                                   Configuration,
                                   (UINT8 *)&Priv->DummyCfg,
                                   &BufferSize,
                                   Progress
                                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return gRT->SetVariable (
                (CHAR16 *)VariableName,
                (EFI_GUID *)&VarGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                EFI_VARIABLE_RUNTIME_ACCESS,
                sizeof (DYN_HII_DUMMY_CONFIG),
                &Priv->DummyCfg
                );
}

/** Handle interactive browser callbacks for the dummy form.

  @param [in]  This           Pointer to the config access protocol.
  @param [in]  Action         Browser action.
  @param [in]  QuestionId     Question ID.
  @param [in]  Type           Question value type.
  @param [in]  Value          Question value.
  @param [out] ActionRequest  Requested browser action.

  @retval EFI_UNSUPPORTED  No interactive callbacks are required.
**/
static
EFI_STATUS
EFIAPI
DummyDriverCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  EFI_BROWSER_ACTION                    Action,
  IN  EFI_QUESTION_ID                       QuestionId,
  IN  UINT8                                 Type,
  IN  EFI_IFR_TYPE_VALUE                    *Value,
  OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  )
{
  return EFI_UNSUPPORTED;
}

/** Add a BOOLEAN default value under the supplied parent.

  @param [in] ParentHandle  Question or condition handle under which default is
                            to be added.
  @param [in] Value         BOOLEAN default value.

  @retval EFI_SUCCESS           The default was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddBooleanDefault (
  IN DYN_HII_HANDLE  ParentHandle,
  IN BOOLEAN         Value
  )
{
  EFI_IFR_TYPE_VALUE  IfrValue;

  ZeroMem (&IfrValue, sizeof (IfrValue));
  IfrValue.b = Value;

  return DynHiiAddDefault (
           ParentHandle,
           0,
           EFI_IFR_TYPE_BOOLEAN,
           IfrValue
           );
}

/** Add a UINT8 default value under the supplied parent.

  @param [in] ParentHandle  Question or condition handle under which default is
                            to be added.
  @param [in] Value         UINT8 default value.

  @retval EFI_SUCCESS           The default was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddUint8Default (
  IN DYN_HII_HANDLE  ParentHandle,
  IN UINT8           Value
  )
{
  EFI_IFR_TYPE_VALUE  IfrValue;

  ZeroMem (&IfrValue, sizeof (IfrValue));
  IfrValue.u8 = Value;

  return DynHiiAddDefault (
           ParentHandle,
           0,
           EFI_IFR_TYPE_NUM_SIZE_8,
           IfrValue
           );
}

/** Add a UINT16 default value under the supplied parent.

  @param [in] ParentHandle  Question or condition handle under which default is
                            to be added.
  @param [in] Value         UINT16 default value.

  @retval EFI_SUCCESS           The default was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddUint16Default (
  IN DYN_HII_HANDLE  ParentHandle,
  IN UINT16          Value
  )
{
  EFI_IFR_TYPE_VALUE  IfrValue;

  ZeroMem (&IfrValue, sizeof (IfrValue));
  IfrValue.u16 = Value;

  return DynHiiAddDefault (
           ParentHandle,
           0,
           EFI_IFR_TYPE_NUM_SIZE_16,
           IfrValue
           );
}

/** Add questions to the advanced demo form.

  @param [in] FormHandle  Advanced form handle.

  @retval EFI_SUCCESS           The questions were added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddAdvancedFormQuestions (
  IN DYN_HII_HANDLE  FormHandle
  )
{
  EFI_STATUS          Status;
  DYN_HII_HANDLE      QuestionHandle;
  DYN_HII_HANDLE      SuppressQuestion;
  EFI_IFR_TYPE_VALUE  Value;

  Status = DynHiiAddSubtitle (
             FormHandle,
             STRING_TOKEN (STR_DUMMY_ADV_FORM_SUBTITLE),
             STRING_TOKEN (STR_DUMMY_ADV_FORM_SUBTITLE),
             0,
             NULL
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddNumeric (
             FormHandle,
             DYN_HII_DUMMY_ADV_LIMIT_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, AdvancedLimit),
             STRING_TOKEN (STR_DUMMY_ADV_LIMIT_PROMPT),
             STRING_TOKEN (STR_DUMMY_ADV_LIMIT_HELP),
             0,
             EFI_IFR_NUMERIC_SIZE_1,
             0,
             10,
             1,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AddUint8Default (QuestionHandle, 4);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //  VFR syntax:
  //    suppressif ideqval AdvancedSwitch == 0;
  //      oneof DiagnosticMode;
  //        option text = "Off", value = 0;
  //        option text = "Summary", value = 1, flags = DEFAULT;
  //        option text = "Verbose", value = 2;
  //      endoneof;
  //    endif;
  Status = DynHiiAddSuppressIf (FormHandle, &SuppressQuestion);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualValExpr (
             SuppressQuestion,
             DYN_HII_DUMMY_ADVANCED_QID,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddOneOf (
             SuppressQuestion,
             DYN_HII_DUMMY_DIAG_MODE_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, DiagnosticMode),
             STRING_TOKEN (STR_DUMMY_DIAG_MODE_PROMPT),
             STRING_TOKEN (STR_DUMMY_DIAG_MODE_HELP),
             0,
             EFI_IFR_NUMERIC_SIZE_1,
             EFI_IFR_TYPE_NUM_SIZE_8,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&Value, sizeof (Value));

  Value.u8 = 0;
  Status   = DynHiiAddOption (
               QuestionHandle,
               STRING_TOKEN (STR_DUMMY_DIAG_OFF),
               0,
               EFI_IFR_TYPE_NUM_SIZE_8,
               Value
               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value.u8 = 1;
  Status   = DynHiiAddOption (
               QuestionHandle,
               STRING_TOKEN (STR_DUMMY_DIAG_SUMMARY),
               EFI_IFR_OPTION_DEFAULT,
               EFI_IFR_TYPE_NUM_SIZE_8,
               Value
               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value.u8 = 2;
  return DynHiiAddOption (
           QuestionHandle,
           STRING_TOKEN (STR_DUMMY_DIAG_VERBOSE),
           0,
           EFI_IFR_TYPE_NUM_SIZE_8,
           Value
           );
}

/** Add the primary demo form.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The form was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddMainForm (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  return DynHiiAddForm (
           Priv->FormsetHandle,
           STRING_TOKEN (STR_DUMMY_MAIN_FORM_TITLE),
           &Priv->MainFormId
           );
}

/** Add the advanced demo form.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The form was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddAdvancedForm (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  DYN_HII_HANDLE  FormHandle;

  Status = DynHiiAddForm (
             Priv->FormsetHandle,
             STRING_TOKEN (STR_DUMMY_ADVANCED_FORM_TITLE),
             &Priv->AdvancedFormId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FormHandle = DynHiiGetForm (Priv->FormsetHandle, Priv->AdvancedFormId);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  return AddAdvancedFormQuestions (FormHandle);
}

/** Add the formset node.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The formset was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
SetupFormset (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  CONST EFI_GUID  FormsetGuid = DYNAMIC_HII_DUMMY_FORM_GUID;

  return DynHiiCreateFormSet (
           &FormsetGuid,
           NULL,
           STRING_TOKEN (STR_DUMMY_FORM_SET_TITLE),
           STRING_TOKEN (STR_DUMMY_FORM_SET_HELP),
           0,
           &Priv->FormsetHandle
           );
}

/** Add forms to the formset.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The forms were added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
SetupFormsetForms (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS  Status;

  Status = AddMainForm (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddAdvancedForm (Priv);
}

/** Add a varstore object to the formset.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The varstore was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
SetupFormsetVarstores (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  CONST CHAR8     VarstoreName[] = "DummyHiiConfig";
  CONST EFI_GUID  VarstoreGuid   = DYNAMIC_HII_DUMMY_FORM_GUID;

  return DynHiiAddVarstoreBuffer (
           Priv->FormsetHandle,
           &VarstoreGuid,
           DYN_HII_DUMMY_VARSTORE_ID,
           sizeof (DYN_HII_DUMMY_CONFIG),
           VarstoreName
           );
}

/** Get the main form handle.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval The main form handle, or NULL if the form does not exist.
**/
static
DYN_HII_HANDLE
GetMainFormHandle (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  return DynHiiGetForm (Priv->FormsetHandle, Priv->MainFormId);
}

/** Add the main form subtitle.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The subtitle was added successfully.
  @retval EFI_NOT_FOUND         The main form was not found.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddMainFormSubtitle (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  DYN_HII_HANDLE  FormHandle;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  return DynHiiAddSubtitle (
           FormHandle,
           STRING_TOKEN (STR_DUMMY_SUBTITLE),
           STRING_TOKEN (STR_DUMMY_SUBTITLE),
           0,
           NULL
           );
}

/** Add a checkbox used as the master condition input.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddEnableFeatureQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  DYN_HII_HANDLE  FormHandle;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  return DynHiiAddCheckbox (
           FormHandle,
           DYN_HII_DUMMY_ENABLE_QID,
           DYN_HII_DUMMY_VARSTORE_ID,
           OFFSET_OF (DYN_HII_DUMMY_CONFIG, EnableFeature),
           STRING_TOKEN (STR_DUMMY_ENABLE_PROMPT),
           STRING_TOKEN (STR_DUMMY_ENABLE_HELP),
           0,
           0,
           NULL
           );
}

/** Add a OneOf question with conditionally emitted options.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddModeQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS          Status;
  DYN_HII_HANDLE      FormHandle;
  DYN_HII_HANDLE      OptionParent;
  DYN_HII_HANDLE      QuestionHandle;
  DYN_HII_HANDLE      SuppressOption;
  EFI_IFR_TYPE_VALUE  Value;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = DynHiiAddOneOf (
             FormHandle,
             DYN_HII_DUMMY_MODE_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, Mode),
             STRING_TOKEN (STR_DUMMY_MODE_PROMPT),
             STRING_TOKEN (STR_DUMMY_MODE_HELP),
             0,
             EFI_IFR_NUMERIC_SIZE_1,
             EFI_IFR_TYPE_NUM_SIZE_8,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem (&Value, sizeof (Value));

  Value.u8 = 0;
  Status   = DynHiiAddOption (
               QuestionHandle,
               STRING_TOKEN (STR_DUMMY_MODE_DISABLED),
               0,
               EFI_IFR_TYPE_NUM_SIZE_8,
               Value
               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Value.u8 = 1;
  Status   = DynHiiAddOption (
               QuestionHandle,
               STRING_TOKEN (STR_DUMMY_MODE_BASIC),
               EFI_IFR_OPTION_DEFAULT,
               EFI_IFR_TYPE_NUM_SIZE_8,
               Value
               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //  VFR syntax:
  //    suppressif ideqval EnableFeature == 0;
  //      option text = "Advanced", value = 2;
  //    endif;
  //
  //  This shows an Option node under a condition whose parent is a OneOf.
  Status = DynHiiAddSuppressIf (QuestionHandle, &SuppressOption);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualValExpr (
             SuppressOption,
             DYN_HII_DUMMY_ENABLE_QID,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OptionParent = SuppressOption;
  Value.u8     = 2;
  return DynHiiAddOption (
           OptionParent,
           STRING_TOKEN (STR_DUMMY_MODE_ADVANCED),
           0,
           EFI_IFR_TYPE_NUM_SIZE_8,
           Value
           );
}

/** Add a numeric question under a grayoutif compound expression.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddThresholdQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  DYN_HII_HANDLE  FormHandle;
  DYN_HII_HANDLE  GrayOutIf;
  DYN_HII_HANDLE  QuestionHandle;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  //  VFR syntax:
  //    grayoutif (questionref(EnableFeature) & 0x1) == 0;
  //      numeric Threshold ...;
  //    endif;
  //
  //  RPN expression sequence:
  //    questionref(EnableFeature), uint64(1), bitwise-and, uint64(0), equal
  Status = DynHiiAddGrayOutIf (FormHandle, &GrayOutIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddQuestionRef1Expr (
             GrayOutIf,
             DYN_HII_DUMMY_ENABLE_QID
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddUint64ValueExpr (GrayOutIf, 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddBitwiseAndExpr (GrayOutIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddUint64ValueExpr (GrayOutIf, 0);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualExpr (GrayOutIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddNumeric (
             GrayOutIf,
             DYN_HII_DUMMY_THRESHOLD_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, Threshold),
             STRING_TOKEN (STR_DUMMY_THRESHOLD_PROMPT),
             STRING_TOKEN (STR_DUMMY_THRESHOLD_HELP),
             0,
             EFI_IFR_NUMERIC_SIZE_2,
             0,
             100,
             5,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUint16Default (QuestionHandle, 20);
}

/** Add a checkbox with a conditional default.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddAdvancedSwitchQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  DYN_HII_HANDLE  FormHandle;
  DYN_HII_HANDLE  DefaultParent;
  DYN_HII_HANDLE  DisableIf;
  DYN_HII_HANDLE  OuterSuppressIf;
  DYN_HII_HANDLE  QuestionHandle;
  CONST UINT16    ModeValues[] = { 1, 2 };

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  //  VFR syntax:
  //    suppressif ideqvallist Mode == { 1, 2 };
  //      disableif not true;
  //        checkbox AdvancedSwitch;
  //          grayoutif not true;
  //            default value = TRUE;
  //          endif;
  //        endcheckbox;
  //      endif;
  //    endif;
  //
  //  This demonstrates nested conditions and a Default node whose parent is a
  //  condition rather than the question directly.
  Status = DynHiiAddSuppressIf (FormHandle, &OuterSuppressIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualValListExpr (
             OuterSuppressIf,
             DYN_HII_DUMMY_MODE_QID,
             ARRAY_SIZE (ModeValues),
             (UINT16 *)ModeValues
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddDisableIf (OuterSuppressIf, &DisableIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddTrueExpr (DisableIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddNotExpr (DisableIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddCheckbox (
             DisableIf,
             DYN_HII_DUMMY_ADVANCED_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, AdvancedEnable),
             STRING_TOKEN (STR_DUMMY_ADVANCED_PROMPT),
             STRING_TOKEN (STR_DUMMY_ADVANCED_HELP),
             0,
             0,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddGrayOutIf (QuestionHandle, &DefaultParent);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddTrueExpr (DefaultParent);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddNotExpr (DefaultParent);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddBooleanDefault (DefaultParent, 1);
}

/** Add a checkbox controlling whether a whole form is present.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddFormEnableQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  DYN_HII_HANDLE  FormHandle;
  DYN_HII_HANDLE  QuestionHandle;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  Status = DynHiiAddCheckbox (
             FormHandle,
             DYN_HII_DUMMY_FORM_ENABLE_QID,
             DYN_HII_DUMMY_VARSTORE_ID,
             OFFSET_OF (DYN_HII_DUMMY_CONFIG, FormEnable),
             STRING_TOKEN (STR_DUMMY_FORM_ENABLE_PROMPT),
             STRING_TOKEN (STR_DUMMY_FORM_ENABLE_HELP),
             0,
             EFI_IFR_CHECKBOX_DEFAULT,
             &QuestionHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddBooleanDefault (QuestionHandle, 1);
}

/** Add a reference question under a suppressif expression.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The question was added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
AddAdvancedFormRefQuestion (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  DYN_HII_HANDLE  FormHandle;
  DYN_HII_HANDLE  SuppressIf;

  FormHandle = GetMainFormHandle (Priv);
  if (FormHandle == NULL) {
    return EFI_NOT_FOUND;
  }

  //  VFR syntax:
  //    suppressif ideqval ShowAdvancedForm == 0 OR ideqval Mode == 0;
  //      goto AdvancedForm;
  //    endif;
  Status = DynHiiAddSuppressIf (FormHandle, &SuppressIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualValExpr (
             SuppressIf,
             DYN_HII_DUMMY_FORM_ENABLE_QID,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddEqualValExpr (
             SuppressIf,
             DYN_HII_DUMMY_MODE_QID,
             0
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiAddOrExpr (SuppressIf);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiAddRef (
           SuppressIf,
           DYN_HII_DUMMY_REF_QID,
           STRING_TOKEN (STR_DUMMY_REF_PROMPT),
           STRING_TOKEN (STR_DUMMY_REF_HELP),
           0,
           Priv->AdvancedFormId,
           NULL
           );
}

/** Question generation functions for the main form
*/
static DUMMY_QUESTION_GEN_TABLE  QuestionGen[] = {
  AddMainFormSubtitle,
  AddEnableFeatureQuestion,
  AddModeQuestion,
  AddThresholdQuestion,
  AddAdvancedSwitchQuestion,
  AddFormEnableQuestion,
  AddAdvancedFormRefQuestion
};

/** Add all question objects to the form.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           The questions were added successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
PopulateQuestionObjs (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  UINT32      Idx;
  UINT32      QuestionCount;
  EFI_STATUS  Status;

  QuestionCount = ARRAY_SIZE (QuestionGen);
  for (Idx = 0; Idx < QuestionCount; Idx++) {
    Status = QuestionGen[Idx](Priv);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Install the config access protocol for the dummy form.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           Config access protocol was installed.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
SetupHiiConfigAccess (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  EFI_STATUS  Status;

  if (Priv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&Priv->HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Priv->ConfigAccess.ExtractConfig = DummyExtractConfig;
  Priv->ConfigAccess.RouteConfig   = DummyRouteConfig;
  Priv->ConfigAccess.Callback      = DummyDriverCallback;

  return gBS->InstallMultipleProtocolInterfaces (
                &Priv->DevHandle,
                &gEfiDevicePathProtocolGuid,
                &mDummyHiiVendorDevicePath,
                &gEfiHiiConfigAccessProtocolGuid,
                &Priv->ConfigAccess,
                NULL
                );
}

/** Initialize the EFI variable backend for the dummy form.

  @param [in] Priv  Pointer to the private structure of the form.

  @retval EFI_SUCCESS           Variable backend was initialized.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
SetupEfiVarBackend (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  BOOLEAN         SetDefaults;
  EFI_STATUS      Status;
  EFI_STRING      ConfigRequestHdr;
  UINTN           BufferSize;
  CONST EFI_GUID  VarGuid        = DYNAMIC_HII_DUMMY_FORM_GUID;
  CONST CHAR16    VariableName[] = DYN_HII_DUMMY_VARIABLE_NAME;

  if (Priv == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  BufferSize = sizeof (DYN_HII_DUMMY_CONFIG);
  Status     = gRT->GetVariable (
                      (CHAR16 *)VariableName,
                      (EFI_GUID *)&VarGuid,
                      NULL,
                      &BufferSize,
                      &Priv->DummyCfg
                      );
  if (!EFI_ERROR (Status)) {
    return EFI_SUCCESS;
  }

  if (Status != EFI_NOT_FOUND) {
    return Status;
  }

  ConfigRequestHdr = HiiConstructConfigHdr (
                       (EFI_GUID *)&VarGuid,
                       (CHAR16 *)VariableName,
                       Priv->DevHandle
                       );
  if (ConfigRequestHdr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SetDefaults = HiiSetToDefaults (
                  ConfigRequestHdr,
                  EFI_HII_DEFAULT_CLASS_STANDARD
                  );
  FreePool (ConfigRequestHdr);

  if (SetDefaults) {
    return EFI_SUCCESS;
  }

  return gRT->SetVariable (
                (CHAR16 *)VariableName,
                (EFI_GUID *)&VarGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS |
                EFI_VARIABLE_RUNTIME_ACCESS,
                sizeof (DYN_HII_DUMMY_CONFIG),
                &Priv->DummyCfg
                );
}

/** Initialize the string package info structure.

  @param [out] StrPkg  Pointer to the string package info structure.

  @retval EFI_SUCCESS           The structure was initialized successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
InitStringPkgInfo (
  OUT DYN_HII_STR_HANDLE  *StrPkg
  )
{
  return DynHiiInitStringPkg (DummyFormGeneratorStrings, StrPkg);
}

/** Allocate and initialize private data.

  @param [out] DummyPriv  Pointer to the private data structure.

  @retval EFI_SUCCESS          Private data was initialized successfully.
  @retval EFI_OUT_OF_RESOURCES Unable to allocate memory.
**/
static
EFI_STATUS
InitFormPrivateData (
  OUT HII_DUMMY_FORM_GEN_PRIV  **DummyPriv
  )
{
  EFI_STATUS               Status;
  HII_DUMMY_FORM_GEN_PRIV  *Priv;

  Priv = AllocateZeroPool (sizeof (*Priv));
  if (Priv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->Signature               = DYN_HII_DUMMY_FORM_GEN_SIGNATURE;
  Priv->DummyCfg.Mode           = 1;
  Priv->DummyCfg.Threshold      = 20;
  Priv->DummyCfg.AdvancedEnable = 1;
  Priv->DummyCfg.FormEnable     = 1;
  Priv->DummyCfg.AdvancedLimit  = 4;
  Priv->DummyCfg.DiagnosticMode = 1;

  Status = InitStringPkgInfo (&Priv->StrPkg);
  if (EFI_ERROR (Status)) {
    FreePool (Priv);
    return Status;
  }

  *DummyPriv = Priv;

  return EFI_SUCCESS;
}

/** Build the dynamic HII dummy form.

  @param [in]  This           Pointer to the HII form generator.
  @param [in]  CmHiiFormInfo  Pointer to the HII form information.
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager protocol.
  @param [in]  HiiHandleList  Optional list of HII handles.
  @param [out] HiiHandle      HII handle of this form.
  @param [out] GenPriv        Private structure of the form.

  @retval EFI_SUCCESS           Form generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES  Unable to allocate memory.
**/
static
EFI_STATUS
EFIAPI
BuildHiiDummyForm (
  IN  CONST HII_FORM_GENERATOR                            *This,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO              *CONST  CmHiiFormInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                    *HiiHandleList OPTIONAL,
  OUT       EFI_HII_HANDLE                                *HiiHandle,
  OUT       VOID                                          **GenPriv
  )
{
  UINT8                    *IfrBuf;
  UINT8                    *StrPkgBuf;
  EFI_STATUS               Status;
  HII_DUMMY_FORM_GEN_PRIV  *Priv;
  CONST EFI_GUID           FormsetGuid = DYNAMIC_HII_DUMMY_FORM_GUID;

  if ((This == NULL) || (HiiHandle == NULL) || (GenPriv == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *HiiHandle = NULL;
  *GenPriv   = NULL;
  IfrBuf     = NULL;
  StrPkgBuf  = NULL;

  Status = InitFormPrivateData (&Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  *GenPriv = Priv;

  Status = SetupFormset (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetupFormsetVarstores (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetupFormsetForms (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopulateQuestionObjs (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetupHiiConfigAccess (Priv);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiGenerateFormPackage (Priv->FormsetHandle, &IfrBuf);
  if (EFI_ERROR (Status)) {
    goto out;
  }

  Status = DynHiiGenerateStringPackage (Priv->StrPkg, &StrPkgBuf);
  if (EFI_ERROR (Status)) {
    goto out;
  }

  *HiiHandle = HiiAddPackages (
                 &FormsetGuid,
                 Priv->DevHandle,
                 IfrBuf,
                 StrPkgBuf,
                 NULL
                 );
  if (*HiiHandle == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto out;
  }

  Status = SetupEfiVarBackend (Priv);

out:
  if (IfrBuf != NULL) {
    FreePool (IfrBuf);
  }

  if (StrPkgBuf != NULL) {
    FreePool (StrPkgBuf);
  }

  return Status;
}

/** Free private data.

  @param [in] Priv  Pointer to the private data structure.
**/
static
VOID
DeInitPrivateData (
  IN HII_DUMMY_FORM_GEN_PRIV  *Priv
  )
{
  if (Priv->FormsetHandle != NULL) {
    DynHiiFreeFormSet (Priv->FormsetHandle);
  }

  if (Priv->StrPkg != NULL) {
    DynHiiFreeStringPackage (Priv->StrPkg);
  }

  FreePool (Priv);
}

/** Free all resources associated with this form.

  @param [in] This           Pointer to the HII form generator.
  @param [in] CmHiiFormInfo  Pointer to the HII form information.
  @param [in] HiiHandle      HII handle of this form.
  @param [in] GenPriv        Private structure for the form.

  @retval EFI_SUCCESS           Resources were freed successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
static
EFI_STATUS
EFIAPI
FreeHiiDummyForm (
  IN  CONST HII_FORM_GENERATOR                    *This,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO      *CONST  CmHiiFormInfo OPTIONAL,
  IN        EFI_HII_HANDLE                        *HiiHandle,
  IN        VOID                                  *GenPriv
  )
{
  HII_DUMMY_FORM_GEN_PRIV  *Priv;

  if ((This == NULL) || (CmHiiFormInfo == NULL) || (HiiHandle == NULL) ||
      (GenPriv == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (This->GeneratorID != CmHiiFormInfo->FormGeneratorId) {
    return EFI_INVALID_PARAMETER;
  }

  Priv = GenPriv;

  if (*HiiHandle != NULL) {
    HiiRemovePackages (*HiiHandle);
  }

  if (Priv->DevHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Priv->DevHandle,
           &gEfiDevicePathProtocolGuid,
           &mDummyHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &Priv->ConfigAccess,
           NULL
           );
  }

  DeInitPrivateData (Priv);

  return EFI_SUCCESS;
}

/** The interface for the dummy form generator.
*/
static HII_FORM_GENERATOR  HiiDummyFormGenerator = {
  CREATE_HII_FORM_GEN_ID (EStdHiiFormsIdDummy),
  L"HII.STD.DUMMY.CONDITION.FORM.GENERATOR",
  BuildHiiDummyForm,
  FreeHiiDummyForm
};

/** Register the generator with the HII form factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The generator is already registered.
**/
EFI_STATUS
EFIAPI
HiiDummyFormConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterHiiFormGenerator (&HiiDummyFormGenerator);
  DEBUG ((DEBUG_INFO, "HII-Dummy: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Deregister the generator from the HII form factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The generator is not registered.
**/
EFI_STATUS
EFIAPI
HiiDummyFormDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterHiiFormGenerator (&HiiDummyFormGenerator);
  DEBUG ((
    DEBUG_INFO,
    "HII-Dummy: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);

  return Status;
}
