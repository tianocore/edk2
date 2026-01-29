#!/usr/bin/python

#
#  Copyright (c) 2014, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

import getopt
import operator
import os
import pickle
import sys
from sys import argv
from cStringIO import StringIO

modules = {}
functions = {}
functions_addr = {}

def usage():
    print "-t,--trace: Location of the Trace file"
    print "-s,--symbols: Location of the symbols and modules"

def get_address_from_string(address):
    return int(address.strip("S:").strip("N:").strip("EL2:").strip("EL1:"), 16)

def get_module_from_addr(modules, addr):
    for key,value in modules.items():
        if (value['start'] <= addr) and (addr <= value['end']):
            return key
    return None

def add_cycles_to_function(functions, func_name, addr, cycles):
    if func_name != "<Unknown>":
        # Check if we are still in the previous function
        if add_cycles_to_function.prev_func_name == func_name:
            add_cycles_to_function.prev_entry['cycles'] += cycles
            return (add_cycles_to_function.prev_func_name, add_cycles_to_function.prev_module_name)

        if func_name in functions.keys():
            for module_name, module_value in functions[func_name].iteritems():
                if (module_value['start'] <= addr) and (addr < module_value['end']):
                    module_value['cycles'] += cycles

                    add_cycles_to_function.prev_func_name   = func_name
                    add_cycles_to_function.prev_module_name = module_name
                    add_cycles_to_function.prev_entry       = module_value
                    return (func_name, module_name)
                elif (module_value['end'] == 0):
                    module_value['cycles'] += cycles

                    add_cycles_to_function.prev_func_name   = func_name
                    add_cycles_to_function.prev_module_name = module_name
                    add_cycles_to_function.prev_entry       = module_value
                    return (func_name, module_name)

        # Workaround to fix the 'info func' limitation that does not expose the 'static' function
        module_name = get_module_from_addr(modules, addr)
        functions[func_name] = {}
        functions[func_name][module_name] = {}
        functions[func_name][module_name]['start']  = 0
        functions[func_name][module_name]['end']    = 0
        functions[func_name][module_name]['cycles'] = cycles
        functions[func_name][module_name]['count']  = 0

        add_cycles_to_function.prev_func_name   = func_name
        add_cycles_to_function.prev_module_name = module_name
        add_cycles_to_function.prev_entry       = functions[func_name][module_name]
        return (func_name, module_name)
    else:
        # Check if we are still in the previous function
        if (add_cycles_to_function.prev_entry is not None) and (add_cycles_to_function.prev_entry['start'] <= addr) and (addr < add_cycles_to_function.prev_entry['end']):
            add_cycles_to_function.prev_entry['cycles'] += cycles
            return (add_cycles_to_function.prev_func_name, add_cycles_to_function.prev_module_name)

        # Generate the key for the given address
        key = addr & ~0x0FFF

        if key not in functions_addr.keys():
            if 'Unknown' not in functions.keys():
                functions['Unknown'] = {}
            if 'Unknown' not in functions['Unknown'].keys():
                functions['Unknown']['Unknown'] = {}
                functions['Unknown']['Unknown']['cycles'] = 0
                functions['Unknown']['Unknown']['count'] = 0
            functions['Unknown']['Unknown']['cycles'] += cycles

            add_cycles_to_function.prev_func_name = None
            return None

        for func_key, module in functions_addr[key].iteritems():
            for module_key, module_value in module.iteritems():
                if (module_value['start'] <= addr) and (addr < module_value['end']):
                    module_value['cycles'] += cycles

                    # In case o <Unknown> we prefer to fallback on the direct search
                    add_cycles_to_function.prev_func_name   = func_key
                    add_cycles_to_function.prev_module_name = module_key
                    add_cycles_to_function.prev_entry       = module_value
                    return (func_key, module_key)

    print "Warning: Function %s @ 0x%x not found" % (func_name, addr)

    add_cycles_to_function.prev_func_name = None
    return None

# Static variables for the previous function
add_cycles_to_function.prev_func_name = None
add_cycles_to_function.prev_entry     = None

def trace_read():
    global trace_process
    line = trace.readline()
    trace_process += len(line)
    return line

#
# Parse arguments
#
trace_name = None
symbols_file = None

opts,args = getopt.getopt(sys.argv[1:], "ht:vs:v", ["help","trace=","symbols="])
if (opts is None) or (not opts):
    usage()
    sys.exit()

for o,a in opts:
    if o in ("-h","--help"):
        usage()
        sys.exit()
    elif o in ("-t","--trace"):
        trace_name = a
    elif o in ("-s","--symbols"):
        symbols_file = a
    else:
        assert False, "Unhandled option (%s)" % o

#
# We try first to see if we run the script from DS-5
#
try:
    from arm_ds.debugger_v1 import Debugger
    from arm_ds.debugger_v1 import DebugException

    # Debugger object for accessing the debugger
    debugger = Debugger()

    # Initialisation commands
    ec = debugger.getExecutionContext(0)
    ec.getExecutionService().stop()
    ec.getExecutionService().waitForStop()
    # in case the execution context reference is out of date
    ec = debugger.getExecutionContext(0)

    #
    # Get the module name and their memory range
    #
    info_file = ec.executeDSCommand("info file")
    info_file_str = StringIO(info_file)

    line = info_file_str.readline().strip('\n')
    while line != '':
        if ("Symbols from" in line):
            # Get the module name from the line 'Symbols from "/home/...."'
            module_name = line.split("\"")[1].split("/")[-1]
            modules[module_name] = {}

            # Look for the text section
            line = info_file_str.readline().strip('\n')
            while (line != '') and ("Symbols from" not in line):
                if ("ER_RO" in line):
                    modules[module_name]['start'] = get_address_from_string(line.split()[0])
                    modules[module_name]['end']   = get_address_from_string(line.split()[2])
                    line = info_file_str.readline().strip('\n')
                    break;
                if (".text" in line):
                    modules[module_name]['start'] = get_address_from_string(line.split()[0])
                    modules[module_name]['end']   = get_address_from_string(line.split()[2])
                    line = info_file_str.readline().strip('\n')
                    break;
                line = info_file_str.readline().strip('\n')
        line = info_file_str.readline().strip('\n')

    #
    # Get the function name and their memory range
    #
    info_func = ec.executeDSCommand("info func")
    info_func_str = StringIO(info_func)

    # Skip the first line 'Low-level symbols ...'
    line = info_func_str.readline().strip('\n')
    func_prev = None
    while line != '':
        # We ignore all the functions after 'Functions in'
        if ("Functions in " in line):
            line = info_func_str.readline().strip('\n')
            while line != '':
                line = info_func_str.readline().strip('\n')
            line = info_func_str.readline().strip('\n')
            continue

        if ("Low-level symbols" in line):
            # We need to fixup the last function of the module
            if func_prev is not None:
                func_prev['end'] = modules[module_name]['end']
                func_prev = None

            line = info_func_str.readline().strip('\n')
            continue

        func_name = line.split()[1]
        func_start = get_address_from_string(line.split()[0])
        module_name = get_module_from_addr(modules, func_start)

        if func_name not in functions.keys():
            functions[func_name] = {}
        functions[func_name][module_name] = {}
        functions[func_name][module_name]['start'] = func_start
        functions[func_name][module_name]['cycles'] = 0
        functions[func_name][module_name]['count'] = 0

        # Set the end address of the previous function
        if func_prev is not None:
            func_prev['end'] = func_start
        func_prev = functions[func_name][module_name]

        line = info_func_str.readline().strip('\n')

    # Fixup the last function
    func_prev['end'] = modules[module_name]['end']

    if symbols_file is not None:
        pickle.dump((modules, functions), open(symbols_file, "w"))
except:
    if symbols_file is None:
        print "Error: Symbols file is required when run out of ARM DS-5"
        sys.exit()

    (modules, functions) = pickle.load(open(symbols_file, "r"))

#
# Build optimized table for the <Unknown> functions
#
functions_addr = {}
for func_key, module in functions.iteritems():
    for module_key, module_value in module.iteritems():
        key = module_value['start'] & ~0x0FFF
        if key not in functions_addr.keys():
            functions_addr[key] = {}
        if func_key not in functions_addr[key].keys():
            functions_addr[key][func_key] = {}
        functions_addr[key][func_key][module_key] = module_value

#
# Process the trace file
#
if trace_name is None:
    sys.exit()

trace = open(trace_name, "r")
trace_size = os.path.getsize(trace_name)
trace_process = 0

# Get the column names from the first line
columns = trace_read().split()
column_addr     = columns.index('Address')
column_cycles   = columns.index('Cycles')
column_function = columns.index('Function')

line = trace_read()
i = 0
prev_callee = None
while line:
    try:
        func_name = line.split('\t')[column_function].strip()
        address   = get_address_from_string(line.split('\t')[column_addr])
        cycles    = int(line.split('\t')[column_cycles])
        callee = add_cycles_to_function(functions, func_name, address, cycles)
        if (prev_callee != None) and (prev_callee != callee):
            functions[prev_callee[0]][prev_callee[1]]['count'] += 1
        prev_callee = callee
    except ValueError:
        pass
    line = trace_read()
    if ((i % 1000000) == 0) and (i != 0):
        percent = (trace_process * 100.00) / trace_size
        print "Processing file ... (%.2f %%)" % (percent)
    i = i + 1

# Fixup the last callee
functions[prev_callee[0]][prev_callee[1]]['count'] += 1

#
# Process results
#
functions_cycles     = {}
all_functions_cycles = {}
total_cycles         = 0

for func_key, module in functions.iteritems():
    for module_key, module_value in module.iteritems():
        key = "%s/%s" % (module_key, func_key)
        functions_cycles[key] = (module_value['cycles'], module_value['count'])
        total_cycles += module_value['cycles']

        if func_key not in all_functions_cycles.keys():
            all_functions_cycles[func_key] = (module_value['cycles'], module_value['count'])
        else:
            all_functions_cycles[func_key] = tuple(map(sum, zip(all_functions_cycles[func_key], (module_value['cycles'], module_value['count']))))

sorted_functions_cycles     = sorted(functions_cycles.iteritems(), key=operator.itemgetter(1), reverse = True)
sorted_all_functions_cycles = sorted(all_functions_cycles.items(), key=operator.itemgetter(1), reverse = True)

print
print "----"
for (key,value) in sorted_functions_cycles[:20]:
    if value[0] != 0:
        print "%s (cycles: %d - %d%%, count: %d)" % (key, value[0], (value[0] * 100) / total_cycles, value[1])
    else:
        break;
print "----"
for (key,value) in sorted_all_functions_cycles[:20]:
    if value[0] != 0:
        print "%s (cycles: %d - %d%%, count: %d)" % (key, value[0], (value[0] * 100) / total_cycles, value[1])
    else:
        break;
