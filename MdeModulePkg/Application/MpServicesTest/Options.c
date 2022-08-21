/** @file
  Options handling code.

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  Copyright (c) 2010-2019  Finnbarr P. Murphy.   All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/ShellParameters.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "Options.h"

STATIC UINTN   Argc;
STATIC CHAR16  **Argv;

/**

  This function provides argc and argv.

  @return Status
**/
EFI_STATUS
GetArg (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParameters
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;
  return EFI_SUCCESS;
}

/**
  Checks if the character is a decimal digit.

  @param Char The character to check.

  @return TRUE if the character is a decimal digit.
**/
BOOLEAN
IsUnicodeDecimalDigit (
  CHAR16  Char
  )
{
  return ((BOOLEAN)(Char >= L'0' && Char <= L'9'));
}

/**
  Converts the string to an integer.

  @param String The input string.
  @param Value  The converted number.

  @return EFI_SUCCESS on success, or an error code.
**/
EFI_STATUS
UnicodeStringToInteger (
  CHAR16  *String,
  UINTN   *Value
  )
{
  UINTN  Result;

  Result = 0;

  if ((String == NULL) || (StrSize (String) == 0) || (Value == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  while (IsUnicodeDecimalDigit (*String)) {
    if (!(Result <= (DivU64x32 ((((UINT64) ~0) - (*String - L'0')), 10)))) {
      return (EFI_DEVICE_ERROR);
    }

    Result = MultU64x32 (Result, 10) + (*String - L'0');
    String++;
  }

  *Value = Result;

  return (EFI_SUCCESS);
}

/**
  Print app usage.
**/
STATIC
VOID
PrintUsage (
  VOID
  )
{
  Print (L"MpServicesTest:  usage\n");
  Print (L"  MpServicesTest -A [-O]\n");
  Print (L"  MpServicesTest -T <Timeout>\n");
  Print (L"  MpServicesTest -S <Processor #>\n");
  Print (L"  MpServicesTest -P\n");
  Print (L"  MpServicesTest -U <Processor #>\n");
  Print (L"  MpServicesTest -W\n");
  Print (L"  MpServicesTest -E <Processor #>\n");
  Print (L"  MpServicesTest -D <Processor #>\n");
  Print (L"  MpServicesTest -h\n");
  Print (L"Parameter:\n");
  Print (L"  -A:  Run all APs.\n");
  Print (L"  -O:  Run APs sequentially (use with -A).\n");
  Print (L"  -T:  Specify timeout in milliseconds. Default is to wait forever.\n");
  Print (L"  -S:  Specify the single AP to run.\n");
  Print (L"  -P:  Print processor information.\n");
  Print (L"  -U:  Set the specified AP to the Unhealthy status (use with -E/-D).\n");
  Print (L"  -W:  Run WhoAmI and print index of BSP.\n");
  Print (L"  -E:  Enable the specified AP.\n");
  Print (L"  -D:  Disable the specified AP.\n");
  Print (L"  -h:  Print this help page.\n");
}

/**
  Parses any arguments provided on the command line.

  @param Options  The arguments structure.

  @return EFI_SUCCESS on success, or an error code.
**/
EFI_STATUS
ParseArguments (
  MP_SERVICES_TEST_OPTIONS  *Options
  )
{
  EFI_STATUS    Status;
  INT32         ArgIndex;
  CONST CHAR16  *Argument;
  BOOLEAN       NeedsValue;
  UINTN         *Value;

  Status = GetArg ();
  if (EFI_ERROR (Status)) {
    Print (L"Please use the UEFI Shell to run this application!\n", Status);
    return Status;
  }

  if (Argc == 1) {
    PrintUsage ();
  }

  ZeroMem (Options, sizeof (MP_SERVICES_TEST_OPTIONS));

  for (ArgIndex = 1; ArgIndex < Argc; ArgIndex++) {
    Argument   = Argv[ArgIndex];
    NeedsValue = FALSE;

    if (StrCmp (Argument, L"-A") == 0) {
      Options->RunAllAPs = TRUE;
    } else if (StrCmp (Argument, L"-O") == 0) {
      Options->RunAPsSequentially = TRUE;
    } else if (StrCmp (Argument, L"-T") == 0) {
      NeedsValue = TRUE;
      Value      = &Options->Timeout;
    } else if (StrCmp (Argument, L"-S") == 0) {
      Options->RunSingleAP = TRUE;
      NeedsValue           = TRUE;
      Value                = &Options->ProcessorIndex;
    } else if (StrCmp (Argument, L"-P") == 0) {
      Options->PrintProcessorInformation = TRUE;
    } else if (StrCmp (Argument, L"-U") == 0) {
      Options->SetProcessorUnhealthy = TRUE;
    } else if (StrCmp (Argument, L"-W") == 0) {
      Options->PrintBspProcessorIndex = TRUE;
    } else if (StrCmp (Argument, L"-E") == 0) {
      Options->EnableProcessor = TRUE;
      NeedsValue               = TRUE;
      Value                    = &Options->ProcessorIndex;
    } else if (StrCmp (Argument, L"-D") == 0) {
      Options->DisableProcessor = TRUE;
      NeedsValue                = TRUE;
      Value                     = &Options->ProcessorIndex;
    } else {
      PrintUsage ();
      ZeroMem (Options, sizeof (MP_SERVICES_TEST_OPTIONS));
      return EFI_SUCCESS;
    }

    if (NeedsValue) {
      if ((ArgIndex + 1) < Argc) {
        Status = UnicodeStringToInteger (Argv[ArgIndex + 1], Value);
        if (EFI_ERROR (Status)) {
          Print (L"Error: option value must be a positive integer.\n");
          PrintUsage ();
          return EFI_INVALID_PARAMETER;
        }

        ArgIndex++;
      } else {
        PrintUsage ();
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  return EFI_SUCCESS;
}
