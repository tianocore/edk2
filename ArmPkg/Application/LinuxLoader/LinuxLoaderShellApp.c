/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "LinuxLoader.h"

//
// Internal variables
//

CONST EFI_GUID mLinuxLoaderHiiGuid = {
                         0xd5d16edc, 0x35c5, 0x4866,
                         {0xbd, 0xe5, 0x2b, 0x64, 0xa2, 0x26, 0x55, 0x6e}
                         };
EFI_HANDLE mLinuxLoaderHiiHandle;

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-f", TypeValue},
  {L"-d", TypeValue},
  {L"-c", TypeValue},
  {L"-a", TypeValue},
  {NULL , TypeMax  }
  };

/**
  Print a string given the "HII Id" of the format string and a list of
  arguments.

  @param[in] Language           The language of the string to retrieve. If
                                this parameter is NULL, then the current
                                platform language is used.
  @param[in] HiiFormatStringId  The format string Id for getting from Hii.
  @param[in] ...                The variable argument list.

  @retval EFI_SUCCESS           The printing was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.

**/
EFI_STATUS
PrintHii (
  IN CONST CHAR8          *Language OPTIONAL,
  IN CONST EFI_STRING_ID  HiiFormatStringId,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   *HiiFormatString;
  CHAR16   Buffer[MAX_MSG_LEN];

  VA_START (Marker, HiiFormatStringId);

  HiiFormatString = HiiGetString (mLinuxLoaderHiiHandle, HiiFormatStringId, Language);
  if (HiiFormatString != NULL) {
    UnicodeVSPrint (Buffer, sizeof (Buffer), HiiFormatString, Marker);
    Print (L"%s", Buffer);
    FreePool (HiiFormatString);
  } else {
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  VA_END (Marker);

  return EFI_SUCCESS;
}

/**
  Print the help.

  @param[in]  Language  The language of the string to retrieve.  If this
                        parameter is NULL, then the current platform
                        language is used.
**/
VOID
PrintHelp (
  IN CONST CHAR8  *Language OPTIONAL
  )
{
  CHAR16  *Help;
  CHAR16  *Walker;
  CHAR16  *LineEnd;

  //
  // Print the help line by line as it is too big to be printed at once.
  //

  Help = HiiGetString (mLinuxLoaderHiiHandle, STRING_TOKEN (STR_HELP), Language);
  if (Help != NULL) {
    Walker = Help;
    while (*Walker != L'\0') {
      LineEnd = StrStr (Walker, L"\r\n");
      if (LineEnd != NULL) {
        *LineEnd = L'\0';
      }
      Print (L"%s\r\n", Walker);
      if (LineEnd == NULL) {
        break;
      }
      Walker = LineEnd + 2;
    }
    FreePool (Help);
  }

}

/**
  Process the Shell parameters in the case the application has been called
  from the EFI Shell.

  @param[out]  KernelPath        A pointer to the buffer where the path
                                 to the Linux kernel (EFI Shell file path
                                 or device path is stored. The address of
                                 the buffer is NULL in case of an error.
                                 Otherwise, the returned address is the
                                 address of a buffer allocated with
                                 a call to AllocatePool() that has to be
                                 freed by the caller.
  @param[out]  FdtPath           A pointer to the buffer where the path
                                 to the FDT (EFI Shell file path or
                                 device path) is stored. The address of
                                 the buffer is NULL in case of an error or
                                 if the path to the FDT is not defined.
                                 Otherwise, the returned address is the
                                 address of a buffer allocated with a call
                                 to AllocatePool() that has to be freed by
                                 the caller.
  @param[out]  InitrdPath        A pointer to the buffer where the path
                                 (EFI Shell file path or device path)
                                 to the RAM root file system is stored.
                                 The address of the buffer is NULL in case
                                 of an error or if the path to the RAM root
                                 file system is not defined. Otherwise, the
                                 returned address is the address of a
                                 buffer allocated with a call to
                                 AllocatePool() that has to be freed by
                                 the caller.
  @param[out]  LinuxCommandLine  A pointer to the buffer where the Linux
                                 kernel command line is stored. The address
                                 of the buffer is NULL in case of an error
                                 or if the Linux command line is not
                                 defined. Otherwise, the returned address
                                 is the address of a buffer allocated with
                                 a call to AllocatePool() that has to be
                                 freed by the caller.
  @param[out]  AtagMachineType   Value of the ARM Machine Type

  @retval  EFI_SUCCESS            The processing was successfull.
  @retval  EFI_ABORTED            The initialisation of the Shell Library failed.
  @retval  EFI_NOT_FOUND          Path to the Linux kernel not found.
  @retval  EFI_INVALID_PARAMETER  At least one parameter is not valid or there is a
                                  conflict between two parameters.
  @retval  EFI_OUT_OF_RESOURCES   A memory allocation failed.

**/
EFI_STATUS
ProcessShellParameters (
  OUT  CHAR16   **KernelPath,
  OUT  CHAR16   **FdtPath,
  OUT  CHAR16   **InitrdPath,
  OUT  CHAR16   **LinuxCommandLine,
  OUT  UINTN    *AtagMachineType
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *CheckPackage;
  CHAR16        *ProblemParam;
  CONST CHAR16  *FlagValue;
  CONST CHAR16  *ParameterValue;
  UINTN         LinuxCommandLineLen;


  *KernelPath       = NULL;
  *FdtPath          = NULL;
  *InitrdPath       = NULL;
  *LinuxCommandLine = NULL;
  *AtagMachineType  = ARM_FDT_MACHINE_TYPE;

  //
  // Initialise the Shell Library as we are going to use it.
  // Assert that the return code is EFI_SUCCESS as it should.
  // To anticipate any change is the codes returned by
  // ShellInitialize(), leave in case of error.
  //
  Status = ShellInitialize ();
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return EFI_ABORTED;
  }

  Status = ShellCommandLineParse (ParamList, &CheckPackage, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) &&
        (ProblemParam != NULL) ) {
      PrintHii (
        NULL, STRING_TOKEN (STR_SHELL_INVALID_PARAMETER), ProblemParam
        );
      FreePool (ProblemParam);
    } else {
      ASSERT (FALSE);
    }
    goto Error;
  }

  Status = EFI_INVALID_PARAMETER;
  if (ShellCommandLineGetCount (CheckPackage) != 2) {
    PrintHelp (NULL);
    goto Error;
  }

  Status = EFI_OUT_OF_RESOURCES;

  FlagValue = ShellCommandLineGetValue (CheckPackage, L"-a");
  if (FlagValue != NULL) {
    if (ShellCommandLineGetFlag (CheckPackage, L"-d")) {
      PrintHii (NULL, STRING_TOKEN (STR_ATAG_FDT_CONFLICT));
      goto Error;
    }
    *AtagMachineType = StrDecimalToUintn (FlagValue);
  }

  ParameterValue = ShellCommandLineGetRawValue (CheckPackage, 1);
  *KernelPath = AllocateCopyPool (StrSize (ParameterValue), ParameterValue);
  if (*KernelPath == NULL) {
    goto Error;
  }

  FlagValue = ShellCommandLineGetValue (CheckPackage, L"-d");
  if (FlagValue != NULL) {
    *FdtPath = AllocateCopyPool (StrSize (FlagValue), FlagValue);
    if (*FdtPath == NULL) {
      goto Error;
    }
  }

  FlagValue = ShellCommandLineGetValue (CheckPackage, L"-f");
  if (FlagValue != NULL) {
    *InitrdPath = AllocateCopyPool (StrSize (FlagValue), FlagValue);
    if (*InitrdPath == NULL) {
      goto Error;
    }
  }

  FlagValue = ShellCommandLineGetValue (CheckPackage, L"-c");
  if (FlagValue != NULL) {
    LinuxCommandLineLen = StrLen (FlagValue);
    if ((LinuxCommandLineLen != 0) &&
        (FlagValue[0] == L'"'    ) &&
        (FlagValue[LinuxCommandLineLen - 1] == L'"')) {
      FlagValue++;
      LinuxCommandLineLen -= 2;
    }

    *LinuxCommandLine = AllocateCopyPool (
                          (LinuxCommandLineLen + 1) * sizeof (CHAR16),
                          FlagValue
                          );
    if (*LinuxCommandLine == NULL) {
      goto Error;
    }
    (*LinuxCommandLine)[LinuxCommandLineLen] = L'\0';
  }

  Status = EFI_SUCCESS;

Error:
  ShellCommandLineFreeVarList (CheckPackage);

  if (EFI_ERROR (Status)) {
    if (*KernelPath != NULL) {
      FreePool (*KernelPath);
      *KernelPath = NULL;
    }
    if (*FdtPath != NULL) {
      FreePool (*FdtPath);
      *FdtPath = NULL;
    }
    if (*InitrdPath != NULL) {
      FreePool (*InitrdPath);
      *InitrdPath = NULL;
    }
    if (*LinuxCommandLine != NULL) {
      FreePool (*LinuxCommandLine);
      *LinuxCommandLine = NULL;
    }
  }

  return Status;
}
