/** @file
  Provides variable check services and database management.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VARIABLE_CHECK_LIB_H_
#define _VARIABLE_CHECK_LIB_H_

#include <Protocol/VarCheck.h>

typedef enum {
  VarCheckRequestReserved0 = 0,
  VarCheckRequestReserved1 = 1,
  VarCheckFromTrusted = 2,
  VarCheckFromUntrusted = 3,
} VAR_CHECK_REQUEST_SOURCE;

typedef
VOID
(EFIAPI *VAR_CHECK_END_OF_DXE_CALLBACK) (
  VOID
  );

/**
  Register END_OF_DXE callback.
  The callback will be invoked by VarCheckLibInitializeAtEndOfDxe().

  @param[in] Callback           END_OF_DXE callback.

  @retval EFI_SUCCESS           The callback was registered successfully.
  @retval EFI_INVALID_PARAMETER Callback is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the callback register request.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterEndOfDxeCallback (
  IN VAR_CHECK_END_OF_DXE_CALLBACK  Callback
  );

/**
  Var check initialize at END_OF_DXE.

  This function needs to be called at END_OF_DXE.
  Address pointers may be returned,
  and caller needs to ConvertPointer() for the pointers.

  @param[in, out] AddressPointerCount   Output pointer to address pointer count.

  @return Address pointer buffer, NULL if input AddressPointerCount is NULL.

**/
VOID ***
EFIAPI
VarCheckLibInitializeAtEndOfDxe (
  IN OUT UINTN                  *AddressPointerCount OPTIONAL
  );

/**
  Register address pointer.
  The AddressPointer may be returned by VarCheckLibInitializeAtEndOfDxe().

  @param[in] AddressPointer     Address pointer.

  @retval EFI_SUCCESS           The address pointer was registered successfully.
  @retval EFI_INVALID_PARAMETER AddressPointer is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the address pointer register request.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterAddressPointer (
  IN VOID                       **AddressPointer
  );

/**
  Register SetVariable check handler.

  @param[in] Handler            Pointer to check handler.

  @retval EFI_SUCCESS           The SetVariable check handler was registered successfully.
  @retval EFI_INVALID_PARAMETER Handler is NULL.
  @retval EFI_ACCESS_DENIED     EFI_END_OF_DXE_EVENT_GROUP_GUID or EFI_EVENT_GROUP_READY_TO_BOOT has
                                already been signaled.
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource for the SetVariable check handler register request.
  @retval EFI_UNSUPPORTED       This interface is not implemented.
                                For example, it is unsupported in VarCheck protocol if both VarCheck and SmmVarCheck protocols are present.

**/
EFI_STATUS
EFIAPI
VarCheckLibRegisterSetVariableCheckHandler (
  IN VAR_CHECK_SET_VARIABLE_CHECK_HANDLER   Handler
  );

/**
  Variable property set.

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
EFI_STATUS
EFIAPI
VarCheckLibVariablePropertySet (
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
EFI_STATUS
EFIAPI
VarCheckLibVariablePropertyGet (
  IN CHAR16                         *Name,
  IN EFI_GUID                       *Guid,
  OUT VAR_CHECK_VARIABLE_PROPERTY   *VariableProperty
  );

/**
  SetVariable check.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.
  @param[in] RequestSource      Request source.

  @retval EFI_SUCCESS           The SetVariable check result was success.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits, name, GUID,
                                DataSize and Data value was supplied.
  @retval EFI_WRITE_PROTECTED   The variable in question is read-only.
  @retval Others                The other return status from check handler.

**/
EFI_STATUS
EFIAPI
VarCheckLibSetVariableCheck (
  IN CHAR16                     *VariableName,
  IN EFI_GUID                   *VendorGuid,
  IN UINT32                     Attributes,
  IN UINTN                      DataSize,
  IN VOID                       *Data,
  IN VAR_CHECK_REQUEST_SOURCE   RequestSource
  );

#endif
