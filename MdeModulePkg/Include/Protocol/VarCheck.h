/** @file
  Variable check definitions.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_CHECK_H_
#define _VARIABLE_CHECK_H_

#include <Uefi/UefiSpec.h>

typedef struct _EDKII_VAR_CHECK_PROTOCOL EDKII_VAR_CHECK_PROTOCOL;

#define EDKII_VAR_CHECK_PROTOCOL_GUID  {\
  0xaf23b340, 0x97b4, 0x4685, { 0x8d, 0x4f, 0xa3, 0xf2, 0x81, 0x69, 0xb2, 0x1d } \
};

typedef EFI_SET_VARIABLE VAR_CHECK_SET_VARIABLE_CHECK_HANDLER;

/**
  Register SetVariable check handler.
  Variable driver will call the handler to do check before
  really setting the variable into variable storage.

  @param[in] Handler            Pointer to the check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VAR_CHECK_REGISTER_SET_VARIABLE_CHECK_HANDLER)(
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER   Handler
  );

#define VAR_CHECK_VARIABLE_PROPERTY_REVISION  0x0001
//
// 1. Set by VariableLock PROTOCOL
// 2. Set by VarCheck PROTOCOL
//
// If set, other fields for check will be ignored.
//
#define VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY  BIT0

typedef struct {
  UINT16    Revision;
  UINT16    Property;
  UINT32    Attributes;
  UINTN     MinSize;
  UINTN     MaxSize;
} VAR_CHECK_VARIABLE_PROPERTY;

typedef struct {
  EFI_GUID                       *Guid;
  CHAR16                         *Name;
  VAR_CHECK_VARIABLE_PROPERTY    VariableProperty;
} VARIABLE_ENTRY_PROPERTY;

/**
  Variable property set.
  Variable driver will do check according to the VariableProperty before
  really setting the variable into variable storage.

  @param[in] Name               Pointer to the variable name.
  @param[in] Guid               Pointer to the vendor GUID.
  @param[in] VariableProperty   Pointer to the input variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was set successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string,
                                or the fields of VariableProperty are not valid.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the variable property set request.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VAR_CHECK_VARIABLE_PROPERTY_SET)(
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  IN VAR_CHECK_VARIABLE_PROPERTY    *VariableProperty
  );

/**
  Variable property get.

  @param[in]  Name              Pointer to the variable name.
  @param[in]  Guid              Pointer to the vendor GUID.
  @param[out] VariableProperty  Pointer to the output variable property.

  @retval EFI_SUCCESS           The property of variable specified by the Name and Guid was got successfully.
  @retval EFI_INVALID_PARAMETER Name, Guid or VariableProperty is NULL, or Name is an empty string.
  @retval EFI_NOT_FOUND         The property of variable specified by the Name and Guid was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_VAR_CHECK_VARIABLE_PROPERTY_GET)(
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   *VariableProperty
  );

struct _EDKII_VAR_CHECK_PROTOCOL {
  EDKII_VAR_CHECK_REGISTER_SET_VARIABLE_CHECK_HANDLER    RegisterSetVariableCheckHandler;
  EDKII_VAR_CHECK_VARIABLE_PROPERTY_SET                  VariablePropertySet;
  EDKII_VAR_CHECK_VARIABLE_PROPERTY_GET                  VariablePropertyGet;
};

extern EFI_GUID  gEdkiiVarCheckProtocolGuid;

#endif
