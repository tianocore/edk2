/** @file
*
*  Shell command for launching AXF files.
*
*  Copyright (c) 2014, ARM Limited. All rights reserved.
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

#include <Guid/GlobalVariable.h>

#include <Library/PrintLib.h>
#include <Library/HandleParsingLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BdsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

#include <Library/ArmLib.h>

#include "ArmShellCmdRunAxf.h"
#include "ElfLoader.h"
#include "BootMonFsLoader.h"

// Provide arguments to AXF?
typedef VOID (*ELF_ENTRYPOINT)(UINTN arg0, UINTN arg1,
                               UINTN arg2, UINTN arg3);


STATIC
EFI_STATUS
PreparePlatformHardware (
  VOID
  )
{
  //Note: Interrupts will be disabled by the GIC driver when ExitBootServices() will be called.

  // Clean before Disable else the Stack gets corrupted with old data.
  ArmCleanDataCache ();
  ArmDisableDataCache ();
  // Invalidate all the entries that might have snuck in.
  ArmInvalidateDataCache ();

  // Disable and invalidate the instruction cache
  ArmDisableInstructionCache ();
  ArmInvalidateInstructionCache ();

  // Turn off MMU
  ArmDisableMmu();

  return EFI_SUCCESS;
}

// Process arguments to pass to AXF?
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {NULL, TypeMax}
};


/**
  This is the shell command handler function pointer callback type. This
  function handles the command when it is invoked in the shell.

  @param[in] This             The instance of the
                              EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] SystemTable      The pointer to the system table.
  @param[in] ShellParameters  The parameters associated with the command.
  @param[in] Shell            The instance of the shell protocol used in the
                              context of processing this command.

  @return EFI_SUCCESS         The operation was successful.
  @return other               The operation failed.
**/
SHELL_STATUS
EFIAPI
ShellDynCmdRunAxfHandler (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN EFI_SYSTEM_TABLE                      *SystemTable,
  IN EFI_SHELL_PARAMETERS_PROTOCOL         *ShellParameters,
  IN EFI_SHELL_PROTOCOL                    *Shell
  )
{
  LIST_ENTRY        *ParamPackage;
  EFI_STATUS         Status;
  SHELL_STATUS       ShellStatus;
  SHELL_FILE_HANDLE  FileHandle;
  ELF_ENTRYPOINT     StartElf;
  CONST CHAR16      *FileName;
  EFI_FILE_INFO     *Info;
  UINTN              FileSize;
  VOID              *FileData;
  VOID              *Entrypoint;
  LIST_ENTRY         LoadList;
  LIST_ENTRY        *Node;
  LIST_ENTRY        *NextNode;
  RUNAXF_LOAD_LIST  *LoadNode;
  CHAR16            *TmpFileName;
  CHAR16            *TmpChar16;


  ShellStatus = SHELL_SUCCESS;
  FileHandle = NULL;
  FileData = NULL;
  InitializeListHead (&LoadList);

  // Only install if they are not there yet? First time or every time?
  // These can change if the shell exits and start again.
  Status = gBS->InstallMultipleProtocolInterfaces (&gImageHandle,
                &gEfiShellProtocolGuid, Shell,
                &gEfiShellParametersProtocolGuid, ShellParameters,
                NULL);

  if (EFI_ERROR (Status)) {
    return SHELL_DEVICE_ERROR;
  }

  // Update the protocols for the application library
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);
  // Add support to load AXF with optipnal args?

  //
  // Process Command Line arguments
  //
  Status = ShellCommandLineParse (ParamList, &ParamPackage, NULL, TRUE);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_INVALID_ARG), gRunAxfHiiHandle);
    ShellStatus = SHELL_INVALID_PARAMETER;
  } else {
    //
    // Check for "-?"
    //
    if ((ShellCommandLineGetFlag (ParamPackage, L"-?")) ||
        (ShellCommandLineGetRawValue (ParamPackage, 1) == NULL)) {
      //
      // We didn't get a file to load
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_INVALID_ARG), gRunAxfHiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      // For the moment we assume we only ever get one file to load with no arguments.
      FileName = ShellCommandLineGetRawValue (ParamPackage, 1);
      Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
      if (EFI_ERROR (Status)) {
        // BootMonFS supports file extensions, but they are stripped by default
        // when the NOR is programmed.
        // Remove the file extension and try to open again.
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_FILE_NOT_FOUND),
                         gRunAxfHiiHandle, FileName);
        // Go through the filename and remove file extension. Preserve the
        // original name.
        TmpFileName = AllocateCopyPool (StrSize (FileName), (VOID *)FileName);
        if (TmpFileName != NULL) {
          TmpChar16 = StrStr (TmpFileName, L".");
          if (TmpChar16 != NULL) {
            *TmpChar16 = '\0';
            DEBUG((EFI_D_ERROR, "Trying to open file: %s\n", TmpFileName));
            Status = ShellOpenFileByName (TmpFileName, &FileHandle,
                                          EFI_FILE_MODE_READ, 0);
          }
          FreePool (TmpFileName);
        }
        // Do we now have an open file after trying again?
        if (EFI_ERROR (Status)) {
          ShellStatus = SHELL_INVALID_PARAMETER;
          FileHandle = NULL;
        }
      }

      if (FileHandle != NULL) {
        Info = ShellGetFileInfo (FileHandle);
        FileSize = (UINTN) Info->FileSize;
        FreePool (Info);

        //
        // Allocate buffer to read file. 'Runtime' so we can access it after
        // ExitBootServices().
        //
        FileData = AllocateRuntimeZeroPool (FileSize);
        if (FileData == NULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_NO_MEM), gRunAxfHiiHandle);
          ShellStatus = SHELL_OUT_OF_RESOURCES;
        } else {
          //
          // Read file into Buffer
          //
          Status = ShellReadFile (FileHandle, &FileSize, FileData);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_READ_FAIL), gRunAxfHiiHandle);
            SHELL_FREE_NON_NULL (FileData);
            FileData = NULL;
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        }
      }
    }

    //
    // Free the command line package
    //
    ShellCommandLineFreeVarList (ParamPackage);
  }

  // We have a file in memory. Try to work out if we can use it.
  // It can either be in ELF format or BootMonFS format.
  if (FileData != NULL) {
    // Do some validation on the file before we try to load it. The file can
    // either be an proper ELF file or one processed by the FlashLoader.
    // Since the data might need to go to various locations in memory we cannot
    // load the data directly while UEFI is running. We use the file loaders to
    // populate a linked list of data and load addresses. This is processed and
    // data copied to where it needs to go after calling ExitBootServices. At
    // that stage we've reached the point of no return, so overwriting UEFI code
    // does not make a difference.
    Status = ElfCheckFile (FileData);
    if (!EFI_ERROR (Status)) {
      // Load program into memory
      Status = ElfLoadFile ((VOID*)FileData, &Entrypoint, &LoadList);
    } else {
      // Try to see if it is a BootMonFs executable
      Status = BootMonFsCheckFile ((EFI_FILE_HANDLE)FileHandle);
      if (!EFI_ERROR (Status)) {
        // Load program into memory
        Status = BootMonFsLoadFile ((EFI_FILE_HANDLE)FileHandle,
                                    (VOID*)FileData, &Entrypoint, &LoadList);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_RUNAXF_BAD_FILE),
                         gRunAxfHiiHandle);
        SHELL_FREE_NON_NULL (FileData);
        ShellStatus = SHELL_UNSUPPORTED;
      }
    }
  }

  // Program load list created.
  // Shutdown UEFI, copy and jump to code.
  if (!IsListEmpty (&LoadList) && !EFI_ERROR (Status)) {
    // Exit boot services here. This means we cannot return and cannot assume to
    // have access to UEFI functions.
    Status = ShutdownUefiBootServices ();
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR,"Can not shutdown UEFI boot services. Status=0x%X\n",
              Status));
    } else {
      // Process linked list. Copy data to Memory.
      Node = GetFirstNode (&LoadList);
      while (!IsNull (&LoadList, Node)) {
        LoadNode = (RUNAXF_LOAD_LIST *)Node;
        // Do we have data to copy or do we need to set Zeroes (.bss)?
        if (LoadNode->Zeroes) {
          ZeroMem ((VOID*)LoadNode->MemOffset, LoadNode->Length);
        } else {
          CopyMem ((VOID *)LoadNode->MemOffset, (VOID *)LoadNode->FileOffset,
                   LoadNode->Length);
        }
        Node = GetNextNode (&LoadList, Node);
      }

      //
      // Switch off interrupts, caches, mmu, etc
      //
      Status = PreparePlatformHardware ();
      ASSERT_EFI_ERROR (Status);

      StartElf = (ELF_ENTRYPOINT)Entrypoint;
      StartElf (0,0,0,0);

      // We should never get here.. But if we do, spin..
      ASSERT (FALSE);
      while (1);
    }
  }

  // Free file related information as we are returning to UEFI.
  Node = GetFirstNode (&LoadList);
  while (!IsNull (&LoadList, Node)) {
    NextNode = RemoveEntryList (Node);
    FreePool (Node);
    Node = NextNode;
  }
  SHELL_FREE_NON_NULL (FileData);
  if (FileHandle != NULL) {
    ShellCloseFile (&FileHandle);
  }

  // Uninstall protocols as we don't know if they will change.
  // If the shell exits and come in again these mappings may be different
  // and cause a crash.
  Status = gBS->UninstallMultipleProtocolInterfaces (gImageHandle,
                &gEfiShellProtocolGuid, Shell,
                &gEfiShellParametersProtocolGuid, ShellParameters,
                NULL);

  if (EFI_ERROR (Status) && ShellStatus == SHELL_SUCCESS) {
    ShellStatus = SHELL_DEVICE_ERROR;
  }

  return ShellStatus;
}


/**
  This is the command help handler function pointer callback type. This
  function is responsible for displaying help information for the associated
  command.

  @param[in] This             The instance of the
                              EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL.
  @param[in] Language         The pointer to the language string to use.

  @return string              Pool allocated help string, must be freed by
                              caller.
**/
CHAR16*
EFIAPI
ShellDynCmdRunAxfGetHelp (
  IN EFI_SHELL_DYNAMIC_COMMAND_PROTOCOL    *This,
  IN CONST CHAR8                           *Language
  )
{
  CHAR16 *HelpText;

  ASSERT (gRunAxfHiiHandle != NULL);

  // This allocates memory. The caller is responsoible to free.
  HelpText = HiiGetString (gRunAxfHiiHandle, STRING_TOKEN (STR_GET_HELP_RUNAXF),
                           Language);

  return HelpText;
}
