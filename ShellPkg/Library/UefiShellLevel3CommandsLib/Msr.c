/** @file
  Main file for Msr shell level 3 function.
**/

#include "UefiShellLevel3CommandsLib.h"
#include <Library/ShellLib.h>

/**
  Function for 'rdmsr' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMsr (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *MsrString;
  UINT64        MsrIndex64;
  UINT64        MsrValue;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;
  MsrIndex64   = 0;

  //
  // initialize the shell lib
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"rdmsr", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetFlag (Package, L"-?")) {
      ASSERT (FALSE);
    } else if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"rdmsr");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue (Package, 2) != NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"rdmsr");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      MsrString = ShellCommandLineGetRawValue (Package, 1);
      Status    = ShellConvertStringToUint64 (MsrString, &MsrIndex64, FALSE, FALSE);
      if (EFI_ERROR (Status) || (MsrIndex64 > MAX_UINT32)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"rdmsr", MsrString);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
 #if defined (MDE_CPU_IA32) || defined (MDE_CPU_X64)
        MsrValue = AsmReadMsr64 ((UINT32)MsrIndex64);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_MSR_OUTPUT),
          gShellLevel3HiiHandle,
          (UINT32)MsrIndex64,
          MsrValue
          );
 #else
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MSR_UNSUPPORTED), gShellLevel3HiiHandle, L"rdmsr");
        ShellStatus = SHELL_UNSUPPORTED;
 #endif
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
