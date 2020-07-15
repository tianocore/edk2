## @file
# Parser a Inf file and Get specify section data.
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#

import Common.EdkLogger as EdkLogger
from Common.BuildToolError import *
from Common.DataType import *


class InfSectionParser():
    def __init__(self, FilePath):
        self._FilePath = FilePath
        self._FileSectionDataList = []
        self._ParserInf()

    def _ParserInf(self):
        FileLinesList = []
        UserExtFind = False
        FindEnd = True
        FileLastLine = False
        SectionLine = ''
        SectionData = []

        try:
            with open(self._FilePath, "r") as File:
                FileLinesList = File.readlines()
        except BaseException:
            EdkLogger.error("build", AUTOGEN_ERROR, 'File %s is opened failed.' % self._FilePath)

        for Index in range(0, len(FileLinesList)):
            line = str(FileLinesList[Index]).strip()
            if Index + 1 == len(FileLinesList):
                FileLastLine = True
                NextLine = ''
            else:
                NextLine = str(FileLinesList[Index + 1]).strip()
            if UserExtFind and FindEnd == False:
                if line:
                    SectionData.append(line)
            if line.startswith(TAB_SECTION_START) and line.endswith(TAB_SECTION_END):
                SectionLine = line
                UserExtFind = True
                FindEnd = False

            if (NextLine != '' and NextLine[0] == TAB_SECTION_START and \
                NextLine[-1] == TAB_SECTION_END) or FileLastLine:
                UserExtFind = False
                FindEnd = True
                self._FileSectionDataList.append({SectionLine: SectionData[:]})
                del SectionData[:]
                SectionLine = ''

    # Get user extension TianoCore data
    #
    # @return: a list include some dictionary that key is section and value is a list contain all data.
    def GetUserExtensionTianoCore(self):
        UserExtensionTianoCore = []
        if not self._FileSectionDataList:
            return UserExtensionTianoCore
        for SectionDataDict in self._FileSectionDataList:
            for key in SectionDataDict:
                if key.lower().startswith("[userextensions") and key.lower().find('.tianocore.') > -1:
                    SectionLine = key.lstrip(TAB_SECTION_START).rstrip(TAB_SECTION_END)
                    SubSectionList = [SectionLine]
                    if str(SectionLine).find(TAB_COMMA_SPLIT) > -1:
                        SubSectionList = str(SectionLine).split(TAB_COMMA_SPLIT)
                    for SubSection in SubSectionList:
                        if SubSection.lower().find('.tianocore.') > -1:
                            UserExtensionTianoCore.append({SubSection: SectionDataDict[key]})
        return UserExtensionTianoCore

    # Get depex expression
    #
    # @return: a list include some dictionary that key is section and value is a list contain all data.
    def GetDepexExpresionList(self):
        DepexExpressionList = []
        if not self._FileSectionDataList:
            return DepexExpressionList
        for SectionDataDict in self._FileSectionDataList:
            for key in SectionDataDict:
                if key.lower() == "[depex]" or key.lower().startswith("[depex."):
                    SectionLine = key.lstrip(TAB_SECTION_START).rstrip(TAB_SECTION_END)
                    SubSectionList = [SectionLine]
                    if str(SectionLine).find(TAB_COMMA_SPLIT) > -1:
                        SubSectionList = str(SectionLine).split(TAB_COMMA_SPLIT)
                    for SubSection in SubSectionList:
                        SectionList = SubSection.split(TAB_SPLIT)
                        SubKey = ()
                        if len(SectionList) == 1:
                            SubKey = (TAB_ARCH_COMMON, TAB_ARCH_COMMON)
                        elif len(SectionList) == 2:
                            SubKey = (SectionList[1], TAB_ARCH_COMMON)
                        elif len(SectionList) == 3:
                            SubKey = (SectionList[1], SectionList[2])
                        else:
                            EdkLogger.error("build", AUTOGEN_ERROR, 'Section %s is invalid.' % key)
                        DepexExpressionList.append({SubKey: SectionDataDict[key]})
        return DepexExpressionList















