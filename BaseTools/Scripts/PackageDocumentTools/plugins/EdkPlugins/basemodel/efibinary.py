## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import print_function
import array
import uuid
import re
import os
import logging
import core.pe as pe

def GetLogger():
    return logging.getLogger('EFI Binary File')

class EFIBinaryError(Exception):
    def __init__(self, message):
        Exception.__init__(self)
        self._message = message

    def GetMessage(self):
        return self._message

class EfiFd(object):
    EFI_FV_HEADER_SIZE = 0x48

    def __init__(self):
        self._fvs = []

    def Load(self, fd, size):
        index = fd.tell()
        while (index + self.EFI_FV_HEADER_SIZE < size):
            fv = EfiFv(self)
            fv.Load(fd)
            self._fvs.append(fv)
            index += fv.GetHeader().GetFvLength()
            index = align(index, 8)
            fd.seek(index)

    def GetFvs(self):
        return self._fvs

class EfiFv(object):
    FILE_SYSTEM_GUID = uuid.UUID('{8c8ce578-8a3d-4f1c-9935-896185c32dd3}')

    def __init__(self, parent=None):
        self._size         = 0
        self._filename     = None
        self._fvheader     = None
        self._blockentries = []
        self._ffs          = []

        # following field is for FV in FD
        self._parent       = parent
        self._offset       = 0
        self._raw          = array.array('B')

    def Load(self, fd):
        self._offset   = fd.tell()
        self._filename = fd.name

        # get file header
        self._fvheader = EfiFirmwareVolumeHeader.Read(fd)
        #self._fvheader.Dump()

        self._size = self._fvheader.GetFvLength()

        if self._fvheader.GetFileSystemGuid() != self.FILE_SYSTEM_GUID:
            fd.seek(self._offset)
            self._raw.fromfile(fd, self.GetHeader().GetFvLength())
            return

        # read block map
        blockentry = BlockMapEntry.Read(fd)
        self._blockentries.append(blockentry)
        while (blockentry.GetNumberBlocks() != 0 and blockentry.GetLength() != 0):
            self._blockentries.append(blockentry)
            blockentry = BlockMapEntry.Read(fd)


        if self._fvheader.GetSize() + (len(self._blockentries)) * 8 != \
           self._fvheader.GetHeaderLength():
            raise EFIBinaryError("Volume Header length not consistent with block map!")

        index = align(fd.tell(), 8)
        count = 0
        while ((index + EfiFfs.FFS_HEADER_SIZE) < self._size):
            ffs = EfiFfs.Read(fd, self)
            if not isValidGuid(ffs.GetNameGuid()):
                break
            self._ffs.append(ffs)
            count += 1
            index = align(fd.tell(), 8)

        fd.seek(self._offset)
        self._raw.fromfile(fd, self.GetHeader().GetFvLength())

    def GetFfs(self):
        return self._ffs

    def GetHeader(self):
        return self._fvheader

    def GetBlockEntries(self):
        return self._blockentries

    def GetHeaderRawData(self):
        ret = []
        ret += self._fvheader.GetRawData()
        for block in self._blockentries:
            ret += block.GetRawData()
        return ret

    def GetOffset(self):
        return 0

    def GetRawData(self):
        return self._raw.tolist()

class BinaryItem(object):
    def __init__(self, parent=None):
        self._size = 0
        self._arr  = array.array('B')
        self._parent = parent

    @classmethod
    def Read(cls, fd, parent=None):
        item = cls(parent)
        item.fromfile(fd)
        return item

    def Load(self, fd):
        self.fromfile(fd)

    def GetSize(self):
        """should be implemented by inherited class"""

    def fromfile(self, fd):
        self._arr.fromfile(fd, self.GetSize())

    def GetParent(self):
        return self._parent

class EfiFirmwareVolumeHeader(BinaryItem):
    def GetSize(self):
        return 56

    def GetSigunature(self):
        list = self._arr.tolist()
        sig = ''
        for x in list[40:44]:
            sig += chr(x)
        return sig

    def GetAttribute(self):
        return list2int(self._arr.tolist()[44:48])

    def GetErasePolarity(self):
        list = self.GetAttrStrings()
        if 'EFI_FVB2_ERASE_POLARITY' in list:
            return True
        return False

    def GetAttrStrings(self):
        list = []
        value = self.GetAttribute()
        if (value & 0x01) != 0:
            list.append('EFI_FVB2_READ_DISABLED_CAP')
        if (value & 0x02) != 0:
            list.append('EFI_FVB2_READ_ENABLED_CAP')
        if (value & 0x04) != 0:
            list.append('EFI_FVB2_READ_STATUS')
        if (value & 0x08) != 0:
            list.append('EFI_FVB2_WRITE_DISABLED_CAP')
        if (value & 0x10) != 0:
            list.append('EFI_FVB2_WRITE_ENABLED_CAP')
        if (value & 0x20) != 0:
            list.append('EFI_FVB2_WRITE_STATUS')
        if (value & 0x40) != 0:
            list.append('EFI_FVB2_LOCK_CAP')
        if (value & 0x80) != 0:
            list.append('EFI_FVB2_LOCK_STATUS')
        if (value & 0x200) != 0:
            list.append('EFI_FVB2_STICKY_WRITE')
        if (value & 0x400) != 0:
            list.append('EFI_FVB2_MEMORY_MAPPED')
        if (value & 0x800) != 0:
            list.append('EFI_FVB2_ERASE_POLARITY')
        if (value & 0x1000) != 0:
            list.append('EFI_FVB2_READ_LOCK_CAP')
        if (value & 0x00002000) != 0:
            list.append('EFI_FVB2_READ_LOCK_STATUS')
        if (value & 0x00004000) != 0:
            list.append('EFI_FVB2_WRITE_LOCK_CAP')
        if (value & 0x00008000) != 0:
            list.append('EFI_FVB2_WRITE_LOCK_STATUS')

        if (value == 0):
            list.append('EFI_FVB2_ALIGNMENT_1')
        if (value & 0x001F0000) == 0x00010000:
            list.append('EFI_FVB2_ALIGNMENT_2')
        if (value & 0x001F0000) == 0x00020000:
            list.append('EFI_FVB2_ALIGNMENT_4')
        if (value & 0x001F0000) == 0x00030000:
            list.append('EFI_FVB2_ALIGNMENT_8')
        if (value & 0x001F0000) == 0x00040000:
            list.append('EFI_FVB2_ALIGNMENT_16')
        if (value & 0x001F0000) == 0x00050000:
            list.append('EFI_FVB2_ALIGNMENT_32')
        if (value & 0x001F0000) == 0x00060000:
            list.append('EFI_FVB2_ALIGNMENT_64')
        if (value & 0x001F0000) == 0x00070000:
            list.append('EFI_FVB2_ALIGNMENT_128')
        if (value & 0x001F0000) == 0x00080000:
            list.append('EFI_FVB2_ALIGNMENT_256')
        if (value & 0x001F0000) == 0x00090000:
            list.append('EFI_FVB2_ALIGNMENT_512')
        if (value & 0x001F0000) == 0x000A0000:
            list.append('EFI_FVB2_ALIGNMENT_1K')
        if (value & 0x001F0000) == 0x000B0000:
            list.append('EFI_FVB2_ALIGNMENT_2K')
        if (value & 0x001F0000) == 0x000C0000:
            list.append('EFI_FVB2_ALIGNMENT_4K')
        if (value & 0x001F0000) == 0x000D0000:
            list.append('EFI_FVB2_ALIGNMENT_8K')
        if (value & 0x001F0000) == 0x000E0000:
            list.append('EFI_FVB2_ALIGNMENT_16K')
        if (value & 0x001F0000) == 0x000F0000:
            list.append('EFI_FVB2_ALIGNMENT_32K')
        if (value & 0x001F0000) == 0x00100000:
            list.append('EFI_FVB2_ALIGNMENT_64K')
        if (value & 0x001F0000) == 0x00110000:
            list.append('EFI_FVB2_ALIGNMENT_128K')
        if (value & 0x001F0000) == 0x00120000:
            list.append('EFI_FVB2_ALIGNMENT_256K')
        if (value & 0x001F0000) == 0x00130000:
            list.append('EFI_FVB2_ALIGNMENT_512K')

        return list

    def GetHeaderLength(self):
        return list2int(self._arr.tolist()[48:50])

    def Dump(self):
        print('Signature: %s' % self.GetSigunature())
        print('Attribute: 0x%X' % self.GetAttribute())
        print('Header Length: 0x%X' % self.GetHeaderLength())
        print('File system Guid: ', self.GetFileSystemGuid())
        print('Revision: 0x%X' % self.GetRevision())
        print('FvLength: 0x%X' % self.GetFvLength())

    def GetFileSystemGuid(self):
        list = self._arr.tolist()
        return list2guid(list[16:32])

    def GetRevision(self):
        list = self._arr.tolist()
        return int(list[55])

    def GetFvLength(self):
        list = self._arr.tolist()
        return list2int(list[32:40])

    def GetRawData(self):
        return self._arr.tolist()

class BlockMapEntry(BinaryItem):
    def GetSize(self):
        return 8

    def GetNumberBlocks(self):
        list = self._arr.tolist()
        return list2int(list[0:4])

    def GetLength(self):
        list = self._arr.tolist()
        return list2int(list[4:8])

    def GetRawData(self):
        return self._arr.tolist()

    def __str__(self):
        return '[BlockEntry] Number = 0x%X, length=0x%X' % (self.GetNumberBlocks(), self.GetLength())

class EfiFfs(object):
    FFS_HEADER_SIZE  = 24

    def __init__(self, parent=None):
        self._header = None

        # following field is for FFS in FV file.
        self._parent  = parent
        self._offset  = 0
        self._sections = []

    def Load(self, fd):
        self._offset = align(fd.tell(), 8)

        self._header = EfiFfsHeader.Read(fd, self)

        if not isValidGuid(self.GetNameGuid()):
            return

        index = self._offset
        fileend = self._offset + self.GetSize()
        while (index + EfiSection.EFI_SECTION_HEADER_SIZE < fileend):
            section = EfiSection(self)
            section.Load(fd)
            if section.GetSize() == 0 and section.GetHeader().GetType() == 0:
                break
            self._sections.append(section)
            index = fd.tell()

        # rebase file pointer to next ffs file
        index = self._offset + self._header.GetFfsSize()
        index = align(index, 8)
        fd.seek(index)

    def GetOffset(self):
        return self._offset

    def GetSize(self):
        return self._header.GetFfsSize()

    @classmethod
    def Read(cls, fd, parent=None):
        item = cls(parent)
        item.Load(fd)
        return item

    def GetNameGuid(self):
        return self._header.GetNameGuid()

    def DumpContent(self):
        list  = self._content.tolist()
        line  = []
        count = 0
        for item in list:
            if count < 32:
                line.append('0x%X' % int(item))
                count += 1
            else:
                print(' '.join(line))
                count = 0
                line = []
                line.append('0x%X' % int(item))
                count += 1

    def GetHeader(self):
        return self._header

    def GetParent(self):
        return self._parent

    def GetSections(self):
        return self._sections

class EfiFfsHeader(BinaryItem):
    ffs_state_map = {0x01:'EFI_FILE_HEADER_CONSTRUCTION',
                     0x02:'EFI_FILE_HEADER_VALID',
                     0x04:'EFI_FILE_DATA_VALID',
                     0x08:'EFI_FILE_MARKED_FOR_UPDATE',
                     0x10:'EFI_FILE_DELETED',
                     0x20:'EFI_FILE_HEADER_INVALID'}

    def GetSize(self):
        return 24

    def GetNameGuid(self):
        list = self._arr.tolist()
        return list2guid(list[0:16])

    def GetType(self):
        list = self._arr.tolist()
        return int(list[18])


    def GetTypeString(self):
        value = self.GetType()
        if value == 0x01:
            return 'EFI_FV_FILETYPE_RAW'
        if value == 0x02:
            return 'EFI_FV_FILETYPE_FREEFORM'
        if value == 0x03:
            return 'EFI_FV_FILETYPE_SECURITY_CORE'
        if value == 0x04:
            return 'EFI_FV_FILETYPE_PEI_CORE'
        if value == 0x05:
            return 'EFI_FV_FILETYPE_DXE_CORE'
        if value == 0x06:
            return 'EFI_FV_FILETYPE_PEIM'
        if value == 0x07:
            return 'EFI_FV_FILETYPE_DRIVER'
        if value == 0x08:
            return 'EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER'
        if value == 0x09:
            return 'EFI_FV_FILETYPE_APPLICATION'
        if value == 0x0B:
            return 'EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE'
        if value == 0xc0:
            return 'EFI_FV_FILETYPE_OEM_MIN'
        if value == 0xdf:
            return 'EFI_FV_FILETYPE_OEM_MAX'
        if value == 0xe0:
            return 'EFI_FV_FILETYPE_DEBUG_MIN'
        if value == 0xef:
            return 'EFI_FV_FILETYPE_DEBUG_MAX'
        if value == 0xf0:
            return 'EFI_FV_FILETYPE_FFS_PAD'
        if value == 0xff:
            return 'EFI_FV_FILETYPE_FFS_MAX'
        return 'Unknown FFS Type'

    def GetAttributes(self):
        list = self._arr.tolist()
        return int(list[19])

    def GetFfsSize(self):
        list = self._arr.tolist()
        return list2int(list[20:23])

    def GetState(self):
        list = self._arr.tolist()
        state = int(list[23])
        polarity = self.GetParent().GetParent().GetHeader().GetErasePolarity()
        if polarity:
            state = (~state) & 0xFF
        HighestBit = 0x80
        while (HighestBit != 0) and (HighestBit & state) == 0:
            HighestBit = HighestBit >> 1
        return HighestBit

    def GetStateString(self):
        state = self.GetState()
        if state in self.ffs_state_map.keys():
            return self.ffs_state_map[state]
        return 'Unknown Ffs State'

    def Dump(self):
        print("FFS name: ", self.GetNameGuid())
        print("FFS type: ", self.GetType())
        print("FFS attr: 0x%X" % self.GetAttributes())
        print("FFS size: 0x%X" % self.GetFfsSize())
        print("FFS state: 0x%X" % self.GetState())

    def GetRawData(self):
        return self._arr.tolist()


class EfiSection(object):
    EFI_SECTION_HEADER_SIZE = 4

    def __init__(self, parent=None):
        self._size   = 0
        self._parent = parent
        self._offset = 0
        self._contents = array.array('B')

    def Load(self, fd):
        self._offset = align(fd.tell(), 4)

        self._header = EfiSectionHeader.Read(fd, self)

        if self._header.GetTypeString() == "EFI_SECTION_PE32":
             pefile = pe.PEFile(self)
             pefile.Load(fd, self.GetContentSize())

        fd.seek(self._offset)
        self._contents.fromfile(fd, self.GetContentSize())

        # rebase file pointer to next section
        index = self._offset + self.GetSize()
        index = align(index, 4)
        fd.seek(index)

    def GetContentSize(self):
        return self.GetSize() - self.EFI_SECTION_HEADER_SIZE

    def GetContent(self):
        return self._contents.tolist()

    def GetSize(self):
        return self._header.GetSectionSize()

    def GetHeader(self):
        return self._header

    def GetSectionOffset(self):
        return self._offset + self.EFI_SECTION_HEADER_SIZE

class EfiSectionHeader(BinaryItem):
    section_type_map = {0x01: 'EFI_SECTION_COMPRESSION',
                        0x02: 'EFI_SECTION_GUID_DEFINED',
                        0x10: 'EFI_SECTION_PE32',
                        0x11: 'EFI_SECTION_PIC',
                        0x12: 'EFI_SECTION_TE',
                        0x13: 'EFI_SECTION_DXE_DEPEX',
                        0x14: 'EFI_SECTION_VERSION',
                        0x15: 'EFI_SECTION_USER_INTERFACE',
                        0x16: 'EFI_SECTION_COMPATIBILITY16',
                        0x17: 'EFI_SECTION_FIRMWARE_VOLUME_IMAGE',
                        0x18: 'EFI_SECTION_FREEFORM_SUBTYPE_GUID',
                        0x19: 'EFI_SECTION_RAW',
                        0x1B: 'EFI_SECTION_PEI_DEPEX'}
    def GetSize(self):
        return 4

    def GetSectionSize(self):
        list = self._arr.tolist()
        return list2int(list[0:3])

    def GetType(self):
        list = self._arr.tolist()
        return int(list[3])

    def GetTypeString(self):
        type = self.GetType()
        if type not in self.section_type_map.keys():
            return 'Unknown Section Type'
        return self.section_type_map[type]

    def Dump(self):
        print('size = 0x%X' % self.GetSectionSize())
        print('type = 0x%X' % self.GetType())



rMapEntry = re.compile('^(\w+)[ \(\w\)]* \(BaseAddress=([0-9a-fA-F]+), EntryPoint=([0-9a-fA-F]+), GUID=([0-9a-fA-F\-]+)')
class EfiFvMapFile(object):
    def __init__(self):
        self._mapentries = {}

    def Load(self, path):
        if not os.path.exists(path):
            return False

        try:
            file = open(path, 'r')
            lines = file.readlines()
            file.close()
        except:
            return False

        for line in lines:
            if line[0] != ' ':
                # new entry
                ret = rMapEntry.match(line)
                if ret is not None:
                    name     = ret.groups()[0]
                    baseaddr = int(ret.groups()[1], 16)
                    entry    = int(ret.groups()[2], 16)
                    guidstr  = '{' + ret.groups()[3] + '}'
                    guid     = uuid.UUID(guidstr)
                    self._mapentries[guid] = EfiFvMapFileEntry(name, baseaddr, entry, guid)
        return True

    def GetEntry(self, guid):
        if guid in self._mapentries.keys():
            return self._mapentries[guid]
        return None

class EfiFvMapFileEntry(object):
    def __init__(self, name, baseaddr, entry, guid):
        self._name     = name
        self._baseaddr = baseaddr
        self._entry    = entry
        self._guid     = guid

    def GetName(self):
        return self._name

    def GetBaseAddress(self):
        return self._baseaddr

    def GetEntryPoint(self):
        return self._entry

def list2guid(list):
    val1 = list2int(list[0:4])
    val2 = list2int(list[4:6])
    val3 = list2int(list[6:8])
    val4 = 0
    for item in list[8:16]:
        val4 = (val4 << 8) | int(item)

    val  = val1 << 12 * 8 | val2 << 10 * 8 | val3 << 8 * 8 | val4
    guid = uuid.UUID(int=val)
    return guid

def list2int(list):
    val = 0
    for index in range(len(list) - 1, -1, -1):
        val = (val << 8) | int(list[index])
    return val

def align(value, alignment):
    return (value + ((alignment - value) & (alignment - 1)))

gInvalidGuid = uuid.UUID(int=0xffffffffffffffffffffffffffffffff)
def isValidGuid(guid):
    if guid == gInvalidGuid:
        return False
    return True
