/** @file
  Dynamic Hii Cpu Form Generator for Arm platforms.

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
#include <HiiFormGeneratorLib.h>
#include <HiiStringGeneratorLib.h>
#include <Guid/MdeModuleHii.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiFormsLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/TableHelperLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#include "ArmCpuFormGenerator.h"

static EFI_QUESTION_ID  mQuestionId = DYN_HII_ARM_CPU_QUESTION_ID_START;
static EFI_STRING_ID    mStringId   = 1;
static EFI_FORM_ID      mFormId     = 1;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath0 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DYNAMIC_PLATFORM_CFG_ARM_CPU_GUID
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

/** This macro expands to a function that retrieves the GIC
    CPU interface Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGicCInfo,
  CM_ARM_GICC_INFO
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.
**/
EFI_STATUS
EFIAPI
ExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Request,
  OUT EFI_STRING                            *Progress,
  OUT EFI_STRING                            *Results
  )
{
  HII_ARM_CPU_GEN_PRIV             *Priv;
  CHAR16                           *StrPointer;
  UINTN                            Size;
  BOOLEAN                          AllocatedRequest;
  EFI_STRING                       ConfigRequest;
  UINTN                            BufferSize;
  EFI_STATUS                       Status;
  EFI_STRING                       ConfigRequestHdr;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  CONST CHAR16                     VariableName[]       = L"ArmCpuVar";
  CONST EFI_GUID                   DynPlatCfgCpuVarGuid = DYN_PLATFORM_CPU_VARSTORE_GUID;

  if ((Progress == NULL) || (Results == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Priv             = DYN_HII_ARM_CPU_GEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = Priv->HiiConfigRouting;

  AllocatedRequest = FALSE;
  BufferSize       = Priv->ArmCpuVarSize;

  if (Request == NULL) {
    // NULL Request indicates request for entire config database. Build a
    // Request string from the ConfigHdr with appended OFFSET and WIDTH.
    ConfigRequestHdr = HiiConstructConfigHdr (
                         (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                         (CHAR16 *)VariableName,
                         Priv->DevHandle
                         );
    if (ConfigRequestHdr == NULL) {
      ASSERT (0);
      return EFI_UNSUPPORTED;
    }

    Size          = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    if (ConfigRequest == NULL) {
      ASSERT (0);
      FreePool (ConfigRequestHdr);
      return EFI_OUT_OF_RESOURCES;
    }

    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  } else {
    if (!HiiIsConfigHdrMatch (Request, (EFI_GUID *)&DynPlatCfgCpuVarGuid, NULL)) {
      return EFI_NOT_FOUND;
    }

    // Check if Request specifies an element. If not then add OFFSET and WIDTH
    // for the entire config database.
    StrPointer = StrStr (Request, L"PATH");
    if (StrPointer == NULL) {
      ASSERT (StrPointer != NULL);
      return EFI_INVALID_PARAMETER;
    }

    if (StrStr (StrPointer, L"&") == NULL) {
      if (Request == NULL) {
        ASSERT (Request != NULL);
        return EFI_INVALID_PARAMETER;
      }

      Size          = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
      ConfigRequest = AllocateZeroPool (Size);
      if (ConfigRequest == NULL) {
        ASSERT (ConfigRequest != NULL);
        return EFI_OUT_OF_RESOURCES;
      }

      AllocatedRequest = TRUE;
      UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", Request, (UINT64)BufferSize);
    } else {
      ConfigRequest = Request;
    }
  }

  if (StrStr (ConfigRequest, L"OFFSET") == NULL) {
    ASSERT (StrStr (ConfigRequest, L"OFFSET") != NULL);

    if (AllocatedRequest) {
      FreePool (ConfigRequest);
    }

    return EFI_INVALID_PARAMETER;
  }

  // Use helper function BlockToConfig() to extract config data and produce
  // results string.
  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               (UINT8 *)Priv->ArmCpuVar,
                               BufferSize,
                               Results,
                               Progress
                               );

  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (EFI_ERROR (Status)) {
    ASSERT (!EFI_ERROR (Status));
    return Status;
  }

  // Update Progress to point to the terminator in the Request string. If
  // Request was NULL then Progress should also be NULL.
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return EFI_SUCCESS;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.
  @retval EFI_DEVICE_ERROR       If value is 44, return error for testing.

**/
EFI_STATUS
EFIAPI
RouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN  CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                            *Progress
  )
{
  HII_ARM_CPU_GEN_PRIV             *Priv;
  UINTN                            BufferSize;
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  CONST CHAR16                     VariableName[]       = L"ArmCpuVar";
  CONST EFI_GUID                   DynPlatCfgCpuVarGuid = DYN_PLATFORM_CPU_VARSTORE_GUID;

  if ((Configuration == NULL) || (Progress == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Priv             = DYN_HII_ARM_CPU_GEN_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = Priv->HiiConfigRouting;

  if (!HiiIsConfigHdrMatch (Configuration, (EFI_GUID *)&DynPlatCfgCpuVarGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  BufferSize = Priv->ArmCpuVarSize;
  Status     = gRT->GetVariable (
                      (CHAR16 *)VariableName,
                      (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                      NULL,
                      &BufferSize,
                      Priv->ArmCpuVar
                      );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT (Status == EFI_NOT_FOUND);
    return Status;
  }

  BufferSize = Priv->ArmCpuVarSize;
  Status     = HiiConfigRouting->ConfigToBlock (
                                   HiiConfigRouting,
                                   Configuration,
                                   (UINT8 *)Priv->ArmCpuVar,
                                   &BufferSize,
                                   Progress
                                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gRT->SetVariable (
                  (CHAR16 *)VariableName,
                  (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  Priv->ArmCpuVarSize,
                  Priv->ArmCpuVar
                  );
  ASSERT (!EFI_ERROR (Status));

  return Status;
}

/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
DriverCallback (
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

/** Add all Question objects to the Form

  Add all the Question objects to the Form.

  @param [in]  CpuCount   Number of CPU's on the platform.
  @param [in]  Priv       Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Questions were added successfully or other
                        failure codes as returned by the form generation API's.

**/
static
EFI_STATUS
PopulateQuestionObjs (
  IN      UINT32                CpuCount,
  IN      HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  UINT32                   Idx;
  CHAR16                   PromptStr[64];
  UINT32                   QuestionCount;
  UINT32                   MaxCpu;
  EFI_STATUS               Status;
  DYN_HII_QUESTION_DATA    *Question;
  DYN_HII_NUMERIC_DATA     *Numeric;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;

  Question      = Priv->HiiArmCpuFormQuestion;
  QuestionCount = Priv->QuestionCount;
  ZeroMem (PromptStr, sizeof (PromptStr));

  for (Idx = 0; Idx < QuestionCount; Idx++) {
    QuestionHdr = &Question[Idx].QuestionHdr;

    MaxCpu = (Idx * 32);

    MaxCpu = Idx < CpuCount / 32 ? MaxCpu + 31 : MaxCpu + (CpuCount % 32) - 1;
    UnicodeSPrint (
      PromptStr,
      sizeof (PromptStr),
      L"CPU [%u - %u] Enable/Disable",
      Idx * 32,
      MaxCpu
      );

    QuestionHdr->Header.Prompt = mStringId++;
    Status                     = DynHiiAddString (
                                   &Priv->StrPkgInfo->StringList,
                                   QuestionHdr->Header.Prompt,
                                   PromptStr
                                   );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    QuestionHdr->Header.Help = mStringId++;
    Status                   = DynHiiAddString (
                                 &Priv->StrPkgInfo->StringList,
                                 QuestionHdr->Header.Help,
                                 L"Enable or Disable the CPUs"
                                 );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    QuestionHdr->VarStoreId             = DYN_HII_ARM_CPU_VARSTORE_ID;
    QuestionHdr->QuestionId             = mQuestionId++;
    QuestionHdr->VarStoreInfo.VarOffset = (UINT16)(Idx * sizeof (UINT32));
    QuestionHdr->Flags                  = EFI_IFR_FLAG_RESET_REQUIRED;

    Numeric = &Question[Idx].Question.Numeric;

    Numeric->MinValue = 1;
    Numeric->MaxValue = (Idx < (CpuCount / 32)) ? MAX_UINT32 : ((1U << (CpuCount % 32)) - 1U);
    Numeric->Step     = 1;
    Numeric->Flags    = EFI_IFR_DISPLAY_UINT_HEX | EFI_IFR_NUMERIC_SIZE_4;

    Status = DynHiiAddStatement (
               DynHiiGetForm (Priv->Formset, 1),
               DynHiiQtNumeric,
               &Question[Idx]
               );
    if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Add the varstore object to the Formset.

  Add the varstore to the given formset.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the varstore was added successfully or other failure
                        codes as returned by the form generation API's.

**/
static
EFI_STATUS
SetupFormsetVarstores (
  IN HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  DYN_HII_VARSTORE_DATA         Data;
  DYN_HII_VARSTORE_BUFFER_DATA  *HiiVarstore;
  CONST CHAR8                   VarstoreName[] = "ArmCpuVar";
  CONST EFI_GUID                VarstoreGuid   = DYN_PLATFORM_CPU_VARSTORE_GUID;

  HiiVarstore       = Priv->HiiVarstore;
  HiiVarstore->Name = AllocatePool (sizeof (VarstoreName));
  if (HiiVarstore->Name == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyGuid (&HiiVarstore->Guid, &VarstoreGuid);
  HiiVarstore->VarstoreId = DYN_HII_ARM_CPU_VARSTORE_ID;
  HiiVarstore->Size       = (UINT16)(Priv->QuestionCount * sizeof (UINT32));
  AsciiStrCpyS (HiiVarstore->Name, sizeof (VarstoreName), VarstoreName);

  CopyMem (&Data.Buffer, HiiVarstore, sizeof (DYN_HII_VARSTORE_BUFFER_DATA));

  return DynHiiAddFormsetVarStore (
           Priv->Formset,
           DynHiiVarstoreBuffer,
           &Data
           );
}

/** Add the Form object to the Formset

  Add the form to the given formset.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Form was added successfully or other failure
                        codes as returned by the form generation API's.

**/
static
EFI_STATUS
SetupFormsetForms (
  IN HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  EFI_STATUS     Status;
  EFI_STRING_ID  Title;

  Title  = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Title,
             L"Enable/Disable CPU's"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiAddForm (Priv->Formset, mFormId++, Title);
}

/** Create the Formset object for the form.

  Add the Formset object for the form.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the Formset was added successfully or other failure
                        codes as returned by the form generation API's.

**/
static
EFI_STATUS
SetupFormset (
  IN HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  EFI_STATUS      Status;
  EFI_STRING_ID   Title;
  EFI_STRING_ID   Help;
  CONST EFI_GUID  FormsetGuid = DYNAMIC_PLATFORM_CFG_ARM_CPU_GUID;

  Title  = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Title,
             L"Configure CPU's on the platform"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Help   = mStringId++;
  Status = DynHiiAddString (
             &Priv->StrPkgInfo->StringList,
             Help,
             L"Configure CPU's on the platform"
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiCreateFormSet (
           &FormsetGuid,
           NULL,
           Title,
           Help,
           0,
           &Priv->Formset
           );
}

/** Initialize the string package info structure.

  Initialize the structure that contains information needed for the generation
  of the string package.

  @param [out]  StrPkgInfo      Pointer to the string package info structure.

  @retval  EFI_SUCCESS  If the structure was successfully initialized or other
                        failure codes as returned by the string generation
                        API's.

**/
static
EFI_STATUS
InitStringPkgInfo (
  OUT DYN_HII_STR_PKG_INFO  **StrPkgInfo
  )
{
  EFI_STATUS            Status;
  EFI_STRING_ID         LangStrId;
  DYN_HII_STR_PKG_INFO  *Info;

  LangStrId = mStringId++;

  Status = DynHiiInitStringPkgInfo (
             "en-US",
             LangStrId,
             StrPkgInfo
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Info = *StrPkgInfo;

  return DynHiiAddString (
           &Info->StringList,
           LangStrId,
           L"English"
           );
}

/** Initialize the form's private data structure.

  Allocate and initialize the form's private data structure.

  @param [in]  CpuCount         Number of CPU's on the platform.
  @param [out] ArmCpuPriv       Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the structure was initialized successfully or other
                        failure codes as returned by the string generation
                        API's.

**/
static
EFI_STATUS
InitFormPrivateData (
  IN      UINT32                CpuCount,
  OUT     HII_ARM_CPU_GEN_PRIV  **ArmCpuPriv
  )
{
  UINTN                 QuestionSize;
  EFI_STATUS            Status;
  HII_ARM_CPU_GEN_PRIV  *Priv;

  Priv = AllocateZeroPool (sizeof (*Priv));
  if (Priv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *ArmCpuPriv     = Priv;
  Priv->Signature = DYN_HII_ARM_CPU_GEN_SIGNATURE;

  Priv->ConfigAccess.ExtractConfig = ExtractConfig;
  Priv->ConfigAccess.RouteConfig   = RouteConfig;
  Priv->ConfigAccess.Callback      = DriverCallback;

  Priv->QuestionCount = BASE_DIV_ROUND_UP (CpuCount, 32);
  QuestionSize        = sizeof (DYN_HII_QUESTION_DATA) * Priv->QuestionCount;

  Priv->HiiArmCpuFormQuestion = AllocateZeroPool (QuestionSize);
  if (Priv->HiiArmCpuFormQuestion == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->HiiVarstore = AllocateZeroPool (sizeof (DYN_HII_VARSTORE_BUFFER_DATA));
  if (Priv->HiiVarstore == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->VarstoreCount = 1; // only a single variable store entry needed in the form

  Priv->ArmCpuVar = AllocateZeroPool (Priv->QuestionCount * sizeof (UINT32));
  if (Priv->ArmCpuVar == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Priv->ArmCpuVarSize = Priv->QuestionCount * sizeof (UINT32);

  Status = InitStringPkgInfo (&Priv->StrPkgInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/** Setup the EFI variables for storing Question data.

  Set the EFI variables needed for storing the data associated with the
  Questions on the form.

  @param [in]  Priv      Pointer to the private structure of the form.

  @retval  EFI_SUCCESS  If the variables were setup successfully or other
                        failure codes as returned by the Set/Get variable
                        API's.

**/
static
EFI_STATUS
SetupEfiVarBackend (
  IN HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  UINTN           BufferSize;
  BOOLEAN         ActionFlag;
  EFI_STATUS      Status;
  EFI_STRING      ConfigRequestHdr;
  CONST CHAR16    VariableName[]       = L"ArmCpuVar";
  CONST EFI_GUID  DynPlatCfgCpuVarGuid = DYN_PLATFORM_CPU_VARSTORE_GUID;

  //
  // Try to read NV config EFI variable first
  //
  ConfigRequestHdr = HiiConstructConfigHdr (
                       (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                       (CHAR16 *)VariableName,
                       Priv->DevHandle
                       );
  if (ConfigRequestHdr == NULL) {
    ASSERT (0);
    Status = EFI_UNSUPPORTED;
    goto err_out;
  }

  BufferSize = Priv->ArmCpuVarSize;
  Status     = gRT->GetVariable (
                      (CHAR16 *)VariableName,
                      (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                      NULL,
                      &BufferSize,
                      Priv->ArmCpuVar
                      );
  if (EFI_ERROR (Status)) {
    Status = gRT->SetVariable (
                    (CHAR16 *)VariableName,
                    (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    Priv->ArmCpuVarSize,
                    Priv->ArmCpuVar
                    );
    if (EFI_ERROR (Status)) {
      ASSERT (!EFI_ERROR (Status));
      goto err_out;
    }

    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      Status = EFI_INVALID_PARAMETER;
      goto err_out;
    }

    BufferSize = Priv->ArmCpuVarSize;
    Status     = gRT->GetVariable (
                        (CHAR16 *)VariableName,
                        (EFI_GUID *)&DynPlatCfgCpuVarGuid,
                        NULL,
                        &BufferSize,
                        Priv->ArmCpuVar
                        );
    if (EFI_ERROR (Status)) {
      goto err_out;
    }
  } else {
    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      Status = EFI_INVALID_PARAMETER;
      goto err_out;
    }
  }

  Status = EFI_SUCCESS;

err_out:
  if (ConfigRequestHdr != NULL) {
    FreePool (ConfigRequestHdr);
  }

  return Status;
}

/** Construct Hii Forms with Cpu based Questions.

  This function invokes the Configuration Manager protocol interface
  to get the required information for generating the Cpu based
  Hii Forms. It then calls the HII Form and String Generation API's
  for building the form and string package, and then adds these
  packages to the HII database through a call to HiiAddPackages().

  If this function allocates any resources then they must be freed
  in the FreeXXXXRes function.

  @param [in]  This            Pointer to the HII forms generator.
  @param [in]  HiiFormsInfo    Pointer to the HII forms information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  HiiHandleList   List of all Hii handles to which this
                               form creates references.
  @param [out] HiiHandle       Hii handle of this form.

  @retval EFI_SUCCESS            Form generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        Unsupported configuration.

**/
static
EFI_STATUS
EFIAPI
BuildHiiArmCpuForm (
  IN        HII_FORMS_GENERATOR                            *This,
  IN  CONST CM_HII_FORMS_OBJ_INFO                  *CONST  HiiFormsInfo  OPTIONAL,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                     *HiiHandleList OPTIONAL,
  OUT       EFI_HII_HANDLE                                 *HiiHandle
  )
{
  EFI_STATUS            Status;
  UINT32                CpuCount;
  CM_ARM_GICC_INFO      *GicCInfo;
  HII_ARM_CPU_GEN_PRIV  *Priv;
  CONST EFI_GUID        FormsetGuid = DYNAMIC_PLATFORM_CFG_ARM_CPU_GUID;

  *HiiHandle = NULL;

  /// Get the Gicc object first. Rest of the form
  /// formation would depend upon information
  /// obtained from that object
  Status = GetEArmObjGicCInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GicCInfo,
             &CpuCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Hii-ARM: Failed to get GICC Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (CpuCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Hii-ARM: GIC CPU Interface information not provided.\n"
      ));
    ASSERT (CpuCount != 0);
    return EFI_INVALID_PARAMETER;
  }

  Status = InitFormPrivateData (CpuCount, (HII_ARM_CPU_GEN_PRIV **)&This->Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to Initialise the private data structure for the form."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Priv = This->Priv;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **)&Priv->HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Priv->DevHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &Priv->ConfigAccess,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SetupEfiVarBackend (Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to setup the Efi var backend information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = SetupFormset (Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to populate the Formset Strings."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = SetupFormsetVarstores (Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to populate the Varstore information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "INFO: %a: Done with Setting up Formset Varstores\n",
    __func__
    ));

  Status = SetupFormsetForms (Priv);
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to populate the Varstore information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = PopulateQuestionObjs (CpuCount, Priv);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to populate the Cpu Question information."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = DynHiiGenerateFormPackage (
             Priv->Formset,
             &Priv->IfrBuffer
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the IFR Byte Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  Status = DynHiiGenerateStringPackage (Priv->StrPkgInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to serialize the String Package Buffer."
      " Status = %r\n",
      __func__,
      Status
      ));

    return Status;
  }

  /// Add the template data to the Hii Package list
  *HiiHandle = HiiAddPackages (
                 &FormsetGuid,
                 Priv->DevHandle,
                 Priv->IfrBuffer->Data,
                 Priv->StrPkgInfo->StrPkgBuf,
                 NULL
                 );
  if (*HiiHandle == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: %a: Failed to add packages to Hii Database\n",
      __func__
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  return Status;
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form.

  @param [in]  Priv      Pointer to the private structure of the form.

**/
static
VOID
DeInitPrivateData (
  IN HII_ARM_CPU_GEN_PRIV  *Priv
  )
{
  UINT32  Idx;
  UINT32  VarstoreCount;

  VarstoreCount = Priv->VarstoreCount;
  for (Idx = 0; Idx < VarstoreCount; Idx++) {
    if ((Priv->HiiVarstore != NULL) &&
        (Priv->HiiVarstore[Idx].Name != NULL))
    {
      FreePool (Priv->HiiVarstore[Idx].Name);
    }
  }

  DynHiiFreeFormSet (Priv->Formset);
  DynHiiFreeFormPackage (Priv->IfrBuffer);

  if (Priv->StrPkgInfo != NULL) {
    DynHiiFreeStrings (&Priv->StrPkgInfo->StringList);
  }

  DynHiiFreeStringPackage (Priv->StrPkgInfo);

  if (Priv->HiiArmCpuFormQuestion != NULL) {
    FreePool (Priv->HiiArmCpuFormQuestion);
  }

  if (Priv->ArmCpuVar != NULL) {
    FreePool (Priv->ArmCpuVar);
  }

  if (Priv->HiiVarstore != NULL) {
    FreePool (Priv->HiiVarstore);
  }

  FreePool (Priv);
}

/** Free up all resources associated with this form.

  Free up all resources associated with this form. In addition remove
  the Hii Handle from the Hii database.

  @param [in]  This            Pointer to the HII forms generator.
  @param [in]  HiiFormsInfo    Pointer to the HII forms information.
  @param [in]  HiiHandle       Hii handle of this form.

  @retval The resources were freed successfully.

**/
static
EFI_STATUS
EFIAPI
FreeHiiArmCpuForm (
  IN        HII_FORMS_GENERATOR                            *This,
  IN  CONST CM_HII_FORMS_OBJ_INFO                  *CONST  HiiFormsInfo,
  IN  EFI_HII_HANDLE                                       *HiiHandle
  )
{
  HII_ARM_CPU_GEN_PRIV  *Priv;

  ASSERT (This != NULL);
  ASSERT (This->GeneratorID == HiiFormsInfo->FormGeneratorId);

  if (*HiiHandle != NULL) {
    HiiRemovePackages (*HiiHandle);
  }

  if (This->Priv == NULL) {
    return EFI_SUCCESS;
  }

  Priv = This->Priv;
  if (Priv->DevHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Priv->DevHandle,
           &gEfiDevicePathProtocolGuid,
           &mHiiVendorDevicePath0,
           &gEfiHiiConfigAccessProtocolGuid,
           &Priv->ConfigAccess,
           NULL
           );
  }

  DeInitPrivateData (This->Priv);

  return EFI_SUCCESS;
}

/** The interface for the Cpu Form Generator.
*/
static
HII_FORMS_GENERATOR  HiiCpuFormGenGeneric = {
  // Generator ID
  CREATE_HII_FORMS_GEN_ID (EStdHiiFormsIdCpuArm),
  // Generator Description
  L"HII.STD.ARM.CPU.FORM.GENERATOR",
  // Private structure for the Form (to be populated at runtime)
  NULL,
  // Build form function.
  BuildHiiArmCpuForm,
  // Free form function.
  FreeHiiArmCpuForm,
};

/** Register the Generator with the HII Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
HiiCpuFormConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterHiiFormsGenerator (&HiiCpuFormGenGeneric);
  DEBUG ((
    DEBUG_INFO,
    "HII-Cpu: Register Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the HII Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
HiiCpuFormDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterHiiFormsGenerator (&HiiCpuFormGenGeneric);
  DEBUG ((
    DEBUG_INFO,
    "HII-Cpu: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
