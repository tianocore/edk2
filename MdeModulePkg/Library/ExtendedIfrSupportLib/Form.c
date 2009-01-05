/** @file
  Common Library Routines to assist handle HII elements.

Copyright (c) 2007 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "LibraryInternal.h"

extern EFI_HII_DATABASE_PROTOCOL *gIfrLibHiiDatabase;

/**
  Get the specified package from a package list based on an index.
  The Buffer on output is updated to point to a package header in
  the HiiPackageList. This is an internal function.

  @param HiiPackageList  The Package List Header.
  @param PackageIndex    The index of the package to get.
  @param BufferLen       The length of the package.
  @param Buffer          The starting address of package.

  @retval EFI_SUCCESS   This function completes successfully.
  @retval EFI_NOT_FOUND The package is not found.

**/
EFI_STATUS
GetPackageDataFromPackageList (
  IN  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList,
  IN  UINT32                      PackageIndex,
  OUT UINT32                      *BufferLen,
  OUT EFI_HII_PACKAGE_HEADER      **Buffer
  )
{
  UINT32                        Index;
  EFI_HII_PACKAGE_HEADER        *Package;
  UINT32                        Offset;
  UINT32                        PackageListLength;
  EFI_HII_PACKAGE_HEADER        PackageHeader;

  PackageHeader.Length = 0;
  PackageHeader.Type   = 0;

  ASSERT (HiiPackageList != NULL);

  if ((BufferLen == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Package = NULL;
  Index   = 0;
  Offset  = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);
  while (Offset < PackageListLength) {
    Package = (EFI_HII_PACKAGE_HEADER *) (((UINT8 *) HiiPackageList) + Offset);
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    if (Index == PackageIndex) {
      break;
    }
    Offset += PackageHeader.Length;
    Index++;
  }
  if (Offset >= PackageListLength) {
    //
    // no package found in this Package List
    //
    return EFI_NOT_FOUND;
  }

  *BufferLen = PackageHeader.Length;
  *Buffer    = Package;
  return EFI_SUCCESS;
}

/**
  This is the internal worker function to update the data in
  a form specified by FormSetGuid, FormId and Label.

  @param FormSetGuid     The optional Formset GUID.
  @param FormId          The Form ID.
  @param Package         The package header.
  @param PackageLength   The package length.
  @param Label           The label for the update.
  @param Insert          True if inserting opcode to the form.
  @param Data            The data payload.
  @param TempBuffer      The resultant package.
  @param TempBufferSize  The length of the resultant package.

  @retval EFI_OUT_OF_RESOURCES  If there is not enough memory to complete the operation.
  @retval EFI_INVALID_PARAMETER If TempBuffer or TempBufferSize is NULL.
  @retval EFI_SUCCESS    The function completes successfully.

**/
EFI_STATUS
EFIAPI
UpdateFormPackageData (
  IN  EFI_GUID               *FormSetGuid, OPTIONAL
  IN  EFI_FORM_ID            FormId,
  IN  EFI_HII_PACKAGE_HEADER *Package,
  IN  UINT32                 PackageLength,
  IN  UINT16                 Label,
  IN  BOOLEAN                Insert,
  IN  EFI_HII_UPDATE_DATA    *Data,
  OUT UINT8                  **TempBuffer,
  OUT UINT32                 *TempBufferSize
  )
{
  UINTN                     AddSize;
  UINT8                     *BufferPos;
  EFI_HII_PACKAGE_HEADER    PackageHeader;
  UINTN                     Offset;
  EFI_IFR_OP_HEADER         *IfrOpHdr;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;
  UINT8                     ExtendOpCode;
  UINT16                    LabelNumber;
  BOOLEAN                   Updated;
  EFI_IFR_OP_HEADER         *AddOpCode;

  if ((TempBuffer == NULL) || (TempBufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *TempBufferSize = PackageLength;
  if (Data != NULL) {
    *TempBufferSize += Data->Offset;
  }
  *TempBuffer = AllocateZeroPool (*TempBufferSize);
  if (*TempBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (*TempBuffer, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  *TempBufferSize = sizeof (EFI_HII_PACKAGE_HEADER);
  BufferPos = *TempBuffer + sizeof (EFI_HII_PACKAGE_HEADER);

  CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  IfrOpHdr   = (EFI_IFR_OP_HEADER *)((UINT8 *) Package + sizeof (EFI_HII_PACKAGE_HEADER));
  Offset     = sizeof (EFI_HII_PACKAGE_HEADER);
  GetFormSet = (BOOLEAN) ((FormSetGuid == NULL) ? TRUE : FALSE);
  GetForm    = FALSE;
  Updated    = FALSE;

  while (Offset < PackageHeader.Length) {
    CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
    BufferPos += IfrOpHdr->Length;
    *TempBufferSize += IfrOpHdr->Length;

    switch (IfrOpHdr->OpCode) {
    case EFI_IFR_FORM_SET_OP :
      if (FormSetGuid != NULL) {
        if (CompareMem (&((EFI_IFR_FORM_SET *) IfrOpHdr)->Guid, FormSetGuid, sizeof (EFI_GUID)) == 0) {
          GetFormSet = TRUE;
        }
      }
      break;

    case EFI_IFR_FORM_OP:
      if (CompareMem (&((EFI_IFR_FORM *) IfrOpHdr)->FormId, &FormId, sizeof (EFI_FORM_ID)) == 0) {
        GetForm = TRUE;
      }
      break;

    case EFI_IFR_GUID_OP :
      if (!GetFormSet || !GetForm || Updated) {
        //
        // Go to the next Op-Code
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
        continue;
      }

      ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
      LabelNumber = ReadUnaligned16 ((UINT16 *)(VOID*)&((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Number);
      if ((ExtendOpCode != EFI_IFR_EXTEND_OP_LABEL) || (LabelNumber != Label) 
          || !CompareGuid ((EFI_GUID *)(UINTN)(&((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Guid), &mIfrVendorGuid)) {
        //
        // Go to the next Op-Code
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
        continue;
      }

      if (Insert && (Data != NULL)) {
        //
        // insert the DataCount amount of opcodes to TempBuffer if Data is NULL remove
        // DataCount amount of opcodes unless runing into a label.
        //
        AddOpCode = (EFI_IFR_OP_HEADER *)Data->Data;
        AddSize   = 0;
        while (AddSize < Data->Offset) {
          CopyMem (BufferPos, AddOpCode, AddOpCode->Length);
          BufferPos += AddOpCode->Length;
          *TempBufferSize += AddOpCode->Length;

          AddSize += AddOpCode->Length;
          AddOpCode = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (AddOpCode) + AddOpCode->Length);
        }
      } else {
        //
        // Search the next Label.
        //
        while (TRUE) {
          Offset   += IfrOpHdr->Length;
          //
          // Search the next label and Fail if not label found.
          //
          if (Offset >= PackageHeader.Length) {
            goto Fail;
          }
          IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
          if (IfrOpHdr->OpCode == EFI_IFR_GUID_OP) {
            ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
            if ((ExtendOpCode == EFI_IFR_EXTEND_OP_LABEL) && CompareGuid ((EFI_GUID *)(UINTN)(&((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Guid), &mIfrVendorGuid)) {
              break;
            }
          }
        }

        if (Data != NULL) {
          AddOpCode = (EFI_IFR_OP_HEADER *)Data->Data;
          AddSize   = 0;
          while (AddSize < Data->Offset) {
            CopyMem (BufferPos, AddOpCode, AddOpCode->Length);
            BufferPos += AddOpCode->Length;
            *TempBufferSize += AddOpCode->Length;

            AddSize   += AddOpCode->Length;
            AddOpCode = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (AddOpCode) + AddOpCode->Length);
          }
        }

        //
        // copy the next label
        //
        CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
        BufferPos += IfrOpHdr->Length;
        *TempBufferSize += IfrOpHdr->Length;
      }

      Updated = TRUE;
      break;
    default :
      break;
    }

    //
    // Go to the next Op-Code
    //
    Offset   += IfrOpHdr->Length;
    IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
  }

  //
  // Update the package length.
  //
  PackageHeader.Length = *TempBufferSize;
  CopyMem (*TempBuffer, &PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));

Fail:
  if (!Updated) {
    FreePool (*TempBuffer);
    *TempBufferSize = 0;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

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
{
  ASSERT (UpdateData != NULL);

  UpdateData->BufferSize = BufferSize;
  UpdateData->Offset = 0;
  UpdateData->Data = AllocatePool (BufferSize);

  return (UpdateData->Data != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

/**

  This function free the resource of update data.

  @param UpdateData      The adding data;

**/
VOID
IfrLibFreeUpdateData (
  IN EFI_HII_UPDATE_DATA       *UpdateData
  )
{
  ASSERT (UpdateData != NULL);
  
  FreePool (UpdateData->Data);
  UpdateData->Data = NULL;

}

/**
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.

  @param  Handle                 Hii Handle
  @param  FormSetGuid            The formset should be updated.
  @param  FormId                 The form should be updated.
  @param  Label                  Update information starting immediately after this
                                 label in the IFR
  @param  Insert                 If TRUE and Data is not NULL, insert data after
                                 Label. If FALSE, replace opcodes between two
                                 labels with Data
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
  )
{
  EFI_STATUS                   Status;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       Index;
  EFI_HII_PACKAGE_LIST_HEADER  *UpdateBuffer;
  UINTN                        BufferSize;
  UINT8                        *UpdateBufferPos;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  EFI_HII_PACKAGE_HEADER       *Package;
  UINT32                       PackageLength;
  EFI_HII_PACKAGE_HEADER       *TempBuffer;
  UINT32                       TempBufferSize;
  BOOLEAN                      Updated;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiDatabase = gIfrLibHiiDatabase;

  //
  // Get the orginal package list
  //
  BufferSize = 0;
  HiiPackageList   = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
    if (EFI_ERROR (Status)) {
      FreePool (HiiPackageList);
      return Status;
    }
  }

  //
  // Calculate and allocate space for retrieval of IFR data
  //
  BufferSize += Data->Offset;
  UpdateBuffer = AllocateZeroPool (BufferSize);
  if (UpdateBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UpdateBufferPos = (UINT8 *) UpdateBuffer;

  //
  // copy the package list header
  //
  CopyMem (UpdateBufferPos, HiiPackageList, sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  UpdateBufferPos += sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  Updated = FALSE;
  for (Index = 0; ; Index++) {
    Status = GetPackageDataFromPackageList (HiiPackageList, Index, &PackageLength, &Package);
    if (Status == EFI_SUCCESS) {
      CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
      if ((PackageHeader.Type == EFI_HII_PACKAGE_FORMS) && !Updated) {
        Status = UpdateFormPackageData (FormSetGuid, FormId, Package, PackageLength, Label, Insert, Data, (UINT8 **)&TempBuffer, &TempBufferSize);
        if (!EFI_ERROR(Status)) {
          if (FormSetGuid == NULL) {
            Updated = TRUE;
          }
          CopyMem (UpdateBufferPos, TempBuffer, TempBufferSize);
          UpdateBufferPos += TempBufferSize;
          FreePool (TempBuffer);
          continue;
        }
      }

      CopyMem (UpdateBufferPos, Package, PackageLength);
      UpdateBufferPos += PackageLength;
    } else if (Status == EFI_NOT_FOUND) {
      break;
    } else {
      FreePool (HiiPackageList);
      return Status;
    }
  }

  //
  // Update package list length
  //
  BufferSize = UpdateBufferPos - (UINT8 *) UpdateBuffer;
  WriteUnaligned32 (&UpdateBuffer->PackageLength, (UINT32)BufferSize);

  FreePool (HiiPackageList);

  return HiiDatabase->UpdatePackageList (HiiDatabase, Handle, UpdateBuffer);
}


/**
  Configure the buffer accrording to ConfigBody strings in the format of
  <Length:4 bytes>, <Offset: 2 bytes>, <Width:2 bytes>, <Data:n bytes>.
  This ConfigBody strings is generated by UEFI VfrCompiler for the default
  values in a Form Set. The name of the ConfigBody strings is VfrMyIfrNVDataDefault0000
  constructed following this rule: 
   "Vfr" + varstore.name + "Default" + defaultstore.attributes.
  Check the generated C file in Output for details.

  @param  Buffer                 The start address of buffer.
  @param  BufferSize             The size of buffer.
  @param  Number                 The number of the strings.
  @param  ...                    Variable argument list for default value in <AltResp> format 
                                 generated by the tool.

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
  )
{
  VA_LIST                         Args;
  UINTN                           Index;
  UINT32                          TotalLen;
  UINT8                           *BufCfgArray;
  UINT8                           *BufferPos;
  UINT16                          Offset;
  UINT16                          Width;
  UINT8                           *Value;

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  Width  = 0;
  Value  = NULL;

  VA_START (Args, Number);
  for (Index = 0; Index < Number; Index++) {
    BufCfgArray = (UINT8 *) VA_ARG (Args, VOID *);
    TotalLen = ReadUnaligned32 ((UINT32 *)BufCfgArray);
    BufferPos = BufCfgArray + sizeof (UINT32);

    while ((UINT32)(BufferPos - BufCfgArray) < TotalLen) {
      Offset = ReadUnaligned16 ((UINT16 *)BufferPos);
      BufferPos += sizeof (UINT16);
      Width = ReadUnaligned16 ((UINT16 *)BufferPos);
      BufferPos += sizeof (UINT16);
      Value = BufferPos;
      BufferPos += Width;

      if ((UINTN)(Offset + Width) > *BufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      CopyMem ((UINT8 *)Buffer + Offset, Value, Width);
    }
  }
  VA_END (Args);

  *BufferSize = (UINTN)Offset;

  return EFI_SUCCESS;
}


