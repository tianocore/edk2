## @file
# process depex section generation
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Section
from GenFdsGlobalVariable import GenFdsGlobalVariable
import subprocess
from Ffs import Ffs
import os
from CommonDataClass.FdfClass import DepexSectionClassObject
from AutoGen.GenDepex import DependencyExpression
import shutil
from Common import EdkLogger
from Common.BuildToolError import *

## generate data section
#
#
class DepexSection (DepexSectionClassObject):
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        DepexSectionClassObject.__init__(self)

    def __FindGuidValue(self, CName):
        for Arch in GenFdsGlobalVariable.ArchList:
            for PkgDb in GenFdsGlobalVariable.WorkSpace.GetPackageList(GenFdsGlobalVariable.ActivePlatform, 
                                                                       Arch, 
                                                                       GenFdsGlobalVariable.TargetName, 
                                                                       GenFdsGlobalVariable.ToolChainTag):
                if CName in PkgDb.Ppis:
                    return PkgDb.Ppis[CName]
                if CName in PkgDb.Protocols:
                    return PkgDb.Protocols[CName]
                if CName in PkgDb.Guids:
                    return PkgDb.Guids[CName]
        return None

    ## GenSection() method
    #
    #   Generate compressed section
    #
    #   @param  self        The object pointer
    #   @param  OutputPath  Where to place output file
    #   @param  ModuleName  Which module this section belongs to
    #   @param  SecNum      Index of section
    #   @param  KeyStringList  Filter for inputs of section generation
    #   @param  FfsInf      FfsInfStatement object that contains this section data
    #   @param  Dict        dictionary contains macro and its value
    #   @retval tuple       (Generated file name list, section alignment)
    #
    def GenSection(self, OutputPath, ModuleName, SecNum, keyStringList, FfsFile = None, Dict = {}):
        
        if self.ExpressionProcessed == False:
            self.Expression = self.Expression.replace("\n", " ").replace("\r", " ")
            ExpList = self.Expression.split()
            ExpGuidDict = {}

            for Exp in ExpList:
                if Exp.upper() not in ('AND', 'OR', 'NOT', 'TRUE', 'FALSE', 'SOR', 'BEFORE', 'AFTER', 'END'):
                    GuidStr = self.__FindGuidValue(Exp)
                    if GuidStr == None:
                        EdkLogger.error("GenFds", RESOURCE_NOT_AVAILABLE,
                                        "Depex GUID %s could not be found in build DB! (ModuleName: %s)" % (Exp, ModuleName))

                    ExpGuidDict[Exp] = GuidStr

            for Item in ExpGuidDict:
                self.Expression = self.Expression.replace(Item, ExpGuidDict[Item])

            self.Expression = self.Expression.strip()
            self.ExpressionProcessed = True

        if self.DepexType == 'PEI_DEPEX_EXP':
            ModuleType = 'PEIM'
            SecType    = 'PEI_DEPEX'
        elif self.DepexType == 'DXE_DEPEX_EXP':
            ModuleType = 'DXE_DRIVER'
            SecType    = 'DXE_DEPEX'
        elif self.DepexType == 'SMM_DEPEX_EXP':
            ModuleType = 'DXE_SMM_DRIVER'
            SecType    = 'SMM_DEPEX'
        else:
            EdkLogger.error("GenFds", FORMAT_INVALID,
                            "Depex type %s is not valid for module %s" % (self.DepexType, ModuleName))

        InputFile = os.path.join (OutputPath, ModuleName + 'SEC' + SecNum + '.depex')
        InputFile = os.path.normpath(InputFile)
        Depex = DependencyExpression(self.Expression, ModuleType)
        Depex.Generate(InputFile)

        OutputFile = os.path.join (OutputPath, ModuleName + 'SEC' + SecNum + '.dpx')
        OutputFile = os.path.normpath(OutputFile)

        GenFdsGlobalVariable.GenerateSection(OutputFile, [InputFile], Section.Section.SectionType.get (SecType))
        FileList = [OutputFile]
        return FileList, self.Alignment
