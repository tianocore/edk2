#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>

// Replicate the structural signature layout exposed by the true SHIM_IMAGE_LOADER interface
typedef struct _SHIM_IMAGE_LOADER_INTERFACE {
  EFI_IMAGE_LOAD      LoadImage;
  EFI_IMAGE_START     StartImage;
  EFI_IMAGE_UNLOAD    UnloadImage;
  EFI_EXIT            Exit;
} SHIM_IMAGE_LOADER_INTERFACE;

STATIC CONST CHAR16  mShimLoadHelpString[] = L"shim_load <path_to_efi_image> [arguments...]\r\n";

/**
  Built-in UEFI Shell Command Handler function for 'shim_load'.
**/
SHELL_STATUS
EFIAPI
ShellCommandRunShimLoad (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  SHIM_IMAGE_LOADER_INTERFACE  *ShimLoader  = NULL;
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath  = NULL;
  EFI_HANDLE                   TargetHandle = NULL;
  EFI_LOADED_IMAGE_PROTOCOL    *LoadedImage = NULL;
  LIST_ENTRY                   *Package;
  CHAR16                       *ProblemParam;
  CONST CHAR16                 *TargetImagePath;
  CHAR16                       *OptionsBuffer = NULL;
  UINT32                       OptionsSize    = 0;
  UINTN                        Argc;
  UINTN                        Index;

  Status = ShellCommandLineParse (NULL, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    Print (L"Error: ShellCommandLineParse failed. Code: %r (%d)\n", Status, Status);
    return SHELL_INVALID_PARAMETER;
  }

  Argc = ShellCommandLineGetCount (Package);
  if (Argc < 2) {
    Print (L"Error: Too few arguments.\n");
    Print (L"Usage: %s", mShimLoadHelpString);
    ShellCommandLineFreeVarList (Package);
    return SHELL_INVALID_PARAMETER;
  }

  // CORRECTED: Extracted using the standard EDK2 argument accessor function
  TargetImagePath = ShellCommandLineGetRawValue (Package, 1);
  if (TargetImagePath == NULL) {
    Print (L"Error: Unable to retrieve target file path from input parameter mapping.\n");
    ShellCommandLineFreeVarList (Package);
    return SHELL_INVALID_PARAMETER;
  }

  // 1. Locate the interface from the database using the SHIM_IMAGE_LOADER_GUID token space reference
  Status = gBS->LocateProtocol (&gShimImageLoaderGuid, NULL, (VOID **)&ShimLoader);
  if (EFI_ERROR (Status)) {
    Print (L"Error: LocateProtocol for SHIM_IMAGE_LOADER_GUID failed. Code: %r (%d)\n", Status, Status);
    ShellCommandLineFreeVarList (Package);
    return SHELL_NOT_FOUND;
  }

  // 2. Map filesystem target paths to fully qualified firmware device paths
  DevicePath = gEfiShellProtocol->GetDevicePathFromFilePath (TargetImagePath);
  if (DevicePath == NULL) {
    Print (L"Error: Device Path generation failed for target parameter path: %s\n", TargetImagePath);
    ShellCommandLineFreeVarList (Package);
    return SHELL_INVALID_PARAMETER;
  }

  // 3. EXPLICIT INTERFACE CALL: Trigger the explicit LoadImage method from the interface structure
  Print (L"Explicitly invoking SHIM_IMAGE_LOADER method: LoadImage...\n");
  Status = ShimLoader->LoadImage (
                         FALSE,
                         gImageHandle,
                         DevicePath,
                         NULL,
                         0,
                         &TargetHandle
                         );

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }

  if (EFI_ERROR (Status)) {
    Print (L"Security or Load Failure: SHIM_IMAGE_LOADER rejected binary '%s'. Code: %r (%d)\n", TargetImagePath, Status, Status);
    ShellCommandLineFreeVarList (Package);
    return SHELL_DEVICE_ERROR;
  }

  // 4. Synthesize command-line trailing option strings if extra properties were supplied
  if (Argc > 2) {
    for (Index = 2; Index < Argc; Index++) {
      OptionsSize += (UINT32)StrSize (ShellCommandLineGetRawValue (Package, Index));
    }

    OptionsSize += (UINT32)((Argc - 2) * sizeof (CHAR16));

    OptionsBuffer = AllocateZeroPool (OptionsSize);
    if (OptionsBuffer == NULL) {
      Print (L"Error: Memory allocation failed for command-line string buffer. Size requested: %u bytes\n", OptionsSize);

      Status = ShimLoader->UnloadImage (TargetHandle);
      if (EFI_ERROR (Status)) {
        Print (L"Error: Context recovery fallback UnloadImage method failed. Code: %r (%d)\n", Status, Status);
      }

      ShellCommandLineFreeVarList (Package);
      return SHELL_OUT_OF_RESOURCES;
    }

    for (Index = 2; Index < Argc; Index++) {
      StrCatS (OptionsBuffer, OptionsSize / sizeof (CHAR16), ShellCommandLineGetRawValue (Package, Index));
      if (Index < (Argc - 1)) {
        StrCatS (OptionsBuffer, OptionsSize / sizeof (CHAR16), L" ");
      }
    }

    Status = gBS->HandleProtocol (TargetHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
    if (EFI_ERROR (Status)) {
      Print (L"Error: HandleProtocol for gEfiLoadedImageProtocolGuid mapping context failed. Code: %r (%d)\n", Status, Status);
      FreePool (OptionsBuffer);
      OptionsBuffer = NULL;
    } else {
      LoadedImage->LoadOptions     = OptionsBuffer;
      LoadedImage->LoadOptionsSize = OptionsSize;
    }
  }

  ShellCommandLineFreeVarList (Package);

  // 5. EXPLICIT INTERFACE CALL: Trigger the explicit StartImage execution method out of the interface structure
  Print (L"Explicitly invoking SHIM_IMAGE_LOADER method: StartImage...\n");
  Status = ShimLoader->StartImage (TargetHandle, NULL, NULL);

  if (OptionsBuffer != NULL) {
    FreePool (OptionsBuffer);
  }

  if (EFI_ERROR (Status)) {
    Print (L"Execution Aborted: Staged child binary execution failed. Code: %r (%d)\n", Status, Status);
    return SHELL_DEVICE_ERROR;
  }

  Print (L"Success: Target binary completed execution pathways cleanly.\n");
  return SHELL_SUCCESS;
}

/**
  Callback function indicating the help manual string path.
**/
STATIC
CONST CHAR16 *
EFIAPI
ShimLoaderGetManFileName (
  VOID
  )
{
  return mShimLoadHelpString;
}

/**
  Library Constructor to register the command automatically with the shell upon initialization
**/
EFI_STATUS
EFIAPI
ShimLoaderCmdLibConstructor (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = ShellCommandRegisterCommandName (
             L"shim_load",
             ShellCommandRunShimLoad,
             ShimLoaderGetManFileName,
             0,
             L"shim_load",
             TRUE,
             NULL,
             0
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error: ShimLoader static library failed to self-register. Code: %r\n", Status));
  }

  return Status;
}
