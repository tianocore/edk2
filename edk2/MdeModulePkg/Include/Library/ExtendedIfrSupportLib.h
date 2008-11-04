/** @file
  Library header file defines APIs that is related to IFR operations but 
  specific to EDK II implementation.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EXTENDED_IFR_SUPPORT_LIB_H__
#define __EXTENDED_IFR_SUPPORT_LIB_H__

/**
  Create GUIDed opcode for banner. Banner opcode
  EFI_IFR_EXTEND_OP_BANNER is extended opcode specific
  to Intel's implementation.

  @param  Title                  String ID for title
  @param  LineNumber             Line number for this banner
  @param  Alignment              Alignment for this banner, left, center or right
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateBannerOpCode (
  IN      EFI_STRING_ID       Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  );

/**
  This function initialize the data structure for dynamic opcode.

  @param UpdateData     The adding data;
  @param BufferSize     Length of the buffer to fill dynamic opcodes.

  @retval EFI_SUCCESS           Update data is initialized.
  @retval EFI_INVALID_PARAMETER UpdateData is NULL.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to allocate.

**/
EFI_STATUS
IfrLibInitUpdateData (
  IN OUT EFI_HII_UPDATE_DATA   *UpdateData,
  IN UINT32                    BufferSize
  )
;

/**

  This function free the resource of update data.

  @param UpdateData      The adding data;

  @retval EFI_SUCCESS            Resource in UpdateData is released.
  @retval EFI_INVALID_PARAMETER  UpdateData is NULL.

**/
EFI_STATUS
IfrLibFreeUpdateData (
  IN EFI_HII_UPDATE_DATA       *UpdateData
  )
;

/**
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.
  The update make use of a extended opcode EFI_IFR_EXTEND_OP_LABEL
  specific to Intel's implementation to complete the operation.
  

  @param  Handle                 Hii Handle
  @param  FormSetGuid            The formset should be updated.
  @param  FormId                 The form should be updated.
  @param  Label                  Update information starting immediately after this
                                 label in the IFR
  @param  Insert                 If TRUE and Data is not NULL, insert data after
                                 Label. If FALSE, replace opcodes between two
                                 labels with Data.
  @param  Data                   The adding data; If NULL, remove opcodes between
                                 two Label.

  @retval EFI_SUCCESS            Update success.
  @retval Other                  Update fail.

**/
EFI_STATUS
EFIAPI
IfrLibUpdateForm (
  IN EFI_HII_HANDLE            Handle,
  IN EFI_GUID                  *FormSetGuid, OPTIONAL
  IN EFI_FORM_ID               FormId,
  IN UINT16                    Label,
  IN BOOLEAN                   Insert,
  IN EFI_HII_UPDATE_DATA       *Data
  );

/**
  Extract formset class for given HII handle.

  If Handle is not a valid EFI_HII_HANDLE in the default HII database, then
  ASSERT.

  If Class is NULL, then ASSERT.
  IfFormSetTitle is NULL, then ASSERT.
  If FormSetHelp is NULL, then ASSERT.

  @param  HiiHandle              Hii handle
  @param  Class                    On output, Class of the formset
  @param  FormSetTitle          On output,  Formset title string
  @param  FormSetHelp          On output,   Formset help string

  @retval EFI_SUCCESS            Successfully extract Class for specified Hii
                                 handle.

**/
EFI_STATUS
EFIAPI
IfrLibExtractClassFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     UINT16              *Class,
  OUT     EFI_STRING_ID       *FormSetTitle,
  OUT     EFI_STRING_ID       *FormSetHelp
  );

/**
  Configure the buffer accrording to ConfigBody strings in the format of
  <Length:4 bytes>, <Offset: 2 bytes>, <Width:2 bytes>, <Data:n bytes>.

  @param  Buffer                 the start address of buffer.
  @param  BufferSize             the size of buffer.
  @param  Number                 the number of the ConfigBody strings.
  @param  ...                    the ConfigBody strings

  @retval EFI_BUFFER_TOO_SMALL   the BufferSize is too small to operate.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or BufferSize is 0.
  @retval EFI_SUCCESS            Operation successful.

**/
EFI_STATUS
EFIAPI
IfrLibExtractDefault(
  IN VOID                         *Buffer,
  IN UINTN                        *BufferSize,
  UINTN                           Number,
  ...
  );

#endif

