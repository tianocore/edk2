## @file
# Generate a capsule.
#
# This tool generates a UEFI Capsule around an FMP Capsule. The capsule payload
# be signed using signtool or OpenSSL and if it is signed the signed content
# includes an FMP Payload Header.
#
# This tool is intended to be used to generate UEFI Capsules to update the
# system firmware or device firmware for integrated devices. In order to
# keep the tool as simple as possible, it has the following limitations:
#   * Do not support vendor code bytes in a capsule.
#
# Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
GenerateCapsule
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
import json
from Common.Uefi.Capsule.UefiCapsuleHeader import UefiCapsuleHeaderClass
from Common.Uefi.Capsule.FmpCapsuleHeader  import FmpCapsuleHeaderClass
from Common.Uefi.Capsule.FmpAuthHeader     import FmpAuthHeaderClass
from Common.Uefi.Capsule.CapsuleDependency import CapsuleDependencyClass
from Common.Edk2.Capsule.FmpPayloadHeader  import FmpPayloadHeaderClass

#
# Globals for help information
#
__prog__        = 'GenerateCapsule'
__version__     = '0.9'
__copyright__   = 'Copyright (c) 2018, Intel Corporation. All rights reserved.'
__description__ = 'Generate a capsule.\n'

def SignPayloadSignTool (Payload, ToolPath, PfxFile, Verbose = False):
    #
    # Create a temporary directory
    #
    TempDirectoryName = tempfile.mkdtemp()

    #
    # Generate temp file name for the payload contents
    #
    TempFileName = os.path.join (TempDirectoryName, 'Payload.bin')

    #
    # Create temporary payload file for signing
    #
    try:
        with open (TempFileName, 'wb') as File:
            File.write (Payload)
    except:
        shutil.rmtree (TempDirectoryName)
        raise ValueError ('GenerateCapsule: error: can not write temporary payload file.')

    #
    # Build signtool command
    #
    if ToolPath is None:
        ToolPath = ''
    Command = ''
    Command = Command + '"{Path}" '.format (Path = os.path.join (ToolPath, 'signtool.exe'))
    Command = Command + 'sign /fd sha256 /p7ce DetachedSignedData /p7co 1.2.840.113549.1.7.2 '
    Command = Command + '/p7 {TempDir} '.format (TempDir = TempDirectoryName)
    Command = Command + '/f {PfxFile} '.format (PfxFile = PfxFile)
    Command = Command + TempFileName
    if Verbose:
        print (Command)

    #
    # Sign the input file using the specified private key
    #
    try:
        Process = subprocess.Popen (Command, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)
        Result = Process.communicate('')
    except:
        shutil.rmtree (TempDirectoryName)
        raise ValueError ('GenerateCapsule: error: can not run signtool.')

    if Process.returncode != 0:
        shutil.rmtree (TempDirectoryName)
        print (Result[1].decode())
        raise ValueError ('GenerateCapsule: error: signtool failed.')

    #
    # Read the signature from the generated output file
    #
    try:
        with open (TempFileName + '.p7', 'rb') as File:
            Signature = File.read ()
    except:
        shutil.rmtree (TempDirectoryName)
        raise ValueError ('GenerateCapsule: error: can not read signature file.')

    shutil.rmtree (TempDirectoryName)
    return Signature

def VerifyPayloadSignTool (Payload, CertData, ToolPath, PfxFile, Verbose = False):
    print ('signtool verify is not supported.')
    raise ValueError ('GenerateCapsule: error: signtool verify is not supported.')

def SignPayloadOpenSsl (Payload, ToolPath, SignerPrivateCertFile, OtherPublicCertFile, TrustedPublicCertFile, Verbose = False):
    #
    # Build openssl command
    #
    if ToolPath is None:
        ToolPath = ''
    Command = ''
    Command = Command + '"{Path}" '.format (Path = os.path.join (ToolPath, 'openssl'))
    Command = Command + 'smime -sign -binary -outform DER -md sha256 '
    Command = Command + '-signer "{Private}" -certfile "{Public}"'.format (Private = SignerPrivateCertFile, Public = OtherPublicCertFile)
    if Verbose:
        print (Command)

    #
    # Sign the input file using the specified private key and capture signature from STDOUT
    #
    try:
        Process = subprocess.Popen (Command, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)
        Result = Process.communicate(input = Payload)
        Signature = Result[0]
    except:
        raise ValueError ('GenerateCapsule: error: can not run openssl.')

    if Process.returncode != 0:
        print (Result[1].decode())
        raise ValueError ('GenerateCapsule: error: openssl failed.')

    return Signature

def VerifyPayloadOpenSsl (Payload, CertData, ToolPath, SignerPrivateCertFile, OtherPublicCertFile, TrustedPublicCertFile, Verbose = False):
    #
    # Create a temporary directory
    #
    TempDirectoryName = tempfile.mkdtemp()

    #
    # Generate temp file name for the payload contents
    #
    TempFileName = os.path.join (TempDirectoryName, 'Payload.bin')

    #
    # Create temporary payload file for verification
    #
    try:
        with open (TempFileName, 'wb') as File:
            File.write (Payload)
    except:
        shutil.rmtree (TempDirectoryName)
        raise ValueError ('GenerateCapsule: error: can not write temporary payload file.')

    #
    # Build openssl command
    #
    if ToolPath is None:
        ToolPath = ''
    Command = ''
    Command = Command + '"{Path}" '.format (Path = os.path.join (ToolPath, 'openssl'))
    Command = Command + 'smime -verify -inform DER '
    Command = Command + '-content {Content} -CAfile "{Public}"'.format (Content = TempFileName, Public = TrustedPublicCertFile)
    if Verbose:
        print (Command)

    #
    # Verify signature
    #
    try:
        Process = subprocess.Popen (Command, stdin = subprocess.PIPE, stdout = subprocess.PIPE, stderr = subprocess.PIPE, shell = True)
        Result = Process.communicate(input = CertData)
    except:
        shutil.rmtree (TempDirectoryName)
        raise ValueError ('GenerateCapsule: error: can not run openssl.')

    if Process.returncode != 0:
        shutil.rmtree (TempDirectoryName)
        print (Result[1].decode())
        raise ValueError ('GenerateCapsule: error: openssl failed.')

    shutil.rmtree (TempDirectoryName)
    return Payload

if __name__ == '__main__':
    def convert_arg_line_to_args(arg_line):
        for arg in arg_line.split():
            if not arg.strip():
                continue
            yield arg

    def ValidateUnsignedInteger (Argument):
        try:
            Value = int (Argument, 0)
        except:
            Message = '{Argument} is not a valid integer value.'.format (Argument = Argument)
            raise argparse.ArgumentTypeError (Message)
        if Value < 0:
            Message = '{Argument} is a negative value.'.format (Argument = Argument)
            raise argparse.ArgumentTypeError (Message)
        return Value

    def ValidateRegistryFormatGuid (Argument):
        try:
            Value = uuid.UUID (Argument)
        except:
            Message = '{Argument} is not a valid registry format GUID value.'.format (Argument = Argument)
            raise argparse.ArgumentTypeError (Message)
        return Value

    def ConvertJsonValue (Config, FieldName, Convert, Required = True, Default = None, Open = False):
        if FieldName not in Config:
            if Required:
                print ('GenerateCapsule: error: Payload descriptor invalid syntax. Could not find {Key} in payload descriptor.'.format(Key = FieldName))
                sys.exit (1)
            return Default
        try:
            Value = Convert (Config[FieldName])
        except:
            print ('GenerateCapsule: error: {Key} in payload descriptor has invalid syntax.'.format (Key = FieldName))
            sys.exit (1)
        if Open:
            try:
                Value = open (Value, "rb")
            except:
                print ('GenerateCapsule: error: can not open file {File}'.format (File = FieldName))
                sys.exit (1)
        return Value

    def DecodeJsonFileParse (Json):
        if 'Payloads' not in Json:
            print ('GenerateCapsule: error "Payloads" section not found in JSON file {File}'.format (File = args.JsonFile.name))
            sys.exit (1)
        for Config in Json['Payloads']:
            #
            # Parse fields from JSON
            #
            PayloadFile                  = ConvertJsonValue (Config, 'Payload', os.path.expandvars, Required = False)
            Guid                         = ConvertJsonValue (Config, 'Guid', ValidateRegistryFormatGuid, Required = False)
            FwVersion                    = ConvertJsonValue (Config, 'FwVersion', ValidateUnsignedInteger, Required = False)
            LowestSupportedVersion       = ConvertJsonValue (Config, 'LowestSupportedVersion', ValidateUnsignedInteger, Required = False)
            HardwareInstance             = ConvertJsonValue (Config, 'HardwareInstance', ValidateUnsignedInteger, Required = False, Default = 0)
            MonotonicCount               = ConvertJsonValue (Config, 'MonotonicCount', ValidateUnsignedInteger, Required = False, Default = 0)
            SignToolPfxFile              = ConvertJsonValue (Config, 'SignToolPfxFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslSignerPrivateCertFile = ConvertJsonValue (Config, 'OpenSslSignerPrivateCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslOtherPublicCertFile   = ConvertJsonValue (Config, 'OpenSslOtherPublicCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslTrustedPublicCertFile = ConvertJsonValue (Config, 'OpenSslTrustedPublicCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            SigningToolPath              = ConvertJsonValue (Config, 'SigningToolPath', os.path.expandvars, Required = False, Default = None)
            UpdateImageIndex             = ConvertJsonValue (Config, 'UpdateImageIndex', ValidateUnsignedInteger, Required = False, Default = 1)

            PayloadDescriptorList.append (PayloadDescriptor (
                                            PayloadFile,
                                            Guid,
                                            FwVersion,
                                            LowestSupportedVersion,
                                            MonotonicCount,
                                            HardwareInstance,
                                            UpdateImageIndex,
                                            SignToolPfxFile,
                                            OpenSslSignerPrivateCertFile,
                                            OpenSslOtherPublicCertFile,
                                            OpenSslTrustedPublicCertFile,
                                            SigningToolPath
                                            ))

    def EncodeJsonFileParse (Json):
        if 'EmbeddedDrivers' not in Json:
            print ('GenerateCapsule: warning "EmbeddedDrivers" section not found in JSON file {File}'.format (File = args.JsonFile.name))
        else:
            for Config in Json['EmbeddedDrivers']:
                EmbeddedDriverFile      = ConvertJsonValue(Config, 'Driver', os.path.expandvars, Open = True)
                #
                #Read EmbeddedDriver file
                #
                try:
                    if args.Verbose:
                        print ('Read EmbeddedDriver file {File}'.format (File = EmbeddedDriverFile.name))
                    Driver = EmbeddedDriverFile.read()
                except:
                    print ('GenerateCapsule: error: can not read EmbeddedDriver file {File}'.format (File = EmbeddedDriverFile.name))
                    sys.exit (1)
                EmbeddedDriverDescriptorList.append (Driver)

        if 'Payloads' not in Json:
            print ('GenerateCapsule: error: "Payloads" section not found in JSON file {File}'.format (File = args.JsonFile.name))
            sys.exit (1)
        for Config in Json['Payloads']:
            #
            # Parse fields from JSON
            #
            PayloadFile                  = ConvertJsonValue (Config, 'Payload', os.path.expandvars, Open = True)
            Guid                         = ConvertJsonValue (Config, 'Guid', ValidateRegistryFormatGuid)
            FwVersion                    = ConvertJsonValue (Config, 'FwVersion', ValidateUnsignedInteger)
            LowestSupportedVersion       = ConvertJsonValue (Config, 'LowestSupportedVersion', ValidateUnsignedInteger)
            HardwareInstance             = ConvertJsonValue (Config, 'HardwareInstance', ValidateUnsignedInteger, Required = False, Default = 0)
            UpdateImageIndex             = ConvertJsonValue (Config, 'UpdateImageIndex', ValidateUnsignedInteger, Required = False, Default = 1)
            MonotonicCount               = ConvertJsonValue (Config, 'MonotonicCount', ValidateUnsignedInteger, Required = False, Default = 0)
            SignToolPfxFile              = ConvertJsonValue (Config, 'SignToolPfxFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslSignerPrivateCertFile = ConvertJsonValue (Config, 'OpenSslSignerPrivateCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslOtherPublicCertFile   = ConvertJsonValue (Config, 'OpenSslOtherPublicCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            OpenSslTrustedPublicCertFile = ConvertJsonValue (Config, 'OpenSslTrustedPublicCertFile', os.path.expandvars, Required = False, Default = None, Open = True)
            SigningToolPath              = ConvertJsonValue (Config, 'SigningToolPath', os.path.expandvars, Required = False, Default = None)
            DepexExp                     = ConvertJsonValue (Config, 'Dependencies', str, Required = False, Default = None)

            #
            # Read binary input file
            #
            try:
                if args.Verbose:
                    print ('Read binary input file {File}'.format (File = PayloadFile.name))
                Payload = PayloadFile.read()
                PayloadFile.close ()
            except:
                print ('GenerateCapsule: error: can not read binary input file {File}'.format (File = PayloadFile.name))
                sys.exit (1)
            PayloadDescriptorList.append (PayloadDescriptor (
                                            Payload,
                                            Guid,
                                            FwVersion,
                                            LowestSupportedVersion,
                                            MonotonicCount,
                                            HardwareInstance,
                                            UpdateImageIndex,
                                            SignToolPfxFile,
                                            OpenSslSignerPrivateCertFile,
                                            OpenSslOtherPublicCertFile,
                                            OpenSslTrustedPublicCertFile,
                                            SigningToolPath,
                                            DepexExp
                                            ))

    def GenerateOutputJson (PayloadJsonDescriptorList):
        PayloadJson = {
                          "Payloads" : [
                              {
                                  "Guid": str(PayloadDescriptor.Guid).upper(),
                                  "FwVersion": str(PayloadDescriptor.FwVersion),
                                  "LowestSupportedVersion": str(PayloadDescriptor.LowestSupportedVersion),
                                  "MonotonicCount": str(PayloadDescriptor.MonotonicCount),
                                  "Payload": PayloadDescriptor.Payload,
                                  "HardwareInstance": str(PayloadDescriptor.HardwareInstance),
                                  "UpdateImageIndex": str(PayloadDescriptor.UpdateImageIndex),
                                  "SignToolPfxFile": str(PayloadDescriptor.SignToolPfxFile),
                                  "OpenSslSignerPrivateCertFile": str(PayloadDescriptor.OpenSslSignerPrivateCertFile),
                                  "OpenSslOtherPublicCertFile": str(PayloadDescriptor.OpenSslOtherPublicCertFile),
                                  "OpenSslTrustedPublicCertFile": str(PayloadDescriptor.OpenSslTrustedPublicCertFile),
                                  "SigningToolPath": str(PayloadDescriptor.SigningToolPath),
                                  "Dependencies" : str(PayloadDescriptor.DepexExp)
                              }for PayloadDescriptor in PayloadJsonDescriptorList
                          ]
                      }
        OutputJsonFile = args.OutputFile.name + '.json'
        if 'Payloads' in PayloadJson:
            PayloadSection = PayloadJson ['Payloads']
        Index = 0
        for PayloadField in PayloadSection:
            if PayloadJsonDescriptorList[Index].SignToolPfxFile is None:
                del PayloadField ['SignToolPfxFile']
            if PayloadJsonDescriptorList[Index].OpenSslSignerPrivateCertFile is None:
                del PayloadField ['OpenSslSignerPrivateCertFile']
            if PayloadJsonDescriptorList[Index].OpenSslOtherPublicCertFile is None:
                del PayloadField ['OpenSslOtherPublicCertFile']
            if PayloadJsonDescriptorList[Index].OpenSslTrustedPublicCertFile is None:
                del PayloadField ['OpenSslTrustedPublicCertFile']
            if PayloadJsonDescriptorList[Index].SigningToolPath is None:
                del PayloadField ['SigningToolPath']
            Index = Index + 1
        Result = json.dumps (PayloadJson, indent=4, sort_keys=True, separators=(',', ': '))
        with open (OutputJsonFile, 'w') as OutputFile:
            OutputFile.write (Result)

    def CheckArgumentConflict (args):
        if args.Encode:
            if args.InputFile:
                print ('GenerateCapsule: error: Argument InputFile conflicts with Argument -j')
                sys.exit (1)
            if args.EmbeddedDriver:
                print ('GenerateCapsule: error: Argument --embedded-driver conflicts with Argument -j')
                sys.exit (1)
        if args.Guid:
            print ('GenerateCapsule: error: Argument --guid conflicts with Argument -j')
            sys.exit (1)
        if args.FwVersion:
            print ('GenerateCapsule: error: Argument --fw-version conflicts with Argument -j')
            sys.exit (1)
        if args.LowestSupportedVersion:
            print ('GenerateCapsule: error: Argument --lsv conflicts with Argument -j')
            sys.exit (1)
        if args.MonotonicCount:
            print ('GenerateCapsule: error: Argument --monotonic-count conflicts with Argument -j')
            sys.exit (1)
        if args.HardwareInstance:
            print ('GenerateCapsule: error: Argument --hardware-instance conflicts with Argument -j')
            sys.exit (1)
        if args.SignToolPfxFile:
            print ('GenerateCapsule: error: Argument --pfx-file conflicts with Argument -j')
            sys.exit (1)
        if args.OpenSslSignerPrivateCertFile:
            print ('GenerateCapsule: error: Argument --signer-private-cert conflicts with Argument -j')
            sys.exit (1)
        if args.OpenSslOtherPublicCertFile:
            print ('GenerateCapsule: error: Argument --other-public-cert conflicts with Argument -j')
            sys.exit (1)
        if args.OpenSslTrustedPublicCertFile:
            print ('GenerateCapsule: error: Argument --trusted-public-cert conflicts with Argument -j')
            sys.exit (1)
        if args.SigningToolPath:
            print ('GenerateCapsule: error: Argument --signing-tool-path conflicts with Argument -j')
            sys.exit (1)

    class PayloadDescriptor (object):
        def __init__(self,
                     Payload,
                     Guid,
                     FwVersion,
                     LowestSupportedVersion,
                     MonotonicCount               = 0,
                     HardwareInstance             = 0,
                     UpdateImageIndex             = 1,
                     SignToolPfxFile              = None,
                     OpenSslSignerPrivateCertFile = None,
                     OpenSslOtherPublicCertFile   = None,
                     OpenSslTrustedPublicCertFile = None,
                     SigningToolPath              = None,
                     DepexExp                     = None
                     ):
            self.Payload                      = Payload
            self.Guid                         = Guid
            self.FwVersion                    = FwVersion
            self.LowestSupportedVersion       = LowestSupportedVersion
            self.MonotonicCount               = MonotonicCount
            self.HardwareInstance             = HardwareInstance
            self.UpdateImageIndex             = UpdateImageIndex
            self.SignToolPfxFile              = SignToolPfxFile
            self.OpenSslSignerPrivateCertFile = OpenSslSignerPrivateCertFile
            self.OpenSslOtherPublicCertFile   = OpenSslOtherPublicCertFile
            self.OpenSslTrustedPublicCertFile = OpenSslTrustedPublicCertFile
            self.SigningToolPath              = SigningToolPath
            self.DepexExp                     = DepexExp

            self.UseSignTool = self.SignToolPfxFile is not None
            self.UseOpenSsl  = (self.OpenSslSignerPrivateCertFile is not None and
                                self.OpenSslOtherPublicCertFile is not None and
                                self.OpenSslTrustedPublicCertFile is not None)
            self.AnyOpenSsl  = (self.OpenSslSignerPrivateCertFile is not None or
                                self.OpenSslOtherPublicCertFile is not None or
                                self.OpenSslTrustedPublicCertFile is not None)
            self.UseDependency = self.DepexExp is not None

        def Validate(self, args):
            if self.UseSignTool and self.AnyOpenSsl:
                raise argparse.ArgumentTypeError ('Providing both signtool and OpenSSL options is not supported')
            if not self.UseSignTool and not self.UseOpenSsl and self.AnyOpenSsl:
                if args.JsonFile:
                    raise argparse.ArgumentTypeError ('the following JSON fields are required for OpenSSL: OpenSslSignerPrivateCertFile, OpenSslOtherPublicCertFile, OpenSslTrustedPublicCertFile')
                else:
                    raise argparse.ArgumentTypeError ('the following options are required for OpenSSL: --signer-private-cert, --other-public-cert, --trusted-public-cert')
            if self.UseSignTool and platform.system() != 'Windows':
                raise argparse.ArgumentTypeError ('Use of signtool is not supported on this operating system.')
            if args.Encode:
                if self.FwVersion is None or self.LowestSupportedVersion is None:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('the following JSON fields are required: FwVersion, LowestSupportedVersion')
                    else:
                        raise argparse.ArgumentTypeError ('the following options are required: --fw-version, --lsv')
                if self.FwVersion > 0xFFFFFFFF:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('JSON field FwVersion must be an integer in range 0x0..0xffffffff')
                    else:
                        raise argparse.ArgumentTypeError ('--fw-version must be an integer in range 0x0..0xffffffff')
                if self.LowestSupportedVersion > 0xFFFFFFFF:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('JSON field LowestSupportedVersion must be an integer in range 0x0..0xffffffff')
                    else:
                        raise argparse.ArgumentTypeError ('--lsv must be an integer in range 0x0..0xffffffff')

            if args.Encode:
                if self.Guid is None:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('the following JSON field is required: Guid')
                    else:
                        raise argparse.ArgumentTypeError ('the following option is required: --guid')
                if self.HardwareInstance > 0xFFFFFFFFFFFFFFFF:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('JSON field HardwareInstance must be an integer in range 0x0..0xffffffffffffffff')
                    else:
                        raise argparse.ArgumentTypeError ('--hardware-instance must be an integer in range 0x0..0xffffffffffffffff')
                if self.MonotonicCount > 0xFFFFFFFFFFFFFFFF:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('JSON field MonotonicCount must be an integer in range 0x0..0xffffffffffffffff')
                    else:
                        raise argparse.ArgumentTypeError ('--monotonic-count must be an integer in range 0x0..0xffffffffffffffff')
                if self.UpdateImageIndex >0xFF:
                    if args.JsonFile:
                        raise argparse.ArgumentTypeError ('JSON field UpdateImageIndex must be an integer in range 0x0..0xff')
                    else:
                        raise argparse.ArgumentTypeError ('--update-image-index must be an integer in range 0x0..0xff')

            if self.UseSignTool:
                self.SignToolPfxFile.close()
                self.SignToolPfxFile = self.SignToolPfxFile.name
            if self.UseOpenSsl:
                self.OpenSslSignerPrivateCertFile.close()
                self.OpenSslOtherPublicCertFile.close()
                self.OpenSslTrustedPublicCertFile.close()
                self.OpenSslSignerPrivateCertFile = self.OpenSslSignerPrivateCertFile.name
                self.OpenSslOtherPublicCertFile   = self.OpenSslOtherPublicCertFile.name
                self.OpenSslTrustedPublicCertFile = self.OpenSslTrustedPublicCertFile.name

            #
            # Perform additional argument verification
            #
            if args.Encode:
                if 'PersistAcrossReset' not in args.CapsuleFlag:
                    if 'InitiateReset' in args.CapsuleFlag:
                        raise argparse.ArgumentTypeError ('--capflag InitiateReset also requires --capflag PersistAcrossReset')
                if args.CapsuleOemFlag > 0xFFFF:
                    raise argparse.ArgumentTypeError ('--capoemflag must be an integer between 0x0000 and 0xffff')

            return True


    def Encode (PayloadDescriptorList, EmbeddedDriverDescriptorList, Buffer):
        if args.JsonFile:
            CheckArgumentConflict(args)
            try:
                Json = json.loads (args.JsonFile.read ())
            except:
                print ('GenerateCapsule: error: {JSONFile} loads failure. '.format (JSONFile = args.JsonFile))
                sys.exit (1)
            EncodeJsonFileParse(Json)
        else:
            for Driver in args.EmbeddedDriver:
                EmbeddedDriverDescriptorList.append (Driver.read())
            PayloadDescriptorList.append (PayloadDescriptor (
                                            Buffer,
                                            args.Guid,
                                            args.FwVersion,
                                            args.LowestSupportedVersion,
                                            args.MonotonicCount,
                                            args.HardwareInstance,
                                            args.UpdateImageIndex,
                                            args.SignToolPfxFile,
                                            args.OpenSslSignerPrivateCertFile,
                                            args.OpenSslOtherPublicCertFile,
                                            args.OpenSslTrustedPublicCertFile,
                                            args.SigningToolPath,
                                            None
                                            ))
        for SinglePayloadDescriptor in PayloadDescriptorList:
            try:
                SinglePayloadDescriptor.Validate (args)
            except Exception as Msg:
                print ('GenerateCapsule: error:' + str(Msg))
                sys.exit (1)
        for SinglePayloadDescriptor in PayloadDescriptorList:
            Result = SinglePayloadDescriptor.Payload
            try:
                FmpPayloadHeader.FwVersion              = SinglePayloadDescriptor.FwVersion
                FmpPayloadHeader.LowestSupportedVersion = SinglePayloadDescriptor.LowestSupportedVersion
                FmpPayloadHeader.Payload                = SinglePayloadDescriptor.Payload
                Result = FmpPayloadHeader.Encode ()
                if args.Verbose:
                    FmpPayloadHeader.DumpInfo ()
            except:
                print ('GenerateCapsule: error: can not encode FMP Payload Header')
                sys.exit (1)
            if SinglePayloadDescriptor.UseDependency:
                CapsuleDependency.Payload = Result
                CapsuleDependency.DepexExp = SinglePayloadDescriptor.DepexExp
                Result = CapsuleDependency.Encode ()
                if args.Verbose:
                    CapsuleDependency.DumpInfo ()
            if SinglePayloadDescriptor.UseOpenSsl or SinglePayloadDescriptor.UseSignTool:
                #
                # Sign image with 64-bit MonotonicCount appended to end of image
                #
                try:
                    if SinglePayloadDescriptor.UseSignTool:
                        CertData = SignPayloadSignTool (
                            Result + struct.pack ('<Q', SinglePayloadDescriptor.MonotonicCount),
                            SinglePayloadDescriptor.SigningToolPath,
                            SinglePayloadDescriptor.SignToolPfxFile,
                            Verbose = args.Verbose
                        )
                    else:
                        CertData = SignPayloadOpenSsl (
                            Result + struct.pack ('<Q', SinglePayloadDescriptor.MonotonicCount),
                            SinglePayloadDescriptor.SigningToolPath,
                            SinglePayloadDescriptor.OpenSslSignerPrivateCertFile,
                            SinglePayloadDescriptor.OpenSslOtherPublicCertFile,
                            SinglePayloadDescriptor.OpenSslTrustedPublicCertFile,
                            Verbose = args.Verbose
                        )
                except Exception as Msg:
                    print ('GenerateCapsule: error: can not sign payload \n' + str(Msg))
                    sys.exit (1)

                try:
                    FmpAuthHeader.MonotonicCount = SinglePayloadDescriptor.MonotonicCount
                    FmpAuthHeader.CertData       = CertData
                    FmpAuthHeader.Payload        = Result
                    Result = FmpAuthHeader.Encode ()
                    if args.Verbose:
                        FmpAuthHeader.DumpInfo ()
                except:
                    print ('GenerateCapsule: error: can not encode FMP Auth Header')
                    sys.exit (1)
            FmpCapsuleHeader.AddPayload (SinglePayloadDescriptor.Guid, Result, HardwareInstance = SinglePayloadDescriptor.HardwareInstance, UpdateImageIndex = SinglePayloadDescriptor.UpdateImageIndex)
        try:
            for EmbeddedDriver in EmbeddedDriverDescriptorList:
                FmpCapsuleHeader.AddEmbeddedDriver(EmbeddedDriver)

            Result = FmpCapsuleHeader.Encode ()
            if args.Verbose:
                FmpCapsuleHeader.DumpInfo ()
        except:
            print ('GenerateCapsule: error: can not encode FMP Capsule Header')
            sys.exit (1)

        try:
            UefiCapsuleHeader.OemFlags            = args.CapsuleOemFlag
            UefiCapsuleHeader.PersistAcrossReset  = 'PersistAcrossReset'  in args.CapsuleFlag
            UefiCapsuleHeader.PopulateSystemTable = False
            UefiCapsuleHeader.InitiateReset       = 'InitiateReset'       in args.CapsuleFlag
            UefiCapsuleHeader.Payload             = Result
            Result = UefiCapsuleHeader.Encode ()
            if args.Verbose:
                UefiCapsuleHeader.DumpInfo ()
        except:
            print ('GenerateCapsule: error: can not encode UEFI Capsule Header')
            sys.exit (1)
        try:
            if args.Verbose:
                print ('Write binary output file {File}'.format (File = args.OutputFile.name))
            args.OutputFile.write (Result)
            args.OutputFile.close ()
        except:
            print ('GenerateCapsule: error: can not write binary output file {File}'.format (File = args.OutputFile.name))
            sys.exit (1)

    def Decode (PayloadDescriptorList, PayloadJsonDescriptorList, Buffer):
        if args.JsonFile:
            CheckArgumentConflict(args)
        #
        # Parse payload descriptors from JSON
        #
            try:
                Json = json.loads (args.JsonFile.read())
            except:
                print ('GenerateCapsule: error: {JSONFile} loads failure. '.format (JSONFile = args.JsonFile))
                sys.exit (1)
            DecodeJsonFileParse (Json)
        else:
            PayloadDescriptorList.append (PayloadDescriptor (
                                            Buffer,
                                            args.Guid,
                                            args.FwVersion,
                                            args.LowestSupportedVersion,
                                            args.MonotonicCount,
                                            args.HardwareInstance,
                                            args.UpdateImageIndex,
                                            args.SignToolPfxFile,
                                            args.OpenSslSignerPrivateCertFile,
                                            args.OpenSslOtherPublicCertFile,
                                            args.OpenSslTrustedPublicCertFile,
                                            args.SigningToolPath,
                                            None
                                            ))
        #
        # Perform additional verification on payload descriptors
        #
        for SinglePayloadDescriptor in PayloadDescriptorList:
            try:
                SinglePayloadDescriptor.Validate (args)
            except Exception as Msg:
                print ('GenerateCapsule: error:' + str(Msg))
                sys.exit (1)
        try:
            Result = UefiCapsuleHeader.Decode (Buffer)
            if len (Result) > 0:
                Result = FmpCapsuleHeader.Decode (Result)
                if args.JsonFile:
                    if FmpCapsuleHeader.PayloadItemCount != len (PayloadDescriptorList):
                        CapsulePayloadNum = FmpCapsuleHeader.PayloadItemCount
                        JsonPayloadNum = len (PayloadDescriptorList)
                        print ('GenerateCapsule: Decode error: {JsonPayloadNumber} payloads in JSON file {File} and {CapsulePayloadNumber} payloads in Capsule {CapsuleName}'.format (JsonPayloadNumber = JsonPayloadNum, File = args.JsonFile.name, CapsulePayloadNumber = CapsulePayloadNum, CapsuleName = args.InputFile.name))
                        sys.exit (1)
                    for Index in range (0, FmpCapsuleHeader.PayloadItemCount):
                        if Index < len (PayloadDescriptorList):
                            GUID = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateImageTypeId
                            HardwareInstance = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateHardwareInstance
                            UpdateImageIndex = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateImageIndex
                            if PayloadDescriptorList[Index].Guid != GUID or PayloadDescriptorList[Index].HardwareInstance != HardwareInstance:
                                print ('GenerateCapsule: Decode error: Guid or HardwareInstance pair in input JSON file {File} does not match the payload {PayloadIndex} in Capsule {InputCapsule}'.format (File = args.JsonFile.name, PayloadIndex = Index + 1, InputCapsule = args.InputFile.name))
                                sys.exit (1)
                            PayloadDescriptorList[Index].Payload = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).Payload
                            DecodeJsonOutput = args.OutputFile.name + '.Payload.{Index:d}.bin'.format (Index = Index + 1)
                            PayloadJsonDescriptorList.append (PayloadDescriptor (
                                                                DecodeJsonOutput,
                                                                GUID,
                                                                None,
                                                                None,
                                                                None,
                                                                HardwareInstance,
                                                                UpdateImageIndex,
                                                                PayloadDescriptorList[Index].SignToolPfxFile,
                                                                PayloadDescriptorList[Index].OpenSslSignerPrivateCertFile,
                                                                PayloadDescriptorList[Index].OpenSslOtherPublicCertFile,
                                                                PayloadDescriptorList[Index].OpenSslTrustedPublicCertFile,
                                                                PayloadDescriptorList[Index].SigningToolPath,
                                                                None
                                                                ))
                else:
                    PayloadDescriptorList[0].Payload = FmpCapsuleHeader.GetFmpCapsuleImageHeader (0).Payload
                    for Index in range (0, FmpCapsuleHeader.PayloadItemCount):
                        if Index > 0:
                            PayloadDecodeFile = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).Payload
                            PayloadDescriptorList.append (PayloadDescriptor (PayloadDecodeFile,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None,
                                                            None
                                                            ))
                        GUID = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateImageTypeId
                        HardwareInstance = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateHardwareInstance
                        UpdateImageIndex = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).UpdateImageIndex
                        DecodeJsonOutput = args.OutputFile.name + '.Payload.{Index:d}.bin'.format (Index = Index + 1)
                        PayloadJsonDescriptorList.append (PayloadDescriptor (
                                                            DecodeJsonOutput,
                                                            GUID,
                                                            None,
                                                            None,
                                                            None,
                                                            HardwareInstance,
                                                            UpdateImageIndex,
                                                            PayloadDescriptorList[Index].SignToolPfxFile,
                                                            PayloadDescriptorList[Index].OpenSslSignerPrivateCertFile,
                                                            PayloadDescriptorList[Index].OpenSslOtherPublicCertFile,
                                                            PayloadDescriptorList[Index].OpenSslTrustedPublicCertFile,
                                                            PayloadDescriptorList[Index].SigningToolPath,
                                                            None
                                                            ))
                JsonIndex = 0
                for SinglePayloadDescriptor in PayloadDescriptorList:
                    if args.Verbose:
                        print ('========')
                        UefiCapsuleHeader.DumpInfo ()
                        print ('--------')
                        FmpCapsuleHeader.DumpInfo ()
                    if FmpAuthHeader.IsSigned(SinglePayloadDescriptor.Payload):
                        if not SinglePayloadDescriptor.UseOpenSsl and not SinglePayloadDescriptor.UseSignTool:
                            print ('GenerateCapsule: decode warning: can not verify singed payload without cert or pfx file. Index = {Index}'.format (Index = JsonIndex + 1))
                        SinglePayloadDescriptor.Payload = FmpAuthHeader.Decode (SinglePayloadDescriptor.Payload)
                        PayloadJsonDescriptorList[JsonIndex].MonotonicCount = FmpAuthHeader.MonotonicCount
                        if args.Verbose:
                            print ('--------')
                            FmpAuthHeader.DumpInfo ()

                        #
                        # Verify Image with 64-bit MonotonicCount appended to end of image
                        #
                        try:
                          if SinglePayloadDescriptor.UseSignTool:
                              CertData = VerifyPayloadSignTool (
                                           FmpAuthHeader.Payload + struct.pack ('<Q', FmpAuthHeader.MonotonicCount),
                                           FmpAuthHeader.CertData,
                                           SinglePayloadDescriptor.SigningToolPath,
                                           SinglePayloadDescriptor.SignToolPfxFile,
                                           Verbose = args.Verbose
                                           )
                          else:
                              CertData = VerifyPayloadOpenSsl (
                                           FmpAuthHeader.Payload + struct.pack ('<Q', FmpAuthHeader.MonotonicCount),
                                           FmpAuthHeader.CertData,
                                           SinglePayloadDescriptor.SigningToolPath,
                                           SinglePayloadDescriptor.OpenSslSignerPrivateCertFile,
                                           SinglePayloadDescriptor.OpenSslOtherPublicCertFile,
                                           SinglePayloadDescriptor.OpenSslTrustedPublicCertFile,
                                           Verbose = args.Verbose
                                           )
                        except Exception as Msg:
                            print ('GenerateCapsule: warning: payload verification failed Index = {Index} \n'.format (Index = JsonIndex + 1) + str(Msg))
                    else:
                        if args.Verbose:
                            print ('--------')
                            print ('No EFI_FIRMWARE_IMAGE_AUTHENTICATION')

                    PayloadSignature = struct.unpack ('<I', SinglePayloadDescriptor.Payload[0:4])
                    if PayloadSignature != FmpPayloadHeader.Signature:
                        SinglePayloadDescriptor.UseDependency = True
                        try:
                            SinglePayloadDescriptor.Payload = CapsuleDependency.Decode (SinglePayloadDescriptor.Payload)
                            PayloadJsonDescriptorList[JsonIndex].DepexExp = CapsuleDependency.DepexExp
                            if args.Verbose:
                                print ('--------')
                                CapsuleDependency.DumpInfo ()
                        except Exception as Msg:
                            print ('GenerateCapsule: error: invalid dependency expression')
                    else:
                        if args.Verbose:
                            print ('--------')
                            print ('No EFI_FIRMWARE_IMAGE_DEP')

                    try:
                        SinglePayloadDescriptor.Payload = FmpPayloadHeader.Decode (SinglePayloadDescriptor.Payload)
                        PayloadJsonDescriptorList[JsonIndex].FwVersion = FmpPayloadHeader.FwVersion
                        PayloadJsonDescriptorList[JsonIndex].LowestSupportedVersion = FmpPayloadHeader.LowestSupportedVersion
                        JsonIndex = JsonIndex + 1
                        if args.Verbose:
                            print ('--------')
                            FmpPayloadHeader.DumpInfo ()
                            print ('========')
                    except:
                        if args.Verbose:
                            print ('--------')
                            print ('No FMP_PAYLOAD_HEADER')
                            print ('========')
                        sys.exit (1)
                #
                # Write embedded driver file(s)
                #
                for Index in range (0, FmpCapsuleHeader.EmbeddedDriverCount):
                    EmbeddedDriverBuffer = FmpCapsuleHeader.GetEmbeddedDriver (Index)
                    EmbeddedDriverPath = args.OutputFile.name + '.EmbeddedDriver.{Index:d}.efi'.format (Index = Index + 1)
                    try:
                        if args.Verbose:
                            print ('Write embedded driver file {File}'.format (File = EmbeddedDriverPath))
                        with open (EmbeddedDriverPath, 'wb') as EmbeddedDriverFile:
                            EmbeddedDriverFile.write (EmbeddedDriverBuffer)
                    except:
                        print ('GenerateCapsule: error: can not write embedded driver file {File}'.format (File = EmbeddedDriverPath))
                        sys.exit (1)

        except:
            print ('GenerateCapsule: error: can not decode capsule')
            sys.exit (1)
        GenerateOutputJson(PayloadJsonDescriptorList)
        PayloadIndex = 0
        for SinglePayloadDescriptor in PayloadDescriptorList:
            if args.OutputFile is None:
                print ('GenerateCapsule: Decode error: OutputFile is needed for decode output')
                sys.exit (1)
            try:
                if args.Verbose:
                    print ('Write binary output file {File}'.format (File = args.OutputFile.name))
                PayloadDecodePath = args.OutputFile.name + '.Payload.{Index:d}.bin'.format (Index = PayloadIndex + 1)
                with open (PayloadDecodePath, 'wb') as PayloadDecodeFile:
                    PayloadDecodeFile.write (SinglePayloadDescriptor.Payload)
                PayloadIndex = PayloadIndex + 1
            except:
                print ('GenerateCapsule: error: can not write binary output file {File}'.format (File = SinglePayloadDescriptor.OutputFile.name))
                sys.exit (1)

    def DumpInfo (Buffer, args):
        if args.OutputFile is not None:
            raise argparse.ArgumentTypeError ('the following option is not supported for dumpinfo operations: --output')
        try:
            Result = UefiCapsuleHeader.Decode (Buffer)
            print ('========')
            UefiCapsuleHeader.DumpInfo ()
            if len (Result) > 0:
                FmpCapsuleHeader.Decode (Result)
                print ('--------')
                FmpCapsuleHeader.DumpInfo ()
                for Index in range (0, FmpCapsuleHeader.PayloadItemCount):
                    Result = FmpCapsuleHeader.GetFmpCapsuleImageHeader (Index).Payload
                    try:
                        Result = FmpAuthHeader.Decode (Result)
                        print ('--------')
                        FmpAuthHeader.DumpInfo ()
                    except:
                        print ('--------')
                        print ('No EFI_FIRMWARE_IMAGE_AUTHENTICATION')

                    PayloadSignature = struct.unpack ('<I', Result[0:4])
                    if PayloadSignature != FmpPayloadHeader.Signature:
                        try:
                            Result = CapsuleDependency.Decode (Result)
                            print ('--------')
                            CapsuleDependency.DumpInfo ()
                        except:
                            print ('GenerateCapsule: error: invalid dependency expression')
                    else:
                        print ('--------')
                        print ('No EFI_FIRMWARE_IMAGE_DEP')
                    try:
                        Result = FmpPayloadHeader.Decode (Result)
                        print ('--------')
                        FmpPayloadHeader.DumpInfo ()
                    except:
                        print ('--------')
                        print ('No FMP_PAYLOAD_HEADER')
                    print ('========')
        except:
            print ('GenerateCapsule: error: can not decode capsule')
            sys.exit (1)
    #
    # Create command line argument parser object
    #
    parser = argparse.ArgumentParser (
                        prog = __prog__,
                        description = __description__ + __copyright__,
                        conflict_handler = 'resolve',
                        fromfile_prefix_chars = '@'
                        )
    parser.convert_arg_line_to_args = convert_arg_line_to_args

    #
    # Add input and output file arguments
    #
    parser.add_argument("InputFile",  type = argparse.FileType('rb'), nargs='?',
                        help = "Input binary payload filename.")
    parser.add_argument("-o", "--output", dest = 'OutputFile', type = argparse.FileType('wb'),
                        help = "Output filename.")
    #
    # Add group for -e and -d flags that are mutually exclusive and required
    #
    group = parser.add_mutually_exclusive_group (required = True)
    group.add_argument ("-e", "--encode", dest = 'Encode', action = "store_true",
                        help = "Encode file")
    group.add_argument ("-d", "--decode", dest = 'Decode', action = "store_true",
                        help = "Decode file")
    group.add_argument ("--dump-info", dest = 'DumpInfo', action = "store_true",
                        help = "Display FMP Payload Header information")
    #
    # Add optional arguments for this command
    #
    parser.add_argument ("-j", "--json-file", dest = 'JsonFile', type=argparse.FileType('r'),
                         help = "JSON configuration file for multiple payloads and embedded drivers.")
    parser.add_argument ("--capflag", dest = 'CapsuleFlag', action='append', default = [],
                         choices=['PersistAcrossReset', 'InitiateReset'],
                         help = "Capsule flag can be PersistAcrossReset or InitiateReset or not set")
    parser.add_argument ("--capoemflag", dest = 'CapsuleOemFlag', type = ValidateUnsignedInteger, default = 0x0000,
                         help = "Capsule OEM Flag is an integer between 0x0000 and 0xffff.")

    parser.add_argument ("--guid", dest = 'Guid', type = ValidateRegistryFormatGuid,
                         help = "The FMP/ESRT GUID in registry format.  Required for single payload encode operations.")
    parser.add_argument ("--hardware-instance", dest = 'HardwareInstance', type = ValidateUnsignedInteger, default = 0x0000000000000000,
                         help = "The 64-bit hardware instance.  The default is 0x0000000000000000")


    parser.add_argument ("--monotonic-count", dest = 'MonotonicCount', type = ValidateUnsignedInteger, default = 0x0000000000000000,
                         help = "64-bit monotonic count value in header.  Default is 0x0000000000000000.")

    parser.add_argument ("--fw-version", dest = 'FwVersion', type = ValidateUnsignedInteger,
                         help = "The 32-bit version of the binary payload (e.g. 0x11223344 or 5678).  Required for encode operations.")
    parser.add_argument ("--lsv", dest = 'LowestSupportedVersion', type = ValidateUnsignedInteger,
                         help = "The 32-bit lowest supported version of the binary payload (e.g. 0x11223344 or 5678).  Required for encode operations.")

    parser.add_argument ("--pfx-file", dest='SignToolPfxFile', type=argparse.FileType('rb'),
                         help="signtool PFX certificate filename.")

    parser.add_argument ("--signer-private-cert", dest='OpenSslSignerPrivateCertFile', type=argparse.FileType('rb'),
                         help="OpenSSL signer private certificate filename.")
    parser.add_argument ("--other-public-cert", dest='OpenSslOtherPublicCertFile', type=argparse.FileType('rb'),
                         help="OpenSSL other public certificate filename.")
    parser.add_argument ("--trusted-public-cert", dest='OpenSslTrustedPublicCertFile', type=argparse.FileType('rb'),
                         help="OpenSSL trusted public certificate filename.")

    parser.add_argument ("--signing-tool-path", dest = 'SigningToolPath',
                         help = "Path to signtool or OpenSSL tool.  Optional if path to tools are already in PATH.")

    parser.add_argument ("--embedded-driver", dest = 'EmbeddedDriver', type = argparse.FileType('rb'), action='append', default = [],
                         help = "Path to embedded UEFI driver to add to capsule.")

    #
    # Add optional arguments common to all operations
    #
    parser.add_argument ('--version', action='version', version='%(prog)s ' + __version__)
    parser.add_argument ("-v", "--verbose", dest = 'Verbose', action = "store_true",
                         help = "Turn on verbose output with informational messages printed, including capsule headers and warning messages.")
    parser.add_argument ("-q", "--quiet", dest = 'Quiet', action = "store_true",
                         help = "Disable all messages except fatal errors.")
    parser.add_argument ("--debug", dest = 'Debug', type = int, metavar = '[0-9]', choices = range (0, 10), default = 0,
                         help = "Set debug level")
    parser.add_argument ("--update-image-index", dest = 'UpdateImageIndex', type = ValidateUnsignedInteger, default = 0x01, help = "unique number identifying the firmware image within the device ")

    #
    # Parse command line arguments
    #
    args = parser.parse_args()

    #
    # Read binary input file
    #
    Buffer = ''
    if args.InputFile:
        if os.path.getsize (args.InputFile.name) == 0:
            print ('GenerateCapsule: error: InputFile {File} is empty'.format (File = args.InputFile.name))
            sys.exit (1)
        try:
            if args.Verbose:
                print ('Read binary input file {File}'.format (File = args.InputFile.name))
            Buffer = args.InputFile.read ()
            args.InputFile.close ()
        except:
            print ('GenerateCapsule: error: can not read binary input file {File}'.format (File = args.InputFile.name))
            sys.exit (1)

    #
    # Create objects
    #
    UefiCapsuleHeader = UefiCapsuleHeaderClass ()
    FmpCapsuleHeader  = FmpCapsuleHeaderClass ()
    FmpAuthHeader     = FmpAuthHeaderClass ()
    FmpPayloadHeader  = FmpPayloadHeaderClass ()
    CapsuleDependency = CapsuleDependencyClass ()

    EmbeddedDriverDescriptorList = []
    PayloadDescriptorList = []
    PayloadJsonDescriptorList = []

    #
    #Encode Operation
    #
    if args.Encode:
        Encode (PayloadDescriptorList, EmbeddedDriverDescriptorList, Buffer)

    #
    #Decode Operation
    #
    if args.Decode:
        Decode (PayloadDescriptorList, PayloadJsonDescriptorList, Buffer)

    #
    #Dump Info Operation
    #
    if args.DumpInfo:
        DumpInfo (Buffer, args)

    if args.Verbose:
        print('Success')
