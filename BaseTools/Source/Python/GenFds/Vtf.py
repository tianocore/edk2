## @file
# process VTF generation
#
#  Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
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
from __future__ import absolute_import
from .GenFdsGlobalVariable import GenFdsGlobalVariable
import Common.LongFilePathOs as os
from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.DataType import TAB_LINE_BREAK

## generate VTF
#
#
class Vtf (object):

    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        self.KeyArch = None
        self.ArchList = None
        self.UiName = None
        self.ResetBin = None
        self.ComponentStatementList = []

    ## GenVtf() method
    #
    #   Generate VTF
    #
    #   @param  self        The object pointer
    #   @param  FdAddressDict   dictionary contains FV name and its base address
    #   @retval Dict        FV and corresponding VTF file name
    #
    def GenVtf(self, FdAddressDict) :
        self.GenBsfInf()
        BaseAddArg = self.GetBaseAddressArg(FdAddressDict)
        OutputArg, VtfRawDict = self.GenOutputArg()

        Cmd = (
            'GenVtf',
            ) + OutputArg + (
            '-f', self.BsfInfName,
            ) + BaseAddArg

        GenFdsGlobalVariable.CallExternalTool(Cmd, "GenFv -Vtf Failed!")
        GenFdsGlobalVariable.SharpCounter = 0

        return VtfRawDict

    ## GenBsfInf() method
    #
    #   Generate inf used to generate VTF
    #
    #   @param  self        The object pointer
    #
    def GenBsfInf (self):
        FvList = self.GetFvList()
        self.BsfInfName = os.path.join(GenFdsGlobalVariable.FvDir, self.UiName + '.inf')
        BsfInf = open(self.BsfInfName, 'w+')
        if self.ResetBin:
            BsfInf.writelines ("[OPTIONS]" + TAB_LINE_BREAK)
            BsfInf.writelines ("IA32_RST_BIN" + \
                               " = " + \
                               GenFdsGlobalVariable.MacroExtend(GenFdsGlobalVariable.ReplaceWorkspaceMacro(self.ResetBin)) + \
                               TAB_LINE_BREAK)
            BsfInf.writelines (TAB_LINE_BREAK)

        BsfInf.writelines ("[COMPONENTS]" + TAB_LINE_BREAK)

        for ComponentObj in self.ComponentStatementList :
            BsfInf.writelines ("COMP_NAME" + \
                               " = " + \
                               ComponentObj.CompName + \
                               TAB_LINE_BREAK)
            if ComponentObj.CompLoc.upper() == 'NONE':
                BsfInf.writelines ("COMP_LOC" + \
                                   " = " + \
                                   'N' + \
                                   TAB_LINE_BREAK)

            elif ComponentObj.FilePos:
                BsfInf.writelines ("COMP_LOC" + \
                                   " = " + \
                                   ComponentObj.FilePos + \
                                   TAB_LINE_BREAK)
            else:
                Index = FvList.index(ComponentObj.CompLoc.upper())
                if Index == 0:
                    BsfInf.writelines ("COMP_LOC" + \
                                       " = " + \
                                       'F' + \
                                       TAB_LINE_BREAK)
                elif Index == 1:
                    BsfInf.writelines ("COMP_LOC" + \
                                       " = " + \
                                       'S' + \
                                       TAB_LINE_BREAK)

            BsfInf.writelines ("COMP_TYPE" + \
                               " = " + \
                               ComponentObj.CompType + \
                               TAB_LINE_BREAK)
            BsfInf.writelines ("COMP_VER" + \
                               " = " + \
                               ComponentObj.CompVer + \
                               TAB_LINE_BREAK)
            BsfInf.writelines ("COMP_CS" + \
                               " = " + \
                               ComponentObj.CompCs + \
                               TAB_LINE_BREAK)

            BinPath = ComponentObj.CompBin
            if BinPath != '-':
                BinPath = GenFdsGlobalVariable.MacroExtend(GenFdsGlobalVariable.ReplaceWorkspaceMacro(BinPath))
            BsfInf.writelines ("COMP_BIN" + \
                               " = " + \
                               BinPath + \
                               TAB_LINE_BREAK)

            SymPath = ComponentObj.CompSym
            if SymPath != '-':
                SymPath = GenFdsGlobalVariable.MacroExtend(GenFdsGlobalVariable.ReplaceWorkspaceMacro(SymPath))
            BsfInf.writelines ("COMP_SYM" + \
                               " = " + \
                               SymPath + \
                               TAB_LINE_BREAK)
            BsfInf.writelines ("COMP_SIZE" + \
                               " = " + \
                               ComponentObj.CompSize + \
                               TAB_LINE_BREAK)
            BsfInf.writelines (TAB_LINE_BREAK)

        BsfInf.close()

    ## GenFvList() method
    #
    #   Get FV list referenced by VTF components
    #
    #   @param  self        The object pointer
    #
    def GetFvList(self):
        FvList = []
        for component in self.ComponentStatementList :
            if component.CompLoc.upper() != 'NONE' and not (component.CompLoc.upper() in FvList):
                FvList.append(component.CompLoc.upper())

        return FvList

    ## GetBaseAddressArg() method
    #
    #   Get base address arguments for GenVtf
    #
    #   @param  self        The object pointer
    #
    def GetBaseAddressArg(self, FdAddressDict):
        FvList = self.GetFvList()
        CmdStr = tuple()
        for i in FvList:
            (BaseAddress, Size) = FdAddressDict.get(i)
            CmdStr += (
                '-r', '0x%x' % BaseAddress,
                '-s', '0x%x' % Size,
                )
        return CmdStr

    ## GenOutputArg() method
    #
    #   Get output arguments for GenVtf
    #
    #   @param  self        The object pointer
    #
    def GenOutputArg(self):
        FvVtfDict = {}
        OutputFileName = ''
        FvList = self.GetFvList()
        Index = 0
        Arg = tuple()
        for FvObj in FvList:
            Index = Index + 1
            OutputFileName = 'Vtf%d.raw' % Index
            OutputFileName = os.path.join(GenFdsGlobalVariable.FvDir, OutputFileName)
            Arg += ('-o', OutputFileName)
            FvVtfDict[FvObj.upper()] = OutputFileName

        return Arg, FvVtfDict

