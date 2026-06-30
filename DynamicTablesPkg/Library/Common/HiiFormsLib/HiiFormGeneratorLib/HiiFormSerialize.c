/** @file
  Dynamic Hii Form Serialization API functions

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <HiiFormGeneratorLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

#define IFR_INITIAL_BUFFER_SIZE  1024

/** Re-size the IFR byte-stream buffer.

  Re-size the IFR byte-stream buffer to accommodate the TotalSize passed to
  the function.

  @param [in]  BufInfo          IFR buffer data structure pointer.
  @param [in]  TotalSize        Minimum number of bytes that the function should
                                allocate.
  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiResizeIfrByteBuffer (
  IN       DYN_HII_IFR_BUFFER  *BufInfo,
  IN       UINTN               TotalSize
  )
{
  UINTN  NewSize;
  UINT8  *Buf;

  NewSize = BufInfo->Size * 2;
  NewSize = MAX (TotalSize, NewSize);

  Buf = ReallocatePool (BufInfo->Size, NewSize, BufInfo->Data);
  if (Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  BufInfo->Data = Buf;
  BufInfo->Size = (UINT32)NewSize;

  return EFI_SUCCESS;
}

/** Initialize the IFR byte-stream buffer.

  Initialize the IFR byte-stream buffer with IFR_INITIAL_BUFFER_SIZE bytes.
  Also allocate memory for DYN_HII_IFR_BUFFER structure which holds the IFR
  buffer.

  @param [out]  Buf              Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiInitIfrByteBuffer (
  OUT DYN_HII_IFR_BUFFER  **Buf
  )
{
  *Buf = AllocateZeroPool (sizeof (DYN_HII_IFR_BUFFER));
  if (*Buf == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*Buf)->Data = AllocateZeroPool (IFR_INITIAL_BUFFER_SIZE);
  if ((*Buf)->Data == NULL) {
    FreePool (*Buf);
    return EFI_OUT_OF_RESOURCES;
  }

  (*Buf)->Size     = IFR_INITIAL_BUFFER_SIZE;
  (*Buf)->Position = sizeof (UINT32) + sizeof (EFI_HII_PACKAGE_HEADER);

  return EFI_SUCCESS;
}

/** Check if the IFR byte buffer is large enough.

  Check if the IFR byte buffer is large enough to hold Size bytes. If not,
  call the function to re-allocate the sufficient number of bytes.

  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.
  @param [in]  Size               The number of bytes that the buffer should
                                  be able to hold.

  @retval  EFI_SUCCESS            The IFR buffer is large enough, or the re-size
                                  operation was successfull.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiIfrBufferSizeCheck (
  IN       DYN_HII_IFR_BUFFER  *BufInfo,
  IN       UINTN               Size
  )
{
  if (BufInfo->Size - BufInfo->Position >= Size) {
    return EFI_SUCCESS;
  }

  return DynHiiResizeIfrByteBuffer (BufInfo, BufInfo->Size + Size);
}

/** Insert the EFI_IFR_END opcode into the IFR buffer.

  Insert the EFI_IFR_END opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiInsertEndOp (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8        *Buf;
  UINTN        EndSize;
  EFI_STATUS   Status;
  EFI_IFR_END  End;

  if (Hdr->Scope == 0) {
    return EFI_SUCCESS;
  }

  EndSize = sizeof (EFI_IFR_END);
  if (EndSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, EndSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf = BufInfo->Data + BufInfo->Position;

  End.Header.OpCode = EFI_IFR_END_OP;
  End.Header.Length = (UINT8)EndSize;
  End.Header.Scope  = 0;

  CopyMem (Buf, &End, sizeof (EFI_IFR_END));
  BufInfo->Position += sizeof (EFI_IFR_END);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_VARSTORE opcode into the IFR buffer.

  Insert the EFI_IFR_VARSTORE opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeVarstoreBuffer (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                         *Buf;
  UINT32                        StrSize;
  UINTN                         VarstoreSize;
  EFI_STATUS                    Status;
  EFI_IFR_VARSTORE              Varstore;
  DYN_HII_VARSTORE              *VarstoreNode;
  DYN_HII_VARSTORE_BUFFER_DATA  *VarstoreBuf;

  VarstoreNode = (DYN_HII_VARSTORE *)Hdr;
  VarstoreBuf  = &VarstoreNode->Data.Buffer;
  StrSize      = (UINT32)AsciiStrSize (VarstoreBuf->Name);
  VarstoreSize = sizeof (EFI_IFR_VARSTORE) + StrSize;
  if (VarstoreSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, VarstoreSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                    = BufInfo->Data + BufInfo->Position;
  Varstore.Header.OpCode = EFI_IFR_VARSTORE_OP;
  Varstore.Header.Length = (UINT8)VarstoreSize;
  Varstore.Header.Scope  = 0;
  CopyMem (&Varstore.Guid, &VarstoreBuf->Guid, sizeof (EFI_GUID));
  Varstore.VarStoreId = VarstoreBuf->VarstoreId;
  Varstore.Size       = VarstoreBuf->Size;

  CopyMem (Buf, &Varstore, sizeof (EFI_IFR_VARSTORE));
  BufInfo->Position += sizeof (EFI_IFR_VARSTORE);
  Buf               += sizeof (EFI_IFR_VARSTORE);

  Status = AsciiStrCpyS ((CHAR8 *)Buf, StrSize, VarstoreBuf->Name);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BufInfo->Position += StrSize;

  return EFI_SUCCESS;
}

/** Serialize the varstore data into the IFR buffer.

  Check for the type of varstore and call the appropriate function for
  serializing the varstore data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_UNSUPPORTED        Currently unsupported varstore type.
  @retval  EFI_INVALID_PARAMETER  Invalid varstore type.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeVarstore (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS        Status;
  DYN_HII_VARSTORE  *VarstoreNode;

  ASSERT (Hdr->Type == DynHiiNodeVarstore);

  VarstoreNode = (DYN_HII_VARSTORE *)Hdr;
  switch (VarstoreNode->VarstoreType) {
    case DynHiiVarstoreBuffer:
      return DynHiiSerializeVarstoreBuffer (Hdr, BufInfo);
      break;

    case DynHiiVarstoreEfi:
      Status = EFI_UNSUPPORTED;
      break;

    case DynHiiVarstoreNameValue:
      Status = EFI_UNSUPPORTED;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/** Insert the EFI_IFR_FORM opcode into the IFR buffer.

  Insert the EFI_IFR_FORM opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeForm (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8         *Buf;
  UINTN         FormSize;
  EFI_STATUS    Status;
  EFI_IFR_FORM  Form;
  DYN_HII_FORM  *FormNode;

  ASSERT (Hdr->Type == DynHiiNodeForm);

  FormNode = (DYN_HII_FORM *)Hdr;

  FormSize = sizeof (EFI_IFR_FORM);
  if (FormSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, FormSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                = BufInfo->Data + BufInfo->Position;
  Form.Header.OpCode = EFI_IFR_FORM_OP;
  Form.Header.Length = (UINT8)FormSize;
  Form.Header.Scope  = 1;

  Form.FormId    = FormNode->FormId;
  Form.FormTitle = FormNode->Title;

  CopyMem (Buf, &Form, sizeof (EFI_IFR_FORM));
  BufInfo->Position += sizeof (EFI_IFR_FORM);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_FORM_SET opcode into the IFR buffer.

  Insert the EFI_IFR_FORM_SET opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeFormset (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8             *Buf;
  UINT8             Idx;
  UINTN             FormsetSize;
  EFI_STATUS        Status;
  EFI_IFR_FORM_SET  Formset;
  DYN_HII_FORMSET   *FormsetNode;

  ASSERT (Hdr->Type == DynHiiNodeFormset);

  FormsetNode = (DYN_HII_FORMSET *)Hdr;

  ASSERT (FormsetNode->ClassGuidCount <= 3);

  FormsetSize = sizeof (EFI_IFR_FORM_SET) + FormsetNode->ClassGuidCount * sizeof (EFI_GUID);
  if (FormsetSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, FormsetSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                   = BufInfo->Data + BufInfo->Position;
  Formset.Header.OpCode = EFI_IFR_FORM_SET_OP;
  Formset.Header.Length = (UINT8)FormsetSize;
  Formset.Header.Scope  = 1;

  CopyMem (&Formset.Guid, &FormsetNode->FormsetGuid, sizeof (EFI_GUID));
  Formset.FormSetTitle = FormsetNode->Title;
  Formset.Help         = FormsetNode->Help;
  Formset.Flags        = FormsetNode->ClassGuidCount & 0x3;

  CopyMem (Buf, &Formset, sizeof (EFI_IFR_FORM_SET));
  BufInfo->Position += sizeof (EFI_IFR_FORM_SET);

  Buf = BufInfo->Data + BufInfo->Position;
  for (Idx = 0; Idx < Formset.Flags; Idx++) {
    CopyMem ((EFI_GUID *)Buf, &FormsetNode->ClassGuid[Idx], sizeof (EFI_GUID));
    Buf               += sizeof (EFI_GUID);
    BufInfo->Position += sizeof (EFI_GUID);
  }

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_CHECKBOX opcode into the IFR buffer.

  Insert the EFI_IFR_CHECKBOX opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeCheckbox (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                  *Buf;
  UINTN                  CheckboxSize;
  EFI_STATUS             Status;
  EFI_IFR_CHECKBOX       Checkbox;
  DYN_HII_QUESTION_DATA  *Data;
  DYN_HII_STATEMENT      *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  Data          = &StatementNode->Data;

  CheckboxSize = sizeof (EFI_IFR_CHECKBOX);
  if (CheckboxSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, CheckboxSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                    = BufInfo->Data + BufInfo->Position;
  Checkbox.Header.OpCode = EFI_IFR_CHECKBOX_OP;
  Checkbox.Header.Length = (UINT8)CheckboxSize;
  Checkbox.Header.Scope  = 1;

  Checkbox.Question = Data->QuestionHdr;
  Checkbox.Flags    = Data->Question.Checkbox.Flags;

  CopyMem (Buf, &Checkbox, sizeof (EFI_IFR_CHECKBOX));
  BufInfo->Position += sizeof (EFI_IFR_CHECKBOX);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_ONE_OF opcode into the IFR buffer.

  Insert the EFI_IFR_ONE_OF opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_INVALID_PARAMETER  Invalid data size.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeOneOf (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                  *Buf;
  UINTN                  OneOfSize;
  EFI_STATUS             Status;
  EFI_IFR_ONE_OF         OneOf;
  DYN_HII_ONE_OF_DATA    *OneOfData;
  DYN_HII_QUESTION_DATA  *Data;
  DYN_HII_STATEMENT      *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  Data          = &StatementNode->Data;
  OneOfData     = &Data->Question.OneOf;

  OneOfSize = sizeof (EFI_IFR_ONE_OF);
  if (OneOfSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, OneOfSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                 = BufInfo->Data + BufInfo->Position;
  OneOf.Header.OpCode = EFI_IFR_ONE_OF_OP;
  OneOf.Header.Length = (UINT8)OneOfSize;
  OneOf.Header.Scope  = 1;

  OneOf.Question = Data->QuestionHdr;
  OneOf.Flags    = OneOfData->Flags;

  switch (OneOf.Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      OneOf.data.u8.MinValue = (UINT8)OneOfData->MinValue;
      OneOf.data.u8.MaxValue = (UINT8)OneOfData->MaxValue;
      OneOf.data.u8.Step     = (UINT8)OneOfData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_2:
      OneOf.data.u16.MinValue = (UINT16)OneOfData->MinValue;
      OneOf.data.u16.MaxValue = (UINT16)OneOfData->MaxValue;
      OneOf.data.u16.Step     = (UINT16)OneOfData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_4:
      OneOf.data.u32.MinValue = (UINT32)OneOfData->MinValue;
      OneOf.data.u32.MaxValue = (UINT32)OneOfData->MaxValue;
      OneOf.data.u32.Step     = (UINT32)OneOfData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_8:
      OneOf.data.u64.MinValue = (UINT64)OneOfData->MinValue;
      OneOf.data.u64.MaxValue = (UINT64)OneOfData->MaxValue;
      OneOf.data.u64.Step     = (UINT64)OneOfData->Step;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  CopyMem (Buf, &OneOf, sizeof (EFI_IFR_ONE_OF));
  BufInfo->Position += sizeof (EFI_IFR_ONE_OF);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_NUMERIC opcode into the IFR buffer.

  Insert the EFI_IFR_NUMERIC opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_INVALID_PARAMETER  Invalid data size.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeNumeric (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                  *Buf;
  UINTN                  NumericSize;
  EFI_STATUS             Status;
  EFI_IFR_NUMERIC        Numeric;
  DYN_HII_NUMERIC_DATA   *NumericData;
  DYN_HII_QUESTION_DATA  *Data;
  DYN_HII_STATEMENT      *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  Data          = &StatementNode->Data;
  NumericData   = &Data->Question.Numeric;

  NumericSize = sizeof (EFI_IFR_NUMERIC);
  if (NumericSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, NumericSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf                   = BufInfo->Data + BufInfo->Position;
  Numeric.Header.OpCode = EFI_IFR_NUMERIC_OP;
  Numeric.Header.Length = (UINT8)NumericSize;
  Numeric.Header.Scope  = 1;

  Numeric.Question = Data->QuestionHdr;
  Numeric.Flags    = NumericData->Flags;

  switch (Numeric.Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1:
      Numeric.data.u8.MinValue = (UINT8)NumericData->MinValue;
      Numeric.data.u8.MaxValue = (UINT8)NumericData->MaxValue;
      Numeric.data.u8.Step     = (UINT8)NumericData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_2:
      Numeric.data.u16.MinValue = (UINT16)NumericData->MinValue;
      Numeric.data.u16.MaxValue = (UINT16)NumericData->MaxValue;
      Numeric.data.u16.Step     = (UINT16)NumericData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_4:
      Numeric.data.u32.MinValue = (UINT32)NumericData->MinValue;
      Numeric.data.u32.MaxValue = (UINT32)NumericData->MaxValue;
      Numeric.data.u32.Step     = (UINT32)NumericData->Step;
      break;

    case EFI_IFR_NUMERIC_SIZE_8:
      Numeric.data.u64.MinValue = (UINT64)NumericData->MinValue;
      Numeric.data.u64.MaxValue = (UINT64)NumericData->MaxValue;
      Numeric.data.u64.Step     = (UINT64)NumericData->Step;
      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  CopyMem (Buf, &Numeric, sizeof (EFI_IFR_NUMERIC));
  BufInfo->Position += sizeof (EFI_IFR_NUMERIC);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_REF opcode into the IFR buffer.

  Insert the EFI_IFR_REF opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeRef1 (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8              *Buf;
  UINTN              RefSize;
  EFI_STATUS         Status;
  EFI_IFR_REF        Ref;
  DYN_HII_REF_DATA   *RefData;
  DYN_HII_STATEMENT  *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  RefData       = &StatementNode->Data.Question.Ref;

  RefSize = sizeof (EFI_IFR_REF);
  if (RefSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, RefSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf               = BufInfo->Data + BufInfo->Position;
  Ref.Header.OpCode = EFI_IFR_REF_OP;
  Ref.Header.Length = (UINT8)RefSize;
  Ref.Header.Scope  = 0;

  Ref.Question = StatementNode->Data.QuestionHdr;
  Ref.FormId   = RefData->FormId;

  CopyMem (Buf, &Ref, sizeof (EFI_IFR_REF));
  BufInfo->Position += sizeof (EFI_IFR_REF);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_REF4 opcode into the IFR buffer.

  Insert the EFI_IFR_REF4 opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeRef4 (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8              *Buf;
  UINTN              RefSize;
  EFI_STATUS         Status;
  EFI_IFR_REF4       Ref;
  DYN_HII_REF_DATA   *RefData;
  DYN_HII_STATEMENT  *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  RefData       = &StatementNode->Data.Question.Ref;

  RefSize = sizeof (EFI_IFR_REF4);
  if (RefSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, RefSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf               = BufInfo->Data + BufInfo->Position;
  Ref.Header.OpCode = EFI_IFR_REF_OP;
  Ref.Header.Length = (UINT8)RefSize;
  Ref.Header.Scope  = 0;

  Ref.Question   = StatementNode->Data.QuestionHdr;
  Ref.FormId     = RefData->FormId;
  Ref.QuestionId = RefData->QuestionId;
  Ref.DevicePath = RefData->DevicePath;
  CopyMem (&Ref.FormSetId, &RefData->FormSetGuid, sizeof (EFI_GUID));

  CopyMem (Buf, &Ref, sizeof (EFI_IFR_REF4));
  BufInfo->Position += sizeof (EFI_IFR_REF4);

  return EFI_SUCCESS;
}

/** Serialize the cross-reference statement data into the IFR buffer.

  Check for the type of cross-reference statement and call the
  appropriate function for serializing the ref data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_UNSUPPORTED        Currently unsupported Ref type.
  @retval  EFI_INVALID_PARAMETER  Invalid Ref type.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeRef (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS         Status;
  DYN_HII_REF_DATA   *RefData;
  DYN_HII_STATEMENT  *StatementNode;

  StatementNode = (DYN_HII_STATEMENT *)Hdr;
  RefData       = &StatementNode->Data.Question.Ref;

  switch (RefData->RefType) {
    case DynHiiRefOp:
      return DynHiiSerializeRef1 (Hdr, BufInfo);
      break;

    case DynHiiRef2Op:
      Status = EFI_UNSUPPORTED;
      break;

    case DynHiiRef3Op:
      Status = EFI_UNSUPPORTED;
      break;

    case DynHiiRef4Op:
      return DynHiiSerializeRef4 (Hdr, BufInfo);
      break;

    case DynHiiRef5Op:
      Status = EFI_UNSUPPORTED;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/** Serialize the statement/question data into the IFR buffer.

  Check for the type of statement/question and call the appropriate
  function for serializing the data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_INVALID_PARAMETER  Invalid statement type.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeStatement (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS         Status;
  DYN_HII_STATEMENT  *StatementNode;

  ASSERT (Hdr->Type == DynHiiNodeStatement);

  StatementNode = (DYN_HII_STATEMENT *)Hdr;

  switch (StatementNode->QuestionType) {
    case DynHiiQtCheckbox:
      ASSERT (Hdr->Scope == TRUE);
      Status = DynHiiSerializeCheckbox (Hdr, BufInfo);
      break;

    case DynHiiQtNumeric:
      ASSERT (Hdr->Scope == TRUE);
      Status = DynHiiSerializeNumeric (Hdr, BufInfo);
      break;

    case DynHiiQtRef:
      ASSERT (Hdr->Scope == FALSE);
      Status = DynHiiSerializeRef (Hdr, BufInfo);
      break;

    case DynHiiQtOneOf:
      ASSERT (Hdr->Scope == TRUE);
      Status = DynHiiSerializeOneOf (Hdr, BufInfo);
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/** Insert the EFI_IFR_ONE_OF_OPTION opcode into the IFR buffer.

  Insert the EFI_IFR_ONE_OF_OPTION opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeOption (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                  *Buf;
  UINTN                  OptionSize;
  EFI_STATUS             Status;
  EFI_IFR_ONE_OF_OPTION  Option;
  DYN_HII_ONE_OF_OPTION  *OptionNode;

  OptionNode = (DYN_HII_ONE_OF_OPTION *)Hdr;

  OptionSize = sizeof (EFI_IFR_ONE_OF_OPTION);
  if (OptionSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, OptionSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf = BufInfo->Data + BufInfo->Position;

  Option.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
  Option.Header.Length = (UINT8)OptionSize;
  Option.Header.Scope  = 0;

  Option.Option = OptionNode->Text;
  Option.Flags  = OptionNode->Flags;
  Option.Type   = OptionNode->Type;
  Option.Value  = OptionNode->Value;

  CopyMem (Buf, &Option, sizeof (EFI_IFR_ONE_OF_OPTION));
  BufInfo->Position += sizeof (EFI_IFR_ONE_OF_OPTION);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_DEFAULT opcode into the IFR buffer.

  Insert the EFI_IFR_DEFAULT opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeDefault (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8            *Buf;
  UINTN            DefaultSize;
  EFI_STATUS       Status;
  EFI_IFR_DEFAULT  Default;
  DYN_HII_DEFAULT  *DefaultNode;

  DefaultNode = (DYN_HII_DEFAULT *)Hdr;

  DefaultSize = sizeof (EFI_IFR_DEFAULT);
  if (DefaultSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, DefaultSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf = BufInfo->Data + BufInfo->Position;

  Default.Header.OpCode = EFI_IFR_DEFAULT_OP;
  Default.Header.Length = (UINT8)DefaultSize;
  Default.Header.Scope  = 0;

  Default.DefaultId = DefaultNode->DefaultId;
  Default.Type      = DefaultNode->Type;
  Default.Value     = DefaultNode->Value;

  CopyMem (Buf, &Default, sizeof (EFI_IFR_DEFAULT));
  BufInfo->Position += sizeof (EFI_IFR_DEFAULT);

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_DEFAULTSTORE opcode into the IFR buffer.

  In absence of an explicit addition of a defaultstore, insert the implicit
  EFI_IFR_DEFAULTSTORE opcode data into the IFR buffer.

  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiAddImplicitDefaultstores (
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                 Idx;
  UINT8                 *Buf;
  UINT32                DefaultstoreSize;
  EFI_STATUS            Status;
  EFI_IFR_DEFAULTSTORE  Defaultstore;

  DefaultstoreSize = 2 * sizeof (EFI_IFR_DEFAULTSTORE);
  Status           = DynHiiIfrBufferSizeCheck (BufInfo, DefaultstoreSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DefaultstoreSize = sizeof (EFI_IFR_DEFAULTSTORE);
  if (DefaultstoreSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Buf = BufInfo->Data + BufInfo->Position;

  Defaultstore.Header.OpCode = EFI_IFR_DEFAULTSTORE_OP;
  Defaultstore.Header.Length = (UINT8)DefaultstoreSize;
  Defaultstore.Header.Scope  = 0;

  Defaultstore.DefaultName = 0x0;
  for (Idx = 0; Idx < 2; Idx++) {
    Defaultstore.DefaultId = Idx;

    CopyMem (Buf, &Defaultstore, sizeof (EFI_IFR_DEFAULTSTORE));
    Buf               += sizeof (EFI_IFR_DEFAULTSTORE);
    BufInfo->Position += sizeof (EFI_IFR_DEFAULTSTORE);
  }

  return EFI_SUCCESS;
}

/** Insert the EFI_IFR_DEFAULTSTORE opcode into the IFR buffer.

  Insert the EFI_IFR_DEFAULTSTORE opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
DynHiiSerializeDefaultstore (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8                 *Buf;
  UINTN                 DefaultstoreSize;
  EFI_STATUS            Status;
  EFI_IFR_DEFAULTSTORE  Defaultstore;
  DYN_HII_DEFAULTSTORE  *DefaultstoreNode;

  DefaultstoreNode = (DYN_HII_DEFAULTSTORE *)Hdr;

  DefaultstoreSize = sizeof (EFI_IFR_DEFAULTSTORE);
  if (DefaultstoreSize > MAX_HII_OPCODE_LENGTH) {
    return EFI_BAD_BUFFER_SIZE;
  }

  Status = DynHiiIfrBufferSizeCheck (BufInfo, DefaultstoreSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Buf = BufInfo->Data + BufInfo->Position;

  Defaultstore.Header.OpCode = EFI_IFR_DEFAULTSTORE_OP;
  Defaultstore.Header.Length = (UINT8)DefaultstoreSize;
  Defaultstore.Header.Scope  = 0;

  Defaultstore.DefaultName = DefaultstoreNode->DefaultName;
  Defaultstore.DefaultId   = DefaultstoreNode->DefaultId;

  CopyMem (Buf, &Defaultstore, sizeof (EFI_IFR_DEFAULTSTORE));
  BufInfo->Position += sizeof (EFI_IFR_DEFAULTSTORE);

  return EFI_SUCCESS;
}

/** Serialize the given HII node.

  Check for the type of the node, and call the appropriate function for
  inserting the opcode data into the IFR buffer.

  @param [in]  Hdr                Pointer to the node header.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS            The opcode was added successfully to the IFR
                                  buffer.
  @retval  EFI_UNSUPPORTED        The node is currently unsupported.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
static
EFI_STATUS
EFIAPI
DynHiiSerializeNode (
  IN  CONST    DYN_HII_NODE_HDR    *Hdr,
  IN           DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS  Status;

  switch (Hdr->Type) {
    case DynHiiNodeFormset:
      Status = DynHiiSerializeFormset (Hdr, BufInfo);
      break;

    case DynHiiNodeVarstore:
      Status = DynHiiSerializeVarstore (Hdr, BufInfo);
      break;

    case DynHiiNodeForm:
      Status = DynHiiSerializeForm (Hdr, BufInfo);
      break;

    case DynHiiNodeStatement:
      Status = DynHiiSerializeStatement (Hdr, BufInfo);
      break;

    case DynHiiNodeOption:
      Status = DynHiiSerializeOption (Hdr, BufInfo);
      break;

    case DynHiiNodeDefault:
      Status = DynHiiSerializeDefault (Hdr, BufInfo);
      break;

    case DynHiiNodeDefaultStore:
      Status = DynHiiSerializeDefaultstore (Hdr, BufInfo);
      break;

    default:
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}

/** Iterate the default list.

  Iterate through the default list, and for each node found, call the function
  to serialize the node data into the IFR buffer.

  @param [in]  DefaultList        List of default nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the serialization was successfull or any error code
                        returned by the serialization function.

**/
static
EFI_STATUS
DynHiiIterateDefaultList (
  IN  CONST LIST_ENTRY          *DefaultList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  LIST_ENTRY       *Link;
  EFI_STATUS       Status;
  DYN_HII_DEFAULT  *Default;

  Link = GetFirstNode (DefaultList);
  while (!IsNull (DefaultList, Link)) {
    Default = (DYN_HII_DEFAULT *)Link;

    Status = DynHiiSerializeNode (&Default->Hdr, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (DefaultList, Link);
  }

  return EFI_SUCCESS;
}

/** Iterate the option list.

  Iterate through the option list, and for each node found, call the function
  to serialize the node data into the IFR buffer.

  @param [in]  OptionList         List of option nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the serialization was successfull or any error code
                        returned by the serialization function.

**/
static
EFI_STATUS
DynHiiIterateOptionList (
  IN  CONST LIST_ENTRY          *OptionList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  LIST_ENTRY             *Link;
  EFI_STATUS             Status;
  DYN_HII_ONE_OF_OPTION  *Option;

  Link = GetFirstNode (OptionList);
  while (!IsNull (OptionList, Link)) {
    Option = (DYN_HII_ONE_OF_OPTION *)Link;

    Status = DynHiiSerializeNode (&Option->Hdr, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (OptionList, Link);
  }

  return EFI_SUCCESS;
}

/** Process the statement/question node.

  Serialize the statement/question node data by calling the corresponding
  function. Check for any question defaults, and process them if present.
  Finally, add the end opcode for the statement if the statement is scoped.

  @param [in]  Statement          Statement/Question to be processed.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the processing was successfull or any error code
                        returned by the serialization functions.

**/
static
EFI_STATUS
DynHiiProcessStatement (
  IN  CONST DYN_HII_STATEMENT   *Statement,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS  Status;

  Status = DynHiiSerializeNode (&Statement->Hdr, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsListEmpty (&Statement->OptionList)) {
    Status = DynHiiIterateOptionList (&Statement->OptionList, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (!IsListEmpty (&Statement->DefaultList)) {
    Status = DynHiiIterateDefaultList (&Statement->DefaultList, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return DynHiiInsertEndOp (&Statement->Hdr, BufInfo);
}

/** Iterate the statement list.

  Iterate through the statement list, and for each node found, call the
  function to serialize the process the node data.

  @param [in]  StatementList      List of statement nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the processing was successfull or any error code
                        returned by the called functions.

**/
static
EFI_STATUS
DynHiiIterateStatementList (
  IN  CONST LIST_ENTRY          *StatementList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Link;
  DYN_HII_STATEMENT  *Statement;

  Link = GetFirstNode (StatementList);
  while (!IsNull (StatementList, Link)) {
    Statement = (DYN_HII_STATEMENT *)Link;

    Status = DynHiiProcessStatement (Statement, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (StatementList, Link);
  }

  return EFI_SUCCESS;
}

/** Process the form node.

  Serialize the form node data by calling the corresponding function. Process
  the Statements under the form. Finally, add the end opcode for the form.

  @param [in]  Form               Pointer to the Form node.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the processing was successfull or any error code
                        returned by the called functions.

**/
static
EFI_STATUS
DynHiiProcessForm (
  IN  CONST DYN_HII_FORM        *Form,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS  Status;

  Status = DynHiiSerializeNode (&Form->Hdr, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiIterateStatementList (&Form->StatementList, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiInsertEndOp (&Form->Hdr, BufInfo);
}

/** Iterate the varstore list.

  Iterate through the varstore list, and for each node found, call the function
  to serialize the node data into the IFR buffer.

  @param [in]  VarstoreList       List of default nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the serialization was successfull or any error code
                        returned by the serialization function.

**/
static
EFI_STATUS
DynHiiIterateVarstoreList (
  IN  CONST LIST_ENTRY          *VarstoreList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *Link;
  DYN_HII_VARSTORE  *Varstore;

  Link = GetFirstNode (VarstoreList);
  while (!IsNull (VarstoreList, Link)) {
    Varstore = (DYN_HII_VARSTORE *)Link;

    Status = DynHiiSerializeNode (&Varstore->Hdr, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (VarstoreList, Link);
  }

  return EFI_SUCCESS;
}

/** Iterate the form list.

  Iterate through the form list, and for each node found, call the function
  to process the form.

  @param [in]  FormList           List of form nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the serialization was successfull or any error code
                        returned by the called functions.

**/
static
EFI_STATUS
DynHiiIterateFormList (
  IN  CONST LIST_ENTRY          *FormList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Link;
  DYN_HII_FORM  *Form;

  Link = GetFirstNode (FormList);
  while (!IsNull (FormList, Link)) {
    Form = (DYN_HII_FORM *)Link;

    Status = DynHiiProcessForm (Form, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (FormList, Link);
  }

  return EFI_SUCCESS;
}

/** Iterate the default store list.

  Iterate through the default store list, and for each node found, call the
  function to serialize the node into the IFR buffer.

  @param [in]  DefaultstoreList   List of default store nodes.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the serialization was successfull or any error code
                        returned by the called functions.

**/
static
EFI_STATUS
DynHiiIterateDefaultstoreList (
  IN  CONST LIST_ENTRY          *DefaultstoreList,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Link;
  DYN_HII_DEFAULTSTORE  *Defaultstore;

  Link = GetFirstNode (DefaultstoreList);
  while (!IsNull (DefaultstoreList, Link)) {
    Defaultstore = (DYN_HII_DEFAULTSTORE *)Link;

    Status = DynHiiSerializeNode (&Defaultstore->Hdr, BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Link = GetNextNode (DefaultstoreList, Link);
  }

  return EFI_SUCCESS;
}

/** Process the formset node.

  Serialize the formset node data by calling the corresponding function. Process
  the child nodes of the formset, like varstores, forms. Finally, insert the
  end opcode into the IFR buffer.

  @param [in]  Formset            Pointer to the formset node.
  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

  @retval  EFI_SUCCESS  If the processing was successfull or any error code
                        returned by the called functions.

**/
static
EFI_STATUS
DynHiiProcessFormset (
  IN  CONST DYN_HII_FORMSET     *Formset,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  EFI_STATUS  Status;

  Status = DynHiiSerializeNode (&Formset->Hdr, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!IsListEmpty (&Formset->DefaultstoreList)) {
    Status = DynHiiIterateDefaultstoreList (
               &Formset->DefaultstoreList,
               BufInfo
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    Status = DynHiiAddImplicitDefaultstores (BufInfo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = DynHiiIterateVarstoreList (&Formset->VarstoreList, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = DynHiiIterateFormList (&Formset->FormList, BufInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return DynHiiInsertEndOp (&Formset->Hdr, BufInfo);
}

/** Initialize the Form Package header.

  Initialize the EFI_HII_PACKAGE_HEADER for the forms package.

  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.
  @param [in]  Length             Length of the Forms package, including the
                                  preceding four bytes for array length.

**/
static
VOID
DynHiiInitFormPkgHdr (
  IN  DYN_HII_IFR_BUFFER  *BufInfo,
  IN  UINT32              Length
  )
{
  UINT8                   *PkgHdr;
  UINT32                  *Buf;
  EFI_HII_PACKAGE_HEADER  Hdr;

  Buf  = (UINT32 *)BufInfo->Data;
  *Buf = Length;

  PkgHdr     = BufInfo->Data + sizeof (UINT32);
  Hdr.Length = Length - sizeof (UINT32);
  Hdr.Type   = EFI_HII_PACKAGE_FORMS;

  CopyMem (PkgHdr, &Hdr, sizeof (EFI_HII_PACKAGE_HEADER));
}

/** Dump the contents of the IFR buffer.

  Useful for debugging.

  @param [in]  BufInfo            Pointer to the DYN_HII_IFR_BUFFER structure.

**/
static
VOID
DumpIfrBuffer (
  IN      DYN_HII_IFR_BUFFER  *BufInfo
  )
{
  UINT8   *Buf;
  UINT32  Idx;
  UINT32  Idx1;

  DEBUG ((
    DEBUG_VERBOSE,
    "INFO: %a: IfrBuf->Data => %p, "
    "IfrBuf->Size => 0x%x, IfrBuf->Position => 0x%x\n",
    __func__,
    BufInfo->Data,
    BufInfo->Size,
    BufInfo->Position
    ));

  DEBUG ((
    DEBUG_VERBOSE,
    "***** Generated IFR Byte Stream *****\n"
    ));

  DEBUG ((
    DEBUG_VERBOSE,
    "IfrByteStream[] = {\n"
    ));

  Buf = BufInfo->Data;
  for (Idx = 0; Idx < 4; Idx++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "0x%02x, ",
      *Buf++
      ));
  }

  DEBUG ((DEBUG_VERBOSE, "\n\n"));

  for (Idx = 0; Idx < 4; Idx++) {
    DEBUG ((
      DEBUG_VERBOSE,
      "0x%02x, ",
      *Buf++
      ));
  }

  DEBUG ((DEBUG_VERBOSE, "\n\n"));

  for (Idx = 0; Idx < BufInfo->Size - 8;) {
    for (Idx1 = 0; Idx < (BufInfo->Size - 8) && Idx1 < 16; Idx1++, Idx++) {
      DEBUG ((
        DEBUG_VERBOSE,
        "0x%02x, ",
        *Buf++
        ));
    }

    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "};\n\n"
    ));
}

/** Generate the IFR byte-stream for the Formset.

  Serialize the Formset and it's child nodes to generate an IFR byte-stream
  based Form package. This can then be added to the HII database through the
  HiiAddPackages() call.

  @param [in]  Formset          The formset hierarchy that needs to be
                                serialized.
  @param [out] IfrBuf           Pointer to the IFR buffer.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  passed through Data is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateFormPackage (
  IN  CONST DYN_HII_FORMSET     *Formset,
  OUT       DYN_HII_IFR_BUFFER  **IfrBuf
  )
{
  UINT8               *Buf;
  EFI_STATUS          Status;
  DYN_HII_IFR_BUFFER  *BufInfo;

  ASSERT (Formset != NULL);
  ASSERT (IfrBuf != NULL);

  Status = DynHiiInitIfrByteBuffer (&BufInfo);
  if (Status == EFI_OUT_OF_RESOURCES) {
    return Status;
  }

  Status = DynHiiProcessFormset (Formset, BufInfo);
  if (EFI_ERROR (Status)) {
    goto err_handle;
  } else {
    Buf = ReallocatePool (
            BufInfo->Size,
            BufInfo->Position,
            BufInfo->Data
            );
    if (Buf == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto err_handle;
    }

    BufInfo->Data = Buf;
    BufInfo->Size = BufInfo->Position;
    DynHiiInitFormPkgHdr (BufInfo, BufInfo->Size);
    *IfrBuf = BufInfo;
  }

  DumpIfrBuffer (BufInfo);

  return EFI_SUCCESS;

err_handle:
  if (BufInfo->Data != NULL) {
    FreePool (BufInfo->Data);
  }

  FreePool (BufInfo);
  BufInfo = NULL;

  return Status;
}

/** Free up all the memory used by a Form package.

  Free up all the memory that had been allocated for the Form package which
  keeps the serialized IFR byte-stream for the formset.

  @param [in]  IfrBuf           Pointer to the IFR buffer that needs to be freed.

**/
VOID
EFIAPI
DynHiiFreeFormPackage (
  IN  DYN_HII_IFR_BUFFER  *IfrBuf
  )
{
  if (IfrBuf == NULL) {
    return;
  }

  if (IfrBuf->Data != NULL) {
    FreePool (IfrBuf->Data);
  }

  FreePool (IfrBuf);
}
