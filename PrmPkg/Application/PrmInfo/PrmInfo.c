/** @file
  Prints information about the PRM configuration loaded by the system firmware.

  This application also provides some additional testing features for PRM configuration. For example,
  the application can be used to selectively invoke PRM handlers in the UEFI shell environment to
  provide a quick testing path of the PRM infrastructure on the firmware and the PRM module implementation.

  This can also be useful to prepare a PRM enabled firmware and PRM modules prior to formal OS support to
  test the PRM code.

  Copyright (C) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Guid/ZeroGuid.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PrmContextBufferLib.h>
#include <Library/PrmModuleDiscoveryLib.h>
#include <Library/PrmPeCoffLib.h>
#include <Library/ShellLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>

#include "PrmInfo.h"

GLOBAL_REMOVE_IF_UNREFERENCED EFI_STRING_ID  mStringPrmInfoHelpTokenId = STRING_TOKEN (STR_PRMINFO_HELP);
//
// This is the generated String package data for all .UNI files.
// This data array is ready to be used as input of HiiAddPackages() to
// create a packagelist (which contains Form packages, String packages, etc).
//
extern UINT8  PrmInfoStrings[];

STATIC UINTN  mPrmHandlerCount;
STATIC UINTN  mPrmModuleCount;

STATIC EFI_HII_HANDLE  mPrmInfoHiiHandle;
STATIC LIST_ENTRY      mPrmHandlerList;

STATIC CONST SHELL_PARAM_ITEM  mParamList[] = {
  { L"-l", TypeFlag  },
  { L"-t", TypeValue },
  { NULL,  TypeMax   }
};

/**
  Frees all of the nodes in a linked list.

  @param[in] ListHead                   A pointer to the head of the list that should be freed.

  **/
VOID
EFIAPI
FreeList (
  IN LIST_ENTRY  *ListHead
  )
{
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      *NextLink;
  PRM_HANDLER_CONTEXT_LIST_ENTRY  *ListEntry;

  if (ListHead == NULL) {
    return;
  }

  Link = GetFirstNode (&mPrmHandlerList);
  while (!IsNull (&mPrmHandlerList, Link)) {
    ListEntry = CR (Link, PRM_HANDLER_CONTEXT_LIST_ENTRY, Link, PRM_HANDLER_CONTEXT_LIST_ENTRY_SIGNATURE);
    NextLink  = GetNextNode (&mPrmHandlerList, Link);

    RemoveEntryList (Link);
    FreePool (ListEntry);

    Link = NextLink;
  }
}

/**
  Creates a new PRM Module Image Context linked list entry.

  @retval    PrmHandlerContextListEntry If successful, a pointer a PRM Handler Context linked list entry
                                        otherwise, NULL is returned.

**/
PRM_HANDLER_CONTEXT_LIST_ENTRY *
CreateNewPrmHandlerListEntry (
  VOID
  )
{
  PRM_HANDLER_CONTEXT_LIST_ENTRY  *PrmHandlerContextListEntry;

  PrmHandlerContextListEntry = AllocateZeroPool (sizeof (*PrmHandlerContextListEntry));
  if (PrmHandlerContextListEntry == NULL) {
    return NULL;
  }

  PrmHandlerContextListEntry->Signature = PRM_HANDLER_CONTEXT_LIST_ENTRY_SIGNATURE;

  return PrmHandlerContextListEntry;
}

/**
  Creates a new PRM Module Image Context linked list entry.

  @param[in]  RuntimeMmioRanges  A pointer to an array of PRM module config runtime MMIO ranges.

**/
VOID
PrintMmioRuntimeRangeInfo (
  IN PRM_RUNTIME_MMIO_RANGES  *RuntimeMmioRanges
  )
{
  UINTN  RuntimeMmioRangeCount;
  UINTN  RuntimeMmioRangeIndex;

  if (RuntimeMmioRanges == NULL) {
    return;
  }

  RuntimeMmioRangeCount = (UINTN)RuntimeMmioRanges->Count;
  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_RUNTIME_MMIO_COUNT), mPrmInfoHiiHandle, RuntimeMmioRangeCount);

  for (RuntimeMmioRangeIndex = 0; RuntimeMmioRangeIndex < RuntimeMmioRangeCount; RuntimeMmioRangeIndex++) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_PRMINFO_RUNTIME_MMIO_INFO),
      mPrmInfoHiiHandle,
      RuntimeMmioRangeIndex,
      RuntimeMmioRanges->Range[RuntimeMmioRangeIndex].PhysicalBaseAddress,
      RuntimeMmioRanges->Range[RuntimeMmioRangeIndex].VirtualBaseAddress,
      RuntimeMmioRanges->Range[RuntimeMmioRangeIndex].Length
      );
  }
}

/**
  Gathers the PRM handler (and by extension module) information discovered on this system.

  This function must be called to build up the discovered context for other functions in the application. The
  function will optionally print results as determed by the value of the PrintInformation parameter.

  @param[in] PrintInformation           Indicates whether to print information as discovered in the function.

**/
VOID
GatherPrmHandlerInfo (
  IN  BOOLEAN  PrintInformation
  )
{
  EFI_STATUS                           Status;
  UINT16                               MajorVersion;
  UINT16                               MinorVersion;
  UINT16                               HandlerCount;
  UINTN                                HandlerIndex;
  EFI_PHYSICAL_ADDRESS                 CurrentHandlerPhysicalAddress;
  EFI_PHYSICAL_ADDRESS                 CurrentImageAddress;
  PRM_HANDLER_CONTEXT                  CurrentHandlerContext;
  EFI_GUID                             *CurrentModuleGuid;
  EFI_IMAGE_EXPORT_DIRECTORY           *CurrentImageExportDirectory;
  PRM_CONTEXT_BUFFER                   *CurrentContextBuffer;
  PRM_MODULE_EXPORT_DESCRIPTOR_STRUCT  *CurrentExportDescriptorStruct;
  PRM_MODULE_CONTEXT_BUFFERS           *CurrentModuleContextBuffers;
  PRM_HANDLER_CONTEXT_LIST_ENTRY       *CurrentHandlerContextListEntry;
  PRM_MODULE_IMAGE_CONTEXT             *CurrentPrmModuleImageContext;
  PRM_RUNTIME_MMIO_RANGES              *CurrentPrmModuleRuntimeMmioRanges;

  ASSERT (mPrmModuleCount <= mPrmHandlerCount);

  if (mPrmHandlerCount == 0) {
    return;
  }

  // Iterate across all PRM modules discovered
  for (
       CurrentPrmModuleImageContext = NULL, Status = GetNextPrmModuleEntry (&CurrentPrmModuleImageContext);
       !EFI_ERROR (Status);
       Status = GetNextPrmModuleEntry (&CurrentPrmModuleImageContext))
  {
    CurrentImageAddress           = CurrentPrmModuleImageContext->PeCoffImageContext.ImageAddress;
    CurrentImageExportDirectory   = CurrentPrmModuleImageContext->ExportDirectory;
    CurrentExportDescriptorStruct = CurrentPrmModuleImageContext->ExportDescriptor;

    CurrentModuleGuid = &CurrentExportDescriptorStruct->Header.ModuleGuid;
    HandlerCount      = CurrentExportDescriptorStruct->Header.NumberPrmHandlers;

    MajorVersion = 0;
    MinorVersion = 0;
    Status       =  GetImageVersionInPeCoffImage (
                      (VOID *)(UINTN)CurrentImageAddress,
                      &CurrentPrmModuleImageContext->PeCoffImageContext,
                      &MajorVersion,
                      &MinorVersion
                      );
    ASSERT_EFI_ERROR (Status);

    if (PrintInformation) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_PRMINFO_MODULE_NAME),
        mPrmInfoHiiHandle,
        (CHAR8 *)((UINTN)CurrentImageAddress + CurrentImageExportDirectory->Name)
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_MODULE_GUID), mPrmInfoHiiHandle, CurrentModuleGuid);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_MODULE_VERSION), mPrmInfoHiiHandle, MajorVersion, MinorVersion);
    }

    // It is currently valid for a PRM module not to use a context buffer
    CurrentPrmModuleRuntimeMmioRanges = NULL;
    Status                            = GetModuleContextBuffers (
                                          ByModuleGuid,
                                          CurrentModuleGuid,
                                          (CONST PRM_MODULE_CONTEXT_BUFFERS **)&CurrentModuleContextBuffers
                                          );
    ASSERT (!EFI_ERROR (Status) || Status == EFI_NOT_FOUND);
    if (!EFI_ERROR (Status) && (CurrentModuleContextBuffers != NULL)) {
      CurrentPrmModuleRuntimeMmioRanges = CurrentModuleContextBuffers->RuntimeMmioRanges;
    }

    if (PrintInformation) {
      if (CurrentPrmModuleRuntimeMmioRanges != NULL) {
        PrintMmioRuntimeRangeInfo (CurrentPrmModuleRuntimeMmioRanges);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NO_MMIO_RANGES), mPrmInfoHiiHandle);
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_COUNT), mPrmInfoHiiHandle, HandlerCount);
    }

    for (HandlerIndex = 0; HandlerIndex < HandlerCount; HandlerIndex++) {
      ZeroMem (&CurrentHandlerContext, sizeof (CurrentHandlerContext));

      CurrentHandlerContext.ModuleName = (CHAR8 *)((UINTN)CurrentImageAddress + CurrentImageExportDirectory->Name);
      CurrentHandlerContext.Guid       = &CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerGuid;
      CurrentHandlerContext.Name       = (CHAR8 *)CurrentExportDescriptorStruct->PrmHandlerExportDescriptors[HandlerIndex].PrmHandlerName;

      if (PrintInformation) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_NAME), mPrmInfoHiiHandle, CurrentHandlerContext.Name);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_GUID), mPrmInfoHiiHandle, CurrentHandlerContext.Guid);
      }

      Status =  GetExportEntryAddress (
                  CurrentHandlerContext.Name,
                  CurrentImageAddress,
                  CurrentImageExportDirectory,
                  &CurrentHandlerPhysicalAddress
                  );
      ASSERT_EFI_ERROR (Status);
      if (!EFI_ERROR (Status)) {
        CurrentHandlerContext.Handler = (PRM_HANDLER *)(UINTN)CurrentHandlerPhysicalAddress;

        if (PrintInformation) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_PA), mPrmInfoHiiHandle, CurrentHandlerPhysicalAddress);
        }
      } else {
        if (PrintInformation) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_PA_ERROR), mPrmInfoHiiHandle, Status);
        }
      }

      Status =  GetContextBuffer (
                  CurrentHandlerContext.Guid,
                  CurrentModuleContextBuffers,
                  (CONST PRM_CONTEXT_BUFFER **)&CurrentContextBuffer
                  );
      if (!EFI_ERROR (Status)) {
        CurrentHandlerContext.StaticDataBuffer = CurrentContextBuffer->StaticDataBuffer;
      }

      if (PrintInformation) {
        if (CurrentHandlerContext.StaticDataBuffer != NULL) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_PRMINFO_STATIC_DATA_BUFFER),
            mPrmInfoHiiHandle,
            (UINTN)CurrentHandlerContext.StaticDataBuffer
            );
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NO_STATIC_BUFFER), mPrmInfoHiiHandle);
        }
      }

      CurrentHandlerContextListEntry = CreateNewPrmHandlerListEntry ();
      ASSERT (CurrentHandlerContextListEntry != NULL);
      if (CurrentHandlerContextListEntry != NULL) {
        CopyMem (
          &CurrentHandlerContextListEntry->Context,
          &CurrentHandlerContext,
          sizeof (CurrentHandlerContextListEntry->Context)
          );
        InsertTailList (&mPrmHandlerList, &CurrentHandlerContextListEntry->Link);
      }
    }
  }
}

/**
  Populates the given context buffer so it can be passed to a PRM handler.

  @param[in] StaticDataBuffer           A pointer to the static data buffer that will be referenced in the context
                                        buffer that is populated. This is an optional pointer that, if not provided,
                                        by passing NULL will be ignored.
  @param[in] HandlerGuid                A pointer to the GUID of the PRM handler.
  @param[in] ContextBuffer              A pointer to a caller allocated ContextBuffer structure that will be populated
                                        by this function.

  @retval EFI_SUCCESS                   The given ContextBuffer was populated successfully.
  @retval EFI_INVALID_PARAMETER         The HandlerGuid or ContextBuffer actual argument is NULL.

**/
EFI_STATUS
PopulateContextBuffer (
  IN  PRM_DATA_BUFFER     *StaticDataBuffer OPTIONAL,
  IN  EFI_GUID            *HandlerGuid,
  IN  PRM_CONTEXT_BUFFER  *ContextBuffer
  )
{
  if ((HandlerGuid == NULL) || (ContextBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (ContextBuffer, sizeof (*ContextBuffer));

  ContextBuffer->Signature = PRM_CONTEXT_BUFFER_SIGNATURE;
  ContextBuffer->Version   = PRM_CONTEXT_BUFFER_INTERFACE_VERSION;
  CopyGuid (&ContextBuffer->HandlerGuid, HandlerGuid);

  if (StaticDataBuffer != NULL) {
    ContextBuffer->StaticDataBuffer = StaticDataBuffer;
  }

  return EFI_SUCCESS;
}

/**
  Prints a given execution time in the appropriate unit.

  @param[in] TimeInNanoSec              The time to print in unit of nanoseconds.

**/
VOID
PrintExecutionTime (
  IN  UINT64  TimeInNanoSec
  )
{
  UINT64  Sec;
  UINT64  MilliSec;
  UINT64  MicroSec;
  UINT64  NanoSec;
  UINT64  RemainingTime;

  Sec           = 0;
  MilliSec      = 0;
  MicroSec      = 0;
  NanoSec       = 0;
  RemainingTime = TimeInNanoSec;

  if (RemainingTime > ONE_SECOND) {
    Sec            = DivU64x32 (RemainingTime, ONE_SECOND);
    RemainingTime -= MultU64x32 (Sec, ONE_SECOND);
  }

  if (RemainingTime > ONE_MILLISECOND) {
    MilliSec       = DivU64x32 (RemainingTime, ONE_MILLISECOND);
    RemainingTime -= MultU64x32 (MilliSec, ONE_MILLISECOND);
  }

  if (RemainingTime > ONE_MICROSECOND) {
    MicroSec       = DivU64x32 (RemainingTime, ONE_MICROSECOND);
    RemainingTime -= MultU64x32 (MicroSec, ONE_MICROSECOND);
  }

  if (RemainingTime > 0) {
    NanoSec = RemainingTime;
  }

  if (Sec > 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_SECS), mPrmInfoHiiHandle, Sec, MilliSec, MicroSec, NanoSec);
  } else if (MilliSec > 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_MILLI_SECS), mPrmInfoHiiHandle, MilliSec, MicroSec, NanoSec);
  } else if (MicroSec > 0) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_USECS), mPrmInfoHiiHandle, MicroSec, NanoSec);
  } else {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NANO_SECS), mPrmInfoHiiHandle, NanoSec);
  }
}

/**
  Executes the PRM handler with the provided GUID.

  @param[in] HandlerGuid                A pointer to the GUID of the PRM handler to execute.
                                        A zero GUID indicates that all PRM handlers present should be executed.

  @retval EFI_SUCCESS                   The PRM handler(s) were executed.
  @retval EFI_INVALID_PARAMETER         The HandlerGuid actual argument is NULL.
  @retval EFI_NOT_FOUND                 The PRM handler could not be found.

**/
EFI_STATUS
ExecutePrmHandlerByGuid (
  IN  EFI_GUID  *HandlerGuid
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         ExecuteAllHandlers;
  BOOLEAN                         HandlerFound;
  UINT64                          StartTime;
  UINT64                          EndTime;
  PRM_CONTEXT_BUFFER              CurrentContextBuffer;
  PRM_HANDLER_CONTEXT             *HandlerContext;
  PRM_HANDLER_CONTEXT_LIST_ENTRY  *HandlerContextListEntry;
  LIST_ENTRY                      *Link;

  Link         = NULL;
  HandlerFound = FALSE;

  if (HandlerGuid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Zero GUID means execute all discovered handlers
  //
  ExecuteAllHandlers = CompareGuid (HandlerGuid, &gZeroGuid);

  EFI_LIST_FOR_EACH (Link, &mPrmHandlerList) {
    HandlerContextListEntry = CR (Link, PRM_HANDLER_CONTEXT_LIST_ENTRY, Link, PRM_HANDLER_CONTEXT_LIST_ENTRY_SIGNATURE);
    HandlerContext          = &HandlerContextListEntry->Context;

    if (!ExecuteAllHandlers && !CompareGuid (HandlerGuid, HandlerContext->Guid)) {
      continue;
    }

    HandlerFound = TRUE;
    Status       = PopulateContextBuffer (HandlerContext->StaticDataBuffer, HandlerContext->Guid, &CurrentContextBuffer);
    if (!EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_PRMINFO_MODULE_NAME),
        mPrmInfoHiiHandle,
        HandlerContext->ModuleName
        );
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_NAME_HL), mPrmInfoHiiHandle, HandlerContext->Name);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_GUID), mPrmInfoHiiHandle, HandlerContext->Guid);

      StartTime = 0;
      EndTime   = 0;
      if (PcdGetBool (PcdPrmInfoPrintHandlerExecutionTime)) {
        StartTime = GetPerformanceCounter ();
      }

      Status = HandlerContext->Handler (NULL, &CurrentContextBuffer);
      if (PcdGetBool (PcdPrmInfoPrintHandlerExecutionTime)) {
        EndTime = GetPerformanceCounter ();
      }

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_ERR_STATUS), mPrmInfoHiiHandle, Status);
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_SUCC_STATUS), mPrmInfoHiiHandle, Status);
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_EXEC_TIME), mPrmInfoHiiHandle);
      if ((StartTime == 0) && (EndTime == 0)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_UNKNOWN), mPrmInfoHiiHandle);
      } else {
        PrintExecutionTime (GetTimeInNanoSecond (EndTime - StartTime));
      }

      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "%a - %a: An error occurred creating a context buffer for handler %g\n",
        gEfiCallerBaseName,
        __FUNCTION__,
        HandlerContext->Guid
        ));
    }

    if (!ExecuteAllHandlers) {
      break;
    }
  }

  if (!HandlerFound) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Parses the application parameter list and performs the appropriate operations based on the results.

  @retval EFI_SUCCESS                   The parameter list was parsed successfully.
  @retval EFI_INVALID_PARAMETER         An invalid parameter was given to the application.
  @retval EFI_LOAD_ERROR                An error occurred loading the application.

**/
EFI_STATUS
ParseParameterList (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_STATUS    ReturnStatus;
  UINTN         ArgumentCount;
  EFI_GUID      HandlerGuid;
  BOOLEAN       PrintHandlerInfo;
  LIST_ENTRY    *Package;
  LIST_ENTRY    *TempNode;
  CHAR16        *ProblemParam;
  CONST CHAR16  *HandlerGuidStr;

  HandlerGuidStr   = NULL;
  Package          = NULL;
  PrintHandlerInfo = FALSE;
  ReturnStatus     = EFI_SUCCESS;

  InitializeListHead (&mPrmHandlerList);

  //
  // Basic application parameter validation
  //
  Status = ShellCommandLineParseEx (mParamList, &Package, &ProblemParam, FALSE, FALSE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_GEN_PROBLEM), mPrmInfoHiiHandle, APPLICATION_NAME, ProblemParam);
      ReturnStatus = EFI_INVALID_PARAMETER;
      FreePool (ProblemParam);
    } else {
      ReturnStatus = EFI_LOAD_ERROR;
      ASSERT (FALSE);
    }

    goto Done;
  } else if (Package == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NO_ARG), mPrmInfoHiiHandle, APPLICATION_NAME);
    ReturnStatus = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // Get argument count including flags
  //
  for (
       ArgumentCount = 0, TempNode = Package;
       GetNextNode (Package, TempNode) != Package;
       ArgumentCount++, TempNode = GetNextNode (Package, TempNode)
       )
  {
  }

  if (ArgumentCount == 1) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NO_ARG), mPrmInfoHiiHandle, APPLICATION_NAME);
    ReturnStatus = EFI_INVALID_PARAMETER;
    goto Done;
  }

  if (ArgumentCount > 6) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_TOO_MANY), mPrmInfoHiiHandle, APPLICATION_NAME);
    ReturnStatus = EFI_INVALID_PARAMETER;
    goto Done;
  }

  //
  // Parse the actual arguments provided
  //
  if (ShellCommandLineGetFlag (Package, L"-b")) {
    if (ArgumentCount <= 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_PARAM_INV), mPrmInfoHiiHandle, APPLICATION_NAME, L"-b");
      ReturnStatus = EFI_INVALID_PARAMETER;
      goto Done;
    } else {
      ShellSetPageBreakMode (TRUE);
    }
  }

  if (ShellCommandLineGetFlag (Package, L"-l")) {
    PrintHandlerInfo = TRUE;
  }

  if (ShellCommandLineGetFlag (Package, L"-t")) {
    HandlerGuidStr = ShellCommandLineGetValue (Package, L"-t");
    if (HandlerGuidStr != NULL) {
      if (StrnCmp (HandlerGuidStr, L"all", StrLen (HandlerGuidStr)) == 0) {
        CopyGuid (&HandlerGuid, &gZeroGuid);
      } else {
        Status = StrToGuid (HandlerGuidStr, &HandlerGuid);
        if (EFI_ERROR (Status) || (HandlerGuidStr[GUID_STRING_LENGTH] != L'\0')) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_GUID_INV), mPrmInfoHiiHandle, APPLICATION_NAME, HandlerGuidStr);
          ReturnStatus = EFI_INVALID_PARAMETER;
          goto Done;
        }
      }
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_NO_VALUE), mPrmInfoHiiHandle, APPLICATION_NAME, L"-t");
      ReturnStatus = EFI_INVALID_PARAMETER;
      goto Done;
    }
  }

  Status = DiscoverPrmModules (&mPrmModuleCount, &mPrmHandlerCount);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_DISCOVERY_FAILED), mPrmInfoHiiHandle, APPLICATION_NAME);
    DEBUG ((
      DEBUG_ERROR,
      "%a - %a: An error occurred during PRM module discovery (%r)\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
    ReturnStatus = Status;
    goto Done;
  }

  if (PrintHandlerInfo) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LIST_TITLE), mPrmInfoHiiHandle);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_MODULES_FOUND), mPrmInfoHiiHandle, mPrmModuleCount);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLERS_FOUND), mPrmInfoHiiHandle, mPrmHandlerCount);
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_LINE_BREAK), mPrmInfoHiiHandle);
  }

  GatherPrmHandlerInfo (PrintHandlerInfo);

  if (HandlerGuidStr != NULL) {
    Status = ExecutePrmHandlerByGuid (&HandlerGuid);
    if (Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_PRMINFO_HANDLER_NOT_FOUND), mPrmInfoHiiHandle, APPLICATION_NAME, HandlerGuid);
    }
  }

Done:
  FreeList (&mPrmHandlerList);

  if (Package != NULL) {
    ShellCommandLineFreeVarList (Package);
  }

  return ReturnStatus;
}

/**
  Entry point of this UEFI application.

  @param[in] ImageHandle                The firmware allocated handle for the EFI image.
  @param[in] SystemTable                A pointer to the EFI System Table.

  @retval EFI_SUCCESS                   The entry point is executed successfully.
  @retval other                         Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;

  //
  // Retrieve the HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Publish the HII package list to the HII Database
  //
  Status =  gHiiDatabase->NewPackageList (
                            gHiiDatabase,
                            PackageList,
                            NULL,
                            &mPrmInfoHiiHandle
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mPrmInfoHiiHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = ParseParameterList ();
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a - %a: An error occurred parsing user-provided arguments (%r)\n",
      gEfiCallerBaseName,
      __FUNCTION__,
      Status
      ));
  }

  if (mPrmInfoHiiHandle != NULL) {
    HiiRemovePackages (mPrmInfoHiiHandle);
  }

  return EFI_SUCCESS;
}
