/** @file
Implement Functions to convert IFR Opcode in format defined in Framework HII specification to
format defined in UEFI HII Specification.

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_OPCODE_CREATION_H
#define _HII_THUNK_OPCODE_CREATION_H

EFI_STATUS
UCreateEndOfOpcode (
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );

EFI_STATUS
F2UCreateSubtitleOpCode (
  IN CONST FRAMEWORK_EFI_IFR_SUBTITLE  *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );

EFI_STATUS
F2UCreateTextOpCode (
  IN CONST FRAMEWORK_EFI_IFR_TEXT      *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );


EFI_STATUS
F2UCreateGotoOpCode (
  IN CONST FRAMEWORK_EFI_IFR_REF       *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );

EFI_STATUS
F2UCreateOneOfOptionOpCode (
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF_OPTION    *FwOpcode,
  IN       UINTN                              Width,
  OUT      EFI_HII_UPDATE_DATA                *UefiData
  );

EFI_STATUS
F2UCreateOneOfOpCode (
  IN       HII_THUNK_CONTEXT               *ThunkContext,
  IN       UINT16                      VarStoreId,
  IN CONST FRAMEWORK_EFI_IFR_ONE_OF    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *DataCount
  );

EFI_STATUS
F2UCreateOrderedListOpCode (
  IN       HII_THUNK_CONTEXT               *ThunkContext,
  IN       UINT16                      VarStoreId,
  IN CONST FRAMEWORK_EFI_IFR_ORDERED_LIST *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData,
  OUT      FRAMEWORK_EFI_IFR_OP_HEADER **NextFwOpcode,
  OUT      UINTN                       *DataCount
  );


EFI_STATUS
F2UCreateCheckBoxOpCode (
  IN       HII_THUNK_CONTEXT               *ThunkContext,
  IN       UINT16                      VarStoreId,
  IN CONST FRAMEWORK_EFI_IFR_CHECKBOX  *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );


EFI_STATUS
F2UCreateNumericOpCode (
  IN       HII_THUNK_CONTEXT               *ThunkContext,
  IN       UINT16                      VarStoreId,
  IN CONST FRAMEWORK_EFI_IFR_NUMERIC   *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );


EFI_STATUS
F2UCreateStringOpCode (
  IN       HII_THUNK_CONTEXT               *ThunkContext,
  IN       UINT16                      VarStoreId,
  IN CONST FRAMEWORK_EFI_IFR_STRING    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );


EFI_STATUS
F2UCreateBannerOpCode (
  IN CONST FRAMEWORK_EFI_IFR_BANNER    *FwOpcode,
  OUT      EFI_HII_UPDATE_DATA         *UefiData
  );

EFI_STATUS
FwUpdateDataToUefiUpdateData (
  IN       HII_THUNK_CONTEXT                 *ThunkContext,
  IN CONST FRAMEWORK_EFI_HII_UPDATE_DATA    *Data,
  OUT      EFI_HII_UPDATE_DATA              **UefiData
  );
#endif

