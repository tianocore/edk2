##
# Generate symbal for SMI handler profile info.
#
# This tool depends on DIA2Dump.exe (VS) or nm (gcc) to parse debug entry.
#
# Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

from __future__ import print_function
import os
import re
import sys
from optparse import OptionParser

from xml.dom.minidom import parse
import xml.dom.minidom

versionNumber = "1.1"
__copyright__ = "Copyright (c) 2016, Intel Corporation. All rights reserved."

class Symbols:
    def __init__(self):
        self.listLineAddress = []
        self.pdbName = ""
        # Cache for function
        self.functionName = ""
        # Cache for line
        self.sourceName = ""


    def getSymbol (self, rva):
        index = 0
        lineName  = 0
        sourceName = "??"
        while index + 1 < self.lineCount :
            if self.listLineAddress[index][0] <= rva and self.listLineAddress[index + 1][0] > rva :
                offset = rva - self.listLineAddress[index][0]
                functionName = self.listLineAddress[index][1]
                lineName = self.listLineAddress[index][2]
                sourceName = self.listLineAddress[index][3]
                if lineName == 0 :
                  return [functionName]
                else :
                  return [functionName, sourceName, lineName]
            index += 1

        return []

    def parse_debug_file(self, driverName, pdbName):
        if cmp (pdbName, "") == 0 :
            return
        self.pdbName = pdbName;

        try:
            nmCommand = "nm"
            nmLineOption = "-l"
            print("parsing (debug) - " + pdbName)
            os.system ('%s %s %s > nmDump.line.log' % (nmCommand, nmLineOption, pdbName))
        except :
            print('ERROR: nm command not available.  Please verify PATH')
            return

        #
        # parse line
        #
        linefile = open("nmDump.line.log")
        reportLines = linefile.readlines()
        linefile.close()

        # 000113ca T AllocatePool c:\home\edk-ii\MdePkg\Library\UefiMemoryAllocationLib\MemoryAllocationLib.c:399
        patchLineFileMatchString = "([0-9a-fA-F]*)\s+[T|D|t|d]\s+(\w+)\s*((?:[a-zA-Z]:)?[\w+\-./_a-zA-Z0-9\\\\]*):?([0-9]*)"

        for reportLine in reportLines:
            match = re.match(patchLineFileMatchString, reportLine)
            if match is not None:
                rva = int (match.group(1), 16)
                functionName = match.group(2)
                sourceName = match.group(3)
                if cmp (match.group(4), "") != 0 :
                    lineName = int (match.group(4))
                else :
                    lineName = 0
                self.listLineAddress.append ([rva, functionName, lineName, sourceName])

        self.lineCount = len (self.listLineAddress)

        self.listLineAddress = sorted(self.listLineAddress, key=lambda symbolAddress:symbolAddress[0])

    def parse_pdb_file(self, driverName, pdbName):
        if cmp (pdbName, "") == 0 :
            return
        self.pdbName = pdbName;

        try:
            #DIA2DumpCommand = "\"C:\\Program Files (x86)\Microsoft Visual Studio 14.0\\DIA SDK\\Samples\\DIA2Dump\\x64\\Debug\\Dia2Dump.exe\""
            DIA2DumpCommand = "Dia2Dump.exe"
            #DIA2SymbolOption = "-p"
            DIA2LinesOption = "-l"
            print("parsing (pdb) - " + pdbName)
            #os.system ('%s %s %s > DIA2Dump.symbol.log' % (DIA2DumpCommand, DIA2SymbolOption, pdbName))
            os.system ('%s %s %s > DIA2Dump.line.log' % (DIA2DumpCommand, DIA2LinesOption, pdbName))
        except :
            print('ERROR: DIA2Dump command not available.  Please verify PATH')
            return

        #
        # parse line
        #
        linefile = open("DIA2Dump.line.log")
        reportLines = linefile.readlines()
        linefile.close()

        #   ** GetDebugPrintErrorLevel
        # line 32 at [0000C790][0001:0000B790], len = 0x3 c:\home\edk-ii\mdepkg\library\basedebugprinterrorlevellib\basedebugprinterrorlevellib.c (MD5: 687C0AE564079D35D56ED5D84A6164CC)
        # line 36 at [0000C793][0001:0000B793], len = 0x5
        # line 37 at [0000C798][0001:0000B798], len = 0x2

        patchLineFileMatchString = "\s+line ([0-9]+) at \[([0-9a-fA-F]{8})\]\[[0-9a-fA-F]{4}\:[0-9a-fA-F]{8}\], len = 0x[0-9a-fA-F]+\s*([\w+\-\:./_a-zA-Z0-9\\\\]*)\s*"
        patchLineFileMatchStringFunc = "\*\*\s+(\w+)\s*"

        for reportLine in reportLines:
            match = re.match(patchLineFileMatchString, reportLine)
            if match is not None:
                if cmp (match.group(3), "") != 0 :
                    self.sourceName = match.group(3)
                sourceName = self.sourceName
                functionName = self.functionName

                rva = int (match.group(2), 16)
                lineName = int (match.group(1))
                self.listLineAddress.append ([rva, functionName, lineName, sourceName])
            else :
                match = re.match(patchLineFileMatchStringFunc, reportLine)
                if match is not None:
                    self.functionName = match.group(1)

        self.lineCount = len (self.listLineAddress)
        self.listLineAddress = sorted(self.listLineAddress, key=lambda symbolAddress:symbolAddress[0])

class SymbolsFile:
    def __init__(self):
        self.symbolsTable = {}

symbolsFile = ""

driverName = ""
rvaName = ""
symbolName = ""

def getSymbolName(driverName, rva):
    global symbolsFile

    try :
        symbolList = symbolsFile.symbolsTable[driverName]
        if symbolList is not None:
            return symbolList.getSymbol (rva)
        else:
            return []
    except Exception:
        return []

def myOptionParser():
    usage = "%prog [--version] [-h] [--help] [-i inputfile [-o outputfile] [-g guidreffile]]"
    Parser = OptionParser(usage=usage, description=__copyright__, version="%prog " + str(versionNumber))
    Parser.add_option("-i", "--inputfile", dest="inputfilename", type="string", help="The input memory profile info file output from MemoryProfileInfo application in MdeModulePkg")
    Parser.add_option("-o", "--outputfile", dest="outputfilename", type="string", help="The output memory profile info file with symbol, MemoryProfileInfoSymbol.txt will be used if it is not specified")
    Parser.add_option("-g", "--guidref", dest="guidreffilename", type="string", help="The input guid ref file output from build")

    (Options, args) = Parser.parse_args()
    if Options.inputfilename is None:
        Parser.error("no input file specified")
    if Options.outputfilename is None:
        Options.outputfilename = "SmiHandlerProfileInfoSymbol.xml"
    return Options

dictGuid = {
  '00000000-0000-0000-0000-000000000000':'gZeroGuid',
  '2A571201-4966-47F6-8B86-F31E41F32F10':'gEfiEventLegacyBootGuid',
  '27ABF055-B1B8-4C26-8048-748F37BAA2DF':'gEfiEventExitBootServicesGuid',
  '7CE88FB3-4BD7-4679-87A8-A8D8DEE50D2B':'gEfiEventReadyToBootGuid',
  '02CE967A-DD7E-4FFC-9EE7-810CF0470880':'gEfiEndOfDxeEventGroupGuid',
  '60FF8964-E906-41D0-AFED-F241E974E08E':'gEfiDxeSmmReadyToLockProtocolGuid',
  '18A3C6DC-5EEA-48C8-A1C1-B53389F98999':'gEfiSmmSwDispatch2ProtocolGuid',
  '456D2859-A84B-4E47-A2EE-3276D886997D':'gEfiSmmSxDispatch2ProtocolGuid',
  '4CEC368E-8E8E-4D71-8BE1-958C45FC8A53':'gEfiSmmPeriodicTimerDispatch2ProtocolGuid',
  'EE9B8D90-C5A6-40A2-BDE2-52558D33CCA1':'gEfiSmmUsbDispatch2ProtocolGuid',
  '25566B03-B577-4CBF-958C-ED663EA24380':'gEfiSmmGpiDispatch2ProtocolGuid',
  '7300C4A1-43F2-4017-A51B-C81A7F40585B':'gEfiSmmStandbyButtonDispatch2ProtocolGuid',
  '1B1183FA-1823-46A7-8872-9C578755409D':'gEfiSmmPowerButtonDispatch2ProtocolGuid',
  '58DC368D-7BFA-4E77-ABBC-0E29418DF930':'gEfiSmmIoTrapDispatch2ProtocolGuid',
  }

def genGuidString(guidreffile):
    guidLines = guidreffile.readlines()
    for guidLine in guidLines:
        guidLineList = guidLine.split(" ")
        if len(guidLineList) == 2:
            guid = guidLineList[0]
            guidName = guidLineList[1]
            if guid not in dictGuid :
                dictGuid[guid] = guidName

def createSym(symbolName):
    SymbolNode = xml.dom.minidom.Document().createElement("Symbol")
    SymbolFunction = xml.dom.minidom.Document().createElement("Function")
    SymbolFunctionData = xml.dom.minidom.Document().createTextNode(symbolName[0])
    SymbolFunction.appendChild(SymbolFunctionData)
    SymbolNode.appendChild(SymbolFunction)
    if (len(symbolName)) >= 2:
        SymbolSourceFile = xml.dom.minidom.Document().createElement("SourceFile")
        SymbolSourceFileData = xml.dom.minidom.Document().createTextNode(symbolName[1])
        SymbolSourceFile.appendChild(SymbolSourceFileData)
        SymbolNode.appendChild(SymbolSourceFile)
        if (len(symbolName)) >= 3:
            SymbolLineNumber = xml.dom.minidom.Document().createElement("LineNumber")
            SymbolLineNumberData = xml.dom.minidom.Document().createTextNode(str(symbolName[2]))
            SymbolLineNumber.appendChild(SymbolLineNumberData)
            SymbolNode.appendChild(SymbolLineNumber)
    return SymbolNode

def main():
    global symbolsFile
    global Options
    Options = myOptionParser()

    symbolsFile = SymbolsFile()

    try :
        DOMTree = xml.dom.minidom.parse(Options.inputfilename)
    except Exception:
        print("fail to open input " + Options.inputfilename)
        return 1

    if Options.guidreffilename is not None:
        try :
            guidreffile = open(Options.guidreffilename)
        except Exception:
            print("fail to open guidref" + Options.guidreffilename)
            return 1
        genGuidString(guidreffile)
        guidreffile.close()

    SmiHandlerProfile = DOMTree.documentElement

    SmiHandlerDatabase = SmiHandlerProfile.getElementsByTagName("SmiHandlerDatabase")
    SmiHandlerCategory = SmiHandlerDatabase[0].getElementsByTagName("SmiHandlerCategory")
    for smiHandlerCategory in SmiHandlerCategory:
        SmiEntry = smiHandlerCategory.getElementsByTagName("SmiEntry")
        for smiEntry in SmiEntry:
            if smiEntry.hasAttribute("HandlerType"):
                guidValue = smiEntry.getAttribute("HandlerType")
                if guidValue in dictGuid:
                    smiEntry.setAttribute("HandlerType", dictGuid[guidValue])
            SmiHandler = smiEntry.getElementsByTagName("SmiHandler")
            for smiHandler in SmiHandler:
                Module = smiHandler.getElementsByTagName("Module")
                Pdb = Module[0].getElementsByTagName("Pdb")
                if (len(Pdb)) >= 1:
                    driverName = Module[0].getAttribute("Name")
                    pdbName = Pdb[0].childNodes[0].data

                    Module[0].removeChild(Pdb[0])

                    symbolsFile.symbolsTable[driverName] = Symbols()

                    if cmp (pdbName[-3:], "pdb") == 0 :
                        symbolsFile.symbolsTable[driverName].parse_pdb_file (driverName, pdbName)
                    else :
                        symbolsFile.symbolsTable[driverName].parse_debug_file (driverName, pdbName)

                    Handler = smiHandler.getElementsByTagName("Handler")
                    RVA = Handler[0].getElementsByTagName("RVA")
                    print("    Handler RVA: %s" % RVA[0].childNodes[0].data)

                    if (len(RVA)) >= 1:
                        rvaName = RVA[0].childNodes[0].data
                        symbolName = getSymbolName (driverName, int(rvaName, 16))

                        if (len(symbolName)) >= 1:
                            SymbolNode = createSym(symbolName)
                            Handler[0].appendChild(SymbolNode)

                    Caller = smiHandler.getElementsByTagName("Caller")
                    RVA = Caller[0].getElementsByTagName("RVA")
                    print("    Caller RVA: %s" % RVA[0].childNodes[0].data)

                    if (len(RVA)) >= 1:
                        rvaName = RVA[0].childNodes[0].data
                        symbolName = getSymbolName (driverName, int(rvaName, 16))

                        if (len(symbolName)) >= 1:
                            SymbolNode = createSym(symbolName)
                            Caller[0].appendChild(SymbolNode)

    try :
        newfile = open(Options.outputfilename, "w")
    except Exception:
        print("fail to open output" + Options.outputfilename)
        return 1

    newfile.write(DOMTree.toprettyxml(indent = "\t", newl = "\n", encoding = "utf-8"))
    newfile.close()

if __name__ == '__main__':
    sys.exit(main())
