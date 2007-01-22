#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Tools and build configuration"""

from sets import Set

class Config(dict):
    def __init__(self, file):
        """file (target configuration file)"""
        configFile = open(file)
        while True:
            line = configFile.readline()
            if line == "": break    ## no more line

            line = line.strip()
            # skip blank line
            if line == "": continue
            # skip comment line
            if line[0] == '#': continue
            # skip invalid line
            if line[0] == '=':
                print "! invalid configuration:", line
                continue

            defStrings = line.split('=', 1)
            name = defStrings[0].strip()
            value = defStrings[1].strip()
            self[name] = value

        configFile.close()

    def __getitem__(self, attr):
        if attr not in self:
            return ""

        value = dict.__getitem__(self, attr)
        if value == None:
            value = ""
        return value

class ToolConfig(dict):
    def __init__(self, file):
        """file (tools configuration file path)"""
        self.Targets = Set()
        self.Toolchains = Set()
        self.Archs = Set()
        self.ToolCodes = Set()
        self.Families = Set()
        self.Attributes = Set(["FAMILY", "NAME", "PATH", "FLAGS", "EXT", "DPATH", "SPATH", "LIBPATH", "INCLUDEPATH"])
        
        configFile = open(file)
        while True:
            line = configFile.readline()
            if line == "": break

            line = line.strip()
            # skip blank line
            if line == "": continue
            # skip comment line
            if line[0] == '#': continue
            # skip invalid line
            if line[0] == '=':
                print "! invalid definition:", line
                continue

            # split the definition at the first "="
            tool_def = line.split('=', 1)
            name = tool_def[0].strip()
            value = tool_def[1].strip()
            
            # the name of a tool definition must have five parts concatenated by "_"
            keys = name.split('_')
            # skip non-definition line
            if len(keys) < 5: continue
        
            keys = (keys[1], keys[0], keys[2], keys[3], keys[4])
            self[keys] = value
            
            ###############################################
            ## statistics
            ###############################################
            if keys[0] != '*': self.Toolchains.add(keys[0])
            if keys[1] != '*': self.Targets.add(keys[1])
            if keys[2] != '*': self.Archs.add(keys[2])
            if keys[3] != '*': self.ToolCodes.add(keys[3])
            if keys[4] == "FAMILY": self.Families.add(value)
            elif keys[4] == '*': raise Exception("No * allowed in ATTRIBUTE field")

        configFile.close()
        # expand the "*" in each field
        self.expand()

    def __getitem__(self, attrs):
        if len(attrs) != 5:
            return ""
        
        if attrs not in self:
            return ""
        
        value = dict.__getitem__(self, attrs)
        if value == None:
            value = ""
        return value
    
    def expand(self):
        summary = {}
        toolchains = []
        targets = []
        archs = []
        toolcodes = []
        for key in self:
            value = self[key]
            if key[0] == '*':
                toolchains = self.Toolchains
            else:
                toolchains = [key[0]]

            for toolchain in toolchains:
                if key[1] == '*':
                    targets = self.Targets
                else:
                    targets = [key[1]]
                    
                for target in targets:
                    if key[2] == '*':
                        archs = self.Archs
                    else:
                        archs = [key[2]]
                        
                    for arch in archs:
                        if key[3] == '*':
                            toolcodes = self.ToolCodes
                        else:
                            toolcodes = [key[3]]
                            
                        for toolcode in toolcodes:
                            attribute = key[4]
                            summary[(toolchain, target, arch, toolcode, attribute)] = value
        self.clear()
        for toolchain in self.Toolchains:
            for target in self.Targets:
                for arch in self.Archs:
                    for toolcode in self.ToolCodes:
                        key = (toolchain, target, arch, toolcode, "NAME")
                        if key not in summary: continue
                        for attr in self.Attributes:
                            key = (toolchain, target, arch, toolcode, attr)
                            if key not in summary: continue
                            self[key] = summary[key]


    def __str__(self):
        s = ""
        for entry in self:
            s += entry[0] + "_" + entry[1] + "_" + entry[2] + "_" + entry[3] + "_" + entry[4]
            s += " = " + self[entry] + "\n"
        return s

class TargetConfig(Config):
    pass

## for test
if __name__ == "__main__":
    import os
    if "WORKSPACE" not in os.environ:
        raise "No WORKSPACE given"
    cfg = ToolConfig(os.path.join(os.environ["WORKSPACE"], "Tools", "Conf", "tools_def.txt"))
    tgt = TargetConfig(os.path.join(os.environ["WORKSPACE"], "Tools", "Conf", "target.txt"))

    for key in cfg:
        print key,"=",cfg[key]
    
    print
    for name in tgt:
        print name,"=",tgt[name]
        