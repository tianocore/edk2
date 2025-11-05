#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os

import firmware_volume
import build_report
import system_table

# Reload external classes
reload(firmware_volume)
reload(build_report)
reload(system_table)

def readMem32(executionContext, address):
    bytes = executionContext.getMemoryService().read(address, 4, 32)
    return struct.unpack('<I',bytes)[0]

def dump_fv(ec, fv_base, fv_size):
    fv = firmware_volume.FirmwareVolume(ec,
                                        int(build.PCDs['gArmTokenSpaceGuid']['PcdFvBaseAddress'][0],16),
                                        int(build.PCDs['gArmTokenSpaceGuid']['PcdFvSize'][0],16))

    ffs = fv.get_next_ffs()
    while ffs != None:
        print "# %s" % ffs

        section = ffs.get_next_section()
        while section != None:
            print "\t%s" % section
            try:
                print "\t\t- %s" % section.get_debug_filepath()
            except Exception:
                pass
            section = ffs.get_next_section(section)

        ffs = fv.get_next_ffs(ffs)

def dump_system_table(ec, mem_base, mem_size):
    st = system_table.SystemTable(ec, mem_base, mem_size)

    debug_info_table_base = st.get_configuration_table(system_table.DebugInfoTable.CONST_DEBUG_INFO_TABLE_GUID)

    debug_info_table = system_table.DebugInfoTable(ec, debug_info_table_base)
    debug_info_table.dump()

def load_symbol_from_file(ec, filename, address, verbose = False):
    if verbose:
        print "Add symbols of %s at 0x%x" % (filename, address)

    try:
        ec.getImageService().addSymbols(filename, address)
    except:
        try:
            # We could get an exception if the symbols are already loaded
            ec.getImageService().unloadSymbols(filename)
            ec.getImageService().addSymbols(filename, address)
        except:
            print "Warning: not possible to load symbols from %s at 0x%x" % (filename, address)

def is_aarch64(ec):
    success = True
    try:
        # Try to access a Aarch64 specific register
        ec.getRegisterService().getValue('X0')
    except:
        success = False
    return success

class ArmPlatform:
    def __init__(self, sysmembase=None, sysmemsize=None, fvs={}):
        self.sysmembase = sysmembase
        self.sysmemsize = sysmemsize
        self.fvs = fvs

class ArmPlatformDebugger:
    system_table = None
    firmware_volumes = {}

    REGION_TYPE_SYSMEM = 1
    REGION_TYPE_ROM    = 2
    REGION_TYPE_FV     = 3

    def __init__(self, ec, report_log, regions, verbose = False):
        self.ec = ec
        self.verbose = verbose
        fvs = []
        sysmem_base = None
        sysmem_size = None

        if report_log and os.path.isfile(report_log):
            try:
                self.build = build_report.BuildReport(report_log)
            except IOError:
                raise IOError(2, 'Report \'%s\' is not valid' % report_log)

            # Generate list of supported Firmware Volumes
            if self.build.PCDs['gArmTokenSpaceGuid'].has_key('PcdFvSize') and int(self.build.PCDs['gArmTokenSpaceGuid']['PcdFvSize'][0],16) != 0:
                fvs.append((int(self.build.PCDs['gArmTokenSpaceGuid']['PcdFvBaseAddress'][0],16),int(self.build.PCDs['gArmTokenSpaceGuid']['PcdFvSize'][0],16)))
            if self.build.PCDs['gArmTokenSpaceGuid'].has_key('PcdSecureFvSize') and int(self.build.PCDs['gArmTokenSpaceGuid']['PcdSecureFvSize'][0],16) != 0:
                fvs.append((int(self.build.PCDs['gArmTokenSpaceGuid']['PcdSecureFvBaseAddress'][0],16),int(self.build.PCDs['gArmTokenSpaceGuid']['PcdSecureFvSize'][0],16)))
            if self.build.PCDs['gArmTokenSpaceGuid'].has_key('PcdHypFvSize') and int(self.build.PCDs['gArmTokenSpaceGuid']['PcdHypFvSize'][0],16) != 0:
                fvs.append((int(self.build.PCDs['gArmTokenSpaceGuid']['PcdHypFvBaseAddress'][0],16),int(self.build.PCDs['gArmTokenSpaceGuid']['PcdHypFvSize'][0],16)))

            sysmem_base = int(self.build.PCDs['gArmTokenSpaceGuid']['PcdSystemMemoryBase'][0],16)
            sysmem_size = int(self.build.PCDs['gArmTokenSpaceGuid']['PcdSystemMemorySize'][0],16)
        else:
            for region in regions:
                if region[0] == ArmPlatformDebugger.REGION_TYPE_SYSMEM:
                    sysmem_base = region[1]
                    sysmem_size = region[2]
                elif region[0] == ArmPlatformDebugger.REGION_TYPE_FV:
                    fvs.append((region[1],region[2]))
                elif region[0] == ArmPlatformDebugger.REGION_TYPE_ROM:
                    for base in xrange(region[1], region[1] + region[2], 0x400000):
                        signature = struct.unpack("cccc", self.ec.getMemoryService().read(base, 4, 32))
                        if signature == FirmwareVolume.CONST_FV_SIGNATURE:
                            fvs.append((base,0))
                else:
                    print "Region type '%d' Not Supported" % region[0]

        self.platform = ArmPlatform(sysmem_base, sysmem_size, fvs)

    def in_sysmem(self, addr):
        return (self.platform.sysmembase is not None) and (self.platform.sysmembase <= addr) and (addr < self.platform.sysmembase + self.platform.sysmemsize)

    def in_fv(self, addr):
        return (self.get_fv_at(addr) != None)

    def get_fv_at(self, addr):
        for fv in self.platform.fvs:
            if (fv[0] <= addr) and (addr < fv[0] + fv[1]):
                return fv
        return None

    def load_current_symbols(self):
        pc = int(self.ec.getRegisterService().getValue('PC')) & 0xFFFFFFFF
        if self.in_fv(pc):
            debug_infos = []

            (fv_base, fv_size) = self.get_fv_at(pc)

            if self.firmware_volumes.has_key(fv_base) == False:
                self.firmware_volumes[fv_base] = firmware_volume.FirmwareVolume(self.ec, fv_base, fv_size)

            stack_frame = self.ec.getTopLevelStackFrame()
            info = self.firmware_volumes[fv_base].load_symbols_at(int(stack_frame.getRegisterService().getValue('PC')) & 0xFFFFFFFF, self.verbose)
            debug_infos.append(info)
            while stack_frame.next() is not None:
                stack_frame = stack_frame.next()

                # Stack frame attached to 'PC'
                pc = int(stack_frame.getRegisterService().getValue('PC')) & 0xFFFFFFFF

                # Check if the symbols for this stack frame have already been loaded
                found = False
                for debug_info in debug_infos:
                    if (pc >= debug_info[0]) and (pc < debug_info[0] + debug_info[1]):
                        found = True
                if found == False:
                    info = self.firmware_volumes[fv_base].load_symbols_at(pc)
                    debug_infos.append(info)

            #self.firmware_volumes[fv_base].load_symbols_at(pc)
        elif self.in_sysmem(pc):
            debug_infos = []

            if self.system_table is None:
                # Find the System Table
                self.system_table = system_table.SystemTable(self.ec, self.platform.sysmembase, self.platform.sysmemsize)

                # Find the Debug Info Table
                debug_info_table_base = self.system_table.get_configuration_table(system_table.DebugInfoTable.CONST_DEBUG_INFO_TABLE_GUID)
                self.debug_info_table = system_table.DebugInfoTable(self.ec, debug_info_table_base)

            stack_frame = self.ec.getTopLevelStackFrame()
            info = self.debug_info_table.load_symbols_at(int(stack_frame.getRegisterService().getValue('PC')) & 0xFFFFFFFF, self.verbose)
            debug_infos.append(info)
            while stack_frame.next() is not None:
                stack_frame = stack_frame.next()

                # Stack frame attached to 'PC'
                pc = int(stack_frame.getRegisterService().getValue('PC')) & 0xFFFFFFFF

                # Check if the symbols for this stack frame have already been loaded
                found = False
                for debug_info in debug_infos:
                    if (pc >= debug_info[0]) and (pc < debug_info[0] + debug_info[1]):
                        found = True
                if found == False:
                    try:
                        info = self.debug_info_table.load_symbols_at(pc)
                        debug_infos.append(info)
                    except:
                        pass

            #self.debug_info_table.load_symbols_at(pc)
        else:
            raise Exception('ArmPlatformDebugger', "Not supported region")

    def load_all_symbols(self):
        # Load all the XIP symbols attached to the Firmware Volume
        for (fv_base, fv_size) in self.platform.fvs:
            if self.firmware_volumes.has_key(fv_base) == False:
                self.firmware_volumes[fv_base] = firmware_volume.FirmwareVolume(self.ec, fv_base, fv_size)
            self.firmware_volumes[fv_base].load_all_symbols(self.verbose)

        try:
            # Load all symbols of module loaded into System Memory
            if self.system_table is None:
                # Find the System Table
                self.system_table = system_table.SystemTable(self.ec, self.platform.sysmembase, self.platform.sysmemsize)


                # Find the Debug Info Table
                debug_info_table_base = self.system_table.get_configuration_table(system_table.DebugInfoTable.CONST_DEBUG_INFO_TABLE_GUID)
                self.debug_info_table = system_table.DebugInfoTable(self.ec, debug_info_table_base)

            self.debug_info_table.load_all_symbols(self.verbose)
        except:
            # Debugger exception could be excepted if DRAM has not been initialized or if we have not started to run from DRAM yet
            print "Note: no symbols have been found in System Memory (possible cause: the UEFI permanent memory has not been installed yet)"
