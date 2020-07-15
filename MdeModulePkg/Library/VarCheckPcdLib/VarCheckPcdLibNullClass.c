/** @file
  Var Check PCD handler.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DxeServicesLib.h>

#include "VarCheckPcdStructure.h"

//#define DUMP_VAR_CHECK_PCD

GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 mVarCheckPcdHex[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

/**
  Dump some hexadecimal data.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the dump.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to dump.

**/
VOID
VarCheckPcdInternalDumpHex (
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
      Val[Index * 3 + 0]  = mVarCheckPcdHex[TempByte >> 4];
      Val[Index * 3 + 1]  = mVarCheckPcdHex[TempByte & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    DEBUG ((EFI_D_INFO, "%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str));

    Data += Size;
    Offset += Size;
    DataSize -= Size;
  }
}

/**
  Var Check Pcd ValidData.

  @param[in] PcdValidData   Pointer to Pcd ValidData
  @param[in] Data           Data pointer.
  @param[in] DataSize       Size of Data to set.

  @retval TRUE  Check pass
  @retval FALSE Check fail.

**/
BOOLEAN
VarCheckPcdValidData (
  IN VAR_CHECK_PCD_VALID_DATA_HEADER    *PcdValidData,
  IN VOID                               *Data,
  IN UINTN                              DataSize
  )
{
  UINT64   OneData;
  UINT64   Minimum;
  UINT64   Maximum;
  UINT64   OneValue;
  UINT8    *Ptr;

  OneData = 0;
  CopyMem (&OneData, (UINT8 *) Data + PcdValidData->VarOffset, PcdValidData->StorageWidth);

  switch (PcdValidData->Type) {
    case VarCheckPcdValidList:
      Ptr = (UINT8 *) ((VAR_CHECK_PCD_VALID_LIST *) PcdValidData + 1);
      while ((UINTN) Ptr < (UINTN) PcdValidData + PcdValidData->Length) {
        OneValue = 0;
        CopyMem (&OneValue, Ptr, PcdValidData->StorageWidth);
        if (OneData == OneValue) {
          //
          // Match
          //
          break;
        }
        Ptr += PcdValidData->StorageWidth;
      }
      if ((UINTN) Ptr >= ((UINTN) PcdValidData + PcdValidData->Length)) {
        //
        // No match
        //
        DEBUG ((EFI_D_INFO, "VarCheckPcdValidData fail: ValidList mismatch (0x%lx)\n", OneData));
        DEBUG_CODE (VarCheckPcdInternalDumpHex (2, 0, PcdValidData->Length, (UINT8 *) PcdValidData););
        return FALSE;
      }
      break;

    case VarCheckPcdValidRange:
      Minimum = 0;
      Maximum = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_PCD_VALID_RANGE *) PcdValidData + 1);
      while ((UINTN) Ptr < (UINTN) PcdValidData + PcdValidData->Length) {
        CopyMem (&Minimum, Ptr, PcdValidData->StorageWidth);
        Ptr += PcdValidData->StorageWidth;
        CopyMem (&Maximum, Ptr, PcdValidData->StorageWidth);
        Ptr += PcdValidData->StorageWidth;

        if ((OneData >= Minimum) && (OneData <= Maximum)) {
          return TRUE;
        }
      }
      DEBUG ((EFI_D_INFO, "VarCheckPcdValidData fail: ValidRange mismatch (0x%lx)\n", OneData));
      DEBUG_CODE (VarCheckPcdInternalDumpHex (2, 0, PcdValidData->Length, (UINT8 *) PcdValidData););
      return FALSE;
      break;

    default:
      ASSERT (FALSE);
      break;
  }

  return TRUE;
}

VAR_CHECK_PCD_VARIABLE_HEADER   *mVarCheckPcdBin = NULL;
UINTN                           mVarCheckPcdBinSize = 0;

/**
  SetVariable check handler PCD.

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
SetVariableCheckHandlerPcd (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  VAR_CHECK_PCD_VARIABLE_HEADER     *PcdVariable;
  VAR_CHECK_PCD_VALID_DATA_HEADER   *PcdValidData;

  if (mVarCheckPcdBin == NULL) {
    return EFI_SUCCESS;
  }

  if ((((Attributes & EFI_VARIABLE_APPEND_WRITE) == 0) && (DataSize == 0)) || (Attributes == 0)) {
    //
    // Do not check delete variable.
    //
    return EFI_SUCCESS;
  }

  //
  // For Pcd Variable header align.
  //
  PcdVariable = (VAR_CHECK_PCD_VARIABLE_HEADER *) HEADER_ALIGN (mVarCheckPcdBin);
  while ((UINTN) PcdVariable < ((UINTN) mVarCheckPcdBin + mVarCheckPcdBinSize)) {
    if ((StrCmp ((CHAR16 *) (PcdVariable + 1), VariableName) == 0) &&
        (CompareGuid (&PcdVariable->Guid, VendorGuid))) {
      //
      // Found the Pcd Variable that could be used to do check.
      //
      DEBUG ((EFI_D_INFO, "VarCheckPcdVariable - %s:%g with Attributes = 0x%08x Size = 0x%x\n", VariableName, VendorGuid, Attributes, DataSize));
      if ((PcdVariable->Attributes != 0) && PcdVariable->Attributes != Attributes) {
        DEBUG ((EFI_D_INFO, "VarCheckPcdVariable fail for Attributes - 0x%08x\n", PcdVariable->Attributes));
        return EFI_SECURITY_VIOLATION;
      }

      if (DataSize == 0) {
        DEBUG ((EFI_D_INFO, "VarCheckPcdVariable - CHECK PASS with DataSize == 0 !\n"));
        return EFI_SUCCESS;
      }

      //
      // Do the check.
      // For Pcd ValidData header align.
      //
      PcdValidData = (VAR_CHECK_PCD_VALID_DATA_HEADER *) HEADER_ALIGN (((UINTN) PcdVariable + PcdVariable->HeaderLength));
      while ((UINTN) PcdValidData < ((UINTN) PcdVariable + PcdVariable->Length)) {
        if (((UINTN) PcdValidData->VarOffset + PcdValidData->StorageWidth) <= DataSize) {
          if (!VarCheckPcdValidData (PcdValidData, Data, DataSize)) {
            return EFI_SECURITY_VIOLATION;
          }
        }
        //
        // For Pcd ValidData header align.
        //
        PcdValidData = (VAR_CHECK_PCD_VALID_DATA_HEADER *) HEADER_ALIGN (((UINTN) PcdValidData + PcdValidData->Length));
      }

      DEBUG ((EFI_D_INFO, "VarCheckPcdVariable - ALL CHECK PASS!\n"));
      return EFI_SUCCESS;
    }
    //
    // For Pcd Variable header align.
    //
    PcdVariable = (VAR_CHECK_PCD_VARIABLE_HEADER *) HEADER_ALIGN (((UINTN) PcdVariable + PcdVariable->Length));
  }

  // Not found, so pass.
  return EFI_SUCCESS;
}

#ifdef DUMP_VAR_CHECK_PCD
/**
  Dump Pcd ValidData.

  @param[in] PcdValidData    Pointer to Pcd ValidData.

**/
VOID
DumpPcdValidData (
  IN VAR_CHECK_PCD_VALID_DATA_HEADER    *PcdValidData
  )
{
  UINT64    Minimum;
  UINT64    Maximum;
  UINT64    OneValue;
  UINT8     *Ptr;

  DEBUG ((EFI_D_INFO, "  VAR_CHECK_PCD_VALID_DATA_HEADER\n"));
  DEBUG ((EFI_D_INFO, "    Type          - 0x%02x\n", PcdValidData->Type));
  DEBUG ((EFI_D_INFO, "    Length        - 0x%02x\n", PcdValidData->Length));
  DEBUG ((EFI_D_INFO, "    VarOffset     - 0x%04x\n", PcdValidData->VarOffset));
  DEBUG ((EFI_D_INFO, "    StorageWidth  - 0x%02x\n", PcdValidData->StorageWidth));

  switch (PcdValidData->Type) {
    case VarCheckPcdValidList:
      Ptr = (UINT8 *) ((VAR_CHECK_PCD_VALID_LIST *) PcdValidData + 1);
      while ((UINTN) Ptr < ((UINTN) PcdValidData + PcdValidData->Length)) {
        OneValue = 0;
        CopyMem (&OneValue, Ptr, PcdValidData->StorageWidth);
        switch (PcdValidData->StorageWidth) {
          case sizeof (UINT8):
            DEBUG ((EFI_D_INFO, "    ValidList   - 0x%02x\n", OneValue));
            break;
          case sizeof (UINT16):
            DEBUG ((EFI_D_INFO, "    ValidList   - 0x%04x\n", OneValue));
            break;
          case sizeof (UINT32):
            DEBUG ((EFI_D_INFO, "    ValidList   - 0x%08x\n", OneValue));
            break;
          case sizeof (UINT64):
            DEBUG ((EFI_D_INFO, "    ValidList   - 0x%016lx\n", OneValue));
            break;
          default:
            ASSERT (FALSE);
            break;
        }
        Ptr += PcdValidData->StorageWidth;
      }
      break;

    case VarCheckPcdValidRange:
      Minimum = 0;
      Maximum = 0;
      Ptr = (UINT8 *) ((VAR_CHECK_PCD_VALID_RANGE *) PcdValidData + 1);
      while ((UINTN) Ptr < (UINTN) PcdValidData + PcdValidData->Length) {
        CopyMem (&Minimum, Ptr, PcdValidData->StorageWidth);
        Ptr += PcdValidData->StorageWidth;
        CopyMem (&Maximum, Ptr, PcdValidData->StorageWidth);
        Ptr += PcdValidData->StorageWidth;

        switch (PcdValidData->StorageWidth) {
          case sizeof (UINT8):
            DEBUG ((EFI_D_INFO, "    Minimum       - 0x%02x\n", Minimum));
            DEBUG ((EFI_D_INFO, "    Maximum       - 0x%02x\n", Maximum));
            break;
          case sizeof (UINT16):
            DEBUG ((EFI_D_INFO, "    Minimum       - 0x%04x\n", Minimum));
            DEBUG ((EFI_D_INFO, "    Maximum       - 0x%04x\n", Maximum));
            break;
          case sizeof (UINT32):
            DEBUG ((EFI_D_INFO, "    Minimum       - 0x%08x\n", Minimum));
            DEBUG ((EFI_D_INFO, "    Maximum       - 0x%08x\n", Maximum));
            break;
          case sizeof (UINT64):
            DEBUG ((EFI_D_INFO, "    Minimum       - 0x%016lx\n", Minimum));
            DEBUG ((EFI_D_INFO, "    Maximum       - 0x%016lx\n", Maximum));
            break;
          default:
            ASSERT (FALSE);
            break;
        }
      }
      break;

    default:
      ASSERT (FALSE);
      break;
  }
}

/**
  Dump Pcd Variable.

  @param[in] PcdVariable    Pointer to Pcd Variable.

**/
VOID
DumpPcdVariable (
  IN VAR_CHECK_PCD_VARIABLE_HEADER  *PcdVariable
  )
{
  VAR_CHECK_PCD_VALID_DATA_HEADER *PcdValidData;

  DEBUG ((EFI_D_INFO, "VAR_CHECK_PCD_VARIABLE_HEADER\n"));
  DEBUG ((EFI_D_INFO, "  Revision        - 0x%04x\n", PcdVariable->Revision));
  DEBUG ((EFI_D_INFO, "  HeaderLength    - 0x%04x\n", PcdVariable->HeaderLength));
  DEBUG ((EFI_D_INFO, "  Length          - 0x%08x\n", PcdVariable->Length));
  DEBUG ((EFI_D_INFO, "  Type            - 0x%02x\n", PcdVariable->Type));
  DEBUG ((EFI_D_INFO, "  Attributes      - 0x%08x\n", PcdVariable->Attributes));
  DEBUG ((EFI_D_INFO, "  Guid            - %g\n", &PcdVariable->Guid));
  DEBUG ((EFI_D_INFO, "  Name            - %s\n", PcdVariable + 1));

  //
  // For Pcd ValidData header align.
  //
  PcdValidData = (VAR_CHECK_PCD_VALID_DATA_HEADER *) HEADER_ALIGN (((UINTN) PcdVariable + PcdVariable->HeaderLength));
  while ((UINTN) PcdValidData < ((UINTN) PcdVariable + PcdVariable->Length)) {
    //
    // Dump Pcd ValidData related to the Pcd Variable.
    //
    DumpPcdValidData (PcdValidData);
    //
    // For Pcd ValidData header align.
    //
    PcdValidData = (VAR_CHECK_PCD_VALID_DATA_HEADER *) HEADER_ALIGN (((UINTN) PcdValidData + PcdValidData->Length));
  }
}

/**
  Dump Var Check PCD.

  @param[in] VarCheckPcdBin     Pointer to VarCheckPcdBin.
  @param[in] VarCheckPcdBinSize VarCheckPcdBin size.

**/
VOID
DumpVarCheckPcd (
  IN VOID   *VarCheckPcdBin,
  IN UINTN  VarCheckPcdBinSize
  )
{
  VAR_CHECK_PCD_VARIABLE_HEADER     *PcdVariable;

  DEBUG ((EFI_D_INFO, "DumpVarCheckPcd\n"));

  //
  // For Pcd Variable header align.
  //
  PcdVariable = (VAR_CHECK_PCD_VARIABLE_HEADER *) HEADER_ALIGN (VarCheckPcdBin);
  while ((UINTN) PcdVariable < ((UINTN) VarCheckPcdBin + VarCheckPcdBinSize)) {
    DumpPcdVariable (PcdVariable);
    //
    // For Pcd Variable header align.
    //
    PcdVariable = (VAR_CHECK_PCD_VARIABLE_HEADER *) HEADER_ALIGN (((UINTN) PcdVariable + PcdVariable->Length));
  }
}
#endif

/**
  Locate VarCheckPcdBin.

**/
VOID
EFIAPI
LocateVarCheckPcdBin (
  VOID
  )
{
  EFI_STATUS                        Status;
  VAR_CHECK_PCD_VARIABLE_HEADER     *VarCheckPcdBin;
  UINTN                             VarCheckPcdBinSize;

  //
  // Search the VarCheckPcdBin from the first RAW section of current FFS.
  //
  Status = GetSectionFromFfs (
             EFI_SECTION_RAW,
             0,
             (VOID **) &VarCheckPcdBin,
             &VarCheckPcdBinSize
             );
  if (!EFI_ERROR (Status)) {
    //
    // AllocateRuntimeZeroPool () from MemoryAllocateLib is used for runtime access
    // in SetVariable check handler.
    //
    mVarCheckPcdBin = AllocateRuntimeCopyPool (VarCheckPcdBinSize, VarCheckPcdBin);
    ASSERT (mVarCheckPcdBin != NULL);
    //
    // Make sure the allocated buffer for VarCheckPcdBin at required alignment.
    //
    ASSERT ((((UINTN) mVarCheckPcdBin) & (HEADER_ALIGNMENT - 1)) == 0);
    mVarCheckPcdBinSize = VarCheckPcdBinSize;
    FreePool (VarCheckPcdBin);

    DEBUG ((EFI_D_INFO, "VarCheckPcdBin - at 0x%x size = 0x%x\n", mVarCheckPcdBin, mVarCheckPcdBinSize));

#ifdef DUMP_VAR_CHECK_PCD
    DEBUG_CODE (
      DumpVarCheckPcd (mVarCheckPcdBin, mVarCheckPcdBinSize);
    );
#endif
  } else {
    DEBUG ((EFI_D_INFO, "[VarCheckPcd] No VarCheckPcdBin found at the first RAW section\n"));
  }
}

/**
  Constructor function of VarCheckPcdLib to register var check PCD handler.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The constructor executed correctly.

**/
EFI_STATUS
EFIAPI
VarCheckPcdLibNullClassConstructor (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  LocateVarCheckPcdBin ();
  VarCheckLibRegisterAddressPointer ((VOID **) &mVarCheckPcdBin);
  VarCheckLibRegisterSetVariableCheckHandler (SetVariableCheckHandlerPcd);

  return EFI_SUCCESS;
}
