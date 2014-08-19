#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from arm_ds.debugger_v1 import DebugException

import struct
import string

import edk2_debugger

class EfiFileSection(object):
    EFI_SECTION_PE32                  = 0x10
    EFI_SECTION_PIC                   = 0x11
    EFI_SECTION_TE                    = 0x12

    EFI_IMAGE_DEBUG_TYPE_CODEVIEW     = 0x2

    SIZEOF_EFI_FFS_FILE_HEADER        = 0x28

    def __init__(self, ec, base):
        self.base = base
        self.ec = ec

    def __str__(self):
        return "FileSection(type:0x%X, size:0x%x)" % (self.get_type(), self.get_size())

    def get_base(self):
        return self.base

    def get_type(self):
        return struct.unpack("B", self.ec.getMemoryService().read(self.base + 0x3, 1, 8))[0]

    def get_size(self):
        return (struct.unpack("<I", self.ec.getMemoryService().read(self.base, 4, 32))[0] & 0x00ffffff)

    def get_debug_filepath(self):
        type = self.get_type()
        if type == EfiFileSection.EFI_SECTION_TE:
            section = EfiSectionTE(self, ec, self.base + 0x4)
        elif type == EfiFileSection.EFI_SECTION_PE32:
            section = EfiSectionPE32(self, ec, self.base + 0x4)
        else:
            raise Exception("EfiFileSection", "No debug section")
        return section.get_debug_filepath()

class EfiSectionTE:
    SIZEOF_EFI_TE_IMAGE_HEADER        = 0x28
    EFI_TE_IMAGE_SIGNATURE            = ('V','Z')

    def __init__(self, ec, base_te):
        self.ec = ec
        self.base_te = int(base_te)
        te_sig = struct.unpack("cc", self.ec.getMemoryService().read(self.base_te, 2, 32))
        if te_sig != EfiSectionTE.EFI_TE_IMAGE_SIGNATURE:
            raise Exception("EfiFileSectionTE","TE Signature incorrect")

    def get_debug_filepath(self):
        stripped_size = struct.unpack("<H", self.ec.getMemoryService().read(self.base_te + 0x6, 2, 32))[0]
        stripped_size -= EfiSectionTE.SIZEOF_EFI_TE_IMAGE_HEADER

        debug_dir_entry_rva = self.ec.getMemoryService().readMemory32(self.base_te + 0x20)
        if debug_dir_entry_rva == 0:
            raise Exception("EfiFileSectionTE","No debug directory for image")
        debug_dir_entry_rva -= stripped_size

        debug_type = self.ec.getMemoryService().readMemory32(self.base_te + debug_dir_entry_rva + 0xC)
        if (debug_type != 0xdf) and (debug_type != EfiFileSection.EFI_IMAGE_DEBUG_TYPE_CODEVIEW):
            raise Exception("EfiFileSectionTE","Debug type is not dwarf")

        debug_rva = self.ec.getMemoryService().readMemory32(self.base_te + debug_dir_entry_rva + 0x14)
        debug_rva -= stripped_size

        dwarf_sig = struct.unpack("cccc", self.ec.getMemoryService().read(self.base_te + debug_rva, 4, 32))
        if (dwarf_sig != 0x66727764) and (dwarf_sig != FirmwareFile.CONST_NB10_SIGNATURE):
            raise Exception("EfiFileSectionTE","Dwarf debug signature not found")

        if dwarf_sig == 0x66727764:
            filename = self.base_te + debug_rva + 0xc
        else:
            filename = self.base_te + debug_rva + 0x10
        filename = struct.unpack("200s", self.ec.getMemoryService().read(filename, 200, 32))[0]
        return filename[0:string.find(filename,'\0')]

    def get_debug_elfbase(self):
        stripped_size = struct.unpack("<H", self.ec.getMemoryService().read(self.base_te + 0x6, 2, 32))[0]
        stripped_size -= EfiSectionTE.SIZEOF_EFI_TE_IMAGE_HEADER

        base_of_code = self.ec.getMemoryService().readMemory32(self.base_te + 0xC)

        return self.base_te + base_of_code - stripped_size

class EfiSectionPE32:
    def __init__(self, ec, base_pe32):
        self.ec = ec
        self.base_pe32 = base_pe32

    def get_debug_filepath(self):
        # Offset from dos hdr to PE file hdr
        file_header_offset = self.ec.getMemoryService().readMemory32(self.base_pe32 + 0x3C)

        # Offset to debug dir in PE hdrs
        debug_dir_entry_rva = self.ec.getMemoryService().readMemory32(self.base_pe32 + file_header_offset + 0xA8)
        if debug_dir_entry_rva == 0:
            raise Exception("EfiFileSectionPE32","No Debug Directory")

        debug_type = self.ec.getMemoryService().readMemory32(self.base_pe32 + debug_dir_entry_rva + 0xC)
        if (debug_type != 0xdf) and (debug_type != EfiFileSection.EFI_IMAGE_DEBUG_TYPE_CODEVIEW):
            raise Exception("EfiFileSectionPE32","Debug type is not dwarf")


        debug_rva = self.ec.getMemoryService().readMemory32(self.base_pe32 + debug_dir_entry_rva + 0x14)

        dwarf_sig = struct.unpack("cccc", self.ec.getMemoryService().read(str(self.base_pe32 + debug_rva), 4, 32))
        if (dwarf_sig != 0x66727764) and (dwarf_sig != FirmwareFile.CONST_NB10_SIGNATURE):
            raise Exception("EfiFileSectionPE32","Dwarf debug signature not found")

        if dwarf_sig == 0x66727764:
            filename = self.base_pe32 + debug_rva + 0xc
        else:
            filename = self.base_pe32 + debug_rva + 0x10
        filename = struct.unpack("200s", self.ec.getMemoryService().read(str(filename), 200, 32))[0]
        return filename[0:string.find(filename,'\0')]

    def get_debug_elfbase(self):
        # Offset from dos hdr to PE file hdr
        pe_file_header = self.base_pe32 + self.ec.getMemoryService().readMemory32(self.base_pe32 + 0x3C)

        base_of_code = self.base_pe32 + self.ec.getMemoryService().readMemory32(pe_file_header + 0x28)
        base_of_data = self.base_pe32 + self.ec.getMemoryService().readMemory32(pe_file_header + 0x2C)

        if (base_of_code < base_of_data) and (base_of_code != 0):
            return base_of_code
        else:
            return base_of_data

class EfiSectionPE64:
    def __init__(self, ec, base_pe64):
        self.ec = ec
        self.base_pe64 = base_pe64

    def get_debug_filepath(self):
        # Offset from dos hdr to PE file hdr (EFI_IMAGE_NT_HEADERS64)
        #file_header_offset = self.ec.getMemoryService().readMemory32(self.base_pe64 + 0x3C)
        file_header_offset = 0x0

        # Offset to debug dir in PE hdrs
        debug_dir_entry_rva = self.ec.getMemoryService().readMemory32(self.base_pe64 + file_header_offset + 0x138)
        if debug_dir_entry_rva == 0:
            raise Exception("EfiFileSectionPE64","No Debug Directory")

        debug_type = self.ec.getMemoryService().readMemory32(self.base_pe64 + debug_dir_entry_rva + 0xC)
        if (debug_type != 0xdf) and (debug_type != EfiFileSection.EFI_IMAGE_DEBUG_TYPE_CODEVIEW):
            raise Exception("EfiFileSectionPE64","Debug type is not dwarf")


        debug_rva = self.ec.getMemoryService().readMemory32(self.base_pe64 + debug_dir_entry_rva + 0x14)

        dwarf_sig = struct.unpack("cccc", self.ec.getMemoryService().read(str(self.base_pe64 + debug_rva), 4, 32))
        if (dwarf_sig != 0x66727764) and (dwarf_sig != FirmwareFile.CONST_NB10_SIGNATURE):
            raise Exception("EfiFileSectionPE64","Dwarf debug signature not found")

        if dwarf_sig == 0x66727764:
            filename = self.base_pe64 + debug_rva + 0xc
        else:
            filename = self.base_pe64 + debug_rva + 0x10
        filename = struct.unpack("200s", self.ec.getMemoryService().read(str(filename), 200, 32))[0]
        return filename[0:string.find(filename,'\0')]

    def get_debug_elfbase(self):
        # Offset from dos hdr to PE file hdr
        pe_file_header = self.base_pe64 + self.ec.getMemoryService().readMemory32(self.base_pe64 + 0x3C)

        base_of_code = self.base_pe64 + self.ec.getMemoryService().readMemory32(pe_file_header + 0x28)
        base_of_data = self.base_pe64 + self.ec.getMemoryService().readMemory32(pe_file_header + 0x2C)

        if (base_of_code < base_of_data) and (base_of_code != 0):
            return base_of_code
        else:
            return base_of_data

class FirmwareFile:
    EFI_FV_FILETYPE_RAW                   = 0x01
    EFI_FV_FILETYPE_FREEFORM              = 0x02
    EFI_FV_FILETYPE_SECURITY_CORE         = 0x03
    EFI_FV_FILETYPE_PEI_CORE              = 0x04
    EFI_FV_FILETYPE_DXE_CORE              = 0x05
    EFI_FV_FILETYPE_PEIM                  = 0x06
    EFI_FV_FILETYPE_DRIVER                = 0x07
    EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  = 0x08
    EFI_FV_FILETYPE_APPLICATION           = 0x09
    EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE = 0x0B
    EFI_FV_FILETYPE_FFS_MIN               = 0xF0

    CONST_NB10_SIGNATURE = ('N','B','1','0')

    def __init__(self, fv, base, ec):
        self.fv = fv
        self.base = base
        self.ec = ec

    def __str__(self):
        return "FFS(state:0x%x, type:0x%X, size:0x%x)" % (self.get_state(), self.get_type(), self.get_size())

    def get_base(self):
        return self.base

    def get_size(self):
        size = (self.ec.getMemoryService().readMemory32(self.base + 0x14) & 0x00ffffff)

        # Occupied size is the size considering the alignment
        return size + ((0x8 - (size & 0x7)) & 0x7)

    def get_type(self):
        return self.ec.getMemoryService().readMemory8(self.base + 0x12)

    def get_state(self):
        state = self.ec.getMemoryService().readMemory8(self.base + 0x17)

        polarity = self.fv.get_polarity()
        if polarity:
            state = ~state

        highest_bit = 0x80;
        while (highest_bit != 0) and ((highest_bit & state) == 0):
            highest_bit >>= 1

        return highest_bit

    def get_next_section(self, section=None):
        if section == None:
            if self.get_type() != FirmwareFile.EFI_FV_FILETYPE_FFS_MIN:
                section_base = self.get_base() + 0x18;
            else:
                return None
        else:
            section_base = int(section.get_base() + section.get_size())

            # Align to next 4 byte boundary
            if (section_base & 0x3) != 0:
                section_base = section_base + 0x4 - (section_base & 0x3)

        if section_base < self.get_base() + self.get_size():
            return EfiFileSection(self.ec, section_base)
        else:
            return None

class FirmwareVolume:
    CONST_FV_SIGNATURE = ('_','F','V','H')
    EFI_FVB2_ERASE_POLARITY = 0x800

    DebugInfos = []

    def __init__(self, ec, fv_base, fv_size):
        self.ec = ec
        self.fv_base = fv_base
        self.fv_size = fv_size

        try:
            signature = struct.unpack("cccc", self.ec.getMemoryService().read(fv_base + 0x28, 4, 32))
        except DebugException:
            raise Exception("FirmwareVolume", "Not possible to access the defined firmware volume at [0x%X,0x%X]. Could be the used build report does not correspond to your current debugging context." % (int(fv_base),int(fv_base+fv_size)))
        if signature != FirmwareVolume.CONST_FV_SIGNATURE:
            raise Exception("FirmwareVolume", "This is not a valid firmware volume")

    def get_size(self):
        return self.ec.getMemoryService().readMemory32(self.fv_base + 0x20)

    def get_attributes(self):
        return self.ec.getMemoryService().readMemory32(self.fv_base + 0x2C)

    def get_polarity(self):
        attributes = self.get_attributes()
        if attributes & FirmwareVolume.EFI_FVB2_ERASE_POLARITY:
            return 1
        else:
            return 0

    def get_next_ffs(self, ffs=None):
        if ffs == None:
            # Get the offset of the first FFS file from the FV header
            ffs_base = self.fv_base +  self.ec.getMemoryService().readMemory16(self.fv_base + 0x30)
        else:
            # Goto the next FFS file
            ffs_base = int(ffs.get_base() + ffs.get_size())

            # Align to next 8 byte boundary
            if (ffs_base & 0x7) != 0:
                ffs_base = ffs_base + 0x8 - (ffs_base & 0x7)

        if ffs_base < self.fv_base + self.get_size():
            return FirmwareFile(self, ffs_base, self.ec)
        else:
            return None

    def get_debug_info(self):
        self.DebugInfos = []

        ffs = self.get_next_ffs()
        while ffs != None:
            section = ffs.get_next_section()
            while section != None:
                type = section.get_type()
                if (type == EfiFileSection.EFI_SECTION_TE) or (type == EfiFileSection.EFI_SECTION_PE32):
                    self.DebugInfos.append((section.get_base(), section.get_size(), section.get_type()))
                section = ffs.get_next_section(section)
            ffs = self.get_next_ffs(ffs)

    def load_symbols_at(self, addr, verbose = False):
        if self.DebugInfos == []:
            self.get_debug_info()

        for debug_info in self.DebugInfos:
            if (addr >= debug_info[0]) and (addr < debug_info[0] + debug_info[1]):
                if debug_info[2] == EfiFileSection.EFI_SECTION_TE:
                    section = EfiSectionTE(self.ec, debug_info[0] + 0x4)
                elif debug_info[2] == EfiFileSection.EFI_SECTION_PE32:
                    section = EfiSectionPE32(self.ec, debug_info[0] + 0x4)
                else:
                    raise Exception('FirmwareVolume','Section Type not supported')

                try:
                    edk2_debugger.load_symbol_from_file(self.ec, section.get_debug_filepath(), section.get_debug_elfbase(), verbose)
                except Exception, (ErrorClass, ErrorMessage):
                    if verbose:
                        print "Error while loading a symbol file (%s: %s)" % (ErrorClass, ErrorMessage)

                return debug_info

    def load_all_symbols(self, verbose = False):
        if self.DebugInfos == []:
            self.get_debug_info()

        for debug_info in self.DebugInfos:
            if debug_info[2] == EfiFileSection.EFI_SECTION_TE:
                section = EfiSectionTE(self.ec, debug_info[0] + 0x4)
            elif debug_info[2] == EfiFileSection.EFI_SECTION_PE32:
                section = EfiSectionPE32(self.ec, debug_info[0] + 0x4)
            else:
                continue

            try:
                edk2_debugger.load_symbol_from_file(self.ec, section.get_debug_filepath(), section.get_debug_elfbase(), verbose)
            except Exception, (ErrorClass, ErrorMessage):
                if verbose:
                    print "Error while loading a symbol file (%s: %s)" % (ErrorClass, ErrorMessage)

