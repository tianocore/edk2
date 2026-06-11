/** @file
  Functions for dynamically building Formsets.

  This file contains function prototypes that
  are needed for generating Formsets and it's
  underlying nodes dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
    - Dyn        - Dynamic
    - Ref        - Cross-link Reference
**/

#pragma once

#include <Base.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/// A generic, opaque handle for HII objects
typedef VOID *DYN_HII_HANDLE;

/** Get a Form from a given formset

  Get a form from the given formset.

  @param [in]  FormsetHandle     Handle to the formset from which the form is
                                 to be fetched.
  @param [in]  FormId            The form ID of the required form.

  @retval  A pointer to the form object or NULL if form not found.

**/
DYN_HII_HANDLE
EFIAPI
DynHiiGetForm (
  IN  DYN_HII_HANDLE  FormsetHandle,
  IN  EFI_FORM_ID     FormId
  );

/** Get a question from a given form

  Get a question from the given form.

  @param [in]  FormHandle        Handle to the form from which the
                                 question is to be fetched.
  @param [in]  QuestionId        The question ID of the required
                                 question.

  @retval  A pointer to the question object or NULL if form not found.

**/
DYN_HII_HANDLE
EFIAPI
DynHiiGetQuestion (
  IN  DYN_HII_HANDLE   FormHandle,
  IN  EFI_QUESTION_ID  QuestionId
  );

/** Create a HII Formset.

  Create a HII Formset under which other HII nodes can
  be added subsequently.

  @param [in]  FormsetGuid      Formset GUID
  @param [in]  ClassGuid        Pointer to an array of Class GUIDs, if any.
  @param [in]  Title            String ID to the Formset title text.
  @param [in]  Help             String ID to the Formset help text.
  @param [in]  ClassGuidCount   Number of Class GUIDs(can be 0 - 3).
  @param [out] FormsetHandle    The Formset object created by the function.


  @retval  EFI_SUCCESS            The Formset was created successfully.
  @retval  EFI_INVALID_PARAMETER  The FormsetGuid or Formset is NULL, or the
                                  ClassGuidCount > 3.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiCreateFormSet (
  IN  CONST EFI_GUID  *FormsetGuid,
  IN  CONST EFI_GUID  *ClassGuid,
  IN  EFI_STRING_ID   Title,
  IN  EFI_STRING_ID   Help,
  IN  UINT8           ClassGuidCount,
  OUT DYN_HII_HANDLE  *FormsetHandle
  );

/** Add a form to a Formset.

  Create a HII Form and add it to the given Formset.

  @param [in]   FormsetHandle    Formset handle under which Form is to be added.
  @param [in]   Title            String ID to the Form's title text.
  @param [out]  FormId           Form Identifier.

  @retval  EFI_SUCCESS            The Form was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddForm (
  IN   DYN_HII_HANDLE  FormsetHandle,
  IN   EFI_STRING_ID   Title,
  OUT  EFI_FORM_ID     *FormId
  );

/** Add a Subtitle statement to a Form.

  Create a HII Subtitle statement and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  SubtitleFlags    The flags for subtitle opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddSubtitle (
  IN   DYN_HII_HANDLE  FormHandle,
  IN   EFI_STRING_ID   Prompt,
  IN   EFI_STRING_ID   Help,
  IN   UINT8           SubtitleFlags
  );

/** Add a Checkbox Question to a Form.

  Create a HII Checkbox Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  CheckboxFlags    The flags for checkbox opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The Question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddCheckbox (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            CheckboxFlags
  );

/** Add a Numeric Question to a Form.

  Create a HII Numeric Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  NumericFlags     The flags for numeric opcode.
  @param [in]  Minimum          The numeric minimum value.
  @param [in]  Maximum          The numeric maximum value.
  @param [in]  Step             The numeric step for edit.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddNumeric (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            NumericFlags,
  IN   UINT64           Minimum,
  IN   UINT64           Maximum,
  IN   UINT64           Step
  );

/** Add a Ref Question to a Form.

  Create a HII Ref Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
                                If its value is zero, then the link refers to
                                the top of the form.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId
  );

/** Add a Ref2 Question to a Form.

  Create a HII Ref2 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef2 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId
  );

/** Add a Ref3 Question to a Form.

  Create a HII Ref3 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.
  @param [in]  RefFormsetId     Target Formset GUID.
                                If its value is NULL, and RefDevicePath is zero,
                                then the link is to the current form set.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef3 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId,
  IN  CONST   EFI_GUID         *RefFormsetId
  );

/** Add a Ref4 Question to a Form.

  Create a HII Ref4 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  RefFormId        Target Form ID.
  @param [in]  RefQuestionId    Target Question ID.
                                If its value is zero, then the link refers to
                                the top of the form.
  @param [in]  RefFormsetId     Target Formset GUID.
                                If its value is NULL, and RefDevicePath is zero,
                                then the link is to the current form set.
  @param [in]  RefDevicePath    The string ID that specifies the string
                                containing the text representation of the device
                                path to which the form set containing the form
                                specified by FormId. If its value is zero,
                                then the link refers to the current page.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameters is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef4 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags,
  IN          EFI_FORM_ID      RefFormId,
  IN          EFI_QUESTION_ID  RefQuestionId,
  IN  CONST   EFI_GUID         *RefFormsetId,
  IN          EFI_STRING_ID    RefDevicePath
  );

/** Add a Ref5 Question to a Form.

  Create a HII Ref5 Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_UNSUPPORTED        The Ref type is unsupported.

**/
EFI_STATUS
EFIAPI
DynHiiAddRef5 (
  IN          DYN_HII_HANDLE   FormHandle,
  IN          EFI_QUESTION_ID  QuestionId,
  IN          EFI_STRING_ID    Prompt,
  IN          EFI_STRING_ID    Help,
  IN          UINT8            QuestionFlags
  );

/** Add a OneOf Question to a Form.

  Create a HII OneOf Question and add it to the given Form.

  @param [in]  FormHandle       Form handle under which Statement is to be
                                added.
  @param [in]  QuestionId       The question ID.
  @param [in]  VarstoreId       The variable store ID.
  @param [in]  VarOffset        Offset in Storage or String ID of the name (VarName)
                                for the name/value pair.
  @param [in]  Prompt           The string ID for Prompt.
  @param [in]  Help             The string ID for Help.
  @param [in]  QuestionFlags    The flags in Question Header.
  @param [in]  OneOfFlags       The flags for one-of opcode.
  @param [in]  ValueType        The value type taken by the one-of opcode.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Form is NULL.
  @retval  EFI_ALREADY_STARTED    The question has already been added to the
                                  Form.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOneOf (
  IN   DYN_HII_HANDLE   FormHandle,
  IN   EFI_QUESTION_ID  QuestionId,
  IN   EFI_VARSTORE_ID  VarstoreId,
  IN   UINT16           VarOffset,
  IN   EFI_STRING_ID    Prompt,
  IN   EFI_STRING_ID    Help,
  IN   UINT8            QuestionFlags,
  IN   UINT8            OneOfFlags,
  IN   UINT8            ValueType
  );

/** Add an Option to a Question.

  Create a HII Option and add it to the given Statement. Currently, adding
  of options only to the OneOf question type is supported.

  @param [in]  QuestionHandle   Question/Statement handle under which option
                                is to be added.
  @param [in]  Text             The string ID for the option description.
  @param [in]  Flags            Flags associated with the option.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Option.

  @retval  EFI_SUCCESS            The Option was created successfully.
  @retval  EFI_INVALID_PARAMETER  The any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOption (
  IN  DYN_HII_HANDLE      QuestionHandle,
  IN  EFI_STRING_ID       Text,
  IN  UINT8               Flags,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  );

/** Add a Default to a Question.

  Create a HII Default and add it to the given Statement.

  @param [in]  QuestionHandle   Question/Statement handle under which option
                                is to be added.
  @param [in]  DefaultId        The Default Store ID for this Default.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Default.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Statement is NULL.
  @retval  EFI_ALREADY_STARTED    The Default for the corresponding DefaultId
                                  has already been added for the Statement.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddDefault (
  IN  DYN_HII_HANDLE      QuestionHandle,
  IN  UINT16              DefaultId,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  );

/** Add a Buffer type Variable Store (Varstore) under a Formset.

  Add the buffer variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.
  @param [in]  Size               Size of the varstore.
  @param [in]  Name               Ascii name of the varstore.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreBuffer (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId,
  IN            UINT16           Size,
  IN  CONST     CHAR8            *Name
  );

/** Add an EFI type Variable Store (Varstore) under a Formset.

  Add the EFI variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.
  @param [in]  Size               Size of the varstore.
  @param [in]  Attributes         Flags for the varstore.
  @param [in]  Name               Ascii name of the varstore.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreEfi (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId,
  IN            UINT16           Size,
  IN            UINT32           Attributes,
  IN  CONST     CHAR8            *Name
  );

/** Add an name-value type Variable Store (Varstore) under a Formset.

  Add the name-value variable store under a formset.

  @param [in]  FormsetHandle      Formset handle under which Varstore is to be
                                  added.
  @param [in]  Guid               Varstore GUID.
  @param [in]  VarstoreId         Varstore ID.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddVarstoreNameValue (
  IN            DYN_HII_HANDLE   FormsetHandle,
  IN  CONST     EFI_GUID         *Guid,
  IN            EFI_VARSTORE_ID  VarstoreId
  );

/** Generate the IFR byte-stream for the Formset.

  Serialize the Formset and it's child nodes to generate an IFR byte-stream
  based Form package. This can then be added to the HII database through the
  HiiAddPackages() call.

  @param [in]  FormsetHandle      The formset hierarchy that needs to be
                                  serialized.
  @param [out] IfrHandle          Handle of the IFR buffer structure.
  @param [out] IfrBuf             Pointer to the IFR data buffer.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_INVALID_PARAMETER  One of the input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateFormPackage (
  IN  CONST DYN_HII_HANDLE  FormsetHandle,
  OUT       DYN_HII_HANDLE  *IfrHandle,
  OUT       UINT8           **IfrBuf
  );

/** Free up all the memory used by a Formset hierarchy.

  Free up all the memory that had been allocated for the Formset and it's
  child nodes, including Varstores, Forms, individual Statements etc.

  @param [in]  FormsetHandle    Handle to the formset hierarchy that needs to be
                                freed up.

**/
VOID
EFIAPI
DynHiiFreeFormSet (
  IN DYN_HII_HANDLE  FormsetHandle
  );

/** Free up all the memory used by a Form package.

  Free up all the memory that had been allocated for the Form package which
  keeps the serialized IFR byte-stream for the formset.

  @param [in]  IfrHandle           Pointer to DYN_HII_IFR_BUFFER.

**/
VOID
EFIAPI
DynHiiFreeFormPackage (
  IN  DYN_HII_HANDLE  IfrHandle
  );
