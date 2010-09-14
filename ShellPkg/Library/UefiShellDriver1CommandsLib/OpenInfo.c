/** @file
  Main file for OpenInfo shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

EFI_STATUS
EFIAPI
TraverseHandleDatabase (
  IN CONST EFI_HANDLE DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE ControllerHandle OPTIONAL,
  IN UINTN            *HandleCount,
  OUT EFI_HANDLE      **HandleBuffer,
  OUT UINTN           **HandleType
  ){
  EFI_STATUS                          Status;
  UINTN                               HandleIndex;
  EFI_GUID                            **ProtocolGuidArray;
  UINTN                               ArrayCount;
  UINTN                               ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
  UINTN                               OpenInfoCount;
  UINTN                               OpenInfoIndex;
  UINTN                               ChildIndex;

  ASSERT(HandleCount  != NULL);
  ASSERT(HandleBuffer != NULL);
  ASSERT(HandleType   != NULL);
  ASSERT(DriverBindingHandle != NULL || ControllerHandle != NULL);

  *HandleCount                  = 0;
  *HandleBuffer                 = NULL;
  *HandleType                   = NULL;

  //
  // Retrieve the list of all handles from the handle database
  //
  Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                HandleCount,
                HandleBuffer
                );
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  *HandleType = AllocateZeroPool (*HandleCount * sizeof (UINTN));
  ASSERT(*HandleType != NULL);

  for (HandleIndex = 0; HandleIndex < *HandleCount; HandleIndex++) {
    //
    // Retrieve the list of all the protocols on each handle
    //
    Status = gBS->ProtocolsPerHandle (
                  (*HandleBuffer)[HandleIndex],
                  &ProtocolGuidArray,
                  &ArrayCount
                  );
    if (!EFI_ERROR (Status)) {

      for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {

        //
        // Set the bit describing what this handle has
        //
        if        (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiLoadedImageProtocolGuid)           != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_IMAGE_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverBindingProtocolGuid)         != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DRIVER_BINDING_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfiguration2ProtocolGuid)  != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DRIVER_CONFIGURATION_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverConfigurationProtocolGuid)   != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DRIVER_CONFIGURATION_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnostics2ProtocolGuid)    != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DRIVER_DIAGNOSTICS_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDriverDiagnosticsProtocolGuid)     != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DRIVER_DIAGNOSTICS_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentName2ProtocolGuid)        != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_COMPONENT_NAME_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiComponentNameProtocolGuid)         != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_COMPONENT_NAME_HANDLE;
        } else if (CompareGuid (ProtocolGuidArray[ProtocolIndex], &gEfiDevicePathProtocolGuid)            != FALSE) {
          (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_DEVICE_HANDLE;
        } else {
          DEBUG_CODE_BEGIN();
          ASSERT((*HandleType)[HandleIndex] == (*HandleType)[HandleIndex]);
          DEBUG_CODE_END();
        }
        //
        // Retrieve the list of agents that have opened each protocol
        //
        Status = gBS->OpenProtocolInformation (
                      (*HandleBuffer)[HandleIndex],
                      ProtocolGuidArray[ProtocolIndex],
                      &OpenInfo,
                      &OpenInfoCount
                      );
        if (!EFI_ERROR (Status)) {
          for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
            if (DriverBindingHandle != NULL && OpenInfo[OpenInfoIndex].AgentHandle == DriverBindingHandle) {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
                //
                // Mark the device handle as being managed by the driver specified by DriverBindingHandle
                //
                (*HandleType)[HandleIndex] |= (HANDLE_RELATIONSHIP_DEVICE_HANDLE | HANDLE_RELATIONSHIP_CONTROLLER_HANDLE);
              }
              if (ControllerHandle != NULL && (*HandleBuffer)[HandleIndex] == ControllerHandle) {
                if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
                  for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                    if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].ControllerHandle) {
                      (*HandleType)[ChildIndex] |= (HANDLE_RELATIONSHIP_DEVICE_HANDLE | HANDLE_RELATIONSHIP_CHILD_HANDLE);
                    }
                  }
                }
              }
            } else if (DriverBindingHandle == NULL && OpenInfo[OpenInfoIndex].ControllerHandle == ControllerHandle) {
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_DRIVER) == EFI_OPEN_PROTOCOL_BY_DRIVER) {
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].AgentHandle) {
                    //
                    // mark the handle who opened this as a device driver
                    //
                    (*HandleType)[ChildIndex] |= HANDLE_RELATIONSHIP_DEVICE_DRIVER;
                  }
                }
              }
              if ((OpenInfo[OpenInfoIndex].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) == EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) {
                //
                // this handle has people opening by child so it must be a parent
                //
                (*HandleType)[HandleIndex] |= HANDLE_RELATIONSHIP_PARENT_HANDLE;
                for (ChildIndex = 0; ChildIndex < *HandleCount; ChildIndex++) {
                  if ((*HandleBuffer)[ChildIndex] == OpenInfo[OpenInfoIndex].AgentHandle) {
                    (*HandleType)[ChildIndex] |= HANDLE_RELATIONSHIP_BUS_DRIVER;
                  }
                }
              }
            }
          }

          FreePool (OpenInfo);
        }
      }

      FreePool (ProtocolGuidArray);
    }
  }

  if (EFI_ERROR(Status)) {
    if (*HandleType != NULL) {
      FreePool (*HandleType);
    }
    if (*HandleBuffer != NULL) {
      FreePool (*HandleBuffer);
    }

    *HandleCount  = 0;
    *HandleBuffer = NULL;
    *HandleType   = NULL;
  }

  return Status;
}


SHELL_STATUS
EFIAPI
ShellCommandRunOpenInfo (
  VOID                *RESERVED
  ) {
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  EFI_HANDLE          theHandle;
  EFI_HANDLE          *HandleList;
  UINTN               Count;
  UINTN               *Type;

  ShellStatus         = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if EFI_ERROR(Status) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount() > 2){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount() == 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (ShellCommandLineGetRawValue(Package, 1) != NULL && CommandLibGetHandleValue(StrHexToUintn(ShellCommandLineGetRawValue(Package, 1))) == NULL){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, ShellCommandLineGetRawValue(Package, 1));
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        theHandle = CommandLibGetHandleValue(StrHexToUintn(ShellCommandLineGetRawValue(Package, 1)));
        ASSERT(theHandle != NULL);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_OPENINFO_HEADER_LINE), gShellDriver1HiiHandle, StrHexToUintn(ShellCommandLineGetRawValue(Package, 1)), theHandle);
        Status = TraverseHandleDatabase (NULL, theHandle, &Count, &HandleList, &Type);
        if (EFI_ERROR(Status) == FALSE && Count > 0) {
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, ShellCommandLineGetRawValue(Package, 1));
          ShellStatus = SHELL_NOT_FOUND;
        }
      }
    }
  }
  return (ShellStatus);
}
