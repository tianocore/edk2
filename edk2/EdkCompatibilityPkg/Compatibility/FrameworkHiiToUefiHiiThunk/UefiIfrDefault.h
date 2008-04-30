/** @file
  Header file for Function and Macro defintions for to extract default values from UEFI Form package.

  Copyright (c) 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_DEFAULT_
#define _HII_THUNK_UEFI_IFR_DEFAULT_

//
// VARSTORE ID of 0 for Buffer Storage Type Storage is reserved in UEFI IFR form. But VARSTORE ID
// 0 in Framework IFR is the default VarStore ID for storage without explicit declaration. So we have
// to reseved 0xFFEE in UEFI VARSTORE ID to represetn default storage id in Framework IFR.
// Framework VFR has to be ported or pre-processed to change the default VARSTORE to a VARSTORE
// with ID equal to 0xFFEE.
//
#define RESERVED_VARSTORE_ID 0xFFEE

#define UEFI_IFR_BUFFER_STORAGE_NODE_FROM_LIST(a) CR(a, UEFI_IFR_BUFFER_STORAGE_NODE, List, UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE)
#define UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE  EFI_SIGNATURE_32 ('I', 'b', 'S', 'n')
typedef struct {
  LIST_ENTRY   List;
  UINT32       Signature;

  EFI_GUID     Guid;
  CHAR16       *Name;
  UINT16       DefaultId;
  UINT16       StoreId;
  UINTN        Size;
  UINT8        *Buffer;
  
} UEFI_IFR_BUFFER_STORAGE_NODE;

/**
  Get the default value for Buffer Type storage from the first FormSet
  in the Package List specified by a EFI_HII_HANDLE.
  
  The results can be multiple instances of UEFI_IFR_BUFFER_STORAGE_NODE. 
  They are inserted to the link list.
  
  @param  UefiHiiHandle           The handle for the package list.
  @param  UefiDefaultsListHead The head of link list for the output.

  @retval   EFI_SUCCESS          Successful.
  
**/
EFI_STATUS
UefiIfrGetBufferTypeDefaults (
  EFI_HII_HANDLE      UefiHiiHandle,
  LIST_ENTRY          **UefiDefaults
);

/**
  Convert the UEFI Buffer Type default values to a Framework HII default
  values specified by a EFI_HII_VARIABLE_PACK_LIST structure.
  
  @param  ListHead                  The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                                              which contains the default values retrived from
                                              a UEFI form set.
  @param  DefaultMask            The default mask.
                                             The valid values are FRAMEWORK_EFI_IFR_FLAG_DEFAULT
                                             and FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING.
                                            UEFI spec only map FRAMEWORK_EFI_IFR_FLAG_DEFAULT and FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING 
                                            from specification to valid default class.
  @param  VariablePackList     The output default value in a format defined in Framework.
                                             

  @retval   EFI_SUCCESS                       Successful.
  @retval   EFI_INVALID_PARAMETER      The default mask is not FRAMEWORK_EFI_IFR_FLAG_DEFAULT or 
                                                           FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING.
**/

EFI_STATUS
UefiDefaultsToFrameworkDefaults (
  IN     LIST_ENTRY                  *UefiIfrDefaults,
  IN     UINTN                       DefaultMask,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  )
;

/**
  Free up all buffer allocated for the link list of UEFI_IFR_BUFFER_STORAGE_NODE.
    
  @param  ListHead                  The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                                              which contains the default values retrived from
                                              a UEFI form set.
                                             

  @retval   EFI_SUCCESS                       Successful.
  @retval   EFI_INVALID_PARAMETER      The default mask is not FRAMEWORK_EFI_IFR_FLAG_DEFAULT or 
                                                           FRAMEWORK_EFI_IFR_FLAG_MANUFACTURING.
**/
VOID
FreeDefaultList (
  IN     LIST_ENTRY                  *UefiIfrDefaults
  )
;

#endif


