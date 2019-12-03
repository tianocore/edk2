## @file
# Generate a capsule windows driver.
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
GenerateWindowsDriver
'''

import sys
import argparse
import uuid
import struct
import subprocess
import os
import tempfile
import shutil
import platform
import re
import logging
from WindowsCapsuleSupportHelper import WindowsCapsuleSupportHelper
from Common.Uefi.Capsule.FmpCapsuleHeader  import FmpCapsuleHeaderClass
from Common.Uefi.Capsule.UefiCapsuleHeader import UefiCapsuleHeaderClass

#
# Globals for help information
#
__prog__        = 'GenerateWindowsDriver'
__version__     = '0.0'
__copyright__   = 'Copyright (c) 2019, Intel Corporation. All rights reserved.'
__description__ = 'Generate Capsule Windows Driver.\n'

def GetCapGuid (InputFile):
    with open(InputFile, 'rb') as File:
        Buffer = File.read()
    try:
        Result = UefiCapsuleHeader.Decode (Buffer)
        if len (Result) > 0:
            FmpCapsuleHeader.Decode (Result)
            for index in range (0, FmpCapsuleHeader.PayloadItemCount):
                Guid = FmpCapsuleHeader.GetFmpCapsuleImageHeader (index).UpdateImageTypeId
        return Guid
    except:
        print ('GenerateCapsule: error: can not decode capsule')
        sys.exit (1)

def ArgCheck(args):
    Version = args.CapsuleVersion_DotString.split('.')

    if len(Version) != 4:
        logging.critical("Name invalid: '%s'", args.CapsuleVersion_DotString)
        raise ValueError("Name invalid.")
    for sub in Version:
        if  int(sub, 16) > 65536:
            logging.critical("Name invalid: '%s'", args.CapsuleVersion_DotString)
            raise ValueError("Name exceed limit 65536.")

    if not (re.compile(r'[\a-fA-F0-9]*$')).match(args.CapsuleVersion_DotString):
        logging.critical("Name invalid: '%s'", args.CapsuleVersion_DotString)
        raise ValueError("Name has invalid chars.")

def CapsuleGuidCheck(InputFile, Guid):
    CapGuid = GetCapGuid(InputFile)
    if (str(Guid).lower() != str(CapGuid)):
        print('GenerateWindowsDriver error: Different Guid from Capsule')
        sys.exit(1)

if __name__ == '__main__':
    def convert_arg_line_to_args(arg_line):
        for arg in arg_line.split():
            if not arg.strip():
                continue
            yield arg

    parser = argparse.ArgumentParser (
                        prog = __prog__,
                        description = __description__ + __copyright__,
                        conflict_handler = 'resolve',
                        fromfile_prefix_chars = '@'
                        )
    parser.convert_arg_line_to_args = convert_arg_line_to_args
    parser.add_argument("--output-folder", dest = 'OutputFolder', help = "firmware resource update driver package output folder.")
    parser.add_argument("--product-fmp-guid", dest = 'ProductFmpGuid', help = "firmware GUID of resource update driver package")
    parser.add_argument("--capsuleversion-dotstring", dest = 'CapsuleVersion_DotString', help = "firmware version with date on which update driver package is authored")
    parser.add_argument("--capsuleversion-hexstring", dest = 'CapsuleVersion_HexString', help = "firmware version in Hex of update driver package")
    parser.add_argument("--product-fw-provider", dest = 'ProductFwProvider', help = "vendor/provider of entire firmware resource update driver package")
    parser.add_argument("--product-fw-mfg-name", dest = 'ProductFwMfgName', help = "manufacturer/vendor of firmware resource update driver package")
    parser.add_argument("--product-fw-desc", dest = "ProductFwDesc", help = "description about resource update driver")
    parser.add_argument("--capsule-file-name", dest = 'CapsuleFileName', help ="firmware resource image file")
    parser.add_argument("--pfx-file", dest = 'PfxFile', help = "pfx file path used to sign resource update driver")
    parser.add_argument("--arch", dest = 'Arch', help = "supported architecture:arm/x64/amd64/arm64/aarch64", default = 'amd64')
    parser.add_argument("--operating-system-string", dest = 'OperatingSystemString', help = "supported operating system:win10/10/10_au/10_rs2/10_rs3/10_rs4/server10/server2016/serverrs2/serverrs3/serverrs4", default = "win10")

    args = parser.parse_args()
    InputFile = os.path.join(args.OutputFolder, '') + args.CapsuleFileName
    UefiCapsuleHeader = UefiCapsuleHeaderClass ()
    FmpCapsuleHeader  = FmpCapsuleHeaderClass ()
    CapsuleGuidCheck(InputFile, args.ProductFmpGuid)
    ArgCheck(args)
    ProductName = os.path.splitext(args.CapsuleFileName)[0]
    WindowsDriver = WindowsCapsuleSupportHelper ()

    WindowsDriver.PackageWindowsCapsuleFiles (
                                                   args.OutputFolder,
                                                   ProductName,
                                                   args.ProductFmpGuid,
                                                   args.CapsuleVersion_DotString,
                                                   args.CapsuleVersion_HexString,
                                                   args.ProductFwProvider,
                                                   args.ProductFwMfgName,
                                                   args.ProductFwDesc,
                                                   args.CapsuleFileName,
                                                   args.PfxFile,
                                                   None,
                                                   None,
                                                   args.Arch,
                                                   args.OperatingSystemString
                                                   )
