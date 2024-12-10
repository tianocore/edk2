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
import pathlib
from   ctypes import *

sys.dont_write_bytecode = True

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

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

def BuildUniversalPayload(Args):
    DscPath = os.path.normpath(Args.DscPath)
    print("Building FIT for DSC file %s"%DscPath)
    BuildTarget = Args.Target
    ToolChain = Args.ToolChain
    Quiet     = "--quiet"  if Args.Quiet else ""

    if Args.Fit == True:
        PayloadEntryToolChain = ToolChain
        Args.Macro.append("UNIVERSAL_PAYLOAD_FORMAT=FIT")
        UpldEntryFile = "FitUniversalPayloadEntry"
    else:
        PayloadEntryToolChain = 'CLANGDWARF'
        Args.Macro.append("UNIVERSAL_PAYLOAD_FORMAT=ELF")
        UpldEntryFile = "UniversalPayloadEntry"

    BuildDir     = os.path.join(os.environ['WORKSPACE'], os.path.normpath("Build/UefiPayloadPkg{}").format (Args.Arch))
    if Args.Arch == 'X64':
        BuildArch      = "X64"
        FitArch        = "x86_64"
    elif Args.Arch == 'IA32':
        BuildArch      = "IA32 -a X64"
        FitArch        = "x86"
    elif Args.Arch == 'RISCV64':
        BuildArch      = "RISCV64"
        FitArch        = "RISCV64"
    elif Args.Arch == 'AARCH64':
        BuildArch      = "AARCH64"
        FitArch        = "AARCH64"
    else:
        print("Incorrect arch option provided")

    EntryOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, PayloadEntryToolChain), os.path.normpath("{}/UefiPayloadPkg/UefiPayloadEntry/{}/DEBUG/{}.dll".format (Args.Arch, UpldEntryFile, UpldEntryFile)))
    EntryModuleInf = os.path.normpath("UefiPayloadPkg/UefiPayloadEntry/{}.inf".format (UpldEntryFile))
    DxeFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/DXEFV.Fv"))
    BdsFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/BDSFV.Fv"))
    SecFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/SECFV.Fv"))
    NetworkFvOutputDir = os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/NETWORKFV.Fv"))
    PayloadReportPath = os.path.join(BuildDir, "UefiUniversalPayload.txt")
    ModuleReportPath = os.path.join(BuildDir, "UefiUniversalPayloadEntry.txt")
    UpldInfoFile = os.path.join(BuildDir, "UniversalPayloadInfo.bin")

    Pcds = ""
    if (Args.pcd != None):
        for PcdItem in Args.pcd:
            Pcds += " --pcd {}".format (PcdItem)

    Defines = ""
    Defines += " -D BUILD_ARCH={}".format(Args.Arch)
    if (Args.Macro != None):
        for MacroItem in Args.Macro:
            Defines += " -D {}".format (MacroItem)

    if (Args.add_cc_flags!= None):
        CcFlags = " ".join(Args.add_cc_flags)

        # Wrap the CC flags with double quotes since we might have plenty of
        # specified build options
        FinalFlags = f'"{CcFlags}"'
        Defines += " -D APPEND_CC_FLAGS={}".format (FinalFlags)

    #
    # Building DXE core and DXE drivers as DXEFV.
    # In edk2 CI build this step will be done by CI common build step.
    #
    if Args.BuildEntryOnly == False and Args.CiBuild == False:
        BuildPayload = "build -p {} -b {} -a {} -t {} -y {} {}".format (DscPath, BuildTarget, BuildArch, ToolChain, PayloadReportPath, Quiet)
        BuildPayload += Pcds
        BuildPayload += Defines
        RunCommand(BuildPayload)
    #
    # Building Universal Payload entry.
    #
    if Args.PreBuildUplBinary is None:
        BuildModule = "build -p {} -b {} -a {} -m {} -t {} -y {} {}".format (DscPath, BuildTarget, BuildArch, EntryModuleInf, PayloadEntryToolChain, ModuleReportPath, Quiet)
        BuildModule += Pcds
        BuildModule += Defines
        RunCommand(BuildModule)

    if Args.PreBuildUplBinary is not None:
        if Args.Fit == False:
            EntryOutputDir = os.path.join(BuildDir, "UniversalPayload.elf")
        else:
            EntryOutputDir = os.path.join(BuildDir, "UniversalPayload.fit")
        shutil.copy (os.path.abspath(Args.PreBuildUplBinary), EntryOutputDir)

    #
    # Build Universal Payload Information Section ".upld_info"
    #
    if Args.Fit == False:
        upld_info_hdr = UPLD_INFO_HEADER()
        upld_info_hdr.SpecRevision = Args.SpecRevision
        upld_info_hdr.Revision = Args.Revision
        upld_info_hdr.ProducerId = Args.ProducerId.encode()[:16]
        upld_info_hdr.ImageId = Args.ImageId.encode()[:16]
        upld_info_hdr.Attribute |= 1 if BuildTarget == "DEBUG" else 0
        fp = open(UpldInfoFile, 'wb')
        fp.write(bytearray(upld_info_hdr))
        fp.close()

        if Args.BuildEntryOnly == False:
            import Tools.ElfFv as ElfFv
            ElfFv.ReplaceFv (EntryOutputDir, UpldInfoFile, '.upld_info', Alignment = 4)
    if Args.PreBuildUplBinary is None:
        if Args.Fit == False:
            shutil.copy (EntryOutputDir, os.path.join(BuildDir, 'UniversalPayload.elf'))
        else:
            shutil.copy (EntryOutputDir, os.path.join(BuildDir, 'UniversalPayload.fit'))

    MultiFvList = []
    if Args.BuildEntryOnly == False:
        MultiFvList = [
            ['uefi_fv',        os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/DXEFV.Fv"))    ],
            ['bds_fv',         os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/BDSFV.Fv"))    ],
            ['sec_fv',         os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/SECFV.Fv"))    ],
            ['network_fv',     os.path.join(BuildDir, "{}_{}".format (BuildTarget, ToolChain), os.path.normpath("FV/NETWORKFV.Fv"))],
        ]


    if Args.Fit == True:
        import Tools.MkFitImage as MkFitImage
        import pefile
        fit_image_info_header               = MkFitImage.FIT_IMAGE_INFO_HEADER()
        fit_image_info_header.Description   = 'Uefi Universal Payload'
        fit_image_info_header.UplVersion    = Args.SpecRevision
        fit_image_info_header.Type          = 'flat-binary'
        fit_image_info_header.Arch          = FitArch
        fit_image_info_header.Compression   = 'none'
        fit_image_info_header.Revision      = Args.Revision
        fit_image_info_header.BuildType     = Args.Target.lower()
        fit_image_info_header.Capabilities  = None
        fit_image_info_header.Producer      = Args.ProducerId.lower()
        fit_image_info_header.ImageId       = Args.ImageId.lower()
        fit_image_info_header.Binary        = os.path.join(BuildDir, 'UniversalPayload.fit')
        fit_image_info_header.TargetPath    = os.path.join(BuildDir, 'UniversalPayload.fit')
        fit_image_info_header.UefifvPath    = DxeFvOutputDir
        fit_image_info_header.BdsfvPath     = BdsFvOutputDir
        fit_image_info_header.SecfvPath     = SecFvOutputDir
        fit_image_info_header.NetworkfvPath = NetworkFvOutputDir
        fit_image_info_header.DataOffset    = 0x1000
        fit_image_info_header.LoadAddr      = Args.LoadAddress + fit_image_info_header.DataOffset
        fit_image_info_header.Project       = 'tianocore'

        TargetRebaseFile = fit_image_info_header.Binary.replace (pathlib.Path(fit_image_info_header.Binary).suffix, ".pecoff")
        TargetRebaseEntryFile = fit_image_info_header.Binary.replace (pathlib.Path(fit_image_info_header.Binary).suffix, ".entry")


        #
        # Rebase PECOFF to load address
        #
        RunCommand (
            "GenFw -e SEC -o {} {}".format (
              TargetRebaseFile,
              fit_image_info_header.Binary
            ))
        RunCommand (
            "GenFw --rebase 0x{:02X} -o {} {} ".format (
              fit_image_info_header.LoadAddr,
              TargetRebaseFile,
              TargetRebaseFile,
            ))

        #
        # Open PECOFF relocation table binary.
        #
        RelocBinary     = b''
        PeCoff = pefile.PE (TargetRebaseFile)
        if hasattr(PeCoff, 'DIRECTORY_ENTRY_BASERELOC'):
            for reloc in PeCoff.DIRECTORY_ENTRY_BASERELOC:
                for entry in reloc.entries:
                    if (entry.type == 0):
                        continue
                    Type = entry.type
                    Offset = entry.rva + fit_image_info_header.DataOffset
                    RelocBinary += Offset.to_bytes (8, 'little') + Type.to_bytes (8, 'little')
        RelocBinary += b'\x00' * (0x1000 - (len(RelocBinary) % 0x1000))

        #
        # Output UniversalPayload.entry
        #
        TempBinary = open (TargetRebaseFile, 'rb')
        TianoBinary = TempBinary.read ()
        TempBinary.close ()

        TianoEntryBinary = TianoBinary + RelocBinary
        TianoEntryBinary += (b'\x00' * (0x1000 - (len(TianoBinary) % 0x1000)))
        TianoEntryBinarySize = len (TianoEntryBinary)

        TempBinary = open(TargetRebaseEntryFile, "wb")
        TempBinary.truncate()
        TempBinary.write(TianoEntryBinary)
        TempBinary.close()

        #
        # Calculate entry and update relocation table start address and data-size.
        #
        fit_image_info_header.Entry      = PeCoff.OPTIONAL_HEADER.ImageBase + PeCoff.OPTIONAL_HEADER.AddressOfEntryPoint
        fit_image_info_header.RelocStart = fit_image_info_header.DataOffset + len(TianoBinary)
        fit_image_info_header.DataSize   = TianoEntryBinarySize
        fit_image_info_header.Binary     = TargetRebaseEntryFile

        if MkFitImage.MakeFitImage(fit_image_info_header, Args.Arch) is True:
            print('\nSuccessfully build Fit Image')
        else:
            sys.exit(1)
        return MultiFvList, os.path.join(BuildDir, 'UniversalPayload.fit')
    else:
        return MultiFvList, os.path.join(BuildDir, 'UniversalPayload.elf')

def InitArgumentParser(LoadDefault):
    parser = argparse.ArgumentParser(description='For building Universal Payload')
    parser.add_argument('-t', '--ToolChain')
    parser.add_argument('-b', '--Target', default='DEBUG')
    parser.add_argument('-a', '--Arch', choices=['IA32', 'X64', 'RISCV64', 'AARCH64'], help='Specify the ARCH for payload entry module. Default build X64 image.', default ='X64')
    parser.add_argument("-D", "--Macro", action="append", default=["UNIVERSAL_PAYLOAD=TRUE"])
    parser.add_argument('-i', '--ImageId', type=str, help='Specify payload ID (16 bytes maximal).', default ='UEFI')
    parser.add_argument('-q', '--Quiet', action='store_true', help='Disable all build messages except FATAL ERRORS.')
    parser.add_argument("-p", "--pcd", action="append")
    parser.add_argument("-s", "--SpecRevision", type=ValidateSpecRevision, default ='0.7', help='Indicates compliance with a revision of this specification in the BCD format.')
    parser.add_argument("-r", "--Revision", type=Validate32BitInteger, default ='0x0000010105', help='Revision of the Payload binary. Major.Minor.Revision.Build')
    parser.add_argument("-o", "--ProducerId", default ='INTEL', help='A null-terminated OEM-supplied string that identifies the payload producer (16 bytes maximal).')
    parser.add_argument("-e", "--BuildEntryOnly", action='store_true', help='Build UniversalPayload Entry file')
    parser.add_argument("-pb", "--PreBuildUplBinary", default=None, help='Specify the UniversalPayload file')
    parser.add_argument("-sk", "--SkipBuild", action='store_true', help='Skip UniversalPayload build')
    parser.add_argument("-af", "--AddFv", type=ValidateAddFv, action='append', help='Add or replace specific FV into payload, Ex: uefi_fv=XXX.fv')
    parser.add_argument("-f", "--Fit", action='store_true', help='Build UniversalPayload file as UniversalPayload.fit', default=False)
    parser.add_argument('-l', "--LoadAddress", type=int, help='Specify payload load address', default =0x000800000)
    parser.add_argument('-c', '--DscPath', type=str, default="UefiPayloadPkg/UefiPayloadPkg.dsc", help='Path to the DSC file')
    parser.add_argument('-ac', '--add_cc_flags', action='append', help='Add specified CC compile flags')
    parser.add_argument('-ci','--CiBuild', action='store_true', help='Call from edk2 CI Build Process or not', default=False)
    if LoadDefault:
        args, _ = parser.parse_known_args()
    else:
        args = parser.parse_args()
    return args

def UniversalPayloadFullBuild(args):
    MultiFvList = []
    UniversalPayloadBinary = args.PreBuildUplBinary
    if (args.SkipBuild == False):
        MultiFvList, UniversalPayloadBinary = BuildUniversalPayload(args)

    if (args.AddFv != None):
        for (SectionName, SectionFvFile) in args.AddFv:
            MultiFvList.append ([SectionName, SectionFvFile])

    def ReplaceFv (UplBinary, SectionFvFile, SectionName, Arch):
        print (bcolors.OKGREEN + "Patch {}={} into {}".format (SectionName, SectionFvFile, UplBinary) + bcolors.ENDC)
        if (args.Fit == False):
            import Tools.ElfFv as ElfFv
            return ElfFv.ReplaceFv (UplBinary, SectionFvFile, '.upld.{}'.format (SectionName))
        else:
            import Tools.MkFitImage as MkFitImage
            return MkFitImage.ReplaceFv (UplBinary, SectionFvFile, SectionName, Arch)

    if (UniversalPayloadBinary != None):
        for (SectionName, SectionFvFile) in MultiFvList:
            if os.path.exists (SectionFvFile) == False:
                continue
            if (args.Fit == False):
                status = ReplaceFv (UniversalPayloadBinary, SectionFvFile, SectionName, args.Arch)
            else:
                status = ReplaceFv (UniversalPayloadBinary, SectionFvFile, SectionName.replace ("_", "-"), args.Arch)
            if status != 0:
                print (bcolors.FAIL + "[Fail] Patch {}={}".format (SectionName, SectionFvFile) + bcolors.ENDC)
                return status

    print ("\nSuccessfully build Universal Payload")
    return 0

def main():

    args = InitArgumentParser(False)
    return UniversalPayloadFullBuild(args)

if __name__ == '__main__':
    main()
