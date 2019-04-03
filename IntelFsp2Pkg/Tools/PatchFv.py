## @ PatchFv.py
#
# Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

import os
import re
import sys

#
#  Read data from file
#
#  param [in]  binfile     Binary file
#  param [in]  offset      Offset
#  param [in]  len         Length
#
#  retval      value       Value
#
def readDataFromFile (binfile, offset, len=1):
    fd     = open(binfile, "r+b")
    fsize  = os.path.getsize(binfile)
    offval = offset & 0xFFFFFFFF
    if (offval & 0x80000000):
        offval = fsize - (0xFFFFFFFF - offval + 1)
    fd.seek(offval)
    bytearray = [ord(b) for b in fd.read(len)]
    value = 0
    idx   = len - 1
    while  idx >= 0:
        value = value << 8 | bytearray[idx]
        idx = idx - 1
    fd.close()
    return value

#
#  Check FSP header is valid or not
#
#  param [in]  binfile     Binary file
#
#  retval      boolean     True: valid; False: invalid
#
def IsFspHeaderValid (binfile):
    fd     = open (binfile, "rb")
    bindat = fd.read(0x200) # only read first 0x200 bytes
    fd.close()
    HeaderList = ['FSPH' , 'FSPP' , 'FSPE']       # Check 'FSPH', 'FSPP', and 'FSPE' in the FSP header
    OffsetList = []
    for each in HeaderList:
        if each in bindat:
            idx = bindat.index(each)
        else:
            idx = 0
        OffsetList.append(idx)
    if not OffsetList[0] or not OffsetList[1]:    # If 'FSPH' or 'FSPP' is missing, it will return false
        return False
    Revision = ord(bindat[OffsetList[0] + 0x0B])
    #
    # if revision is bigger than 1, it means it is FSP v1.1 or greater revision, which must contain 'FSPE'.
    #
    if Revision > 1 and not OffsetList[2]:
        return False                              # If FSP v1.1 or greater without 'FSPE', then return false
    return True

#
#  Patch data in file
#
#  param [in]  binfile     Binary file
#  param [in]  offset      Offset
#  param [in]  value       Patch value
#  param [in]  len         Length
#
#  retval      len         Length
#
def patchDataInFile (binfile, offset, value, len=1):
    fd     = open(binfile, "r+b")
    fsize  = os.path.getsize(binfile)
    offval = offset & 0xFFFFFFFF
    if (offval & 0x80000000):
        offval = fsize - (0xFFFFFFFF - offval + 1)
    bytearray = []
    idx = 0
    while  idx < len:
        bytearray.append(value & 0xFF)
        value          = value >> 8
        idx            = idx + 1
    fd.seek(offval)
    fd.write("".join(chr(b) for b in bytearray))
    fd.close()
    return len


class Symbols:
    def __init__(self):
        self.dictSymbolAddress = {}
        self.dictGuidNameXref  = {}
        self.dictFfsOffset     = {}
        self.dictVariable      = {}
        self.dictModBase       = {}
        self.fdFile            = None
        self.string            = ""
        self.fdBase            = 0xFFFFFFFF
        self.fdSize            = 0
        self.index             = 0
        self.fvList            = []
        self.parenthesisOpenSet   =  '([{<'
        self.parenthesisCloseSet  =  ')]}>'

    #
    #  Get FD file
    #
    #  retval      self.fdFile Retrieve FD file
    #
    def getFdFile (self):
        return self.fdFile

    #
    #  Get FD size
    #
    #  retval      self.fdSize Retrieve the size of FD file
    #
    def getFdSize (self):
        return self.fdSize

    def parseFvInfFile (self, infFile):
        fvInfo = {}
        fvFile            = infFile[0:-4] + ".Fv"
        fvInfo['Name']    = os.path.splitext(os.path.basename(infFile))[0]
        fvInfo['Offset']  = self.getFvOffsetInFd(fvFile)
        fvInfo['Size']    = readDataFromFile (fvFile, 0x20, 4)
        fdIn        = open(infFile, "r")
        rptLines    = fdIn.readlines()
        fdIn.close()
        fvInfo['Base'] = 0
        for rptLine in rptLines:
            match = re.match("^EFI_BASE_ADDRESS\s*=\s*(0x[a-fA-F0-9]+)", rptLine)
            if match:
                fvInfo['Base'] = int(match.group(1), 16)
                break
        self.fvList.append(dict(fvInfo))
        return 0

    #
    #  Create dictionaries
    #
    #  param [in]  fvDir       FV's directory
    #  param [in]  fvNames     All FV's names
    #
    #  retval      0           Created dictionaries successfully
    #
    def createDicts (self, fvDir, fvNames):
        #
        # If the fvDir is not a dirctory, then raise an exception
        #
        if not os.path.isdir(fvDir):
            raise Exception ("'%s' is not a valid directory!" % FvDir)

        #
        # If the Guid.xref is not existing in fvDir, then raise an exception
        #
        xrefFile = os.path.join(fvDir, "Guid.xref")
        if not os.path.exists(xrefFile):
            raise Exception("Cannot open GUID Xref file '%s'!" % xrefFile)

        #
        # Add GUID reference to dictionary
        #
        self.dictGuidNameXref  = {}
        self.parseGuidXrefFile(xrefFile)

        #
        # Split up each FV from fvNames and get the fdBase
        #
        fvList = fvNames.split(":")
        fdBase = fvList.pop()
        if len(fvList) == 0:
            fvList.append(fdBase)

        #
        # If the FD file is not existing, then raise an exception
        #
        fdFile =  os.path.join(fvDir, fdBase.strip() + ".fd")
        if not os.path.exists(fdFile):
            raise Exception("Cannot open FD file '%s'!" % fdFile)

        #
        # Get the size of the FD file
        #
        self.fdFile = fdFile
        self.fdSize = os.path.getsize(fdFile)

        #
        # If the INF file, which is the first element of fvList, is not existing, then raise an exception
        #
        infFile = os.path.join(fvDir, fvList[0].strip()) + ".inf"
        if not os.path.exists(infFile):
            raise Exception("Cannot open INF file '%s'!" % infFile)

        #
        # Parse INF file in order to get fdBase and then assign those values to dictVariable
        #
        self.parseInfFile(infFile)
        self.dictVariable = {}
        self.dictVariable["FDSIZE"] =  self.fdSize
        self.dictVariable["FDBASE"] =  self.fdBase

        #
        # Collect information from FV MAP file and FV TXT file then
        # put them into dictionaries
        #
        self.fvList = []
        self.dictSymbolAddress = {}
        self.dictFfsOffset     = {}
        for file in fvList:

            #
            # If the .Fv.map file is not existing, then raise an exception.
            # Otherwise, parse FV MAP file
            #
            fvFile  = os.path.join(fvDir, file.strip()) + ".Fv"
            mapFile = fvFile + ".map"
            if not os.path.exists(mapFile):
                raise Exception("Cannot open MAP file '%s'!" % mapFile)

            infFile  = fvFile[0:-3] + ".inf"
            self.parseFvInfFile(infFile)
            self.parseFvMapFile(mapFile)

            #
            # If the .Fv.txt file is not existing, then raise an exception.
            # Otherwise, parse FV TXT file
            #
            fvTxtFile  = fvFile + ".txt"
            if not os.path.exists(fvTxtFile):
                raise Exception("Cannot open FV TXT file '%s'!" % fvTxtFile)

            self.parseFvTxtFile(fvTxtFile)

        for fv in self.fvList:
            self.dictVariable['_BASE_%s_' % fv['Name']] = fv['Base']
        #
        # Search all MAP files in FFS directory if it exists then parse MOD MAP file
        #
        ffsDir = os.path.join(fvDir, "Ffs")
        if (os.path.isdir(ffsDir)):
            for item in os.listdir(ffsDir):
                if len(item) <= 0x24:
                    continue
                mapFile =os.path.join(ffsDir, item, "%s.map" % item[0:0x24])
                if not os.path.exists(mapFile):
                    continue
                self.parseModMapFile(item[0x24:], mapFile)

        return 0

    #
    #  Get FV offset in FD file
    #
    #  param [in]  fvFile      FV file
    #
    #  retval      offset      Got FV offset successfully
    #
    def getFvOffsetInFd(self, fvFile):
        #
        # Check if the first 0x70 bytes of fvFile can be found in fdFile
        #
        fvHandle = open(fvFile, "r+b")
        fdHandle = open(self.fdFile, "r+b")
        offset = fdHandle.read().find(fvHandle.read(0x70))
        fvHandle.close()
        fdHandle.close()
        if offset == -1:
            raise Exception("Could not locate FV file %s in FD!" % fvFile)
        return offset

    #
    #  Parse INF file
    #
    #  param [in]  infFile     INF file
    #
    #  retval      0           Parsed INF file successfully
    #
    def parseInfFile(self, infFile):
        #
        # Get FV offset and search EFI_BASE_ADDRESS in the FD file
        # then assign the value of EFI_BASE_ADDRESS to fdBase
        #
        fvOffset    = self.getFvOffsetInFd(infFile[0:-4] + ".Fv")
        fdIn        = open(infFile, "r")
        rptLine     = fdIn.readline()
        self.fdBase = 0xFFFFFFFF
        while (rptLine != "" ):
            #EFI_BASE_ADDRESS = 0xFFFDF400
            match = re.match("^EFI_BASE_ADDRESS\s*=\s*(0x[a-fA-F0-9]+)", rptLine)
            if match is not None:
                self.fdBase = int(match.group(1), 16) - fvOffset
            rptLine  = fdIn.readline()
        fdIn.close()
        if self.fdBase == 0xFFFFFFFF:
            raise Exception("Could not find EFI_BASE_ADDRESS in INF file!" % fvFile)
        return 0

    #
    #  Parse FV TXT file
    #
    #  param [in]  fvTxtFile   .Fv.txt file
    #
    #  retval      0           Parsed FV TXT file successfully
    #
    def parseFvTxtFile(self, fvTxtFile):
        fvName   = os.path.basename(fvTxtFile)[0:-7].upper()
        #
        # Get information from .Fv.txt in order to create a dictionary
        # For example,
        # self.dictFfsOffset[912740BE-2284-4734-B971-84B027353F0C] = 0x000D4078
        #
        fvOffset = self.getFvOffsetInFd(fvTxtFile[0:-4])
        fdIn     = open(fvTxtFile, "r")
        rptLine  = fdIn.readline()
        while (rptLine != "" ):
            match = re.match("(0x[a-fA-F0-9]+)\s([0-9a-fA-F\-]+)", rptLine)
            if match is not None:
                if match.group(2) in self.dictFfsOffset:
                    self.dictFfsOffset[fvName + ':' + match.group(2)] = "0x%08X" % (int(match.group(1), 16) + fvOffset)
                else:
                    self.dictFfsOffset[match.group(2)] = "0x%08X" % (int(match.group(1), 16) + fvOffset)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    #
    #  Parse FV MAP file
    #
    #  param [in]  mapFile     .Fv.map file
    #
    #  retval      0           Parsed FV MAP file successfully
    #
    def parseFvMapFile(self, mapFile):
        #
        # Get information from .Fv.map in order to create dictionaries
        # For example,
        # self.dictModBase[FspSecCore:BASE]  = 4294592776 (0xfffa4908)
        # self.dictModBase[FspSecCore:ENTRY] = 4294606552 (0xfffa7ed8)
        # self.dictModBase[FspSecCore:TEXT]  = 4294593080 (0xfffa4a38)
        # self.dictModBase[FspSecCore:DATA]  = 4294612280 (0xfffa9538)
        # self.dictSymbolAddress[FspSecCore:_SecStartup] = 0x00fffa4a38
        #
        fdIn     = open(mapFile, "r")
        rptLine  = fdIn.readline()
        modName  = ""
        foundModHdr = False
        while (rptLine != "" ):
            if rptLine[0] != ' ':
                #DxeIpl (Fixed Flash Address, BaseAddress=0x00fffb4310, EntryPoint=0x00fffb4958)
                #(GUID=86D70125-BAA3-4296-A62F-602BEBBB9081 .textbaseaddress=0x00fffb4398 .databaseaddress=0x00fffb4178)
                match = re.match("([_a-zA-Z0-9\-]+)\s\(.+BaseAddress=(0x[0-9a-fA-F]+),\s+EntryPoint=(0x[0-9a-fA-F]+)\)", rptLine)
                if match is not None:
                    foundModHdr = True
                    modName = match.group(1)
                    if len(modName) == 36:
                       modName = self.dictGuidNameXref[modName.upper()]
                    self.dictModBase['%s:BASE'  % modName] = int (match.group(2), 16)
                    self.dictModBase['%s:ENTRY' % modName] = int (match.group(3), 16)
                match = re.match("\(GUID=([A-Z0-9\-]+)\s+\.textbaseaddress=(0x[0-9a-fA-F]+)\s+\.databaseaddress=(0x[0-9a-fA-F]+)\)", rptLine)
                if match is not None:
                    if foundModHdr:
                        foundModHdr = False
                    else:
                        modName = match.group(1)
                        if len(modName) == 36:
                            modName = self.dictGuidNameXref[modName.upper()]
                    self.dictModBase['%s:TEXT' % modName] = int (match.group(2), 16)
                    self.dictModBase['%s:DATA' % modName] = int (match.group(3), 16)
            else:
                #   0x00fff8016c    __ModuleEntryPoint
                foundModHdr = False
                match = re.match("^\s+(0x[a-z0-9]+)\s+([_a-zA-Z0-9]+)", rptLine)
                if match is not None:
                    self.dictSymbolAddress["%s:%s"%(modName, match.group(2))] = match.group(1)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    #
    #  Parse MOD MAP file
    #
    #  param [in]  moduleName  Module name
    #  param [in]  mapFile     .Fv.map file
    #
    #  retval      0           Parsed MOD MAP file successfully
    #  retval      1           There is no moduleEntryPoint in modSymbols
    #
    def parseModMapFile(self, moduleName, mapFile):
        #
        # Get information from mapFile by moduleName in order to create a dictionary
        # For example,
        # self.dictSymbolAddress[FspSecCore:___guard_fids_count] = 0x00fffa4778
        #
        modSymbols  = {}
        fdIn        = open(mapFile, "r")
        reportLines = fdIn.readlines()
        fdIn.close()

        moduleEntryPoint = "__ModuleEntryPoint"
        reportLine = reportLines[0]
        if reportLine.strip().find("Archive member included") != -1:
            #GCC
            #                0x0000000000001d55                IoRead8
            patchMapFileMatchString = "\s+(0x[0-9a-fA-F]{16})\s+([^\s][^0x][_a-zA-Z0-9\-]+)\s"
            matchKeyGroupIndex = 2
            matchSymbolGroupIndex  = 1
            prefix = '_'
        else:
            #MSFT
            #0003:00000190       _gComBase                  00007a50     SerialPo
            patchMapFileMatchString =  "^\s[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\w+)\s+([0-9a-fA-F]{8}\s+)"
            matchKeyGroupIndex = 1
            matchSymbolGroupIndex  = 2
            prefix = ''

        for reportLine in reportLines:
            match = re.match(patchMapFileMatchString, reportLine)
            if match is not None:
                modSymbols[prefix + match.group(matchKeyGroupIndex)] = match.group(matchSymbolGroupIndex)

        # Handle extra module patchable PCD variable in Linux map since it might have different format
        # .data._gPcd_BinaryPatch_PcdVpdBaseAddress
        #        0x0000000000003714        0x4 /tmp/ccmytayk.ltrans1.ltrans.o
        handleNext = False
        if matchSymbolGroupIndex == 1:
            for reportLine in reportLines:
                if handleNext:
                    handleNext = False
                    pcdName = match.group(1)
                    match   = re.match("\s+(0x[0-9a-fA-F]{16})\s+", reportLine)
                    if match is not None:
                        modSymbols[prefix + pcdName] = match.group(1)
                else:
                    match = re.match("^\s\.data\.(_gPcd_BinaryPatch[_a-zA-Z0-9\-]+)", reportLine)
                    if match is not None:
                        handleNext = True
                        continue

        if not moduleEntryPoint in modSymbols:
            return 1

        modEntry = '%s:%s' % (moduleName,moduleEntryPoint)
        if not modEntry in self.dictSymbolAddress:
            modKey = '%s:ENTRY' % moduleName
            if modKey in self.dictModBase:
                baseOffset = self.dictModBase['%s:ENTRY' % moduleName] - int(modSymbols[moduleEntryPoint], 16)
            else:
               return 2
        else:
            baseOffset = int(self.dictSymbolAddress[modEntry], 16) - int(modSymbols[moduleEntryPoint], 16)
        for symbol in modSymbols:
            fullSym = "%s:%s" % (moduleName, symbol)
            if not fullSym in self.dictSymbolAddress:
                self.dictSymbolAddress[fullSym] = "0x00%08x" % (baseOffset+ int(modSymbols[symbol], 16))
        return 0

    #
    #  Parse Guid.xref file
    #
    #  param [in]  xrefFile    the full directory of Guid.xref file
    #
    #  retval      0           Parsed Guid.xref file successfully
    #
    def parseGuidXrefFile(self, xrefFile):
        #
        # Get information from Guid.xref in order to create a GuidNameXref dictionary
        # The dictGuidNameXref, for example, will be like
        # dictGuidNameXref [1BA0062E-C779-4582-8566-336AE8F78F09] = FspSecCore
        #
        fdIn     = open(xrefFile, "r")
        rptLine  = fdIn.readline()
        while (rptLine != "" ):
            match = re.match("([0-9a-fA-F\-]+)\s([_a-zA-Z0-9]+)", rptLine)
            if match is not None:
                self.dictGuidNameXref[match.group(1).upper()] = match.group(2)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    #
    #  Get current character
    #
    #  retval      elf.string[self.index]
    #  retval      ''                       Exception
    #
    def getCurr(self):
        try:
            return self.string[self.index]
        except Exception:
            return ''

    #
    #  Check to see if it is last index
    #
    #  retval      self.index
    #
    def isLast(self):
        return self.index == len(self.string)

    #
    #  Move to next index
    #
    def moveNext(self):
        self.index += 1

    #
    #  Skip space
    #
    def skipSpace(self):
        while not self.isLast():
            if self.getCurr() in ' \t':
                self.moveNext()
            else:
                return

    #
    #  Parse value
    #
    #  retval      value
    #
    def parseValue(self):
        self.skipSpace()
        var = ''
        while not self.isLast():
            char = self.getCurr()
            if char.lower() in '_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789:-':
                var += char
                self.moveNext()
            else:
                break

        if ':' in var:
            partList = var.split(':')
            lenList  = len(partList)
            if lenList != 2 and lenList != 3:
                raise Exception("Unrecognized expression %s" % var)
            modName = partList[lenList-2]
            modOff  = partList[lenList-1]
            if ('-' not in  modName) and (modOff[0] in '0123456789'):
                # MOD: OFFSET
                var = self.getModGuid(modName) + ":" + modOff
            if '-' in var:  # GUID:OFFSET
                value = self.getGuidOff(var)
            else:
                value = self.getSymbols(var)
                self.synUsed   = True
        else:
            if var[0] in '0123456789':
                value = self.getNumber(var)
            else:
                value = self.getVariable(var)
        return int(value)

    #
    #  Parse single operation
    #
    #  retval      ~self.parseBrace() or self.parseValue()
    #
    def parseSingleOp(self):
        self.skipSpace()
        char = self.getCurr()
        if char == '~':
            self.moveNext()
            return ~self.parseBrace()
        else:
            return self.parseValue()

    #
    #  Parse symbol of Brace([, {, <)
    #
    #  retval      value or self.parseSingleOp()
    #
    def parseBrace(self):
        self.skipSpace()
        char = self.getCurr()
        parenthesisType = self.parenthesisOpenSet.find(char)
        if parenthesisType >= 0:
            self.moveNext()
            value = self.parseExpr()
            self.skipSpace()
            if self.getCurr() != self.parenthesisCloseSet[parenthesisType]:
                raise Exception("No closing brace")
            self.moveNext()
            if parenthesisType   == 1:  # [ : Get content
                value = self.getContent(value)
            elif parenthesisType == 2:  # { : To  address
                value = self.toAddress(value)
            elif parenthesisType == 3:  # < : To  offset
                value = self.toOffset(value)
            return value
        else:
            return self.parseSingleOp()

    #
    #  Parse symbol of Multiplier(*)
    #
    #  retval      value or self.parseSingleOp()
    #
    def parseMul(self):
        values = [self.parseBrace()]
        while True:
            self.skipSpace()
            char = self.getCurr()
            if char == '*':
                self.moveNext()
                values.append(self.parseBrace())
            else:
                break
        value  = 1
        for each in values:
            value *= each
        return value

    #
    #  Parse symbol of And(&) and Or(|)
    #
    #  retval      value
    #
    def parseAndOr(self):
        value  = self.parseMul()
        op     = None
        while True:
            self.skipSpace()
            char = self.getCurr()
            if char == '&':
                self.moveNext()
                value &= self.parseMul()
            elif char == '|':
                div_index = self.index
                self.moveNext()
                value |= self.parseMul()
            else:
                break

        return value

    #
    #  Parse symbol of Add(+) and Minus(-)
    #
    #  retval      sum(values)
    #
    def parseAddMinus(self):
        values = [self.parseAndOr()]
        while True:
            self.skipSpace()
            char = self.getCurr()
            if char == '+':
                self.moveNext()
                values.append(self.parseAndOr())
            elif char == '-':
                self.moveNext()
                values.append(-1 * self.parseAndOr())
            else:
                break
        return sum(values)

    #
    #  Parse expression
    #
    #  retval      self.parseAddMinus()
    #
    def parseExpr(self):
        return self.parseAddMinus()

    #
    #  Get result
    #
    #  retval      value
    #
    def getResult(self):
        value = self.parseExpr()
        self.skipSpace()
        if not self.isLast():
            raise Exception("Unexpected character found '%s'" % self.getCurr())
        return value

    #
    #  Get module GUID
    #
    #  retval      value
    #
    def getModGuid(self, var):
        guid = (guid for guid,name in self.dictGuidNameXref.items() if name==var)
        try:
            value = guid.next()
        except Exception:
            raise Exception("Unknown module name %s !" % var)
        return value

    #
    #  Get variable
    #
    #  retval      value
    #
    def getVariable(self, var):
        value = self.dictVariable.get(var, None)
        if value == None:
            raise Exception("Unrecognized variable '%s'" % var)
        return value

    #
    #  Get number
    #
    #  retval      value
    #
    def getNumber(self, var):
        var = var.strip()
        if var.startswith('0x'):  # HEX
            value = int(var, 16)
        else:
            value = int(var, 10)
        return value

    #
    #  Get content
    #
    #  param [in]  value
    #
    #  retval      value
    #
    def getContent(self, value):
        return readDataFromFile (self.fdFile, self.toOffset(value), 4)

    #
    #  Change value to address
    #
    #  param [in]  value
    #
    #  retval      value
    #
    def toAddress(self, value):
        if value < self.fdSize:
            value = value + self.fdBase
        return value

    #
    #  Change value to offset
    #
    #  param [in]  value
    #
    #  retval      value
    #
    def toOffset(self, value):
        offset = None
        for fvInfo in self.fvList:
            if (value >= fvInfo['Base']) and (value < fvInfo['Base'] + fvInfo['Size']):
                offset = value - fvInfo['Base'] + fvInfo['Offset']
        if not offset:
            if (value >= self.fdBase) and (value < self.fdBase + self.fdSize):
                offset = value - self.fdBase
            else:
                offset = value
        if offset >= self.fdSize:
            raise Exception("Invalid file offset 0x%08x !" % value)
        return offset

    #
    #  Get GUID offset
    #
    #  param [in]  value
    #
    #  retval      value
    #
    def getGuidOff(self, value):
        # GUID:Offset
        symbolName = value.split(':')
        if len(symbolName) == 3:
            fvName  = symbolName[0].upper()
            keyName = '%s:%s' % (fvName, symbolName[1])
            offStr  = symbolName[2]
        elif len(symbolName) == 2:
            keyName = symbolName[0]
            offStr  = symbolName[1]
        if keyName in self.dictFfsOffset:
            value = (int(self.dictFfsOffset[keyName], 16) + int(offStr, 16)) & 0xFFFFFFFF
        else:
            raise Exception("Unknown GUID %s !" % value)
        return value

    #
    #  Get symbols
    #
    #  param [in]  value
    #
    #  retval      ret
    #
    def getSymbols(self, value):
        if self.dictSymbolAddress.has_key(value):
            # Module:Function
            ret = int (self.dictSymbolAddress[value], 16)
        else:
            raise Exception("Unknown symbol %s !" % value)
        return ret

    #
    #  Evaluate symbols
    #
    #  param [in]  expression
    #  param [in]  isOffset
    #
    #  retval      value & 0xFFFFFFFF
    #
    def evaluate(self, expression, isOffset):
        self.index     = 0
        self.synUsed   = False
        self.string    = expression
        value = self.getResult()
        if isOffset:
            if self.synUsed:
                # Consider it as an address first
                value = self.toOffset(value)
            if value & 0x80000000:
                # Consider it as a negative offset next
                offset = (~value & 0xFFFFFFFF) + 1
                if offset < self.fdSize:
                    value = self.fdSize - offset
            if value >= self.fdSize:
                raise Exception("Invalid offset expression !")
        return value & 0xFFFFFFFF

#
#  Print out the usage
#
def usage():
    print "Usage: \n\tPatchFv FvBuildDir [FvFileBaseNames:]FdFileBaseNameToPatch \"Offset, Value\""

def main():
    #
    # Parse the options and args
    #
    symTables = Symbols()

    #
    # If the arguments are less than 4, then return an error.
    #
    if len(sys.argv) < 4:
        Usage()
        return 1

    #
    # If it fails to create dictionaries, then return an error.
    #
    if symTables.createDicts(sys.argv[1], sys.argv[2]) != 0:
        print "ERROR: Failed to create symbol dictionary!!"
        return 2

    #
    # Get FD file and size
    #
    fdFile = symTables.getFdFile()
    fdSize = symTables.getFdSize()

    try:
        #
        # Check to see if FSP header is valid
        #
        ret = IsFspHeaderValid(fdFile)
        if ret == False:
          raise Exception ("The FSP header is not valid. Stop patching FD.")
        comment = ""
        for fvFile in  sys.argv[3:]:
            #
            # Check to see if it has enough arguments
            #
            items = fvFile.split(",")
            if len (items) < 2:
                raise Exception("Expect more arguments for '%s'!" % fvFile)

            comment = ""
            command = ""
            params  = []
            for item in items:
                item = item.strip()
                if item.startswith("@"):
                    comment = item[1:]
                elif item.startswith("$"):
                    command = item[1:]
                else:
                    if len(params) == 0:
                        isOffset = True
                    else :
                        isOffset = False
                    #
                    # Parse symbols then append it to params
                    #
                    params.append (symTables.evaluate(item, isOffset))

            #
            # Patch a new value into FD file if it is not a command
            #
            if command == "":
                # Patch a DWORD
                if len (params) == 2:
                    offset   = params[0]
                    value    = params[1]
                    oldvalue = readDataFromFile(fdFile, offset, 4)
                    ret = patchDataInFile (fdFile, offset, value, 4) - 4
                else:
                    raise Exception ("Patch command needs 2 parameters !")

                if ret:
                    raise Exception ("Patch failed for offset 0x%08X" % offset)
                else:
                    print  "Patched offset 0x%08X:[%08X] with value 0x%08X  # %s" % (offset, oldvalue, value, comment)

            elif command == "COPY":
                #
                # Copy binary block from source to destination
                #
                if len (params) == 3:
                    src  = symTables.toOffset(params[0])
                    dest = symTables.toOffset(params[1])
                    clen = symTables.toOffset(params[2])
                    if (dest + clen <= fdSize) and (src + clen <= fdSize):
                        oldvalue = readDataFromFile(fdFile, src, clen)
                        ret = patchDataInFile (fdFile, dest, oldvalue, clen) - clen
                    else:
                        raise Exception ("Copy command OFFSET or LENGTH parameter is invalid !")
                else:
                    raise Exception ("Copy command needs 3 parameters !")

                if ret:
                    raise Exception ("Copy failed from offset 0x%08X to offset 0x%08X!" % (src, dest))
                else :
                    print  "Copied %d bytes from offset 0x%08X ~ offset 0x%08X  # %s" % (clen, src, dest, comment)
            else:
                raise Exception ("Unknown command %s!" % command)
        return 0

    except Exception as (ex):
        print "ERROR: %s" % ex
        return 1

if __name__ == '__main__':
    sys.exit(main())
