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

import edk2_debugger
import firmware_volume

class DebugInfoTable:
    CONST_DEBUG_INFO_TABLE_GUID = ( 0x49152E77L, 0x47641ADAL, 0xFE7AA2B7L, 0x8B5ED9FEL)

    DebugInfos = []

    def __init__(self, ec, debug_info_table_header_offset):
        self.ec = ec
        self.base = debug_info_table_header_offset

    def get_debug_info(self):
        # Get the information from EFI_DEBUG_IMAGE_INFO_TABLE_HEADER
        count = self.ec.getMemoryService().readMemory32(self.base + 0x4)
        if edk2_debugger.is_aarch64(self.ec):
            debug_info_table_base = self.ec.getMemoryService().readMemory64(self.base + 0x8)
        else:
            debug_info_table_base = self.ec.getMemoryService().readMemory32(self.base + 0x8)

        self.DebugInfos = []

        for i in range(0, count):
            # Get the address of the structure EFI_DEBUG_IMAGE_INFO
            if edk2_debugger.is_aarch64(self.ec):
                debug_info = self.ec.getMemoryService().readMemory64(debug_info_table_base + (i * 8))
            else:
                debug_info = self.ec.getMemoryService().readMemory32(debug_info_table_base + (i * 4))

            if debug_info:
                debug_info_type = self.ec.getMemoryService().readMemory32(debug_info)
                # Normal Debug Info Type
                if debug_info_type == 1:
                    if edk2_debugger.is_aarch64(self.ec):
                        # Get the base address of the structure EFI_LOADED_IMAGE_PROTOCOL
                        loaded_image_protocol = self.ec.getMemoryService().readMemory64(debug_info + 0x8)

                        image_base = self.ec.getMemoryService().readMemory64(loaded_image_protocol + 0x40)
                        image_size = self.ec.getMemoryService().readMemory32(loaded_image_protocol + 0x48)
                    else:
                        # Get the base address of the structure EFI_LOADED_IMAGE_PROTOCOL
                        loaded_image_protocol = self.ec.getMemoryService().readMemory32(debug_info + 0x4)

                        image_base = self.ec.getMemoryService().readMemory32(loaded_image_protocol + 0x20)
                        image_size = self.ec.getMemoryService().readMemory32(loaded_image_protocol + 0x28)

                    self.DebugInfos.append((image_base,image_size))

    # Return (base, size)
    def load_symbols_at(self, addr, verbose = False):
        if self.DebugInfos == []:
            self.get_debug_info()

        found = False
        for debug_info in self.DebugInfos:
            if (addr >= debug_info[0]) and (addr < debug_info[0] + debug_info[1]):
                if edk2_debugger.is_aarch64(self.ec):
                    section = firmware_volume.EfiSectionPE64(self.ec, debug_info[0])
                else:
                    section = firmware_volume.EfiSectionPE32(self.ec, debug_info[0])

                try:
                    edk2_debugger.load_symbol_from_file(self.ec, section.get_debug_filepath(), section.get_debug_elfbase(), verbose)
                except Exception, (ErrorClass, ErrorMessage):
                    if verbose:
                        print "Error while loading a symbol file (%s: %s)" % (ErrorClass, ErrorMessage)

                found = True
                return debug_info

        if found == False:
            raise Exception('DebugInfoTable','No symbol found at 0x%x' % addr)

    def load_all_symbols(self, verbose = False):
        if self.DebugInfos == []:
            self.get_debug_info()

        for debug_info in self.DebugInfos:
            if edk2_debugger.is_aarch64(self.ec):
                section = firmware_volume.EfiSectionPE64(self.ec, debug_info[0])
            else:
                section = firmware_volume.EfiSectionPE32(self.ec, debug_info[0])

            try:
                edk2_debugger.load_symbol_from_file(self.ec, section.get_debug_filepath(), section.get_debug_elfbase(), verbose)
            except Exception, (ErrorClass, ErrorMessage):
                if verbose:
                    print "Error while loading a symbol file (%s: %s)" % (ErrorClass, ErrorMessage)

    def dump(self):
        self.get_debug_info()
        for debug_info in self.DebugInfos:
            base_pe32 = debug_info[0]
            if edk2_debugger.is_aarch64(self.ec):
                section = firmware_volume.EfiSectionPE64(self.ec, base_pe32)
            else:
                section = firmware_volume.EfiSectionPE32(self.ec, base_pe32)
            print section.get_debug_filepath()

class SystemTable:
    CONST_ST_SIGNATURE = ('I','B','I',' ','S','Y','S','T')

    def __init__(self, ec, membase, memsize):
        self.membase = membase
        self.memsize = memsize
        self.ec = ec

        found = False

        # Start from the top of the memory
        offset = self.membase + self.memsize
        # Align to highest 4MB boundary
        offset = offset & ~0x3FFFFF
        # We should not have a System Table at the top of the System Memory
        offset = offset - 0x400000

        # Start at top and look on 4MB boundaries for system table ptr structure
        while offset > self.membase:
            try:
                signature = struct.unpack("cccccccc", self.ec.getMemoryService().read(str(offset), 8, 32))
            except DebugException:
                raise Exception('SystemTable','Fail to access System Memory. Ensure all the memory in the region [0x%x;0x%X] is accessible.' % (membase,membase+memsize))
            if signature == SystemTable.CONST_ST_SIGNATURE:
                found = True
                if edk2_debugger.is_aarch64(self.ec):
                    self.system_table_base = self.ec.getMemoryService().readMemory64(offset + 0x8)
                else:
                    self.system_table_base = self.ec.getMemoryService().readMemory32(offset + 0x8)
                break
            offset = offset - 0x400000

        if not found:
            raise Exception('SystemTable','System Table not found in System Memory [0x%x;0x%X]' % (membase,membase+memsize))

    def get_configuration_table(self, conf_table_guid):
        if edk2_debugger.is_aarch64(self.ec):
            # Number of configuration Table entry
            conf_table_entry_count = self.ec.getMemoryService().readMemory32(self.system_table_base + 0x68)

            # Get location of the Configuration Table entries
            conf_table_offset = self.ec.getMemoryService().readMemory64(self.system_table_base + 0x70)
        else:
            # Number of configuration Table entry
            conf_table_entry_count = self.ec.getMemoryService().readMemory32(self.system_table_base + 0x40)

            # Get location of the Configuration Table entries
            conf_table_offset = self.ec.getMemoryService().readMemory32(self.system_table_base + 0x44)

        for i in range(0, conf_table_entry_count):
            if edk2_debugger.is_aarch64(self.ec):
                offset = conf_table_offset + (i * 0x18)
            else:
                offset = conf_table_offset + (i * 0x14)
            guid = struct.unpack("<IIII", self.ec.getMemoryService().read(str(offset), 16, 32))
            if guid == conf_table_guid:
                if edk2_debugger.is_aarch64(self.ec):
                    return self.ec.getMemoryService().readMemory64(offset + 0x10)
                else:
                    return self.ec.getMemoryService().readMemory32(offset + 0x10)

        raise Exception('SystemTable','Configuration Table not found')
