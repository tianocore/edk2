/** @file
  Var Check Hii bin generation.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VarCheckHiiGen.h"

LIST_ENTRY mVarCheckHiiList = INITIALIZE_LIST_HEAD_VARIABLE (mVarCheckHiiList);

#define VAR_CHECK_HII_VARIABLE_NODE_SIGNATURE   SIGNATURE_32 ('V', 'C', 'H', 'V')

typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;
  VAR_CHECK_HII_VARIABLE_HEADER *HiiVariable;
  EFI_VARSTORE_ID               VarStoreId;

  VAR_CHECK_HII_QUESTION_HEADER **HiiQuestionArray;
} VAR_CHECK_HII_VARIABLE_NODE;

#define VAR_CHECK_HII_VARIABLE_FROM_LINK(a) CR (a, VAR_CHECK_HII_VARIABLE_NODE, Link, VAR_CHECK_HII_VARIABLE_NODE_SIGNATURE)

CHAR16 *mVarName = NULL;
UINTN  mMaxVarNameSize = 0;

#ifdef DUMP_HII_DATA
GLOBAL_REMOVE_IF_UNREFERENCED VAR_CHECK_HII_OPCODE_STRING   mIfrOpCodeStringTable[] = {
  {EFI_IFR_VARSTORE_OP,             "EFI_IFR_VARSTORE_OP"},
  {EFI_IFR_VARSTORE_EFI_OP,         "EFI_IFR_VARSTORE_EFI_OP"},
  {EFI_IFR_ONE_OF_OP,               "EFI_IFR_ONE_OF_OP"},
  {EFI_IFR_CHECKBOX_OP,             "EFI_IFR_CHECKBOX_OP"},
  {EFI_IFR_NUMERIC_OP,              "EFI_IFR_NUMERIC_OP"},
  {EFI_IFR_ORDERED_LIST_OP,         "EFI_IFR_ORDERED_LIST_OP"},
  {EFI_IFR_ONE_OF_OPTION_OP,        "EFI_IFR_ONE_OF_OPTION_OP"},
};

/**
  Ifr opcode to string.

  @param[in] IfrOpCode  Ifr OpCode.

  @return Pointer to string.

**/
CHAR8 *
IfrOpCodeToStr (
  IN UINT8  IfrOpCode
  )
{
  UINTN  Index;
  for (Index = 0; Index < ARRAY_SIZE (mIfrOpCodeStringTable); Index++) {
    if (mIfrOpCodeStringTable[Index].HiiOpCode == IfrOpCode) {
      return mIfrOpCodeStringTable[Index].HiiOpCodeStr;
    }
  }

  return "<UnknownIfrOpCode>";
}

GLOBAL_REMOVE_IF_UNREFERENCED VAR_CHECK_HII_PACKAGE_TYPE_STRING  mPackageTypeStringTable[] = {
  {EFI_HII_PACKAGE_TYPE_ALL,            "EFI_HII_PACKAGE_TYPE_ALL"},
  {EFI_HII_PACKAGE_TYPE_GUID,           "EFI_HII_PACKAGE_TYPE_GUID"},
  {EFI_HII_PACKAGE_FORMS,               "EFI_HII_PACKAGE_FORMS"},
  {EFI_HII_PACKAGE_STRINGS,             "EFI_HII_PACKAGE_STRINGS"},
  {EFI_HII_PACKAGE_FONTS,               "EFI_HII_PACKAGE_FONTS"},
  {EFI_HII_PACKAGE_IMAGES,              "EFI_HII_PACKAGE_IMAGES"},
  {EFI_HII_PACKAGE_SIMPLE_FONTS,        "EFI_HII_PACKAGE_SIMPLE_FONTS"},
  {EFI_HII_PACKAGE_DEVICE_PATH,         "EFI_HII_PACKAGE_DEVICE_PATH"},
  {EFI_HII_PACKAGE_KEYBOARD_LAYOUT,     "EFI_HII_PACKAGE_KEYBOARD_LAYOUT"},
  {EFI_HII_PACKAGE_ANIMATIONS,          "EFI_HII_PACKAGE_ANIMATIONS"},
  {EFI_HII_PACKAGE_END,                 "EFI_HII_PACKAGE_END"},
  {EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN,   "EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN"},
  {EFI_HII_PACKAGE_TYPE_SYSTEM_END,     "EFI_HII_PACKAGE_TYPE_SYSTEM_END"},
};

/**
  Hii Package type to string.

  @param[in] PackageType    Package Type

  @return Pointer to string.

**/
CHAR8 *
HiiPackageTypeToStr (
  IN UINT8  PackageType
  )
{
  UINTN     Index;
  for (Index = 0; Index < ARRAY_SIZE (mPackageTypeStringTable); Index++) {
    if (mPackageTypeStringTable[Index].PackageType == PackageType) {
      return mPackageTypeStringTable[Index].PackageTypeStr;
    }
  }

  return "<UnknownPackageType>";
}

/**
  Dump Hii Package.

  @param[in] HiiPackage         Pointer to Hii Package.

**/
VOID
DumpHiiPackage (
  IN VOID       *HiiPackage
  )
{
  EFI_HII_PACKAGE_HEADER        *HiiPackageHeader;
  EFI_IFR_OP_HEADER             *IfrOpCodeHeader;
  EFI_IFR_VARSTORE              *IfrVarStore;
  EFI_IFR_VARSTORE_EFI          *IfrEfiVarStore;
  BOOLEAN                       QuestionStoredInBitField;

  HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) HiiPackage;
  QuestionStoredInBitField = FALSE;

  DEBUG ((DEBUG_INFO, "  HiiPackageHeader->Type   - 0x%02x (%a)\n", HiiPackageHeader->Type, HiiPackageTypeToStr ((UINT8) HiiPackageHeader->Type)));
  DEBUG ((DEBUG_INFO, "  HiiPackageHeader->Length - 0x%06x\n", HiiPackageHeader->Length));

  switch (HiiPackageHeader->Type) {
    case EFI_HII_PACKAGE_FORMS:
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) (HiiPackageHeader + 1);

      while ((UINTN) IfrOpCodeHeader < ((UINTN) HiiPackageHeader + HiiPackageHeader->Length)) {
        switch (IfrOpCodeHeader->OpCode) {
          case EFI_IFR_VARSTORE_OP:
            IfrVarStore = (EFI_IFR_VARSTORE *) IfrOpCodeHeader;
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->OpCode - 0x%02x (%a)\n", IfrOpCodeHeader->OpCode, IfrOpCodeToStr (IfrOpCodeHeader->OpCode)));
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Length - 0x%02x\n", IfrOpCodeHeader->Length));
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Scope  - 0x%02x\n", IfrOpCodeHeader->Scope));
            DEBUG ((DEBUG_INFO, "      Guid       - %g\n", &IfrVarStore->Guid));
            DEBUG ((DEBUG_INFO, "      VarStoreId - 0x%04x\n", IfrVarStore->VarStoreId));
            DEBUG ((DEBUG_INFO, "      Size       - 0x%04x\n", IfrVarStore->Size));
            DEBUG ((DEBUG_INFO, "      Name       - %a\n", IfrVarStore->Name));
            break;

          case EFI_IFR_VARSTORE_EFI_OP:
            IfrEfiVarStore = (EFI_IFR_VARSTORE_EFI *) IfrOpCodeHeader;
            if (IfrEfiVarStore->Header.Length >= sizeof (EFI_IFR_VARSTORE_EFI)) {
              DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->OpCode - 0x%02x (%a)\n", IfrOpCodeHeader->OpCode, IfrOpCodeToStr (IfrOpCodeHeader->OpCode)));
              DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Length - 0x%02x\n", IfrOpCodeHeader->Length));
              DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Scope  - 0x%02x\n", IfrOpCodeHeader->Scope));
              DEBUG ((DEBUG_INFO, "      Guid       - %g\n", &IfrEfiVarStore->Guid));
              DEBUG ((DEBUG_INFO, "      VarStoreId - 0x%04x\n", IfrEfiVarStore->VarStoreId));
              DEBUG ((DEBUG_INFO, "      Size       - 0x%04x\n", IfrEfiVarStore->Size));
              DEBUG ((DEBUG_INFO, "      Attributes - 0x%08x\n", IfrEfiVarStore->Attributes));
              DEBUG ((DEBUG_INFO, "      Name       - %a\n", IfrEfiVarStore->Name));
            }
            break;

          case EFI_IFR_GUID_OP:
            if (CompareGuid ((EFI_GUID *)((UINTN)IfrOpCodeHeader + sizeof (EFI_IFR_OP_HEADER)), &gEdkiiIfrBitVarstoreGuid)) {
              QuestionStoredInBitField = TRUE;
            }
            break;

          case EFI_IFR_ONE_OF_OP:
          case EFI_IFR_CHECKBOX_OP:
          case EFI_IFR_NUMERIC_OP:
          case EFI_IFR_ORDERED_LIST_OP:
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->OpCode - 0x%02x (%a) (%a)\n", IfrOpCodeHeader->OpCode, IfrOpCodeToStr (IfrOpCodeHeader->OpCode), (QuestionStoredInBitField? "bit level": "byte level")));
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Length - 0x%02x\n", IfrOpCodeHeader->Length));
            DEBUG ((DEBUG_INFO, "    IfrOpCodeHeader->Scope  - 0x%02x\n", IfrOpCodeHeader->Scope));
            DEBUG ((DEBUG_INFO, "      Prompt       - 0x%04x\n", ((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.Header.Prompt));
            DEBUG ((DEBUG_INFO, "      Help         - 0x%04x\n", ((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.Header.Help));
            DEBUG ((DEBUG_INFO, "      QuestionId   - 0x%04x\n", ((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.QuestionId));
            DEBUG ((DEBUG_INFO, "      VarStoreId   - 0x%04x\n", ((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.VarStoreId));
            DEBUG ((DEBUG_INFO, "      VarStoreInfo - 0x%04x (%a)\n", ((EFI_IFR_ONE_OF * )IfrOpCodeHeader)->Question.VarStoreInfo.VarOffset, (QuestionStoredInBitField? "bit level": "byte level")));
            {
              EFI_IFR_ONE_OF            *IfrOneOf;
              EFI_IFR_CHECKBOX          *IfrCheckBox;
              EFI_IFR_NUMERIC           *IfrNumeric;
              EFI_IFR_ORDERED_LIST      *IfrOrderedList;

              switch (IfrOpCodeHeader->OpCode) {
                case EFI_IFR_ONE_OF_OP:
                  IfrOneOf = (EFI_IFR_ONE_OF *) IfrOpCodeHeader;
                  DEBUG ((DEBUG_INFO, "      Flags         - 0x%02x\n", IfrOneOf->Flags));
                  if (QuestionStoredInBitField) {
                    //
                    // For OneOf stored in bit field, the option value are saved as UINT32 type.
                    //
                    DEBUG ((DEBUG_INFO, "      MinValue      - 0x%08x\n", IfrOneOf->data.u32.MinValue));
                    DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%08x\n", IfrOneOf->data.u32.MaxValue));
                    DEBUG ((DEBUG_INFO, "      Step          - 0x%08x\n", IfrOneOf->data.u32.Step));
                  } else {
                    switch (IfrOneOf->Flags & EFI_IFR_NUMERIC_SIZE) {
                    case EFI_IFR_NUMERIC_SIZE_1:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%02x\n", IfrOneOf->data.u8.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%02x\n", IfrOneOf->data.u8.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%02x\n", IfrOneOf->data.u8.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_2:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%04x\n", IfrOneOf->data.u16.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%04x\n", IfrOneOf->data.u16.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%04x\n", IfrOneOf->data.u16.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_4:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%08x\n", IfrOneOf->data.u32.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%08x\n", IfrOneOf->data.u32.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%08x\n", IfrOneOf->data.u32.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_8:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%016lx\n", IfrOneOf->data.u64.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%016lx\n", IfrOneOf->data.u64.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%016lx\n", IfrOneOf->data.u64.Step));
                      break;
                    }
                  }
                  break;
                case EFI_IFR_CHECKBOX_OP:
                  IfrCheckBox = (EFI_IFR_CHECKBOX *) IfrOpCodeHeader;
                  DEBUG ((DEBUG_INFO, "      Flags         - 0x%02x\n", IfrCheckBox->Flags));
                  break;
                case EFI_IFR_NUMERIC_OP:
                  IfrNumeric = (EFI_IFR_NUMERIC *) IfrOpCodeHeader;
                  DEBUG ((DEBUG_INFO, "      Flags         - 0x%02x\n", IfrNumeric->Flags));
                  if (QuestionStoredInBitField) {
                    //
                    // For Numeric stored in bit field, the MinValue,MaxValue and Step are saved as UINT32 type.
                    //
                    DEBUG ((DEBUG_INFO, "      MinValue      - 0x%08x\n", IfrNumeric->data.u32.MinValue));
                    DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%08x\n", IfrNumeric->data.u32.MaxValue));
                    DEBUG ((DEBUG_INFO, "      Step          - 0x%08x\n", IfrNumeric->data.u32.Step));
                  } else {
                    switch (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE) {
                    case EFI_IFR_NUMERIC_SIZE_1:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%02x\n", IfrNumeric->data.u8.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%02x\n", IfrNumeric->data.u8.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%02x\n", IfrNumeric->data.u8.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_2:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%04x\n", IfrNumeric->data.u16.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%04x\n", IfrNumeric->data.u16.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%04x\n", IfrNumeric->data.u16.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_4:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%08x\n", IfrNumeric->data.u32.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%08x\n", IfrNumeric->data.u32.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%08x\n", IfrNumeric->data.u32.Step));
                      break;
                    case EFI_IFR_NUMERIC_SIZE_8:
                      DEBUG ((DEBUG_INFO, "      MinValue      - 0x%016lx\n", IfrNumeric->data.u64.MinValue));
                      DEBUG ((DEBUG_INFO, "      MaxValue      - 0x%016lx\n", IfrNumeric->data.u64.MaxValue));
                      DEBUG ((DEBUG_INFO, "      Step          - 0x%016lx\n", IfrNumeric->data.u64.Step));
                      break;
                    }
                  }
                  break;
                case EFI_IFR_ORDERED_LIST_OP:
                  IfrOrderedList = (EFI_IFR_ORDERED_LIST *) IfrOpCodeHeader;
                  DEBUG ((DEBUG_INFO, "      MaxContainers - 0x%02x\n", IfrOrderedList->MaxContainers));
                  DEBUG ((DEBUG_INFO, "      Flags         - 0x%02x\n", IfrOrderedList->Flags));
                  break;
                default:
                  break;
              }

              if (IfrOpCodeHeader->Scope != 0) {
                UINTN                       Scope;
                EFI_IFR_ONE_OF_OPTION       *IfrOneOfOption;

                IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
                Scope = 1;
                while (Scope != 0) {
                  switch (IfrOpCodeHeader->OpCode) {
                    case EFI_IFR_ONE_OF_OPTION_OP:
                      IfrOneOfOption = (EFI_IFR_ONE_OF_OPTION *)IfrOpCodeHeader;
                      DEBUG ((DEBUG_INFO, "!!!!    IfrOpCodeHeader->OpCode - 0x%02x (%a)\n", (UINTN)IfrOpCodeHeader->OpCode, IfrOpCodeToStr (IfrOpCodeHeader->OpCode)));
                      DEBUG ((DEBUG_INFO, "!!!!    IfrOpCodeHeader->Scope  - 0x%02x\n", IfrOpCodeHeader->Scope));
                      DEBUG ((DEBUG_INFO, "!!!!      Option                - 0x%04x\n", IfrOneOfOption->Option));
                      DEBUG ((DEBUG_INFO, "!!!!      Flags                 - 0x%02x\n", IfrOneOfOption->Flags));
                      DEBUG ((DEBUG_INFO, "!!!!      Type                  - 0x%02x\n", IfrOneOfOption->Type));
                      switch (IfrOneOfOption->Type) {
                        case EFI_IFR_TYPE_NUM_SIZE_8:
                          DEBUG ((DEBUG_INFO, "!!!!      Value                 - 0x%02x\n", IfrOneOfOption->Value.u8));
                          break;
                        case EFI_IFR_TYPE_NUM_SIZE_16:
                          DEBUG ((DEBUG_INFO, "!!!!      Value                 - 0x%04x\n", IfrOneOfOption->Value.u16));
                          break;
                        case EFI_IFR_TYPE_NUM_SIZE_32:
                          DEBUG ((DEBUG_INFO, "!!!!      Value                 - 0x%08x\n", IfrOneOfOption->Value.u32));
                          break;
                        case EFI_IFR_TYPE_NUM_SIZE_64:
                          DEBUG ((DEBUG_INFO, "!!!!      Value                 - 0x%016lx\n", IfrOneOfOption->Value.u64));
                          break;
                        case EFI_IFR_TYPE_BOOLEAN:
                          DEBUG ((DEBUG_INFO, "!!!!      Value                 - 0x%02x\n", IfrOneOfOption->Value.b));
                          break;
                        default:
                          break;
                      }
                      break;
                  }

                  if (IfrOpCodeHeader->OpCode == EFI_IFR_END_OP) {
                    QuestionStoredInBitField = FALSE;
                    ASSERT (Scope > 0);
                    Scope--;
                    if (Scope == 0) {
                      break;
                    }
                  } else if (IfrOpCodeHeader->Scope != 0) {
                    Scope++;
                  }
                  IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
                }
              }
            }
          default:
            break;
        }
        IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
      }
      break;
    default:
      break;
  }
}

/**
  Dump Hii Database.

  @param[in] HiiDatabase        Pointer to Hii Database.
  @param[in] HiiDatabaseSize    Hii Database size.

**/
VOID
DumpHiiDatabase (
  IN VOID       *HiiDatabase,
  IN UINTN      HiiDatabaseSize
  )
{
  EFI_HII_PACKAGE_LIST_HEADER   *HiiPackageListHeader;
  EFI_HII_PACKAGE_HEADER        *HiiPackageHeader;

  DEBUG ((DEBUG_INFO, "HiiDatabaseSize - 0x%x\n", HiiDatabaseSize));
  HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) HiiDatabase;

  while ((UINTN) HiiPackageListHeader < ((UINTN) HiiDatabase + HiiDatabaseSize)) {
    DEBUG ((DEBUG_INFO, "HiiPackageListHeader->PackageListGuid - %g\n", &HiiPackageListHeader->PackageListGuid));
    DEBUG ((DEBUG_INFO, "HiiPackageListHeader->PackageLength   - 0x%x\n", (UINTN)HiiPackageListHeader->PackageLength));
    HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *)(HiiPackageListHeader + 1);

    while ((UINTN) HiiPackageHeader < (UINTN) HiiPackageListHeader + HiiPackageListHeader->PackageLength) {

      DumpHiiPackage (HiiPackageHeader);

      HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINTN) HiiPackageHeader + HiiPackageHeader->Length);
    }

    HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) ((UINTN) HiiPackageListHeader + HiiPackageListHeader->PackageLength);
  }

  return ;
}
#endif

/**
  Allocates a buffer of a certain pool type.

  Allocates the number bytes specified by AllocationSize of a certain pool type and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  MemoryType            The type of memory to allocate.
  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalVarCheckAllocatePool (
  IN EFI_MEMORY_TYPE  MemoryType,
  IN UINTN            AllocationSize
  )
{
  EFI_STATUS  Status;
  VOID        *Memory;

  Status = gBS->AllocatePool (MemoryType, AllocationSize, &Memory);
  if (EFI_ERROR (Status)) {
    Memory = NULL;
  }
  return Memory;
}

/**
  Allocates and zeros a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData, clears the
  buffer with zeros, and returns a pointer to the allocated buffer.  If AllocationSize is 0, then a
  valid buffer of 0 size is returned.  If there is not enough memory remaining to satisfy the
  request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate and zero.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalVarCheckAllocateZeroPool (
  IN UINTN            AllocationSize
  )
{
  VOID  *Memory;

  Memory = InternalVarCheckAllocatePool (EfiBootServicesData, AllocationSize);
  if (Memory != NULL) {
    Memory = ZeroMem (Memory, AllocationSize);
  }
  return Memory;
}

/**
  Frees a buffer that was previously allocated with one of the pool allocation functions in the
  Memory Allocation Library.

  Frees the buffer specified by Buffer.  Buffer must have been allocated on a previous call to the
  pool allocation services of the Memory Allocation Library.  If it is not possible to free pool
  resources, then this function will perform no actions.

  If Buffer was not allocated with a pool allocation function in the Memory Allocation Library,
  then ASSERT().

  @param  Buffer                The pointer to the buffer to free.

**/
VOID
EFIAPI
InternalVarCheckFreePool (
  IN VOID   *Buffer
  )
{
  EFI_STATUS    Status;

  Status = gBS->FreePool (Buffer);
  ASSERT_EFI_ERROR (Status);
}

/**
  Reallocates a buffer of type EfiBootServicesData.

  Allocates and zeros the number bytes specified by NewSize from memory of type
  EfiBootServicesData.  If OldBuffer is not NULL, then the smaller of OldSize and
  NewSize bytes are copied from OldBuffer to the newly allocated buffer, and
  OldBuffer is freed.  A pointer to the newly allocated buffer is returned.
  If NewSize is 0, then a valid buffer of 0 size is  returned.  If there is not
  enough memory remaining to satisfy the request, then NULL is returned.

  If the allocation of the new buffer is successful and the smaller of NewSize and OldSize
  is greater than (MAX_ADDRESS - OldBuffer + 1), then ASSERT().

  @param  OldSize        The size, in bytes, of OldBuffer.
  @param  NewSize        The size, in bytes, of the buffer to reallocate.
  @param  OldBuffer      The buffer to copy to the allocated buffer.  This is an optional
                         parameter that may be NULL.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
InternalVarCheckReallocatePool (
  IN UINTN            OldSize,
  IN UINTN            NewSize,
  IN VOID             *OldBuffer  OPTIONAL
  )
{
  VOID  *NewBuffer;

  NewBuffer = InternalVarCheckAllocateZeroPool (NewSize);
  if (NewBuffer != NULL && OldBuffer != NULL) {
    CopyMem (NewBuffer, OldBuffer, MIN (OldSize, NewSize));
    InternalVarCheckFreePool (OldBuffer);
  }
  return NewBuffer;
}

/**
  Merge Hii Question.

  @param[in, out] HiiVariableNode   Pointer to Hii Variable node.
  @param[in]      HiiQuestion       Pointer to Hii Question.
  @param[in]      FromFv            Hii Question from FV.

**/
VOID
MergeHiiQuestion (
  IN OUT VAR_CHECK_HII_VARIABLE_NODE    *HiiVariableNode,
  IN VAR_CHECK_HII_QUESTION_HEADER      *HiiQuestion,
  IN BOOLEAN                            FromFv
  )
{
  VAR_CHECK_HII_QUESTION_HEADER     *HiiQuestion1;
  VAR_CHECK_HII_QUESTION_HEADER     *HiiQuestion2;
  VAR_CHECK_HII_QUESTION_HEADER     *NewHiiQuestion;
  UINT8                             NewLength;
  UINT64                            Minimum1;
  UINT64                            Maximum1;
  UINT64                            OneValue1;
  UINT64                            Minimum2;
  UINT64                            Maximum2;
  UINT64                            OneValue2;
  UINT8                             *Ptr;
  UINT8                             *Ptr1;
  UINT8                             *Ptr2;
  UINTN                             ArrayIndex;

  //
  // Hii Question from Hii Database has high priority.
  // Do not to merge Hii Question from Fv to Hii Question from Hii Database.
  //
  if (FromFv) {
    InternalVarCheckFreePool (HiiQuestion);
    return;
  }

  if (HiiQuestion->BitFieldStore) {
    ArrayIndex = HiiQuestion->VarOffset;
  } else {
    ArrayIndex = HiiQuestion->VarOffset * 8;
  }

  HiiQuestion1 = HiiVariableNode->HiiQuestionArray[ArrayIndex];
  HiiQuestion2 = HiiQuestion;

  ASSERT ((HiiQuestion1->OpCode == HiiQuestion2->OpCode) && (HiiQuestion1->StorageWidth == HiiQuestion2->StorageWidth));

  switch (HiiQuestion1->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      DEBUG ((DEBUG_INFO, "MergeHiiQuestion - EFI_IFR_ONE_OF_OP VarOffset = 0x%04x (%a)\n", HiiQuestion1->VarOffset, (HiiQuestion1->BitFieldStore? "bit level": "byte level")));
      //
      // Get the length of Hii Question 1.
      //
      NewLength = HiiQuestion1->Length;

      //
      // Check if the one of options in Hii Question 2 have been in Hii Question 1.
      //
      Ptr2 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion2 + 1);
      while ((UINTN) Ptr2 < (UINTN) HiiQuestion2 + HiiQuestion2->Length) {
        OneValue2 = 0;
        CopyMem (&OneValue2, Ptr2, HiiQuestion2->StorageWidth);

        Ptr1 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion1 + 1);
        while ((UINTN) Ptr1 < (UINTN) HiiQuestion1 + HiiQuestion1->Length) {
          OneValue1 = 0;
          CopyMem (&OneValue1, Ptr1, HiiQuestion1->StorageWidth);
          if (OneValue2 == OneValue1) {
            //
            // Match
            //
            break;
          }
          Ptr1 += HiiQuestion1->StorageWidth;
        }
        if ((UINTN) Ptr1 >= ((UINTN) HiiQuestion1 + HiiQuestion1->Length)) {
          //
          // No match
          //
          NewLength = (UINT8) (NewLength + HiiQuestion1->StorageWidth);
        }
        Ptr2 += HiiQuestion2->StorageWidth;
      }

      if (NewLength > HiiQuestion1->Length) {
        //
        // Merge the one of options of Hii Question 2 and Hii Question 1.
        //
        NewHiiQuestion = InternalVarCheckAllocateZeroPool (NewLength);
        ASSERT (NewHiiQuestion != NULL);
        CopyMem (NewHiiQuestion, HiiQuestion1, HiiQuestion1->Length);
        //
        // Use the new length.
        //
        NewHiiQuestion->Length = NewLength;
        Ptr = (UINT8 *) NewHiiQuestion + HiiQuestion1->Length;

        Ptr2 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion2 + 1);
        while ((UINTN) Ptr2 < (UINTN) HiiQuestion2 + HiiQuestion2->Length) {
          OneValue2 = 0;
          CopyMem (&OneValue2, Ptr2, HiiQuestion2->StorageWidth);

          Ptr1 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion1 + 1);
          while ((UINTN) Ptr1 < (UINTN) HiiQuestion1 + HiiQuestion1->Length) {
            OneValue1 = 0;
            CopyMem (&OneValue1, Ptr1, HiiQuestion1->StorageWidth);
            if (OneValue2 == OneValue1) {
              //
              // Match
              //
              break;
            }
            Ptr1 += HiiQuestion1->StorageWidth;
          }
          if ((UINTN) Ptr1 >= ((UINTN) HiiQuestion1 + HiiQuestion1->Length)) {
            //
            // No match
            //
            CopyMem (Ptr, &OneValue2, HiiQuestion1->StorageWidth);
            Ptr += HiiQuestion1->StorageWidth;
          }
          Ptr2 += HiiQuestion2->StorageWidth;
        }

        HiiVariableNode->HiiQuestionArray[ArrayIndex] = NewHiiQuestion;
        InternalVarCheckFreePool (HiiQuestion1);
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      DEBUG ((DEBUG_INFO, "MergeHiiQuestion - EFI_IFR_CHECKBOX_OP VarOffset = 0x%04x (%a)\n", HiiQuestion1->VarOffset, (HiiQuestion1->BitFieldStore? "bit level": "byte level")));
      break;

    case EFI_IFR_NUMERIC_OP:
      DEBUG ((DEBUG_INFO, "MergeHiiQuestion - EFI_IFR_NUMERIC_OP VarOffset = 0x%04x (%a)\n", HiiQuestion1->VarOffset, (HiiQuestion1->BitFieldStore? "bit level": "byte level")));
      //
      // Get minimum and maximum of Hii Question 1.
      //
      Minimum1 = 0;
      Maximum1 = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_NUMERIC *) HiiQuestion1 + 1);
      CopyMem (&Minimum1, Ptr, HiiQuestion1->StorageWidth);
      Ptr += HiiQuestion1->StorageWidth;
      CopyMem (&Maximum1, Ptr, HiiQuestion1->StorageWidth);

      //
      // Get minimum and maximum of Hii Question 2.
      //
      Minimum2 = 0;
      Maximum2 = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_NUMERIC *) HiiQuestion2 + 1);
      CopyMem (&Minimum2, Ptr, HiiQuestion2->StorageWidth);
      Ptr += HiiQuestion2->StorageWidth;
      CopyMem (&Maximum2, Ptr, HiiQuestion2->StorageWidth);

      //
      // Update minimum.
      //
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_NUMERIC *) HiiQuestion1 + 1);
      if (Minimum2 < Minimum1) {
        Minimum1 = Minimum2;
        CopyMem (Ptr, &Minimum1, HiiQuestion1->StorageWidth);
      }
      //
      // Update maximum.
      //
      Ptr += HiiQuestion1->StorageWidth;
      if (Maximum2 > Maximum1) {
        Maximum1 = Maximum2;
        CopyMem (Ptr, &Maximum1, HiiQuestion1->StorageWidth);
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      DEBUG ((DEBUG_INFO, "MergeHiiQuestion - EFI_IFR_ORDERED_LIST_OP VarOffset = 0x%04x\n", HiiQuestion1->VarOffset));
      //
      // Get the length of Hii Question 1.
      //
      NewLength = HiiQuestion1->Length;

      //
      // Check if the one of options in Hii Question 2 have been in Hii Question 1.
      //
      Ptr2 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion2 + 1);
      while ((UINTN) Ptr2 < (UINTN) HiiQuestion2 + HiiQuestion2->Length) {
        OneValue2 = 0;
        CopyMem (&OneValue2, Ptr2, HiiQuestion2->StorageWidth);

        Ptr1 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion1 + 1);
        while ((UINTN) Ptr1 < (UINTN) HiiQuestion1 + HiiQuestion1->Length) {
          OneValue1 = 0;
          CopyMem (&OneValue1, Ptr1, HiiQuestion1->StorageWidth);
          if (OneValue2 == OneValue1) {
            //
            // Match
            //
            break;
          }
          Ptr1 += HiiQuestion1->StorageWidth;
        }
        if ((UINTN) Ptr1 >= ((UINTN) HiiQuestion1 + HiiQuestion1->Length)) {
          //
          // No match
          //
          NewLength = (UINT8) (NewLength + HiiQuestion1->StorageWidth);
        }
        Ptr2 += HiiQuestion2->StorageWidth;
      }

      if (NewLength > HiiQuestion1->Length) {
        //
        // Merge the one of options of Hii Question 2 and Hii Question 1.
        //
        NewHiiQuestion = InternalVarCheckAllocateZeroPool (NewLength);
        ASSERT (NewHiiQuestion != NULL);
        CopyMem (NewHiiQuestion, HiiQuestion1, HiiQuestion1->Length);
        //
        // Use the new length.
        //
        NewHiiQuestion->Length = NewLength;
        Ptr = (UINT8 *) NewHiiQuestion + HiiQuestion1->Length;

        Ptr2 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion2 + 1);
        while ((UINTN) Ptr2 < (UINTN) HiiQuestion2 + HiiQuestion2->Length) {
          OneValue2 = 0;
          CopyMem (&OneValue2, Ptr2, HiiQuestion2->StorageWidth);

          Ptr1 = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion1 + 1);
          while ((UINTN) Ptr1 < (UINTN) HiiQuestion1 + HiiQuestion1->Length) {
            OneValue1 = 0;
            CopyMem (&OneValue1, Ptr1, HiiQuestion1->StorageWidth);
            if (OneValue2 == OneValue1) {
              //
              // Match
              //
              break;
            }
            Ptr1 += HiiQuestion1->StorageWidth;
          }
          if ((UINTN) Ptr1 >= ((UINTN) HiiQuestion1 + HiiQuestion1->Length)) {
            //
            // No match
            //
            CopyMem (Ptr, &OneValue2, HiiQuestion1->StorageWidth);
            Ptr += HiiQuestion1->StorageWidth;
          }
          Ptr2 += HiiQuestion2->StorageWidth;
        }

        HiiVariableNode->HiiQuestionArray[ArrayIndex] = NewHiiQuestion;
        InternalVarCheckFreePool (HiiQuestion1);
      }
      break;

    default:
      ASSERT (FALSE);
      return;
      break;
  }

  //
  //
  // Hii Question 2 has been merged with Hii Question 1.
  //
  InternalVarCheckFreePool (HiiQuestion2);
}

/**
  Get OneOf option data.

  @param[in]  IfrOpCodeHeader   Pointer to Ifr OpCode header.
  @param[out] Count             Pointer to option count.
  @param[out] Width             Pointer to option width.
  @param[out] OptionBuffer      Pointer to option buffer.

**/
VOID
GetOneOfOption (
  IN  EFI_IFR_OP_HEADER     *IfrOpCodeHeader,
  OUT UINTN                 *Count,
  OUT UINT8                 *Width,
  OUT VOID                  *OptionBuffer OPTIONAL
  )
{
  UINTN                     Scope;
  EFI_IFR_ONE_OF_OPTION     *IfrOneOfOption;

  //
  // Assume all OPTION has same Width.
  //
  *Count = 0;

  if (IfrOpCodeHeader->Scope != 0) {
    //
    // Nested OpCode.
    //
    Scope = 1;
    IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
    while (Scope != 0) {
      switch (IfrOpCodeHeader->OpCode) {
        case EFI_IFR_ONE_OF_OPTION_OP:
          IfrOneOfOption = (EFI_IFR_ONE_OF_OPTION *) IfrOpCodeHeader;
          switch (IfrOneOfOption->Type) {
            case EFI_IFR_TYPE_NUM_SIZE_8:
              *Count = *Count + 1;
              *Width = sizeof (UINT8);
              if (OptionBuffer != NULL) {
                CopyMem (OptionBuffer, &IfrOneOfOption->Value.u8, sizeof (UINT8));
                OptionBuffer = (UINT8 *) OptionBuffer + 1;
              }
              break;
            case EFI_IFR_TYPE_NUM_SIZE_16:
              *Count = *Count + 1;
              *Width = sizeof (UINT16);
              if (OptionBuffer != NULL) {
                CopyMem (OptionBuffer, &IfrOneOfOption->Value.u16, sizeof (UINT16));
                OptionBuffer = (UINT16 *) OptionBuffer + 1;
              }
              break;
            case EFI_IFR_TYPE_NUM_SIZE_32:
              *Count = *Count + 1;
              *Width = sizeof (UINT32);
              if (OptionBuffer != NULL) {
                CopyMem (OptionBuffer, &IfrOneOfOption->Value.u32, sizeof (UINT32));
                OptionBuffer = (UINT32 *) OptionBuffer + 1;
              }
              break;
            case EFI_IFR_TYPE_NUM_SIZE_64:
              *Count = *Count + 1;
              *Width = sizeof (UINT64);
              if (OptionBuffer != NULL) {
                CopyMem (OptionBuffer, &IfrOneOfOption->Value.u64, sizeof (UINT64));
                OptionBuffer = (UINT64 *) OptionBuffer + 1;
              }
              break;
            case EFI_IFR_TYPE_BOOLEAN:
              *Count = *Count + 1;
              *Width = sizeof (BOOLEAN);
              if (OptionBuffer != NULL) {
                CopyMem (OptionBuffer, &IfrOneOfOption->Value.b, sizeof (BOOLEAN));
                OptionBuffer = (BOOLEAN *) OptionBuffer + 1;
              }
              break;
            default:
              break;
          }
          break;
      }

      //
      // Until End OpCode.
      //
      if (IfrOpCodeHeader->OpCode == EFI_IFR_END_OP) {
        ASSERT (Scope > 0);
        Scope--;
        if (Scope == 0) {
          break;
        }
      } else if (IfrOpCodeHeader->Scope != 0) {
        //
        // Nested OpCode.
        //
        Scope++;
      }
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
    }
  }

  return ;
}

/**
  Parse Hii Question Oneof.

  @param[in] IfrOpCodeHeader    Pointer to Ifr OpCode header.
  @param[in] StoredInBitField   Whether the OneOf is stored in bit field Storage.

  return Pointer to Hii Question.

**/
VAR_CHECK_HII_QUESTION_HEADER *
ParseHiiQuestionOneOf (
  IN EFI_IFR_OP_HEADER  *IfrOpCodeHeader,
  IN BOOLEAN            StoredInBitField
  )
{
  EFI_IFR_ONE_OF                *IfrOneOf;
  VAR_CHECK_HII_QUESTION_ONEOF  *OneOf;
  UINTN                         Length;
  UINT8                         Width;
  UINTN                         OptionCount;
  UINT8                         OptionWidth;
  UINT8                         BitWidth;

  IfrOneOf = (EFI_IFR_ONE_OF *) IfrOpCodeHeader;
  BitWidth = 0;

  if (StoredInBitField) {
    //
    // When OneOf stored in bit field, the bit width is saved in the lower six bits of the flag.
    // And the options in the OneOf is saved as UINT32 type.
    //
    BitWidth = IfrOneOf->Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
    Width = sizeof (UINT32);
  } else {
    Width = (UINT8) (1 << (IfrOneOf->Flags & EFI_IFR_NUMERIC_SIZE));
  }

  GetOneOfOption (IfrOpCodeHeader, &OptionCount, &OptionWidth, NULL);
  ASSERT (Width == OptionWidth);

  Length = sizeof (*OneOf) + OptionCount * Width;

  OneOf = InternalVarCheckAllocateZeroPool (Length);
  ASSERT (OneOf != NULL);
  OneOf->OpCode         = EFI_IFR_ONE_OF_OP;
  OneOf->Length         = (UINT8) Length;
  OneOf->VarOffset      = IfrOneOf->Question.VarStoreInfo.VarOffset;
  OneOf->BitFieldStore  = StoredInBitField;
  if (StoredInBitField) {
    OneOf->StorageWidth = BitWidth;
  } else {
    OneOf->StorageWidth = Width;
  }

  GetOneOfOption (IfrOpCodeHeader, &OptionCount, &OptionWidth, OneOf + 1);

  return (VAR_CHECK_HII_QUESTION_HEADER *) OneOf;
}

/**
  Parse Hii Question CheckBox.

  @param[in] IfrOpCodeHeader    Pointer to Ifr OpCode header.
  @param[in] StoredInBitField   Whether the CheckBox is stored in bit field Storage.

  return Pointer to Hii Question.

**/
VAR_CHECK_HII_QUESTION_HEADER *
ParseHiiQuestionCheckBox (
  IN EFI_IFR_OP_HEADER  *IfrOpCodeHeader,
  IN BOOLEAN            StoredInBitField
  )
{
  EFI_IFR_CHECKBOX                  *IfrCheckBox;
  VAR_CHECK_HII_QUESTION_CHECKBOX   *CheckBox;

  IfrCheckBox = (EFI_IFR_CHECKBOX *) IfrOpCodeHeader;

  CheckBox = InternalVarCheckAllocateZeroPool (sizeof (*CheckBox));
  ASSERT (CheckBox != NULL);
  CheckBox->OpCode         = EFI_IFR_CHECKBOX_OP;
  CheckBox->Length         = (UINT8) sizeof (*CheckBox);;
  CheckBox->VarOffset      = IfrCheckBox->Question.VarStoreInfo.VarOffset;
  CheckBox->BitFieldStore  = StoredInBitField;
  if (StoredInBitField) {
    CheckBox->StorageWidth = 1;
  } else {
    CheckBox->StorageWidth = (UINT8) sizeof (BOOLEAN);
  }

  return (VAR_CHECK_HII_QUESTION_HEADER *) CheckBox;
}

/**
  Parse Hii Question Numeric.

  @param[in] IfrOpCodeHeader    Pointer to Ifr OpCode header.
  @param[in] StoredInBitField   Whether the Numeric is stored in bit field Storage.

  return Pointer to Hii Question.

**/
VAR_CHECK_HII_QUESTION_HEADER *
ParseHiiQuestionNumeric (
  IN EFI_IFR_OP_HEADER  *IfrOpCodeHeader,
  IN BOOLEAN            StoredInBitField
  )
{
  EFI_IFR_NUMERIC                   *IfrNumeric;
  VAR_CHECK_HII_QUESTION_NUMERIC    *Numeric;
  UINT8                             Width;
  UINT8                             BitWidth;

  IfrNumeric = (EFI_IFR_NUMERIC *) IfrOpCodeHeader;
  BitWidth = 0;

  Numeric = InternalVarCheckAllocateZeroPool (sizeof (VAR_CHECK_HII_QUESTION_NUMERIC) + 2 * sizeof (UINT64));
  ASSERT (Numeric != NULL);

  if (StoredInBitField) {
    //
    // When Numeric stored in bit field, the bit field width is saved in the lower six bits of the flag.
    // And the Minimum Maximum of Numeric is saved as UINT32 type.
    //
    BitWidth = IfrNumeric->Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
    Width = sizeof (UINT32);
  } else {
    Width = (UINT8) (1 << (IfrNumeric->Flags & EFI_IFR_NUMERIC_SIZE));
  }

  Numeric->OpCode         = EFI_IFR_NUMERIC_OP;
  Numeric->Length         = (UINT8) (sizeof (VAR_CHECK_HII_QUESTION_NUMERIC) + 2 * Width);
  Numeric->VarOffset      = IfrNumeric->Question.VarStoreInfo.VarOffset;
  Numeric->BitFieldStore  = StoredInBitField;
  if (StoredInBitField) {
    Numeric->StorageWidth = BitWidth;
  } else {
    Numeric->StorageWidth = Width;
  }

  CopyMem (Numeric + 1, &IfrNumeric->data, Width * 2);

  return (VAR_CHECK_HII_QUESTION_HEADER *) Numeric;
}

/**
  Parse Hii Question OrderedList.

  @param[in] IfrOpCodeHeader    Pointer to Ifr OpCode header.

  return Pointer to Hii Question.

**/
VAR_CHECK_HII_QUESTION_HEADER *
ParseHiiQuestionOrderedList (
  IN EFI_IFR_OP_HEADER  *IfrOpCodeHeader
  )
{
  EFI_IFR_ORDERED_LIST                  *IfrOrderedList;
  VAR_CHECK_HII_QUESTION_ORDEREDLIST    *OrderedList;
  UINTN                                 Length;
  UINTN                                 OptionCount;
  UINT8                                 OptionWidth;

  IfrOrderedList = (EFI_IFR_ORDERED_LIST *) IfrOpCodeHeader;

  GetOneOfOption (IfrOpCodeHeader, &OptionCount, &OptionWidth, NULL);

  Length = sizeof (*OrderedList) + OptionCount * OptionWidth;

  OrderedList = InternalVarCheckAllocateZeroPool (Length);
  ASSERT (OrderedList != NULL);
  OrderedList->OpCode        = EFI_IFR_ORDERED_LIST_OP;
  OrderedList->Length        = (UINT8) Length;
  OrderedList->VarOffset     = IfrOrderedList->Question.VarStoreInfo.VarOffset;
  OrderedList->StorageWidth  = OptionWidth;
  OrderedList->MaxContainers = IfrOrderedList->MaxContainers;
  OrderedList->BitFieldStore = FALSE;

  GetOneOfOption (IfrOpCodeHeader, &OptionCount, &OptionWidth, OrderedList + 1);

  return (VAR_CHECK_HII_QUESTION_HEADER *) OrderedList;
}

/**
  Parse and create Hii Question node.

  @param[in] HiiVariableNode    Pointer to Hii Variable node.
  @param[in] IfrOpCodeHeader    Pointer to Ifr OpCode header.
  @param[in] FromFv             Hii Question from FV.
  @param[in] StoredInBitField   Whether the Question is stored in bit field Storage.

**/
VOID
ParseHiiQuestion (
  IN VAR_CHECK_HII_VARIABLE_NODE    *HiiVariableNode,
  IN  EFI_IFR_OP_HEADER             *IfrOpCodeHeader,
  IN BOOLEAN                        FromFv,
  IN BOOLEAN                        StoredInBitField
  )
{
  VAR_CHECK_HII_QUESTION_HEADER *HiiQuestion;
  UINTN                         ArrayIndex;

  //
  // Currently only OneOf, CheckBox and Numeric can be stored in bit field.
  //
  switch (IfrOpCodeHeader->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      HiiQuestion = ParseHiiQuestionOneOf (IfrOpCodeHeader, StoredInBitField);
      break;

    case EFI_IFR_CHECKBOX_OP:
      HiiQuestion = ParseHiiQuestionCheckBox (IfrOpCodeHeader, StoredInBitField);
      break;

    case EFI_IFR_NUMERIC_OP:
      HiiQuestion = ParseHiiQuestionNumeric (IfrOpCodeHeader, StoredInBitField);
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      HiiQuestion = ParseHiiQuestionOrderedList (IfrOpCodeHeader);
      break;

    default:
      ASSERT (FALSE);
      return;
      break;
  }

  if (StoredInBitField) {
    ArrayIndex = HiiQuestion->VarOffset;
  } else {
    ArrayIndex = HiiQuestion->VarOffset * 8;
  }
  if (HiiVariableNode->HiiQuestionArray[ArrayIndex] != NULL) {
    MergeHiiQuestion (HiiVariableNode, HiiQuestion, FromFv);
  } else {
    HiiVariableNode->HiiQuestionArray[ArrayIndex] = HiiQuestion;
  }
}

/**
  Find Hii variable node by name and GUID.

  @param[in] Name   Pointer to variable name.
  @param[in] Guid   Pointer to vendor GUID.

  @return Pointer to Hii Variable node.

**/
VAR_CHECK_HII_VARIABLE_NODE *
FindHiiVariableNode (
  IN CHAR16     *Name,
  IN EFI_GUID   *Guid
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  LIST_ENTRY                    *Link;

  for (Link = mVarCheckHiiList.ForwardLink
      ;Link != &mVarCheckHiiList
      ;Link = Link->ForwardLink) {
    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (Link);

    if ((StrCmp (Name, (CHAR16 *) (HiiVariableNode->HiiVariable + 1)) == 0) &&
        CompareGuid (Guid, &HiiVariableNode->HiiVariable->Guid)) {
      return HiiVariableNode;
    }
  }

  return NULL;
}

/**
  Find Hii variable node by var store id.

  @param[in] VarStoreId         Var store id.

  @return Pointer to Hii Variable node.

**/
VAR_CHECK_HII_VARIABLE_NODE *
FindHiiVariableNodeByVarStoreId (
  IN EFI_VARSTORE_ID            VarStoreId
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  LIST_ENTRY                    *Link;

  if (VarStoreId == 0) {
    //
    // The variable store identifier, which is unique within the current form set.
    // A value of zero is invalid.
    //
    return NULL;
  }

  for (Link = mVarCheckHiiList.ForwardLink
      ;Link != &mVarCheckHiiList
      ;Link = Link->ForwardLink) {
    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (Link);
    //
    // The variable store identifier, which is unique within the current form set.
    //
    if (VarStoreId == HiiVariableNode->VarStoreId) {
      return HiiVariableNode;
    }
  }

  return NULL;
}

/**
  Destroy var store id in the Hii Variable node after parsing one Hii Package.

**/
VOID
DestroyVarStoreId (
  VOID
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  LIST_ENTRY                    *Link;

  for (Link = mVarCheckHiiList.ForwardLink
      ;Link != &mVarCheckHiiList
      ;Link = Link->ForwardLink) {
    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (Link);
    //
    // The variable store identifier, which is unique within the current form set.
    // A value of zero is invalid.
    //
    HiiVariableNode->VarStoreId = 0;
  }
}

/**
  Create Hii Variable node.

  @param[in] IfrEfiVarStore  Pointer to EFI VARSTORE.

**/
VOID
CreateHiiVariableNode (
  IN EFI_IFR_VARSTORE_EFI   *IfrEfiVarStore
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  VAR_CHECK_HII_VARIABLE_HEADER *HiiVariable;
  UINTN                         HeaderLength;
  CHAR16                        *VarName;
  UINTN                         VarNameSize;

  //
  // Get variable name.
  //
  VarNameSize = AsciiStrSize ((CHAR8 *) IfrEfiVarStore->Name) * sizeof (CHAR16);
  if (VarNameSize > mMaxVarNameSize) {
    mVarName = InternalVarCheckReallocatePool (mMaxVarNameSize, VarNameSize, mVarName);
    ASSERT (mVarName != NULL);
    mMaxVarNameSize = VarNameSize;
  }
  AsciiStrToUnicodeStrS ((CHAR8 *) IfrEfiVarStore->Name, mVarName, mMaxVarNameSize / sizeof (CHAR16));
  VarName = mVarName;

  HiiVariableNode = FindHiiVariableNode (
                      VarName,
                      &IfrEfiVarStore->Guid
                      );
  if (HiiVariableNode == NULL) {
    //
    // Not found, then create new.
    //
    HeaderLength = sizeof (*HiiVariable) + VarNameSize;
    HiiVariable = InternalVarCheckAllocateZeroPool (HeaderLength);
    ASSERT (HiiVariable != NULL);
    HiiVariable->Revision = VAR_CHECK_HII_REVISION;
    HiiVariable->OpCode = EFI_IFR_VARSTORE_EFI_OP;
    HiiVariable->HeaderLength = (UINT16) HeaderLength;
    HiiVariable->Size = IfrEfiVarStore->Size;
    HiiVariable->Attributes = IfrEfiVarStore->Attributes;
    CopyGuid (&HiiVariable->Guid, &IfrEfiVarStore->Guid);
    StrCpyS ((CHAR16 *) (HiiVariable + 1), VarNameSize / sizeof (CHAR16), VarName);

    HiiVariableNode = InternalVarCheckAllocateZeroPool (sizeof (*HiiVariableNode));
    ASSERT (HiiVariableNode != NULL);
    HiiVariableNode->Signature = VAR_CHECK_HII_VARIABLE_NODE_SIGNATURE;
    HiiVariableNode->HiiVariable = HiiVariable;
    //
    // The variable store identifier, which is unique within the current form set.
    //
    HiiVariableNode->VarStoreId = IfrEfiVarStore->VarStoreId;
    HiiVariableNode->HiiQuestionArray = InternalVarCheckAllocateZeroPool (IfrEfiVarStore->Size * 8 * sizeof (VAR_CHECK_HII_QUESTION_HEADER *));

    InsertTailList (&mVarCheckHiiList, &HiiVariableNode->Link);
  } else {
    HiiVariableNode->VarStoreId = IfrEfiVarStore->VarStoreId;
  }
}

/**
  Parse and create Hii Variable node list.

  @param[in] HiiPackage         Pointer to Hii Package.

**/
VOID
ParseHiiVariable (
  IN VOID       *HiiPackage
  )
{
  EFI_HII_PACKAGE_HEADER    *HiiPackageHeader;
  EFI_IFR_OP_HEADER         *IfrOpCodeHeader;
  EFI_IFR_VARSTORE_EFI      *IfrEfiVarStore;

  HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) HiiPackage;

  switch (HiiPackageHeader->Type) {
    case EFI_HII_PACKAGE_FORMS:
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) (HiiPackageHeader + 1);

      while ((UINTN) IfrOpCodeHeader < (UINTN) HiiPackageHeader + HiiPackageHeader->Length) {
        switch (IfrOpCodeHeader->OpCode) {
          case EFI_IFR_VARSTORE_EFI_OP:
            //
            // Come to EFI VARSTORE in Form Package.
            //
            IfrEfiVarStore = (EFI_IFR_VARSTORE_EFI *) IfrOpCodeHeader;
            if ((IfrEfiVarStore->Header.Length >= sizeof (EFI_IFR_VARSTORE_EFI)) &&
                ((IfrEfiVarStore->Attributes & EFI_VARIABLE_NON_VOLATILE) != 0)) {
              //
              // Only create node list for Hii Variable with NV attribute.
              //
              CreateHiiVariableNode (IfrEfiVarStore);
            }
            break;

          default:
            break;
        }
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
      }
      break;

    default:
      break;
  }
}

/**
  Var Check Parse Hii Package.

  @param[in] HiiPackage         Pointer to Hii Package.
  @param[in] FromFv             Hii Package from FV.

**/
VOID
VarCheckParseHiiPackage (
  IN VOID       *HiiPackage,
  IN BOOLEAN    FromFv
  )
{
  EFI_HII_PACKAGE_HEADER        *HiiPackageHeader;
  EFI_IFR_OP_HEADER             *IfrOpCodeHeader;
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  BOOLEAN                       QuestionStoredInBitField;

  //
  // Parse and create Hii Variable node list for this Hii Package.
  //
  ParseHiiVariable (HiiPackage);

  HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) HiiPackage;

  QuestionStoredInBitField = FALSE;

  switch (HiiPackageHeader->Type) {
    case EFI_HII_PACKAGE_FORMS:
      IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) (HiiPackageHeader + 1);

      while ((UINTN) IfrOpCodeHeader < (UINTN) HiiPackageHeader + HiiPackageHeader->Length) {
        switch (IfrOpCodeHeader->OpCode) {
          case EFI_IFR_GUID_OP:
            if (CompareGuid ((EFI_GUID *)((UINTN)IfrOpCodeHeader + sizeof (EFI_IFR_OP_HEADER)), &gEdkiiIfrBitVarstoreGuid)) {
              QuestionStoredInBitField = TRUE;
            }
            break;

          case EFI_IFR_END_OP:
            QuestionStoredInBitField = FALSE;
            break;

          case EFI_IFR_ONE_OF_OP:
          case EFI_IFR_CHECKBOX_OP:
          case EFI_IFR_NUMERIC_OP:
          case EFI_IFR_ORDERED_LIST_OP:
            HiiVariableNode = FindHiiVariableNodeByVarStoreId (((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.VarStoreId);
            if ((HiiVariableNode == NULL) ||
                //
                // No related Hii Variable node found.
                //
                ((((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.Header.Prompt == 0) && (((EFI_IFR_ONE_OF *) IfrOpCodeHeader)->Question.Header.Help == 0))) {
                //
                // meanless IFR item introduced by ECP.
                //
            } else {
              //
              // Normal IFR
              //
              ParseHiiQuestion (HiiVariableNode, IfrOpCodeHeader, FromFv, QuestionStoredInBitField);
            }
          default:
            break;
        }
        IfrOpCodeHeader = (EFI_IFR_OP_HEADER *) ((UINTN) IfrOpCodeHeader + IfrOpCodeHeader->Length);
      }
      break;

    default:
      break;
  }
  DestroyVarStoreId ();
}

/**
  Var Check Parse Hii Database.

  @param[in] HiiDatabase        Pointer to Hii Database.
  @param[in] HiiDatabaseSize    Hii Database size.

**/
VOID
VarCheckParseHiiDatabase (
  IN VOID       *HiiDatabase,
  IN UINTN      HiiDatabaseSize
  )
{
  EFI_HII_PACKAGE_LIST_HEADER   *HiiPackageListHeader;
  EFI_HII_PACKAGE_HEADER        *HiiPackageHeader;

  HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) HiiDatabase;

  while ((UINTN) HiiPackageListHeader < ((UINTN) HiiDatabase + HiiDatabaseSize)) {
    HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) (HiiPackageListHeader + 1);

    while ((UINTN) HiiPackageHeader < ((UINTN) HiiPackageListHeader + HiiPackageListHeader->PackageLength)) {
      //
      // Parse Hii Package.
      //
      VarCheckParseHiiPackage (HiiPackageHeader, FALSE);

      HiiPackageHeader = (EFI_HII_PACKAGE_HEADER *) ((UINTN) HiiPackageHeader + HiiPackageHeader->Length);
    }

    HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *) ((UINTN) HiiPackageListHeader + HiiPackageListHeader->PackageLength);
  }
}

/**
  Destroy Hii Variable node.

**/
VOID
DestroyHiiVariableNode (
  VOID
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  LIST_ENTRY                    *HiiVariableLink;
  UINTN                         Index;

  while (mVarCheckHiiList.ForwardLink != &mVarCheckHiiList) {
    HiiVariableLink = mVarCheckHiiList.ForwardLink;
    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (HiiVariableLink);

    RemoveEntryList (&HiiVariableNode->Link);

    //
    // Free the allocated buffer.
    //
    for (Index = 0; Index < HiiVariableNode->HiiVariable->Size * (UINTN) 8; Index++) {
      if (HiiVariableNode->HiiQuestionArray[Index] != NULL) {
        InternalVarCheckFreePool (HiiVariableNode->HiiQuestionArray[Index]);
      }
    }
    InternalVarCheckFreePool (HiiVariableNode->HiiQuestionArray);
    InternalVarCheckFreePool (HiiVariableNode->HiiVariable);
    InternalVarCheckFreePool (HiiVariableNode);
  }
}

/**
  Build VarCheckHiiBin.

  @param[out] Size      Pointer to VarCheckHii size.

  @return Pointer to VarCheckHiiBin.

**/
VOID *
BuildVarCheckHiiBin (
  OUT UINTN  *Size
  )
{
  VAR_CHECK_HII_VARIABLE_NODE   *HiiVariableNode;
  LIST_ENTRY                    *HiiVariableLink;
  UINTN                         Index;
  VOID                          *Data;
  UINT8                         *Ptr;
  UINT32                        BinSize;
  UINT32                        HiiVariableLength;

  //
  // Get Size
  //
  BinSize = 0;

  for (HiiVariableLink = mVarCheckHiiList.ForwardLink
      ;HiiVariableLink != &mVarCheckHiiList
      ;HiiVariableLink = HiiVariableLink->ForwardLink) {
    //
    // For Hii Variable header align.
    //
    BinSize = (UINT32) HEADER_ALIGN (BinSize);

    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (HiiVariableLink);
    HiiVariableLength = HiiVariableNode->HiiVariable->HeaderLength;

    for (Index = 0; Index < HiiVariableNode->HiiVariable->Size * (UINTN) 8; Index++) {
      if (HiiVariableNode->HiiQuestionArray[Index] != NULL) {
        //
        // For Hii Question header align.
        //
        HiiVariableLength = (UINT32) HEADER_ALIGN (HiiVariableLength);
        HiiVariableLength += HiiVariableNode->HiiQuestionArray[Index]->Length;
      }
    }

    HiiVariableNode->HiiVariable->Length = HiiVariableLength;
    BinSize += HiiVariableLength;
  }

  DEBUG ((DEBUG_INFO, "VarCheckHiiBin - size = 0x%x\n", BinSize));
  if (BinSize == 0) {
    *Size = BinSize;
    return NULL;
  }

  //
  // AllocatePages () and AllocatePool () from gBS are used for the process of VarCheckHiiBin generation.
  // Only here AllocateRuntimeZeroPool () from MemoryAllocateLib is used for runtime access
  // in SetVariable check handler.
  //
  Data = AllocateRuntimeZeroPool (BinSize);
  ASSERT (Data != NULL);
  //
  // Make sure the allocated buffer for VarCheckHiiBin at required alignment.
  //
  ASSERT ((((UINTN) Data) & (HEADER_ALIGNMENT - 1)) == 0);
  DEBUG ((DEBUG_INFO, "VarCheckHiiBin - built at 0x%x\n", Data));

  //
  // Gen Data
  //
  Ptr = Data;
  for (HiiVariableLink = mVarCheckHiiList.ForwardLink
      ;HiiVariableLink != &mVarCheckHiiList
      ;HiiVariableLink = HiiVariableLink->ForwardLink) {
    //
    // For Hii Variable header align.
    //
    Ptr = (UINT8 *) HEADER_ALIGN (Ptr);

    HiiVariableNode = VAR_CHECK_HII_VARIABLE_FROM_LINK (HiiVariableLink);
    CopyMem (Ptr, HiiVariableNode->HiiVariable, HiiVariableNode->HiiVariable->HeaderLength);
    Ptr += HiiVariableNode->HiiVariable->HeaderLength;

    for (Index = 0; Index < HiiVariableNode->HiiVariable->Size * (UINTN) 8; Index++) {
      if (HiiVariableNode->HiiQuestionArray[Index] != NULL) {
        //
        // For Hii Question header align.
        //
        Ptr = (UINT8 *) HEADER_ALIGN (Ptr);
        CopyMem (Ptr, HiiVariableNode->HiiQuestionArray[Index], HiiVariableNode->HiiQuestionArray[Index]->Length);
        Ptr += HiiVariableNode->HiiQuestionArray[Index]->Length;
      }
    }
  }

  *Size = BinSize;
  return Data;
}

/**
  Generate VarCheckHiiBin from Hii Database and FV.

**/
VOID
EFIAPI
VarCheckHiiGen (
  VOID
  )
{
  VarCheckHiiGenFromHiiDatabase ();
  VarCheckHiiGenFromFv ();

  mVarCheckHiiBin = BuildVarCheckHiiBin (&mVarCheckHiiBinSize);
  if (mVarCheckHiiBin == NULL) {
    DEBUG ((DEBUG_INFO, "[VarCheckHii] This driver could be removed from *.dsc and *.fdf\n"));
    return;
  }

  DestroyHiiVariableNode ();
  if (mVarName != NULL) {
    InternalVarCheckFreePool (mVarName);
  }

#ifdef DUMP_VAR_CHECK_HII
  DEBUG_CODE (
    DumpVarCheckHii (mVarCheckHiiBin, mVarCheckHiiBinSize);
  );
#endif
}

