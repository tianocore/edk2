## @file
# This file contains the script to build UniversalPayload
#
# Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
##

import argparse
import subprocess
import os
import shutil
import sys
from   ctypes import *
from Tools.ElfFv import ReplaceFv
sys.dont_write_bytecode = True
import yaml

class UPLD_INFO_HEADER(LittleEndianStructure):
    _pack_ = 1
    _fields_ = [
        ('Identifier',           ARRAY(c_char, 4)),
        ('HeaderLength',         c_uint32),
        ('SpecRevision',         c_uint16),
        ('Reserved',             c_uint16),
        ('Revision',             c_uint32),
        ('Attribute',            c_uint32),
        ('Capability',           c_uint32),
        ('ProducerId',           ARRAY(c_char, 16)),
        ('ImageId',              ARRAY(c_char, 16)),
        ]

    def __init__(self):
        self.Identifier     =  b'PLDH'
        self.HeaderLength   = sizeof(UPLD_INFO_HEADER)
        self.SpecRevision   = 0x0070
        self.Revision       = 0x0000010105
        self.ImageId        = b'UEFI'
        self.ProducerId     = b'INTEL'

def BuildUniversalPayload(Args):
    def RunCommand(cmd):
        print(cmd)
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,cwd=os.environ['WORKSPACE'])
        while True:
            line = p.stdout.readline()
            if not line:
                break
            print(line.strip().decode(errors='ignore'))

        p.communicate()
        if p.returncode != 0:
            print("- Failed - error happened when run command: %s"%cmd)
            raise Exception("ERROR: when run command: %s"%cmd)

    BuildTarget = Args.Target
    ToolChain = Args.ToolChain
    Quiet     = "--quiet"  if Args.Quiet else ""
    ElfToolChain = 'CLANGDWARF'
    BuildDir     = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/UefiPayloadPkgX64"))
    BuildModule = ""
    BuildArch = ""
    if Args.Arch == 'X64':
        BuildArch      = "X64"
        EntryOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ElfToolChain), os.path.normpath("X64/UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry/DEBUG/UniversalPayloadEntry.dll"))
    else:
        BuildArch      = "IA32 -a X64"
        EntryOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ElfToolChain), os.path.normpath("IA32/UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry/DEBUG/UniversalPayloadEntry.dll"))

    if Args.PreBuildUplBinary is not None:
        EntryOutputDir = os.path.abspath(Args.PreBuildUplBinary)
    DscPath = os.path.normpath("UefiPayloadPkg/UefiPayloadPkg.dsc")
    ModuleReportPath = os.path.join(BuildDir, "UefiUniversalPayloadEntry.txt")
    UpldInfoFile = os.path.join(BuildDir, "UniversalPayloadInfo.bin")

    PcdCustoms= dict()
    if (Args.pcd != None):
        for PcdItem in Args.pcd:
            SplitValue = str(PcdItem).split("=")
            PcdCustoms[SplitValue[0]] = SplitValue[1]

    DefineCustoms= dict()
    if (Args.Macro != None):
        for MacroItem in Args.Macro:
            SplitValue = str(MacroItem).split("=")
            DefineCustoms[SplitValue[0]] = SplitValue[1]

    Pcds = ""
    if Args.Settings['PcdSetting'] != "":
        PcdCustoms.update(Args.Settings['PcdSetting'])
    for Item in PcdCustoms:
        Pcds += " -p {0}={1}".format (Item, PcdCustoms[Item])

    Defines = ""
    if Args.Settings['Macro'] != "":
        DefineCustoms.update(Args.Settings['Macro'])
    for Item in DefineCustoms:
        Defines += " -D {0}={1}".format (Item, DefineCustoms[Item])

    #
    # Building DXE core and DXE drivers as DXEFV.
    #
    if Args.BuildEntryOnly == False:
        PayloadReportPath = os.path.join(BuildDir, "UefiUniversalPayload.txt")
        BuildPayload = "build -p {} -b {} -a X64 -t {} -y {} {}".format (DscPath, BuildTarget, ToolChain, PayloadReportPath, Quiet)
        BuildPayload += Pcds
        BuildPayload += Defines
        RunCommand(BuildPayload)
    #
    # Building Universal Payload entry.
    #
    if Args.PreBuildUplBinary is None:
        EntryModuleInf = os.path.normpath("UefiPayloadPkg/UefiPayloadEntry/UniversalPayloadEntry.inf")
        BuildModule = "build -p {} -b {} -a {} -m {} -t {} -y {} {}".format (DscPath, BuildTarget, BuildArch, EntryModuleInf, ElfToolChain, ModuleReportPath, Quiet)
        BuildModule += Pcds
        BuildModule += Defines
        RunCommand(BuildModule)
    #
    # Buid Universal Payload Information Section ".upld_info"
    #
    upld_info_hdr              = UPLD_INFO_HEADER()

    upld_info_hdr.SpecRevision = Args.SpecRevision if Args.SpecRevision is not None else Args.Settings['SpecRevision']
    upld_info_hdr.Revision     = Args.Revision if Args.Revision is not None else Args.Settings['Revision']
    upld_info_hdr.ProducerId   = Args.ProducerId.encode()[:16] if Args.ProducerId is not None else str(Args.Settings['ProducerId']).encode()[:16]
    upld_info_hdr.ImageId      = Args.ImageId.encode()[:16] if Args.ImageId is not None else str(Args.Settings['ImageId']).encode()[:16]
    upld_info_hdr.Attribute   |= 1 if BuildTarget == "DEBUG" else 0
    fp = open(UpldInfoFile, 'wb')
    fp.write(bytearray(upld_info_hdr))
    fp.close()

    MultiFvList = []
    if Args.BuildEntryOnly == False:
        MultiFvList = [
            ['uefi_fv',    os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/DXEFV.Fv"))    ],
            ['bds_fv',     os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/BDSFV.Fv"))    ],
            ['network_fv', os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/NETWORKFV.Fv"))    ],
        ]
        AddSectionName = '.upld_info'
        ReplaceFv (EntryOutputDir, UpldInfoFile, AddSectionName, Alignment = 4)

    shutil.copy (EntryOutputDir, os.path.join(BuildDir, 'UniversalPayload.elf'))

    return MultiFvList, os.path.join(BuildDir, 'UniversalPayload.elf')

def main():
    def ValidateSpecRevision (Argument):
        try:
            (MajorStr, MinorStr) = Argument.split('.')
        except:
            raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))
        #
        # Spec Revision Bits 15 : 8 - Major Version. Bits 7 : 0 - Minor Version.
        #
        if len(MinorStr) > 0 and len(MinorStr) < 3:
            try:
                Minor = int(MinorStr, 16) if len(MinorStr) == 2 else (int(MinorStr, 16) << 4)
            except:
                raise argparse.ArgumentTypeError ('{} Minor version of SpecRevision is not a valid integer value.'.format (Argument))
        else:
            raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))

        if len(MajorStr) > 0 and len(MajorStr) < 3:
            try:
                Major = int(MajorStr, 16)
            except:
                raise argparse.ArgumentTypeError ('{} Major version of SpecRevision is not a valid integer value.'.format (Argument))
        else:
            raise argparse.ArgumentTypeError ('{} is not a valid SpecRevision format (Major[8-bits].Minor[8-bits]).'.format (Argument))

        return int('0x{0:02x}{1:02x}'.format(Major, Minor), 0)

    def GetUplVerFile(Argument):
      with open(Argument, 'r') as inFile:
        jsonData = yaml.safe_load(inFile)
      return jsonData

    def Validate32BitInteger (Argument):
        try:
            Value = int (Argument, 0)
        except:
            raise argparse.ArgumentTypeError ('{} is not a valid integer value.'.format (Argument))
        if Value < 0:
            raise argparse.ArgumentTypeError ('{} is a negative value.'.format (Argument))
        if Value > 0xffffffff:
            raise argparse.ArgumentTypeError ('{} is larger than 32-bits.'.format (Argument))
        return Value

    def ValidateAddFv (Argument):
        Value = Argument.split ("=")
        if len (Value) != 2:
            raise argparse.ArgumentTypeError ('{} is incorrect format with "xxx_fv=xxx.fv"'.format (Argument))
        if Value[0][-3:] != "_fv":
            raise argparse.ArgumentTypeError ('{} is incorrect format with "xxx_fv=xxx.fv"'.format (Argument))
        if Value[1][-3:].lower () != ".fv":
            raise argparse.ArgumentTypeError ('{} is incorrect format with "xxx_fv=xxx.fv"'.format (Argument))
        if os.path.exists (Value[1]) == False:
            raise argparse.ArgumentTypeError ('File {} is not found.'.format (Value[1]))
        return Value

    parser = argparse.ArgumentParser(description='For building Universal Payload')
    parser.add_argument('-t', '--ToolChain')
    parser.add_argument('-b', '--Target', default='DEBUG')
    parser.add_argument('-a', '--Arch', choices=['IA32', 'X64'], help='Specify the ARCH for payload entry module. Default build X64 image.', default ='X64')
    parser.add_argument("-D", "--Macro", action="append", default=["UNIVERSAL_PAYLOAD=TRUE"])
    parser.add_argument('-i', '--ImageId', type=str, help='Specify payload ID (16 bytes maximal).', default =None)
    parser.add_argument('-q', '--Quiet', action='store_true', help='Disable all build messages except FATAL ERRORS.')
    parser.add_argument("-p", "--pcd", action="append")
    group1 = parser.add_mutually_exclusive_group()
    arg_a = group1.add_argument('-st',"--Settings",type=GetUplVerFile, default=os.path.abspath('UefiPayloadPkg//Settings//UplSettings_NULL.yaml'), help='Get the UplSettings.yaml')
    group1.add_argument('-s', "--SpecRevision", type=ValidateSpecRevision, default =None, help='Indicates compliance with a revision of this specification in the BCD format.')
    group2 = parser.add_mutually_exclusive_group()
    group2.add_argument('-r', "--Revision", type=Validate32BitInteger, default =None, help='Revision of the Payload binary. Major.Minor.Revision.Build')
    group2._group_actions.append(arg_a)

    parser.add_argument("-o", "--ProducerId", default =None, help='A null-terminated OEM-supplied string that identifies the payload producer (16 bytes maximal).')
    parser.add_argument("-e", "--BuildEntryOnly", action='store_true', help='Build UniversalPayload Entry file')
    parser.add_argument("-pb", "--PreBuildUplBinary", default=None, help='Specify the UniversalPayload file')
    parser.add_argument("-sk", "--SkipBuild", action='store_true', help='Skip UniversalPayload build')
    parser.add_argument("-af", "--AddFv", type=ValidateAddFv, action='append', help='Add or replace specific FV into payload, Ex: uefi_fv=XXX.fv')
    args = parser.parse_args()

    MultiFvList = []
    UniversalPayloadBinary = args.PreBuildUplBinary
    if (args.SkipBuild == False):
        MultiFvList, UniversalPayloadBinary = BuildUniversalPayload(args)

    if (args.AddFv != None):
        for (SectionName, SectionFvFile) in args.AddFv:
            MultiFvList.append ([SectionName, SectionFvFile])

    if (UniversalPayloadBinary != None):
        for (SectionName, SectionFvFile) in MultiFvList:
            if os.path.exists (SectionFvFile) == False:
                continue
            print ("Patch {}={} into {}".format (SectionName, SectionFvFile, UniversalPayloadBinary))
            ReplaceFv (UniversalPayloadBinary, SectionFvFile, '.upld.{}'.format (SectionName))

    print ("\nSuccessfully build Universal Payload")

if __name__ == '__main__':
    main()
