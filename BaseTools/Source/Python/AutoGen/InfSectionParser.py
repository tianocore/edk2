## @file
# Parser a Inf file and Get specify section data.
#
# Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
        Filename = self._FilePath
        FileLinesList = []
        UserExtFind = False
        FindEnd = True
        FileLastLine = False
        SectionLine = ''
        SectionData = []
        
        try:
            FileLinesList = open(Filename, "r", 0).readlines()
        except BaseException:
            EdkLogger.error("build", AUTOGEN_ERROR, 'File %s is opened failed.' % Filename)
        
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
            if line.lower().startswith(TAB_SECTION_START) and line.lower().endswith(TAB_SECTION_END):
                SectionLine = line
                UserExtFind = True
                FindEnd = False
            
            if (NextLine != '' and NextLine[0] == TAB_SECTION_START and \
                NextLine[-1] == TAB_SECTION_END) or FileLastLine:
                UserExtFind = False
                FindEnd = True
                self._FileSectionDataList.append({SectionLine: SectionData[:]})
                SectionData = []
                SectionLine = ''
    

    # Get depex expresion
    #
    # @return: a list include some dictionary that key is section and value is a list contain all data.
    def GetDepexExpresionList(self):
        DepexExpresionList = []
        if not self._FileSectionDataList:
            return DepexExpresionList
        for SectionDataDict in self._FileSectionDataList:
            for key in SectionDataDict.keys():
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
                        DepexExpresionList.append({SubKey: SectionDataDict[key]})
        return DepexExpresionList















