/** @file
  Measure TCG required variable.

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Guid/ImageAuthentication.h>
#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/TpmMeasurementLib.h>

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
} VARIABLE_TYPE;

typedef struct {
  CHAR16      *VariableName;
  EFI_GUID    *VendorGuid;
  VOID        *Data;
  UINTN       Size;
} VARIABLE_RECORD;

#define  MEASURED_AUTHORITY_COUNT_MAX  0x100

UINTN            mMeasuredAuthorityCount    = 0;
UINTN            mMeasuredAuthorityCountMax = 0;
VARIABLE_RECORD  *mMeasuredAuthorityList    = NULL;

VARIABLE_TYPE  mVariableType[] = {
  { EFI_IMAGE_SECURITY_DATABASE, &gEfiImageSecurityDatabaseGuid },
};

/**
  This function will check if VarName should be recorded and return the address of VarName if it is needed.

  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.

  @return the address of VarName.
**/
CHAR16 *
AssignVarName (
  IN      CHAR16  *VarName
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mVariableType)/sizeof (mVariableType[0]); Index++) {
    if (StrCmp (VarName, mVariableType[Index].VariableName) == 0) {
      return mVariableType[Index].VariableName;
    }
  }

  return NULL;
}

/**
  This function will check if VendorGuid should be recorded and return the address of VendorGuid if it is needed.

  @param[in]  VendorGuid        A unique identifier for the vendor.

  @return the address of VendorGuid.
**/
EFI_GUID *
AssignVendorGuid (
  IN      EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mVariableType)/sizeof (mVariableType[0]); Index++) {
    if (CompareGuid (VendorGuid, mVariableType[Index].VendorGuid)) {
      return mVariableType[Index].VendorGuid;
    }
  }

  return NULL;
}

/**
  This function will add variable information to MeasuredAuthorityList.

  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
**/
EFI_STATUS
AddDataMeasured (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  IN      VOID      *Data,
  IN      UINTN     Size
  )
{
  VARIABLE_RECORD  *NewMeasuredAuthorityList;

  ASSERT (mMeasuredAuthorityCount <= mMeasuredAuthorityCountMax);
  if (mMeasuredAuthorityCount == mMeasuredAuthorityCountMax) {
    //
    // Need enlarge
    //
    NewMeasuredAuthorityList = AllocateZeroPool (sizeof (VARIABLE_RECORD) * (mMeasuredAuthorityCountMax + MEASURED_AUTHORITY_COUNT_MAX));
    if (NewMeasuredAuthorityList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (mMeasuredAuthorityList != NULL) {
      CopyMem (NewMeasuredAuthorityList, mMeasuredAuthorityList, sizeof (VARIABLE_RECORD) * mMeasuredAuthorityCount);
      FreePool (mMeasuredAuthorityList);
    }

    mMeasuredAuthorityList      = NewMeasuredAuthorityList;
    mMeasuredAuthorityCountMax += MEASURED_AUTHORITY_COUNT_MAX;
  }

  //
  // Add new entry
  //
  mMeasuredAuthorityList[mMeasuredAuthorityCount].VariableName = AssignVarName (VarName);
  mMeasuredAuthorityList[mMeasuredAuthorityCount].VendorGuid   = AssignVendorGuid (VendorGuid);
  mMeasuredAuthorityList[mMeasuredAuthorityCount].Size         = Size;
  mMeasuredAuthorityList[mMeasuredAuthorityCount].Data         = AllocatePool (Size);
  if (mMeasuredAuthorityList[mMeasuredAuthorityCount].Data == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (mMeasuredAuthorityList[mMeasuredAuthorityCount].Data, Data, Size);
  mMeasuredAuthorityCount++;

  return EFI_SUCCESS;
}

/**
  This function will return if this variable is already measured.

  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval TRUE  The data is already measured.
  @retval FALSE The data is not measured yet.
**/
BOOLEAN
IsDataMeasured (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  IN      VOID      *Data,
  IN      UINTN     Size
  )
{
  UINTN  Index;

  for (Index = 0; Index < mMeasuredAuthorityCount; Index++) {
    if ((StrCmp (VarName, mMeasuredAuthorityList[Index].VariableName) == 0) &&
        (CompareGuid (VendorGuid, mMeasuredAuthorityList[Index].VendorGuid)) &&
        (CompareMem (Data, mMeasuredAuthorityList[Index].Data, Size) == 0) &&
        (Size == mMeasuredAuthorityList[Index].Size))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function will return if this variable is SecureAuthority Variable.

  @param[in]  VariableName      A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.

  @retval TRUE  This is SecureAuthority Variable
  @retval FALSE This is not SecureAuthority Variable
**/
BOOLEAN
IsSecureAuthorityVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mVariableType)/sizeof (mVariableType[0]); Index++) {
    if ((StrCmp (VariableName, mVariableType[Index].VariableName) == 0) &&
        (CompareGuid (VendorGuid, mVariableType[Index].VendorGuid)))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Measure and log an EFI variable, and extend the measurement result into a specific PCR.

  @param[in]  VarName           A Null-terminated string that is the name of the vendor's variable.
  @param[in]  VendorGuid        A unique identifier for the vendor.
  @param[in]  VarData           The content of the variable data.
  @param[in]  VarSize           The size of the variable data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_DEVICE_ERROR      The operation was unsuccessful.

**/
EFI_STATUS
EFIAPI
MeasureVariable (
  IN      CHAR16    *VarName,
  IN      EFI_GUID  *VendorGuid,
  IN      VOID      *VarData,
  IN      UINTN     VarSize
  )
{
  EFI_STATUS          Status;
  UINTN               VarNameLength;
  UEFI_VARIABLE_DATA  *VarLog;
  UINT32              VarLogSize;

  //
  // The UEFI_VARIABLE_DATA.VariableData value shall be the EFI_SIGNATURE_DATA value
  // from the EFI_SIGNATURE_LIST that contained the authority that was used to validate the image
  //
  VarNameLength = StrLen (VarName);
  VarLogSize    = (UINT32)(sizeof (*VarLog) + VarNameLength * sizeof (*VarName) + VarSize
                           - sizeof (VarLog->UnicodeName) - sizeof (VarLog->VariableData));

  VarLog = (UEFI_VARIABLE_DATA *)AllocateZeroPool (VarLogSize);
  if (VarLog == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (&VarLog->VariableName, VendorGuid, sizeof (VarLog->VariableName));
  VarLog->UnicodeNameLength  = VarNameLength;
  VarLog->VariableDataLength = VarSize;
  CopyMem (
    VarLog->UnicodeName,
    VarName,
    VarNameLength * sizeof (*VarName)
    );
  CopyMem (
    (CHAR16 *)VarLog->UnicodeName + VarNameLength,
    VarData,
    VarSize
    );

  DEBUG ((DEBUG_INFO, "DxeImageVerification: MeasureVariable (Pcr - %x, EventType - %x, ", (UINTN)7, (UINTN)EV_EFI_VARIABLE_AUTHORITY));
  DEBUG ((DEBUG_INFO, "VariableName - %s, VendorGuid - %g)\n", VarName, VendorGuid));

  Status = TpmMeasureAndLogData (
             7,
             EV_EFI_VARIABLE_AUTHORITY,
             VarLog,
             VarLogSize,
             VarLog,
             VarLogSize
             );
  FreePool (VarLog);

  return Status;
}

/**
  SecureBoot Hook for processing image verification.

  @param[in] VariableName                 Name of Variable to be found.
  @param[in] VendorGuid                   Variable vendor GUID.
  @param[in] DataSize                     Size of Data found. If size is less than the
                                          data, this value contains the required size.
  @param[in] Data                         Data pointer.

**/
VOID
EFIAPI
SecureBootHook (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_STATUS  Status;

  if (!IsSecureAuthorityVariable (VariableName, VendorGuid)) {
    return;
  }

  if (IsDataMeasured (VariableName, VendorGuid, Data, DataSize)) {
    DEBUG ((DEBUG_ERROR, "MeasureSecureAuthorityVariable - IsDataMeasured\n"));
    return;
  }

  Status = MeasureVariable (
             VariableName,
             VendorGuid,
             Data,
             DataSize
             );
  DEBUG ((DEBUG_INFO, "MeasureBootPolicyVariable - %r\n", Status));

  if (!EFI_ERROR (Status)) {
    AddDataMeasured (VariableName, VendorGuid, Data, DataSize);
  }

  return;
}
