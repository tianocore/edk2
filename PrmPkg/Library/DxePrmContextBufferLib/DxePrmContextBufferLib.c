/** @file

  The PRM Buffer Context library provides a general abstraction for context buffer management.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrmContextBufferLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/PrmConfig.h>

#define _DBGMSGID_  "[PRMCONTEXTBUFFERLIB]"

/**
  Finds a PRM context buffer for the given PRM handler GUID.

  Note: PRM_MODULE_CONTEXT_BUFFERS is at the PRM module level while PRM_CONTEXT_BUFFER is at the PRM handler level.

  @param[in]  HandlerGuid                 A pointer to the PRM handler GUID.
  @param[in]  ModuleContextBuffers        A pointer to the PRM context buffers structure for the PRM module.
  @param[out] PrmModuleContextBuffer      A pointer to a pointer that will be set to the PRM context buffer
                                          if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffer was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffer for the given PRM handler GUID could not be found.

**/
EFI_STATUS
FindContextBufferInModuleBuffers (
  IN  CONST EFI_GUID                    *HandlerGuid,
  IN  CONST PRM_MODULE_CONTEXT_BUFFERS  *ModuleContextBuffers,
  OUT CONST PRM_CONTEXT_BUFFER          **ContextBuffer
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "    %a %a - Entry.\n", _DBGMSGID_, __func__));

  if ((HandlerGuid == NULL) || (ModuleContextBuffers == NULL) || (ContextBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < ModuleContextBuffers->BufferCount; Index++) {
    if (CompareGuid (&ModuleContextBuffers->Buffer[Index].HandlerGuid, HandlerGuid)) {
      *ContextBuffer = &ModuleContextBuffers->Buffer[Index];
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Returns a PRM context buffers structure for the given PRM search type.

  This function allows a caller to get the context buffers structure for a PRM module with either the PRM module
  GUID or the GUID for a PRM handler in the module.

  Note: PRM_MODULE_CONTEXT_BUFFERS is at the PRM module level while PRM_CONTEXT_BUFFER is at the PRM handler level.

  @param[in]  GuidSearchType              The type of GUID passed in the Guid argument.
  @param[in]  Guid                        A pointer to the GUID of a PRM module or PRM handler. The actual GUID type
                                          will be interpreted based on the value passed in GuidSearchType.
  @param[out] PrmModuleContextBuffers     A pointer to a pointer that will be set to the PRM context buffers
                                          structure if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffers structure was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffers for the given GUID could not be found.

**/
EFI_STATUS
GetModuleContextBuffers (
  IN  PRM_GUID_SEARCH_TYPE              GuidSearchType,
  IN  CONST EFI_GUID                    *Guid,
  OUT CONST PRM_MODULE_CONTEXT_BUFFERS  **PrmModuleContextBuffers
  )
{
  EFI_STATUS                Status;
  UINTN                     HandleCount;
  UINTN                     Index;
  EFI_HANDLE                *HandleBuffer;
  PRM_CONFIG_PROTOCOL       *PrmConfigProtocol;
  CONST PRM_CONTEXT_BUFFER  *PrmContextBuffer;

  DEBUG ((DEBUG_INFO, "    %a %a - Entry.\n", _DBGMSGID_, __func__));

  if ((Guid == NULL) || (PrmModuleContextBuffers == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *PrmModuleContextBuffers = NULL;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gPrmConfigProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gPrmConfigProtocolGuid,
                      (VOID **)&PrmConfigProtocol
                      );
      ASSERT_EFI_ERROR (Status);
      if (EFI_ERROR (Status) || (PrmConfigProtocol == NULL)) {
        continue;
      }

      if (GuidSearchType == ByModuleGuid) {
        if (CompareGuid (&PrmConfigProtocol->ModuleContextBuffers.ModuleGuid, Guid)) {
          DEBUG ((
            DEBUG_INFO,
            "      %a %a: Found a PRM configuration protocol for PRM module %g.\n",
            _DBGMSGID_,
            __func__,
            Guid
            ));

          *PrmModuleContextBuffers = &PrmConfigProtocol->ModuleContextBuffers;
          return EFI_SUCCESS;
        }
      } else {
        Status = FindContextBufferInModuleBuffers (Guid, &PrmConfigProtocol->ModuleContextBuffers, &PrmContextBuffer);
        if (!EFI_ERROR (Status)) {
          *PrmModuleContextBuffers = &PrmConfigProtocol->ModuleContextBuffers;
          return EFI_SUCCESS;
        }
      }
    }
  }

  DEBUG ((
    DEBUG_INFO,
    "      %a %a: Could not locate a PRM configuration protocol for PRM handler %g.\n",
    _DBGMSGID_,
    __func__,
    Guid
    ));

  return EFI_NOT_FOUND;
}

/**
  Returns a PRM context buffer for the given PRM handler.

  @param[in]  PrmHandlerGuid              A pointer to the GUID for the PRM handler.
  @param[in]  PrmModuleContextBuffers     A pointer to a PRM_MODULE_CONTEXT_BUFFERS structure. If this optional
                                          parameter is provided, the handler context buffer will be searched for in this
                                          buffer structure which saves time by not performing a global search for the
                                          module buffer structure.
  @param[out] PrmContextBuffer            A pointer to a pointer that will be set to the PRM context buffer
                                          if successfully found.

  @retval EFI_SUCCESS                     The PRM context buffer was found.
  @retval EFI_INVALID_PARAMETER           A required parameter pointer is NULL.
  @retval EFI_NOT_FOUND                   The context buffer for the PRM handler could not be found.

**/
EFI_STATUS
GetContextBuffer (
  IN  CONST EFI_GUID                    *PrmHandlerGuid,
  IN  CONST PRM_MODULE_CONTEXT_BUFFERS  *PrmModuleContextBuffers  OPTIONAL,
  OUT CONST PRM_CONTEXT_BUFFER          **PrmContextBuffer
  )
{
  EFI_STATUS                        Status;
  CONST PRM_MODULE_CONTEXT_BUFFERS  *ContextBuffers;

  DEBUG ((DEBUG_INFO, "    %a %a - Entry.\n", _DBGMSGID_, __func__));

  if ((PrmHandlerGuid == NULL) || (PrmContextBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *PrmContextBuffer = NULL;

  if (PrmModuleContextBuffers == NULL) {
    Status = GetModuleContextBuffers (ByHandlerGuid, PrmHandlerGuid, &ContextBuffers);
    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }
  } else {
    ContextBuffers = PrmModuleContextBuffers;
  }

  Status = FindContextBufferInModuleBuffers (PrmHandlerGuid, ContextBuffers, PrmContextBuffer);

  return Status;
}
