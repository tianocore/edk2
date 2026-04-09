#!/usr/bin/python3
'''
Copyright 2021 (c) Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

EFI gdb commands based on efi_debugging classes.

Example usage:
OvmfPkg/build.sh qemu -gdb tcp::9000
gdb -ex "target remote localhost:9000" -ex "source efi_gdb.py"

(gdb) help efi
Commands for debugging EFI. efi <cmd>

List of efi subcommands:

efi devicepath -- Display an EFI device path.
efi guid -- Display info about EFI GUID's.
efi hob -- Dump EFI HOBs. Type 'hob -h' for more info.
efi symbols -- Load Symbols for EFI. Type 'efi_symbols -h' for more info.
efi table -- Dump EFI System Tables. Type 'table -h' for more info.

This module is coded against a generic gdb remote serial stub. It should work
with QEMU, JTAG debugger, or a generic EFI gdb remote serial stub.

If you are debugging with QEMU or a JTAG hardware debugger you can insert
a CpuDeadLoop(); in your code, attach with gdb, and then `p Index=1` to
step past. If you have a debug stub in EFI you can use CpuBreakpoint();.
'''

from gdb.printing import RegexpCollectionPrettyPrinter
from gdb.printing import register_pretty_printer
import gdb
import os
import sys
import uuid
import optparse
import shlex

# gdb will not import from the same path as this script.
# so lets fix that for gdb...
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from efi_debugging import PeTeImage, patch_ctypes            # noqa: E402
from efi_debugging import EfiHob, GuidNames, EfiStatusClass  # noqa: E402
from efi_debugging import EfiBootMode, EfiDevicePath         # noqa: E402
from efi_debugging import EfiConfigurationTable, EfiTpl      # noqa: E402


class GdbFileObject(object):
    '''Provide a file like object required by efi_debugging'''

    def __init__(self):
        self.inferior = gdb.selected_inferior()
        self.offset = 0

    def tell(self):
        return self.offset

    def read(self, size=-1):
        if size == -1:
            # arbitrary default size
            size = 0x1000000

        try:
            data = self.inferior.read_memory(self.offset, size)
        except MemoryError:
            data = bytearray(size)
            assert False
        if len(data) != size:
            raise MemoryError(
                f'gdb could not read memory 0x{size:x}'
                + f' bytes from 0x{self.offset:08x}')
        else:
            # convert memoryview object to a bytestring.
            return data.tobytes()

    def readable(self):
        return True

    def seek(self, offset, whence=0):
        if whence == 0:
            self.offset = offset
        elif whence == 1:
            self.offset += offset
        else:
            # whence == 2 is seek from end
            raise NotImplementedError

    def seekable(self):
        return True

    def write(self, data):
        self.inferior.write_memory(self.offset, data)
        return len(data)

    def writable(self):
        return True

    def truncate(self, size=None):
        raise NotImplementedError

    def flush(self):
        raise NotImplementedError

    def fileno(self):
        raise NotImplementedError


class EfiSymbols:
    """Class to manage EFI Symbols"""

    loaded = {}
    stride = None
    range = None
    verbose = False

    def __init__(self, file=None):
        EfiSymbols.file = file if file else GdbFileObject()

    @ classmethod
    def __str__(cls):
        return ''.join(f'{value}\n' for value in cls.loaded.values())

    @ classmethod
    def configure_search(cls, stride, range=None, verbose=False):
        cls.stride = stride
        cls.range = range
        cls.verbose = verbose

    @ classmethod
    def clear(cls):
        cls.loaded = {}

    @ classmethod
    def add_symbols_for_pecoff(cls, pecoff):
        '''Tell lldb the location of the .text and .data sections.'''

        if pecoff.TextAddress in cls.loaded:
            return 'Already Loaded: '
        try:
            res = 'Loading Symbols Failed:'
            res = gdb.execute('add-symbol-file ' + pecoff.CodeViewPdb +
                              ' ' + hex(pecoff.TextAddress) +
                              ' -s .data ' + hex(pecoff.DataAddress),
                              False, True)

            cls.loaded[pecoff.TextAddress] = pecoff
            if cls.verbose:
                print(f'\n{res:s}\n')
            return ''
        except gdb.error:
            return res

    @ classmethod
    def address_to_symbols(cls, address, reprobe=False):
        '''
        Given an address search backwards for a PE/COFF (or TE) header
        and load symbols. Return a status string.
        '''
        if not isinstance(address, int):
            address = int(address)

        pecoff = cls.address_in_loaded_pecoff(address)
        if not reprobe and pecoff is not None:
            # skip the probe of the remote
            return f'{pecoff} is already loaded'

        pecoff = PeTeImage(cls.file, None)
        if pecoff.pcToPeCoff(address, cls.stride, cls.range):
            res = cls.add_symbols_for_pecoff(pecoff)
            return f'{res}{pecoff}'
        else:
            return f'0x{address:08x} not in a PE/COFF (or TE) image'

    @ classmethod
    def address_in_loaded_pecoff(cls, address):
        if not isinstance(address, int):
            address = int(address)

        for value in cls.loaded.values():
            if (address >= value.LoadAddress and
                    address <= value.EndLoadAddress):
                return value

        return None

    @ classmethod
    def unload_symbols(cls, address):
        if not isinstance(address, int):
            address = int(address)

        pecoff = cls.address_in_loaded_pecoff(address)
        try:
            res = 'Unloading Symbols Failed:'
            res = gdb.execute(
                f'remove-symbol-file -a {hex(pecoff.TextAddress):s}',
                False, True)
            del cls.loaded[pecoff.LoadAddress]
            return res
        except gdb.error:
            return res


class CHAR16_PrettyPrinter(object):

    def __init__(self, val):
        self.val = val

    def to_string(self):
        if int(self.val) < 0x20:
            return f"L'\\x{int(self.val):02x}'"
        else:
            return f"L'{chr(self.val):s}'"


class EFI_TPL_PrettyPrinter(object):

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return str(EfiTpl(int(self.val)))


class EFI_STATUS_PrettyPrinter(object):

    def __init__(self, val):
        self.val = val

    def to_string(self):
        status = int(self.val)
        return f'{str(EfiStatusClass(status)):s} (0x{status:08x})'


class EFI_BOOT_MODE_PrettyPrinter(object):

    def __init__(self, val):
        self.val = val

    def to_string(self):
        return str(EfiBootMode(int(self.val)))


class EFI_GUID_PrettyPrinter(object):
    """Print 'EFI_GUID' as 'xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx'"""

    def __init__(self, val):
        self.val = val

    def to_string(self):
        # if we could get a byte like object of *(unsigned char (*)[16])
        # then we could just use uuid.UUID() to convert
        Data1 = int(self.val['Data1'])
        Data2 = int(self.val['Data2'])
        Data3 = int(self.val['Data3'])
        Data4 = self.val['Data4']
        guid = f'{Data1:08X}-{Data2:04X}-'
        guid += f'{Data3:04X}-'
        guid += f'{int(Data4[0]):02X}{int(Data4[1]):02X}-'
        guid += f'{int(Data4[2]):02X}{int(Data4[3]):02X}'
        guid += f'{int(Data4[4]):02X}{int(Data4[5]):02X}'
        guid += f'{int(Data4[6]):02X}{int(Data4[7]):02X}'
        return str(GuidNames(guid))


def build_pretty_printer():
    # Turn off via: disable pretty-printer global EFI
    pp = RegexpCollectionPrettyPrinter("EFI")
    # you can also tell gdb `x/sh <address>` to print CHAR16 string
    pp.add_printer('CHAR16', '^CHAR16$', CHAR16_PrettyPrinter)
    pp.add_printer('EFI_BOOT_MODE', '^EFI_BOOT_MODE$',
                   EFI_BOOT_MODE_PrettyPrinter)
    pp.add_printer('EFI_GUID', '^EFI_GUID$', EFI_GUID_PrettyPrinter)
    pp.add_printer('EFI_STATUS', '^EFI_STATUS$', EFI_STATUS_PrettyPrinter)
    pp.add_printer('EFI_TPL', '^EFI_TPL$', EFI_TPL_PrettyPrinter)
    return pp


class EfiDevicePathCmd (gdb.Command):
    """Display an EFI device path. Type 'efi devicepath -h' for more info"""

    def __init__(self):
        super(EfiDevicePathCmd, self).__init__(
            "efi devicepath", gdb.COMMAND_NONE)

        self.file = GdbFileObject()

    def create_options(self, arg, from_tty):
        usage = "usage: %prog [options] [arg]"
        description = (
            "Command that can load EFI PE/COFF and TE image symbols. ")

        self.parser = optparse.OptionParser(
            description=description,
            prog='efi devicepath',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-v',
            '--verbose',
            action='store_true',
            dest='verbose',
            help='hex dump extra data',
            default=False)

        self.parser.add_option(
            '-n',
            '--node',
            action='store_true',
            dest='node',
            help='dump a single device path node',
            default=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

        return self.parser.parse_args(shlex.split(arg))

    def invoke(self, arg, from_tty):
        '''gdb command to dump EFI device paths'''

        try:
            (options, _) = self.create_options(arg, from_tty)
            if options.help:
                self.parser.print_help()
                return

            dev_addr = int(gdb.parse_and_eval(arg))
        except ValueError:
            print("Invalid argument!")
            return

        if options.node:
            print(EfiDevicePath(
                self.file).device_path_node_str(dev_addr,
                                                options.verbose))
        else:
            device_path = EfiDevicePath(self.file, dev_addr, options.verbose)
            if device_path.valid():
                print(device_path)


class EfiGuidCmd (gdb.Command):
    """Display info about EFI GUID's. Type 'efi guid -h' for more info"""

    def __init__(self):
        super(EfiGuidCmd, self).__init__("efi guid",
                                         gdb.COMMAND_NONE,
                                         gdb.COMPLETE_EXPRESSION)
        self.file = GdbFileObject()

    def create_options(self, arg, from_tty):
        usage = "usage: %prog [options] [arg]"
        description = (
            "Show EFI_GUID values and the C name of the EFI_GUID variables"
            "in the C code. If symbols are loaded the Guid.xref file"
            "can be processed and the complete GUID database can be shown."
            "This command also suports generating new GUID's, and showing"
            "the value used to initialize the C variable.")

        self.parser = optparse.OptionParser(
            description=description,
            prog='efi guid',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-n',
            '--new',
            action='store_true',
            dest='new',
            help='Generate a new GUID',
            default=False)

        self.parser.add_option(
            '-v',
            '--verbose',
            action='store_true',
            dest='verbose',
            help='Also display GUID C structure values',
            default=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

        return self.parser.parse_args(shlex.split(arg))

    def invoke(self, arg, from_tty):
        '''gdb command to dump EFI System Tables'''

        try:
            (options, args) = self.create_options(arg, from_tty)
            if options.help:
                self.parser.print_help()
                return
            if len(args) >= 1:
                # guid { 0x414e6bdd, 0xe47b, 0x47cc,
                #        { 0xb2, 0x44, 0xbb, 0x61, 0x02, 0x0c,0xf5, 0x16 }}
                # this generates multiple args
                guid = ' '.join(args)
        except ValueError:
            print('bad arguments!')
            return

        if options.new:
            guid = uuid.uuid4()
            print(str(guid).upper())
            print(GuidNames.to_c_guid(guid))
            return

        if len(args) > 0:
            if GuidNames.is_guid_str(arg):
                # guid 05AD34BA-6F02-4214-952E-4DA0398E2BB9
                key = guid.upper()
                name = GuidNames.to_name(key)
            elif GuidNames.is_c_guid(arg):
                # guid { 0x414e6bdd, 0xe47b, 0x47cc,
                #        { 0xb2, 0x44, 0xbb, 0x61, 0x02, 0x0c,0xf5, 0x16 }}
                key = GuidNames.from_c_guid(arg)
                name = GuidNames.to_name(key)
            else:
                # guid gEfiDxeServicesTableGuid
                name = guid
                try:
                    key = GuidNames.to_guid(name)
                    name = GuidNames.to_name(key)
                except ValueError:
                    return

            extra = f'{GuidNames.to_c_guid(key)}: ' if options.verbose else ''
            print(f'{key}: {extra}{name}')

        else:
            for key, value in GuidNames._dict_.items():
                if options.verbose:
                    extra = f'{GuidNames.to_c_guid(key)}: '
                else:
                    extra = ''
                print(f'{key}: {extra}{value}')


class EfiHobCmd (gdb.Command):
    """Dump EFI HOBs. Type 'hob -h' for more info."""

    def __init__(self):
        super(EfiHobCmd, self).__init__("efi hob", gdb.COMMAND_NONE)
        self.file = GdbFileObject()

    def create_options(self, arg, from_tty):
        usage = "usage: %prog [options] [arg]"
        description = (
            "Command that can load EFI PE/COFF and TE image symbols. ")

        self.parser = optparse.OptionParser(
            description=description,
            prog='efi hob',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-a',
            '--address',
            type="int",
            dest='address',
            help='Parse HOBs from address',
            default=None)

        self.parser.add_option(
            '-t',
            '--type',
            type="int",
            dest='type',
            help='Only dump HOBS of his type',
            default=None)

        self.parser.add_option(
            '-v',
            '--verbose',
            action='store_true',
            dest='verbose',
            help='hex dump extra data',
            default=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

        return self.parser.parse_args(shlex.split(arg))

    def invoke(self, arg, from_tty):
        '''gdb command to dump EFI System Tables'''

        try:
            (options, _) = self.create_options(arg, from_tty)
            if options.help:
                self.parser.print_help()
                return
        except ValueError:
            print('bad arguments!')
            return

        if options.address:
            try:
                value = gdb.parse_and_eval(options.address)
                address = int(value)
            except ValueError:
                address = None
        else:
            address = None

        hob = EfiHob(self.file,
                     address,
                     options.verbose).get_hob_by_type(options.type)
        print(hob)


class EfiTablesCmd (gdb.Command):
    """Dump EFI System Tables. Type 'table -h' for more info."""

    def __init__(self):
        super(EfiTablesCmd, self).__init__("efi table", gdb.COMMAND_NONE)

        self.file = GdbFileObject()

    def create_options(self, arg, from_tty):
        usage = "usage: %prog [options] [arg]"
        description = "Dump EFI System Tables. Requires symbols to be loaded"

        self.parser = optparse.OptionParser(
            description=description,
            prog='efi table',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

        return self.parser.parse_args(shlex.split(arg))

    def invoke(self, arg, from_tty):
        '''gdb command to dump EFI System Tables'''

        try:
            (options, _) = self.create_options(arg, from_tty)
            if options.help:
                self.parser.print_help()
                return
        except ValueError:
            print('bad arguments!')
            return

        gST = gdb.lookup_global_symbol('gST')
        if gST is None:
            print('Error: This command requires symbols for gST to be loaded')
            return

        table = EfiConfigurationTable(
            self.file, int(gST.value(gdb.selected_frame())))
        if table:
            print(table, '\n')


class EfiSymbolsCmd (gdb.Command):
    """Load Symbols for EFI. Type 'efi symbols -h' for more info."""

    def __init__(self):
        super(EfiSymbolsCmd, self).__init__("efi symbols",
                                            gdb.COMMAND_NONE,
                                            gdb.COMPLETE_EXPRESSION)
        self.file = GdbFileObject()
        self.gST = None
        self.efi_symbols = EfiSymbols(self.file)

    def create_options(self, arg, from_tty):
        usage = "usage: %prog [options]"
        description = (
            "Command that can load EFI PE/COFF and TE image symbols. "
            "If you are having trouble in PEI try adding --pei. "
            "Given any address search backward for the PE/COFF (or TE header) "
            "and then parse the PE/COFF image to get debug info. "
            "The address can come from the current pc, pc values in the "
            "frame, or an address provided to the command"
            "")

        self.parser = optparse.OptionParser(
            description=description,
            prog='efi symbols',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-a',
            '--address',
            type="str",
            dest='address',
            help='Load symbols for image that contains address',
            default=None)

        self.parser.add_option(
            '-c',
            '--clear',
            action='store_true',
            dest='clear',
            help='Clear the cache of loaded images',
            default=False)

        self.parser.add_option(
            '-f',
            '--frame',
            action='store_true',
            dest='frame',
            help='Load symbols for current stack frame',
            default=False)

        self.parser.add_option(
            '-p',
            '--pc',
            action='store_true',
            dest='pc',
            help='Load symbols for pc',
            default=False)

        self.parser.add_option(
            '--pei',
            action='store_true',
            dest='pei',
            help='Load symbols for PEI (searches every 4 bytes)',
            default=False)

        self.parser.add_option(
            '-e',
            '--extended',
            action='store_true',
            dest='extended',
            help='Try to load all symbols based on config tables',
            default=False)

        self.parser.add_option(
            '-r',
            '--range',
            type="long",
            dest='range',
            help='How far to search backward for start of PE/COFF Image',
            default=None)

        self.parser.add_option(
            '-s',
            '--stride',
            type="long",
            dest='stride',
            help='Boundary to search for PE/COFF header',
            default=None)

        self.parser.add_option(
            '-t',
            '--thread',
            action='store_true',
            dest='thread',
            help='Load symbols for the frames of all threads',
            default=False)

        self.parser.add_option(
            '-v',
            '--verbose',
            action='store_true',
            dest='verbose',
            help='Show more info on symbols loading in gdb',
            default=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

        return self.parser.parse_args(shlex.split(arg))

    def save_user_state(self):
        self.pagination = gdb.parameter("pagination")
        if self.pagination:
            gdb.execute("set pagination off")

        self.user_selected_thread = gdb.selected_thread()
        self.user_selected_frame = gdb.selected_frame()

    def restore_user_state(self):
        self.user_selected_thread.switch()
        self.user_selected_frame.select()

        if self.pagination:
            gdb.execute("set pagination on")

    def canonical_address(self, address):
        '''
        Scrub out 48-bit non canonical addresses
        Raw frames in gdb can have some funky values
        '''

        # Skip lowest 256 bytes to avoid interrupt frames
        if address > 0xFF and address < 0x00007FFFFFFFFFFF:
            return True
        if address >= 0xFFFF800000000000:
            return True

        return False

    def pc_set_for_frames(self):
        '''Return a set for the PC's in the current frame'''
        pc_list = []
        frame = gdb.newest_frame()
        while frame:
            pc = int(frame.read_register('pc'))
            if self.canonical_address(pc):
                pc_list.append(pc)
            frame = frame.older()

        return set(pc_list)

    def invoke(self, arg, from_tty):
        '''gdb command to symbolicate all the frames from all the threads'''

        try:
            (options, _) = self.create_options(arg, from_tty)
            if options.help:
                self.parser.print_help()
                return
        except ValueError:
            print('bad arguments!')
            return

        self.dont_repeat()

        self.save_user_state()

        if options.clear:
            self.efi_symbols.clear()
            return

        if options.pei:
            # XIP code can be 4 byte aligned in the FV
            options.stride = 4
            options.range = 0x100000
        self.efi_symbols.configure_search(options.stride,
                                          options.range,
                                          options.verbose)

        if options.thread:
            thread_list = gdb.selected_inferior().threads()
        else:
            thread_list = (gdb.selected_thread(),)

        address = None
        if options.address:
            value = gdb.parse_and_eval(options.address)
            address = int(value)
        elif options.pc:
            address = gdb.selected_frame().pc()

        if address:
            res = self.efi_symbols.address_to_symbols(address)
            print(res)
        else:

            for thread in thread_list:
                thread.switch()

                # You can not iterate over frames as you load symbols. Loading
                # symbols changes the frames gdb can see due to inlining and
                # boom. So we loop adding symbols for the current frame, and
                # we test to see if new frames have shown up. If new frames
                # show up we process those new frames. Thus 1st pass is the
                # raw frame, and other passes are only new PC values.
                NewPcSet = self.pc_set_for_frames()
                while NewPcSet:
                    PcSet = self.pc_set_for_frames()
                    for pc in NewPcSet:
                        res = self.efi_symbols.address_to_symbols(pc)
                        print(res)

                    NewPcSet = PcSet.symmetric_difference(
                        self.pc_set_for_frames())

        # find the EFI System tables the 1st time
        if self.gST is None:
            gST = gdb.lookup_global_symbol('gST')
            if gST is not None:
                self.gST = int(gST.value(gdb.selected_frame()))
                table = EfiConfigurationTable(self.file, self.gST)
            else:
                table = None
        else:
            table = EfiConfigurationTable(self.file, self.gST)

        if options.extended and table:
            # load symbols from EFI System Table entry
            for address, _ in table.DebugImageInfo():
                res = self.efi_symbols.address_to_symbols(address)
                print(res)

        # sync up the GUID database from the build output
        for m in gdb.objfiles():
            if GuidNames.add_build_guid_file(str(m.filename)):
                break

        self.restore_user_state()


class EfiCmd (gdb.Command):
    """Commands for debugging EFI. efi <cmd>"""

    def __init__(self):
        super(EfiCmd, self).__init__("efi",
                                     gdb.COMMAND_NONE,
                                     gdb.COMPLETE_NONE,
                                     True)

    def invoke(self, arg, from_tty):
        '''default to loading symbols'''
        if '-h' in arg or '--help' in arg:
            gdb.execute('help efi')
        else:
            # default to loading all symbols
            gdb.execute('efi symbols --extended')


class LoadEmulatorEfiSymbols(gdb.Breakpoint):
    '''
    breakpoint for EmulatorPkg to load symbols
    Note: make sure SecGdbScriptBreak is not optimized away!
    Also turn off the dlopen() flow like on macOS.
    '''
    def stop(self):
        symbols = EfiSymbols()
        # Emulator adds SizeOfHeaders so we need file alignment to search
        symbols.configure_search(0x20)

        frame = gdb.newest_frame()

        try:
            # gdb was looking at spill address, pre spill :(
            LoadAddress = frame.read_register('rdx')
            AddSymbolFlag = frame.read_register('rcx')
        except gdb.error:
            LoadAddress = frame.read_var('LoadAddress')
            AddSymbolFlag = frame.read_var('AddSymbolFlag')

        if AddSymbolFlag == 1:
            res = symbols.address_to_symbols(LoadAddress)
        else:
            res = symbols.unload_symbols(LoadAddress)
        print(res)

        # keep running
        return False


# Get python backtraces to debug errors in this script
gdb.execute("set python print-stack full")

# tell efi_debugging how to walk data structures with pointers
try:
    pointer_width = gdb.lookup_type('int').pointer().sizeof
except ValueError:
    pointer_width = 8
patch_ctypes(pointer_width)

register_pretty_printer(None, build_pretty_printer(), replace=True)

# gdb commands that we are adding
# add `efi` prefix gdb command
EfiCmd()

# subcommands for `efi`
EfiSymbolsCmd()
EfiTablesCmd()
EfiHobCmd()
EfiDevicePathCmd()
EfiGuidCmd()

#
bp = LoadEmulatorEfiSymbols('SecGdbScriptBreak', internal=True)
if bp.pending:
    try:
        gdb.selected_frame()
        # Not the emulator so do this when you attach
        gdb.execute('efi symbols --frame --extended', True)
        gdb.execute('bt')
        # If you want to skip the above commands comment them out
        pass
    except gdb.error:
        # If you load the script and there is no target ignore the error.
        pass
else:
    # start the emulator
    gdb.execute('run')
