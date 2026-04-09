## @file
# Generate capsules for RiscVVirt platform
#   openssl, gcab must be install and in path
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2025, Ventana Micro Systems Inc. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
GenCapsuleAll
'''

import os
import sys
import argparse
import subprocess
import glob
import shutil
import struct
import datetime

#
# Globals for help information
#
__prog__        = 'GenCapsuleAll'
__copyright__   = 'Copyright (c) 2019, Intel Corporation. All rights reserved.'
__description__ = 'Generate RiscVVirt capsules.\n'

#
# Globals
#
gWorkspace = ''
gBaseToolsPath = ''
gArgs      = None

def LogAlways(Message):
    sys.stdout.write (__prog__ + ': ' + Message + '\n')
    sys.stdout.flush()

def Log(Message):
    global gArgs
    if not gArgs.Verbose:
        return
    sys.stdout.write (__prog__ + ': ' + Message + '\n')
    sys.stdout.flush()

def Error(Message, ExitValue=1):
    sys.stderr.write (__prog__ + ': ERROR: ' + Message + '\n')
    sys.exit (ExitValue)

def RelativePath(target):
    global gWorkspace
    Log('RelativePath' + target)
    return os.path.relpath (target, gWorkspace)

def NormalizePath(target):
    if isinstance(target, tuple):
        return os.path.normpath (os.path.join (*target))
    else:
        return os.path.normpath (target)

def RemoveFile(target):
    target = NormalizePath(target)
    if not target or target == os.pathsep:
        Error ('RemoveFile() invalid target')
    if os.path.exists(target):
        os.remove (target)
        Log ('remove %s' % (RelativePath (target)))

def RemoveDirectory(target):
    target = NormalizePath(target)
    if not target or target == os.pathsep:
        Error ('RemoveDirectory() invalid target')
    if os.path.exists(target):
        Log ('rmdir %s' % (RelativePath (target)))
        shutil.rmtree(target)

def CreateDirectory(target):
    target = NormalizePath(target)
    if not os.path.exists(target):
        Log ('mkdir %s' % (RelativePath (target)))
        os.makedirs (target)

def Copy(src, dst):
    src = NormalizePath(src)
    dst = NormalizePath(dst)
    for File in glob.glob(src):
        Log ('copy %s -> %s' % (RelativePath (File), RelativePath (dst)))
        shutil.copy (File, dst)

GenerateCapsuleCommand = '''
GenerateCapsule
--encode
--guid {FMP_CAPSULE_GUID}
--fw-version {FMP_CAPSULE_VERSION}
--lsv {FMP_CAPSULE_LSV}
--signer-private-cert={BASE_TOOLS_PATH}/Source/Python/Pkcs7Sign/TestCert.pem
--other-public-cert={BASE_TOOLS_PATH}/Source/Python/Pkcs7Sign/TestSub.pub.pem
--trusted-public-cert={BASE_TOOLS_PATH}/Source/Python/Pkcs7Sign/TestRoot.pub.pem
-o {FMP_CAPSULE_FILE}
{FMP_CAPSULE_PAYLOAD}
'''
MetaInfoXmlTemplate = '''
<?xml version="1.0" encoding="UTF-8"?>
<component type="firmware">
  <id>com.intel.FMP_CAPSULE_BASE_NAME.firmware</id>
  <name>FMP_CAPSULE_BASE_NAME</name>
  <summary>System firmware for the FMP_CAPSULE_BASE_NAME</summary>
  <description>
    Description of System firmware for the FMP_CAPSULE_BASE_NAME
  </description>
  <provides>
    <firmware type="flashed">FMP_CAPSULE_GUID</firmware>
  </provides>
  <url type="homepage">http://www.tianocore.org</url>
  <metadata_license>CC0-1.0</metadata_license>
  <project_license>BSD</project_license>
  <developer_name>Tianocore</developer_name>
  <releases>
    <release version="FMP_CAPSULE_VERSION_DECIMAL" date="FMP_CAPSULE_DATE">
      <description>
        Build FMP_CAPSULE_STRING
      </description>
    </release>
  </releases>
  <!-- most OEMs do not need to do this... -->
  <custom>
    <value key="LVFS::InhibitDownload"/>
  </custom>
</component>
'''

def GenCapsuleDevice (BaseName, PayloadFileName, Guid, Version, Lsv, CapsulesPath, CapsulesSubDir):
    global gBaseToolsPath
    LogAlways ('Generate Capsule: {0} {1:08x} {2:08x} {3}'.format (Guid, Version, Lsv, PayloadFileName))

    VersionString = '.'.join([str(ord(x)) for x in struct.pack('>I', Version).decode()])

    FmpCapsuleFile = NormalizePath ((CapsulesPath, CapsulesSubDir, BaseName + '.' + VersionString + '.cap'))
    Command = GenerateCapsuleCommand.format (
                FMP_CAPSULE_GUID    = Guid,
                FMP_CAPSULE_VERSION = Version,
                FMP_CAPSULE_LSV     = Lsv,
                BASE_TOOLS_PATH     = gBaseToolsPath,
                FMP_CAPSULE_FILE    = FmpCapsuleFile,
                FMP_CAPSULE_PAYLOAD = PayloadFileName
                )
    Command = ' '.join(Command.splitlines()).strip()
    if gArgs.Verbose:
        Command = Command + ' -v'

    Log (Command)

    Process = subprocess.Popen(Command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    ProcessOutput = Process.communicate()

    if Process.returncode == 0:
        Log (ProcessOutput[0].decode())
    else:
        LogAlways (Command)
        LogAlways (ProcessOutput[0].decode())
        Error ('GenerateCapsule returned an error')

    Copy (PayloadFileName, (CapsulesPath, 'firmware.bin'))
    MetaInfoXml = MetaInfoXmlTemplate
    MetaInfoXml = MetaInfoXml.replace ('FMP_CAPSULE_GUID', Guid)
    MetaInfoXml = MetaInfoXml.replace ('FMP_CAPSULE_BASE_NAME', BaseName)
    MetaInfoXml = MetaInfoXml.replace ('FMP_CAPSULE_VERSION_DECIMAL', str(Version))
    MetaInfoXml = MetaInfoXml.replace ('FMP_CAPSULE_STRING', VersionString)
    MetaInfoXml = MetaInfoXml.replace ('FMP_CAPSULE_DATE', str(datetime.date.today()))
    f = open (NormalizePath ((CapsulesPath, 'firmware.metainfo.xml')), 'w')
    f.write(MetaInfoXml)
    f.close()

    Command = 'gcab --create firmware.cab firmware.bin firmware.metainfo.xml'
    Log (Command)

    Process = subprocess.Popen(Command, cwd=CapsulesPath, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    ProcessOutput = Process.communicate()

    if Process.returncode == 0:
        Log (ProcessOutput[0].decode())
    else:
        LogAlways (Command)
        LogAlways (ProcessOutput[0].decode())
        Error ('GenerateCapsule returned an error')

    FmpCabinetFile = NormalizePath ((CapsulesPath, CapsulesSubDir, BaseName + '.' + VersionString + '.cab'))

    Copy ((CapsulesPath, 'firmware.cab'), FmpCabinetFile)

    RemoveFile ((CapsulesPath, 'firmware.cab'))
    RemoveFile ((CapsulesPath, 'firmware.metainfo.xml'))
    RemoveFile ((CapsulesPath, 'firmware.bin'))

if __name__ == '__main__':
    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser (
                        prog = __prog__,
                        description = __description__ + __copyright__,
                        conflict_handler = 'resolve'
                        )
    parser.add_argument (
             '-a', '--arch', dest = 'Arch', nargs = '+', action = 'append',
             required = True,
             help = '''ARCHS is one of list: IA32, X64, IPF, AARCH64, RISCV64
                       or EBC, which overrides target.txt's TARGET_ARCH definition.
                       To specify more archs, please repeat this option.'''
             )
    parser.add_argument (
             '-t', '--tagname', dest = 'ToolChain', required = True,
             help = '''Using the Tool Chain Tagname to build the platform,
                       overriding target.txt's TOOL_CHAIN_TAG definition.'''
             )
    parser.add_argument (
             '-p', '--platform', dest = 'PlatformFile', required = True,
             help = '''Build the platform specified by the DSC file name argument,
                       overriding target.txt's ACTIVE_PLATFORM definition.'''
             )
    parser.add_argument (
             '-b', '--buildtarget', dest = 'BuildTarget', required = True,
             help = '''Using the TARGET to build the platform, overriding
                       target.txt's TARGET definition.'''
             )
    parser.add_argument (
             '--conf=', dest = 'ConfDirectory', required = True,
             help = '''Specify the customized Conf directory.'''
             )
    parser.add_argument (
             '-D', '--define', dest = 'Define', nargs='*', action = 'append',
             help = '''Macro: "Name [= Value]".'''
             )
    parser.add_argument (
             '-v', '--verbose', dest = 'Verbose', action = 'store_true',
             help = '''Turn on verbose output with informational messages printed'''
             )
    parser.add_argument (
             '--package', dest = 'Package', nargs = '*', action = 'append',
             help = '''The directory name of a package of tests to copy'''
             )

    #
    # Parse command line arguments
    #
    gArgs, remaining = parser.parse_known_args()
    gArgs.BuildType = 'all'
    for BuildType in ['all', 'fds', 'genc', 'genmake', 'clean', 'cleanall', 'modules', 'libraries', 'run']:
        if BuildType in remaining:
            gArgs.BuildType = BuildType
            remaining.remove(BuildType)
            break
    gArgs.Remaining = ' '.join(remaining)

    #
    # Get WORKSPACE environment variable
    #
    try:
        gWorkspace = os.environ['WORKSPACE']
    except:
        Error ('WORKSPACE environment variable not set')

    #
    # Get PACKAGES_PATH and generate prioritized list of paths
    #
    PathList = [gWorkspace]
    try:
        PathList += os.environ['PACKAGES_PATH'].split(os.pathsep)
    except:
        pass

    #
    # Determine full path to BaseTools
    #
    for Path in PathList:
        if os.path.exists (os.path.join (Path, 'BaseTools')):
            gBaseToolsPath = os.path.join (Path, 'BaseTools')
            break

    #
    # Parse PLATFORM_NAME from DSC file
    #
    for Path in PathList:
        if os.path.exists (os.path.join (Path, gArgs.PlatformFile)):
            Dsc = open (os.path.join (Path, gArgs.PlatformFile), 'r').readlines()
            break
    for Line in Dsc:
        if Line.strip().startswith('PLATFORM_NAME'):
            OutputDirectory = os.path.join ('Build', Line.strip().split('=')[1].strip())
            break

    #
    # Determine full paths to EDK II build directory, EDK II build output
    # directory and the CPU arch of the UEFI phase.
    #
    CommandDir = os.path.dirname(sys.argv[0])
    EdkiiBuildDir = os.path.join (gWorkspace, OutputDirectory)
    EdkiiBuildOutput = os.path.join (EdkiiBuildDir, gArgs.BuildTarget + '_' + gArgs.ToolChain)
    UefiArch = gArgs.Arch[0][0]
    if len (gArgs.Arch) > 1:
        if ['RISCV64'] in gArgs.Arch:
            UefiArch = 'RISCV64'

    CapsulesPath = NormalizePath((EdkiiBuildDir, 'Capsules'))

    CapsulesSubDir = 'TestCert' + '_' + UefiArch + '_' + gArgs.BuildTarget + '_' + gArgs.ToolChain

    #
    # Create output directories
    #
    try:
        CreateDirectory ((CapsulesPath))
    except:
        pass
    try:
        CreateDirectory ((CapsulesPath, CapsulesSubDir))
    except:
        pass

    #
    #  Copy CapsuleApp
    #
    Copy ((EdkiiBuildOutput, UefiArch, 'CapsuleApp.efi'), (CapsulesPath, CapsulesSubDir))

    #
    # Generate capsules for RiscVVirt Firmware Updates
    #
    CodeFW = os.path.join (EdkiiBuildOutput, 'FV', 'RISCV_VIRT_CODE.fd')
    GenCapsuleDevice('RISCVVIRT', CodeFW,'bcbacac2-1d1d-4c14-89a3-5e27496b702d',0x00010000,0x00000000, CapsulesPath, CapsulesSubDir)
