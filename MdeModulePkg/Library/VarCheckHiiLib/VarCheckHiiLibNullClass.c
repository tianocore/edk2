/** @file
  Var Check Hii handler.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VarCheckHii.h"

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mVarCheckHiiHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/**
  Dump some hexadecimal data.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the dump.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to dump.

**/
VOID
VarCheckHiiInternalDumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  )
{
  UINT8 *Data;

  CHAR8 Val[50];

  CHAR8 Str[20];

  UINT8 TempByte;
  UINTN Size;
  UINTN Index;

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte            = Data[Index];
      Val[Index * 3 + 0]  = mVarCheckHiiHex[TempByte >> 4];
      Val[Index * 3 + 1]  = mVarCheckHiiHex[TempByte & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    DEBUG ((DEBUG_INFO , "%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str));

    Data += Size;
    Offset += Size;
    DataSize -= Size;
  }
}

/**
  Var Check Hii Question.

  @param[in] HiiQuestion    Pointer to Hii Question
  @param[in] Data           Data pointer.
  @param[in] DataSize       Size of Data to set.

  @retval TRUE  Check pass
  @retval FALSE Check fail.

**/
BOOLEAN
VarCheckHiiQuestion (
  IN VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion,
  IN VOID                           *Data,
  IN UINTN                          DataSize
  )
{
  UINT64   OneData;
  UINT64   Minimum;
  UINT64   Maximum;
  UINT64   OneValue;
  UINT8    *Ptr;
  UINT8    Index;
  UINT8    MaxContainers;
  UINT8    StartBit;
  UINT8    EndBit;
  UINT8    TotalBits;
  UINT16   VarOffsetByteLevel;
  UINT8    StorageWidthByteLevel;

  if (HiiQuestion->BitFieldStore) {
    VarOffsetByteLevel    = HiiQuestion->VarOffset / 8;
    TotalBits             = HiiQuestion->VarOffset % 8 + HiiQuestion->StorageWidth;
    StorageWidthByteLevel = (TotalBits % 8 == 0 ? TotalBits / 8: TotalBits / 8 + 1);
  } else {
    VarOffsetByteLevel    = HiiQuestion->VarOffset;
    StorageWidthByteLevel = HiiQuestion->StorageWidth;
  }

  if (((UINT32) VarOffsetByteLevel + StorageWidthByteLevel) > DataSize) {
    DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: (VarOffset(0x%04x) + StorageWidth(0x%02x)) > Size(0x%x)\n", VarOffsetByteLevel, StorageWidthByteLevel, DataSize));
    return FALSE;
  }

  OneData = 0;
  CopyMem (&OneData, (UINT8 *) Data + VarOffsetByteLevel, StorageWidthByteLevel);
  if (HiiQuestion->BitFieldStore) {
    //
    // Get the value from the bit field.
    //
    StartBit = HiiQuestion->VarOffset % 8;
    EndBit   = StartBit + HiiQuestion->StorageWidth - 1;
    OneData  = BitFieldRead64 (OneData, StartBit, EndBit);
  }

  switch (HiiQuestion->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion + 1);
      while ((UINTN) Ptr < (UINTN) HiiQuestion + HiiQuestion->Length) {
        OneValue = 0;
        if (HiiQuestion->BitFieldStore) {
          //
          // For OneOf stored in bit field, the value of options are saved as UINT32 type.
          //
          CopyMem (&OneValue, Ptr, sizeof (UINT32));
        } else {
          CopyMem (&OneValue, Ptr, HiiQuestion->StorageWidth);
        }
        if (OneData == OneValue) {
          //
          // Match
          //
          break;
        }
        if (HiiQuestion->BitFieldStore) {
          Ptr += sizeof (UINT32);
        } else {
          Ptr += HiiQuestion->StorageWidth;
        }
      }
      if ((UINTN) Ptr >= ((UINTN) HiiQuestion + HiiQuestion->Length)) {
        //
        // No match
        //
        DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: OneOf mismatch (0x%lx)\n", OneData));
        DEBUG_CODE (VarCheckHiiInternalDumpHex (2, 0, HiiQuestion->Length, (UINT8 *) HiiQuestion););
        return FALSE;
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      if ((OneData != 0) && (OneData != 1)) {
        DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: CheckBox mismatch (0x%lx)\n", OneData));
        DEBUG_CODE (VarCheckHiiInternalDumpHex (2, 0, HiiQuestion->Length, (UINT8 *) HiiQuestion););
        return FALSE;
      }
      break;

    case EFI_IFR_NUMERIC_OP:
      Minimum = 0;
      Maximum = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_NUMERIC *) HiiQuestion + 1);
      if (HiiQuestion->BitFieldStore) {
        //
        // For Numeric stored in bit field, the value of Maximum/Minimum are saved as UINT32 type.
        //
        CopyMem (&Minimum, Ptr, sizeof (UINT32));
        Ptr += sizeof (UINT32);
        CopyMem (&Maximum, Ptr, sizeof (UINT32));
        Ptr += sizeof (UINT32);
      } else {
        CopyMem (&Minimum, Ptr, HiiQuestion->StorageWidth);
        Ptr += HiiQuestion->StorageWidth;
        CopyMem (&Maximum, Ptr, HiiQuestion->StorageWidth);
        Ptr += HiiQuestion->StorageWidth;
      }

      //
      // No need to check Step, because it is ONLY for UI.
      //
      if ((OneData < Minimum) || (OneData > Maximum)) {
        DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: Numeric mismatch (0x%lx)\n", OneData));
        DEBUG_CODE (VarCheckHiiInternalDumpHex (2, 0, HiiQuestion->Length, (UINT8 *) HiiQuestion););
        return FALSE;
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      MaxContainers = ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion)->MaxContainers;
      if (((UINT32) HiiQuestion->VarOffset + HiiQuestion->StorageWidth * MaxContainers) > DataSize) {
        DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: (VarOffset(0x%04x) + StorageWidth(0x%02x) * MaxContainers(0x%02x)) > Size(0x%x)\n", HiiQuestion->VarOffset, HiiQuestion->StorageWidth, MaxContainers, DataSize));
        return FALSE;
      }
      for (Index = 0; Index < MaxContainers; Index++) {
        OneData = 0;
        CopyMem (&OneData, (UINT8 *) Data + HiiQuestion->VarOffset + HiiQuestion->StorageWidth * Index, HiiQuestion->StorageWidth);
        if (OneData == 0) {
          //
          // The value of 0 is used to determine if a particular "slot" in the array is empty.
          //
          continue;
        }

        Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion + 1);
        while ((UINTN) Ptr < ((UINTN) HiiQuestion + HiiQuestion->Length)) {
          OneValue = 0;
          CopyMem (&OneValue, Ptr, HiiQuestion->StorageWidth);
          if (OneData == OneValue) {
            //
            // Match
            //
            break;
          }
          Ptr += HiiQuestion->StorageWidth;
        }
        if ((UINTN) Ptr >= ((UINTN) HiiQuestion + HiiQuestion->Length)) {
          //
          // No match
          //
          DEBUG ((DEBUG_INFO , "VarCheckHiiQuestion fail: OrderedList mismatch\n"));
          DEBUG_CODE (VarCheckHiiInternalDumpHex (2, 0, HiiQuestion->StorageWidth * MaxContainers, (UINT8 *) Data + HiiQuestion->VarOffset););
          DEBUG_CODE (VarCheckHiiInternalDumpHex (2, 0, HiiQuestion->Length, (UINT8 *) HiiQuestion););
          return FALSE;
        }
      }
      break;

    default:
      ASSERT (FALSE);
      break;
  }

  return TRUE;
}

VAR_CHECK_HII_VARIABLE_HEADER   *mVarCheckHiiBin = NULL;
UINTN                           mVarCheckHiiBinSize = 0;

/**
  SetVariable check handler HII.

  @param[in] VariableName       Name of Variable to set.
  @param[in] VendorGuid         Variable vendor GUID.
  @param[in] Attributes         Attribute value of the variable.
  @param[in] DataSize           Size of Data to set.
  @param[in] Data               Data pointer.

  @retval EFI_SUCCESS               The SetVariable check result was success.
  @retval EFI_SECURITY_VIOLATION    Check fail.

**/
EFI_STATUS
EFIAPI
SetVariableCheckHandlerHii (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  VAR_CHECK_HII_VARIABLE_HEADER     *HiiVariable;
  VAR_CHECK_HII_QUESTION_HEADER     *HiiQuestion;

  if (mVarCheckHiiBin == NULL) {
    return EFI_SUCCESS;
  }

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
    //
    // Do not check delete variable.
    //
    return EFI_SUCCESS;
  }

  //
  // For Hii Variable header align.
  //
  HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *) HEADER_ALIGN (mVarCheckHiiBin);
  while ((UINTN) HiiVariable < ((UINTN) mVarCheckHiiBin + mVarCheckHiiBinSize)) {
    if ((StrCmp ((CHAR16 *) (HiiVariable + 1), VariableName) == 0) &&
        (CompareGuid (&HiiVariable->Guid, VendorGuid))) {
      //
      // Found the Hii Variable that could be used to do check.
      //
      DEBUG ((DEBUG_INFO , "VarCheckHiiVariable - %s:%g with Attributes = 0x%08x Size = 0x%x\n", VariableName, VendorGuid, Attributes, DataSize));
      if (HiiVariable->Attributes != Attributes) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Attributes - 0x%08x\n", HiiVariable->Attributes));
        return EFI_SECURITY_VIOLATION;
      }

      if (DataSize == 0) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - CHECK PASS with DataSize == 0 !\n"));
        return EFI_SUCCESS;
      }

      if (HiiVariable->Size != DataSize) {
        DEBUG ((DEBUG_INFO, "VarCheckHiiVariable fail for Size - 0x%x\n", HiiVariable->Size));
        return EFI_SECURITY_VIOLATION;
      }

      //
      // Do the check.
      // For Hii Question header align.
      //
      HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *) HEADER_ALIGN (((UINTN) HiiVariable + HiiVariable->HeaderLength));
      while ((UINTN) HiiQuestion < ((UINTN) HiiVariable + HiiVariable->Length)) {
        if (!VarCheckHiiQuestion (HiiQuestion, Data, DataSize)) {
          return EFI_SECURITY_VIOLATION;
        }
        //
        // For Hii Question header align.
        //
        HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *) HEADER_ALIGN (((UINTN) HiiQuestion + HiiQuestion->Length));
      }

      DEBUG ((DEBUG_INFO, "VarCheckHiiVariable - ALL CHECK PASS!\n"));
      return EFI_SUCCESS;
    }
    //
    // For Hii Variable header align.
    //
    HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *) HEADER_ALIGN (((UINTN) HiiVariable + HiiVariable->Length));
  }

  // Not found, so pass.
  return EFI_SUCCESS;
}

#ifdef DUMP_VAR_CHECK_HII
GLOBAL_REMOVE_IF_UNREFERENCED VAR_CHECK_HII_OPCODE_STRING   mHiiOpCodeStringTable[] = {
  {EFI_IFR_VARSTORE_EFI_OP,     "EfiVarStore"},
  {EFI_IFR_ONE_OF_OP,           "OneOf"},
  {EFI_IFR_CHECKBOX_OP,         "CheckBox"},
  {EFI_IFR_NUMERIC_OP,          "Numeric"},
  {EFI_IFR_ORDERED_LIST_OP,     "OrderedList"},
};

/**
  HII opcode to string.

  @param[in] HiiOpCode  Hii OpCode.

  @return Pointer to string.

**/
CHAR8 *
HiiOpCodeToStr (
  IN UINT8  HiiOpCode
  )
{
  UINTN     Index;
  for (Index = 0; Index < ARRAY_SIZE (mHiiOpCodeStringTable); Index++) {
    if (mHiiOpCodeStringTable[Index].HiiOpCode == HiiOpCode) {
      return mHiiOpCodeStringTable[Index].HiiOpCodeStr;
    }
  }

  return "<UnknownHiiOpCode>";
}

/**
  Dump Hii Question.

  @param[in] HiiQuestion    Pointer to Hii Question.

**/
VOID
DumpHiiQuestion (
  IN VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion
  )
{
  UINT64    Minimum;
  UINT64    Maximum;
  UINT64    OneValue;
  UINT8     *Ptr;

  DEBUG ((DEBUG_INFO, "  VAR_CHECK_HII_QUESTION_HEADER\n"));
  DEBUG ((DEBUG_INFO, "    OpCode        - 0x%02x (%a) (%a)\n", HiiQuestion->OpCode, HiiOpCodeToStr (HiiQuestion->OpCode), (HiiQuestion->BitFieldStore? "bit level": "byte level")));
  DEBUG ((DEBUG_INFO, "    Length        - 0x%02x\n", HiiQuestion->Length));
  DEBUG ((DEBUG_INFO, "    VarOffset     - 0x%04x (%a)\n", HiiQuestion->VarOffset, (HiiQuestion->BitFieldStore? "bit level": "byte level")));
  DEBUG ((DEBUG_INFO, "    StorageWidth  - 0x%02x (%a)\n", HiiQuestion->StorageWidth, (HiiQuestion->BitFieldStore? "bit level": "byte level")));

  switch (HiiQuestion->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ONEOF *) HiiQuestion + 1);
      while ((UINTN) Ptr < ((UINTN) HiiQuestion + HiiQuestion->Length)) {
        OneValue = 0;
        if (HiiQuestion->BitFieldStore) {
          //
          // For OneOf stored in bit field, the value of options are saved as UINT32 type.
          //
          CopyMem (&OneValue, Ptr, sizeof (UINT32));
          DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%08x\n", OneValue));
        } else {
          CopyMem (&OneValue, Ptr, HiiQuestion->StorageWidth);
          switch (HiiQuestion->StorageWidth) {
            case sizeof (UINT8):
              DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%02x\n", OneValue));
              break;
            case sizeof (UINT16):
              DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%04x\n", OneValue));
              break;
            case sizeof (UINT32):
              DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%08x\n", OneValue));
              break;
            case sizeof (UINT64):
              DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%016lx\n", OneValue));
              break;
            default:
              ASSERT (FALSE);
              break;
          }
        }
        if (HiiQuestion->BitFieldStore) {
          Ptr += sizeof (UINT32);
        } else {
          Ptr += HiiQuestion->StorageWidth;
        }
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      break;

    case EFI_IFR_NUMERIC_OP:
      Minimum = 0;
      Maximum = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_NUMERIC *) HiiQuestion + 1);
      if(HiiQuestion->BitFieldStore) {
        //
        // For Numeric stored in bit field, the value of Maximum/Minimum are saved as UINT32 type.
        //
        CopyMem (&Minimum, Ptr, sizeof (UINT32));
        Ptr += sizeof (UINT32);
        CopyMem (&Maximum, Ptr, sizeof (UINT32));
        Ptr += sizeof (UINT32);

        DEBUG ((DEBUG_INFO, "    Minimum       - 0x%08x\n", Minimum));
        DEBUG ((DEBUG_INFO, "    Maximum       - 0x%08x\n", Maximum));
      } else {
        CopyMem (&Minimum, Ptr, HiiQuestion->StorageWidth);
        Ptr += HiiQuestion->StorageWidth;
        CopyMem (&Maximum, Ptr, HiiQuestion->StorageWidth);
        Ptr += HiiQuestion->StorageWidth;

        switch (HiiQuestion->StorageWidth) {
          case sizeof (UINT8):
            DEBUG ((DEBUG_INFO, "    Minimum       - 0x%02x\n", Minimum));
            DEBUG ((DEBUG_INFO, "    Maximum       - 0x%02x\n", Maximum));
            break;
          case sizeof (UINT16):
            DEBUG ((DEBUG_INFO, "    Minimum       - 0x%04x\n", Minimum));
            DEBUG ((DEBUG_INFO, "    Maximum       - 0x%04x\n", Maximum));
            break;
          case sizeof (UINT32):
            DEBUG ((DEBUG_INFO, "    Minimum       - 0x%08x\n", Minimum));
            DEBUG ((DEBUG_INFO, "    Maximum       - 0x%08x\n", Maximum));
            break;
          case sizeof (UINT64):
            DEBUG ((DEBUG_INFO, "    Minimum       - 0x%016lx\n", Minimum));
            DEBUG ((DEBUG_INFO, "    Maximum       - 0x%016lx\n", Maximum));
            break;
          default:
            ASSERT (FALSE);
            break;
        }
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      DEBUG ((DEBUG_INFO, "    MaxContainers - 0x%02x\n", ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion)->MaxContainers));
      Ptr = (UINT8 *) ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *) HiiQuestion + 1);
      while ((UINTN) Ptr < ((UINTN) HiiQuestion + HiiQuestion->Length)) {
        OneValue = 0;
        CopyMem (&OneValue, Ptr, HiiQuestion->StorageWidth);
        switch (HiiQuestion->StorageWidth) {
          case sizeof (UINT8):
            DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%02x\n", OneValue));
            break;
          case sizeof (UINT16):
            DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%04x\n", OneValue));
            break;
          case sizeof (UINT32):
            DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%08x\n", OneValue));
            break;
          case sizeof (UINT64):
            DEBUG ((DEBUG_INFO, "    OneOfOption   - 0x%016lx\n", OneValue));
            break;
          default:
            ASSERT (FALSE);
            break;
        }
        Ptr += HiiQuestion->StorageWidth;
      }
      break;

    default:
      ASSERT (FALSE);
      break;
  }
}

/**
  Dump Hii Variable.

  @param[in] HiiVariable    Pointer to Hii Variable.

**/
VOID
DumpHiiVariable (
  IN VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariable
  )
{
  VAR_CHECK_HII_QUESTION_HEADER *HiiQuestion;

  DEBUG ((DEBUG_INFO, "VAR_CHECK_HII_VARIABLE_HEADER\n"));
  DEBUG ((DEBUG_INFO, "  Revision        - 0x%04x\n", HiiVariable->Revision));
  DEBUG ((DEBUG_INFO, "  HeaderLength    - 0x%04x\n", HiiVariable->HeaderLength));
  DEBUG ((DEBUG_INFO, "  Length          - 0x%08x\n", HiiVariable->Length));
  DEBUG ((DEBUG_INFO, "  OpCode          - 0x%02x (%a)\n", HiiVariable->OpCode, HiiOpCodeToStr (HiiVariable->OpCode)));
  DEBUG ((DEBUG_INFO, "  Size            - 0x%04x\n", HiiVariable->Size));
  DEBUG ((DEBUG_INFO, "  Attributes      - 0x%08x\n", HiiVariable->Attributes));
  DEBUG ((DEBUG_INFO, "  Guid            - %g\n", &HiiVariable->Guid));
  DEBUG ((DEBUG_INFO, "  Name            - %s\n", HiiVariable + 1));

  //
  // For Hii Question header align.
  //
  HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *) HEADER_ALIGN (((UINTN) HiiVariable + HiiVariable->HeaderLength));
  while ((UINTN) HiiQuestion < ((UINTN) HiiVariable + HiiVariable->Length)) {
    //
    // Dump Hii Question related to the Hii Variable.
    //
    DumpHiiQuestion (HiiQuestion);
    //
    // For Hii Question header align.
    //
    HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *) HEADER_ALIGN (((UINTN) HiiQuestion + HiiQuestion->Length));
  }
}

/**
  Dump Var Check HII.

  @param[in] VarCheckHiiBin     Pointer to VarCheckHiiBin.
  @param[in] VarCheckHiiBinSize VarCheckHiiBin size.

**/
VOID
DumpVarCheckHii (
  IN VOID   *VarCheckHiiBin,
  IN UINTN  VarCheckHiiBinSize
  )
{
  VAR_CHECK_HII_VARIABLE_HEADER     *HiiVariable;

  DEBUG ((DEBUG_INFO, "DumpVarCheckHii\n"));

  //
  // For Hii Variable header align.
  //
  HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *) HEADER_ALIGN (VarCheckHiiBin);
  while ((UINTN) HiiVariable < ((UINTN) VarCheckHiiBin + VarCheckHiiBinSize)) {
    DumpHiiVariable (HiiVariable);
    //
    // For Hii Variable header align.
    //
    HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *) HEADER_ALIGN (((UINTN) HiiVariable + HiiVariable->Length));
  }
}
#endif

/**
  Constructor function of VarCheckHiiLib to register var check HII handler.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckHiiLibNullClassConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  VarCheckLibRegisterEndOfDxeCallback (VarCheckHiiGen);
  VarCheckLibRegisterAddressPointer ((VOID **) &mVarCheckHiiBin);
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerHii);

  return EFI_SUCCESS;
}

