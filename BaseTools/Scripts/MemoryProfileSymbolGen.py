##
# Generate symbal for memory profile info.
#
# This tool depends on DIA2Dump.exe (VS) or nm (gcc) to parse debug entry.
#
# Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

from __future__ import print_function
import os
import re
import sys
from optparse import OptionParser

versionNumber = "1.1"
__copyright__ = "Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved."

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
                  return " (" + self.listLineAddress[index][1] + "() - " + ")"
                else :
                  return " (" + self.listLineAddress[index][1] + "() - " + sourceName + ":" + str(lineName) + ")"
            index += 1

        return " (unknown)"

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

        # 000113ca T AllocatePool  c:\home\edk-ii\MdePkg\Library\UefiMemoryAllocationLib\MemoryAllocationLib.c:399
        patchLineFileMatchString = "([0-9a-fA-F]*)\s+[T|D|t|d]\s+(\w+)\s*((?:[a-zA-Z]:)?[\w+\-./_a-zA-Z0-9\\\\]*):?([0-9]*)"

        for reportLine in reportLines:
            #print "check - " + reportLine
            match = re.match(patchLineFileMatchString, reportLine)
            if match is not None:
                #print "match - " + reportLine[:-1]
                #print "0 - " + match.group(0)
                #print "1 - " + match.group(1)
                #print "2 - " + match.group(2)
                #print "3 - " + match.group(3)
                #print "4 - " + match.group(4)

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

        #for key in self.listLineAddress :
            #print "rva - " + "%x"%(key[0]) + ", func - " + key[1] + ", line - " + str(key[2]) + ", source - " + key[3]

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
        #  line 32 at [0000C790][0001:0000B790], len = 0x3  c:\home\edk-ii\mdepkg\library\basedebugprinterrorlevellib\basedebugprinterrorlevellib.c (MD5: 687C0AE564079D35D56ED5D84A6164CC)
        #  line 36 at [0000C793][0001:0000B793], len = 0x5
        #  line 37 at [0000C798][0001:0000B798], len = 0x2

        patchLineFileMatchString = "\s+line ([0-9]+) at \[([0-9a-fA-F]{8})\]\[[0-9a-fA-F]{4}\:[0-9a-fA-F]{8}\], len = 0x[0-9a-fA-F]+\s*([\w+\-\:./_a-zA-Z0-9\\\\]*)\s*"
        patchLineFileMatchStringFunc = "\*\*\s+(\w+)\s*"

        for reportLine in reportLines:
            #print "check line - " + reportLine
            match = re.match(patchLineFileMatchString, reportLine)
            if match is not None:
                #print "match - " + reportLine[:-1]
                #print "0 - " + match.group(0)
                #print "1 - " + match.group(1)
                #print "2 - " + match.group(2)
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

        #for key in self.listLineAddress :
            #print "rva - " + "%x"%(key[0]) + ", func - " + key[1] + ", line - " + str(key[2]) + ", source - " + key[3]

class SymbolsFile:
    def __init__(self):
        self.symbolsTable = {}

symbolsFile = ""

driverName = ""
rvaName = ""
symbolName = ""

def getSymbolName(driverName, rva):
    global symbolsFile

    #print "driverName - " + driverName

    try :
        symbolList = symbolsFile.symbolsTable[driverName]
        if symbolList is not None:
            return symbolList.getSymbol (rva)
        else:
            return " (???)"
    except Exception:
        return " (???)"

def processLine(newline):
    global driverName
    global rvaName

    driverPrefixLen = len("Driver - ")
    # get driver name
    if cmp(newline[0:driverPrefixLen], "Driver - ") == 0 :
        driverlineList = newline.split(" ")
        driverName = driverlineList[2]
        #print "Checking : ", driverName

        # EDKII application output
        pdbMatchString = "Driver - \w* \(Usage - 0x[0-9a-fA-F]+\) \(Pdb - ([:\-.\w\\\\/]*)\)\s*"
        pdbName = ""
        match = re.match(pdbMatchString, newline)
        if match is not None:
            #print "match - " + newline
            #print "0 - " + match.group(0)
            #print "1 - " + match.group(1)
            pdbName = match.group(1)
            #print "PDB - " + pdbName

        symbolsFile.symbolsTable[driverName] = Symbols()

        if cmp (pdbName[-3:], "pdb") == 0 :
            symbolsFile.symbolsTable[driverName].parse_pdb_file (driverName, pdbName)
        else :
            symbolsFile.symbolsTable[driverName].parse_debug_file (driverName, pdbName)

    elif cmp(newline, "") == 0 :
        driverName = ""

    # check entry line
    if newline.find ("<==") != -1 :
        entry_list = newline.split(" ")
        rvaName = entry_list[4]
        #print "rva : ", rvaName
        symbolName = getSymbolName (driverName, int(rvaName, 16))
    else :
        rvaName = ""
        symbolName = ""

    if cmp(rvaName, "") == 0 :
        return newline
    else :
        return newline + symbolName

def myOptionParser():
    usage = "%prog [--version] [-h] [--help] [-i inputfile [-o outputfile]]"
    Parser = OptionParser(usage=usage, description=__copyright__, version="%prog " + str(versionNumber))
    Parser.add_option("-i", "--inputfile", dest="inputfilename", type="string", help="The input memory profile info file output from MemoryProfileInfo application in MdeModulePkg")
    Parser.add_option("-o", "--outputfile", dest="outputfilename", type="string", help="The output memory profile info file with symbol, MemoryProfileInfoSymbol.txt will be used if it is not specified")

    (Options, args) = Parser.parse_args()
    if Options.inputfilename is None:
        Parser.error("no input file specified")
    if Options.outputfilename is None:
        Options.outputfilename = "MemoryProfileInfoSymbol.txt"
    return Options

def main():
    global symbolsFile
    global Options
    Options = myOptionParser()

    symbolsFile = SymbolsFile()

    try :
        file = open(Options.inputfilename)
    except Exception:
        print("fail to open " + Options.inputfilename)
        return 1
    try :
        newfile = open(Options.outputfilename, "w")
    except Exception:
        print("fail to open " + Options.outputfilename)
        return 1

    try:
        while True:
            line = file.readline()
            if not line:
                break
            newline = line[:-1]

            newline = processLine(newline)

            newfile.write(newline)
            newfile.write("\n")
    finally:
        file.close()
        newfile.close()

if __name__ == '__main__':
    sys.exit(main())
