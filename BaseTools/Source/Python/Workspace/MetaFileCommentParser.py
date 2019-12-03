## @file
# This file is used to check format of comments
#
# Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from CommonDataClass.DataClass import (
    MODEL_PCD_PATCHABLE_IN_MODULE,
    MODEL_PCD_DYNAMIC_EX,
    MODEL_PCD_DYNAMIC,
    MODEL_EFI_GUID,
    MODEL_EFI_PPI,
    MODEL_EFI_PROTOCOL
)
from Common.BuildToolError import FORMAT_INVALID
import Common.EdkLogger as EdkLogger

UsageList = ("PRODUCES", "PRODUCED", "ALWAYS_PRODUCES", "ALWAYS_PRODUCED", "SOMETIMES_PRODUCES",
             "SOMETIMES_PRODUCED", "CONSUMES", "CONSUMED", "ALWAYS_CONSUMES", "ALWAYS_CONSUMED",
             "SOMETIMES_CONSUMES", "SOMETIMES_CONSUMED", "SOMETIME_CONSUMES")
ErrorMsgMap = {
    MODEL_EFI_GUID      : "The usage for this GUID is not listed in this INF: %s[%d]:%s",
    MODEL_EFI_PPI       : "The usage for this PPI is not listed in this INF: %s[%d]:%s.",
    MODEL_EFI_PROTOCOL  : "The usage for this Protocol is not listed in this INF: %s[%d]:%s.",
    MODEL_PCD_DYNAMIC   : "The usage for this PCD is not listed in this INF: %s[%d]:%s."
}

def CheckInfComment(SectionType, Comments, InfFile, LineNo, ValueList):
    if SectionType in [MODEL_PCD_PATCHABLE_IN_MODULE, MODEL_PCD_DYNAMIC_EX, MODEL_PCD_DYNAMIC]:
        CheckUsage(Comments, UsageList, InfFile, LineNo, ValueList[0]+'.'+ValueList[1], ErrorMsgMap[MODEL_PCD_DYNAMIC])
    elif SectionType in [MODEL_EFI_GUID, MODEL_EFI_PPI]:
        CheckUsage(Comments, UsageList, InfFile, LineNo, ValueList[0], ErrorMsgMap[SectionType])
    elif SectionType == MODEL_EFI_PROTOCOL:
        CheckUsage(Comments, UsageList + ("TO_START", "BY_START"), InfFile, LineNo, ValueList[0], ErrorMsgMap[SectionType])

def CheckUsage(Comments, Usages, InfFile, LineNo, Value, ErrorMsg):
    for Comment in Comments:
        for Word in Comment[0].replace('#', ' ').split():
            if Word in Usages:
                return
    EdkLogger.error(
        "Parser", FORMAT_INVALID,
        ErrorMsg % (InfFile, LineNo, Value)
    )
