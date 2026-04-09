#!/usr/bin/python3
'''
Copyright (c) Apple Inc. 2021
SPDX-License-Identifier: BSD-2-Clause-Patent

Example usage:
OvmfPkg/build.sh qemu -gdb tcp::9000
lldb -o "gdb-remote localhost:9000" -o "command script import efi_lldb.py"
'''

import optparse
import shlex
import subprocess
import uuid
import sys
import os
from pathlib import Path
from efi_debugging import EfiDevicePath, EfiConfigurationTable, EfiTpl
from efi_debugging import EfiHob, GuidNames, EfiStatusClass, EfiBootMode
from efi_debugging import PeTeImage, patch_ctypes

try:
    # Just try for LLDB in case PYTHONPATH is already correctly setup
    import lldb
except ImportError:
    try:
        env = os.environ.copy()
        env['LLDB_DEFAULT_PYTHON_VERSION'] = str(sys.version_info.major)
        lldb_python_path = subprocess.check_output(
            ["xcrun", "lldb", "-P"], env=env).decode("utf-8").strip()
        sys.path.append(lldb_python_path)
        import lldb
    except ValueError:
        print("Couldn't find LLDB.framework from lldb -P")
        print("PYTHONPATH should match the currently selected lldb")
        sys.exit(-1)


class LldbFileObject(object):
    '''
    Class that fakes out file object to abstract lldb from the generic code.
    For lldb this is memory so we don't have a concept of the end of the file.
    '''

    def __init__(self, process):
        # _exe_ctx is lldb.SBExecutionContext
        self._process = process
        self._offset = 0
        self._SBError = lldb.SBError()

    def tell(self):
        return self._offset

    def read(self, size=-1):
        if size == -1:
            # arbitrary default size
            size = 0x1000000

        data = self._process.ReadMemory(self._offset, size, self._SBError)
        if self._SBError.fail:
            raise MemoryError(
                f'lldb could not read memory 0x{size:x} '
                f' bytes from 0x{self._offset:08x}')
        else:
            return data

    def readable(self):
        return True

    def seek(self, offset, whence=0):
        if whence == 0:
            self._offset = offset
        elif whence == 1:
            self._offset += offset
        else:
            # whence == 2 is seek from end
            raise NotImplementedError

    def seekable(self):
        return True

    def write(self, data):
        result = self._process.WriteMemory(self._offset, data, self._SBError)
        if self._SBError.fail:
            raise MemoryError(
                f'lldb could not write memory to 0x{self._offset:08x}')
        return result

    def writable(self):
        return True

    def truncate(self, size=None):
        raise NotImplementedError

    def flush(self):
        raise NotImplementedError

    def fileno(self):
        raise NotImplementedError


class EfiSymbols:
    """
    Class to manage EFI Symbols
    You need to pass file, and exe_ctx to load symbols.
    You can print(EfiSymbols()) to see the currently loaded symbols
    """

    loaded = {}
    stride = None
    range = None
    verbose = False

    def __init__(self, target=None):
        if target:
            EfiSymbols.target = target
            EfiSymbols._file = LldbFileObject(target.process)

    @ classmethod
    def __str__(cls):
        return ''.join(f'{pecoff}\n' for (pecoff, _) in cls.loaded.values())

    @ classmethod
    def configure_search(cls, stride, range, verbose=False):
        cls.stride = stride
        cls.range = range
        cls.verbose = verbose

    @ classmethod
    def clear(cls):
        cls.loaded = {}

    @ classmethod
    def add_symbols_for_pecoff(cls, pecoff):
        '''Tell lldb the location of the .text and .data sections.'''

        if pecoff.LoadAddress in cls.loaded:
            return 'Already Loaded: '

        module = cls.target.AddModule(None, None, str(pecoff.CodeViewUuid))
        if not module:
            module = cls.target.AddModule(pecoff.CodeViewPdb,
                                          None,
                                          str(pecoff.CodeViewUuid))
        if module.IsValid():
            SBError = cls.target.SetModuleLoadAddress(
                module, pecoff.LoadAddress + pecoff.TeAdjust)
            if SBError.success:
                cls.loaded[pecoff.LoadAddress] = (pecoff, module)
                return ''

        return 'Symbols NOT FOUND: '

    @ classmethod
    def address_to_symbols(cls, address, reprobe=False):
        '''
        Given an address search backwards for a PE/COFF (or TE) header
        and load symbols. Return a status string.
        '''
        if not isinstance(address, int):
            address = int(address)

        pecoff, _ = cls.address_in_loaded_pecoff(address)
        if not reprobe and pecoff is not None:
            # skip the probe of the remote
            return f'{pecoff} is already loaded'

        pecoff = PeTeImage(cls._file, None)
        if pecoff.pcToPeCoff(address, cls.stride, cls.range):
            res = cls.add_symbols_for_pecoff(pecoff)
            return f'{res}{pecoff}'
        else:
            return f'0x{address:08x} not in a PE/COFF (or TE) image'

    @ classmethod
    def address_in_loaded_pecoff(cls, address):
        if not isinstance(address, int):
            address = int(address)

        for (pecoff, module) in cls.loaded.values():
            if (address >= pecoff.LoadAddress and
                    address <= pecoff.EndLoadAddress):

                return pecoff, module

        return None, None

    @ classmethod
    def unload_symbols(cls, address):
        pecoff, module = cls.address_in_loaded_pecoff(address)
        if module:
            name = str(module)
            cls.target.ClearModuleLoadAddress(module)
            cls.target.RemoveModule(module)
            del cls.loaded[pecoff.LoadAddress]
            return f'{name:s} was unloaded'
        return f'0x{address:x} was not in a loaded image'


def arg_to_address(frame, arg):
    ''' convert an lldb command arg into a memory address (addr_t)'''

    if arg is None:
        return None

    arg_str = arg if isinstance(arg, str) else str(arg)
    SBValue = frame.EvaluateExpression(arg_str)
    if SBValue.error.fail:
        return arg

    if (SBValue.TypeIsPointerType() or
            SBValue.value_type == lldb.eValueTypeRegister or
            SBValue.value_type == lldb.eValueTypeRegisterSet or
            SBValue.value_type == lldb.eValueTypeConstResult):
        try:
            addr = SBValue.GetValueAsAddress()
        except ValueError:
            addr = SBValue.unsigned
    else:
        try:
            addr = SBValue.address_of.GetValueAsAddress()
        except ValueError:
            addr = SBValue.address_of.unsigned

    return addr


def arg_to_data(frame, arg):
    ''' convert an lldb command arg into a data vale (uint32_t/uint64_t)'''
    if not isinstance(arg, str):
        arg_str = str(str)

    SBValue = frame.EvaluateExpression(arg_str)
    return SBValue.unsigned


class EfiDevicePathCommand:

    def create_options(self):
        ''' standard lldb command help/options parser'''
        usage = "usage: %prog [options]"
        description = '''Command that can EFI Config Tables
'''

        # Pass add_help_option = False, since this keeps the command in line
        # with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        self.parser = optparse.OptionParser(
            description=description,
            prog='devicepath',
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

    def get_short_help(self):
        '''standard lldb function method'''
        return "Display EFI Tables"

    def get_long_help(self):
        '''standard lldb function method'''
        return self.help_string

    def __init__(self, debugger, internal_dict):
        '''standard lldb function method'''
        self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        '''standard lldb function method'''
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            dev_list = []
            for arg in args:
                dev_list.append(arg_to_address(exe_ctx.frame, arg))
        except ValueError:
            # if you don't handle exceptions, passing an incorrect argument
            # to the OptionParser will cause LLDB to exit (courtesy of
            # OptParse dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return

        if options.help:
            self.parser.print_help()
            return

        file = LldbFileObject(exe_ctx.process)

        for dev_addr in dev_list:
            if options.node:
                print(EfiDevicePath(file).device_path_node_str(
                    dev_addr, options.verbose))
            else:
                device_path = EfiDevicePath(file, dev_addr, options.verbose)
                if device_path.valid():
                    print(device_path)


class EfiHobCommand:
    def create_options(self):
        ''' standard lldb command help/options parser'''
        usage = "usage: %prog [options]"
        description = '''Command that can EFI dump EFI HOBs'''

        # Pass add_help_option = False, since this keeps the command in line
        # with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        self.parser = optparse.OptionParser(
            description=description,
            prog='table',
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

    def get_short_help(self):
        '''standard lldb function method'''
        return "Display EFI Hobs"

    def get_long_help(self):
        '''standard lldb function method'''
        return self.help_string

    def __init__(self, debugger, internal_dict):
        '''standard lldb function method'''
        self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        '''standard lldb function method'''
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, _) = self.parser.parse_args(command_args)
        except ValueError:
            # if you don't handle exceptions, passing an incorrect argument
            # to the OptionParser will cause LLDB to exit (courtesy of
            # OptParse dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return

        if options.help:
            self.parser.print_help()
            return

        address = arg_to_address(exe_ctx.frame, options.address)

        file = LldbFileObject(exe_ctx.process)
        hob = EfiHob(file, address, options.verbose).get_hob_by_type(
            options.type)
        print(hob)


class EfiTableCommand:

    def create_options(self):
        ''' standard lldb command help/options parser'''
        usage = "usage: %prog [options]"
        description = '''Command that can display EFI Config Tables
'''

        # Pass add_help_option = False, since this keeps the command in line
        # with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        self.parser = optparse.OptionParser(
            description=description,
            prog='table',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

    def get_short_help(self):
        '''standard lldb function method'''
        return "Display EFI Tables"

    def get_long_help(self):
        '''standard lldb function method'''
        return self.help_string

    def __init__(self, debugger, internal_dict):
        '''standard lldb function method'''
        self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        '''standard lldb function method'''
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, _) = self.parser.parse_args(command_args)
        except ValueError:
            # if you don't handle exceptions, passing an incorrect argument
            # to the OptionParser will cause LLDB to exit (courtesy of
            # OptParse dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return

        if options.help:
            self.parser.print_help()
            return

        gST = exe_ctx.target.FindFirstGlobalVariable('gST')
        if gST.error.fail:
            print('Error: This command requires symbols for gST to be loaded')
            return

        file = LldbFileObject(exe_ctx.process)
        table = EfiConfigurationTable(file, gST.unsigned)
        if table:
            print(table, '\n')


class EfiGuidCommand:

    def create_options(self):
        ''' standard lldb command help/options parser'''
        usage = "usage: %prog [options]"
        description = '''
            Command that can display all EFI GUID's or give info on a
            specific GUID's
            '''
        self.parser = optparse.OptionParser(
            description=description,
            prog='guid',
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

    def get_short_help(self):
        '''standard lldb function method'''
        return "Display EFI GUID's"

    def get_long_help(self):
        '''standard lldb function method'''
        return self.help_string

    def __init__(self, debugger, internal_dict):
        '''standard lldb function method'''
        self.create_options()
        self.help_string = self.parser.format_help()

    def __call__(self, debugger, command, exe_ctx, result):
        '''standard lldb function method'''
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, args) = self.parser.parse_args(command_args)
            if len(args) >= 1:
                # guid { 0x414e6bdd, 0xe47b, 0x47cc,
                #      { 0xb2, 0x44, 0xbb, 0x61, 0x02, 0x0c,0xf5, 0x16 }}
                # this generates multiple args
                arg = ' '.join(args)
        except ValueError:
            # if you don't handle exceptions, passing an incorrect argument
            # to the OptionParser will cause LLDB to exit (courtesy of
            # OptParse dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return

        if options.help:
            self.parser.print_help()
            return

        if options.new:
            guid = uuid.uuid4()
            print(str(guid).upper())
            print(GuidNames.to_c_guid(guid))
            return

        if len(args) > 0:
            if GuidNames.is_guid_str(arg):
                # guid 05AD34BA-6F02-4214-952E-4DA0398E2BB9
                key = arg.lower()
                name = GuidNames.to_name(key)
            elif GuidNames.is_c_guid(arg):
                # guid { 0x414e6bdd, 0xe47b, 0x47cc,
                #      { 0xb2, 0x44, 0xbb, 0x61, 0x02, 0x0c,0xf5, 0x16 }}
                key = GuidNames.from_c_guid(arg)
                name = GuidNames.to_name(key)
            else:
                # guid gEfiDxeServicesTableGuid
                name = arg
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


class EfiSymbolicateCommand(object):
    '''Class to abstract an lldb command'''

    def create_options(self):
        ''' standard lldb command help/options parser'''
        usage = "usage: %prog [options]"
        description = '''Command that can load EFI PE/COFF and TE image
        symbols. If you are having trouble in PEI try adding --pei.
        '''

        # Pass add_help_option = False, since this keeps the command in line
        # with lldb commands, and we wire up "help command" to work by
        # providing the long & short help methods below.
        self.parser = optparse.OptionParser(
            description=description,
            prog='efi_symbols',
            usage=usage,
            add_help_option=False)

        self.parser.add_option(
            '-a',
            '--address',
            type="int",
            dest='address',
            help='Load symbols for image at address',
            default=None)

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
            help='Try to load all symbols based on config tables.',
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
            '-h',
            '--help',
            action='store_true',
            dest='help',
            help='Show help for the command',
            default=False)

    def get_short_help(self):
        '''standard lldb function method'''
        return (
            "Load symbols based on an address that is part of"
            " a PE/COFF EFI image.")

    def get_long_help(self):
        '''standard lldb function method'''
        return self.help_string

    def __init__(self, debugger, unused):
        '''standard lldb function method'''
        self.create_options()
        self.help_string = self.parser.format_help()

    def lldb_print(self, lldb_str):
        # capture command out like an lldb command
        self.result.PutCString(lldb_str)
        # flush the output right away
        self.result.SetImmediateOutputFile(
            self.exe_ctx.target.debugger.GetOutputFile())

    def __call__(self, debugger, command, exe_ctx, result):
        '''standard lldb function method'''
        # Use the Shell Lexer to properly parse up command options just like a
        # shell would
        command_args = shlex.split(command)

        try:
            (options, _) = self.parser.parse_args(command_args)
        except ValueError:
            # if you don't handle exceptions, passing an incorrect argument
            # to the OptionParser will cause LLDB to exit (courtesy of
            # OptParse dealing with argument errors by throwing SystemExit)
            result.SetError("option parsing failed")
            return

        if options.help:
            self.parser.print_help()
            return

        file = LldbFileObject(exe_ctx.process)
        efi_symbols = EfiSymbols(exe_ctx.target)
        self.result = result
        self.exe_ctx = exe_ctx

        if options.pei:
            # XIP code ends up on a 4 byte boundary.
            options.stride = 4
            options.range = 0x100000
        efi_symbols.configure_search(options.stride, options.range)

        if not options.pc and options.address is None:
            # default to
            options.frame = True

        if options.frame:
            if not exe_ctx.frame.IsValid():
                result.SetError("invalid frame")
                return

            threads = exe_ctx.process.threads if options.thread else [
                exe_ctx.thread]

            for thread in threads:
                for frame in thread:
                    res = efi_symbols.address_to_symbols(frame.pc)
                    self.lldb_print(res)

        else:
            if options.address is not None:
                address = options.address
            elif options.pc:
                try:
                    address = exe_ctx.thread.GetSelectedFrame().pc
                except ValueError:
                    result.SetError("invalid pc")
                    return
            else:
                address = 0

            res = efi_symbols.address_to_symbols(address.pc)
            print(res)

        if options.extended:

            gST = exe_ctx.target.FindFirstGlobalVariable('gST')
            if gST.error.fail:
                print('Error: This command requires symbols to be loaded')
            else:
                table = EfiConfigurationTable(file, gST.unsigned)
                for address, _ in table.DebugImageInfo():
                    res = efi_symbols.address_to_symbols(address)
                    self.lldb_print(res)

        # keep trying module file names until we find a GUID xref file
        for m in exe_ctx.target.modules:
            if GuidNames.add_build_guid_file(str(m.file)):
                break


def CHAR16_TypeSummary(valobj, internal_dict):
    '''
    Display CHAR16 as a String in the debugger.
    Note: utf-8 is returned as that is the value for the debugger.
    '''
    SBError = lldb.SBError()
    Str = ''
    if valobj.TypeIsPointerType():
        if valobj.GetValueAsUnsigned() == 0:
            return "NULL"

        # CHAR16 *   max string size 1024
        for i in range(1024):
            Char = valobj.GetPointeeData(i, 1).GetUnsignedInt16(SBError, 0)
            if SBError.fail or Char == 0:
                break
            Str += chr(Char)
        return 'L"' + Str + '"'

    if valobj.num_children == 0:
        # CHAR16
        return "L'" + chr(valobj.unsigned) + "'"

    else:
        # CHAR16 []
        for i in range(valobj.num_children):
            Char = valobj.GetChildAtIndex(i).data.GetUnsignedInt16(SBError, 0)
            if Char == 0:
                break
            Str += chr(Char)
        return 'L"' + Str + '"'

    return Str


def CHAR8_TypeSummary(valobj, internal_dict):
    '''
    Display CHAR8 as a String in the debugger.
    Note: utf-8 is returned as that is the value for the debugger.
    '''
    SBError = lldb.SBError()
    Str = ''
    if valobj.TypeIsPointerType():
        if valobj.GetValueAsUnsigned() == 0:
            return "NULL"

        # CHAR8 *   max string size 1024
        for i in range(1024):
            Char = valobj.GetPointeeData(i, 1).GetUnsignedInt8(SBError, 0)
            if SBError.fail or Char == 0:
                break
            Str += chr(Char)
        Str = '"' + Str + '"'
        return Str

    if valobj.num_children == 0:
        # CHAR8
        return "'" + chr(valobj.unsigned) + "'"
    else:
        # CHAR8 []
        for i in range(valobj.num_children):
            Char = valobj.GetChildAtIndex(i).data.GetUnsignedInt8(SBError, 0)
            if SBError.fail or Char == 0:
                break
            Str += chr(Char)
        return '"' + Str + '"'

    return Str


def EFI_STATUS_TypeSummary(valobj, internal_dict):
    if valobj.TypeIsPointerType():
        return ''
    return str(EfiStatusClass(valobj.unsigned))


def EFI_TPL_TypeSummary(valobj, internal_dict):
    if valobj.TypeIsPointerType():
        return ''
    return str(EfiTpl(valobj.unsigned))


def EFI_GUID_TypeSummary(valobj, internal_dict):
    if valobj.TypeIsPointerType():
        return ''
    return str(GuidNames(bytes(valobj.data.uint8)))


def EFI_BOOT_MODE_TypeSummary(valobj, internal_dict):
    if valobj.TypeIsPointerType():
        return ''
    '''Return #define name for EFI_BOOT_MODE'''
    return str(EfiBootMode(valobj.unsigned))


def lldb_type_formaters(debugger, mod_name):
    '''Teach lldb about EFI types'''

    category = debugger.GetDefaultCategory()
    FormatBool = lldb.SBTypeFormat(lldb.eFormatBoolean)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("BOOLEAN"), FormatBool)

    FormatHex = lldb.SBTypeFormat(lldb.eFormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT64"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT64"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT32"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT32"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT16"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT16"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINT8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INT8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("UINTN"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("INTN"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("CHAR8"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("CHAR16"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier(
        "EFI_PHYSICAL_ADDRESS"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier(
        "PHYSICAL_ADDRESS"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier("EFI_LBA"), FormatHex)
    category.AddTypeFormat(
        lldb.SBTypeNameSpecifier("EFI_BOOT_MODE"), FormatHex)
    category.AddTypeFormat(lldb.SBTypeNameSpecifier(
        "EFI_FV_FILETYPE"), FormatHex)

    #
    # Smart type printing for EFI
    #

    debugger.HandleCommand(
        f'type summary add GUID - -python-function '
        f'{mod_name}.EFI_GUID_TypeSummary')
    debugger.HandleCommand(
        f'type summary add EFI_GUID --python-function '
        f'{mod_name}.EFI_GUID_TypeSummary')
    debugger.HandleCommand(
        f'type summary add EFI_STATUS --python-function '
        f'{mod_name}.EFI_STATUS_TypeSummary')
    debugger.HandleCommand(
        f'type summary add EFI_TPL - -python-function '
        f'{mod_name}.EFI_TPL_TypeSummary')
    debugger.HandleCommand(
        f'type summary add EFI_BOOT_MODE --python-function '
        f'{mod_name}.EFI_BOOT_MODE_TypeSummary')

    debugger.HandleCommand(
        f'type summary add CHAR16 --python-function '
        f'{mod_name}.CHAR16_TypeSummary')

    # W605 this is the correct escape sequence for the lldb command
    debugger.HandleCommand(
        f'type summary add --regex "CHAR16 \[[0-9]+\]" '  # noqa: W605
        f'--python-function {mod_name}.CHAR16_TypeSummary')

    debugger.HandleCommand(
        f'type summary add CHAR8 --python-function '
        f'{mod_name}.CHAR8_TypeSummary')

    # W605 this is the correct escape sequence for the lldb command
    debugger.HandleCommand(
        f'type summary add --regex "CHAR8 \[[0-9]+\]"  '  # noqa: W605
        f'--python-function {mod_name}.CHAR8_TypeSummary')


class LldbWorkaround:
    needed = True

    @classmethod
    def activate(cls):
        if cls.needed:
            lldb.debugger.HandleCommand("process handle SIGALRM -n false")
            cls.needed = False


def LoadEmulatorEfiSymbols(frame, bp_loc, internal_dict):
    #
    # This is an lldb breakpoint script, and assumes the breakpoint is on a
    # function with the same prototype as SecGdbScriptBreak(). The
    # argument names are important as lldb looks them up.
    #
    # VOID
    # SecGdbScriptBreak (
    #   char                *FileName,
    #   int                 FileNameLength,
    #   long unsigned int   LoadAddress,
    #   int                 AddSymbolFlag
    #   )
    # {
    #   return;
    # }
    #
    # When the emulator loads a PE/COFF image, it calls the stub function with
    # the filename of the symbol file, the length of the FileName, the
    # load address and a flag to indicate if this is a load or unload operation
    #
    LldbWorkaround().activate()

    symbols = EfiSymbols(frame.thread.process.target)
    LoadAddress = frame.FindVariable("LoadAddress").unsigned
    if frame.FindVariable("AddSymbolFlag").unsigned == 1:
        res = symbols.address_to_symbols(LoadAddress)
    else:
        res = symbols.unload_symbols(LoadAddress)
    print(res)

    # make breakpoint command continue
    return False


def __lldb_init_module(debugger, internal_dict):
    '''
    This initializer is being run from LLDB in the embedded command interpreter
    '''

    mod_name = Path(__file__).stem
    lldb_type_formaters(debugger, mod_name)

    # Add any commands contained in this module to LLDB
    debugger.HandleCommand(
        f'command script add -c {mod_name}.EfiSymbolicateCommand efi_symbols')
    debugger.HandleCommand(
        f'command script add -c {mod_name}.EfiGuidCommand guid')
    debugger.HandleCommand(
        f'command script add -c {mod_name}.EfiTableCommand table')
    debugger.HandleCommand(
        f'command script add -c {mod_name}.EfiHobCommand hob')
    debugger.HandleCommand(
        f'command script add -c {mod_name}.EfiDevicePathCommand devicepath')

    print('EFI specific commands have been installed.')

    # patch the ctypes c_void_p values if the debuggers OS and EFI have
    # different ideas on the size of the debug.
    try:
        patch_ctypes(debugger.GetSelectedTarget().addr_size)
    except ValueError:
        # incase the script is imported and the debugger has not target
        # defaults to sizeof(UINTN) == sizeof(UINT64)
        patch_ctypes()

    try:
        target = debugger.GetSelectedTarget()
        if target.FindFunctions('SecGdbScriptBreak').symbols:
            breakpoint = target.BreakpointCreateByName('SecGdbScriptBreak')
            # Set the emulator breakpoints, if we are in the emulator
            cmd = 'breakpoint command add -s python -F '
            cmd += f'efi_lldb.LoadEmulatorEfiSymbols {breakpoint.GetID()}'
            debugger.HandleCommand(cmd)
            print('Type r to run emulator.')
        else:
            raise ValueError("No Emulator Symbols")

    except ValueError:
        # default action when the script is imported
        debugger.HandleCommand("efi_symbols --frame --extended")
        debugger.HandleCommand("register read")
        debugger.HandleCommand("bt all")


if __name__ == '__main__':
    pass
