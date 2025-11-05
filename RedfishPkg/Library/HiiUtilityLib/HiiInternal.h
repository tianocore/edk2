/** @file
  HII internal header file.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_INTERNAL_H_
#define HII_INTERNAL_H_

#include <Uefi.h>

#include <Protocol/UnicodeCollation.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/UserManager.h>
#include <Protocol/DevicePathFromText.h>
#include <Protocol/RegularExpressionProtocol.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/ZeroGuid.h>
#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>

#include "HiiExpression.h"
#include <Library/HiiUtilityLib.h>

#define EXPRESSION_STACK_SIZE_INCREMENT  0x100
#define EFI_IFR_SPECIFICATION_VERSION    (UINT16) (((EFI_SYSTEM_TABLE_REVISION >> 16) << 8) | (((EFI_SYSTEM_TABLE_REVISION & 0xFFFF) / 10) << 4) | ((EFI_SYSTEM_TABLE_REVISION & 0xFFFF) % 10))

///
/// Definition of HII_FORM_CONFIG_REQUEST
//
typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;

  CHAR16                 *ConfigRequest; ///< <ConfigRequest> = <ConfigHdr> + <RequestElement>
  CHAR16                 *ConfigAltResp; ///< Alt config response string for this ConfigRequest.
  UINTN                  ElementCount;   ///< Number of <RequestElement> in the <ConfigRequest>
  UINTN                  SpareStrLen;
  CHAR16                 *RestoreConfigRequest; ///< When submit form fail, the element need to be restored
  CHAR16                 *SyncConfigRequest;    ///< When submit form fail, the element need to be synced

  HII_FORMSET_STORAGE    *Storage;
} HII_FORM_CONFIG_REQUEST;

#define HII_FORM_CONFIG_REQUEST_SIGNATURE  SIGNATURE_32 ('F', 'C', 'R', 'S')
#define HII_FORM_CONFIG_REQUEST_FROM_LINK(a)  CR (a, HII_FORM_CONFIG_REQUEST, Link, HII_FORM_CONFIG_REQUEST_SIGNATURE)

///
/// Incremental string length of ConfigRequest
///
#define CONFIG_REQUEST_STRING_INCREMENTAL  1024

/**
  Allocate new memory and then copy the Unicode string Source to Destination.

  @param[in,out]  Dest                   Location to copy string
  @param[in]      Src                    String to copy

**/
VOID
NewStringCopy (
  IN OUT CHAR16  **Dest,
  IN     CHAR16  *Src
  );

/**
  Set Value of given Name in a NameValue Storage.

  @param[in]  Storage                The NameValue Storage.
  @param[in]  Name                   The Name.
  @param[in]  Value                  The Value to set.
  @param[out] ReturnNode             The node use the input name.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
SetValueByName (
  IN     HII_FORMSET_STORAGE  *Storage,
  IN     CHAR16               *Name,
  IN     CHAR16               *Value,
  OUT HII_NAME_VALUE_NODE     **ReturnNode
  );

/**
  Get bit field value from the buffer and then set the value for the question.
  Note: Data type UINT32 can cover all the bit field value.

  @param[in]  Question        The question refer to bit field.
  @param[in]  Buffer          Point to the buffer which the question value get from.
  @param[out] QuestionValue   The Question Value retrieved from Bits.

**/
VOID
GetBitsQuestionValue (
  IN     HII_STATEMENT     *Question,
  IN     UINT8             *Buffer,
  OUT HII_STATEMENT_VALUE  *QuestionValue
  );

/**
  Set bit field value to the buffer.
  Note: Data type UINT32 can cover all the bit field value.

  @param[in]     Question        The question refer to bit field.
  @param[in,out] Buffer          Point to the buffer which the question value set to.
  @param[in]     Value           The bit field value need to set.

**/
VOID
SetBitsQuestionValue (
  IN     HII_STATEMENT  *Question,
  IN OUT UINT8          *Buffer,
  IN     UINT32         Value
  );

/**
  Convert the buffer value to HiiValue.

  @param[in]  Question              The question.
  @param[in]  Value                 Unicode buffer save the question value.
  @param[out] QuestionValue         The Question Value retrieved from Buffer.

  @retval  Status whether convert the value success.

**/
EFI_STATUS
BufferToQuestionValue (
  IN     HII_STATEMENT     *Question,
  IN     CHAR16            *Value,
  OUT HII_STATEMENT_VALUE  *QuestionValue
  );

/**
  Get the string based on the StringId and HII Package List Handle.

  @param[in]  Token                  The String's ID.
  @param[in]  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetTokenString (
  IN EFI_STRING_ID   Token,
  IN EFI_HII_HANDLE  HiiHandle
  );

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param[in] ConfigString  String to be converted

**/
VOID
EFIAPI
HiiStringToLowercase (
  IN EFI_STRING  ConfigString
  );

/**
  Evaluate if the result is a non-zero value.

  @param[in]  Result       The result to be evaluated.

  @retval TRUE             It is a non-zero value.
  @retval FALSE            It is a zero value.

**/
BOOLEAN
IsHiiValueTrue (
  IN EFI_HII_VALUE  *Result
  );

/**
  Set a new string to string package.

  @param[in]  String              A pointer to the Null-terminated Unicode string
                                  to add or update in the String Package associated
                                  with HiiHandle.
  @param[in]  HiiHandle           A handle that was previously registered in the
                                  HII Database.

  @return the Id for this new string.

**/
EFI_STRING_ID
NewHiiString (
  IN CHAR16          *String,
  IN EFI_HII_HANDLE  HiiHandle
  );

/**
  Perform nosubmitif check for a Form.

  @param[in]  FormSet                FormSet data structure.
  @param[in]  Form                   Form data structure.
  @param[in]  Question               The Question to be validated.

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
ValidateNoSubmit (
  IN HII_FORMSET    *FormSet,
  IN HII_FORM       *Form,
  IN HII_STATEMENT  *Question
  );

/**
  Perform NoSubmit check for each Form in FormSet.

  @param[in]     FormSet                FormSet data structure.
  @param[in,out] CurrentForm            Current input form data structure.
  @param[out]    Statement              The statement for this check.

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
NoSubmitCheck (
  IN     HII_FORMSET  *FormSet,
  IN OUT HII_FORM     **CurrentForm,
  OUT HII_STATEMENT   **Statement
  );

/**
  Convert setting of Buffer Storage or NameValue Storage to <ConfigResp>.

  @param[in]  Storage                The Storage to be converted.
  @param[in]  ConfigResp             The returned <ConfigResp>.
  @param[in]  ConfigRequest          The ConfigRequest string.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
StorageToConfigResp (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               **ConfigResp,
  IN CHAR16               *ConfigRequest
  );

/**
  Convert <ConfigResp> to settings in Buffer Storage or NameValue Storage.

  @param[in]  Storage                The Storage to receive the settings.
  @param[in]  ConfigResp             The <ConfigResp> to be converted.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
ConfigRespToStorage (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               *ConfigResp
  );

/**
  Fetch the Ifr binary data of a FormSet.

  @param[in]  Handle             PackageList Handle
  @param[in,out]  FormSetGuid    On input, GUID or class GUID of a formset. If not
                                 specified (NULL or zero GUID), take the first
                                 FormSet with class GUID EFI_HII_PLATFORM_SETUP_FORMSET_GUID
                                 found in package list.
                                 On output, GUID of the formset found(if not NULL).
  @param[out]  BinaryLength      The length of the FormSet IFR binary.
  @param[out]  BinaryData        The buffer designed to receive the FormSet.

  @retval EFI_SUCCESS            Buffer filled with the requested FormSet.
                                 BufferLength was updated.
  @retval EFI_INVALID_PARAMETER  The handle is unknown.
  @retval EFI_NOT_FOUND          A form or FormSet on the requested handle cannot
                                 be found with the requested FormId.

**/
EFI_STATUS
GetIfrBinaryData (
  IN     EFI_HII_HANDLE  Handle,
  IN OUT EFI_GUID        *FormSetGuid,
  OUT    UINTN           *BinaryLength,
  OUT    UINT8           **BinaryData
  );

/**
  Fill storage with settings requested from Configuration Driver.

  @param[in] FormSet                FormSet data structure.
  @param[in] Storage                Buffer Storage.

**/
VOID
LoadFormSetStorage (
  IN HII_FORMSET          *FormSet,
  IN HII_FORMSET_STORAGE  *Storage
  );

/**
  Free resources of a Form.

  @param[in]     FormSet                Pointer of the FormSet
  @param[in,out] Form                   Pointer of the Form.

**/
VOID
DestroyForm (
  IN     HII_FORMSET  *FormSet,
  IN OUT HII_FORM     *Form
  );

/**
  Get formset storage based on the input varstoreid info.

  @param[in]  FormSet                Pointer of the current FormSet.
  @param[in]  VarStoreId             Varstore ID info.

  @return Pointer to a HII_FORMSET_STORAGE data structure.

**/
HII_FORMSET_STORAGE *
GetFstStgFromVarId (
  IN HII_FORMSET      *FormSet,
  IN EFI_VARSTORE_ID  VarStoreId
  );

/**
  Zero extend integer/boolean/date/time to UINT64 for comparing.

  @param[in]  Value                  HII Value to be converted.

**/
VOID
ExtendValueToU64 (
  IN HII_STATEMENT_VALUE  *Value
  );

/**
  Parse opcodes in the formset IFR binary.

  @param[in]  FormSet                Pointer of the FormSet data structure.

  @retval EFI_SUCCESS            Opcode parse success.
  @retval Other                  Opcode parse fail.

**/
EFI_STATUS
ParseOpCodes (
  IN HII_FORMSET  *FormSet
  );

#endif // HII_INTERNAL_H_
