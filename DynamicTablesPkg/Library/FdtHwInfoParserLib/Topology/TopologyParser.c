/** @file
  Generic topology parser.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>

#include "Topology/TopologyHierarchyParser.h"
#include "Topology/TopologyParser.h"
#include "Topology/TopologyUtility.h"
#include "CmObjectDescUtility.h"

/** Free memory owned by a topology parser context.

  @param [in, out] Context  Topology parser context.
**/
STATIC
VOID
EFIAPI
FreeTopologyContext (
  IN OUT TOPOLOGY_PARSER_CONTEXT  *Context
  )
{
  if (Context == NULL) {
    return;
  }

  if (Context->ProcHierarchyBuffers != NULL) {
    FreePool (Context->ProcHierarchyBuffers);
  }

  if (Context->CpuNodes != NULL) {
    FreePool (Context->CpuNodes);
  }
}

/** Processor topology parser.

  Parse a Device Tree and populate the following objects:
  - CM_ARCH_COMMON_PROC_HIERARCHY_INFO
  - CM_ARCH_COMMON_CACHE_INFO
  - CM_ARCH_COMMON_OBJ_REF arrays referenced by processor hierarchy nodes

  @param [in]  FdtParserHandle  Parser instance handle.
  @param [in]  FdtBranch        Unused. The parser always resolves the global
                                "\cpus" node.

  @retval EFI_ABORTED             Malformed topology or cache information was
                                  found.
  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The "\cpus" node or CPU topology was not
                                  found.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
EFI_STATUS
EFIAPI
TopologyInfoParser (
  IN CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN INT32                            FdtBranch
  )
{
  EFI_STATUS               Status;
  TOPOLOGY_PARSER_CONTEXT  Context;
  INT32                    CpusNode;
  UINT32                   Index;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  CpusNode = FdtPathOffset (FdtParserHandle->Fdt, "/cpus");
  if (CpusNode < 0) {
    ASSERT (FALSE);
    return EFI_NOT_FOUND;
  }

  ZeroMem (&Context, sizeof (Context));
  Context.FdtParserHandle = FdtParserHandle;

  Status = CreateProcHierarchyInfo (&Context, CpusNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  for (Index = 0; Index < Context.ProcHierarchyCount; Index++) {
    Status = AddSingleCmObjWithToken (
               Context.FdtParserHandle,
               CREATE_CM_ARCH_COMMON_OBJECT_ID (EArchCommonObjProcHierarchyInfo),
               &Context.ProcHierarchyBuffers[Index].Info,
               sizeof (Context.ProcHierarchyBuffers[Index].Info),
               Context.ProcHierarchyBuffers[Index].Info.Token
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

exit_handler:
  FreeTopologyContext (&Context);
  return Status;
}
