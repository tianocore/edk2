/** @file
  Depex debug helper for DXE Core.

  This file provides diagnostic capabilities for DXE driver dependency
  expressions (Depex). It helps developers understand why drivers are not
  being dispatched by converting Depex RPN (Reverse Polish Notation) streams
  into human-readable format and providing detailed dependency analysis.

  Copyright (c) 2026, Dell Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Protocol/LoadedImage.h>

//
// Type definitions (local to this file)
//
typedef struct {
  EFI_GUID    DriverGuid;
  EFI_GUID    ProtocolGuid;
  BOOLEAN     IsCycle;
} DEPENDENCY_LINK;

typedef struct {
  DEPENDENCY_LINK    *Links;
  UINTN              LinkCount;
  UINTN              Capacity;
  BOOLEAN            CycleDetected;
  UINTN              CycleStartIndex;
} DEPENDENCY_CHAIN;

typedef struct {
  EFI_GUID    FileNameGuid;
  BOOLEAN     Discovered;
  BOOLEAN     Scheduled;
  BOOLEAN     Loaded;
  BOOLEAN     Requested;
  BOOLEAN     Unrequested;
  BOOLEAN     Untrusted;
  BOOLEAN     Initialized;
  BOOLEAN     OpenProtoLocked;
  BOOLEAN     DepexProtocolError;
  BOOLEAN     LastEvalTrue;
  UINT32      Round;
} DEPEX_DIAG_DRIVER_STATE;

typedef struct {
  CHAR8    *Buffer;
  UINTN    Capacity;
  UINTN    Length;
} STRING_BUILDER;

//
// Constants for string buffer sizes
//
#define DEPEX_STRING_BUFFER_SIZE       64
#define DEPEX_INITIAL_CHAIN_CAPACITY   16
#define DEPEX_CHAIN_GROWTH_MULTIPLIER  2
#define DEPEX_STACK_SIZE               32
#define DEPEX_MAX_GUID_STRING_LENGTH   64
#define DEPEX_OUTPUT_BUFFER_SIZE       4096
#define DEPEX_STRING_EXTRA_SIZE        4

//
// Constants for debug output formatting
//
#define DEPEX_DEBUG_INDENT_SPACING  2
#define DEPEX_DEBUG_OTHER_FLAGS     "  Other flags:\n"

// Standard Depex opcodes per PI spec
#define EFI_DEP_BEFORE        0x00
#define EFI_DEP_AFTER         0x01
#define EFI_DEP_PUSH          0x02
#define EFI_DEP_AND           0x03
#define EFI_DEP_OR            0x04
#define EFI_DEP_NOT           0x05
#define EFI_DEP_TRUE          0x06
#define EFI_DEP_FALSE         0x07
#define EFI_DEP_END           0x08
#define EFI_DEP_SOR           0x09
#define EFI_DEP_REPLACE_TRUE  0xff

#define MAX_TRACE_DEPTH  6

// External variables from Dispatcher.c
extern LIST_ENTRY  mDiscoveredList;

// Internal cache variables
STATIC DEPEX_DIAG_DRIVER_STATE  *mDriverStateCache = NULL;
STATIC UINTN                    mDriverStateCount  = 0;

/**
  Convert a GUID to a string representation for debug output.

  @param Guid       Pointer to the GUID to convert.
  @param Buffer     Buffer to store the GUID string.
  @param BufferSize Size of the buffer in bytes.

  @retval Pointer to the GUID string, or NULL if conversion fails.
**/
STATIC
CHAR8 *
GuidToString (
  IN EFI_GUID  *Guid,
  IN CHAR8     *Buffer,
  IN UINTN     BufferSize
  )
{
  if ((Guid == NULL) || (Buffer == NULL)) {
    return NULL;
  }

  AsciiSPrint (Buffer, BufferSize, "%g", Guid);
  return Buffer;
}

/**
  Build a cache of driver state information from the DXE core's discovered list.

  @param Cache  Pointer to receive the allocated driver state array.
  @param Count  Pointer to receive the number of drivers in the cache.

  @retval EFI_SUCCESS            Cache successfully built.
  @retval EFI_INVALID_PARAMETER  Cache or Count is NULL.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
DiagBuildDriverStateCache (
  OUT DEPEX_DIAG_DRIVER_STATE  **Cache,
  OUT UINTN                    *Count
  )
{
  UINTN                    DiscoveredCount;
  LIST_ENTRY               *Link;
  DEPEX_DIAG_DRIVER_STATE  *Array;
  UINTN                    Index;
  EFI_CORE_DRIVER_ENTRY    *DriverEntry;

  if ((Cache == NULL) || (Count == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Cache = NULL;
  *Count = 0;

  DiscoveredCount = 0;
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DiscoveredCount++;
  }

  if (DiscoveredCount == 0) {
    return EFI_SUCCESS;
  }

  Array = (DEPEX_DIAG_DRIVER_STATE *)AllocateZeroPool (sizeof (DEPEX_DIAG_DRIVER_STATE) * DiscoveredCount);
  if (Array == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Index = 0;
  for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
    DriverEntry                     = CR (Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
    Array[Index].FileNameGuid       = DriverEntry->FileName;
    Array[Index].Discovered         = TRUE;
    Array[Index].Unrequested        = DriverEntry->Unrequested;
    Array[Index].Requested          = (BOOLEAN) !DriverEntry->Unrequested;
    Array[Index].Scheduled          = DriverEntry->Scheduled;
    Array[Index].Initialized        = DriverEntry->Initialized;
    Array[Index].Untrusted          = DriverEntry->Untrusted;
    Array[Index].DepexProtocolError = DriverEntry->DepexProtocolError;
    Array[Index].Loaded             = DriverEntry->Initialized;
    Array[Index].LastEvalTrue       = FALSE;
    Array[Index].Round              = 0;
    Index++;
  }

  *Cache = Array;
  *Count = DiscoveredCount;
  return EFI_SUCCESS;
}

/**
  Query protocol provider information for a given protocol GUID.

  @param  ProtocolGuid Pointer to the protocol GUID to query.
  @param Providers   Pointer to receive the count of protocol providers (optional).
  @param Exists      Pointer to receive whether the protocol exists (optional).

  @retval EFI_SUCCESS  Query completed successfully.
  @retval Other        Error from LocateProtocol or LocateHandleBuffer.
**/
STATIC
EFI_STATUS
DiagQueryProtocolProviders (
  IN  EFI_GUID  *ProtocolGuid,
  OUT UINTN     *Providers,
  OUT BOOLEAN   *Exists
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  *Handles;
  UINTN       Count;
  VOID        *Interface;

  Handles   = NULL;
  Count     = 0;
  Interface = NULL;

  if (Providers != NULL) {
    *Providers = 0;
  }

  if (Exists != NULL) {
    *Exists = FALSE;
  }

  Status = gBS->LocateProtocol (ProtocolGuid, NULL, &Interface);
  if (!EFI_ERROR (Status)) {
    if (Exists != NULL) {
      *Exists = TRUE;
    }
  } else if (Status != EFI_NOT_FOUND) {
    return Status;
  }

  Status = gBS->LocateHandleBuffer (ByProtocol, ProtocolGuid, NULL, &Count, &Handles);
  if (Status == EFI_NOT_FOUND) {
    if (Providers != NULL) {
      *Providers = 0;
    }

    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Providers != NULL) {
    *Providers = Count;
  }

  if (Handles != NULL) {
    FreePool (Handles);
  }

  return EFI_SUCCESS;
}

/**
  Lightweight Depex expression evaluator for root cause analysis.

  @param  Depex     Pointer to the Depex buffer.
  @param  Length    Size of the Depex buffer in bytes.
  @param  Verbose   If TRUE, prints verbose debug output.
  @param FailGuid  Pointer to receive the GUID of the first missing protocol (optional).

  @retval TRUE   Depex expression evaluates to TRUE (all dependencies satisfied).
  @retval FALSE  Depex expression evaluates to FALSE or contains errors.
**/
STATIC
BOOLEAN
EvaluateDepexLite (
  UINT8     *Depex,
  UINTN     Length,
  BOOLEAN   Verbose,
  EFI_GUID  **FailGuid
  )
{
  UINTN     Index;
  BOOLEAN   Stack[DEPEX_STACK_SIZE];
  UINTN     Sp;
  EFI_GUID  *Fail;
  UINT8     Op;
  EFI_GUID  *Guid;
  UINTN     Providers;
  BOOLEAN   Ok;

  Index = 0;
  Sp    = 0;
  Fail  = NULL;

  while (Index < Length) {
    Op = Depex[Index++];

    switch (Op) {
      case EFI_DEP_PUSH:
        Guid   = (EFI_GUID *)&Depex[Index];
        Index += sizeof (EFI_GUID);

        DiagQueryProtocolProviders (Guid, &Providers, NULL);
        Ok = (Providers > 0);

        if (Verbose) {
          DEBUG ((
            DEBUG_VERBOSE,
            "  Protocol(%g): providers=%u %a\n",
            Guid,
            (UINT32)Providers,
            Ok ? "OK" : "MISSING"
            ));
        }

        if (!Ok && (Fail == NULL)) {
          Fail = Guid;
        }

        if (Sp >= DEPEX_STACK_SIZE) {
          return FALSE;
        }

        Stack[Sp++] = Ok;
        break;

      case EFI_DEP_AND:
        if (Sp < 2) {
          return FALSE;
        }

        Stack[Sp-2] = Stack[Sp-2] && Stack[Sp-1];
        Sp--;
        break;

      case EFI_DEP_OR:
        if (Sp < 2) {
          return FALSE;
        }

        Stack[Sp-2] = Stack[Sp-2] || Stack[Sp-1];
        Sp--;
        break;

      case EFI_DEP_NOT:
        if (Sp < 1) {
          return FALSE;
        }

        Stack[Sp-1] = !Stack[Sp-1];
        break;

      case EFI_DEP_TRUE:
        if (Sp >= DEPEX_STACK_SIZE) {
          return FALSE;
        }

        Stack[Sp++] = TRUE;
        break;

      case EFI_DEP_FALSE:
        if (Sp >= DEPEX_STACK_SIZE) {
          return FALSE;
        }

        Stack[Sp++] = FALSE;
        break;

      case EFI_DEP_END:
        goto Done;

      default:
        return FALSE;
    }
  }

Done:
  if (FailGuid != NULL) {
    *FailGuid = Fail;
  }

  return (Sp > 0) ? Stack[Sp-1] : FALSE;
}

/**
  Initialize a dependency chain structure.

  @param Chain Pointer to the dependency chain structure to initialize.

  @retval EFI_SUCCESS            Chain successfully initialized.
  @retval EFI_INVALID_PARAMETER  Chain is NULL.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
ChainInit (
  OUT DEPENDENCY_CHAIN  *Chain
  )
{
  if (Chain == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Chain->Links = (DEPENDENCY_LINK *)AllocateZeroPool (sizeof (DEPENDENCY_LINK) * DEPEX_INITIAL_CHAIN_CAPACITY);
  if (Chain->Links == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Chain->Capacity        = DEPEX_INITIAL_CHAIN_CAPACITY;
  Chain->LinkCount       = 0;
  Chain->CycleDetected   = FALSE;
  Chain->CycleStartIndex = 0;

  return EFI_SUCCESS;
}

/**
  Free resources allocated for a dependency chain.

  @param Chain Pointer to the dependency chain structure to free.
**/
STATIC
VOID
ChainFree (
  IN OUT DEPENDENCY_CHAIN  *Chain
  )
{
  if ((Chain != NULL) && (Chain->Links != NULL)) {
    FreePool (Chain->Links);
    Chain->Links     = NULL;
    Chain->LinkCount = 0;
  }
}

/**
  Add a dependency link to the chain.

  @param Chain        Pointer to the dependency chain structure.
  @param DriverGuid   GUID of the driver requiring the protocol.
  @param ProtocolGuid GUID of the protocol being depended on.
  @param IsCycle      Flag indicating if this link creates a cycle.

  @retval EFI_SUCCESS            Link successfully added.
  @retval EFI_INVALID_PARAMETER  Chain, DriverGuid, or ProtocolGuid is NULL.
  @retval EFI_OUT_OF_RESOURCES   Memory reallocation failed.
**/
STATIC
EFI_STATUS
ChainAddLink (
  IN OUT DEPENDENCY_CHAIN  *Chain,
  IN     EFI_GUID          *DriverGuid,
  IN     EFI_GUID          *ProtocolGuid,
  IN     BOOLEAN           IsCycle
  )
{
  DEPENDENCY_LINK  *NewLinks;

  if ((Chain == NULL) || (DriverGuid == NULL) || (ProtocolGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Chain->LinkCount >= Chain->Capacity) {
    NewLinks = ReallocatePool (
                 Chain->Capacity * sizeof (DEPENDENCY_LINK),
                 (Chain->Capacity * DEPEX_CHAIN_GROWTH_MULTIPLIER) * sizeof (DEPENDENCY_LINK),
                 Chain->Links
                 );
    if (NewLinks == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Chain->Links     = (DEPENDENCY_LINK *)NewLinks;
    Chain->Capacity *= DEPEX_CHAIN_GROWTH_MULTIPLIER;
  }

  CopyGuid (&Chain->Links[Chain->LinkCount].DriverGuid, DriverGuid);
  CopyGuid (&Chain->Links[Chain->LinkCount].ProtocolGuid, ProtocolGuid);
  Chain->Links[Chain->LinkCount].IsCycle = IsCycle;
  Chain->LinkCount++;

  return EFI_SUCCESS;
}

/**
  Check if a driver is already present in the dependency chain.

  @param Chain      Pointer to the dependency chain structure.
  @param DriverGuid GUID of the driver to search for.

  @retval TRUE  Driver is found in the chain (cycle detected).
  @retval FALSE Driver is not found in the chain.
**/
STATIC
BOOLEAN
IsDriverInChain (
  IN DEPENDENCY_CHAIN  *Chain,
  IN EFI_GUID          *DriverGuid
  )
{
  UINTN  Index;

  if ((Chain == NULL) || (DriverGuid == NULL)) {
    return FALSE;
  }

  for (Index = 0; Index < Chain->LinkCount; Index++) {
    if (CompareGuid (&Chain->Links[Index].DriverGuid, DriverGuid)) {
      Chain->CycleStartIndex = Index;
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Recursively trace dependency chains from a starting driver.

  @param CurrentDriver GUID of the current driver being traced.
  @param Depth         Current recursion depth (prevents infinite recursion).
  @param Chain         Pointer to the dependency chain to build.
**/
STATIC
VOID
TraceChainRecursive (
  IN     EFI_GUID          *CurrentDriver,
  IN     UINTN             Depth,
  IN OUT DEPENDENCY_CHAIN  *Chain
  )
{
  EFI_CORE_DRIVER_ENTRY  *DriverEntry;
  EFI_CORE_DRIVER_ENTRY  *Entry;
  LIST_ENTRY             *Link;
  UINT8                  *Depex;
  UINTN                  DepexSize;
  UINTN                  Index;
  UINT8                  OpCode;
  EFI_GUID               ProtocolGuid;
  EFI_HANDLE             *Handles;
  UINTN                  HandleCount;
  EFI_STATUS             Status;
  UINTN                  Providers;
  BOOLEAN                Exists;

  Handles     = NULL;
  HandleCount = 0;

  if (Depth >= MAX_TRACE_DEPTH) {
    return;
  }

  if (IsDriverInChain (Chain, CurrentDriver)) {
    Chain->CycleDetected = TRUE;
    if (Chain->LinkCount > 0) {
      Chain->Links[Chain->LinkCount - 1].IsCycle = TRUE;
    }

    return;
  }

  DriverEntry = NULL;
  for (Index = 0; Index < mDriverStateCount; Index++) {
    if (CompareGuid (&mDriverStateCache[Index].FileNameGuid, CurrentDriver)) {
      for (Link = mDiscoveredList.ForwardLink; Link != &mDiscoveredList; Link = Link->ForwardLink) {
        Entry = CR (Link, EFI_CORE_DRIVER_ENTRY, Link, EFI_CORE_DRIVER_ENTRY_SIGNATURE);
        if (CompareGuid (&Entry->FileName, CurrentDriver)) {
          DriverEntry = Entry;
          break;
        }
      }

      break;
    }
  }

  if (DriverEntry == NULL) {
    return;
  }

  Depex     = (UINT8 *)DriverEntry->Depex;
  DepexSize = DriverEntry->DepexSize;

  Index = 0;
  while (Index < DepexSize) {
    OpCode = Depex[Index];

    if (OpCode == EFI_DEP_PUSH) {
      CopyMem (&ProtocolGuid, &Depex[Index + 1], sizeof (EFI_GUID));
      Index += 1 + sizeof (EFI_GUID);

      DiagQueryProtocolProviders (&ProtocolGuid, &Providers, &Exists);

      if (!Exists) {
        ChainAddLink (Chain, CurrentDriver, &ProtocolGuid, FALSE);
        return;
      }

      Handles     = NULL;
      HandleCount = 0;
      Status      = gBS->LocateHandleBuffer (ByProtocol, &ProtocolGuid, NULL, &HandleCount, &Handles);
      if (!EFI_ERROR (Status) && (Handles != NULL) && (HandleCount > 0)) {
        ChainAddLink (Chain, CurrentDriver, &ProtocolGuid, FALSE);
        FreePool (Handles);
      }
    } else if (OpCode == EFI_DEP_END) {
      Index++;
      break;
    } else if ((OpCode == EFI_DEP_AND) || (OpCode == EFI_DEP_OR) || (OpCode == EFI_DEP_NOT)) {
      Index++;
    } else if ((OpCode == EFI_DEP_BEFORE) || (OpCode == EFI_DEP_AFTER)) {
      Index += 1 + sizeof (EFI_GUID);
    } else if ((OpCode == EFI_DEP_TRUE) || (OpCode == EFI_DEP_FALSE) || (OpCode == EFI_DEP_SOR)) {
      Index++;
    } else {
      Index++;
    }
  }
}

/**
  Print a formatted dependency chain to debug output.

  @param Chain           Pointer to the dependency chain to print.
  @param RootDriver      GUID of the root driver (for reference).
  @param MissingProtocol GUID of the missing protocol causing the failure (optional).
**/
STATIC
VOID
PrintDependencyChain (
  IN DEPENDENCY_CHAIN  *Chain,
  IN EFI_GUID          *RootDriver,
  IN EFI_GUID          *MissingProtocol
  )
{
  UINTN  Index;
  CHAR8  GuidStr[DEPEX_MAX_GUID_STRING_LENGTH];

  if ((Chain == NULL) || (Chain->LinkCount == 0)) {
    return;
  }

  DEBUG ((DEBUG_VERBOSE, "[DEPENDENCY CHAIN]" "\n"));

  for (Index = 0; Index < Chain->LinkCount; Index++) {
    if (Index == 0) {
      GuidToString (&Chain->Links[Index].ProtocolGuid, GuidStr, sizeof (GuidStr));
      DEBUG ((DEBUG_VERBOSE, "  waiting for Protocol(%a)\n", GuidStr));
    } else {
      GuidToString (&Chain->Links[Index - 1].DriverGuid, GuidStr, sizeof (GuidStr));
      DEBUG ((DEBUG_VERBOSE, "    provided by Driver %a\n", GuidStr));

      if (Chain->Links[Index].IsCycle) {
        DEBUG ((
          DEBUG_VERBOSE,
          "      but Driver %g waiting for Protocol(%g)\n",
          &Chain->Links[Index].DriverGuid,
          &Chain->Links[Index].ProtocolGuid
          ));
        DEBUG ((DEBUG_VERBOSE, "        -> MISSING\n"));
      } else if (Index == Chain->LinkCount - 1) {
        DEBUG ((
          DEBUG_VERBOSE,
          "      but Driver %g waiting for Protocol(%g)\n",
          &Chain->Links[Index].DriverGuid,
          &Chain->Links[Index].ProtocolGuid
          ));
        DEBUG ((DEBUG_VERBOSE, "        -> MISSING\n"));
      }
    }
  }

  if (Chain->LinkCount == 1) {
    DEBUG ((DEBUG_VERBOSE, "    (no provider found)\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "[ROOT CAUSE]" "\n"));
  if (MissingProtocol != NULL) {
    DEBUG ((DEBUG_VERBOSE, "  Protocol(%g) missing\n", MissingProtocol));
  } else if (Chain->CycleDetected) {
    DEBUG ((DEBUG_VERBOSE, "  " "circular dependency ("));
    for (Index = Chain->CycleStartIndex; Index < Chain->LinkCount; Index++) {
      if (Index > Chain->CycleStartIndex) {
        DEBUG ((DEBUG_VERBOSE, " <-> "));
      }

      DEBUG ((DEBUG_VERBOSE, "%g", &Chain->Links[Index].DriverGuid));
    }

    DEBUG ((DEBUG_VERBOSE, ")\n"));
  }
}

/**
  Print a comprehensive summary of a driver's Depex and status.

  This function analyzes the driver's Depex, evaluates the truth table of dependencies,
  checks for missing protocols, and prints the results to the debug log.

  @param[in] DriverEntry  Pointer to the Core driver entry.

  @retval EFI_SUCCESS             Summary printed successfully.
  @retval EFI_INVALID_PARAMETER   DriverEntry is NULL.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
CorePrintDriverDepexSummary (
  IN EFI_CORE_DRIVER_ENTRY  *DriverEntry
  )
{
  UINT8             *Depex;
  BOOLEAN           Verbose;
  EFI_GUID          *FailGuid;
  BOOLEAN           Result;
  DEPENDENCY_CHAIN  Chain;
  EFI_STATUS        Status;
  UINTN             Providers;
  BOOLEAN           Exists;

  if (DriverEntry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ChainInit (&Chain);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (mDriverStateCache == NULL) {
    DiagBuildDriverStateCache (&mDriverStateCache, &mDriverStateCount);
  }

  Depex    = (UINT8 *)DriverEntry->Depex;
  Verbose  = PcdGetBool (PcdDepexDebugVerbose);
  FailGuid = NULL;

  Result = EvaluateDepexLite (Depex, DriverEntry->DepexSize, Verbose, &FailGuid);

  if (!Result) {
    DEBUG ((
      DEBUG_INFO,
      "Driver %g not dispatched\n",
      &DriverEntry->FileName
      ));

    if (FailGuid != NULL) {
      DEBUG ((
        DEBUG_INFO,
        "[DEPEX] missing Protocol(%g)\n",
        FailGuid
        ));
    }

    if (Verbose && (FailGuid != NULL)) {
      DEBUG ((DEBUG_VERBOSE, "\n"));
      DEBUG ((DEBUG_VERBOSE, "[DEPEX TRACE]\n"));

      DiagQueryProtocolProviders (FailGuid, &Providers, &Exists);
      if (Exists) {
        DEBUG ((DEBUG_VERBOSE, "  Protocol(%g): providers=%u OK\n", FailGuid, (UINT32)Providers));
      } else {
        DEBUG ((DEBUG_VERBOSE, "  Protocol(%g): providers=%u MISSING\n", FailGuid, (UINT32)Providers));
      }

      DEBUG ((DEBUG_VERBOSE, "\n"));

      Chain.LinkCount = 0;
      TraceChainRecursive (&DriverEntry->FileName, 0, &Chain);
      PrintDependencyChain (&Chain, &DriverEntry->FileName, FailGuid);
    }
  }

  ChainFree (&Chain);

  return EFI_SUCCESS;
}
