## @file
# process depex section generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
from . import Section
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import Common.LongFilePathOs as os
from CommonDataClass.FdfClass import DepexSectionClassObject
from AutoGen.GenDepex import DependencyExpression
from Common import EdkLogger
from Common.BuildToolError import *
from Common.Misc import PathClass
from Common.DataType import *

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
            PkgList = GenFdsGlobalVariable.WorkSpace.GetPackageList(GenFdsGlobalVariable.ActivePlatform,
                                                                    Arch,
                                                                    GenFdsGlobalVariable.TargetName,
                                                                    GenFdsGlobalVariable.ToolChainTag)
            for Inf in GenFdsGlobalVariable.FdfParser.Profile.InfList:
                ModuleData = GenFdsGlobalVariable.WorkSpace.BuildObject[
                                                            PathClass(Inf, GenFdsGlobalVariable.WorkSpaceDir),
                                                            Arch,
                                                            GenFdsGlobalVariable.TargetName,
                                                            GenFdsGlobalVariable.ToolChainTag
                                                            ]
                for Pkg in ModuleData.Packages:
                    if Pkg not in PkgList:
                        PkgList.append(Pkg)
            for PkgDb in PkgList:
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
    def GenSection(self, OutputPath, ModuleName, SecNum, keyStringList, FfsFile = None, Dict = None, IsMakefile = False):
        if self.ExpressionProcessed == False:
            self.Expression = self.Expression.replace("\n", " ").replace("\r", " ")
            ExpList = self.Expression.split()

            for Exp in ExpList:
                if Exp.upper() not in ('AND', 'OR', 'NOT', 'TRUE', 'FALSE', 'SOR', 'BEFORE', 'AFTER', 'END'):
                    GuidStr = self.__FindGuidValue(Exp)
                    if GuidStr is None:
                        EdkLogger.error("GenFds", RESOURCE_NOT_AVAILABLE,
                                        "Depex GUID %s could not be found in build DB! (ModuleName: %s)" % (Exp, ModuleName))

                    self.Expression = self.Expression.replace(Exp, GuidStr)

            self.Expression = self.Expression.strip()
            self.ExpressionProcessed = True

        if self.DepexType == 'PEI_DEPEX_EXP':
            ModuleType = SUP_MODULE_PEIM
            SecType    = BINARY_FILE_TYPE_PEI_DEPEX
        elif self.DepexType == 'DXE_DEPEX_EXP':
            ModuleType = SUP_MODULE_DXE_DRIVER
            SecType    = BINARY_FILE_TYPE_DXE_DEPEX
        elif self.DepexType == 'SMM_DEPEX_EXP':
            ModuleType = SUP_MODULE_DXE_SMM_DRIVER
            SecType    = BINARY_FILE_TYPE_SMM_DEPEX
        else:
            EdkLogger.error("GenFds", FORMAT_INVALID,
                            "Depex type %s is not valid for module %s" % (self.DepexType, ModuleName))

        InputFile = os.path.join (OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + '.depex')
        InputFile = os.path.normpath(InputFile)
        Depex = DependencyExpression(self.Expression, ModuleType)
        Depex.Generate(InputFile)

        OutputFile = os.path.join (OutputPath, ModuleName + SUP_MODULE_SEC + SecNum + '.dpx')
        OutputFile = os.path.normpath(OutputFile)

        GenFdsGlobalVariable.GenerateSection(OutputFile, [InputFile], Section.Section.SectionType.get (SecType), IsMakefile=IsMakefile)
        return [OutputFile], self.Alignment
