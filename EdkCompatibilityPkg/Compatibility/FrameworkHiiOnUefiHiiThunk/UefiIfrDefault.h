/** @file
  Header file for Function and Macro defintions for to extract default values from UEFI Form package.

  Copyright (c) 2008 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _HII_THUNK_UEFI_IFR_DEFAULT_
#define _HII_THUNK_UEFI_IFR_DEFAULT_

#define UEFI_IFR_BUFFER_STORAGE_NODE_FROM_LIST(a) CR(a, UEFI_IFR_BUFFER_STORAGE_NODE, List, UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE)
#define UEFI_IFR_BUFFER_STORAGE_NODE_SIGNATURE  SIGNATURE_32 ('I', 'b', 'S', 'n')
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
  Get the default value for Buffer Type storage from the FormSet in ThunkContext.
  
  The results can be multiple instances of UEFI_IFR_BUFFER_STORAGE_NODE. 
  They are inserted to the link list.
  
  @param  ThunkContext  Hii thunk context.
  @param  UefiDefaults  The head of link list for the output.

  @retval   EFI_SUCCESS          Successful.
  
**/
EFI_STATUS
UefiIfrGetBufferTypeDefaults (
  IN  HII_THUNK_CONTEXT   *ThunkContext,
  OUT LIST_ENTRY          **UefiDefaults
  );

/**
  Convert the UEFI Buffer Type default values to a Framework HII default
  values specified by a EFI_HII_VARIABLE_PACK_LIST structure.
  
  @param  ListHead             The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                               which contains the default values retrived from a UEFI form set.
  @param  DefaultMask          The default mask.
                               The valid values are EFI_IFR_FLAG_DEFAULT and EFI_IFR_FLAG_MANUFACTURING.
                               UEFI spec only map EFI_IFR_FLAG_DEFAULT and EFI_IFR_FLAG_MANUFACTURING 
                               from specification to valid default class.
  @param  UefiFormSetDefaultVarStoreId
                               ID of the default varstore in FormSet.
  @param  VariablePackList     The output default value in a format defined in Framework.

  @retval   EFI_SUCCESS                Successful.
  @retval   EFI_INVALID_PARAMETER      The default mask is not EFI_IFR_FLAG_DEFAULT or 
                                       EFI_IFR_FLAG_MANUFACTURING.
**/
EFI_STATUS
UefiDefaultsToFwDefaults (
  IN     LIST_ENTRY                  *ListHead,
  IN     UINTN                       DefaultMask,
  IN     EFI_VARSTORE_ID             UefiFormSetDefaultVarStoreId,
  OUT    EFI_HII_VARIABLE_PACK_LIST  **VariablePackList
  );

/**
  Free up all buffer allocated for the link list of UEFI_IFR_BUFFER_STORAGE_NODE.
    
  @param  ListHead                  The link list of UEFI_IFR_BUFFER_STORAGE_NODE
                                    which contains the default values retrived from
                                    a UEFI form set.

**/
VOID
FreeDefaultList (
  IN     LIST_ENTRY                  *ListHead
  );

#endif


