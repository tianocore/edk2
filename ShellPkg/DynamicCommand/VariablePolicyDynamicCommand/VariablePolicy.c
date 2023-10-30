/** @file
  Main file for the "varpolicy" dynamic UEFI shell command and application.

  This feature can provide detailed UEFI variable policy configuration
  information in the UEFI shell.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VariablePolicy.h"

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/ShellLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>

#include <Protocol/HiiPackageList.h>
#include <Protocol/VariablePolicy.h>

#define VAR_POLICY_FLAG_STATS_STR    L"-s"
#define VAR_POLICY_FLAG_POLICY_STR   L"-p"
#define VAR_POLICY_FLAG_VERBOSE_STR  L"-v"

#define VAR_POLICY_CMD_MIN_ATTR_STR_LEN  64

EFI_HII_HANDLE  mVarPolicyShellCommandHiiHandle = NULL;

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { VAR_POLICY_FLAG_POLICY_STR,  TypeFlag },
  { VAR_POLICY_FLAG_STATS_STR,   TypeFlag },
  { VAR_POLICY_FLAG_VERBOSE_STR, TypeFlag },
  { NULL,                        TypeMax  }
};

STATIC CONST VAR_POLICY_CMD_VAR_NAMESPACE  mVarNamespaces[] = {
  {
    VariableVendorCapsule,
    &gEfiCapsuleVendorGuid,
    L"Capsule"
  },
  {
    VariableVendorCapsuleReport,
    &gEfiCapsuleReportGuid,
    L"Capsule Reporting"
  },
  {
    VariableVendorGlobal,
    &gEfiGlobalVariableGuid,
    L"UEFI Global"
  },
  {
    VariableVendorMemoryTypeInfo,
    &gEfiMemoryTypeInformationGuid,
    L"Memory Type Information"
  },
  {
    VariableVendorMonotonicCounter,
    &gMtcVendorGuid,
    L"Monotonic Counter"
  },
  {
    VariableVendorMorControl,
    &gEfiMemoryOverwriteRequestControlLockGuid,
    L"Memory Overwrite Request (MOR) Control Lock"
  },
  {
    VariableVendorShell,
    &gShellVariableGuid,
    L"UEFI Shell"
  },
  {
    VariableVendorShell,
    &gShellAliasGuid,
    L"UEFI Shell Alias"
  }
};

/**
  Returns UEFI variable attribute information in a string.

  AttributesStrSize must at least be VAR_POLICY_CMD_MIN_ATTR_STR_LEN in length
  or EFI_INVALID_PARAMETER will be returned.

  @param[in]  Attributes             The UEFI variable attributes.
  @param[in]  AttributesStrSize      The size, in bytes, of AttributesStr.
  @param[out] AttributesStr          The Unicode string for the given attributes.

  @retval EFI_SUCCESS           The attributes were converted to a string successfully.
  @retval EFI_INVALID_PARAMETER The AttributesStr pointer is NULL.

**/
EFI_STATUS
GetAttributesString (
  IN  UINT32  Attributes,
  IN  UINTN   AttributesStrSize,
  OUT CHAR16  *AttributesStr
  )
{
  if ((AttributesStr == NULL) || (AttributesStrSize < VAR_POLICY_CMD_MIN_ATTR_STR_LEN)) {
    return EFI_INVALID_PARAMETER;
  }

  AttributesStr[0] = L'0';
  AttributesStr[1] = L'x';
  AttributesStr[2] = L'\0';

  UnicodeValueToStringS (AttributesStr + 2, AttributesStrSize - 2, (RADIX_HEX), (INT64)Attributes, 30);

  if (Attributes == 0) {
    StrCatS (AttributesStr, AttributesStrSize, L" No Attributes");
  } else {
    if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == EFI_VARIABLE_NON_VOLATILE) {
      StrCatS (AttributesStr, AttributesStrSize, L" NV");
      Attributes ^= EFI_VARIABLE_NON_VOLATILE;
    }

    if ((Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS) == EFI_VARIABLE_BOOTSERVICE_ACCESS) {
      StrCatS (AttributesStr, AttributesStrSize, L" BS");
      Attributes ^= EFI_VARIABLE_BOOTSERVICE_ACCESS;
    }

    if ((Attributes & EFI_VARIABLE_RUNTIME_ACCESS) == EFI_VARIABLE_RUNTIME_ACCESS) {
      StrCatS (AttributesStr, AttributesStrSize, L" RT");
      Attributes ^= EFI_VARIABLE_RUNTIME_ACCESS;
    }

    if ((Attributes & EFI_VARIABLE_HARDWARE_ERROR_RECORD) == EFI_VARIABLE_HARDWARE_ERROR_RECORD) {
      StrCatS (AttributesStr, AttributesStrSize, L" HW-Error");
      Attributes ^= EFI_VARIABLE_HARDWARE_ERROR_RECORD;
    }

    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
      StrCatS (AttributesStr, AttributesStrSize, L" Auth-WA");
      Attributes ^= EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS;
    }

    if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) {
      StrCatS (AttributesStr, AttributesStrSize, L" Auth-TIME-WA");
      Attributes ^= EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;
    }

    if ((Attributes & EFI_VARIABLE_APPEND_WRITE) == EFI_VARIABLE_APPEND_WRITE) {
      StrCatS (AttributesStr, AttributesStrSize, L" APPEND-W");
      Attributes ^= EFI_VARIABLE_APPEND_WRITE;
    }

    if (Attributes != 0) {
      StrCatS (AttributesStr, AttributesStrSize, L" <Unknown Attribute>");
    }
  }

  return EFI_SUCCESS;
}

/**
  Prints UEFI variable statistics information.

  @param[in] TotalVariables             Total number of UEFI variables discovered.
  @param[in] TotalVariablesSize         Total size of UEFI variables discovered.

**/
VOID
PrintStats (
  IN  UINTN  TotalVariables,
  IN  UINTN  TotalVariablesSize
  )
{
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_HEADER_1), mVarPolicyShellCommandHiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_HEADER_2), mVarPolicyShellCommandHiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_HEADER_1), mVarPolicyShellCommandHiiHandle);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_TOTAL_VARS), mVarPolicyShellCommandHiiHandle, TotalVariables);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_TOTAL_SIZE), mVarPolicyShellCommandHiiHandle, TotalVariablesSize, TotalVariablesSize);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_STATS_HEADER_1), mVarPolicyShellCommandHiiHandle);
}

/**
  Returns information for the given variable namespace if available.

  @param[in]  VariableGuid      The UEFI variable vendor (namespace) GUID.

  @return     Pointer to a namespace info structure on a GUID match.
  @return     NULL on lack of a GUID match.

**/
CONST VAR_POLICY_CMD_VAR_NAMESPACE *
GetNameSpaceInfo (
  IN  EFI_GUID  *VariableGuid
  )
{
  UINTN  Index;

  if (VariableGuid == NULL) {
    ASSERT (VariableGuid != NULL);
    return NULL;
  }

  for (Index = 0; Index < ARRAY_SIZE (mVarNamespaces); Index++) {
    if (CompareGuid (mVarNamespaces[Index].VendorGuid, VariableGuid)) {
      return &mVarNamespaces[Index];
    }
  }

  return NULL;
}

/**
  Print non-verbose information about the variable.

  @param[in]    VariableName          A pointer the Unicode variable name.
  @param[in]    VariableGuid          A pointer to the variable vendor GUID.
  @param[in]    VariableSize          The size of the UEFI variable in bytes.
  @param[in]    VariableAttributes    The UEFI variable attributes.

  @retval   EFI_SUCCESS               The non-verbose variable information was printed successfully.
  @retval   EFI_INVALID_PARAMETER     A pointer argument passed to the function was NULL.
  @retval   EFI_OUT_OF_RESOURCES      Insufficient memory resources to print the attributes.

**/
EFI_STATUS
PrintNonVerboseVarInfo (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VariableGuid,
  IN UINTN     VariableSize,
  IN UINT32    VariableAttributes
  )
{
  EFI_STATUS                          Status;
  CHAR16                              *AttributesStr;
  CHAR16                              *DescriptionStr;
  CONST VAR_POLICY_CMD_VAR_NAMESPACE  *CmdVarNamespace;

  AttributesStr  = NULL;
  DescriptionStr = NULL;

  if ((VariableName == NULL) || (VariableGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  CmdVarNamespace = GetNameSpaceInfo (VariableGuid);

  if (CmdVarNamespace == NULL) {
    DescriptionStr = AllocatePages (1);
    if (DescriptionStr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    ZeroMem ((VOID *)DescriptionStr, EFI_PAGES_TO_SIZE (1));
    UnicodeSPrint (DescriptionStr, EFI_PAGES_TO_SIZE (1), L"Unknown Vendor (%g)", VariableGuid);
  } else {
    DescriptionStr = CmdVarNamespace->Description;
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_VAR_TYPE), mVarPolicyShellCommandHiiHandle, DescriptionStr);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_VAR_NAME), mVarPolicyShellCommandHiiHandle, VariableName);
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_VAR_SIZE), mVarPolicyShellCommandHiiHandle, VariableSize, VariableSize);

  AttributesStr = AllocatePages (1);
  if (AttributesStr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  ZeroMem ((VOID *)AttributesStr, EFI_PAGES_TO_SIZE (1));
  Status = GetAttributesString (VariableAttributes, EFI_PAGES_TO_SIZE (1), AttributesStr);
  if (Status == EFI_SUCCESS) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_VAR_POL_VAR_ATTR),
      mVarPolicyShellCommandHiiHandle,
      AttributesStr
      );
  }

  Status = EFI_SUCCESS;

Exit:
  if (AttributesStr != NULL) {
    FreePages (AttributesStr, 1);
  }

  if ((CmdVarNamespace == NULL) && (DescriptionStr != NULL)) {
    FreePages (DescriptionStr, 1);
  }

  return Status;
}

/**
  Print verbose information about the variable.

  @param[in]    Data                  A pointer to the variable data buffer.
  @param[in]    DataSize              The size of data, in bytes, in the variable data buffer.

  @retval   EFI_SUCCESS               The verbose variable information was printed successfully.
  @retval   EFI_INVALID_PARAMETER     A pointer argument passed to the function was NULL.

**/
EFI_STATUS
PrintVerboseVarInfo (
  IN  VOID   *Data,
  IN  UINTN  DataSize
  )
{
  if ((DataSize == 0) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  VAR_POLICY_CMD_SHELL_DUMP_HEX (0, Data, DataSize);

  return EFI_SUCCESS;
}

/**
  Prints variable policy information for the given variable.

  @param[in]  VariableName    A pointer to the Unicode string of the UEFI variable name.
  @param[in]  VendorGuid      A pointer to the UEFI variable vendor GUID.

  @return TRUE if a variable policy was found and printed for the variable.
  @return FALSE if an error occurred and/or a variable policy was not found and
          printed for the variable.

**/
BOOLEAN
PrintVariablePolicyInfo (
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS                         Status;
  VARIABLE_POLICY_ENTRY              VariablePolicyEntry;
  VARIABLE_LOCK_ON_VAR_STATE_POLICY  LockOnVarStatePolicy;
  UINTN                              VariablePolicyVariableNameBufferSize;
  UINTN                              ReturnedVariableNameSize;
  BOOLEAN                            PolicyHeaderPresent;
  CHAR16                             *VariablePolicyVariableName;
  CHAR16                             *VariableAttributesStr;
  EDKII_VARIABLE_POLICY_PROTOCOL     *VariablePolicy;

  PolicyHeaderPresent        = FALSE;
  VariableAttributesStr      = NULL;
  VariablePolicyVariableName = NULL;

  if ((VariableName == NULL) || (VendorGuid == NULL)) {
    ASSERT ((VariableName != NULL) && (VendorGuid != NULL));
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_INT_ERR), mVarPolicyShellCommandHiiHandle);
    return FALSE;
  }

  Status = gBS->LocateProtocol (&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID **)&VariablePolicy);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_NO_PROT), mVarPolicyShellCommandHiiHandle);
    return FALSE;
  }

  VariablePolicyVariableNameBufferSize = EFI_PAGES_TO_SIZE (1);
  VariablePolicyVariableName           = AllocatePages (EFI_SIZE_TO_PAGES (VariablePolicyVariableNameBufferSize));
  if (VariablePolicyVariableName == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    ASSERT_EFI_ERROR (Status);
    goto Done;
  }

  ZeroMem (VariablePolicyVariableName, VariablePolicyVariableNameBufferSize);
  ReturnedVariableNameSize = VariablePolicyVariableNameBufferSize;
  Status                   =  VariablePolicy->GetVariablePolicyInfo (
                                                VariableName,
                                                VendorGuid,
                                                &ReturnedVariableNameSize,
                                                &VariablePolicyEntry,
                                                VariablePolicyVariableName
                                                );
  if (Status == EFI_NOT_READY) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_NOT_INIT), mVarPolicyShellCommandHiiHandle);
  } else if (Status == EFI_NOT_FOUND) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_NOT_FOUND), mVarPolicyShellCommandHiiHandle);
  } else if (EFI_ERROR (Status)) {
    // A different error return code is not expected
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_UNEXP_ERR), mVarPolicyShellCommandHiiHandle, Status);
  } else {
    PolicyHeaderPresent = TRUE;
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_HEADER_1), mVarPolicyShellCommandHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_HEADER_2), mVarPolicyShellCommandHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_HEADER_1), mVarPolicyShellCommandHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_VERSION), mVarPolicyShellCommandHiiHandle, VariablePolicyEntry.Version);

    if ((ReturnedVariableNameSize > 0) && (VariablePolicyVariableName[0] != CHAR_NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_VARIABLE), mVarPolicyShellCommandHiiHandle, VariablePolicyVariableName);
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_VARIABLE), mVarPolicyShellCommandHiiHandle, L"<Entire Namespace>");
    }

    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_NAMESPACE), mVarPolicyShellCommandHiiHandle, &VariablePolicyEntry.Namespace);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_MIN_SIZE), mVarPolicyShellCommandHiiHandle, VariablePolicyEntry.MinSize);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_MAX_SIZE), mVarPolicyShellCommandHiiHandle, VariablePolicyEntry.MaxSize);

    switch (VariablePolicyEntry.LockPolicyType) {
      case VARIABLE_POLICY_TYPE_NO_LOCK:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_LOCK_TYPE), mVarPolicyShellCommandHiiHandle, L"No Lock");
        break;
      case VARIABLE_POLICY_TYPE_LOCK_NOW:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_LOCK_TYPE), mVarPolicyShellCommandHiiHandle, L"Lock Now");
        break;
      case VARIABLE_POLICY_TYPE_LOCK_ON_CREATE:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_LOCK_TYPE), mVarPolicyShellCommandHiiHandle, L"On Create");
        break;
      case VARIABLE_POLICY_TYPE_LOCK_ON_VAR_STATE:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_LOCK_TYPE), mVarPolicyShellCommandHiiHandle, L"On Variable State");

        ZeroMem (VariablePolicyVariableName, VariablePolicyVariableNameBufferSize);
        ReturnedVariableNameSize = VariablePolicyVariableNameBufferSize;
        Status                   =  VariablePolicy->GetLockOnVariableStateVariablePolicyInfo (
                                                      VariableName,
                                                      VendorGuid,
                                                      &ReturnedVariableNameSize,
                                                      &LockOnVarStatePolicy,
                                                      VariablePolicyVariableName
                                                      );
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_UNEXP_ERR), mVarPolicyShellCommandHiiHandle, Status);
          goto Done;
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_STATE_NS), mVarPolicyShellCommandHiiHandle, &LockOnVarStatePolicy.Namespace);
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_STATE_VAL), mVarPolicyShellCommandHiiHandle, LockOnVarStatePolicy.Value);
          if ((ReturnedVariableNameSize > 0) && (VariablePolicyVariableName[0] != CHAR_NULL)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_STATE_NAME), mVarPolicyShellCommandHiiHandle, VariablePolicyVariableName);
          } else {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_STATE_NAME), mVarPolicyShellCommandHiiHandle, L"<Entire Namespace>");
          }
        }

        break;
      default:
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_LOCK_TYPE), mVarPolicyShellCommandHiiHandle, L"Unknown");
        break;
    }

    VariableAttributesStr = AllocatePages (1);
    if (VariableAttributesStr == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      ASSERT_EFI_ERROR (Status);
      goto Done;
    }

    ZeroMem (VariableAttributesStr, EFI_PAGES_TO_SIZE (1));
    Status = GetAttributesString (VariablePolicyEntry.AttributesMustHave, EFI_PAGES_TO_SIZE (1), VariableAttributesStr);
    if (Status == EFI_SUCCESS) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_VAR_POL_POLICY_ATTR_MUST),
        mVarPolicyShellCommandHiiHandle
        );
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_VAR_POL_POLICY_ATTR_GEN),
        mVarPolicyShellCommandHiiHandle,
        VariableAttributesStr
        );
    }

    ZeroMem (VariableAttributesStr, EFI_PAGES_TO_SIZE (1));
    Status = GetAttributesString (VariablePolicyEntry.AttributesCantHave, EFI_PAGES_TO_SIZE (1), VariableAttributesStr);
    if (Status == EFI_SUCCESS) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_VAR_POL_POLICY_ATTR_NOT),
        mVarPolicyShellCommandHiiHandle
        );
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_VAR_POL_POLICY_ATTR_GEN),
        mVarPolicyShellCommandHiiHandle,
        VariableAttributesStr
        );
    }
  }

Done:
  if (PolicyHeaderPresent) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VAR_POL_POLICY_HEADER_1), mVarPolicyShellCommandHiiHandle);
  }

  if (VariableAttributesStr != NULL) {
    FreePages (VariableAttributesStr, 1);
  }

  if (VariablePolicyVariableName != NULL) {
    FreePages (VariablePolicyVariableName, 1);
  }

  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_LINE_BREAK), mVarPolicyShellCommandHiiHandle);

  return Status == EFI_SUCCESS;
}

/**
  Gets the next UEFI variable name.

  This buffer manages the UEFI variable name buffer, performing memory reallocations as necessary.

  Note: The first time this function is called, VariableNameBufferSize must be 0 and
  the VariableName buffer pointer must point to NULL.

  @param[in,out] VariableNameBufferSize   On input, a pointer to a buffer that holds the current
                                          size of the VariableName buffer in bytes.
                                          On output, a pointer to a buffer that holds the updated
                                          size of the VariableName buffer in bytes.
  @param[in,out] VariableName             On input, a pointer to a pointer to a buffer that holds the
                                          current UEFI variable name.
                                          On output, a pointer to a pointer to a buffer that holds the
                                          next UEFI variable name.
  @param[in,out] VariableGuid             On input, a pointer to a buffer that holds the current UEFI
                                          variable GUID.
                                          On output, a pointer to a buffer that holds the next UEFI
                                          variable GUID.

  @retval    EFI_SUCCESS              The next UEFI variable name was found successfully.
  @retval    EFI_INVALID_PARAMETER    A pointer argument is NULL or initial input values are invalid.
  @retval    EFI_OUT_OF_RESOURCES     Insufficient memory resources to allocate a required buffer.
  @retval    Others                   Return status codes from the UEFI spec define GetNextVariableName() interface.

**/
EFI_STATUS
GetNextVariableNameWithDynamicReallocation (
  IN  OUT UINTN     *VariableNameBufferSize,
  IN  OUT CHAR16    **VariableName,
  IN  OUT EFI_GUID  *VariableGuid
  )
{
  EFI_STATUS  Status;
  UINTN       NextVariableNameBufferSize;

  if ((VariableNameBufferSize == NULL) || (VariableName == NULL) || (VariableGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (*VariableNameBufferSize == 0) {
    if (*VariableName != NULL) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // Allocate a buffer to temporarily hold variable names. To reduce memory
    // allocations, the default buffer size is 256 characters. The buffer can
    // be reallocated if expansion is necessary (should be very rare).
    //
    *VariableNameBufferSize = sizeof (CHAR16) * 256;
    *VariableName           = AllocateZeroPool (*VariableNameBufferSize);
    if (*VariableName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem ((VOID *)VariableGuid, sizeof (EFI_GUID));
  }

  NextVariableNameBufferSize = *VariableNameBufferSize;
  Status                     = gRT->GetNextVariableName (
                                      &NextVariableNameBufferSize,
                                      *VariableName,
                                      VariableGuid
                                      );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    *VariableName = ReallocatePool (
                      *VariableNameBufferSize,
                      NextVariableNameBufferSize,
                      *VariableName
                      );
    if (*VariableName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *VariableNameBufferSize = NextVariableNameBufferSize;

    Status = gRT->GetNextVariableName (
                    &NextVariableNameBufferSize,
                    *VariableName,
                    VariableGuid
                    );
    ASSERT (Status != EFI_BUFFER_TOO_SMALL);
  }

  return Status;
}

/**
  Dumps UEFI variable information.

  This is the main function that enumerates UEFI variables and prints the information
  selected by the user.

  @param[in] Verbose            Whether to print verbose information.
  @param[in] Stats              Whether to print statistical information.
  @param[in] PolicyCheck        Whether to print variable policy related information.


  @retval    EFI_SUCCESS              The UEFI variable information was dumped successfully.
  @retval    EFI_DEVICE_ERROR         An error occurred attempting to get UEFI variable information.
  @retval    EFI_OUT_OF_RESOURCES     Insufficient memory resources to allocate a required buffer.

**/
EFI_STATUS
DumpVars (
  IN  BOOLEAN  Verbose,
  IN  BOOLEAN  Stats,
  IN  BOOLEAN  PolicyCheck
  )
{
  EFI_STATUS  Status;
  EFI_STATUS  GetNextVariableStatus;
  UINT32      Attributes;
  UINTN       CurrentVariableDataBufferSize;
  UINTN       DataSize;
  UINTN       TotalDataSize;
  UINTN       TotalVariables;
  UINTN       TotalVariablesWithPolicy;
  UINTN       VariableNameBufferSize;
  EFI_GUID    VariableGuid;
  CHAR16      *VariableName;
  VOID        *Data;

  Status                        = EFI_SUCCESS;
  Data                          = NULL;
  VariableName                  = NULL;
  CurrentVariableDataBufferSize = 0;
  TotalDataSize                 = 0;
  TotalVariables                = 0;
  TotalVariablesWithPolicy      = 0;
  VariableNameBufferSize        = 0;

  do {
    GetNextVariableStatus = GetNextVariableNameWithDynamicReallocation (
                              &VariableNameBufferSize,
                              &VariableName,
                              &VariableGuid
                              );

    if (!EFI_ERROR (GetNextVariableStatus)) {
      DataSize = 0;
      Status   = gRT->GetVariable (
                        VariableName,
                        &VariableGuid,
                        &Attributes,
                        &DataSize,
                        NULL
                        );
      if (Status != EFI_BUFFER_TOO_SMALL) {
        // If the variable exists, a zero size buffer should be too small
        Status = EFI_DEVICE_ERROR;
        goto DeallocateAndExit;
      }

      TotalDataSize += DataSize;
      TotalVariables++;

      if (!Stats || Verbose) {
        Status = PrintNonVerboseVarInfo (VariableName, &VariableGuid, DataSize, Attributes);
        if (!EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_LINE_BREAK), mVarPolicyShellCommandHiiHandle);
        }
      }

      if (PolicyCheck || Verbose) {
        if (PrintVariablePolicyInfo (VariableName, &VariableGuid)) {
          TotalVariablesWithPolicy++;
        }
      }

      if (Verbose) {
        if (CurrentVariableDataBufferSize < DataSize) {
          if (Data != NULL) {
            FreePool (Data);
          }

          Data = AllocateZeroPool (DataSize);
          if (Data == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DeallocateAndExit;
          }

          CurrentVariableDataBufferSize = DataSize;
        }

        Status = gRT->GetVariable (
                        VariableName,
                        &VariableGuid,
                        NULL,
                        &DataSize,
                        Data
                        );
        if (EFI_ERROR (Status)) {
          Status = EFI_DEVICE_ERROR;
          goto DeallocateAndExit;
        }

        Status = PrintVerboseVarInfo (Data, DataSize);
        if (EFI_ERROR (Status)) {
          Status = EFI_DEVICE_ERROR;
          goto DeallocateAndExit;
        }
      }
    }
  } while (!EFI_ERROR (GetNextVariableStatus));

  if (TotalVariables == 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VARS), mVarPolicyShellCommandHiiHandle);
  } else {
    if (Verbose || Stats) {
      PrintStats (TotalVariables, TotalDataSize);
    }

    if (Verbose || PolicyCheck) {
      ASSERT (TotalVariablesWithPolicy <= TotalVariables);

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_LINE_BREAK), mVarPolicyShellCommandHiiHandle);
      if (TotalVariablesWithPolicy == TotalVariables) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_VAR_POL_POLICY_STATS_PASS),
          mVarPolicyShellCommandHiiHandle,
          TotalVariablesWithPolicy,
          TotalVariables
          );
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_VAR_POL_POLICY_STATS_FAIL),
          mVarPolicyShellCommandHiiHandle,
          TotalVariablesWithPolicy,
          TotalVariables
          );
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_LINE_BREAK), mVarPolicyShellCommandHiiHandle);
    }
  }

  Status = EFI_SUCCESS;

DeallocateAndExit:
  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  if (Data != NULL) {
    FreePool (Data);
  }

  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Main entry function for the "varpolicy" command/app.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval   SHELL_SUCCESS               The "varpolicy" shell command executed successfully.
  @retval   SHELL_ABORTED               Failed to initialize the shell library.
  @retval   SHELL_INVALID_PARAMETER     An argument passed to the shell command is invalid.
  @retval   Others                      A different error occurred.

**/
SHELL_STATUS
EFIAPI
RunVarPolicy (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  SHELL_STATUS  ShellStatus;
  BOOLEAN       PolicyCheck;
  BOOLEAN       StatsDump;
  BOOLEAN       VerboseDump;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;

  Package     = NULL;
  ShellStatus = SHELL_INVALID_PARAMETER;
  Status      = EFI_SUCCESS;
  PolicyCheck = FALSE;
  StatsDump   = FALSE;
  VerboseDump = FALSE;

  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return SHELL_ABORTED;
  }

  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), mVarPolicyShellCommandHiiHandle, VAR_POLICY_COMMAND_NAME, ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), mVarPolicyShellCommandHiiHandle, VAR_POLICY_COMMAND_NAME);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    PolicyCheck = ShellCommandLineGetFlag (Package, VAR_POLICY_FLAG_POLICY_STR);
    StatsDump   = ShellCommandLineGetFlag (Package, VAR_POLICY_FLAG_STATS_STR);
    VerboseDump = ShellCommandLineGetFlag (Package, VAR_POLICY_FLAG_VERBOSE_STR);

    Status = DumpVars (VerboseDump, StatsDump, PolicyCheck);
    ASSERT_EFI_ERROR (Status);
  }

Done:
  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return ShellStatus;
}

/**
  Retrieve HII package list from ImageHandle and publish to HII database.

  @param[in] ImageHandle    The image handle of the process.

  @return HII handle.

**/
EFI_HII_HANDLE
InitializeHiiPackage (
  IN  EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_HII_HANDLE               HiiHandle;

  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase,
                           PackageList,
                           NULL,
                           &HiiHandle
                           );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return HiiHandle;
}
