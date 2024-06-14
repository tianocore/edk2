## @file
# Routines for generating AutoGen.h and AutoGen.c
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
from __future__ import absolute_import
import string
import collections
import struct
from Common import EdkLogger
from Common import GlobalData
from Common.BuildToolError import *
from Common.DataType import *
from Common.Misc import *
from Common.StringUtils import StringToArray
from .StrGather import *
from .GenPcdDb import CreatePcdDatabaseCode
from .IdfClassObject import *

import json
import secrets

## PCD type string
gItemTypeStringDatabase  = {
    TAB_PCDS_FEATURE_FLAG       :   TAB_PCDS_FIXED_AT_BUILD,
    TAB_PCDS_FIXED_AT_BUILD     :   TAB_PCDS_FIXED_AT_BUILD,
    TAB_PCDS_PATCHABLE_IN_MODULE:   'BinaryPatch',
    TAB_PCDS_DYNAMIC            :   '',
    TAB_PCDS_DYNAMIC_DEFAULT    :   '',
    TAB_PCDS_DYNAMIC_VPD        :   '',
    TAB_PCDS_DYNAMIC_HII        :   '',
    TAB_PCDS_DYNAMIC_EX         :   '',
    TAB_PCDS_DYNAMIC_EX_DEFAULT :   '',
    TAB_PCDS_DYNAMIC_EX_VPD     :   '',
    TAB_PCDS_DYNAMIC_EX_HII     :   '',
}


## Datum size
gDatumSizeStringDatabase = {TAB_UINT8:'8',TAB_UINT16:'16',TAB_UINT32:'32',TAB_UINT64:'64','BOOLEAN':'BOOLEAN',TAB_VOID:'8'}
gDatumSizeStringDatabaseH = {TAB_UINT8:'8',TAB_UINT16:'16',TAB_UINT32:'32',TAB_UINT64:'64','BOOLEAN':'BOOL',TAB_VOID:'PTR'}
gDatumSizeStringDatabaseLib = {TAB_UINT8:'8',TAB_UINT16:'16',TAB_UINT32:'32',TAB_UINT64:'64','BOOLEAN':'Bool',TAB_VOID:'Ptr'}

## AutoGen File Header Templates
gAutoGenHeaderString = TemplateString("""\
/**
  DO NOT EDIT
  FILE auto-generated
  Module name:
    ${FileName}
  Abstract:       Auto-generated ${FileName} for building module or library.
**/
""")

gAutoGenHPrologueString = TemplateString("""
#ifndef _${File}_${Guid}
#define _${File}_${Guid}

""")

gAutoGenHCppPrologueString = """\
#ifdef __cplusplus
extern "C" {
#endif

"""

gAutoGenHEpilogueString = """

#ifdef __cplusplus
}
#endif

#endif
"""

## PEI Core Entry Point Templates
gPeiCoreEntryPointPrototype = TemplateString("""
${BEGIN}
VOID
EFIAPI
${Function} (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN VOID                           *Context
  );
${END}
""")

gPeiCoreEntryPointString = TemplateString("""
${BEGIN}
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN VOID                           *Context
  )

{
  ${Function} (SecCoreData, PpiList, Context);
}
${END}
""")


## DXE Core Entry Point Templates
gDxeCoreEntryPointPrototype = TemplateString("""
${BEGIN}
VOID
EFIAPI
${Function} (
  IN VOID  *HobStart
  );
${END}
""")

gDxeCoreEntryPointString = TemplateString("""
${BEGIN}
VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID  *HobStart
  )

{
  ${Function} (HobStart);
}
${END}
""")

## PEIM Entry Point Templates
gPeimEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );
${END}
""")

gPeimEntryPointString = [
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};
${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  return ${Function} (FileHandle, PeiServices);
}
${END}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )

{
  EFI_STATUS  Status;
  EFI_STATUS  CombinedStatus;

  CombinedStatus = EFI_LOAD_ERROR;
${BEGIN}
  Status = ${Function} (FileHandle, PeiServices);
  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {
    CombinedStatus = Status;
  }
${END}
  return CombinedStatus;
}
""")
]

## SMM_CORE Entry Point Templates
gSmmCoreEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );
${END}
""")

gSmmCoreEntryPointString = TemplateString("""
${BEGIN}
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
""")

## MM_CORE_STANDALONE Entry Point Templates
gMmCoreStandaloneEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN VOID *HobStart
  );
${END}
""")

gMmCoreStandaloneEntryPointString = TemplateString("""
${BEGIN}
const UINT32 _gMmRevision = ${PiSpecVersion};

VOID
EFIAPI
ProcessModuleEntryPointList (
  IN VOID *HobStart
  )
{
  ${Function} (HobStart);
}
${END}
""")

## MM_STANDALONE Entry Point Templates
gMmStandaloneEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  );
${END}
""")

gMmStandaloneEntryPointString = [
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gMmRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  )

{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gMmRevision = ${PiSpecVersion};
${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  )

{
  return ${Function} (ImageHandle, MmSystemTable);
}
${END}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gMmRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  )

{
  EFI_STATUS  Status;
  EFI_STATUS  CombinedStatus;

  CombinedStatus = EFI_LOAD_ERROR;
${BEGIN}
  Status = ${Function} (ImageHandle, MmSystemTable);
  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {
    CombinedStatus = Status;
  }
${END}
  return CombinedStatus;
}
""")
]

## DXE SMM Entry Point Templates
gDxeSmmEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gDxeSmmEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus;

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mDriverEntryPointStatus = EFI_LOAD_ERROR;

${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
${END}

  return mDriverEntryPointStatus;
}
""")
]

## UEFI Driver Entry Point Templates
gUefiDriverEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gUefiDriverEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (gImageHandle, gST);
  }
  gBS->Exit (gImageHandle, Status, 0, NULL);
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};
const UINT32 _gDxeRevision = ${PiSpecVersion};

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus;

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mDriverEntryPointStatus = EFI_LOAD_ERROR;
  ${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
  ${END}
  return mDriverEntryPointStatus;
}

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}
""")
]


## UEFI Application Entry Point Templates
gUefiApplicationEntryPointPrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
${END}
""")

gUefiApplicationEntryPointString = [
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  return ${Function} (ImageHandle, SystemTable);
}
${END}
VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (EFI_ERROR (Status)) {
    ProcessLibraryDestructorList (gImageHandle, gST);
  }
  gBS->Exit (gImageHandle, Status, 0, NULL);
}
"""),
TemplateString("""
const UINT32 _gUefiDriverRevision = ${UefiSpecVersion};

EFI_STATUS
EFIAPI
ProcessModuleEntryPointList (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )

{
  ${BEGIN}
  if (SetJump (&mJumpContext) == 0) {
    ExitDriver (${Function} (ImageHandle, SystemTable));
    ASSERT (FALSE);
  }
  ${END}
  return mDriverEntryPointStatus;
}

static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;
static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;

VOID
EFIAPI
ExitDriver (
  IN EFI_STATUS  Status
  )
{
  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {
    mDriverEntryPointStatus = Status;
  }
  LongJump (&mJumpContext, (UINTN)-1);
  ASSERT (FALSE);
}
""")
]

## UEFI Unload Image Templates
gUefiUnloadImagePrototype = TemplateString("""
${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle
  );
${END}
""")

gUefiUnloadImageString = [
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  return EFI_SUCCESS;
}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

${BEGIN}
EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  return ${Function} (ImageHandle);
}
${END}
"""),
TemplateString("""
GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gDriverUnloadImageCount = ${Count};

EFI_STATUS
EFIAPI
ProcessModuleUnloadList (
  IN EFI_HANDLE        ImageHandle
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;
${BEGIN}
  if (EFI_ERROR (Status)) {
    ${Function} (ImageHandle);
  } else {
    Status = ${Function} (ImageHandle);
  }
${END}
  return Status;
}
""")
]

gLibraryStructorPrototype = {
SUP_MODULE_BASE  : TemplateString("""${BEGIN}
RETURN_STATUS
EFIAPI
${Function} (
  VOID
  );${END}
"""),

'PEI'   : TemplateString("""${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  );${END}
"""),

'DXE'   : TemplateString("""${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );${END}
"""),

'MM'   : TemplateString("""${BEGIN}
EFI_STATUS
EFIAPI
${Function} (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  );${END}
"""),
}

gLibraryStructorCall = {
SUP_MODULE_BASE  : TemplateString("""${BEGIN}
  Status = ${Function} ();
  ASSERT_RETURN_ERROR (Status);${END}
"""),

'PEI'   : TemplateString("""${BEGIN}
  Status = ${Function} (FileHandle, PeiServices);
  ASSERT_EFI_ERROR (Status);${END}
"""),

'DXE'   : TemplateString("""${BEGIN}
  Status = ${Function} (ImageHandle, SystemTable);
  ASSERT_EFI_ERROR (Status);${END}
"""),

'MM'   : TemplateString("""${BEGIN}
  Status = ${Function} (ImageHandle, MmSystemTable);
  ASSERT_EFI_ERROR (Status);${END}
"""),
}

## Library Constructor and Destructor Templates
gLibraryString = {
SUP_MODULE_BASE  :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  VOID
  )
{
${BEGIN}  RETURN_STATUS  Status;
${FunctionCall}${END}
}
"""),

'PEI'   :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  IN       EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),

'DXE'   :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),

'MM'   :   TemplateString("""
${BEGIN}${FunctionPrototype}${END}

VOID
EFIAPI
ProcessLibrary${Type}List (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_MM_SYSTEM_TABLE   *MmSystemTable
  )
{
${BEGIN}  EFI_STATUS  Status;
${FunctionCall}${END}
}
"""),
}

gBasicHeaderFile = "Base.h"

gModuleTypeHeaderFile = {
    SUP_MODULE_BASE              :   [gBasicHeaderFile, "Library/DebugLib.h"],
    SUP_MODULE_SEC               :   ["PiPei.h", "Library/DebugLib.h"],
    SUP_MODULE_PEI_CORE          :   ["PiPei.h", "Library/DebugLib.h", "Library/PeiCoreEntryPoint.h"],
    SUP_MODULE_PEIM              :   ["PiPei.h", "Library/DebugLib.h", "Library/PeimEntryPoint.h"],
    SUP_MODULE_DXE_CORE          :   ["PiDxe.h", "Library/DebugLib.h", "Library/DxeCoreEntryPoint.h"],
    SUP_MODULE_DXE_DRIVER        :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_DXE_SMM_DRIVER    :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_DXE_RUNTIME_DRIVER:   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_DXE_SAL_DRIVER    :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_UEFI_DRIVER       :   ["Uefi.h",  "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_UEFI_APPLICATION  :   ["Uefi.h",  "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiBootServicesTableLib.h", "Library/UefiApplicationEntryPoint.h"],
    SUP_MODULE_SMM_CORE          :   ["PiDxe.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/UefiDriverEntryPoint.h"],
    SUP_MODULE_MM_STANDALONE     :   ["PiMm.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/StandaloneMmDriverEntryPoint.h"],
    SUP_MODULE_MM_CORE_STANDALONE :  ["PiMm.h", "Library/BaseLib.h", "Library/DebugLib.h", "Library/StandaloneMmCoreEntryPoint.h"],
    SUP_MODULE_USER_DEFINED      :   [gBasicHeaderFile, "Library/DebugLib.h"],
    SUP_MODULE_HOST_APPLICATION  :   [gBasicHeaderFile, "Library/DebugLib.h"]
}

## Autogen internal worker macro to define DynamicEx PCD name includes both the TokenSpaceGuidName
#  the TokenName and Guid comparison to avoid define name collisions.
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenH    The TemplateString object for header file
#
#
def DynExPcdTokenNumberMapping(Info, AutoGenH):
    ExTokenCNameList = []
    PcdExList        = []
    # Even it is the Library, the PCD is saved in the ModulePcdList
    PcdList = Info.ModulePcdList
    for Pcd in PcdList:
        if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
            ExTokenCNameList.append(Pcd.TokenCName)
            PcdExList.append(Pcd)
    if len(ExTokenCNameList) == 0:
        return
    AutoGenH.Append('\n#define COMPAREGUID(Guid1, Guid2) (BOOLEAN)(*(CONST UINT64*)Guid1 == *(CONST UINT64*)Guid2 && *((CONST UINT64*)Guid1 + 1) == *((CONST UINT64*)Guid2 + 1))\n')
    # AutoGen for each PCD listed in a [PcdEx] section of a Module/Lib INF file.
    # Auto generate a macro for each TokenName that takes a Guid pointer as a parameter.
    # Use the Guid pointer to see if it matches any of the token space GUIDs.
    TokenCNameList = set()
    for TokenCName in ExTokenCNameList:
        if TokenCName in TokenCNameList:
            continue
        Index = 0
        Count = ExTokenCNameList.count(TokenCName)
        for Pcd in PcdExList:
            RealTokenCName = Pcd.TokenCName
            for PcdItem in GlobalData.MixedPcd:
                if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                    RealTokenCName = PcdItem[0]
                    break
            if Pcd.TokenCName == TokenCName:
                Index = Index + 1
                if Index == 1:
                    AutoGenH.Append('\n#define __PCD_%s_ADDR_CMP(GuidPtr)  (' % (RealTokenCName))
                    AutoGenH.Append('\\\n  (GuidPtr == &%s) ? _PCD_TOKEN_%s_%s:'
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, RealTokenCName))
                else:
                    AutoGenH.Append('\\\n  (GuidPtr == &%s) ? _PCD_TOKEN_%s_%s:'
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, RealTokenCName))
                if Index == Count:
                    AutoGenH.Append('0 \\\n  )\n')
                TokenCNameList.add(TokenCName)

    TokenCNameList = set()
    for TokenCName in ExTokenCNameList:
        if TokenCName in TokenCNameList:
            continue
        Index = 0
        Count = ExTokenCNameList.count(TokenCName)
        for Pcd in PcdExList:
            RealTokenCName = Pcd.TokenCName
            for PcdItem in GlobalData.MixedPcd:
                if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                    RealTokenCName = PcdItem[0]
                    break
            if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET and Pcd.TokenCName == TokenCName:
                Index = Index + 1
                if Index == 1:
                    AutoGenH.Append('\n#define __PCD_%s_VAL_CMP(GuidPtr)  (' % (RealTokenCName))
                    AutoGenH.Append('\\\n  (GuidPtr == NULL) ? 0:')
                    AutoGenH.Append('\\\n  COMPAREGUID (GuidPtr, &%s) ? _PCD_TOKEN_%s_%s:'
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, RealTokenCName))
                else:
                    AutoGenH.Append('\\\n  COMPAREGUID (GuidPtr, &%s) ? _PCD_TOKEN_%s_%s:'
                                    % (Pcd.TokenSpaceGuidCName, Pcd.TokenSpaceGuidCName, RealTokenCName))
                if Index == Count:
                    AutoGenH.Append('0 \\\n  )\n')
                    # Autogen internal worker macro to compare GUIDs.  Guid1 is a pointer to a GUID.
                    # Guid2 is a C name for a GUID. Compare pointers first because optimizing compiler
                    # can do this at build time on CONST GUID pointers and optimize away call to COMPAREGUID().
                    #  COMPAREGUID() will only be used if the Guid passed in is local to the module.
                    AutoGenH.Append('#define _PCD_TOKEN_EX_%s(GuidPtr)   __PCD_%s_ADDR_CMP(GuidPtr) ? __PCD_%s_ADDR_CMP(GuidPtr) : __PCD_%s_VAL_CMP(GuidPtr)  \n'
                                    % (RealTokenCName, RealTokenCName, RealTokenCName, RealTokenCName))
                TokenCNameList.add(TokenCName)

## Create code for module PCDs
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      Pcd         The PCD object
#
def CreateModulePcdCode(Info, AutoGenC, AutoGenH, Pcd):
    TokenSpaceGuidValue = Pcd.TokenSpaceGuidValue   #Info.GuidList[Pcd.TokenSpaceGuidCName]
    PcdTokenNumber = Info.PlatformInfo.PcdTokenNumber
    #
    # Write PCDs
    #
    TokenCName = Pcd.TokenCName
    for PcdItem in GlobalData.MixedPcd:
        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
            TokenCName = PcdItem[0]
            break
    PcdTokenName = '_PCD_TOKEN_' + TokenCName
    PatchPcdSizeTokenName = '_PCD_PATCHABLE_' + TokenCName +'_SIZE'
    PatchPcdSizeVariableName = '_gPcd_BinaryPatch_Size_' + TokenCName
    PatchPcdMaxSizeVariable = '_gPcd_BinaryPatch_MaxSize_' + TokenCName
    FixPcdSizeTokenName = '_PCD_SIZE_' + TokenCName
    FixedPcdSizeVariableName = '_gPcd_FixedAtBuild_Size_' + TokenCName

    if Pcd.PcdValueFromComm:
        Pcd.DefaultValue = Pcd.PcdValueFromComm
    elif Pcd.PcdValueFromFdf:
        Pcd.DefaultValue = Pcd.PcdValueFromFdf

    if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
        TokenNumber = int(Pcd.TokenValue, 0)
        # Add TokenSpaceGuidValue value to PcdTokenName to discriminate the DynamicEx PCDs with
        # different Guids but same TokenCName
        PcdExTokenName = '_PCD_TOKEN_' + Pcd.TokenSpaceGuidCName + '_' + TokenCName
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdExTokenName, TokenNumber))
    else:
        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) not in PcdTokenNumber:
            # If one of the Source built modules listed in the DSC is not listed in FDF modules,
            # and the INF lists a PCD can only use the PcdsDynamic access method (it is only
            # listed in the DEC file that declares the PCD as PcdsDynamic), then build tool will
            # report warning message notify the PI that they are attempting to build a module
            # that must be included in a flash image in order to be functional. These Dynamic PCD
            # will not be added into the Database unless it is used by other modules that are
            # included in the FDF file.
            # In this case, just assign an invalid token number to make it pass build.
            if Pcd.Type in PCD_DYNAMIC_TYPE_SET:
                TokenNumber = 0
            else:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "No generated token number for %s.%s\n" % (Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
        else:
            TokenNumber = PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName]
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdTokenName, TokenNumber))

    EdkLogger.debug(EdkLogger.DEBUG_3, "Creating code for " + TokenCName + "." + Pcd.TokenSpaceGuidCName)
    if Pcd.Type not in gItemTypeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown PCD type [%s] of PCD %s.%s" % (Pcd.Type, Pcd.TokenSpaceGuidCName, TokenCName),
                        ExtraData="[%s]" % str(Info))

    DatumSize = gDatumSizeStringDatabase[Pcd.DatumType] if Pcd.DatumType in gDatumSizeStringDatabase else gDatumSizeStringDatabase[TAB_VOID]
    DatumSizeLib = gDatumSizeStringDatabaseLib[Pcd.DatumType] if Pcd.DatumType in gDatumSizeStringDatabaseLib else gDatumSizeStringDatabaseLib[TAB_VOID]
    GetModeName = '_PCD_GET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_GET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_' + TokenCName
    SetModeName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_' + TokenCName
    SetModeStatusName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_S_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_S_' + TokenCName
    GetModeSizeName = '_PCD_GET_MODE_SIZE' + '_' + TokenCName

    if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
        if Info.IsLibrary:
            PcdList = Info.LibraryPcdList
        else:
            PcdList = Info.ModulePcdList + Info.LibraryPcdList
        PcdExCNameTest = 0
        for PcdModule in PcdList:
            if PcdModule.Type in PCD_DYNAMIC_EX_TYPE_SET and Pcd.TokenCName == PcdModule.TokenCName:
                PcdExCNameTest += 1
            # get out early once we found > 1...
            if PcdExCNameTest > 1:
                break
        # Be compatible with the current code which using PcdToken and PcdGet/Set for DynamicEx Pcd.
        # If only PcdToken and PcdGet/Set used in all Pcds with different CName, it should succeed to build.
        # If PcdToken and PcdGet/Set used in the Pcds with different Guids but same CName, it should failed to build.
        if PcdExCNameTest > 1:
            AutoGenH.Append('// Disabled the macros, as PcdToken and PcdGet/Set are not allowed in the case that more than one DynamicEx Pcds are different Guids but same CName.\n')
            AutoGenH.Append('// #define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetExSize(&%s, %s)\n' % (GetModeSizeName, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
        else:
            AutoGenH.Append('#define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('#define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            AutoGenH.Append('#define %s LibPcdGetExSize(&%s, %s)\n' % (GetModeSizeName, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
    elif Pcd.Type in PCD_DYNAMIC_TYPE_SET:
        PcdCNameTest = 0
        for PcdModule in Info.LibraryPcdList + Info.ModulePcdList:
            if PcdModule.Type in PCD_DYNAMIC_TYPE_SET and Pcd.TokenCName == PcdModule.TokenCName:
                PcdCNameTest += 1
            # get out early once we found > 1...
            if PcdCNameTest > 1:
                break
        if PcdCNameTest > 1:
            EdkLogger.error("build", AUTOGEN_ERROR, "More than one Dynamic Pcds [%s] are different Guids but same CName. They need to be changed to DynamicEx type to avoid the confliction.\n" % (TokenCName), ExtraData="[%s]" % str(Info.MetaFile.Path))
        else:
            AutoGenH.Append('#define %s  LibPcdGet%s(%s)\n' % (GetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s  LibPcdGetSize(%s)\n' % (GetModeSizeName, PcdTokenName))
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%s(%s, (SizeOfBuffer), (Buffer))\n' %(SetModeName, DatumSizeLib, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%sS(%s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSet%s(%s, (Value))\n' % (SetModeName, DatumSizeLib, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSet%sS(%s, (Value))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
    else:
        PcdVariableName = '_gPcd_' + gItemTypeStringDatabase[Pcd.Type] + '_' + TokenCName
        Const = 'const'
        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            Const = ''
        Type = ''
        Array = ''
        Value = Pcd.DefaultValue
        Unicode = False
        ValueNumber = 0

        if Pcd.DatumType == 'BOOLEAN':
            BoolValue = Value.upper()
            if BoolValue == 'TRUE' or BoolValue == '1':
                Value = '1U'
            elif BoolValue == 'FALSE' or BoolValue == '0':
                Value = '0U'

        if Pcd.DatumType in TAB_PCD_CLEAN_NUMERIC_TYPES:
            try:
                if Value.upper().endswith('L'):
                    Value = Value[:-1]
                if Value.startswith('0') and not Value.lower().startswith('0x') and len(Value) > 1 and Value.lstrip('0'):
                    Value = Value.lstrip('0')
                ValueNumber = int (Value, 0)
            except:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "PCD value is not valid dec or hex number for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
            if ValueNumber < 0:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "PCD can't be set to negative value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
            elif ValueNumber > MAX_VAL_TYPE[Pcd.DatumType]:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "Too large PCD value for datum type [%s] of PCD %s.%s" % (Pcd.DatumType, Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
            if Pcd.DatumType == TAB_UINT64 and not Value.endswith('ULL'):
                Value += 'ULL'
            elif Pcd.DatumType != TAB_UINT64 and not Value.endswith('U'):
                Value += 'U'

        if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
            if not Pcd.MaxDatumSize:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "Unknown [MaxDatumSize] of PCD [%s.%s]" % (Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))

            ArraySize = int(Pcd.MaxDatumSize, 0)
            if Value[0] == '{':
                Type = '(VOID *)'
                ValueSize = len(Value.split(','))
            else:
                if Value[0] == 'L':
                    Unicode = True
                Value = Value.lstrip('L')   #.strip('"')
                Value = eval(Value)         # translate escape character
                ValueSize = len(Value) + 1
                NewValue = '{'
                for Index in range(0, len(Value)):
                    if Unicode:
                        NewValue = NewValue + str(ord(Value[Index]) % 0x10000) + ', '
                    else:
                        NewValue = NewValue + str(ord(Value[Index]) % 0x100) + ', '
                if Unicode:
                    ArraySize = ArraySize // 2
                Value = NewValue + '0 }'
            if ArraySize < ValueSize:
                if Pcd.MaxSizeUserSet:
                    EdkLogger.error("build", AUTOGEN_ERROR,
                                "The maximum size of VOID* type PCD '%s.%s' is less than its actual size occupied." % (Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
                else:
                    ArraySize = Pcd.GetPcdSize()
                    if Unicode:
                        ArraySize = ArraySize // 2
            Array = '[%d]' % ArraySize
        #
        # skip casting for fixed at build since it breaks ARM assembly.
        # Long term we need PCD macros that work in assembly
        #
        elif Pcd.Type != TAB_PCDS_FIXED_AT_BUILD and Pcd.DatumType in TAB_PCD_NUMERIC_TYPES_VOID:
            Value = "((%s)%s)" % (Pcd.DatumType, Value)

        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            PcdValueName = '_PCD_PATCHABLE_VALUE_' + TokenCName
        else:
            PcdValueName = '_PCD_VALUE_' + TokenCName

        if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
            #
            # For unicode, UINT16 array will be generated, so the alignment of unicode is guaranteed.
            #
            AutoGenH.Append('#define %s  %s%s\n' %(PcdValueName, Type, PcdVariableName))
            if Unicode:
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s UINT16 %s%s = %s;\n' % (Const, PcdVariableName, Array, Value))
                AutoGenH.Append('extern %s UINT16 %s%s;\n' %(Const, PcdVariableName, Array))
            else:
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s UINT8 %s%s = %s;\n' % (Const, PcdVariableName, Array, Value))
                AutoGenH.Append('extern %s UINT8 %s%s;\n' %(Const, PcdVariableName, Array))
            AutoGenH.Append('#define %s  %s%s\n' %(GetModeName, Type, PcdVariableName))

            PcdDataSize = Pcd.GetPcdSize()
            if Pcd.Type == TAB_PCDS_FIXED_AT_BUILD:
                AutoGenH.Append('#define %s %s\n' % (FixPcdSizeTokenName, PcdDataSize))
                AutoGenH.Append('#define %s  %s \n' % (GetModeSizeName, FixPcdSizeTokenName))
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED const UINTN %s = %s;\n' % (FixedPcdSizeVariableName, PcdDataSize))
            if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
                AutoGenH.Append('#define %s %s\n' % (PatchPcdSizeTokenName, Pcd.MaxDatumSize))
                AutoGenH.Append('#define %s  %s \n' % (GetModeSizeName, PatchPcdSizeVariableName))
                AutoGenH.Append('extern UINTN %s; \n' % PatchPcdSizeVariableName)
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED UINTN %s = %s;\n' % (PatchPcdSizeVariableName, PcdDataSize))
                AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED const UINTN %s = %s;\n' % (PatchPcdMaxSizeVariable, Pcd.MaxDatumSize))
        elif Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            AutoGenH.Append('#define %s  %s\n' %(PcdValueName, Value))
            AutoGenC.Append('volatile %s %s %s = %s;\n' %(Const, Pcd.DatumType, PcdVariableName, PcdValueName))
            AutoGenH.Append('extern volatile %s  %s  %s%s;\n' % (Const, Pcd.DatumType, PcdVariableName, Array))
            AutoGenH.Append('#define %s  %s%s\n' % (GetModeName, Type, PcdVariableName))

            PcdDataSize = Pcd.GetPcdSize()
            AutoGenH.Append('#define %s %s\n' % (PatchPcdSizeTokenName, PcdDataSize))

            AutoGenH.Append('#define %s  %s \n' % (GetModeSizeName, PatchPcdSizeVariableName))
            AutoGenH.Append('extern UINTN %s; \n' % PatchPcdSizeVariableName)
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED UINTN %s = %s;\n' % (PatchPcdSizeVariableName, PcdDataSize))
        else:
            PcdDataSize = Pcd.GetPcdSize()
            AutoGenH.Append('#define %s %s\n' % (FixPcdSizeTokenName, PcdDataSize))
            AutoGenH.Append('#define %s  %s \n' % (GetModeSizeName, FixPcdSizeTokenName))

            AutoGenH.Append('#define %s  %s\n' %(PcdValueName, Value))
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s %s = %s;\n' %(Const, Pcd.DatumType, PcdVariableName, PcdValueName))
            AutoGenH.Append('extern %s  %s  %s%s;\n' % (Const, Pcd.DatumType, PcdVariableName, Array))
            AutoGenH.Append('#define %s  %s%s\n' % (GetModeName, Type, PcdVariableName))

        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE:
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtrAndSize((VOID *)_gPcd_BinaryPatch_%s, &_gPcd_BinaryPatch_Size_%s, (UINTN)_PCD_PATCHABLE_%s_SIZE, (SizeOfBuffer), (Buffer))\n' % (SetModeName, Pcd.TokenCName, Pcd.TokenCName, Pcd.TokenCName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtrAndSizeS((VOID *)_gPcd_BinaryPatch_%s, &_gPcd_BinaryPatch_Size_%s, (UINTN)_PCD_PATCHABLE_%s_SIZE, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, Pcd.TokenCName, Pcd.TokenCName, Pcd.TokenCName))
            else:
                AutoGenH.Append('#define %s(Value)  (%s = (Value))\n' % (SetModeName, PcdVariableName))
                AutoGenH.Append('#define %s(Value)  ((%s = (Value)), RETURN_SUCCESS) \n' % (SetModeStatusName, PcdVariableName))
        else:
            AutoGenH.Append('//#define %s  ASSERT(FALSE)  // It is not allowed to set value for a FIXED_AT_BUILD PCD\n' % SetModeName)

## Create code for library module PCDs
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      Pcd         The PCD object
#
def CreateLibraryPcdCode(Info, AutoGenC, AutoGenH, Pcd):
    PcdTokenNumber = Info.PlatformInfo.PcdTokenNumber
    TokenSpaceGuidCName = Pcd.TokenSpaceGuidCName
    TokenCName  = Pcd.TokenCName
    for PcdItem in GlobalData.MixedPcd:
        if (TokenCName, TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
            TokenCName = PcdItem[0]
            break
    PcdTokenName = '_PCD_TOKEN_' + TokenCName
    FixPcdSizeTokenName = '_PCD_SIZE_' + TokenCName
    PatchPcdSizeTokenName = '_PCD_PATCHABLE_' + TokenCName +'_SIZE'
    PatchPcdSizeVariableName = '_gPcd_BinaryPatch_Size_' + TokenCName
    PatchPcdMaxSizeVariable = '_gPcd_BinaryPatch_MaxSize_' + TokenCName
    FixedPcdSizeVariableName = '_gPcd_FixedAtBuild_Size_' + TokenCName

    if Pcd.PcdValueFromComm:
        Pcd.DefaultValue = Pcd.PcdValueFromComm
    elif Pcd.PcdValueFromFdf:
        Pcd.DefaultValue = Pcd.PcdValueFromFdf
    #
    # Write PCDs
    #
    if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET:
        TokenNumber = int(Pcd.TokenValue, 0)
    else:
        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) not in PcdTokenNumber:
            # If one of the Source built modules listed in the DSC is not listed in FDF modules,
            # and the INF lists a PCD can only use the PcdsDynamic access method (it is only
            # listed in the DEC file that declares the PCD as PcdsDynamic), then build tool will
            # report warning message notify the PI that they are attempting to build a module
            # that must be included in a flash image in order to be functional. These Dynamic PCD
            # will not be added into the Database unless it is used by other modules that are
            # included in the FDF file.
            # In this case, just assign an invalid token number to make it pass build.
            if Pcd.Type in PCD_DYNAMIC_TYPE_SET:
                TokenNumber = 0
            else:
                EdkLogger.error("build", AUTOGEN_ERROR,
                                "No generated token number for %s.%s\n" % (Pcd.TokenSpaceGuidCName, TokenCName),
                                ExtraData="[%s]" % str(Info))
        else:
            TokenNumber = PcdTokenNumber[Pcd.TokenCName, Pcd.TokenSpaceGuidCName]

    if Pcd.Type not in gItemTypeStringDatabase:
        EdkLogger.error("build", AUTOGEN_ERROR,
                        "Unknown PCD type [%s] of PCD %s.%s" % (Pcd.Type, Pcd.TokenSpaceGuidCName, TokenCName),
                        ExtraData="[%s]" % str(Info))

    DatumType   = Pcd.DatumType
    DatumSize = gDatumSizeStringDatabase[Pcd.DatumType] if Pcd.DatumType in gDatumSizeStringDatabase else gDatumSizeStringDatabase[TAB_VOID]
    DatumSizeLib = gDatumSizeStringDatabaseLib[Pcd.DatumType] if Pcd.DatumType in gDatumSizeStringDatabaseLib else gDatumSizeStringDatabaseLib[TAB_VOID]
    GetModeName = '_PCD_GET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_GET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_' + TokenCName
    SetModeName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_' + TokenCName
    SetModeStatusName = '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[Pcd.DatumType] + '_S_' + TokenCName if Pcd.DatumType in gDatumSizeStringDatabaseH else '_PCD_SET_MODE_' + gDatumSizeStringDatabaseH[TAB_VOID] + '_S_' + TokenCName
    GetModeSizeName = '_PCD_GET_MODE_SIZE' + '_' + TokenCName

    Type = ''
    Array = ''
    if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
        if Pcd.DefaultValue[0]== '{':
            Type = '(VOID *)'
        Array = '[]'
    PcdItemType = Pcd.Type
    if PcdItemType in PCD_DYNAMIC_EX_TYPE_SET:
        PcdExTokenName = '_PCD_TOKEN_' + TokenSpaceGuidCName + '_' + TokenCName
        AutoGenH.Append('\n#define %s  %dU\n' % (PcdExTokenName, TokenNumber))

        if Info.IsLibrary:
            PcdList = Info.LibraryPcdList
        else:
            PcdList = Info.ModulePcdList
        PcdExCNameTest = 0
        for PcdModule in PcdList:
            if PcdModule.Type in PCD_DYNAMIC_EX_TYPE_SET and Pcd.TokenCName == PcdModule.TokenCName:
                PcdExCNameTest += 1
            # get out early once we found > 1...
            if PcdExCNameTest > 1:
                break
        # Be compatible with the current code which using PcdGet/Set for DynamicEx Pcd.
        # If only PcdGet/Set used in all Pcds with different CName, it should succeed to build.
        # If PcdGet/Set used in the Pcds with different Guids but same CName, it should failed to build.
        if PcdExCNameTest > 1:
            AutoGenH.Append('// Disabled the macros, as PcdToken and PcdGet/Set are not allowed in the case that more than one DynamicEx Pcds are different Guids but same CName.\n')
            AutoGenH.Append('// #define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            AutoGenH.Append('// #define %s  LibPcdGetExSize(&%s, %s)\n' % (GetModeSizeName, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('// #define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
        else:
            AutoGenH.Append('#define %s  %s\n' % (PcdTokenName, PcdExTokenName))
            AutoGenH.Append('#define %s  LibPcdGetEx%s(&%s, %s)\n' % (GetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            AutoGenH.Append('#define %s LibPcdGetExSize(&%s, %s)\n' % (GetModeSizeName, Pcd.TokenSpaceGuidCName, PcdTokenName))
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%s(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSetEx%sS(&%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%s(&%s, %s, (Value))\n' % (SetModeName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSetEx%sS(&%s, %s, (Value))\n' % (SetModeStatusName, DatumSizeLib, Pcd.TokenSpaceGuidCName, PcdTokenName))
    else:
        AutoGenH.Append('#define _PCD_TOKEN_%s  %dU\n' % (TokenCName, TokenNumber))
    if PcdItemType in PCD_DYNAMIC_TYPE_SET:
        PcdList = []
        PcdCNameList = []
        PcdList.extend(Info.LibraryPcdList)
        PcdList.extend(Info.ModulePcdList)
        for PcdModule in PcdList:
            if PcdModule.Type in PCD_DYNAMIC_TYPE_SET:
                PcdCNameList.append(PcdModule.TokenCName)
        if PcdCNameList.count(Pcd.TokenCName) > 1:
            EdkLogger.error("build", AUTOGEN_ERROR, "More than one Dynamic Pcds [%s] are different Guids but same CName.They need to be changed to DynamicEx type to avoid the confliction.\n" % (TokenCName), ExtraData="[%s]" % str(Info.MetaFile.Path))
        else:
            AutoGenH.Append('#define %s  LibPcdGet%s(%s)\n' % (GetModeName, DatumSizeLib, PcdTokenName))
            AutoGenH.Append('#define %s  LibPcdGetSize(%s)\n' % (GetModeSizeName, PcdTokenName))
            if DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%s(%s, (SizeOfBuffer), (Buffer))\n' %(SetModeName, DatumSizeLib, PcdTokenName))
                AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPcdSet%sS(%s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
            else:
                AutoGenH.Append('#define %s(Value)  LibPcdSet%s(%s, (Value))\n' % (SetModeName, DatumSizeLib, PcdTokenName))
                AutoGenH.Append('#define %s(Value)  LibPcdSet%sS(%s, (Value))\n' % (SetModeStatusName, DatumSizeLib, PcdTokenName))
    if PcdItemType == TAB_PCDS_PATCHABLE_IN_MODULE:
        PcdVariableName = '_gPcd_' + gItemTypeStringDatabase[TAB_PCDS_PATCHABLE_IN_MODULE] + '_' + TokenCName
        if DatumType not in TAB_PCD_NUMERIC_TYPES:
            if DatumType == TAB_VOID and Array == '[]':
                DatumType = [TAB_UINT8, TAB_UINT16][Pcd.DefaultValue[0] == 'L']
            else:
                DatumType = TAB_UINT8
            AutoGenH.Append('extern %s _gPcd_BinaryPatch_%s%s;\n' %(DatumType, TokenCName, Array))
        else:
            AutoGenH.Append('extern volatile  %s  %s%s;\n' % (DatumType, PcdVariableName, Array))
        AutoGenH.Append('#define %s  %s_gPcd_BinaryPatch_%s\n' %(GetModeName, Type, TokenCName))
        PcdDataSize = Pcd.GetPcdSize()
        if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtrAndSize((VOID *)_gPcd_BinaryPatch_%s, &%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeName, TokenCName, PatchPcdSizeVariableName, PatchPcdMaxSizeVariable))
            AutoGenH.Append('#define %s(SizeOfBuffer, Buffer)  LibPatchPcdSetPtrAndSizeS((VOID *)_gPcd_BinaryPatch_%s, &%s, %s, (SizeOfBuffer), (Buffer))\n' % (SetModeStatusName, TokenCName, PatchPcdSizeVariableName, PatchPcdMaxSizeVariable))
            AutoGenH.Append('#define %s %s\n' % (PatchPcdSizeTokenName, PatchPcdMaxSizeVariable))
            AutoGenH.Append('extern const UINTN %s; \n' % PatchPcdMaxSizeVariable)
        else:
            AutoGenH.Append('#define %s(Value)  (%s = (Value))\n' % (SetModeName, PcdVariableName))
            AutoGenH.Append('#define %s(Value)  ((%s = (Value)), RETURN_SUCCESS)\n' % (SetModeStatusName, PcdVariableName))
            AutoGenH.Append('#define %s %s\n' % (PatchPcdSizeTokenName, PcdDataSize))

        AutoGenH.Append('#define %s %s\n' % (GetModeSizeName, PatchPcdSizeVariableName))
        AutoGenH.Append('extern UINTN %s; \n' % PatchPcdSizeVariableName)

    if PcdItemType == TAB_PCDS_FIXED_AT_BUILD or PcdItemType == TAB_PCDS_FEATURE_FLAG:
        key = ".".join((Pcd.TokenSpaceGuidCName, Pcd.TokenCName))
        PcdVariableName = '_gPcd_' + gItemTypeStringDatabase[Pcd.Type] + '_' + TokenCName
        if DatumType == TAB_VOID and Array == '[]':
            DatumType = [TAB_UINT8, TAB_UINT16][Pcd.DefaultValue[0] == 'L']
        if DatumType not in TAB_PCD_NUMERIC_TYPES_VOID:
            DatumType = TAB_UINT8
        AutoGenH.Append('extern const %s _gPcd_FixedAtBuild_%s%s;\n' %(DatumType, TokenCName, Array))
        AutoGenH.Append('#define %s  %s_gPcd_FixedAtBuild_%s\n' %(GetModeName, Type, TokenCName))
        AutoGenH.Append('//#define %s  ASSERT(FALSE)  // It is not allowed to set value for a FIXED_AT_BUILD PCD\n' % SetModeName)

        ConstFixedPcd = False
        if PcdItemType == TAB_PCDS_FIXED_AT_BUILD and (key in Info.ConstPcd or (Info.IsLibrary and not Info.ReferenceModules)):
            ConstFixedPcd = True
            if key in Info.ConstPcd:
                Pcd.DefaultValue = Info.ConstPcd[key]
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                AutoGenH.Append('#define _PCD_VALUE_%s %s%s\n' %(TokenCName, Type, PcdVariableName))
            else:
                AutoGenH.Append('#define _PCD_VALUE_%s %s\n' %(TokenCName, Pcd.DefaultValue))
        PcdDataSize = Pcd.GetPcdSize()
        if PcdItemType == TAB_PCDS_FIXED_AT_BUILD:
            if Pcd.DatumType not in TAB_PCD_NUMERIC_TYPES:
                if ConstFixedPcd:
                    AutoGenH.Append('#define %s %s\n' % (FixPcdSizeTokenName, PcdDataSize))
                    AutoGenH.Append('#define %s %s\n' % (GetModeSizeName, FixPcdSizeTokenName))
                else:
                    AutoGenH.Append('#define %s %s\n' % (GetModeSizeName, FixedPcdSizeVariableName))
                    AutoGenH.Append('#define %s %s\n' % (FixPcdSizeTokenName, FixedPcdSizeVariableName))
                    AutoGenH.Append('extern const UINTN %s; \n' % FixedPcdSizeVariableName)
            else:
                AutoGenH.Append('#define %s %s\n' % (FixPcdSizeTokenName, PcdDataSize))
                AutoGenH.Append('#define %s %s\n' % (GetModeSizeName, FixPcdSizeTokenName))

## Create code for library constructor
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateLibraryConstructorCode(Info, AutoGenC, AutoGenH):
    #
    # Library Constructors
    #
    ConstructorPrototypeString = TemplateString()
    ConstructorCallingString = TemplateString()
    if Info.IsLibrary:
        DependentLibraryList = [Info.Module]
    else:
        DependentLibraryList = Info.DependentLibraryList
    for Lib in DependentLibraryList:
        if len(Lib.ConstructorList) <= 0:
            continue
        Dict = {'Function':Lib.ConstructorList}
        if Lib.ModuleType in [SUP_MODULE_BASE, SUP_MODULE_SEC]:
            ConstructorPrototypeString.Append(gLibraryStructorPrototype[SUP_MODULE_BASE].Replace(Dict))
            ConstructorCallingString.Append(gLibraryStructorCall[SUP_MODULE_BASE].Replace(Dict))
        if Info.ModuleType not in [SUP_MODULE_BASE, SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION]:
            if Lib.ModuleType in SUP_MODULE_SET_PEI:
                ConstructorPrototypeString.Append(gLibraryStructorPrototype['PEI'].Replace(Dict))
                ConstructorCallingString.Append(gLibraryStructorCall['PEI'].Replace(Dict))
            elif Lib.ModuleType in [SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER,
                                    SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER, SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_SMM_CORE]:
                ConstructorPrototypeString.Append(gLibraryStructorPrototype['DXE'].Replace(Dict))
                ConstructorCallingString.Append(gLibraryStructorCall['DXE'].Replace(Dict))
            elif Lib.ModuleType in [SUP_MODULE_MM_STANDALONE, SUP_MODULE_MM_CORE_STANDALONE]:
                ConstructorPrototypeString.Append(gLibraryStructorPrototype['MM'].Replace(Dict))
                ConstructorCallingString.Append(gLibraryStructorCall['MM'].Replace(Dict))

    if str(ConstructorPrototypeString) == '':
        ConstructorPrototypeList = []
    else:
        ConstructorPrototypeList = [str(ConstructorPrototypeString)]
    if str(ConstructorCallingString) == '':
        ConstructorCallingList = []
    else:
        ConstructorCallingList = [str(ConstructorCallingString)]

    Dict = {
        'Type'              :   'Constructor',
        'FunctionPrototype' :   ConstructorPrototypeList,
        'FunctionCall'      :   ConstructorCallingList
    }
    if Info.IsLibrary:
        AutoGenH.Append("${BEGIN}${FunctionPrototype}${END}", Dict)
    else:
        if Info.ModuleType in [SUP_MODULE_BASE, SUP_MODULE_SEC, SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION]:
            AutoGenC.Append(gLibraryString[SUP_MODULE_BASE].Replace(Dict))
            if Info.ModuleType == SUP_MODULE_SEC and Info.AutoGenVersion >= 0x0001001E:
                AutoGenH.Append(("\n"
                                 "// ProcessLibraryConstructorList() declared here because SEC has no standard entry point.\n"
                                 "VOID\n"
                                 "EFIAPI\n"
                                 "ProcessLibraryConstructorList (\n"
                                 "  VOID\n"
                                 "  );\n"))
        elif Info.ModuleType in SUP_MODULE_SET_PEI:
            AutoGenC.Append(gLibraryString['PEI'].Replace(Dict))
        elif Info.ModuleType in [SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER,
                                 SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER, SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_SMM_CORE]:
            AutoGenC.Append(gLibraryString['DXE'].Replace(Dict))
        elif Info.ModuleType in [SUP_MODULE_MM_STANDALONE, SUP_MODULE_MM_CORE_STANDALONE]:
            AutoGenC.Append(gLibraryString['MM'].Replace(Dict))

## Create code for library destructor
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateLibraryDestructorCode(Info, AutoGenC, AutoGenH):
    #
    # Library Destructors
    #
    DestructorPrototypeString = TemplateString()
    DestructorCallingString = TemplateString()
    if Info.IsLibrary:
        DependentLibraryList = [Info.Module]
    else:
        DependentLibraryList = Info.DependentLibraryList
    for Index in range(len(DependentLibraryList)-1, -1, -1):
        Lib = DependentLibraryList[Index]
        if len(Lib.DestructorList) <= 0:
            continue
        Dict = {'Function':Lib.DestructorList}
        if Lib.ModuleType in [SUP_MODULE_BASE, SUP_MODULE_SEC]:
            DestructorPrototypeString.Append(gLibraryStructorPrototype[SUP_MODULE_BASE].Replace(Dict))
            DestructorCallingString.Append(gLibraryStructorCall[SUP_MODULE_BASE].Replace(Dict))
        if Info.ModuleType not in [SUP_MODULE_BASE, SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION]:
            if Lib.ModuleType in SUP_MODULE_SET_PEI:
                DestructorPrototypeString.Append(gLibraryStructorPrototype['PEI'].Replace(Dict))
                DestructorCallingString.Append(gLibraryStructorCall['PEI'].Replace(Dict))
            elif Lib.ModuleType in [SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER,
                                    SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER, SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_SMM_CORE]:
                DestructorPrototypeString.Append(gLibraryStructorPrototype['DXE'].Replace(Dict))
                DestructorCallingString.Append(gLibraryStructorCall['DXE'].Replace(Dict))
            elif Lib.ModuleType in [SUP_MODULE_MM_STANDALONE, SUP_MODULE_MM_CORE_STANDALONE]:
                DestructorPrototypeString.Append(gLibraryStructorPrototype['MM'].Replace(Dict))
                DestructorCallingString.Append(gLibraryStructorCall['MM'].Replace(Dict))

    if str(DestructorPrototypeString) == '':
        DestructorPrototypeList = []
    else:
        DestructorPrototypeList = [str(DestructorPrototypeString)]
    if str(DestructorCallingString) == '':
        DestructorCallingList = []
    else:
        DestructorCallingList = [str(DestructorCallingString)]

    Dict = {
        'Type'              :   'Destructor',
        'FunctionPrototype' :   DestructorPrototypeList,
        'FunctionCall'      :   DestructorCallingList
    }
    if Info.IsLibrary:
        AutoGenH.Append("${BEGIN}${FunctionPrototype}${END}", Dict)
    else:
        if Info.ModuleType in [SUP_MODULE_BASE, SUP_MODULE_SEC, SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION]:
            AutoGenC.Append(gLibraryString[SUP_MODULE_BASE].Replace(Dict))
        elif Info.ModuleType in SUP_MODULE_SET_PEI:
            AutoGenC.Append(gLibraryString['PEI'].Replace(Dict))
        elif Info.ModuleType in [SUP_MODULE_DXE_CORE, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_SMM_DRIVER, SUP_MODULE_DXE_RUNTIME_DRIVER,
                                 SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER, SUP_MODULE_UEFI_APPLICATION, SUP_MODULE_SMM_CORE]:
            AutoGenC.Append(gLibraryString['DXE'].Replace(Dict))
        elif Info.ModuleType in [SUP_MODULE_MM_STANDALONE, SUP_MODULE_MM_CORE_STANDALONE]:
            AutoGenC.Append(gLibraryString['MM'].Replace(Dict))


## Create code for ModuleEntryPoint
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateModuleEntryPointCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary or Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_SEC]:
        return
    #
    # Module Entry Points
    #
    NumEntryPoints = len(Info.Module.ModuleEntryPointList)
    if 'PI_SPECIFICATION_VERSION' in Info.Module.Specification:
        PiSpecVersion = Info.Module.Specification['PI_SPECIFICATION_VERSION']
    else:
        PiSpecVersion = '0x00000000'
    if 'UEFI_SPECIFICATION_VERSION' in Info.Module.Specification:
        UefiSpecVersion = Info.Module.Specification['UEFI_SPECIFICATION_VERSION']
    else:
        UefiSpecVersion = '0x00000000'
    Dict = {
        'Function'       :   Info.Module.ModuleEntryPointList,
        'PiSpecVersion'  :   PiSpecVersion + 'U',
        'UefiSpecVersion':   UefiSpecVersion + 'U'
    }

    if Info.ModuleType in [SUP_MODULE_PEI_CORE, SUP_MODULE_DXE_CORE, SUP_MODULE_SMM_CORE, SUP_MODULE_MM_CORE_STANDALONE]:
        if Info.SourceFileList:
            if NumEntryPoints != 1:
                EdkLogger.error(
                  "build",
                  AUTOGEN_ERROR,
                  '%s must have exactly one entry point' % Info.ModuleType,
                  File=str(Info),
                  ExtraData= ", ".join(Info.Module.ModuleEntryPointList)
                  )
    if Info.ModuleType == SUP_MODULE_PEI_CORE:
        AutoGenC.Append(gPeiCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gPeiCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_DXE_CORE:
        AutoGenC.Append(gDxeCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gDxeCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_SMM_CORE:
        AutoGenC.Append(gSmmCoreEntryPointString.Replace(Dict))
        AutoGenH.Append(gSmmCoreEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_MM_CORE_STANDALONE:
        AutoGenC.Append(gMmCoreStandaloneEntryPointString.Replace(Dict))
        AutoGenH.Append(gMmCoreStandaloneEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_PEIM:
        if NumEntryPoints < 2:
            AutoGenC.Append(gPeimEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gPeimEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gPeimEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType in [SUP_MODULE_DXE_RUNTIME_DRIVER, SUP_MODULE_DXE_DRIVER, SUP_MODULE_DXE_SAL_DRIVER, SUP_MODULE_UEFI_DRIVER]:
        if NumEntryPoints < 2:
            AutoGenC.Append(gUefiDriverEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gUefiDriverEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gUefiDriverEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_DXE_SMM_DRIVER:
        if NumEntryPoints == 0:
            AutoGenC.Append(gDxeSmmEntryPointString[0].Replace(Dict))
        else:
            AutoGenC.Append(gDxeSmmEntryPointString[1].Replace(Dict))
        AutoGenH.Append(gDxeSmmEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_MM_STANDALONE:
        if NumEntryPoints < 2:
            AutoGenC.Append(gMmStandaloneEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gMmStandaloneEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gMmStandaloneEntryPointPrototype.Replace(Dict))
    elif Info.ModuleType == SUP_MODULE_UEFI_APPLICATION:
        if NumEntryPoints < 2:
            AutoGenC.Append(gUefiApplicationEntryPointString[NumEntryPoints].Replace(Dict))
        else:
            AutoGenC.Append(gUefiApplicationEntryPointString[2].Replace(Dict))
        AutoGenH.Append(gUefiApplicationEntryPointPrototype.Replace(Dict))

## Create code for ModuleUnloadImage
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateModuleUnloadImageCode(Info, AutoGenC, AutoGenH):
    if Info.IsLibrary or Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_BASE, SUP_MODULE_SEC]:
        return
    #
    # Unload Image Handlers
    #
    NumUnloadImage = len(Info.Module.ModuleUnloadImageList)
    Dict = {'Count':str(NumUnloadImage) + 'U', 'Function':Info.Module.ModuleUnloadImageList}
    if NumUnloadImage < 2:
        AutoGenC.Append(gUefiUnloadImageString[NumUnloadImage].Replace(Dict))
    else:
        AutoGenC.Append(gUefiUnloadImageString[2].Replace(Dict))
    AutoGenH.Append(gUefiUnloadImagePrototype.Replace(Dict))

## Create code for GUID
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateGuidDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_BASE]:
        GuidType = TAB_GUID
    else:
        GuidType = "EFI_GUID"

    if Info.GuidList:
        if not Info.IsLibrary:
            AutoGenC.Append("\n// Guids\n")
        AutoGenH.Append("\n// Guids\n")
    #
    # GUIDs
    #
    for Key in Info.GuidList:
        if not Info.IsLibrary:
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.GuidList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for protocol
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateProtocolDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_BASE]:
        GuidType = TAB_GUID
    else:
        GuidType = "EFI_GUID"

    if Info.ProtocolList:
        if not Info.IsLibrary:
            AutoGenC.Append("\n// Protocols\n")
        AutoGenH.Append("\n// Protocols\n")
    #
    # Protocol GUIDs
    #
    for Key in Info.ProtocolList:
        if not Info.IsLibrary:
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.ProtocolList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for PPI
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreatePpiDefinitionCode(Info, AutoGenC, AutoGenH):
    if Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_BASE]:
        GuidType = TAB_GUID
    else:
        GuidType = "EFI_GUID"

    if Info.PpiList:
        if not Info.IsLibrary:
            AutoGenC.Append("\n// PPIs\n")
        AutoGenH.Append("\n// PPIs\n")
    #
    # PPI GUIDs
    #
    for Key in Info.PpiList:
        if not Info.IsLibrary:
            AutoGenC.Append('GLOBAL_REMOVE_IF_UNREFERENCED %s %s = %s;\n' % (GuidType, Key, Info.PpiList[Key]))
        AutoGenH.Append('extern %s %s;\n' % (GuidType, Key))

## Create code for PCD
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreatePcdCode(Info, AutoGenC, AutoGenH):

    # Collect Token Space GUIDs used by DynamicEc PCDs
    TokenSpaceList = []
    for Pcd in Info.ModulePcdList:
        if Pcd.Type in PCD_DYNAMIC_EX_TYPE_SET and Pcd.TokenSpaceGuidCName not in TokenSpaceList:
            TokenSpaceList.append(Pcd.TokenSpaceGuidCName)

    SkuMgr = Info.PlatformInfo.Platform.SkuIdMgr
    AutoGenH.Append("\n// Definition of SkuId Array\n")
    AutoGenH.Append("extern UINT64 _gPcd_SkuId_Array[];\n")
    # Add extern declarations to AutoGen.h if one or more Token Space GUIDs were found
    if TokenSpaceList:
        AutoGenH.Append("\n// Definition of PCD Token Space GUIDs used in this module\n\n")
        if Info.ModuleType in [SUP_MODULE_USER_DEFINED, SUP_MODULE_HOST_APPLICATION, SUP_MODULE_BASE]:
            GuidType = TAB_GUID
        else:
            GuidType = "EFI_GUID"
        for Item in TokenSpaceList:
            AutoGenH.Append('extern %s %s;\n' % (GuidType, Item))

    if Info.IsLibrary:
        if Info.ModulePcdList:
            AutoGenH.Append("\n// PCD definitions\n")
        for Pcd in Info.ModulePcdList:
            CreateLibraryPcdCode(Info, AutoGenC, AutoGenH, Pcd)
        DynExPcdTokenNumberMapping (Info, AutoGenH)
    else:
        AutoGenC.Append("\n// Definition of SkuId Array\n")
        AutoGenC.Append("GLOBAL_REMOVE_IF_UNREFERENCED UINT64 _gPcd_SkuId_Array[] = %s;\n" % SkuMgr.DumpSkuIdArrary())
        if Info.ModulePcdList:
            AutoGenH.Append("\n// Definition of PCDs used in this module\n")
            AutoGenC.Append("\n// Definition of PCDs used in this module\n")
        for Pcd in Info.ModulePcdList:
            CreateModulePcdCode(Info, AutoGenC, AutoGenH, Pcd)
        DynExPcdTokenNumberMapping (Info, AutoGenH)
        if Info.LibraryPcdList:
            AutoGenH.Append("\n// Definition of PCDs used in libraries is in AutoGen.c\n")
            AutoGenC.Append("\n// Definition of PCDs used in libraries\n")
        for Pcd in Info.LibraryPcdList:
            CreateModulePcdCode(Info, AutoGenC, AutoGenC, Pcd)
    CreatePcdDatabaseCode(Info, AutoGenC, AutoGenH)

## Create code for unicode string definition
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      UniGenCFlag     UniString is generated into AutoGen C file when it is set to True
#   @param      UniGenBinBuffer Buffer to store uni string package data
#
def CreateUnicodeStringCode(Info, AutoGenC, AutoGenH, UniGenCFlag, UniGenBinBuffer):
    WorkingDir = os.getcwd()
    os.chdir(Info.WorkspaceDir)

    IncList = [Info.MetaFile.Dir]
    # Get all files under [Sources] section in inf file for EDK-II module
    EDK2Module = True
    SrcList = [F for F in Info.SourceFileList]

    if 'BUILD' in Info.BuildOption and Info.BuildOption['BUILD']['FLAGS'].find('-c') > -1:
        CompatibleMode = True
    else:
        CompatibleMode = False

    #
    # -s is a temporary option dedicated for building .UNI files with ISO 639-2 language codes of EDK Shell in EDK2
    #
    if 'BUILD' in Info.BuildOption and Info.BuildOption['BUILD']['FLAGS'].find('-s') > -1:
        if CompatibleMode:
            EdkLogger.error("build", AUTOGEN_ERROR,
                            "-c and -s build options should be used exclusively",
                            ExtraData="[%s]" % str(Info))
        ShellMode = True
    else:
        ShellMode = False

    #RFC4646 is only for EDKII modules and ISO639-2 for EDK modules
    if EDK2Module:
        FilterInfo = [EDK2Module] + [Info.PlatformInfo.Platform.RFCLanguages]
    else:
        FilterInfo = [EDK2Module] + [Info.PlatformInfo.Platform.ISOLanguages]
    Header, Code = GetStringFiles(Info.UnicodeFileList, SrcList, IncList, Info.IncludePathList, ['.uni', '.inf'], Info.Name, CompatibleMode, ShellMode, UniGenCFlag, UniGenBinBuffer, FilterInfo)
    if CompatibleMode or UniGenCFlag:
        AutoGenC.Append("\n//\n//Unicode String Pack Definition\n//\n")
        AutoGenC.Append(Code)
        AutoGenC.Append("\n")
    AutoGenH.Append("\n//\n//Unicode String ID\n//\n")
    AutoGenH.Append(Header)
    if CompatibleMode or UniGenCFlag:
        AutoGenH.Append("\n#define STRING_ARRAY_NAME %sStrings\n" % Info.Name)
    os.chdir(WorkingDir)

def CreateIdfFileCode(Info, AutoGenC, StringH, IdfGenCFlag, IdfGenBinBuffer):
    if len(Info.IdfFileList) > 0:
        ImageFiles = IdfFileClassObject(sorted (Info.IdfFileList))
        if ImageFiles.ImageFilesDict:
            Index = 1
            PaletteIndex = 1
            IncList = [Info.MetaFile.Dir]
            SrcList = [F for F in Info.SourceFileList]
            SkipList = ['.jpg', '.png', '.bmp', '.inf', '.idf']
            FileList = GetFileList(SrcList, IncList, SkipList)
            ValueStartPtr = 60
            StringH.Append("\n//\n//Image ID\n//\n")
            ImageInfoOffset = 0
            PaletteInfoOffset = 0
            ImageBuffer = pack('x')
            PaletteBuffer = pack('x')
            BufferStr = ''
            PaletteStr = ''
            FileDict = {}
            for Idf in ImageFiles.ImageFilesDict:
                if ImageFiles.ImageFilesDict[Idf]:
                    for FileObj in ImageFiles.ImageFilesDict[Idf]:
                        for sourcefile in Info.SourceFileList:
                            if FileObj.FileName == sourcefile.File:
                                if not sourcefile.Ext.upper() in ['.PNG', '.BMP', '.JPG']:
                                    EdkLogger.error("build", AUTOGEN_ERROR, "The %s's postfix must be one of .bmp, .jpg, .png" % (FileObj.FileName), ExtraData="[%s]" % str(Info))
                                FileObj.File = sourcefile
                                break
                        else:
                            EdkLogger.error("build", AUTOGEN_ERROR, "The %s in %s is not defined in the driver's [Sources] section" % (FileObj.FileName, Idf), ExtraData="[%s]" % str(Info))

                    for FileObj in ImageFiles.ImageFilesDict[Idf]:
                        ID = FileObj.ImageID
                        File = FileObj.File
                        try:
                            SearchImageID (FileObj, FileList)
                            if FileObj.Referenced:
                                if (ValueStartPtr - len(DEFINE_STR + ID)) <= 0:
                                    Line = DEFINE_STR + ' ' + ID + ' ' + DecToHexStr(Index, 4) + '\n'
                                else:
                                    Line = DEFINE_STR + ' ' + ID + ' ' * (ValueStartPtr - len(DEFINE_STR + ID)) + DecToHexStr(Index, 4) + '\n'

                                if File not in FileDict:
                                    FileDict[File] = Index
                                else:
                                    DuplicateBlock = pack('B', EFI_HII_IIBT_DUPLICATE)
                                    DuplicateBlock += pack('H', FileDict[File])
                                    ImageBuffer += DuplicateBlock
                                    BufferStr = WriteLine(BufferStr, '// %s: %s: %s' % (DecToHexStr(Index, 4), ID, DecToHexStr(Index, 4)))
                                    TempBufferList = AscToHexList(DuplicateBlock)
                                    BufferStr = WriteLine(BufferStr, CreateArrayItem(TempBufferList, 16) + '\n')
                                    StringH.Append(Line)
                                    Index += 1
                                    continue

                                TmpFile = open(File.Path, 'rb')
                                Buffer = TmpFile.read()
                                TmpFile.close()
                                if File.Ext.upper() == '.PNG':
                                    TempBuffer = pack('B', EFI_HII_IIBT_IMAGE_PNG)
                                    TempBuffer += pack('I', len(Buffer))
                                    TempBuffer += Buffer
                                elif File.Ext.upper() == '.JPG':
                                    ImageType, = struct.unpack('4s', Buffer[6:10])
                                    if ImageType != b'JFIF':
                                        EdkLogger.error("build", FILE_TYPE_MISMATCH, "The file %s is not a standard JPG file." % File.Path)
                                    TempBuffer = pack('B', EFI_HII_IIBT_IMAGE_JPEG)
                                    TempBuffer += pack('I', len(Buffer))
                                    TempBuffer += Buffer
                                elif File.Ext.upper() == '.BMP':
                                    TempBuffer, TempPalette = BmpImageDecoder(File, Buffer, PaletteIndex, FileObj.TransParent)
                                    if len(TempPalette) > 1:
                                        PaletteIndex += 1
                                        NewPalette = pack('H', len(TempPalette))
                                        NewPalette += TempPalette
                                        PaletteBuffer += NewPalette
                                        PaletteStr = WriteLine(PaletteStr, '// %s: %s: %s' % (DecToHexStr(PaletteIndex - 1, 4), ID, DecToHexStr(PaletteIndex - 1, 4)))
                                        TempPaletteList = AscToHexList(NewPalette)
                                        PaletteStr = WriteLine(PaletteStr, CreateArrayItem(TempPaletteList, 16) + '\n')
                                ImageBuffer += TempBuffer
                                BufferStr = WriteLine(BufferStr, '// %s: %s: %s' % (DecToHexStr(Index, 4), ID, DecToHexStr(Index, 4)))
                                TempBufferList = AscToHexList(TempBuffer)
                                BufferStr = WriteLine(BufferStr, CreateArrayItem(TempBufferList, 16) + '\n')

                                StringH.Append(Line)
                                Index += 1
                        except IOError:
                            EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=File.Path)

            BufferStr = WriteLine(BufferStr, '// End of the Image Info')
            BufferStr = WriteLine(BufferStr, CreateArrayItem(DecToHexList(EFI_HII_IIBT_END, 2)) + '\n')
            ImageEnd = pack('B', EFI_HII_IIBT_END)
            ImageBuffer += ImageEnd

            if len(ImageBuffer) > 1:
                ImageInfoOffset = 12
            if len(PaletteBuffer) > 1:
                PaletteInfoOffset = 12 + len(ImageBuffer) - 1 # -1 is for the first empty pad byte of ImageBuffer

            IMAGE_PACKAGE_HDR = pack('=II', ImageInfoOffset, PaletteInfoOffset)
            # PACKAGE_HEADER_Length = PACKAGE_HEADER + ImageInfoOffset + PaletteInfoOffset + ImageBuffer Length + PaletteCount + PaletteBuffer Length
            if len(PaletteBuffer) > 1:
                PACKAGE_HEADER_Length = 4 + 4 + 4 + len(ImageBuffer) - 1 + 2 + len(PaletteBuffer) - 1
            else:
                PACKAGE_HEADER_Length = 4 + 4 + 4 + len(ImageBuffer) - 1
            if PaletteIndex > 1:
                PALETTE_INFO_HEADER = pack('H', PaletteIndex - 1)
            # EFI_HII_PACKAGE_HEADER length max value is 0xFFFFFF
            Hex_Length = '%06X' % PACKAGE_HEADER_Length
            if PACKAGE_HEADER_Length > 0xFFFFFF:
                EdkLogger.error("build", AUTOGEN_ERROR, "The Length of EFI_HII_PACKAGE_HEADER exceed its maximum value", ExtraData="[%s]" % str(Info))
            PACKAGE_HEADER = pack('=HBB', int('0x' + Hex_Length[2:], 16), int('0x' + Hex_Length[0:2], 16), EFI_HII_PACKAGE_IMAGES)

            IdfGenBinBuffer.write(PACKAGE_HEADER)
            IdfGenBinBuffer.write(IMAGE_PACKAGE_HDR)
            if len(ImageBuffer) > 1 :
                IdfGenBinBuffer.write(ImageBuffer[1:])
            if PaletteIndex > 1:
                IdfGenBinBuffer.write(PALETTE_INFO_HEADER)
            if len(PaletteBuffer) > 1:
                IdfGenBinBuffer.write(PaletteBuffer[1:])

            if IdfGenCFlag:
                TotalLength = EFI_HII_ARRAY_SIZE_LENGTH + PACKAGE_HEADER_Length
                AutoGenC.Append("\n//\n//Image Pack Definition\n//\n")
                AllStr = WriteLine('', CHAR_ARRAY_DEFIN + ' ' + Info.Module.BaseName + 'Images' + '[] = {\n')
                AllStr = WriteLine(AllStr, '// STRGATHER_OUTPUT_HEADER')
                AllStr = WriteLine(AllStr, CreateArrayItem(DecToHexList(TotalLength)) + '\n')
                AllStr = WriteLine(AllStr, '// Image PACKAGE HEADER\n')
                IMAGE_PACKAGE_HDR_List = AscToHexList(PACKAGE_HEADER)
                IMAGE_PACKAGE_HDR_List += AscToHexList(IMAGE_PACKAGE_HDR)
                AllStr = WriteLine(AllStr, CreateArrayItem(IMAGE_PACKAGE_HDR_List, 16) + '\n')
                AllStr = WriteLine(AllStr, '// Image DATA\n')
                if BufferStr:
                    AllStr = WriteLine(AllStr, BufferStr)
                if PaletteStr:
                    AllStr = WriteLine(AllStr, '// Palette Header\n')
                    PALETTE_INFO_HEADER_List = AscToHexList(PALETTE_INFO_HEADER)
                    AllStr = WriteLine(AllStr, CreateArrayItem(PALETTE_INFO_HEADER_List, 16) + '\n')
                    AllStr = WriteLine(AllStr, '// Palette Data\n')
                    AllStr = WriteLine(AllStr, PaletteStr)
                AllStr = WriteLine(AllStr, '};')
                AutoGenC.Append(AllStr)
                AutoGenC.Append("\n")
                StringH.Append('\nextern unsigned char ' + Info.Module.BaseName + 'Images[];\n')
                StringH.Append("\n#define IMAGE_ARRAY_NAME %sImages\n" % Info.Module.BaseName)

# typedef struct _EFI_HII_IMAGE_PACKAGE_HDR {
#   EFI_HII_PACKAGE_HEADER  Header;          # Standard package header, where Header.Type = EFI_HII_PACKAGE_IMAGES
#   UINT32                  ImageInfoOffset;
#   UINT32                  PaletteInfoOffset;
# } EFI_HII_IMAGE_PACKAGE_HDR;

# typedef struct {
#   UINT32   Length:24;
#   UINT32   Type:8;
#   UINT8    Data[];
# } EFI_HII_PACKAGE_HEADER;

# typedef struct _EFI_HII_IMAGE_BLOCK {
#   UINT8    BlockType;
#   UINT8    BlockBody[];
# } EFI_HII_IMAGE_BLOCK;

def BmpImageDecoder(File, Buffer, PaletteIndex, TransParent):
    ImageType, = struct.unpack('2s', Buffer[0:2])
    if ImageType!= b'BM': # BMP file type is 'BM'
        EdkLogger.error("build", FILE_TYPE_MISMATCH, "The file %s is not a standard BMP file." % File.Path)
    BMP_IMAGE_HEADER = collections.namedtuple('BMP_IMAGE_HEADER', ['bfSize', 'bfReserved1', 'bfReserved2', 'bfOffBits', 'biSize', 'biWidth', 'biHeight', 'biPlanes', 'biBitCount', 'biCompression', 'biSizeImage', 'biXPelsPerMeter', 'biYPelsPerMeter', 'biClrUsed', 'biClrImportant'])
    BMP_IMAGE_HEADER_STRUCT = struct.Struct('IHHIIIIHHIIIIII')
    BmpHeader = BMP_IMAGE_HEADER._make(BMP_IMAGE_HEADER_STRUCT.unpack_from(Buffer[2:]))
    #
    # Doesn't support compress.
    #
    if BmpHeader.biCompression != 0:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "The compress BMP file %s is not support." % File.Path)

    # The Width and Height is UINT16 type in Image Package
    if BmpHeader.biWidth > 0xFFFF:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "The BMP file %s Width is exceed 0xFFFF." % File.Path)
    if BmpHeader.biHeight > 0xFFFF:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "The BMP file %s Height is exceed 0xFFFF." % File.Path)

    PaletteBuffer = pack('x')
    if BmpHeader.biBitCount == 1:
        if TransParent:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_1BIT_TRANS)
        else:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_1BIT)
        ImageBuffer += pack('B', PaletteIndex)
        Width = (BmpHeader.biWidth + 7)//8
        if BmpHeader.bfOffBits > BMP_IMAGE_HEADER_STRUCT.size + 2:
            PaletteBuffer = Buffer[BMP_IMAGE_HEADER_STRUCT.size + 2 : BmpHeader.bfOffBits]
    elif BmpHeader.biBitCount == 4:
        if TransParent:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_4BIT_TRANS)
        else:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_4BIT)
        ImageBuffer += pack('B', PaletteIndex)
        Width = (BmpHeader.biWidth + 1)//2
        if BmpHeader.bfOffBits > BMP_IMAGE_HEADER_STRUCT.size + 2:
            PaletteBuffer = Buffer[BMP_IMAGE_HEADER_STRUCT.size + 2 : BmpHeader.bfOffBits]
    elif BmpHeader.biBitCount == 8:
        if TransParent:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_8BIT_TRANS)
        else:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_8BIT)
        ImageBuffer += pack('B', PaletteIndex)
        Width = BmpHeader.biWidth
        if BmpHeader.bfOffBits > BMP_IMAGE_HEADER_STRUCT.size + 2:
            PaletteBuffer = Buffer[BMP_IMAGE_HEADER_STRUCT.size + 2 : BmpHeader.bfOffBits]
    elif BmpHeader.biBitCount == 24:
        if TransParent:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_24BIT_TRANS)
        else:
            ImageBuffer = pack('B', EFI_HII_IIBT_IMAGE_24BIT)
        Width = BmpHeader.biWidth * 3
    else:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "Only support the 1 bit, 4 bit, 8bit, 24 bit BMP files.", ExtraData="[%s]" % str(File.Path))

    ImageBuffer += pack('H', BmpHeader.biWidth)
    ImageBuffer += pack('H', BmpHeader.biHeight)
    Start = BmpHeader.bfOffBits
    End = BmpHeader.bfSize - 1
    for Height in range(0, BmpHeader.biHeight):
        if Width % 4 != 0:
            Start = End + (Width % 4) - 4 - Width
        else:
            Start = End - Width
        ImageBuffer += Buffer[Start + 1 : Start + Width + 1]
        End = Start

    # handle the Palette info,  BMP use 4 bytes for R, G, B and Reserved info while EFI_HII_RGB_PIXEL only have the R, G, B info
    if PaletteBuffer and len(PaletteBuffer) > 1:
        PaletteTemp = pack('x')
        for Index in range(0, len(PaletteBuffer)):
            if Index % 4 == 3:
                continue
            PaletteTemp += PaletteBuffer[Index:Index+1]
        PaletteBuffer = PaletteTemp[1:]
    return ImageBuffer, PaletteBuffer

## Create common code
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateHeaderCode(Info, AutoGenC, AutoGenH):
    # file header
    AutoGenH.Append(gAutoGenHeaderString.Replace({'FileName':'AutoGen.h'}))
    # header file Prologue
    AutoGenH.Append(gAutoGenHPrologueString.Replace({'File':'AUTOGENH','Guid':Info.Guid.replace('-', '_')}))
    AutoGenH.Append(gAutoGenHCppPrologueString)

    # header files includes
    if Info.ModuleType in gModuleTypeHeaderFile:
        AutoGenH.Append("#include <%s>\n" % gModuleTypeHeaderFile[Info.ModuleType][0])
    #
    # if either PcdLib in [LibraryClasses] sections or there exist Pcd section, add PcdLib.h
    # As if modules only uses FixedPcd, then PcdLib is not needed in [LibraryClasses] section.
    #
    if 'PcdLib' in Info.Module.LibraryClasses or Info.Module.Pcds:
        AutoGenH.Append("#include <Library/PcdLib.h>\n")

    AutoGenH.Append('\nextern GUID  gEfiCallerIdGuid;')
    AutoGenH.Append('\nextern GUID  gEdkiiDscPlatformGuid;')
    AutoGenH.Append('\nextern CHAR8 *gEfiCallerBaseName;\n\n')

    if Info.IsLibrary:
        return

    AutoGenH.Append("#define EFI_CALLER_ID_GUID \\\n  %s\n" % GuidStringToGuidStructureString(Info.Guid))
    AutoGenH.Append("#define EDKII_DSC_PLATFORM_GUID \\\n  %s\n" % GuidStringToGuidStructureString(Info.PlatformInfo.Guid))

    if Info.IsLibrary:
        return
    # C file header
    AutoGenC.Append(gAutoGenHeaderString.Replace({'FileName':'AutoGen.c'}))
    # C file header files includes
    if Info.ModuleType in gModuleTypeHeaderFile:
        for Inc in gModuleTypeHeaderFile[Info.ModuleType]:
            AutoGenC.Append("#include <%s>\n" % Inc)
    else:
        AutoGenC.Append("#include <%s>\n" % gBasicHeaderFile)

    #
    # Publish the CallerId Guid
    #
    AutoGenC.Append('\nGLOBAL_REMOVE_IF_UNREFERENCED GUID gEfiCallerIdGuid = %s;\n' % GuidStringToGuidStructureString(Info.Guid))
    AutoGenC.Append('\nGLOBAL_REMOVE_IF_UNREFERENCED GUID gEdkiiDscPlatformGuid = %s;\n' % GuidStringToGuidStructureString(Info.PlatformInfo.Guid))
    AutoGenC.Append('\nGLOBAL_REMOVE_IF_UNREFERENCED CHAR8 *gEfiCallerBaseName = "%s";\n' % Info.Name)

## Create common code for header file
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#
def CreateFooterCode(Info, AutoGenC, AutoGenH):
    AutoGenH.Append(gAutoGenHEpilogueString)

## Create code for a module
#
#   @param      Info        The ModuleAutoGen object
#   @param      AutoGenC    The TemplateString object for C code
#   @param      AutoGenH    The TemplateString object for header file
#   @param      StringH     The TemplateString object for header file
#   @param      UniGenCFlag     UniString is generated into AutoGen C file when it is set to True
#   @param      UniGenBinBuffer Buffer to store uni string package data
#   @param      StringIdf       The TemplateString object for header file
#   @param      IdfGenCFlag     IdfString is generated into AutoGen C file when it is set to True
#   @param      IdfGenBinBuffer Buffer to store Idf string package data
#
def CreateCode(Info, AutoGenC, AutoGenH, StringH, UniGenCFlag, UniGenBinBuffer, StringIdf, IdfGenCFlag, IdfGenBinBuffer):
    CreateHeaderCode(Info, AutoGenC, AutoGenH)

    # The only 32 bit archs we have are IA32 and ARM, everything else is 64 bit
    Bitwidth = 32 if Info.Arch == 'IA32' or Info.Arch == 'ARM' else 64

    if GlobalData.gStackCookieValues64 == [] and os.path.exists(os.path.join(Info.PlatformInfo.BuildDir, "StackCookieValues64.json")):
        with open (os.path.join(Info.PlatformInfo.BuildDir, "StackCookieValues64.json"), "r") as file:
            GlobalData.gStackCookieValues64 = json.load(file)
    if GlobalData.gStackCookieValues32 == [] and os.path.exists(os.path.join(Info.PlatformInfo.BuildDir, "StackCookieValues32.json")):
        with open (os.path.join(Info.PlatformInfo.BuildDir, "StackCookieValues32.json"), "r") as file:
            GlobalData.gStackCookieValues32 = json.load(file)

    try:
        if Bitwidth == 32:
            CookieValue = int(GlobalData.gStackCookieValues32[hash(Info.Guid) % len(GlobalData.gStackCookieValues32)])
        else:
            CookieValue = int(GlobalData.gStackCookieValues64[hash(Info.Guid) % len(GlobalData.gStackCookieValues64)])
    except:
        EdkLogger.warn("build", "Failed to get Stack Cookie Value List! Generating random value.", ExtraData="[%s]" % str(Info))
        if Bitwidth == 32:
            CookieValue = secrets.randbelow (0xFFFFFFFF)
        else:
            CookieValue = secrets.randbelow (0xFFFFFFFFFFFFFFFF)

    AutoGenH.Append((
        '#define STACK_COOKIE_VALUE 0x%XULL\n' % CookieValue
        if Bitwidth == 64 else
        '#define STACK_COOKIE_VALUE 0x%X\n' % CookieValue
    ))

    CreateGuidDefinitionCode(Info, AutoGenC, AutoGenH)
    CreateProtocolDefinitionCode(Info, AutoGenC, AutoGenH)
    CreatePpiDefinitionCode(Info, AutoGenC, AutoGenH)
    CreatePcdCode(Info, AutoGenC, AutoGenH)
    CreateLibraryConstructorCode(Info, AutoGenC, AutoGenH)
    CreateLibraryDestructorCode(Info, AutoGenC, AutoGenH)
    CreateModuleEntryPointCode(Info, AutoGenC, AutoGenH)
    CreateModuleUnloadImageCode(Info, AutoGenC, AutoGenH)

    if Info.UnicodeFileList:
        FileName = "%sStrDefs.h" % Info.Name
        StringH.Append(gAutoGenHeaderString.Replace({'FileName':FileName}))
        StringH.Append(gAutoGenHPrologueString.Replace({'File':'STRDEFS', 'Guid':Info.Guid.replace('-', '_')}))
        CreateUnicodeStringCode(Info, AutoGenC, StringH, UniGenCFlag, UniGenBinBuffer)

        GuidMacros = []
        for Guid in Info.Module.Guids:
            if Guid in Info.Module.GetGuidsUsedByPcd():
                continue
            GuidMacros.append('#define %s %s' % (Guid, Info.Module.Guids[Guid]))
        for Guid, Value in list(Info.Module.Protocols.items()) + list(Info.Module.Ppis.items()):
            GuidMacros.append('#define %s %s' % (Guid, Value))
        # supports FixedAtBuild and FeaturePcd usage in VFR file
        if Info.VfrFileList and Info.ModulePcdList:
            GuidMacros.append('#define %s %s' % ('FixedPcdGetBool(TokenName)', '_PCD_VALUE_##TokenName'))
            GuidMacros.append('#define %s %s' % ('FixedPcdGet8(TokenName)', '_PCD_VALUE_##TokenName'))
            GuidMacros.append('#define %s %s' % ('FixedPcdGet16(TokenName)', '_PCD_VALUE_##TokenName'))
            GuidMacros.append('#define %s %s' % ('FixedPcdGet32(TokenName)', '_PCD_VALUE_##TokenName'))
            GuidMacros.append('#define %s %s' % ('FixedPcdGet64(TokenName)', '_PCD_VALUE_##TokenName'))
            GuidMacros.append('#define %s %s' % ('FeaturePcdGet(TokenName)', '_PCD_VALUE_##TokenName'))
            for Pcd in Info.ModulePcdList:
                if Pcd.Type in [TAB_PCDS_FIXED_AT_BUILD, TAB_PCDS_FEATURE_FLAG]:
                    TokenCName = Pcd.TokenCName
                    Value = Pcd.DefaultValue
                    if Pcd.DatumType == 'BOOLEAN':
                        BoolValue = Value.upper()
                        if BoolValue == 'TRUE':
                            Value = '1'
                        elif BoolValue == 'FALSE':
                            Value = '0'
                    for PcdItem in GlobalData.MixedPcd:
                        if (Pcd.TokenCName, Pcd.TokenSpaceGuidCName) in GlobalData.MixedPcd[PcdItem]:
                            TokenCName = PcdItem[0]
                            break
                    GuidMacros.append('#define %s %s' % ('_PCD_VALUE_'+TokenCName, Value))

        if Info.IdfFileList:
            GuidMacros.append('#include "%sImgDefs.h"' % Info.Name)

        if GuidMacros:
            StringH.Append('\n#ifdef VFRCOMPILE\n%s\n#endif\n' % '\n'.join(GuidMacros))

        StringH.Append("\n#endif\n")
        AutoGenH.Append('#include "%s"\n' % FileName)

    if Info.IdfFileList:
        FileName = "%sImgDefs.h" % Info.Name
        StringIdf.Append(gAutoGenHeaderString.Replace({'FileName':FileName}))
        StringIdf.Append(gAutoGenHPrologueString.Replace({'File':'IMAGEDEFS', 'Guid':Info.Guid.replace('-', '_')}))
        CreateIdfFileCode(Info, AutoGenC, StringIdf, IdfGenCFlag, IdfGenBinBuffer)

        StringIdf.Append("\n#endif\n")
        AutoGenH.Append('#include "%s"\n' % FileName)

    CreateFooterCode(Info, AutoGenC, AutoGenH)

## Create the code file
#
#   @param      FilePath     The path of code file
#   @param      Content      The content of code file
#   @param      IsBinaryFile The flag indicating if the file is binary file or not
#
#   @retval     True        If file content is changed or file doesn't exist
#   @retval     False       If the file exists and the content is not changed
#
def Generate(FilePath, Content, IsBinaryFile):
    return SaveFileOnChange(FilePath, Content, IsBinaryFile)

