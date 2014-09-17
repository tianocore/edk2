## @file
# Parse FV image
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
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
import Common.LongFilePathOs as os
import re
import sys
import uuid
import struct
import codecs
import copy

from UserDict import IterableUserDict
from cStringIO import StringIO
from array import array
from Common.LongFilePathSupport import OpenLongFilePath as open
from CommonDataClass import *
from Common.Misc import sdict, GuidStructureStringToGuidString

import Common.EdkLogger as EdkLogger

import EotGlobalData

# Global definiton
gFfsPrintTitle  = "%-36s  %-21s %8s %8s %8s  %-4s %-36s" % ("GUID", "TYPE", "OFFSET", "SIZE", "FREE", "ALIGN", "NAME")
gFfsPrintFormat = "%36s  %-21s %8X %8X %8X  %4s %-36s"
gGuidStringFormat = "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"
gPeiAprioriFileNameGuid = '1b45cc0a-156a-428a-af62-49864da0e6e6'
gAprioriGuid = 'fc510ee7-ffdc-11d4-bd41-0080c73c8881'
gIndention = -4

## Image() class
#
#  A class for Image
#
class Image(array):
    _HEADER_ = struct.Struct("")
    _HEADER_SIZE_ = _HEADER_.size

    def __new__(cls, *args, **kwargs):
        return array.__new__(cls, 'B')

    def __init__(m, ID=None):
        if ID == None:
            m._ID_ = str(uuid.uuid1()).upper()
        else:
            m._ID_ = ID
        m._BUF_ = None
        m._LEN_ = None
        m._OFF_ = None

        m._SubImages = sdict() # {offset: Image()}

        array.__init__(m, 'B')

    def __repr__(m):
        return m._ID_

    def __len__(m):
        Len = array.__len__(m)
        for Offset in m._SubImages:
            Len += len(m._SubImages[Offset])
        return Len

    def _Unpack(m):
        m.extend(m._BUF_[m._OFF_ : m._OFF_ + m._LEN_])
        return len(m)

    def _Pack(m, PadByte=0xFF):
        raise NotImplementedError

    def frombuffer(m, Buffer, Offset=0, Size=None):
        m._BUF_ = Buffer
        m._OFF_ = Offset
        # we may need the Size information in advance if it's given
        m._LEN_ = Size
        m._LEN_ = m._Unpack()

    def empty(m):
        del m[0:]

    def GetField(m, FieldStruct, Offset=0):
        return FieldStruct.unpack_from(m, Offset)

    def SetField(m, FieldStruct, Offset, *args):
        # check if there's enough space
        Size = FieldStruct.size
        if Size > len(m):
            m.extend([0] * (Size - len(m)))
        FieldStruct.pack_into(m, Offset, *args)

    def _SetData(m, Data):
        if len(m) < m._HEADER_SIZE_:
            m.extend([0] * (m._HEADER_SIZE_ - len(m)))
        else:
            del m[m._HEADER_SIZE_:]
        m.extend(Data)

    def _GetData(m):
        if len(m) > m._HEADER_SIZE_:
            return m[m._HEADER_SIZE_:]
        return None

    Data = property(_GetData, _SetData)

## FirmwareVolume() class
#
#  A class for Firmware Volume
#
class FirmwareVolume(Image):
    # Read FvLength, Attributes, HeaderLength, Checksum
    _HEADER_ = struct.Struct("16x 1I2H8B 1Q 4x 1I 1H 1H")
    _HEADER_SIZE_ = _HEADER_.size

    _FfsGuid = "8C8CE578-8A3D-4F1C-9935-896185C32DD3"

    _GUID_      = struct.Struct("16x 1I2H8B")
    _LENGTH_    = struct.Struct("16x 16x 1Q")
    _SIG_       = struct.Struct("16x 16x 8x 1I")
    _ATTR_      = struct.Struct("16x 16x 8x 4x 1I")
    _HLEN_      = struct.Struct("16x 16x 8x 4x 4x 1H")
    _CHECKSUM_  = struct.Struct("16x 16x 8x 4x 4x 2x 1H")

    def __init__(self, Name=''):
        Image.__init__(self)
        self.Name = Name
        self.FfsDict = sdict()
        self.OrderedFfsDict = sdict()
        self.UnDispatchedFfsDict = sdict()
        self.NoDepexFfsDict = sdict()
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

    def Dispatch(self, Db = None):
        if Db == None:
            return False
        self.UnDispatchedFfsDict = copy.copy(self.FfsDict)
        # Find PeiCore, DexCore, PeiPriori, DxePriori first
        FfsSecCoreGuid = None
        FfsPeiCoreGuid = None
        FfsDxeCoreGuid = None
        FfsPeiPrioriGuid = None
        FfsDxePrioriGuid = None
        for FfsID in self.UnDispatchedFfsDict:
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
            if Ffs.Guid.lower() == gPeiAprioriFileNameGuid:
                FfsPeiPrioriGuid = FfsID
                continue
            if Ffs.Guid.lower() == gAprioriGuid:
                FfsDxePrioriGuid = FfsID
                continue

        # Parse SEC_CORE first
        if FfsSecCoreGuid != None:
            self.OrderedFfsDict[FfsSecCoreGuid] = self.UnDispatchedFfsDict.pop(FfsSecCoreGuid)
            self.LoadPpi(Db, FfsSecCoreGuid)

        # Parse PEI first
        if FfsPeiCoreGuid != None:
            self.OrderedFfsDict[FfsPeiCoreGuid] = self.UnDispatchedFfsDict.pop(FfsPeiCoreGuid)
            self.LoadPpi(Db, FfsPeiCoreGuid)
            if FfsPeiPrioriGuid != None:
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
        if FfsDxeCoreGuid != None:
            self.OrderedFfsDict[FfsDxeCoreGuid] = self.UnDispatchedFfsDict.pop(FfsDxeCoreGuid)
            self.LoadProtocol(Db, FfsDxeCoreGuid)
            if FfsDxePrioriGuid != None:
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

    def DisPatchNoDepexFfs(self, Db):
        # Last Load Drivers without Depex
        for FfsID in self.NoDepexFfsDict:
            NewFfs = self.NoDepexFfsDict.pop(FfsID)
            self.OrderedFfsDict[FfsID] = NewFfs
            self.LoadProtocol(Db, FfsID)

        return True

    def LoadCallbackProtocol(self):
        IsLoad = True
        for Protocol in self.ProtocolList:
            for Callback in self.ProtocolList[Protocol][1]:
                if Callback[0] not in self.OrderedFfsDict.keys():
                    IsLoad = False
                    continue
            if IsLoad:
                EotGlobalData.gProtocolList[Protocol.lower()] = self.ProtocolList[Protocol][0]
                self.ProtocolList.pop(Protocol)

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
        for FfsID in self.UnDispatchedFfsDict:
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
                    if FileDepex != None:
                        ScheduleList.insert.insert(FileDepex[1], FfsID, NewFfs, FileDepex[0])
                    else:
                        ScheduleList[FfsID] = NewFfs
                else:
                    self.UnDispatchedFfsDict[FfsID].Depex = DepexString

        for FfsID in ScheduleList:
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
        for FfsID in self.UnDispatchedFfsDict:
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
        FvInfo +=  "[FV:%s] file_system=%s size=%x checksum=%s\n" % (self.Name, self.FileSystemGuid, self.Size, self.Checksum)
        FfsInfo = "\n".join([str(self.FfsDict[FfsId]) for FfsId in self.FfsDict])
        gIndention -= 4
        return FvInfo + FfsInfo

    def _Unpack(self):
        Size = self._LENGTH_.unpack_from(self._BUF_, self._OFF_)[0]
        self.empty()
        self.extend(self._BUF_[self._OFF_:self._OFF_+Size])

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
                if LastFfsObj != None:
                    LastFfsObj.FreeSpace = EndOfFv - LastFfsObj._OFF_ - len(LastFfsObj)
            else:
                if FfsId in self.FfsDict:
                    EdkLogger.error("FV", 0, "Duplicate GUID in FFS",
                                    ExtraData="\t%s @ %s\n\t%s @ %s" \
                                    % (FfsObj.Guid, FfsObj.Offset,
                                       self.FfsDict[FfsId].Guid, self.FfsDict[FfsId].Offset))
                self.FfsDict[FfsId] = FfsObj
                if LastFfsObj != None:
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

    def __init__(m, CompressedData=None, CompressionType=None, UncompressedLength=None):
        Image.__init__(m)
        if UncompressedLength != None:
            m.UncompressedLength = UncompressedLength
        if CompressionType != None:
            m.CompressionType = CompressionType
        if CompressedData != None:
            m.Data = CompressedData

    def __str__(m):
        global gIndention
        S = "algorithm=%s uncompressed=%x" % (m.CompressionType, m.UncompressedLength)
        for Sec in m.Sections:
            S += '\n' + str(Sec)

        return S

    def _SetOriginalSize(m, Size):
        m.SetField(m._ORIG_SIZE_, 0, Size)

    def _GetOriginalSize(m):
        return m.GetField(m._ORIG_SIZE_)[0]

    def _SetCompressionType(m, Type):
        m.SetField(m._CMPRS_TYPE_, 0, Type)

    def _GetCompressionType(m):
        return m.GetField(m._CMPRS_TYPE_)[0]

    def _GetSections(m):
        try:
            import EfiCompressor
            TmpData = EfiCompressor.FrameworkDecompress(
                                        m[m._HEADER_SIZE_:],
                                        len(m) - m._HEADER_SIZE_
                                        )
            DecData = array('B')
            DecData.fromstring(TmpData)
        except:
            import EfiCompressor
            TmpData = EfiCompressor.UefiDecompress(
                                        m[m._HEADER_SIZE_:],
                                        len(m) - m._HEADER_SIZE_
                                        )
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

    def __init__(m, SectionDefinitionGuid=None, DataOffset=None, Attributes=None, Data=None):
        Image.__init__(m)
        if SectionDefinitionGuid != None:
            m.SectionDefinitionGuid = SectionDefinitionGuid
        if DataOffset != None:
            m.DataOffset = DataOffset
        if Attributes != None:
            m.Attributes = Attributes
        if Data != None:
            m.Data = Data

    def __str__(m):
        S = "guid=%s" % (gGuidStringFormat % m.SectionDefinitionGuid)
        for Sec in m.Sections:
            S += "\n" + str(Sec)
        return S

    def _Unpack(m):
        # keep header in this Image object
        m.empty()
        m.extend(m._BUF_[m._OFF_ : m._OFF_ + m._LEN_])
        return len(m)

    def _SetAttribute(m, Attribute):
        m.SetField(m._ATTR_, 0, Attribute)

    def _GetAttribute(m):
        return m.GetField(m._ATTR_)[0]

    def _SetGuid(m, Guid):
        m.SetField(m._GUID_, 0, Guid)

    def _GetGuid(m):
        return m.GetField(m._GUID_)

    def _SetDataOffset(m, Offset):
        m.SetField(m._DATA_OFFSET_, 0, Offset)

    def _GetDataOffset(m):
        return m.GetField(m._DATA_OFFSET_)[0]

    def _GetSections(m):
        SectionList = []
        Guid = gGuidStringFormat % m.SectionDefinitionGuid
        if Guid == m.CRC32_GUID:
            # skip the CRC32 value, we don't do CRC32 verification here
            Offset = m.DataOffset - 4
            while Offset < len(m):
                Sec = Section()
                try:
                    Sec.frombuffer(m, Offset)
                    Offset += Sec.Size
                    # the section is aligned to 4-byte boundary
                    Offset = (Offset + 3) & (~3)
                except:
                    break
                SectionList.append(Sec)
        elif Guid == m.TIANO_COMPRESS_GUID:
            try:
                import EfiCompressor
                # skip the header
                Offset = m.DataOffset - 4
                TmpData = EfiCompressor.FrameworkDecompress(m[Offset:], len(m)-Offset)
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
        elif Guid == m.LZMA_COMPRESS_GUID:
            try:
                import LzmaCompressor
                # skip the header
                Offset = m.DataOffset - 4
                TmpData = LzmaCompressor.LzmaDecompress(m[Offset:], len(m)-Offset)
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

    def __init__(m):
        Image.__init__(m)
        m._ExprList = []

    def __str__(m):
        global gIndention
        gIndention += 4
        Indention = ' ' * gIndention
        S = '\n'
        for T in m.Expression:
            if T in m._OPCODE_STRING_:
                S += Indention + m._OPCODE_STRING_[T]
                if T not in [0x00, 0x01, 0x02]:
                    S += '\n'
            else:
                S += ' ' + gGuidStringFormat % T + '\n'
        gIndention -= 4
        return S

    def _Unpack(m):
        # keep header in this Image object
        m.empty()
        m.extend(m._BUF_[m._OFF_ : m._OFF_ + m._LEN_])
        return len(m)

    def _GetExpression(m):
        if m._ExprList == []:
            Offset = 0
            CurrentData = m._OPCODE_
            while Offset < len(m):
                Token = CurrentData.unpack_from(m, Offset)
                Offset += CurrentData.size
                if len(Token) == 1:
                    Token = Token[0]
                    if Token in m._NEXT_:
                        CurrentData = m._NEXT_[Token]
                    else:
                        CurrentData = m._GUID_
                else:
                    CurrentData = m._OPCODE_
                m._ExprList.append(Token)
                if CurrentData == None:
                    break
        return m._ExprList

    Expression = property(_GetExpression)

## Ui() class
#
#  A class for Ui
#
class Ui(Image):
    _HEADER_ = struct.Struct("")
    _HEADER_SIZE_ = 0

    def __init__(m):
        Image.__init__(m)

    def __str__(m):
        return m.String

    def _Unpack(m):
        # keep header in this Image object
        m.empty()
        m.extend(m._BUF_[m._OFF_ : m._OFF_ + m._LEN_])
        return len(m)

    def _GetUiString(m):
        return codecs.utf_16_decode(m[0:-2].tostring())[0]

    String = property(_GetUiString)

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

    def __init__(m, Type=None, Size=None):
        Image.__init__(m)
        m._Alignment = 1
        if Type != None:
            m.Type = Type
        if Size != None:
            m.Size = Size

    def __str__(m):
        global gIndention
        gIndention += 4
        SectionInfo = ' ' * gIndention
        if m.Type in m._TypeName:
            SectionInfo += "[SECTION:%s] offset=%x size=%x" % (m._TypeName[m.Type], m._OFF_, m.Size)
        else:
            SectionInfo += "[SECTION:%x<unknown>] offset=%x size=%x " % (m.Type, m._OFF_, m.Size)
        for Offset in m._SubImages:
            SectionInfo += ", " + str(m._SubImages[Offset])
        gIndention -= 4
        return SectionInfo

    def _Unpack(m):
        m.empty()
        Type, = m._TYPE_.unpack_from(m._BUF_, m._OFF_)
        Size1, Size2, Size3 = m._SIZE_.unpack_from(m._BUF_, m._OFF_)
        Size = Size1 + (Size2 << 8) + (Size3 << 16)

        if Type not in m._SectionSubImages:
            # no need to extract sub-image, keep all in this Image object
            m.extend(m._BUF_[m._OFF_ : m._OFF_ + Size])
        else:
            # keep header in this Image object
            m.extend(m._BUF_[m._OFF_ : m._OFF_ + m._HEADER_SIZE_])
            #
            # use new Image object to represent payload, which may be another kind
            # of image such as PE32
            #
            PayloadOffset = m._HEADER_SIZE_
            PayloadLen = m.Size - m._HEADER_SIZE_
            Payload = m._SectionSubImages[m.Type]()
            Payload.frombuffer(m._BUF_, m._OFF_ + m._HEADER_SIZE_, PayloadLen)
            m._SubImages[PayloadOffset] = Payload

        return Size

    def _SetSize(m, Size):
        Size1 = Size & 0xFF
        Size2 = (Size & 0xFF00) >> 8
        Size3 = (Size & 0xFF0000) >> 16
        m.SetField(m._SIZE_, 0, Size1, Size2, Size3)

    def _GetSize(m):
        Size1, Size2, Size3 = m.GetField(m._SIZE_)
        return Size1 + (Size2 << 8) + (Size3 << 16)

    def _SetType(m, Type):
        m.SetField(m._TYPE_, 0, Type)

    def _GetType(m):
        return m.GetField(m._TYPE_)[0]

    def _GetAlignment(m):
        return m._Alignment

    def _SetAlignment(m, Alignment):
        m._Alignment = Alignment
        AlignmentMask = Alignment - 1
        # section alignment is actually for payload, so we need to add header size
        PayloadOffset = m._OFF_ + m._HEADER_SIZE_
        if (PayloadOffset & (~AlignmentMask)) == 0:
            return
        NewOffset = (PayloadOffset + AlignmentMask) & (~AlignmentMask)
        while (NewOffset - PayloadOffset) < m._HEADER_SIZE_:
            NewOffset += m._Alignment

    def tofile(m, f):
        m.Size = len(m)
        Image.tofile(m, f)
        for Offset in m._SubImages:
            m._SubImages[Offset].tofile(f)

    Type = property(_GetType, _SetType)
    Size = property(_GetSize, _SetSize)
    Alignment = property(_GetAlignment, _SetAlignment)
    # SubTypeGuid = property(_GetGuid, _SetGuid)

## PadSection() class
#
#  A class for Pad Section
#
class PadSection(Section):
    def __init__(m, Size):
        Section.__init__(m)
        m.Type = 0x19
        m.Size = Size
        m.Data = [0] * (Size - m._HEADER_SIZE_)

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

    VTF_GUID = "1BA0062E-C779-4582-8566-336AE8F78F09"

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
        SectionInfo = '\n'.join([str(self.Sections[Offset]) for Offset in self.Sections])
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

    def _SetSize(m, Size):
        Size1 = Size & 0xFF
        Size2 = (Size & 0xFF00) >> 8
        Size3 = (Size & 0xFF0000) >> 16
        m.SetField(m._SIZE_, 0, Size1, Size2, Size3)

    def _GetSize(m):
        Size1, Size2, Size3 = m.GetField(m._SIZE_)
        return Size1 + (Size2 << 8) + (Size3 << 16)

    def _SetType(m, Type):
        m.SetField(m._TYPE_, 0, Type)

    def _GetType(m):
        return m.GetField(m._TYPE_)[0]

    def _SetAttributes(self, Value):
        self.SetField(m._ATTR_, 0, Value)

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
        self.SetField(m._STATE_, 0, Value)

    def _GetState(self):
        return self.GetField(m._STATE_)[0]

    Name = property(_GetName, _SetName)
    Guid = property(_GetGuid)
    Type = property(_GetType, _SetType)
    Size = property(_GetSize, _SetSize)
    Attributes = property(_GetAttributes, _SetAttributes)
    Fixed = property(_GetFixed)
    Checksum = property(_GetCheckSum)
    Alignment = property(_GetAlignment)
    State = property(_GetState, _SetState)

## PeImage() class
#
#  A class for PE Image
#
class PeImage:
    #
    # just extract e_lfanew
    #
    _DosHeaderFormat = "60x 1I"
    #
    # Machine
    # NumberOfSections
    # SizeOfOptionalHeader
    #
    _FileHeaderFormat = "4x 1H 1H 4x 4x 4x 1H 2x"
    #
    # Magic
    # SizeOfImage
    # SizeOfHeaders
    # CheckSum
    # NumberOfRvaAndSizes
    #
    _OptionalHeader32Format = "1H 54x 1I 1I 1I 24x 1I"
    _OptionalHeader64Format = ""
    def __init__(self, Buf, Offset, Size):
        self.Offset = Offset
        self.Size = Size
        self.Machine = 0x014c # IA32
        self.NumberOfSections = 0
        self.SizeOfImage = 0
        self.SizeOfOptionalHeader = 0
        self.Checksum = 0
        self._PeImageBuf = Buf
        self._SectionList = []

        self._DosHeader = struct.Struct(PeImage._DosHeaderFormat)
        self._FileHeader = struct.Struct(PeImage._FileHeaderFormat)
        self._OptionalHeader32 = struct.Struct(PeImage._OptionalHeader32Format)

        self.Buffer = None

        self._Unpack()

    def __str__(self):
        pass

    def __len__(self):
        return self.Size

    def _Unpack(self):
        # from DOS header, get the offset of PE header
        FileHeaderOffset, = self._DosHeader.unpack_from(self._PeImageBuf, self.Offset)
        if FileHeaderOffset < struct.calcsize(self._DosHeaderFormat):
            EdkLogger.error("PE+", 0, "Invalid offset of IMAGE_FILE_HEADER: %s" % FileHeaderOffset)

        # from FILE header, get the optional header size
        self.Machine, self.NumberOfSections, self.SizeOfOptionalHeader = \
            self._FileHeader.unpack_from(self._PeImageBuf, self.Offset + FileHeaderOffset)

        print "Machine=%x NumberOfSections=%x SizeOfOptionalHeader=%x" % (self.Machine, self.NumberOfSections, self.SizeOfOptionalHeader)
        # optional header follows the FILE header
        OptionalHeaderOffset = FileHeaderOffset + struct.calcsize(self._FileHeaderFormat)
        Magic, self.SizeOfImage, SizeOfHeaders, self.Checksum, NumberOfRvaAndSizes = \
            self._OptionalHeader32.unpack_from(self._PeImageBuf, self.Offset + OptionalHeaderOffset)
        print "Magic=%x SizeOfImage=%x SizeOfHeaders=%x, Checksum=%x, NumberOfRvaAndSizes=%x" % (Magic, self.SizeOfImage, SizeOfHeaders, self.Checksum, NumberOfRvaAndSizes)

        PeImageSectionTableOffset = OptionalHeaderOffset + self.SizeOfOptionalHeader
        PeSections = PeSectionTable(self._PeImageBuf, self.Offset + PeImageSectionTableOffset, self.NumberOfSections)

        print "%x" % PeSections.GetFileAddress(0x3920)

## PeSectionTable() class
#
#  A class for PE Section Table
#
class PeSectionTable:
    def __init__(self, Buf, Offset, NumberOfSections):
        self._SectionList = []

        SectionHeaderOffset = Offset
        for TableIndex in range(0, NumberOfSections):
            SectionHeader = PeSectionHeader(Buf, SectionHeaderOffset)
            self._SectionList.append(SectionHeader)
            SectionHeaderOffset += len(SectionHeader)
            print SectionHeader

    def GetFileAddress(self, Rva):
        for PeSection in self._SectionList:
            if Rva in PeSection:
                return PeSection[Rva]

## PeSectionHeader() class
#
#  A class for PE Section Header
#
class PeSectionHeader:
    #
    # VirtualAddress
    # SizeOfRawData
    # PointerToRawData
    #
    _HeaderFormat = "12x 1I 1I 1I 16x"
    _HeaderLength = struct.calcsize(_HeaderFormat)

    def __init__(self, Buf, Offset):
        self.VirtualAddressStart, self.SizeOfRawData, self.PointerToRawData = \
            struct.unpack_from(self._HeaderFormat, Buf, Offset)
        self.VirtualAddressEnd = self.VirtualAddressStart + self.SizeOfRawData - 1

    def __str__(self):
        return "VirtualAddress=%x, SizeOfRawData=%x, PointerToRawData=%x" % (self.VirtualAddressStart, self.SizeOfRawData, self.PointerToRawData)

    def __len__(self):
        return self._HeaderLength

    def __contains__(self, Rva):
        return Rva >= self.VirtualAddressStart and Rva <= self.VirtualAddressEnd

    def __getitem__(self, Rva):
        return Rva - self.VirtualAddressStart + self.PointerToRawData

## LinkMap() class
#
#  A class for Link Map
#
class LinkMap:
    _StartFlag = {
        "MSFT"  :   re.compile("Address +Publics by Value +Rva\+Base +Lib:Object"),
        "GCC"   :   re.compile("^\.(text|bss|data|edata)"),
    }

    _MappingFormat = {
        "MSFT"  :   re.compile("([0-9a-f]+):([0-9a-f]+)\s+_+([0-9A-Za-z]+)\s+([0-9a-f]+)\s+"),
        "GCC"   :   re.compile("^(\.\w)?\s+(0x[0-9a-f]+)\s+_+([0-9A-Za-z]+)"),
    }

    def __init__(self, MapFile, MapType="MSFT"):
        self.File = MapFile
        self.MapType = MapType
        self._Globals = {}  # global:RVA

        self._Parse()

    def _Parse(self):
        MapFile = open(self.File, 'r')
        MappingTitle = self._StartFlag[self.MapType]
        MappingFormat = self._MappingFormat[self.MapType]
        MappingStart = False
        try:
            for Line in MapFile:
                Line = Line.strip()
                if not MappingStart:
                    if MappingTitle.match(Line) != None:
                        MappingStart = True
                    continue
                ResultList = MappingFormat.findall(Line)
                if len(ResultList) == 0 or len(ResultList[0]) != 4:
                    continue
                self._Globals[ResultList[2]] = int(ResultList[3], 16)
                EdkLogger.verbose(ResultList[0])
        finally:
            MapFile.close()

    def __contains__(self, Var):
        return Var in self._Globals

    def __getitem__(self, Var):
        if Var not in self._Globals:
            return None
        return self._Globals[Var]

## MultipleFv() class
#
#  A class for Multiple FV
#
class MultipleFv(FirmwareVolume):
    def __init__(self, FvList):
        FirmwareVolume.__init__(self)
        self.BasicInfo = []
        for FvPath in FvList:
            FvName = os.path.splitext(os.path.split(FvPath)[1])[0]
            Fd = open(FvPath, 'rb')
            Buf = array('B')
            try:
                Buf.fromfile(Fd, os.path.getsize(FvPath))
            except EOFError:
                pass

            Fv = FirmwareVolume(FvName)
            Fv.frombuffer(Buf, 0, len(Buf))

            self.BasicInfo.append([Fv.Name, Fv.FileSystemGuid, Fv.Size])
            self.FfsDict.append(Fv.FfsDict)

# Version and Copyright
__version_number__ = "0.01"
__version__ = "%prog Version " + __version_number__
__copyright__ = "Copyright (c) 2008, Intel Corporation. All rights reserved."

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
# @retval Options   A optparse.Values object containing the parsed options
# @retval InputFile Path of file to be trimmed
#
def GetOptions():
    OptionList = [
        make_option("-a", "--arch", dest="Arch",
                          help="The input file is preprocessed source code, including C or assembly code"),
        make_option("-p", "--platform", dest="ActivePlatform",
                          help="The input file is preprocessed VFR file"),
        make_option("-m", "--module", dest="ActiveModule",
                          help="Convert standard hex format (0xabcd) to MASM format (abcdh)"),
        make_option("-f", "--FDF-file", dest="FdfFile",
                          help="Convert standard hex format (0xabcd) to MASM format (abcdh)"),
        make_option("-o", "--output", dest="OutputDirectory",
                          help="File to store the trimmed content"),
        make_option("-t", "--toolchain-tag", dest="ToolChain",
                          help=""),
        make_option("-k", "--msft", dest="MakefileType", action="store_const", const="nmake",
                          help=""),
        make_option("-g", "--gcc", dest="MakefileType", action="store_const", const="gmake",
                          help=""),
        make_option("-v", "--verbose", dest="LogLevel", action="store_const", const=EdkLogger.VERBOSE,
                          help="Run verbosely"),
        make_option("-d", "--debug", dest="LogLevel", type="int",
                          help="Run with debug information"),
        make_option("-q", "--quiet", dest="LogLevel", action="store_const", const=EdkLogger.QUIET,
                          help="Run quietly"),
        make_option("-?", action="help", help="show this help message and exit"),
    ]

    # use clearer usage to override default usage message
    UsageString = "%prog [-a ARCH] [-p PLATFORM] [-m MODULE] [-t TOOLCHAIN_TAG] [-k] [-g] [-v|-d <debug_level>|-q] [-o <output_directory>] [GenC|GenMake]"

    Parser = OptionParser(description=__copyright__, version=__version__, option_list=OptionList, usage=UsageString)
    Parser.set_defaults(Arch=[])
    Parser.set_defaults(ActivePlatform=None)
    Parser.set_defaults(ActiveModule=None)
    Parser.set_defaults(OutputDirectory="build")
    Parser.set_defaults(FdfFile=None)
    Parser.set_defaults(ToolChain="MYTOOLS")
    if sys.platform == "win32":
        Parser.set_defaults(MakefileType="nmake")
    else:
        Parser.set_defaults(MakefileType="gmake")
    Parser.set_defaults(LogLevel=EdkLogger.INFO)

    Options, Args = Parser.parse_args()

    # error check
    if len(Args) == 0:
        Options.Target = "genmake"
        sys.argv.append("genmake")
    elif len(Args) == 1:
        Options.Target = Args[0].lower()
        if Options.Target not in ["genc", "genmake"]:
            EdkLogger.error("AutoGen", OPTION_NOT_SUPPORTED, "Not supported target",
                            ExtraData="%s\n\n%s" % (Options.Target, Parser.get_usage()))
    else:
        EdkLogger.error("AutoGen", OPTION_NOT_SUPPORTED, "Too many targets",
                        ExtraData=Parser.get_usage())

    return Options

## Entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @retval 0     Tool was successful
# @retval 1     Tool failed
#
def Main():
    from build import build
    try:
        Option = GetOptions()
        build.main()
    except Exception, e:
        print e
        return 1

    return 0

# This acts like the main() function for the script, unless it is 'import'ed into another script.
if __name__ == '__main__':
    EdkLogger.Initialize()
    # sys.exit(Main())

    if len(sys.argv) > 1:
        FilePath = sys.argv[1]
        if FilePath.lower().endswith(".fv"):
            fd = open(FilePath, 'rb')
            buf = array('B')
            try:
                buf.fromfile(fd, os.path.getsize(FilePath))
            except EOFError:
                pass

            fv = FirmwareVolume("FVRECOVERY")
            fv.frombuffer(buf, 0, len(buf))
            #fv.Dispatch(None)
            print fv
        elif FilePath.endswith(".efi"):
            fd = open(FilePath, 'rb')
            buf = array('B')
            Size = os.path.getsize(FilePath)

            try:
                buf.fromfile(fd, Size)
            except EOFError:
                pass

            PeSection = Section(Type=0x10)
            PeSection.Data = buf
            sf, ext = os.path.splitext(os.path.basename(FilePath))
            sf += ".sec"
            PeSection.tofile(open(sf, 'wb'))
        elif FilePath.endswith(".map"):
            mf = LinkMap(FilePath)
