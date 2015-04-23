## @ PatchFv.py
#
# Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
##

import os
import re
import sys

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

def IsFspHeaderValid (binfile):
    fd     = open (binfile, "rb")
    bindat = fd.read(0x200)
    fd.close()
    HeaderList = ['FSPH' , 'FSPP' , 'FSPE']
    OffsetList = []
    for each in HeaderList:
        if each in bindat:
            idx = bindat.index(each)
        else:
            idx = 0
        OffsetList.append(idx)
    if not OffsetList[0] or not OffsetList[1]:
        return False
    Revision = ord(bindat[OffsetList[0] + 0x0B])
    if Revision > 1 and not OffsetList[2]:
        return False
    return True

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
        self.parenthesisOpenSet   =  '([{<'
        self.parenthesisCloseSet  =  ')]}>'

    def getFdFile (self):
        return self.fdFile

    def getFdSize (self):
        return self.fdSize

    def createDicts (self, fvDir, fvNames):
        if not os.path.isdir(fvDir):
            raise Exception ("'%s' is not a valid directory!" % FvDir)

        xrefFile = os.path.join(fvDir, "Guid.xref")
        if not os.path.exists(xrefFile):
            raise Exception("Cannot open GUID Xref file '%s'!" % xrefFile)

        self.dictGuidNameXref  = {}
        self.parseGuidXrefFile(xrefFile)

        fvList = fvNames.split(":")
        fdBase = fvList.pop()
        if len(fvList) == 0:
            fvList.append(fdBase)

        fdFile =  os.path.join(fvDir, fdBase.strip() + ".fd")
        if not os.path.exists(fdFile):
            raise Exception("Cannot open FD file '%s'!" % fdFile)

        self.fdFile = fdFile
        self.fdSize = os.path.getsize(fdFile)

        infFile = os.path.join(fvDir, fvList[0].strip()) + ".inf"
        if not os.path.exists(infFile):
            raise Exception("Cannot open INF file '%s'!" % infFile)

        self.parseInfFile(infFile)

        self.dictVariable = {}
        self.dictVariable["FDSIZE"] =  self.fdSize
        self.dictVariable["FDBASE"] =  self.fdBase

        self.dictSymbolAddress = {}
        self.dictFfsOffset     = {}
        for file in fvList:

            fvFile  = os.path.join(fvDir, file.strip()) + ".Fv"
            mapFile = fvFile + ".map"
            if not os.path.exists(mapFile):
                raise Exception("Cannot open MAP file '%s'!" % mapFile)

            self.parseFvMapFile(mapFile)

            fvTxtFile  = fvFile + ".txt"
            if not os.path.exists(fvTxtFile):
                raise Exception("Cannot open FV TXT file '%s'!" % fvTxtFile)

            self.parseFvTxtFile(fvTxtFile)

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

    def getFvOffsetInFd(self, fvFile):
        fvHandle = open(fvFile, "r+b")
        fdHandle = open(self.fdFile, "r+b")
        offset = fdHandle.read().find(fvHandle.read(0x70))
        fvHandle.close()
        fdHandle.close()
        if offset == -1:
            raise Exception("Could not locate FV file %s in FD!" % fvFile)
        return offset

    def parseInfFile(self, infFile):
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

    def parseFvTxtFile(self, fvTxtFile):
        fvOffset = self.getFvOffsetInFd(fvTxtFile[0:-4])
        fdIn     = open(fvTxtFile, "r")
        rptLine  = fdIn.readline()
        while (rptLine != "" ):
            match = re.match("(0x[a-fA-F0-9]+)\s([0-9a-fA-F\-]+)", rptLine)
            if match is not None:
                self.dictFfsOffset[match.group(2)] = "0x%08X" % (int(match.group(1), 16) + fvOffset)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    def parseFvMapFile(self, mapFile):
        fdIn     = open(mapFile, "r")
        rptLine  = fdIn.readline()
        modName  = ""
        while (rptLine != "" ):
            if rptLine[0] != ' ':
                #DxeIpl (Fixed Flash Address, BaseAddress=0x00fffb4310, EntryPoint=0x00fffb4958)
                #(GUID=86D70125-BAA3-4296-A62F-602BEBBB9081 .textbaseaddress=0x00fffb4398 .databaseaddress=0x00fffb4178)
                match = re.match("([_a-zA-Z0-9\-]+)\s\(.+BaseAddress=(0x[0-9a-fA-F]+),\s+EntryPoint=(0x[0-9a-fA-F]+)\)", rptLine)
                if match is not None:
                    modName = match.group(1)
                    if len(modName) == 36:
                       modName = self.dictGuidNameXref[modName.upper()]
                    self.dictModBase['%s:BASE'  % modName] = int (match.group(2), 16)
                    self.dictModBase['%s:ENTRY' % modName] = int (match.group(3), 16)
                match = re.match("\(GUID=([A-Z0-9\-]+)\s+\.textbaseaddress=(0x[0-9a-fA-F]+)\s+\.databaseaddress=(0x[0-9a-fA-F]+)\)", rptLine)
                if match is not None:
                    modName = match.group(1)
                    if len(modName) == 36:
                       modName = self.dictGuidNameXref[modName.upper()]
                       self.dictModBase['%s:TEXT' % modName] = int (match.group(2), 16)
                       self.dictModBase['%s:DATA' % modName] = int (match.group(3), 16)
            else:
                #   0x00fff8016c    __ModuleEntryPoint
                match = re.match("^\s+(0x[a-z0-9]+)\s+([_a-zA-Z0-9]+)", rptLine)
                if match is not None:
                    self.dictSymbolAddress["%s:%s"%(modName, match.group(2))] = match.group(1)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    def parseModMapFile(self, moduleName, mapFile):
        modSymbols  = {}
        fdIn        = open(mapFile, "r")
        reportLine  = fdIn.readline()
        if reportLine.strip().find("Archive member included because of file (symbol)") != -1:
            #GCC
            #                0x0000000000001d55                IoRead8
            patchMapFileMatchString = "\s+(0x[0-9a-fA-F]{16})\s+([^\s][^0x][_a-zA-Z0-9\-]+)\s"
            matchKeyGroupIndex = 2
            matchSymbolGroupIndex  = 1
            moduleEntryPoint = "_ModuleEntryPoint"
        else:
            #MSFT
            #0003:00000190       _gComBase                  00007a50     SerialPo
            patchMapFileMatchString =  "^\s[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\w+)\s+([0-9a-fA-F]{8}\s+)"
            matchKeyGroupIndex = 1
            matchSymbolGroupIndex  = 2
            moduleEntryPoint = "__ModuleEntryPoint"
        while (reportLine != "" ):
            match = re.match(patchMapFileMatchString, reportLine)
            if match is not None:
                modSymbols[match.group(matchKeyGroupIndex)] = match.group(matchSymbolGroupIndex)
            reportLine  = fdIn.readline()
        fdIn.close()

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

    def parseGuidXrefFile(self, xrefFile):
        fdIn     = open(xrefFile, "r")
        rptLine  = fdIn.readline()
        while (rptLine != "" ):
            match = re.match("([0-9a-fA-F\-]+)\s([_a-zA-Z0-9]+)", rptLine)
            if match is not None:
                self.dictGuidNameXref[match.group(1).upper()] = match.group(2)
            rptLine  = fdIn.readline()
        fdIn.close()
        return 0

    def getCurr(self):
        try:
            return self.string[self.index]
        except Exception:
            return ''

    def isLast(self):
        return self.index == len(self.string)

    def moveNext(self):
        self.index += 1

    def skipSpace(self):
        while not self.isLast():
            if self.getCurr() in ' \t':
                self.moveNext()
            else:
                return

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
            if len(partList) != 2:
                raise Exception("Unrecognized expression %s" % var)
            modName = partList[0]
            modOff  = partList[1]
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

    def parseSingleOp(self):
        self.skipSpace()
        char = self.getCurr()
        if char == '~':
            self.moveNext()
            return ~self.parseBrace()
        else:
            return self.parseValue()

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

    def parseAndOr(self):
        values = [self.parseMul()]
        op     = None
        value  = 0xFFFFFFFF
        while True:
            self.skipSpace()
            char = self.getCurr()
            if char == '&':
                self.moveNext()
                values.append(self.parseMul())
                op = char
            elif char == '|':
                div_index = self.index
                self.moveNext()
                values.append(self.parseMul())
                value = 0
                op = char
            else:
                break

        for each in values:
            if op == '|':
                value |= each
            else:
                value &= each

        return value

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

    def parseExpr(self):
        return self.parseAddMinus()

    def getResult(self):
        value = self.parseExpr()
        self.skipSpace()
        if not self.isLast():
            raise Exception("Unexpected character found '%s'" % self.getCurr())
        return value

    def getModGuid(self, var):
        guid = (guid for guid,name in self.dictGuidNameXref.items() if name==var)
        try:
            value = guid.next()
        except Exception:
            raise Exception("Unknown module name %s !" % var)
        return value

    def getVariable(self, var):
        value = self.dictVariable.get(var, None)
        if value == None:
            raise Exception("Unrecognized variable '%s'" % var)
        return value

    def getNumber(self, var):
        var = var.strip()
        if var.startswith('0x'):  # HEX
            value = int(var, 16)
        else:
            value = int(var, 10)
        return value

    def getContent(self, value):
        if (value >= self.fdBase) and (value < self.fdBase + self.fdSize):
            value = value - self.fdBase
        if value >= self.fdSize:
            raise Exception("Invalid file offset 0x%08x !" % value)
        return readDataFromFile (self.fdFile, value, 4)

    def toAddress(self, value):
        if value < self.fdSize:
            value = value + self.fdBase
        return value

    def toOffset(self, value):
        if value > self.fdBase:
            value = value - self.fdBase
        return value

    def getGuidOff(self, value):
        # GUID:Offset
        symbolName = value.split(':')
        if len(symbolName) == 2 and self.dictFfsOffset.has_key(symbolName[0]):
            value = (int(self.dictFfsOffset[symbolName[0]], 16) + int(symbolName[1], 16)) & 0xFFFFFFFF
        else:
            raise Exception("Unknown GUID %s !" % value)
        return value

    def getSymbols(self, value):
        if self.dictSymbolAddress.has_key(value):
            # Module:Function
            ret = int (self.dictSymbolAddress[value], 16)
        else:
            raise Exception("Unknown symbol %s !" % value)
        return ret

    def evaluate(self, expression, isOffset):
        self.index     = 0
        self.synUsed   = False
        self.string    = expression
        value = self.getResult()
        if isOffset:
            if self.synUsed:
                # Consider it as an address first
                if (value >= self.fdBase) and (value < self.fdBase + self.fdSize):
                    value = value - self.fdBase
            if value & 0x80000000:
                # Consider it as a negative offset next
                offset = (~value & 0xFFFFFFFF) + 1
                if offset < self.fdSize:
                    value = self.fdSize - offset
            if value >= self.fdSize:
                raise Exception("Invalid offset expression !")
        return value & 0xFFFFFFFF

def usage():
    print "Usage: \n\tPatchFv FvBuildDir [FvFileBaseNames:]FdFileBaseNameToPatch \"Offset, Value\""

def main():
    #
    # Parse the options and args
    #
    symTables = Symbols()

    if len(sys.argv) < 4:
        Usage()
        return 1

    if symTables.createDicts(sys.argv[1], sys.argv[2]) != 0:
        print "ERROR: Failed to create symbol dictionary!!"
        return 2

    fdFile = symTables.getFdFile()
    fdSize = symTables.getFdSize()

    try:
        ret = IsFspHeaderValid(fdFile)
        if ret == False:
          raise Exception ("The FSP header is not valid. Stop patching FD.")
        comment = ""
        for fvFile in  sys.argv[3:]:
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
                    params.append (symTables.evaluate(item, isOffset))

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
                # Copy binary block from source to destination
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
