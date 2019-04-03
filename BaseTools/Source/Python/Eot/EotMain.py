## @file
# This file is used to be the main entrance of EOT tool
#
# Copyright (c) 2008 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

##
# Import Modules
#
from __future__ import absolute_import
import Common.LongFilePathOs as os, time, glob
import Common.EdkLogger as EdkLogger
import Eot.EotGlobalData as EotGlobalData
from optparse import OptionParser
from Common.StringUtils import NormPath
from Common import BuildToolError
from Common.Misc import GuidStructureStringToGuidString
from collections import OrderedDict as sdict
from Eot.Parser import *
from Eot.InfParserLite import EdkInfParser
from Common.StringUtils import GetSplitValueList
from Eot import c
from Eot import Database
from array import array
from Eot.Report import Report
from Common.BuildVersion import gBUILD_VERSION
from Eot.Parser import ConvertGuid
from Common.LongFilePathSupport import OpenLongFilePath as open
import struct
import uuid
import copy
import codecs
from GenFds.AprioriSection import DXE_APRIORI_GUID, PEI_APRIORI_GUID

gGuidStringFormat = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
gIndention = -4

class Image(array):
    _HEADER_ = struct.Struct("")
    _HEADER_SIZE_ = _HEADER_.size

    def __new__(cls, *args, **kwargs):
        return array.__new__(cls, 'B')

    def __init__(self, ID=None):
        if ID is None:
            self._ID_ = str(uuid.uuid1()).upper()
        else:
            self._ID_ = ID
        self._BUF_ = None
        self._LEN_ = None
        self._OFF_ = None

        self._SubImages = sdict() # {offset: Image()}

        array.__init__(self)

    def __repr__(self):
        return self._ID_

    def __len__(self):
        Len = array.__len__(self)
        for Offset in self._SubImages.keys():
            Len += len(self._SubImages[Offset])
        return Len

    def _Unpack(self):
        self.extend(self._BUF_[self._OFF_ : self._OFF_ + self._LEN_])
        return len(self)

    def _Pack(self, PadByte=0xFF):
        raise NotImplementedError

    def frombuffer(self, Buffer, Offset=0, Size=None):
        self._BUF_ = Buffer
        self._OFF_ = Offset
        # we may need the Size information in advance if it's given
        self._LEN_ = Size
        self._LEN_ = self._Unpack()

    def empty(self):
        del self[0:]

    def GetField(self, FieldStruct, Offset=0):
        return FieldStruct.unpack_from(self, Offset)

    def SetField(self, FieldStruct, Offset, *args):
        # check if there's enough space
        Size = FieldStruct.size
        if Size > len(self):
            self.extend([0] * (Size - len(self)))
        FieldStruct.pack_into(self, Offset, *args)

    def _SetData(self, Data):
        if len(self) < self._HEADER_SIZE_:
            self.extend([0] * (self._HEADER_SIZE_ - len(self)))
        else:
            del self[self._HEADER_SIZE_:]
        self.extend(Data)

    def _GetData(self):
        if len(self) > self._HEADER_SIZE_:
            return self[self._HEADER_SIZE_:]
        return None

    Data = property(_GetData, _SetData)

## CompressedImage() class
#
#  A class for Compressed Image
#
class CompressedImage(Image):
    # UncompressedLength = 4-byte
    # CompressionType = 1-byte
    _HEADER_ = struct.Struct("1I 1B")
    _HEADER_SIZE_ = _HEADER_.size

    _ORIG_SIZE_     = struct.Struct("1I")
    _CMPRS_TYPE_    = struct.Struct("4x 1B")

    def __init__(self, CompressedData=None, CompressionType=None, UncompressedLength=None):
        Image.__init__(self)
        if UncompressedLength is not None:
            self.UncompressedLength = UncompressedLength
        if CompressionType is not None:
            self.CompressionType = CompressionType
        if CompressedData is not None:
            self.Data = CompressedData

    def __str__(self):
        global gIndention
        S = "algorithm=%s uncompressed=%x" % (self.CompressionType, self.UncompressedLength)
        for Sec in self.Sections:
            S += '\n' + str(Sec)

        return S

    def _SetOriginalSize(self, Size):
        self.SetField(self._ORIG_SIZE_, 0, Size)

    def _GetOriginalSize(self):
        return self.GetField(self._ORIG_SIZE_)[0]

    def _SetCompressionType(self, Type):
        self.SetField(self._CMPRS_TYPE_, 0, Type)

    def _GetCompressionType(self):
        return self.GetField(self._CMPRS_TYPE_)[0]

    def _GetSections(self):
        try:
            TmpData = DeCompress('Efi', self[self._HEADER_SIZE_:])
            DecData = array('B')
            DecData.fromstring(TmpData)
        except:
            TmpData = DeCompress('Framework', self[self._HEADER_SIZE_:])
            DecData = array('B')
            DecData.fromstring(TmpData)

        SectionList = []
        Offset = 0
        while Offset < len(DecData):
            Sec = Section()
            try:
                Sec.frombuffer(DecData, Offset)
                Offset += Sec.Size
                # the section is aligned to 4-byte boundary
            except:
                break
            SectionList.append(Sec)
        return SectionList

    UncompressedLength = property(_GetOriginalSize, _SetOriginalSize)
    CompressionType = property(_GetCompressionType, _SetCompressionType)
    Sections = property(_GetSections)

## Ui() class
#
#  A class for Ui
#
class Ui(Image):
    _HEADER_ = struct.Struct("")
    _HEADER_SIZE_ = 0

    def __init__(self):
        Image.__init__(self)

    def __str__(self):
        return self.String

    def _Unpack(self):
        # keep header in this Image object
        self.empty()
        self.extend(self._BUF_[self._OFF_ : self._OFF_ + self._LEN_])
        return len(self)

    def _GetUiString(self):
        return codecs.utf_16_decode(self[0:-2].tostring())[0]

    String = property(_GetUiString)

## Depex() class
#
#  A class for Depex
#
class Depex(Image):
    _HEADER_ = struct.Struct("")
    _HEADER_SIZE_ = 0

    _GUID_          = struct.Struct("1I2H8B")
    _OPCODE_        = struct.Struct("1B")

    _OPCODE_STRING_ = {
        0x00    :   "BEFORE",
        0x01    :   "AFTER",
        0x02    :   "PUSH",
        0x03    :   "AND",
        0x04    :   "OR",
        0x05    :   "NOT",
        0x06    :   "TRUE",
        0x07    :   "FALSE",
        0x08    :   "END",
        0x09    :   "SOR"
    }

    _NEXT_ = {
        -1      :   _OPCODE_,   # first one in depex must be an opcdoe
        0x00    :   _GUID_,     #"BEFORE",
        0x01    :   _GUID_,     #"AFTER",
        0x02    :   _GUID_,     #"PUSH",
        0x03    :   _OPCODE_,   #"AND",
        0x04    :   _OPCODE_,   #"OR",
        0x05    :   _OPCODE_,   #"NOT",
        0x06    :   _OPCODE_,   #"TRUE",
        0x07    :   _OPCODE_,   #"FALSE",
        0x08    :   None,       #"END",
        0x09    :   _OPCODE_,   #"SOR"
    }

    def __init__(self):
        Image.__init__(self)
        self._ExprList = []

    def __str__(self):
        global gIndention
        gIndention += 4
        Indention = ' ' * gIndention
        S = '\n'
        for T in self.Expression:
            if T in self._OPCODE_STRING_:
                S += Indention + self._OPCODE_STRING_[T]
                if T not in [0x00, 0x01, 0x02]:
                    S += '\n'
            else:
                S += ' ' + gGuidStringFormat % T + '\n'
        gIndention -= 4
        return S

    def _Unpack(self):
        # keep header in this Image object
        self.empty()
        self.extend(self._BUF_[self._OFF_ : self._OFF_ + self._LEN_])
        return len(self)

    def _GetExpression(self):
        if self._ExprList == []:
            Offset = 0
            CurrentData = self._OPCODE_
            while Offset < len(self):
                Token = CurrentData.unpack_from(self, Offset)
                Offset += CurrentData.size
                if len(Token) == 1:
                    Token = Token[0]
                    if Token in self._NEXT_:
                        CurrentData = self._NEXT_[Token]
                    else:
                        CurrentData = self._GUID_
                else:
                    CurrentData = self._OPCODE_
                self._ExprList.append(Token)
                if CurrentData is None:
                    break
        return self._ExprList

    Expression = property(_GetExpression)

# # FirmwareVolume() class
#
#  A class for Firmware Volume
#
class FirmwareVolume(Image):
    # Read FvLength, Attributes, HeaderLength, Checksum
    _HEADER_ = struct.Struct("16x 1I2H8B 1Q 4x 1I 1H 1H")
    _HEADER_SIZE_ = _HEADER_.size

    _FfsGuid = "8C8CE578-8A3D-4F1C-9935-896185C32DD3"

    _GUID_ = struct.Struct("16x 1I2H8B")
    _LENGTH_ = struct.Struct("16x 16x 1Q")
    _SIG_ = struct.Struct("16x 16x 8x 1I")
    _ATTR_ = struct.Struct("16x 16x 8x 4x 1I")
    _HLEN_ = struct.Struct("16x 16x 8x 4x 4x 1H")
    _CHECKSUM_ = struct.Struct("16x 16x 8x 4x 4x 2x 1H")

    def __init__(self, Name=''):
        Image.__init__(self)
        self.Name = Name
        self.FfsDict = sdict()
        self.OrderedFfsDict = sdict()
        self.UnDispatchedFfsDict = sdict()
        self.ProtocolList = sdict()

    def CheckArchProtocol(self):
        for Item in EotGlobalData.gArchProtocolGuids:
            if Item.lower() not in EotGlobalData.gProtocolList:
                return False
        return True

    def ParseDepex(self, Depex, Type):
        List = None
        if Type == 'Ppi':
            List = EotGlobalData.gPpiList
        if Type == 'Protocol':
            List = EotGlobalData.gProtocolList
        DepexStack = []
        DepexList = []
        DepexString = ''
        FileDepex = None
        CouldBeLoaded = True
        for Index in range(0, len(Depex.Expression)):
            Item = Depex.Expression[Index]
            if Item == 0x00:
                Index = Index + 1
                Guid = gGuidStringFormat % Depex.Expression[Index]
                if Guid in self.OrderedFfsDict and Depex.Expression[Index + 1] == 0x08:
                    return (True, 'BEFORE %s' % Guid, [Guid, 'BEFORE'])
            elif Item == 0x01:
                Index = Index + 1
                Guid = gGuidStringFormat % Depex.Expression[Index]
                if Guid in self.OrderedFfsDict and Depex.Expression[Index + 1] == 0x08:
                    return (True, 'AFTER %s' % Guid, [Guid, 'AFTER'])
            elif Item == 0x02:
                Index = Index + 1
                Guid = gGuidStringFormat % Depex.Expression[Index]
                if Guid.lower() in List:
                    DepexStack.append(True)
                    DepexList.append(Guid)
                else:
                    DepexStack.append(False)
                    DepexList.append(Guid)
                continue
            elif Item == 0x03 or Item == 0x04:
                DepexStack.append(eval(str(DepexStack.pop()) + ' ' + Depex._OPCODE_STRING_[Item].lower() + ' ' + str(DepexStack.pop())))
                DepexList.append(str(DepexList.pop()) + ' ' + Depex._OPCODE_STRING_[Item].upper() + ' ' + str(DepexList.pop()))
            elif Item == 0x05:
                DepexStack.append(eval(Depex._OPCODE_STRING_[Item].lower() + ' ' + str(DepexStack.pop())))
                DepexList.append(Depex._OPCODE_STRING_[Item].lower() + ' ' + str(DepexList.pop()))
            elif Item == 0x06:
                DepexStack.append(True)
                DepexList.append('TRUE')
                DepexString = DepexString + 'TRUE' + ' '
            elif Item == 0x07:
                DepexStack.append(False)
                DepexList.append('False')
                DepexString = DepexString + 'FALSE' + ' '
            elif Item == 0x08:
                if Index != len(Depex.Expression) - 1:
                    CouldBeLoaded = False
                else:
                    CouldBeLoaded = DepexStack.pop()
            else:
                CouldBeLoaded = False
        if DepexList != []:
            DepexString = DepexList[0].strip()
        return (CouldBeLoaded, DepexString, FileDepex)

    def Dispatch(self, Db=None):
        if Db is None:
            return False
        self.UnDispatchedFfsDict = copy.copy(self.FfsDict)
        # Find PeiCore, DexCore, PeiPriori, DxePriori first
        FfsSecCoreGuid = None
        FfsPeiCoreGuid = None
        FfsDxeCoreGuid = None
        FfsPeiPrioriGuid = None
        FfsDxePrioriGuid = None
        for FfsID in list(self.UnDispatchedFfsDict.keys()):
            Ffs = self.UnDispatchedFfsDict[FfsID]
            if Ffs.Type == 0x03:
                FfsSecCoreGuid = FfsID
                continue
            if Ffs.Type == 0x04:
                FfsPeiCoreGuid = FfsID
                continue
            if Ffs.Type == 0x05:
                FfsDxeCoreGuid = FfsID
                continue
            if Ffs.Guid.lower() == PEI_APRIORI_GUID.lower():
                FfsPeiPrioriGuid = FfsID
                continue
            if Ffs.Guid.lower() == DXE_APRIORI_GUID.lower():
                FfsDxePrioriGuid = FfsID
                continue

        # Parse SEC_CORE first
        if FfsSecCoreGuid is not None:
            self.OrderedFfsDict[FfsSecCoreGuid] = self.UnDispatchedFfsDict.pop(FfsSecCoreGuid)
            self.LoadPpi(Db, FfsSecCoreGuid)

        # Parse PEI first
        if FfsPeiCoreGuid is not None:
            self.OrderedFfsDict[FfsPeiCoreGuid] = self.UnDispatchedFfsDict.pop(FfsPeiCoreGuid)
            self.LoadPpi(Db, FfsPeiCoreGuid)
            if FfsPeiPrioriGuid is not None:
                # Load PEIM described in priori file
                FfsPeiPriori = self.UnDispatchedFfsDict.pop(FfsPeiPrioriGuid)
                if len(FfsPeiPriori.Sections) == 1:
                    Section = FfsPeiPriori.Sections.popitem()[1]
                    if Section.Type == 0x19:
                        GuidStruct = struct.Struct('1I2H8B')
                        Start = 4
                        while len(Section) > Start:
                            Guid = GuidStruct.unpack_from(Section[Start : Start + 16])
                            GuidString = gGuidStringFormat % Guid
                            Start = Start + 16
                            if GuidString in self.UnDispatchedFfsDict:
                                self.OrderedFfsDict[GuidString] = self.UnDispatchedFfsDict.pop(GuidString)
                                self.LoadPpi(Db, GuidString)

        self.DisPatchPei(Db)

        # Parse DXE then
        if FfsDxeCoreGuid is not None:
            self.OrderedFfsDict[FfsDxeCoreGuid] = self.UnDispatchedFfsDict.pop(FfsDxeCoreGuid)
            self.LoadProtocol(Db, FfsDxeCoreGuid)
            if FfsDxePrioriGuid is not None:
                # Load PEIM described in priori file
                FfsDxePriori = self.UnDispatchedFfsDict.pop(FfsDxePrioriGuid)
                if len(FfsDxePriori.Sections) == 1:
                    Section = FfsDxePriori.Sections.popitem()[1]
                    if Section.Type == 0x19:
                        GuidStruct = struct.Struct('1I2H8B')
                        Start = 4
                        while len(Section) > Start:
                            Guid = GuidStruct.unpack_from(Section[Start : Start + 16])
                            GuidString = gGuidStringFormat % Guid
                            Start = Start + 16
                            if GuidString in self.UnDispatchedFfsDict:
                                self.OrderedFfsDict[GuidString] = self.UnDispatchedFfsDict.pop(GuidString)
                                self.LoadProtocol(Db, GuidString)

        self.DisPatchDxe(Db)

    def LoadProtocol(self, Db, ModuleGuid):
        SqlCommand = """select GuidValue from Report
                        where SourceFileFullPath in
                        (select Value1 from Inf where BelongsToFile =
                        (select BelongsToFile from Inf
                        where Value1 = 'FILE_GUID' and Value2 like '%s' and Model = %s)
                        and Model = %s)
                        and ItemType = 'Protocol' and ItemMode = 'Produced'""" \
                        % (ModuleGuid, 5001, 3007)
        RecordSet = Db.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            SqlCommand = """select Value2 from Inf where BelongsToFile =
                            (select DISTINCT BelongsToFile from Inf
                            where Value1 =
                            (select SourceFileFullPath from Report
                            where GuidValue like '%s' and ItemMode = 'Callback'))
                            and Value1 = 'FILE_GUID'""" % Record[0]
            CallBackSet = Db.TblReport.Exec(SqlCommand)
            if CallBackSet != []:
                EotGlobalData.gProtocolList[Record[0].lower()] = ModuleGuid
            else:
                EotGlobalData.gProtocolList[Record[0].lower()] = ModuleGuid

    def LoadPpi(self, Db, ModuleGuid):
        SqlCommand = """select GuidValue from Report
                        where SourceFileFullPath in
                        (select Value1 from Inf where BelongsToFile =
                        (select BelongsToFile from Inf
                        where Value1 = 'FILE_GUID' and Value2 like '%s' and Model = %s)
                        and Model = %s)
                        and ItemType = 'Ppi' and ItemMode = 'Produced'""" \
                        % (ModuleGuid, 5001, 3007)
        RecordSet = Db.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            EotGlobalData.gPpiList[Record[0].lower()] = ModuleGuid

    def DisPatchDxe(self, Db):
        IsInstalled = False
        ScheduleList = sdict()
        for FfsID in list(self.UnDispatchedFfsDict.keys()):
            CouldBeLoaded = False
            DepexString = ''
            FileDepex = None
            Ffs = self.UnDispatchedFfsDict[FfsID]
            if Ffs.Type == 0x07:
                # Get Depex
                IsFoundDepex = False
                for Section in Ffs.Sections.values():
                    # Find Depex
                    if Section.Type == 0x13:
                        IsFoundDepex = True
                        CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(Section._SubImages[4], 'Protocol')
                        break
                    if Section.Type == 0x01:
                        CompressSections = Section._SubImages[4]
                        for CompressSection in CompressSections.Sections:
                            if CompressSection.Type == 0x13:
                                IsFoundDepex = True
                                CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(CompressSection._SubImages[4], 'Protocol')
                                break
                            if CompressSection.Type == 0x02:
                                NewSections = CompressSection._SubImages[4]
                                for NewSection in NewSections.Sections:
                                    if NewSection.Type == 0x13:
                                        IsFoundDepex = True
                                        CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(NewSection._SubImages[4], 'Protocol')
                                        break

                # Not find Depex
                if not IsFoundDepex:
                    CouldBeLoaded = self.CheckArchProtocol()
                    DepexString = ''
                    FileDepex = None

                # Append New Ffs
                if CouldBeLoaded:
                    IsInstalled = True
                    NewFfs = self.UnDispatchedFfsDict.pop(FfsID)
                    NewFfs.Depex = DepexString
                    if FileDepex is not None:
                        ScheduleList.insert(FileDepex[1], FfsID, NewFfs, FileDepex[0])
                    else:
                        ScheduleList[FfsID] = NewFfs
                else:
                    self.UnDispatchedFfsDict[FfsID].Depex = DepexString

        for FfsID in ScheduleList.keys():
            NewFfs = ScheduleList.pop(FfsID)
            FfsName = 'UnKnown'
            self.OrderedFfsDict[FfsID] = NewFfs
            self.LoadProtocol(Db, FfsID)

            SqlCommand = """select Value2 from Inf
                            where BelongsToFile = (select BelongsToFile from Inf where Value1 = 'FILE_GUID' and lower(Value2) = lower('%s') and Model = %s)
                            and Model = %s and Value1='BASE_NAME'""" % (FfsID, 5001, 5001)
            RecordSet = Db.TblReport.Exec(SqlCommand)
            if RecordSet != []:
                FfsName = RecordSet[0][0]

        if IsInstalled:
            self.DisPatchDxe(Db)

    def DisPatchPei(self, Db):
        IsInstalled = False
        for FfsID in list(self.UnDispatchedFfsDict.keys()):
            CouldBeLoaded = True
            DepexString = ''
            FileDepex = None
            Ffs = self.UnDispatchedFfsDict[FfsID]
            if Ffs.Type == 0x06 or Ffs.Type == 0x08:
                # Get Depex
                for Section in Ffs.Sections.values():
                    if Section.Type == 0x1B:
                        CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(Section._SubImages[4], 'Ppi')
                        break
                    if Section.Type == 0x01:
                        CompressSections = Section._SubImages[4]
                        for CompressSection in CompressSections.Sections:
                            if CompressSection.Type == 0x1B:
                                CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(CompressSection._SubImages[4], 'Ppi')
                                break
                            if CompressSection.Type == 0x02:
                                NewSections = CompressSection._SubImages[4]
                                for NewSection in NewSections.Sections:
                                    if NewSection.Type == 0x1B:
                                        CouldBeLoaded, DepexString, FileDepex = self.ParseDepex(NewSection._SubImages[4], 'Ppi')
                                        break

                # Append New Ffs
                if CouldBeLoaded:
                    IsInstalled = True
                    NewFfs = self.UnDispatchedFfsDict.pop(FfsID)
                    NewFfs.Depex = DepexString
                    self.OrderedFfsDict[FfsID] = NewFfs
                    self.LoadPpi(Db, FfsID)
                else:
                    self.UnDispatchedFfsDict[FfsID].Depex = DepexString

        if IsInstalled:
            self.DisPatchPei(Db)


    def __str__(self):
        global gIndention
        gIndention += 4
        FvInfo = '\n' + ' ' * gIndention
        FvInfo += "[FV:%s] file_system=%s size=%x checksum=%s\n" % (self.Name, self.FileSystemGuid, self.Size, self.Checksum)
        FfsInfo = "\n".join([str(self.FfsDict[FfsId]) for FfsId in self.FfsDict])
        gIndention -= 4
        return FvInfo + FfsInfo

    def _Unpack(self):
        Size = self._LENGTH_.unpack_from(self._BUF_, self._OFF_)[0]
        self.empty()
        self.extend(self._BUF_[self._OFF_:self._OFF_ + Size])

        # traverse the FFS
        EndOfFv = Size
        FfsStartAddress = self.HeaderSize
        LastFfsObj = None
        while FfsStartAddress < EndOfFv:
            FfsObj = Ffs()
            FfsObj.frombuffer(self, FfsStartAddress)
            FfsId = repr(FfsObj)
            if ((self.Attributes & 0x00000800) != 0 and len(FfsObj) == 0xFFFFFF) \
                or ((self.Attributes & 0x00000800) == 0 and len(FfsObj) == 0):
                if LastFfsObj is not None:
                    LastFfsObj.FreeSpace = EndOfFv - LastFfsObj._OFF_ - len(LastFfsObj)
            else:
                if FfsId in self.FfsDict:
                    EdkLogger.error("FV", 0, "Duplicate GUID in FFS",
                                    ExtraData="\t%s @ %s\n\t%s @ %s" \
                                    % (FfsObj.Guid, FfsObj.Offset,
                                       self.FfsDict[FfsId].Guid, self.FfsDict[FfsId].Offset))
                self.FfsDict[FfsId] = FfsObj
                if LastFfsObj is not None:
                    LastFfsObj.FreeSpace = FfsStartAddress - LastFfsObj._OFF_ - len(LastFfsObj)

            FfsStartAddress += len(FfsObj)
            #
            # align to next 8-byte aligned address: A = (A + 8 - 1) & (~(8 - 1))
            # The next FFS must be at the latest next 8-byte aligned address
            #
            FfsStartAddress = (FfsStartAddress + 7) & (~7)
            LastFfsObj = FfsObj

    def _GetAttributes(self):
        return self.GetField(self._ATTR_, 0)[0]

    def _GetSize(self):
        return self.GetField(self._LENGTH_, 0)[0]

    def _GetChecksum(self):
        return self.GetField(self._CHECKSUM_, 0)[0]

    def _GetHeaderLength(self):
        return self.GetField(self._HLEN_, 0)[0]

    def _GetFileSystemGuid(self):
        return gGuidStringFormat % self.GetField(self._GUID_, 0)

    Attributes = property(_GetAttributes)
    Size = property(_GetSize)
    Checksum = property(_GetChecksum)
    HeaderSize = property(_GetHeaderLength)
    FileSystemGuid = property(_GetFileSystemGuid)

## GuidDefinedImage() class
#
#  A class for GUID Defined Image
#
class GuidDefinedImage(Image):
    _HEADER_ = struct.Struct("1I2H8B 1H 1H")
    _HEADER_SIZE_ = _HEADER_.size

    _GUID_          = struct.Struct("1I2H8B")
    _DATA_OFFSET_   = struct.Struct("16x 1H")
    _ATTR_          = struct.Struct("18x 1H")

    CRC32_GUID          = "FC1BCDB0-7D31-49AA-936A-A4600D9DD083"
    TIANO_COMPRESS_GUID = 'A31280AD-481E-41B6-95E8-127F4C984779'
    LZMA_COMPRESS_GUID  = 'EE4E5898-3914-4259-9D6E-DC7BD79403CF'

    def __init__(self, SectionDefinitionGuid=None, DataOffset=None, Attributes=None, Data=None):
        Image.__init__(self)
        if SectionDefinitionGuid is not None:
            self.SectionDefinitionGuid = SectionDefinitionGuid
        if DataOffset is not None:
            self.DataOffset = DataOffset
        if Attributes is not None:
            self.Attributes = Attributes
        if Data is not None:
            self.Data = Data

    def __str__(self):
        S = "guid=%s" % (gGuidStringFormat % self.SectionDefinitionGuid)
        for Sec in self.Sections:
            S += "\n" + str(Sec)
        return S

    def _Unpack(self):
        # keep header in this Image object
        self.empty()
        self.extend(self._BUF_[self._OFF_ : self._OFF_ + self._LEN_])
        return len(self)

    def _SetAttribute(self, Attribute):
        self.SetField(self._ATTR_, 0, Attribute)

    def _GetAttribute(self):
        return self.GetField(self._ATTR_)[0]

    def _SetGuid(self, Guid):
        self.SetField(self._GUID_, 0, Guid)

    def _GetGuid(self):
        return self.GetField(self._GUID_)

    def _SetDataOffset(self, Offset):
        self.SetField(self._DATA_OFFSET_, 0, Offset)

    def _GetDataOffset(self):
        return self.GetField(self._DATA_OFFSET_)[0]

    def _GetSections(self):
        SectionList = []
        Guid = gGuidStringFormat % self.SectionDefinitionGuid
        if Guid == self.CRC32_GUID:
            # skip the CRC32 value, we don't do CRC32 verification here
            Offset = self.DataOffset - 4
            while Offset < len(self):
                Sec = Section()
                try:
                    Sec.frombuffer(self, Offset)
                    Offset += Sec.Size
                    # the section is aligned to 4-byte boundary
                    Offset = (Offset + 3) & (~3)
                except:
                    break
                SectionList.append(Sec)
        elif Guid == self.TIANO_COMPRESS_GUID:
            try:
                # skip the header
                Offset = self.DataOffset - 4
                TmpData = DeCompress('Framework', self[self.Offset:])
                DecData = array('B')
                DecData.fromstring(TmpData)
                Offset = 0
                while Offset < len(DecData):
                    Sec = Section()
                    try:
                        Sec.frombuffer(DecData, Offset)
                        Offset += Sec.Size
                        # the section is aligned to 4-byte boundary
                        Offset = (Offset + 3) & (~3)
                    except:
                        break
                    SectionList.append(Sec)
            except:
                pass
        elif Guid == self.LZMA_COMPRESS_GUID:
            try:
                # skip the header
                Offset = self.DataOffset - 4

                TmpData = DeCompress('Lzma', self[self.Offset:])
                DecData = array('B')
                DecData.fromstring(TmpData)
                Offset = 0
                while Offset < len(DecData):
                    Sec = Section()
                    try:
                        Sec.frombuffer(DecData, Offset)
                        Offset += Sec.Size
                        # the section is aligned to 4-byte boundary
                        Offset = (Offset + 3) & (~3)
                    except:
                        break
                    SectionList.append(Sec)
            except:
                pass

        return SectionList

    Attributes = property(_GetAttribute, _SetAttribute)
    SectionDefinitionGuid = property(_GetGuid, _SetGuid)
    DataOffset = property(_GetDataOffset, _SetDataOffset)
    Sections = property(_GetSections)

## Section() class
#
#  A class for Section
#
class Section(Image):
    _TypeName = {
        0x00    :   "<unknown>",
        0x01    :   "COMPRESSION",
        0x02    :   "GUID_DEFINED",
        0x10    :   "PE32",
        0x11    :   "PIC",
        0x12    :   "TE",
        0x13    :   "DXE_DEPEX",
        0x14    :   "VERSION",
        0x15    :   "USER_INTERFACE",
        0x16    :   "COMPATIBILITY16",
        0x17    :   "FIRMWARE_VOLUME_IMAGE",
        0x18    :   "FREEFORM_SUBTYPE_GUID",
        0x19    :   "RAW",
        0x1B    :   "PEI_DEPEX"
    }

    _SectionSubImages = {
        0x01    :   CompressedImage,
        0x02    :   GuidDefinedImage,
        0x17    :   FirmwareVolume,
        0x13    :   Depex,
        0x1B    :   Depex,
        0x15    :   Ui
    }

    # Size = 3-byte
    # Type = 1-byte
    _HEADER_ = struct.Struct("3B 1B")
    _HEADER_SIZE_ = _HEADER_.size

    # SubTypeGuid
    # _FREE_FORM_SUBTYPE_GUID_HEADER_ = struct.Struct("1I2H8B")
    _SIZE_          = struct.Struct("3B")
    _TYPE_          = struct.Struct("3x 1B")

    def __init__(self, Type=None, Size=None):
        Image.__init__(self)
        self._Alignment = 1
        if Type is not None:
            self.Type = Type
        if Size is not None:
            self.Size = Size

    def __str__(self):
        global gIndention
        gIndention += 4
        SectionInfo = ' ' * gIndention
        if self.Type in self._TypeName:
            SectionInfo += "[SECTION:%s] offset=%x size=%x" % (self._TypeName[self.Type], self._OFF_, self.Size)
        else:
            SectionInfo += "[SECTION:%x<unknown>] offset=%x size=%x " % (self.Type, self._OFF_, self.Size)
        for Offset in self._SubImages.keys():
            SectionInfo += ", " + str(self._SubImages[Offset])
        gIndention -= 4
        return SectionInfo

    def _Unpack(self):
        self.empty()
        Type, = self._TYPE_.unpack_from(self._BUF_, self._OFF_)
        Size1, Size2, Size3 = self._SIZE_.unpack_from(self._BUF_, self._OFF_)
        Size = Size1 + (Size2 << 8) + (Size3 << 16)

        if Type not in self._SectionSubImages:
            # no need to extract sub-image, keep all in this Image object
            self.extend(self._BUF_[self._OFF_ : self._OFF_ + Size])
        else:
            # keep header in this Image object
            self.extend(self._BUF_[self._OFF_ : self._OFF_ + self._HEADER_SIZE_])
            #
            # use new Image object to represent payload, which may be another kind
            # of image such as PE32
            #
            PayloadOffset = self._HEADER_SIZE_
            PayloadLen = self.Size - self._HEADER_SIZE_
            Payload = self._SectionSubImages[self.Type]()
            Payload.frombuffer(self._BUF_, self._OFF_ + self._HEADER_SIZE_, PayloadLen)
            self._SubImages[PayloadOffset] = Payload

        return Size

    def _SetSize(self, Size):
        Size1 = Size & 0xFF
        Size2 = (Size & 0xFF00) >> 8
        Size3 = (Size & 0xFF0000) >> 16
        self.SetField(self._SIZE_, 0, Size1, Size2, Size3)

    def _GetSize(self):
        Size1, Size2, Size3 = self.GetField(self._SIZE_)
        return Size1 + (Size2 << 8) + (Size3 << 16)

    def _SetType(self, Type):
        self.SetField(self._TYPE_, 0, Type)

    def _GetType(self):
        return self.GetField(self._TYPE_)[0]

    def _GetAlignment(self):
        return self._Alignment

    def _SetAlignment(self, Alignment):
        self._Alignment = Alignment
        AlignmentMask = Alignment - 1
        # section alignment is actually for payload, so we need to add header size
        PayloadOffset = self._OFF_ + self._HEADER_SIZE_
        if (PayloadOffset & (~AlignmentMask)) == 0:
            return
        NewOffset = (PayloadOffset + AlignmentMask) & (~AlignmentMask)
        while (NewOffset - PayloadOffset) < self._HEADER_SIZE_:
            NewOffset += self._Alignment

    def tofile(self, f):
        self.Size = len(self)
        Image.tofile(self, f)
        for Offset in self._SubImages:
            self._SubImages[Offset].tofile(f)

    Type = property(_GetType, _SetType)
    Size = property(_GetSize, _SetSize)
    Alignment = property(_GetAlignment, _SetAlignment)

## Ffs() class
#
#  A class for Ffs Section
#
class Ffs(Image):
    _FfsFormat = "24B%(payload_size)sB"
    # skip IntegrityCheck
    _HEADER_ = struct.Struct("1I2H8B 2x 1B 1B 3B 1B")
    _HEADER_SIZE_ = _HEADER_.size

    _NAME_      = struct.Struct("1I2H8B")
    _INT_CHECK_ = struct.Struct("16x 1H")
    _TYPE_      = struct.Struct("18x 1B")
    _ATTR_      = struct.Struct("19x 1B")
    _SIZE_      = struct.Struct("20x 3B")
    _STATE_     = struct.Struct("23x 1B")

    FFS_ATTRIB_FIXED              = 0x04
    FFS_ATTRIB_DATA_ALIGNMENT     = 0x38
    FFS_ATTRIB_CHECKSUM           = 0x40

    _TypeName = {
        0x00    :   "<unknown>",
        0x01    :   "RAW",
        0x02    :   "FREEFORM",
        0x03    :   "SECURITY_CORE",
        0x04    :   "PEI_CORE",
        0x05    :   "DXE_CORE",
        0x06    :   "PEIM",
        0x07    :   "DRIVER",
        0x08    :   "COMBINED_PEIM_DRIVER",
        0x09    :   "APPLICATION",
        0x0A    :   "SMM",
        0x0B    :   "FIRMWARE_VOLUME_IMAGE",
        0x0C    :   "COMBINED_SMM_DXE",
        0x0D    :   "SMM_CORE",
        0x0E    :   "MM_STANDALONE",
        0x0F    :   "MM_CORE_STANDALONE",
        0xc0    :   "OEM_MIN",
        0xdf    :   "OEM_MAX",
        0xe0    :   "DEBUG_MIN",
        0xef    :   "DEBUG_MAX",
        0xf0    :   "FFS_MIN",
        0xff    :   "FFS_MAX",
        0xf0    :   "FFS_PAD",
    }

    def __init__(self):
        Image.__init__(self)
        self.FreeSpace = 0

        self.Sections = sdict()
        self.Depex = ''

        self.__ID__ = None

    def __str__(self):
        global gIndention
        gIndention += 4
        Indention = ' ' * gIndention
        FfsInfo = Indention
        FfsInfo +=  "[FFS:%s] offset=%x size=%x guid=%s free_space=%x alignment=%s\n" % \
                    (Ffs._TypeName[self.Type], self._OFF_, self.Size, self.Guid, self.FreeSpace, self.Alignment)
        SectionInfo = '\n'.join([str(self.Sections[Offset]) for Offset in self.Sections.keys()])
        gIndention -= 4
        return FfsInfo + SectionInfo + "\n"

    def __len__(self):
        return self.Size

    def __repr__(self):
        return self.__ID__

    def _Unpack(self):
        Size1, Size2, Size3 = self._SIZE_.unpack_from(self._BUF_, self._OFF_)
        Size = Size1 + (Size2 << 8) + (Size3 << 16)
        self.empty()
        self.extend(self._BUF_[self._OFF_ : self._OFF_ + Size])

        # Pad FFS may use the same GUID. We need to avoid it.
        if self.Type == 0xf0:
            self.__ID__ = str(uuid.uuid1()).upper()
        else:
            self.__ID__ = self.Guid

        # Traverse the SECTION. RAW and PAD do not have sections
        if self.Type not in [0xf0, 0x01] and Size > 0 and Size < 0xFFFFFF:
            EndOfFfs = Size
            SectionStartAddress = self._HEADER_SIZE_
            while SectionStartAddress < EndOfFfs:
                SectionObj = Section()
                SectionObj.frombuffer(self, SectionStartAddress)
                #f = open(repr(SectionObj), 'wb')
                #SectionObj.Size = 0
                #SectionObj.tofile(f)
                #f.close()
                self.Sections[SectionStartAddress] = SectionObj
                SectionStartAddress += len(SectionObj)
                SectionStartAddress = (SectionStartAddress + 3) & (~3)

    def Pack(self):
        pass

    def SetFreeSpace(self, Size):
        self.FreeSpace = Size

    def _GetGuid(self):
        return gGuidStringFormat % self.Name

    def _SetName(self, Value):
        # Guid1, Guid2, Guid3, Guid4, Guid5, Guid6, Guid7, Guid8, Guid9, Guid10, Guid11
        self.SetField(self._NAME_, 0, Value)

    def _GetName(self):
        # Guid1, Guid2, Guid3, Guid4, Guid5, Guid6, Guid7, Guid8, Guid9, Guid10, Guid11
        return self.GetField(self._NAME_)

    def _SetSize(self, Size):
        Size1 = Size & 0xFF
        Size2 = (Size & 0xFF00) >> 8
        Size3 = (Size & 0xFF0000) >> 16
        self.SetField(self._SIZE_, 0, Size1, Size2, Size3)

    def _GetSize(self):
        Size1, Size2, Size3 = self.GetField(self._SIZE_)
        return Size1 + (Size2 << 8) + (Size3 << 16)

    def _SetType(self, Type):
        self.SetField(self._TYPE_, 0, Type)

    def _GetType(self):
        return self.GetField(self._TYPE_)[0]

    def _SetAttributes(self, Value):
        self.SetField(self._ATTR_, 0, Value)

    def _GetAttributes(self):
        return self.GetField(self._ATTR_)[0]

    def _GetFixed(self):
        if (self.Attributes & self.FFS_ATTRIB_FIXED) != 0:
            return True
        return False

    def _GetCheckSum(self):
        if (self.Attributes & self.FFS_ATTRIB_CHECKSUM) != 0:
            return True
        return False

    def _GetAlignment(self):
        return (self.Attributes & self.FFS_ATTRIB_DATA_ALIGNMENT) >> 3

    def _SetState(self, Value):
        self.SetField(self._STATE_, 0, Value)

    def _GetState(self):
        return self.GetField(self._STATE_)[0]

    Name = property(_GetName, _SetName)
    Guid = property(_GetGuid)
    Type = property(_GetType, _SetType)
    Size = property(_GetSize, _SetSize)
    Attributes = property(_GetAttributes, _SetAttributes)
    Fixed = property(_GetFixed)
    Checksum = property(_GetCheckSum)
    Alignment = property(_GetAlignment)
    State = property(_GetState, _SetState)


## MultipleFv() class
#
#  A class for Multiple FV
#
class MultipleFv(FirmwareVolume):
    def __init__(self, FvList):
        FirmwareVolume.__init__(self)
        self.BasicInfo = []
        for FvPath in FvList:
            Fd = None
            FvName = os.path.splitext(os.path.split(FvPath)[1])[0]
            if FvPath.strip():
                Fd = open(FvPath, 'rb')
            Buf = array('B')
            try:
                Buf.fromfile(Fd, os.path.getsize(FvPath))
            except EOFError:
                pass

            Fv = FirmwareVolume(FvName)
            Fv.frombuffer(Buf, 0, len(Buf))

            self.BasicInfo.append([Fv.Name, Fv.FileSystemGuid, Fv.Size])
            self.FfsDict.update(Fv.FfsDict)

## Class Eot
#
# This class is used to define Eot main entrance
#
# @param object:          Inherited from object class
#
class Eot(object):
    ## The constructor
    #
    #   @param  self:      The object pointer
    #
    def __init__(self, CommandLineOption=True, IsInit=True, SourceFileList=None, \
                 IncludeDirList=None, DecFileList=None, GuidList=None, LogFile=None,
                 FvFileList="", MapFileList="", Report='Report.html', Dispatch=None):
        # Version and Copyright
        self.VersionNumber = ("0.02" + " " + gBUILD_VERSION)
        self.Version = "%prog Version " + self.VersionNumber
        self.Copyright = "Copyright (c) 2008 - 2018, Intel Corporation  All rights reserved."
        self.Report = Report

        self.IsInit = IsInit
        self.SourceFileList = SourceFileList
        self.IncludeDirList = IncludeDirList
        self.DecFileList = DecFileList
        self.GuidList = GuidList
        self.LogFile = LogFile
        self.FvFileList = FvFileList
        self.MapFileList = MapFileList
        self.Dispatch = Dispatch

        # Check workspace environment
        if "EFI_SOURCE" not in os.environ:
            if "EDK_SOURCE" not in os.environ:
                pass
            else:
                EotGlobalData.gEDK_SOURCE = os.path.normpath(os.getenv("EDK_SOURCE"))
        else:
            EotGlobalData.gEFI_SOURCE = os.path.normpath(os.getenv("EFI_SOURCE"))
            EotGlobalData.gEDK_SOURCE = os.path.join(EotGlobalData.gEFI_SOURCE, 'Edk')

        if "WORKSPACE" not in os.environ:
            EdkLogger.error("EOT", BuildToolError.ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                            ExtraData="WORKSPACE")
        else:
            EotGlobalData.gWORKSPACE = os.path.normpath(os.getenv("WORKSPACE"))

        EotGlobalData.gMACRO['WORKSPACE'] = EotGlobalData.gWORKSPACE
        EotGlobalData.gMACRO['EFI_SOURCE'] = EotGlobalData.gEFI_SOURCE
        EotGlobalData.gMACRO['EDK_SOURCE'] = EotGlobalData.gEDK_SOURCE

        # Parse the options and args
        if CommandLineOption:
            self.ParseOption()

        if self.FvFileList:
            for FvFile in GetSplitValueList(self.FvFileList, ' '):
                FvFile = os.path.normpath(FvFile)
                if not os.path.isfile(FvFile):
                    EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "Can not find file %s " % FvFile)
                EotGlobalData.gFV_FILE.append(FvFile)
        else:
            EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "The fv file list of target platform was not specified")

        if self.MapFileList:
            for MapFile in GetSplitValueList(self.MapFileList, ' '):
                MapFile = os.path.normpath(MapFile)
                if not os.path.isfile(MapFile):
                    EdkLogger.error("Eot", EdkLogger.EOT_ERROR, "Can not find file %s " % MapFile)
                EotGlobalData.gMAP_FILE.append(MapFile)

        # Generate source file list
        self.GenerateSourceFileList(self.SourceFileList, self.IncludeDirList)

        # Generate guid list of dec file list
        self.ParseDecFile(self.DecFileList)

        # Generate guid list from GUID list file
        self.ParseGuidList(self.GuidList)

        # Init Eot database
        EotGlobalData.gDb = Database.Database(Database.DATABASE_PATH)
        EotGlobalData.gDb.InitDatabase(self.IsInit)

        # Build ECC database
        self.BuildDatabase()

        # Parse Ppi/Protocol
        self.ParseExecutionOrder()

        # Merge Identifier tables
        self.GenerateQueryTable()

        # Generate report database
        self.GenerateReportDatabase()

        # Load Fv Info
        self.LoadFvInfo()

        # Load Map Info
        self.LoadMapInfo()

        # Generate Report
        self.GenerateReport()

        # Convert log file
        self.ConvertLogFile(self.LogFile)

        # DONE
        EdkLogger.quiet("EOT FINISHED!")

        # Close Database
        EotGlobalData.gDb.Close()

    ## ParseDecFile() method
    #
    #  parse DEC file and get all GUID names with GUID values as {GuidName : GuidValue}
    #  The Dict is stored in EotGlobalData.gGuidDict
    #
    #  @param self: The object pointer
    #  @param DecFileList: A list of all DEC files
    #
    def ParseDecFile(self, DecFileList):
        if DecFileList:
            path = os.path.normpath(DecFileList)
            lfr = open(path, 'rb')
            for line in lfr:
                path = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                if os.path.exists(path):
                    dfr = open(path, 'rb')
                    for line in dfr:
                        line = CleanString(line)
                        list = line.split('=')
                        if len(list) == 2:
                            EotGlobalData.gGuidDict[list[0].strip()] = GuidStructureStringToGuidString(list[1].strip())


    ## ParseGuidList() method
    #
    #  Parse Guid list and get all GUID names with GUID values as {GuidName : GuidValue}
    #  The Dict is stored in EotGlobalData.gGuidDict
    #
    #  @param self: The object pointer
    #  @param GuidList: A list of all GUID and its value
    #
    def ParseGuidList(self, GuidList):
        Path = os.path.join(EotGlobalData.gWORKSPACE, GuidList)
        if os.path.isfile(Path):
            for Line in open(Path):
                if Line.strip():
                    (GuidName, GuidValue) = Line.split()
                    EotGlobalData.gGuidDict[GuidName] = GuidValue

    ## ConvertLogFile() method
    #
    #  Parse a real running log file to get real dispatch order
    #  The result is saved to old file name + '.new'
    #
    #  @param self: The object pointer
    #  @param LogFile: A real running log file name
    #
    def ConvertLogFile(self, LogFile):
        newline = []
        lfr = None
        lfw = None
        if LogFile:
            lfr = open(LogFile, 'rb')
            lfw = open(LogFile + '.new', 'wb')
            for line in lfr:
                line = line.strip()
                line = line.replace('.efi', '')
                index = line.find("Loading PEIM at ")
                if index > -1:
                    newline.append(line[index + 55 : ])
                    continue
                index = line.find("Loading driver at ")
                if index > -1:
                    newline.append(line[index + 57 : ])
                    continue

        for line in newline:
            lfw.write(line + '\r\n')

        if lfr:
            lfr.close()
        if lfw:
            lfw.close()

    ## GenerateSourceFileList() method
    #
    #  Generate a list of all source files
    #  1. Search the file list one by one
    #  2. Store inf file name with source file names under it like
    #  { INF file name: [source file1, source file2, ...]}
    #  3. Search the include list to find all .h files
    #  4. Store source file list to EotGlobalData.gSOURCE_FILES
    #  5. Store INF file list to EotGlobalData.gINF_FILES
    #
    #  @param self: The object pointer
    #  @param SourceFileList: A list of all source files
    #  @param IncludeFileList: A list of all include files
    #
    def GenerateSourceFileList(self, SourceFileList, IncludeFileList):
        EdkLogger.quiet("Generating source files list ... ")
        mSourceFileList = []
        mInfFileList = []
        mDecFileList = []
        mFileList = {}
        mCurrentInfFile = ''
        mCurrentSourceFileList = []

        if SourceFileList:
            sfl = open(SourceFileList, 'r')
            for line in sfl:
                line = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                if line[-2:].upper() == '.C' or  line[-2:].upper() == '.H':
                    if line not in mCurrentSourceFileList:
                        mCurrentSourceFileList.append(line)
                        mSourceFileList.append(line)
                        EotGlobalData.gOP_SOURCE_FILES.write('%s\n' % line)
                if line[-4:].upper() == '.INF':
                    if mCurrentInfFile != '':
                        mFileList[mCurrentInfFile] = mCurrentSourceFileList
                        mCurrentSourceFileList = []
                    mCurrentInfFile = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line))
                    EotGlobalData.gOP_INF.write('%s\n' % mCurrentInfFile)
            if mCurrentInfFile not in mFileList:
                mFileList[mCurrentInfFile] = mCurrentSourceFileList

        # Get all include files from packages
        if IncludeFileList:
            ifl = open(IncludeFileList, 'rb')
            for line in ifl:
                if not line.strip():
                    continue
                newline = os.path.normpath(os.path.join(EotGlobalData.gWORKSPACE, line.strip()))
                for Root, Dirs, Files in os.walk(str(newline)):
                    for File in Files:
                        FullPath = os.path.normpath(os.path.join(Root, File))
                        if FullPath not in mSourceFileList and File[-2:].upper() == '.H':
                            mSourceFileList.append(FullPath)
                            EotGlobalData.gOP_SOURCE_FILES.write('%s\n' % FullPath)
                        if FullPath not in mDecFileList and File.upper().find('.DEC') > -1:
                            mDecFileList.append(FullPath)

        EotGlobalData.gSOURCE_FILES = mSourceFileList
        EotGlobalData.gOP_SOURCE_FILES.close()

        EotGlobalData.gINF_FILES = mFileList
        EotGlobalData.gOP_INF.close()

    ## GenerateReport() method
    #
    #  Generate final HTML report
    #
    #  @param self: The object pointer
    #
    def GenerateReport(self):
        EdkLogger.quiet("Generating report file ... ")
        Rep = Report(self.Report, EotGlobalData.gFV, self.Dispatch)
        Rep.GenerateReport()

    ## LoadMapInfo() method
    #
    #  Load map files and parse them
    #
    #  @param self: The object pointer
    #
    def LoadMapInfo(self):
        if EotGlobalData.gMAP_FILE != []:
            EdkLogger.quiet("Parsing Map file ... ")
            EotGlobalData.gMap = ParseMapFile(EotGlobalData.gMAP_FILE)

    ## LoadFvInfo() method
    #
    #  Load FV binary files and parse them
    #
    #  @param self: The object pointer
    #
    def LoadFvInfo(self):
        EdkLogger.quiet("Parsing FV file ... ")
        EotGlobalData.gFV = MultipleFv(EotGlobalData.gFV_FILE)
        EotGlobalData.gFV.Dispatch(EotGlobalData.gDb)

        for Protocol in EotGlobalData.gProtocolList:
            EotGlobalData.gOP_UN_MATCHED_IN_LIBRARY_CALLING.write('%s\n' %Protocol)

    ## GenerateReportDatabase() method
    #
    #  Generate data for the information needed by report
    #  1. Update name, macro and value of all found PPI/PROTOCOL GUID
    #  2. Install hard coded PPI/PROTOCOL
    #
    #  @param self: The object pointer
    #
    def GenerateReportDatabase(self):
        EdkLogger.quiet("Generating the cross-reference table of GUID for Ppi/Protocol ... ")

        # Update Protocol/Ppi Guid
        SqlCommand = """select DISTINCT GuidName from Report"""
        RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            GuidName = Record[0]
            GuidMacro = ''
            GuidMacro2 = ''
            GuidValue = ''

            # Find guid value defined in Dec file
            if GuidName in EotGlobalData.gGuidDict:
                GuidValue = EotGlobalData.gGuidDict[GuidName]
                SqlCommand = """update Report set GuidMacro = '%s', GuidValue = '%s' where GuidName = '%s'""" %(GuidMacro, GuidValue, GuidName)
                EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                continue

            # Search defined Macros for guid name
            SqlCommand ="""select DISTINCT Value, Modifier from Query where Name like '%s'""" % GuidName
            GuidMacroSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            # Ignore NULL result
            if not GuidMacroSet:
                continue
            GuidMacro = GuidMacroSet[0][0].strip()
            if not GuidMacro:
                continue
            # Find Guid value of Guid Macro
            SqlCommand ="""select DISTINCT Value from Query2 where Value like '%%%s%%' and Model = %s""" % (GuidMacro, MODEL_IDENTIFIER_MACRO_DEFINE)
            GuidValueSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            if GuidValueSet != []:
                GuidValue = GuidValueSet[0][0]
                GuidValue = GuidValue[GuidValue.find(GuidMacro) + len(GuidMacro) :]
                GuidValue = GuidValue.lower().replace('\\', '').replace('\r', '').replace('\n', '').replace('l', '').strip()
                GuidValue = GuidStructureStringToGuidString(GuidValue)
                SqlCommand = """update Report set GuidMacro = '%s', GuidValue = '%s' where GuidName = '%s'""" %(GuidMacro, GuidValue, GuidName)
                EotGlobalData.gDb.TblReport.Exec(SqlCommand)
                continue

        # Update Hard Coded Ppi/Protocol
        SqlCommand = """select DISTINCT GuidValue, ItemType from Report where ModuleID = -2 and ItemMode = 'Produced'"""
        RecordSet = EotGlobalData.gDb.TblReport.Exec(SqlCommand)
        for Record in RecordSet:
            if Record[1] == 'Ppi':
                EotGlobalData.gPpiList[Record[0].lower()] = -2
            if Record[1] == 'Protocol':
                EotGlobalData.gProtocolList[Record[0].lower()] = -2

    ## GenerateQueryTable() method
    #
    #  Generate two tables improve query performance
    #
    #  @param self: The object pointer
    #
    def GenerateQueryTable(self):
        EdkLogger.quiet("Generating temp query table for analysis ... ")
        for Identifier in EotGlobalData.gIdentifierTableList:
            SqlCommand = """insert into Query (Name, Modifier, Value, Model)
                            select Name, Modifier, Value, Model from %s where (Model = %s or Model = %s)""" \
                            % (Identifier[0], MODEL_IDENTIFIER_VARIABLE, MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION)
            EotGlobalData.gDb.TblReport.Exec(SqlCommand)
            SqlCommand = """insert into Query2 (Name, Modifier, Value, Model)
                            select Name, Modifier, Value, Model from %s where Model = %s""" \
                            % (Identifier[0], MODEL_IDENTIFIER_MACRO_DEFINE)
            EotGlobalData.gDb.TblReport.Exec(SqlCommand)

    ## ParseExecutionOrder() method
    #
    #  Get final execution order
    #  1. Search all PPI
    #  2. Search all PROTOCOL
    #
    #  @param self: The object pointer
    #
    def ParseExecutionOrder(self):
        EdkLogger.quiet("Searching Ppi/Protocol ... ")
        for Identifier in EotGlobalData.gIdentifierTableList:
            ModuleID, ModuleName, ModuleGuid, SourceFileID, SourceFileFullPath, ItemName, ItemType, ItemMode, GuidName, GuidMacro, GuidValue, BelongsToFunction, Enabled = \
            -1, '', '', -1, '', '', '', '', '', '', '', '', 0

            SourceFileID = Identifier[0].replace('Identifier', '')
            SourceFileFullPath = Identifier[1]
            Identifier = Identifier[0]

            # Find Ppis
            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallPpi', '->InstallPpi', 'PeiInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.ReInstallPpi', '->ReInstallPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 2)

            SearchPpiCallFunction(Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            ItemMode = 'Consumed'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.LocatePpi', '->LocatePpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Ppi', ItemMode)

            ItemMode = 'Callback'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.NotifyPpi', '->NotifyPpi', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchPpi(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode)

            # Find Protocols
            ItemMode = 'Produced'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallProtocolInterface', '.ReInstallProtocolInterface', '->InstallProtocolInterface', '->ReInstallProtocolInterface', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 1)

            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.InstallMultipleProtocolInterfaces', '->InstallMultipleProtocolInterfaces', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 2)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

            ItemMode = 'Consumed'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.LocateProtocol', '->LocateProtocol', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 0)

            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.HandleProtocol', '->HandleProtocol', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 1)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

            ItemMode = 'Callback'
            SqlCommand = """select Value, Name, BelongsToFile, StartLine, EndLine from %s
                            where (Name like '%%%s%%' or Name like '%%%s%%') and Model = %s""" \
                            % (Identifier, '.RegisterProtocolNotify', '->RegisterProtocolNotify', MODEL_IDENTIFIER_FUNCTION_CALLING)
            SearchProtocols(SqlCommand, Identifier, SourceFileID, SourceFileFullPath, ItemMode, 0)

            SearchFunctionCalling(Identifier, SourceFileID, SourceFileFullPath, 'Protocol', ItemMode)

        # Hard Code
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gEfiSecPlatformInformationPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gEfiNtLoadAsDllPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gNtPeiLoadFileGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiNtAutoScanPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gNtFwhPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiNtThunkPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiPlatformTypePpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiFrequencySelectionCpuPpiGuid', '', '', '', 0)
        EotGlobalData.gDb.TblReport.Insert(-2, '', '', -1, '', '', 'Ppi', 'Produced', 'gPeiCachePpiGuid', '', '', '', 0)

        EotGlobalData.gDb.Conn.commit()


    ## BuildDatabase() methoc
    #
    #  Build the database for target
    #
    #  @param self: The object pointer
    #
    def BuildDatabase(self):
        # Clean report table
        EotGlobalData.gDb.TblReport.Drop()
        EotGlobalData.gDb.TblReport.Create()

        # Build database
        if self.IsInit:
            self.BuildMetaDataFileDatabase(EotGlobalData.gINF_FILES)
            EdkLogger.quiet("Building database for source code ...")
            c.CreateCCodeDB(EotGlobalData.gSOURCE_FILES)
            EdkLogger.quiet("Building database for source code done!")

        EotGlobalData.gIdentifierTableList = GetTableList((MODEL_FILE_C, MODEL_FILE_H), 'Identifier', EotGlobalData.gDb)

    ## BuildMetaDataFileDatabase() method
    #
    #  Build the database for meta data files
    #
    #  @param self: The object pointer
    #  @param Inf_Files: A list for all INF files
    #
    def BuildMetaDataFileDatabase(self, Inf_Files):
        EdkLogger.quiet("Building database for meta data files ...")
        for InfFile in Inf_Files:
            if not InfFile:
                continue
            EdkLogger.quiet("Parsing %s ..."  % str(InfFile))
            EdkInfParser(InfFile, EotGlobalData.gDb, Inf_Files[InfFile])

        EotGlobalData.gDb.Conn.commit()
        EdkLogger.quiet("Building database for meta data files done!")

    ## ParseOption() method
    #
    #  Parse command line options
    #
    #  @param self: The object pointer
    #
    def ParseOption(self):
        (Options, Target) = self.EotOptionParser()

        # Set log level
        self.SetLogLevel(Options)

        if Options.FvFileList:
            self.FvFileList = Options.FvFileList

        if Options.MapFileList:
            self.MapFileList = Options.FvMapFileList

        if Options.SourceFileList:
            self.SourceFileList = Options.SourceFileList

        if Options.IncludeDirList:
            self.IncludeDirList = Options.IncludeDirList

        if Options.DecFileList:
            self.DecFileList = Options.DecFileList

        if Options.GuidList:
            self.GuidList = Options.GuidList

        if Options.LogFile:
            self.LogFile = Options.LogFile

        if Options.keepdatabase:
            self.IsInit = False

    ## SetLogLevel() method
    #
    #  Set current log level of the tool based on args
    #
    #  @param self: The object pointer
    #  @param Option: The option list including log level setting
    #
    def SetLogLevel(self, Option):
        if Option.verbose is not None:
            EdkLogger.SetLevel(EdkLogger.VERBOSE)
        elif Option.quiet is not None:
            EdkLogger.SetLevel(EdkLogger.QUIET)
        elif Option.debug is not None:
            EdkLogger.SetLevel(Option.debug + 1)
        else:
            EdkLogger.SetLevel(EdkLogger.INFO)

    ## EotOptionParser() method
    #
    #  Using standard Python module optparse to parse command line option of this tool.
    #
    #  @param self: The object pointer
    #
    #  @retval Opt   A optparse.Values object containing the parsed options
    #  @retval Args  Target of build command
    #
    def EotOptionParser(self):
        Parser = OptionParser(description = self.Copyright, version = self.Version, prog = "Eot.exe", usage = "%prog [options]")
        Parser.add_option("-m", "--makefile filename", action="store", type="string", dest='MakeFile',
            help="Specify a makefile for the platform.")
        Parser.add_option("-c", "--dsc filename", action="store", type="string", dest="DscFile",
            help="Specify a dsc file for the platform.")
        Parser.add_option("-f", "--fv filename", action="store", type="string", dest="FvFileList",
            help="Specify fv file list, quoted by \"\".")
        Parser.add_option("-a", "--map filename", action="store", type="string", dest="MapFileList",
            help="Specify map file list, quoted by \"\".")
        Parser.add_option("-s", "--source files", action="store", type="string", dest="SourceFileList",
            help="Specify source file list by a file")
        Parser.add_option("-i", "--include dirs", action="store", type="string", dest="IncludeDirList",
            help="Specify include dir list by a file")
        Parser.add_option("-e", "--dec files", action="store", type="string", dest="DecFileList",
            help="Specify dec file list by a file")
        Parser.add_option("-g", "--guid list", action="store", type="string", dest="GuidList",
            help="Specify guid file list by a file")
        Parser.add_option("-l", "--log filename", action="store", type="string", dest="LogFile",
            help="Specify real execution log file")

        Parser.add_option("-k", "--keepdatabase", action="store_true", type=None, help="The existing Eot database will not be cleaned except report information if this option is specified.")

        Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
        Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                                   "including library instances selected, final dependency expression, "\
                                                                                   "and warning messages, etc.")
        Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")

        (Opt, Args)=Parser.parse_args()

        return (Opt, Args)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    # Initialize log system
    EdkLogger.Initialize()
    EdkLogger.IsRaiseError = False
    EdkLogger.quiet(time.strftime("%H:%M:%S, %b.%d %Y ", time.localtime()) + "[00:00]" + "\n")

    StartTime = time.clock()
    Eot = Eot(CommandLineOption=False,
              SourceFileList=r'C:\TestEot\Source.txt',
              GuidList=r'C:\TestEot\Guid.txt',
              FvFileList=r'C:\TestEot\FVRECOVERY.Fv')
    FinishTime = time.clock()

    BuildDuration = time.strftime("%M:%S", time.gmtime(int(round(FinishTime - StartTime))))
    EdkLogger.quiet("\n%s [%s]" % (time.strftime("%H:%M:%S, %b.%d %Y", time.localtime()), BuildDuration))
