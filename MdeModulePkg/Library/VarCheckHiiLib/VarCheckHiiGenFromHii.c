/** @file
  Var Check Hii generation from Hii Database.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VarCheckHiiGen.h"

/**
  Generate from Hii Database.

**/
VOID
VarCheckHiiGenFromHiiDatabase (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINTN                      BufferSize;
  VOID                       *Buffer;
  EFI_PHYSICAL_ADDRESS       BufferAddress;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Call first time with zero buffer length.
  // Should fail with EFI_BUFFER_TOO_SMALL.
  //
  BufferSize = 0;
  Buffer     = NULL;
  Status     = HiiDatabase->ExportPackageLists (HiiDatabase, 0, &BufferSize, Buffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    //
    // Allocate buffer to hold the HII Database.
    //
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES (BufferSize), &BufferAddress);
    ASSERT_EFI_ERROR (Status);
    Buffer = (VOID *)(UINTN)BufferAddress;

    //
    // Export HII Database into the buffer.
    //
    Status = HiiDatabase->ExportPackageLists (HiiDatabase, 0, &BufferSize, Buffer);
    ASSERT_EFI_ERROR (Status);

    DEBUG ((DEBUG_INFO, "VarCheckHiiGenDxeFromHii - HII Database exported at 0x%x, size = 0x%x\n", Buffer, BufferSize));

 #ifdef DUMP_HII_DATA
    DEBUG_CODE (
      DumpHiiDatabase (Buffer, BufferSize);
      );
 #endif

    VarCheckParseHiiDatabase (Buffer, BufferSize);

    gBS->FreePages (BufferAddress, EFI_SIZE_TO_PAGES (BufferSize));
  }
}

#ifdef DUMP_VAR_CHECK_HII
GLOBAL_REMOVE_IF_UNREFERENCED VAR_CHECK_HII_OPCODE_STRING  mHiiOpCodeStringTable[] = {
  { EFI_IFR_VARSTORE_EFI_OP, "EfiVarStore" },
  { EFI_IFR_ONE_OF_OP,       "OneOf"       },
  { EFI_IFR_CHECKBOX_OP,     "CheckBox"    },
  { EFI_IFR_NUMERIC_OP,      "Numeric"     },
  { EFI_IFR_ORDERED_LIST_OP, "OrderedList" },
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
  UINTN  Index;

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
  UINT64  Minimum;
  UINT64  Maximum;
  UINT64  OneValue;
  UINT8   *Ptr;

  DEBUG ((DEBUG_INFO, "  VAR_CHECK_HII_QUESTION_HEADER\n"));
  DEBUG ((DEBUG_INFO, "    OpCode        - 0x%02x (%a) (%a)\n", HiiQuestion->OpCode, HiiOpCodeToStr (HiiQuestion->OpCode), (HiiQuestion->BitFieldStore ? "bit level" : "byte level")));
  DEBUG ((DEBUG_INFO, "    Length        - 0x%02x\n", HiiQuestion->Length));
  DEBUG ((DEBUG_INFO, "    VarOffset     - 0x%04x (%a)\n", HiiQuestion->VarOffset, (HiiQuestion->BitFieldStore ? "bit level" : "byte level")));
  DEBUG ((DEBUG_INFO, "    StorageWidth  - 0x%02x (%a)\n", HiiQuestion->StorageWidth, (HiiQuestion->BitFieldStore ? "bit level" : "byte level")));

  switch (HiiQuestion->OpCode) {
    case EFI_IFR_ONE_OF_OP:
      Ptr = (UINT8 *)((VAR_CHECK_HII_QUESTION_ONEOF *)HiiQuestion + 1);
      while ((UINTN)Ptr < ((UINTN)HiiQuestion + HiiQuestion->Length)) {
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
      Ptr     = (UINT8 *)((VAR_CHECK_HII_QUESTION_NUMERIC *)HiiQuestion + 1);
      if (HiiQuestion->BitFieldStore) {
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
      DEBUG ((DEBUG_INFO, "    MaxContainers - 0x%02x\n", ((VAR_CHECK_HII_QUESTION_ORDEREDLIST *)HiiQuestion)->MaxContainers));
      Ptr = (UINT8 *)((VAR_CHECK_HII_QUESTION_ORDEREDLIST *)HiiQuestion + 1);
      while ((UINTN)Ptr < ((UINTN)HiiQuestion + HiiQuestion->Length)) {
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
  VAR_CHECK_HII_QUESTION_HEADER  *HiiQuestion;

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
  HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->HeaderLength));
  while ((UINTN)HiiQuestion < ((UINTN)HiiVariable + HiiVariable->Length)) {
    //
    // Dump Hii Question related to the Hii Variable.
    //
    DumpHiiQuestion (HiiQuestion);
    //
    // For Hii Question header align.
    //
    HiiQuestion = (VAR_CHECK_HII_QUESTION_HEADER *)HEADER_ALIGN (((UINTN)HiiQuestion + HiiQuestion->Length));
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
  VAR_CHECK_HII_VARIABLE_HEADER  *HiiVariable;

  DEBUG ((DEBUG_INFO, "DumpVarCheckHii\n"));

  //
  // For Hii Variable header align.
  //
  HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (VarCheckHiiBin);
  while ((UINTN)HiiVariable < ((UINTN)VarCheckHiiBin + VarCheckHiiBinSize)) {
    DumpHiiVariable (HiiVariable);
    //
    // For Hii Variable header align.
    //
    HiiVariable = (VAR_CHECK_HII_VARIABLE_HEADER *)HEADER_ALIGN (((UINTN)HiiVariable + HiiVariable->Length));
  }
}

#endif
