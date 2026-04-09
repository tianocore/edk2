## @file
# Generate AutoGen.h, AutoGen.c and *.depex files
#
# Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2018, Hewlett Packard Enterprise Development, L.P.<BR>
# Copyright (c) 2019, American Megatrends, Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

## Import Modules
#
from __future__ import print_function
from __future__ import absolute_import
from Common.DataType import TAB_STAR
## Base class for AutoGen
#
#   This class just implements the cache mechanism of AutoGen objects.
#
class AutoGen(object):
    # database to maintain the objects in each child class
    __ObjectCache = {}    # (BuildTarget, ToolChain, ARCH, platform file): AutoGen object

    ## Factory method
    #
    #   @param  Class           class object of real AutoGen class
    #                           (WorkspaceAutoGen, ModuleAutoGen or PlatformAutoGen)
    #   @param  Workspace       Workspace directory or WorkspaceAutoGen object
    #   @param  MetaFile        The path of meta file
    #   @param  Target          Build target
    #   @param  Toolchain       Tool chain name
    #   @param  Arch            Target arch
    #   @param  *args           The specific class related parameters
    #   @param  **kwargs        The specific class related dict parameters
    #

    def __new__(cls, Workspace, MetaFile, Target, Toolchain, Arch, *args, **kwargs):
        # check if the object has been created
        Key = (Target, Toolchain, Arch, MetaFile)
        if Key in cls.__ObjectCache:
            # if it exists, just return it directly
            return cls.__ObjectCache[Key]
            # it didnt exist. create it, cache it, then return it
        RetVal = cls.__ObjectCache[Key] = super(AutoGen, cls).__new__(cls)
        return RetVal


    ## hash() operator
    #
    #  The file path of platform file will be used to represent hash value of this object
    #
    #   @retval int     Hash value of the file path of platform file
    #
    def __hash__(self):
        return hash(self.MetaFile)

    ## str() operator
    #
    #  The file path of platform file will be used to represent this object
    #
    #   @retval string  String of platform file path
    #
    def __str__(self):
        return str(self.MetaFile)

    ## "==" operator
    def __eq__(self, Other):
        return Other and self.MetaFile == Other

    @classmethod
    def Cache(cls):
        return cls.__ObjectCache

#
# The priority list while override build option
#
PrioList = {"0x11111"  : 16,     #  TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE (Highest)
            "0x01111"  : 15,     #  ******_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
            "0x10111"  : 14,     #  TARGET_*********_ARCH_COMMANDTYPE_ATTRIBUTE
            "0x00111"  : 13,     #  ******_*********_ARCH_COMMANDTYPE_ATTRIBUTE
            "0x11011"  : 12,     #  TARGET_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
            "0x01011"  : 11,     #  ******_TOOLCHAIN_****_COMMANDTYPE_ATTRIBUTE
            "0x10011"  : 10,     #  TARGET_*********_****_COMMANDTYPE_ATTRIBUTE
            "0x00011"  : 9,      #  ******_*********_****_COMMANDTYPE_ATTRIBUTE
            "0x11101"  : 8,      #  TARGET_TOOLCHAIN_ARCH_***********_ATTRIBUTE
            "0x01101"  : 7,      #  ******_TOOLCHAIN_ARCH_***********_ATTRIBUTE
            "0x10101"  : 6,      #  TARGET_*********_ARCH_***********_ATTRIBUTE
            "0x00101"  : 5,      #  ******_*********_ARCH_***********_ATTRIBUTE
            "0x11001"  : 4,      #  TARGET_TOOLCHAIN_****_***********_ATTRIBUTE
            "0x01001"  : 3,      #  ******_TOOLCHAIN_****_***********_ATTRIBUTE
            "0x10001"  : 2,      #  TARGET_*********_****_***********_ATTRIBUTE
            "0x00001"  : 1}      #  ******_*********_****_***********_ATTRIBUTE (Lowest)
## Calculate the priority value of the build option
#
# @param    Key    Build option definition contain: TARGET_TOOLCHAIN_ARCH_COMMANDTYPE_ATTRIBUTE
#
# @retval   Value  Priority value based on the priority list.
#
def CalculatePriorityValue(Key):
    Target, ToolChain, Arch, CommandType, Attr = Key.split('_')
    PriorityValue = 0x11111
    if Target == TAB_STAR:
        PriorityValue &= 0x01111
    if ToolChain == TAB_STAR:
        PriorityValue &= 0x10111
    if Arch == TAB_STAR:
        PriorityValue &= 0x11011
    if CommandType == TAB_STAR:
        PriorityValue &= 0x11101
    if Attr == TAB_STAR:
        PriorityValue &= 0x11110

    return PrioList["0x%0.5x" % PriorityValue]
