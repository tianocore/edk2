#
#  Copyright (c) 2021, Arm Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

from arm_ds.debugger_v1 import DebugException

import subprocess, os, edk2_debugger, re

def get_module_name(line):
    path = line.rsplit(' ')[1]
    return os.path.splitext(os.path.basename(path))[0]

def get_module_path(line):
    return line.rsplit(' ')[1]

def get_module_entrypoint(list, module_name):
    line = [i for i in list if module_name in i and re.search(r'\b'+module_name+r'\b', i)]
    if len(line) == 0:
        # Module was not loaded using DxeDispatcher or PeiDispatcher. It is a SEC module
        # Symbols for these modules are loaded from FV, not from console log
        return None

    entrypoint_str =  line[0].rsplit(' ')[4]
    return entrypoint_str.rsplit('=')[1]

def load_symbol_from_console(ec, console_file, objdump, verbose):
    if objdump is None:
        print "Error: A path to objdump tool is not specified, but -i parameter is provided"
    elif not os.path.exists(objdump):
        print "Error: Provided path to objdump is invalid: %s" % objdump
    elif not os.path.exists(console_file):
        print "Error: UEFI console file is not found: %s" % console_file
    else:

        full_list = open(console_file).read().splitlines()

        efi_list = [i for i in full_list if "EntryPoint=" in i]

        full_list = dict.fromkeys(full_list)
        full_list = [i for i in full_list if "add-symbol-file" in i]

        module_dict = {}

        for line in full_list:
            name = get_module_name(line)
            module_dict[name] = (get_module_path(line), get_module_entrypoint(efi_list, name))

        for module in module_dict:
            entrypoint_addr = module_dict[module][1]

            if entrypoint_addr is not None:
                path = module_dict[module][0]
                if not os.path.exists(path):
                    print "Module not found: " + path + ". Skipping..."
                    continue

                sp = subprocess.Popen([objdump,'-S', path], stdout = subprocess.PIPE)

                objdump_out = sp.stdout.readlines()
                entrypoint_record = [i for i in objdump_out if "<_ModuleEntryPoint>" in i]

                entrypoint_offset = entrypoint_record[0].split(' ')[0]

                load_addr = int(entrypoint_addr, 16) - int(entrypoint_offset, 16)

                edk2_debugger.load_symbol_from_file(ec, path, load_addr, verbose)
